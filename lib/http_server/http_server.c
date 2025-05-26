#include "http_server.h"
#include "websocket.h"

#include <esp_http_server.h>
#include <esp_log.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static const char *TAG = "http-server";

static const char *index_html =
"<!DOCTYPE html>"
"<html>"
"<head>"
"  <meta charset='UTF-8'>"
"  <title>ESP32 WebSocket</title>"
"</head>"
"<body>"
"  <h1>ESP32 WebSocket Test</h1>"
"  <input type='text' id='input-message'>"
"  <button onclick=\"sendMessage()\">Send message</button>"
"  <p>Received:</p>"
"  <pre id='messages'></pre>"
"  <script>"
"    const ws=new WebSocket('ws://'+location.host+'/ws');"
"    ws.onopen=()=>console.log('WebSocket connected');"
"    ws.onmessage=(event)=>{"
"      document.getElementById('messages').textContent+=event.data+'\\n';"
"    };"
"    function sendMessage(){"
"      message=document.getElementById('input-message').value;"
"      console.log(message.length);"
"      if(message.length>"STR(WS_BUFFER_SIZE)") return;"
"      ws.send(message);"
"    }"
"  </script>"
"</body>"
"</html>";

static esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

void start_http_server(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	httpd_handle_t server = NULL;
	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_uri_t index_uri = {
			.uri      = "/",
			.method   = HTTP_GET,
			.handler  = get_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(server, &index_uri);

		// start websocket
		websocket_register_uri(server);

		ESP_LOGI(TAG, "HTTP server with WebSocket started");
	} else {
		ESP_LOGE(TAG, "Failed to start HTTP server");
	}
}
