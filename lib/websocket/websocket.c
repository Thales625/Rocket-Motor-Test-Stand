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
	/*
	printf("client fd: %d\n", client_fd);

	uint8_t buffer[WS_BUFFER_SIZE] = {0};

	httpd_ws_frame_t ws_pkt = {
		.type = HTTPD_WS_TYPE_TEXT,
		.payload = buffer
	};

	esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, sizeof(buffer));
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "WebSocket recv error: %s", esp_err_to_name(ret));
		return ret;
	}

	buffer[ws_pkt.len] = 0; // null-terminate
	ESP_LOGI(TAG, "Received '%s'", buffer);

	char message[WS_BUFFER_SIZE];

	snprintf(message, sizeof(message), "(%d): %s", client_fd, (const char *)buffer);

	return websocket_broadcast(message);
	*/
}

esp_err_t websocket_broadcast(const char *data) {
	if (!ws_server) return ESP_FAIL;

	httpd_ws_frame_t ws_pkt = {
		.type = HTTPD_WS_TYPE_TEXT,
		.payload = (uint8_t *)data,
		.len = strlen(data),
		.final = true
	};

	// send message mutex
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
