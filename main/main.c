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

#define MAX_RUNTIME 50e6

static circular_buffer_t buffer;
static circular_reader_t sd_reader;
static circular_reader_t websocket_reader;

static hx711_t hx711;

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool running;

bool is_running() {
	portENTER_CRITICAL(&mux);
	bool r = running;
	portEXIT_CRITICAL(&mux);
	return r;
}

void sensor_task(void *arg) {
	int64_t ut0 = esp_timer_get_time();
	
	int32_t data;
	while (is_running()) {
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

	portENTER_CRITICAL(&mux);
	running = false;
	portEXIT_CRITICAL(&mux);

	ESP_LOGI(TAG, "STOP EXECUTION");

	vTaskDelete(NULL);
}

void sd_task(void *arg) {
	int32_t value;
	char text[WS_BUFFER_SIZE];
	while (is_running()) {
		while (circular_buffer_read(&buffer, &sd_reader, &value)) {
			snprintf(text, sizeof(text), "I(%lld): %" PRId32 "", esp_timer_get_time() / 1000, value);

			sdcard_write(text);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	sdcard_close_file();
	vTaskDelay(pdMS_TO_TICKS(200));
	sdcard_umount();

	vTaskDelete(NULL);
}

void websocket_task(void *arg) {
	int32_t value;
	char text[WS_BUFFER_SIZE];
	while (is_running()) {
		while (circular_buffer_read(&buffer, &websocket_reader, &value)) {
			snprintf(text, sizeof(text), "I(%lld): %" PRId32 "", esp_timer_get_time() / 1000, value);
			websocket_broadcast(text);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	// TODO: close websocket and http server

	vTaskDelete(NULL);
}

void app_main(void) {
	running = 1;

	// initialize sd card
	esp_err_t ret = sdcard_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SD card");
		return;
	}

	vTaskDelay(pdMS_TO_TICKS(5000));

	ESP_LOGI(TAG, "Init");

	ret = sdcard_open_file("test.txt", "w");
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open/create file");
		return;
	}

	// initialize hx711
	hx711 = (hx711_t) {
		.dout = GPIO_NUM_33,
		.pd_sck = GPIO_NUM_32,
		.gain = HX711_GAIN_A_64
	};

	ESP_ERROR_CHECK(hx711_init(&hx711));

	// initialize http server
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
	xTaskCreate(websocket_task, "websocket_task", 4096, NULL, 4, NULL);
}