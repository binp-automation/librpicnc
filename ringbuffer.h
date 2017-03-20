#pragma once

#include <stdlib.h>
#include <string.h>


typedef struct {
	int size, elem_size;
	int head, tail;
	int occupancy;
	uint8_t *data;
} RB;


RB *rb_init(int size, int elem_size) {
	if (size <= 0 || elem_size <= 0) {
		goto err_size;
	}
	
	RB *rb = (RB*) malloc(sizeof(RB));
	if (!rb) {
		goto err_rb;
	}
	
	rb->data = (uint8_t*) malloc(size*elem_size);
	if (!rb->data) {
		goto err_data;
	}
	
	rb->size = size;
	rb->elem_size = elem_size;
	rb->head = size - 1;
	rb->tail = 0;
	rb->occupancy = 0;
	
	return rb;
	
// err_other:
	free(rb->data);
err_data:
	free(rb);
err_rb:
err_size:
	return NULL;
}

void rb_free(RB *rb) {
	if (rb) {
		if (rb->data) {
			free(rb->data);
		}
		free(rb);
	}
}

uint8_t *rb_head(RB *rb) {
	return rb->data + rb->head*rb->elem_size;
}

uint8_t *rb_tail(RB *rb) {
	return rb->data + rb->tail*rb->elem_size;
}

int rb_occupancy(const RB *rb) {
	return rb->occupancy;
}

int rb_empty(const RB *rb) {
	return rb->occupancy <= 0;
}

int rb_full(const RB *rb) {
	return rb->occupancy >= rb->size;
}

int rb_push(RB *rb, const uint8_t *data) {
	if (rb->occupancy >= rb->size) {
		// ring buffer is full
		return 1;
	}
	
	rb->head = (rb->head + 1) % rb->size;
	rb->occupancy += 1;
	
	if (data != NULL) {
		memcpy(rb_head(rb), data, rb->elem_size);
	}
	
	return 0;
}

int rb_pop(RB *rb, uint8_t *data) {
	if (rb->occupancy <= 0) {
		// ring buffer is empty
		return 1;
	}
	
	if (data != NULL) {
		memcpy(data, rb_tail(rb), rb->elem_size);
	}
	
	rb->tail = (rb->tail + 1) % rb->size;
	rb->occupancy -= 1;
	
	return 0;
}
