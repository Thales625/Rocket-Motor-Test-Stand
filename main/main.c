#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sdcard.h"
#include "hx711.h"

// PINOUT
#define SD_MOSI_PIN 33
#define SD_MISO_PIN 26
#define SD_SCLK_PIN 25
#define SD_CS_PIN 32

#define HX711_DOUT_PIN 17
#define HX711_SCK_PIN 16

#define PBUTTON_PIN 2

// #define DEBUG
// #define LIST_FILES
// #define READ_DATA

#define SD_FILE "datalog.txt"

static const char *TAG = "main";

static FILE *file_ptr;

static hx711_t hx711 = {
	.dout = HX711_DOUT_PIN,
	.pd_sck = HX711_SCK_PIN,
	.gain = HX711_GAIN_A_64
};

void capture_save_data(void *arg) {
	int64_t ut;
	
	char text[128];
	int32_t data;
	esp_err_t r;
	while (gpio_get_level(PBUTTON_PIN)) {
		ut = esp_timer_get_time();

		// wait hx711 collect data
		r = hx711_wait(&hx711, 500);
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

		// write sdcard
		snprintf(text, sizeof(text), "%lld %" PRId32 "", ut / 1000, data);
		sdcard_write(text, file_ptr);

		#ifdef DEBUG
		// print raw data
		printf("%" PRIi32 "\n", data);
		#endif
	}

	sdcard_close_file(file_ptr);
	sdcard_umount();

	vTaskDelete(NULL);
}

void app_main(void) {
	// configure GPIO
	gpio_config_t io_conf = {
		.pin_bit_mask = BIT64(PBUTTON_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&io_conf);

	// initialize hx711
	ESP_ERROR_CHECK(hx711_init(&hx711));

	// initialize sd card
	if (sdcard_init(SD_MOSI_PIN, SD_MISO_PIN, SD_SCLK_PIN, SD_CS_PIN) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SD card");
		return;
	}

	#ifdef LIST_FILES
	sdcard_list_files();
	sdcard_umount();
	return;
	#endif
	
	#ifdef READ_DATA
	sdcard_read_file(SD_FILE);
	sdcard_umount();
	return;
	#endif

	#ifdef DEBUG
	ESP_LOGI(TAG, "Press the button to start data capture");
	#endif
	
	while (gpio_get_level(PBUTTON_PIN)) vTaskDelay(pdMS_TO_TICKS(500));

	#ifdef DEBUG
	ESP_LOGI(TAG, "Release the button");
	#endif

	while (!gpio_get_level(PBUTTON_PIN)) vTaskDelay(pdMS_TO_TICKS(500));

	#ifdef DEBUG
	ESP_LOGI(TAG, "Starting data capture and save...");
	#endif
	
	if (sdcard_open_file(SD_FILE, "w", &file_ptr) != ESP_OK) {
		sdcard_umount();
		ESP_LOGE(TAG, "Failed to open/create file");
		return;
	}

	xTaskCreate(capture_save_data, "capture_save_data", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}