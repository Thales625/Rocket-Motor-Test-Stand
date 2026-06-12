#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <dirent.h>

#include "sdcard.h"
#include "hx711.h"

// PINOUT
#define SD_MOSI_PIN GPIO_NUM_33
#define SD_MISO_PIN GPIO_NUM_26
#define SD_SCLK_PIN GPIO_NUM_25
#define SD_CS_PIN GPIO_NUM_32

#define HX711_DOUT_PIN GPIO_NUM_17
#define HX711_SCK_PIN GPIO_NUM_16

#define BUTTON_PIN GPIO_NUM_2
#define LED_PIN GPIO_NUM_21

// #define LIST_FILES
// #define READ_DATA

static const char *TAG = "main";

static FILE *file_ptr;
static char filename[32];

static hx711_t hx711 = {
    .dout = HX711_DOUT_PIN,
    .pd_sck = HX711_SCK_PIN,
    .gain = HX711_GAIN_A_64
};

static void blink(uint32_t duration) {
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(duration));
    gpio_set_level(LED_PIN, 0);
}

static void avionics_abort(int code) {
    // set safe state
    gpio_set_level(LED_PIN, 0);

    // log
    ESP_LOGW(TAG, "ABORT! code: %d", code);

    // abort loop
    while (1) {
        for (int i=0; i<code; i++) {
            blink(100);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int get_next_log_number(void) {
    DIR *dir;
    struct dirent *entry;
    int max_num = 0;

    dir = opendir("/sdcard");
    if (!dir) {
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        int num;

        if (sscanf(entry->d_name, "LOG%d.TXT", &num) == 1) {
            if (num > max_num) {
                max_num = num;
            }
        }
    }

    closedir(dir);

    return max_num + 1;
}

void capture_save_data(void *arg) {
    int64_t ut;

    char text[128];
    int32_t data;
    esp_err_t err;

    const TickType_t debounce_time = pdMS_TO_TICKS(500);
	TickType_t button_press_start = 0;

    gpio_set_level(LED_PIN, 1);

    while (1) {
        // button debounce
        if (!gpio_get_level(BUTTON_PIN)) {
			if (button_press_start == 0) {
				button_press_start = xTaskGetTickCount();
			} else if ((xTaskGetTickCount() - button_press_start) >= debounce_time) {
				break;
			}
		} else {
			button_press_start = 0;
		}

        // wait hx711 collect data
        err = hx711_wait(&hx711, 500);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Device not found: %d (%s)\n", err, esp_err_to_name(err));
            continue;
        }

        ut = esp_timer_get_time();

        // read raw data
        err = hx711_read_data(&hx711, &data);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not read data: %d (%s)\n", err, esp_err_to_name(err));
            continue;
        }

        // write sdcard
        snprintf(text, sizeof(text), "%lld %" PRId32 "", ut / 1000, data);
        sdcard_write(text, file_ptr);

        printf("%" PRIi32 "\n", data);
    }

    sdcard_close_file(file_ptr);
    sdcard_umount();

    gpio_set_level(LED_PIN, 0);

    ESP_LOGI(TAG, "Finished");

    vTaskDelete(NULL);
}

void app_main(void) {
    // configure GPIO
    gpio_config_t io_conf;

    // LED
    io_conf.pin_bit_mask = BIT64(LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // Push Button
    io_conf.pin_bit_mask = BIT64(BUTTON_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // initialize hx711
    if (hx711_init(&hx711) != ESP_OK) {
        ESP_LOGE(TAG, "HX711 failed to init");
        avionics_abort(1);
        return;
    }
    ESP_LOGI(TAG, "HX711 initialized");

    // initialize sd card
    if (sdcard_init(SD_MOSI_PIN, SD_MISO_PIN, SD_SCLK_PIN, SD_CS_PIN) != ESP_OK) {
        ESP_LOGE(TAG, "SDCard failed to init");
        avionics_abort(2);
        return;
    }
    ESP_LOGI(TAG, "SDCard initialized");

    #ifdef LIST_FILES
    sdcard_list_files();
    sdcard_umount();
    return;
    #endif

    #ifdef READ_DATA
    strcpy(filename, "LOG1.TXT");
    sdcard_read_file(filename);
    sdcard_umount();
    return;
    #endif

    // generate next filename
    sprintf(filename, "LOG%d.TXT", get_next_log_number());

    gpio_set_level(LED_PIN, 1);

    ESP_LOGI(TAG, "Press the button to start data capture");

    while (gpio_get_level(BUTTON_PIN)) vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Release the button");

    while (!gpio_get_level(BUTTON_PIN)) vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Starting data capture and save in %s", filename);

    gpio_set_level(LED_PIN, 0);

    if (sdcard_open_file(filename, "w", &file_ptr) != ESP_OK) {
        ESP_LOGE(TAG, "SDCard: Failed to open/create file");
        sdcard_umount();
        avionics_abort(3);
        return;
    }

    xTaskCreate(capture_save_data, "capture_save_data", configMINIMAL_STACK_SIZE * 5, NULL, 10, NULL);
}