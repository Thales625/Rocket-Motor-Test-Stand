#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "circular_buffer.h"
#include "wifi.h"
#include "http_server.h"
#include "websocket.h"
#include "sdcard.h"
#include "hx711.h"

static const char *TAG = "main";

#define MAX_RUNTIME 20e6

static circular_buffer_t buffer;
static circular_reader_t sd_reader;
static circular_reader_t websocket_reader;

static FILE *file_ptr;

static hx711_t hx711 = {
	.dout = GPIO_NUM_33,
	.pd_sck = GPIO_NUM_32,
	.gain = HX711_GAIN_A_64
};

static portMUX_TYPE running_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool running;

static bool get_running() {
	portENTER_CRITICAL(&running_mux);
	bool r = running;
	portEXIT_CRITICAL(&running_mux);
	return r;
}

static void set_running(bool value) {
	portENTER_CRITICAL(&running_mux);
	running = value;
	portEXIT_CRITICAL(&running_mux);
}

void sensor_task(void *arg) {
	int64_t ut0 = esp_timer_get_time();
	
	int32_t data;
	while (get_running()) {
		esp_err_t r = hx711_wait(&hx711, 500);

		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}

		// read raw data
		r = hx711_read_data(&hx711, &data);
		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}

		circular_buffer_write(&buffer, data);
		
		if ((esp_timer_get_time() - ut0) > MAX_RUNTIME) break;

		vTaskDelay(pdMS_TO_TICKS(100));
	}

	set_running(false);

	ESP_LOGI(TAG, "STOP EXECUTION");

	vTaskDelete(NULL);
}

void sd_task(void *arg) {
	int32_t value;
	char text[WS_BUFFER_SIZE];
	while (get_running()) {
		while (circular_buffer_read(&buffer, &sd_reader, &value)) {
			snprintf(text, sizeof(text), "I(%lld): %" PRId32 "", esp_timer_get_time() / 1000, value);

			sdcard_write(text, file_ptr);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	sdcard_close_file(file_ptr);
	sdcard_umount();
	vTaskDelete(NULL);
}

void websocket_task(void *arg) {
	int32_t value;
	char text[WS_BUFFER_SIZE];
	while (get_running()) {
		while (circular_buffer_read(&buffer, &websocket_reader, &value)) {
			snprintf(text, sizeof(text), "I(%lld): %" PRId32 "", esp_timer_get_time() / 1000, value);
			websocket_broadcast(text);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	websocket_stop();
	stop_http_server();
	wifi_stop_softap();
	vTaskDelete(NULL);
}

// #define DEBUG

void app_main(void) {
	running = 1;

	vTaskDelay(pdMS_TO_TICKS(5000));

	// initialize sd card
	if (sdcard_init() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SD card");
		return;
	}

	#ifdef DEBUG
	sdcard_read_file("test.txt");
	sdcard_umount();
	return;
	#endif

	if (sdcard_open_file("test.txt", "w", &file_ptr) != ESP_OK) {
		sdcard_umount();
		ESP_LOGE(TAG, "Failed to open/create file");
		return;
	}

	// initialize hx711
	ESP_ERROR_CHECK(hx711_init(&hx711));

	// initialize wifi and http server
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_softap();
	start_http_server();

	// circular buffer
	circular_buffer_init(&buffer);
	sd_reader.read_index = 0;
	websocket_reader.read_index = 0;

	// tasks
	xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
	xTaskCreate(sd_task, "sd_task", 4096, NULL, 4, NULL);
	xTaskCreate(websocket_task, "websocket_task", 4096, NULL, 3, NULL);
}