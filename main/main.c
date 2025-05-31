#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sdcard.h"
#include "hx711.h"

static const char *TAG = "main";

static FILE *file_ptr;

static hx711_t hx711 = {
	.dout = GPIO_NUM_33,
	.pd_sck = GPIO_NUM_32,
	.gain = HX711_GAIN_A_64
};

#define MAX_RUNTIME 50e6

#define DEBUG
// #define READ_DATA

void capture_data(void *arg) {
	int64_t ut0 = esp_timer_get_time();
	int64_t ut;
	
	char text[128];
	int32_t data;
	esp_err_t r;
	while (1) {
		r = hx711_wait(&hx711, 500);
		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}

		#ifndef DEBUG
		ut = esp_timer_get_time();
		if ((ut - ut0) > MAX_RUNTIME) break;
		#endif

		// read raw data
		r = hx711_read_data(&hx711, &data);
		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}

		#ifndef DEBUG
		// write sdcard
		snprintf(text, sizeof(text), "%lld %" PRId32 "", ut / 1000, data);
		sdcard_write(text, file_ptr);
		#endif

		// print raw data
		printf("%" PRIi32 "\n", data);

		vTaskDelay(pdMS_TO_TICKS(100));
	}

	#ifndef DEBUG
	sdcard_close_file(file_ptr);
	sdcard_umount();
	#endif

	vTaskDelete(NULL);
}

void app_main(void) {
	#ifndef DEBUG
	// initialize sd card
	if (sdcard_init() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SD card");
		return;
	}
	
	#ifdef READ_DATA
	sdcard_read_file("test.txt");
	sdcard_umount();
	return;
	#endif
	
	vTaskDelay(pdMS_TO_TICKS(5000));
	
	if (sdcard_open_file("test.txt", "w", &file_ptr) != ESP_OK) {
		sdcard_umount();
		ESP_LOGE(TAG, "Failed to open/create file");
		return;
	}
	#endif

	// initialize hx711
	ESP_ERROR_CHECK(hx711_init(&hx711));

	xTaskCreate(capture_data, "capture_data", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}