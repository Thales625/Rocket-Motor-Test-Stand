#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "http_server.h"
#include "websocket.h"

#include "circular_buffer.h"

static const char *TAG = "main-app";

static circular_buffer_t buffer;
static circular_reader_t sd_reader;
static circular_reader_t websocket_reader;

void sensor_task(void *arg) {
	uint32_t value = 1;
	while (1) {
		// uint32_t value = read_sensor(); // real sensor reading
		circular_buffer_write(&buffer, value++);
	
		if (value > 1000) value = 0; // linear value generation

		vTaskDelay(pdMS_TO_TICKS(300));
	}
}

void sd_task(void *arg) {
	uint32_t value;
	while (1) {
		while (circular_buffer_read(&buffer, &sd_reader, &value)) {
			// save_to_sd(value); // real save to sd method
            ESP_LOGI(TAG, "Send to sd %ld", value);
		}
		vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void websocket_task(void *arg) {
	uint32_t value;

	while (1) {
		while (circular_buffer_read(&buffer, &websocket_reader, &value)) {
            char text[WS_BUFFER_SIZE];
            snprintf(text, sizeof(text), "I(%lld): %ld", esp_timer_get_time() / 1000, value);
            websocket_broadcast(text);
		}
		vTaskDelay(pdMS_TO_TICKS(250));
	}
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_softap();
    start_http_server();

    circular_buffer_init(&buffer);
	sd_reader.read_index = 0;
	websocket_reader.read_index = 0;

    xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
	xTaskCreate(sd_task, "sd_task", 4096, NULL, 4, NULL);
	xTaskCreate(websocket_task, "websocket_task", 4096, NULL, 4, NULL);
}