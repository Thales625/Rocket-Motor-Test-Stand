#pragma once

#include "esp_http_server.h"

void websocket_register_uri(httpd_handle_t server);
esp_err_t websocket_send_text(const char *data);