#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define CIRCULAR_BUFFER_SIZE 128
#define CIRCULAR_BUFFER_MASK (CIRCULAR_BUFFER_SIZE - 1)
#define CIRCULAR_BUFFER_INDEX(i) ((i) & CIRCULAR_BUFFER_MASK)

_Static_assert((CIRCULAR_BUFFER_SIZE & CIRCULAR_BUFFER_MASK) == 0, "CIRCULAR_BUFFER_SIZE must be a power of 2");

typedef struct {
	uint32_t data[CIRCULAR_BUFFER_SIZE];
	volatile size_t head;
} circular_buffer_t;

typedef struct {
	size_t read_index;
} circular_reader_t;

void circular_buffer_init(circular_buffer_t *buffer);
void circular_buffer_write(circular_buffer_t *buffer, uint32_t value);
bool circular_buffer_read(circular_buffer_t *buffer, circular_reader_t *reader, uint32_t *out);
