#include "circular_buffer.h"

#include "freertos/portmacro.h"
#include "esp_log.h"

static const char *TAG = "circular-buffer";

static portMUX_TYPE buffer_mux = portMUX_INITIALIZER_UNLOCKED;

void circular_buffer_init(circular_buffer_t *buffer) {
	buffer->head = 0;
}

void circular_buffer_write(circular_buffer_t *buffer, uint32_t value) {
	ESP_LOGI(TAG, "write buffer");
	// disables interrupts to avoid race condition
	portENTER_CRITICAL(&buffer_mux);

	// buffer->data[buffer->head % CIRCULAR_BUFFER_SIZE] = value;
	// buffer->head++;

	buffer->data[CIRCULAR_BUFFER_INDEX(buffer->head++)] = value; // head overflow is safe due to unsigned wraparound logic

	portEXIT_CRITICAL(&buffer_mux);
}

bool circular_buffer_read(circular_buffer_t *buffer, circular_reader_t *reader, uint32_t *out) {
	// safe read without blocking the writer
	portENTER_CRITICAL(&buffer_mux);

	size_t available = buffer->head - reader->read_index;
	if (available == 0) {
		portEXIT_CRITICAL(&buffer_mux);
		ESP_LOGI(TAG, "no new data");   
		return false;  // no new data for this reader
	}

	if (available > CIRCULAR_BUFFER_SIZE) {
		// reader is too slow, data has been lost
		reader->read_index = buffer->head - CIRCULAR_BUFFER_SIZE;
		ESP_LOGW(TAG, "reader too slow");   
	}

	*out = buffer->data[CIRCULAR_BUFFER_INDEX(reader->read_index)];
	reader->read_index++; // reader index overflow is safe due to unsigned wraparound logic

	portEXIT_CRITICAL(&buffer_mux);
	return true;
}
