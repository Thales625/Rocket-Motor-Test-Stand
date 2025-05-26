#pragma once

#include "esp_http_server.h"

#define WS_BUFFER_SIZE 128
#define WS_MAX_CLIENTS 16

esp_err_t websocket_broadcast(const char *data);
void websocket_register_uri(httpd_handle_t server);