#include "wifi.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";

#define WIFI_SSID "ESP32-NETWORK"
#define WIFI_PASS "12345678"

void wifi_init_softap(void) {
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {
		.ap = {
			.ssid = WIFI_SSID,
			.ssid_len = strlen(WIFI_SSID),
			.password = WIFI_PASS,
			.max_connection = 4,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};

	if (strlen(WIFI_PASS) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "Wi-Fi AP started. SSID:%s Password:%s", WIFI_SSID, WIFI_PASS);
}

void wifi_stop_softap(void) {
    esp_err_t err;

    err = esp_wifi_stop();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi stopped successfully.");
    } else if (err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "Wi-Fi was not initialized (no stop action needed).");
    } else {
        ESP_LOGE(TAG, "Failed to stop Wi-Fi: %s", esp_err_to_name(err));
    }

    // deinitialize the Wi-Fi driver
    ESP_LOGI(TAG, "Deinitializing the Wi-Fi driver...");
    err = esp_wifi_deinit();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi driver deinitialized successfully.");
    } else if (err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "Wi-Fi driver was not initialized for deinitialization.");
    } else {
        ESP_LOGE(TAG, "Failed to deinitialize Wi-Fi: %s", esp_err_to_name(err));
    }

    // destroy the default SoftAP network interface (netif)
    ESP_LOGI(TAG, "Destroying default Wi-Fi AP network interface...");
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif) {
        esp_netif_destroy_default_wifi(ap_netif);
        ESP_LOGI(TAG, "Default Wi-Fi AP network interface destroyed.");
    } else {
        ESP_LOGI(TAG, "No default Wi-Fi AP network interface found to destroy (it may have already been destroyed or never created).");
    }
}