#include "websocket.h"

#include "esp_log.h"
#include <string.h>

static const char *TAG = "websocket";

static httpd_handle_t ws_server = NULL;
static int client_fd = -1;

static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "NEW CONNECTION");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt = {0};
    uint8_t buffer[128] = {0};

    ws_pkt.payload = buffer;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, sizeof(buffer));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WebSocket recv error: %s", esp_err_to_name(ret));
        return ret;
    }

    buffer[ws_pkt.len] = 0; // null-terminate
    ESP_LOGI(TAG, "Received: %s", buffer);

    // echo message
    // websocket_send_text((const char *) buffer);

    // echo to sender
    client_fd = httpd_req_to_sockfd(req); // get req client_fd
    return httpd_ws_send_frame(req, &ws_pkt);
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
}

esp_err_t websocket_send_text(const char *data) {
    if (ws_server && client_fd != -1) {
        httpd_ws_frame_t ws_pkt = {
            .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)data,
            .len = strlen(data),
            .final = true
        };
        return httpd_ws_send_frame_async(ws_server, client_fd, &ws_pkt);
    }
    return ESP_FAIL;
}
