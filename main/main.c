#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <hx711.h>

static const char *TAG = "hx711";

#define AVG_TIMES 5

void test(void *pvParameters) {
	hx711_t dev = {
		.dout = GPIO_NUM_18,
		.pd_sck = GPIO_NUM_19,
		.gain = HX711_GAIN_A_64
	};

	// initialize device
	ESP_ERROR_CHECK(hx711_init(&dev));

	// read from device
	while (1) {
		esp_err_t r = hx711_wait(&dev, 500);

		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}

		int32_t data;

		// read raw data
		r = hx711_read_data(&dev, &data);
		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}
		
		// read average data
		/*
		r = hx711_read_average(&dev, AVG_TIMES, &data);
		if (r != ESP_OK) {
			ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
			continue;
		}
		*/

		// ESP_LOGI(TAG, "Raw data: %" PRIi32, data);
		printf("%" PRIi32 "\n", data); // print raw data for python(tools/main.py) to read and plot

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void app_main() {
	xTaskCreate(test, "test", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}