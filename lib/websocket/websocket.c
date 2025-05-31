#include "websocket.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "websocket";

static httpd_handle_t ws_server = NULL;

static int clients[WS_MAX_CLIENTS];
static size_t client_index = 0;

static SemaphoreHandle_t clients_mutex = NULL;

static esp_err_t websocket_handler(httpd_req_t *req) {
	int client_fd = httpd_req_to_sockfd(req); // get client_fd

	if (req->method == HTTP_GET) { // handshake
		ESP_LOGI(TAG, "New connection request, client %d", client_fd);

		xSemaphoreTake(clients_mutex, portMAX_DELAY);
		if (client_index < WS_MAX_CLIENTS) {
			// check if client already connected
			for (size_t i=0; i<client_index; i++) {
				if (client_fd == clients[i]) {
					xSemaphoreGive(clients_mutex);
					return ESP_OK;
				}
			}

			clients[client_index++] = client_fd;

			ESP_LOGI(TAG, "Client %d connected", client_fd);
		} else {
			ESP_LOGW(TAG, "Max clients reached. Connection rejected");
		}
		xSemaphoreGive(clients_mutex);
		return ESP_OK;
	}
	return ESP_OK;
}

void websocket_register_uri(httpd_handle_t server) {
	httpd_uri_t ws_uri = {
		.uri = "/ws",
		.method = HTTP_GET,
		.handler = websocket_handler,
		.user_ctx = NULL,
		.is_websocket = true
	};
	httpd_register_uri_handler(server, &ws_uri);
	ws_server = server;

	// init mutex
	if (clients_mutex == NULL) {
		clients_mutex = xSemaphoreCreateMutex();
		if (clients_mutex == NULL) {
			ESP_LOGE(TAG, "Failed to create clients mutex!");
		}
	}
}

esp_err_t websocket_broadcast(const char *data) {
	if (!ws_server) return ESP_FAIL;

	httpd_ws_frame_t ws_pkt = {
		.type = HTTPD_WS_TYPE_TEXT,
		.payload = (uint8_t *)data,
		.len = strlen(data),
		.final = true
	};

	// send message
	xSemaphoreTake(clients_mutex, portMAX_DELAY);
	for (size_t i=0; i<client_index; i++) {
		if (httpd_ws_send_frame_async(ws_server, clients[i], &ws_pkt) != ESP_OK) {
			ESP_LOGW(TAG, "Failed to send to client %d", clients[i]);

			// remove client[i]
			clients[i] = clients[--client_index];
			i--;
		}
	}
	xSemaphoreGive(clients_mutex);

	return ESP_OK;
}

esp_err_t websocket_stop(void) {
	if (clients_mutex == NULL || ws_server == NULL) {
		ws_server = NULL;
		return ESP_OK;
	}

	if (xSemaphoreTake(clients_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
		httpd_ws_frame_t ws_close_pkt = {
			.type = HTTPD_WS_TYPE_CLOSE,
			.payload = NULL,
			.len = 0,
			.final = true
		};

		for (size_t i = 0; i < client_index; i++) {
			esp_err_t send_err = httpd_ws_send_frame_async(ws_server, clients[i], &ws_close_pkt);

			if (send_err != ESP_OK) {
				ESP_LOGW(TAG, "Failed to send CLOSE frame to client %d: %s", clients[i], esp_err_to_name(send_err));
				// httpd_sess_trigger_close(ws_server, clients[i]);
			}
		}

		client_index = 0;

		xSemaphoreGive(clients_mutex);
	} else {
		ESP_LOGE(TAG, "Não foi possível obter o mutex dos clientes para parar o WebSocket.");
	}

	if (ws_server) {
	    ESP_LOGI(TAG, "Unregister handler /ws");
	    httpd_unregister_uri_handler(ws_server, "/ws", HTTP_GET);
	}

	ws_server = NULL;

	// delete mutex?

	return ESP_OK;
}