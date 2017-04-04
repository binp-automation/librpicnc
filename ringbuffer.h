#pragma once

#include <stdlib.h>
#include <string.h>

#define define_ringbuffer(name, pref, type)\
	typedef struct {\
		int size;\
		int head, tail;\
		int occupancy;\
		type *data;\
	} name;\
\
\
	name *pref##_init(int size) {\
		if (size <= 0) {\
			goto err_size;\
		}\
		\
		name *rb = (name*) malloc(sizeof(name));\
		if (!rb) {\
			goto err_rb;\
		}\
		\
		rb->data = (type*) malloc(size*sizeof(type));\
		if (!rb->data) {\
			goto err_data;\
		}\
		\
		rb->size = size;\
		rb->head = size - 1;\
		rb->tail = 0;\
		rb->occupancy = 0;\
		\
		return rb;\
		\
		free(rb->data);\
	err_data:\
		free(rb);\
	err_rb:\
	err_size:\
		return NULL;\
	}\
\
	void pref##_free(name *rb) {\
		if (rb) {\
			if (rb->data) {\
				free(rb->data);\
			}\
			free(rb);\
		}\
	}\
\
	type *pref##_head(name *rb) {\
		return rb->data + rb->head;\
	}\
\
	type *pref##_tail(name *rb) {\
		return rb->data + rb->tail;\
	}\
\
	int pref##_occupancy(const name *rb) {\
		return rb->occupancy;\
	}\
\
	int pref##_empty(const name *rb) {\
		return rb->occupancy <= 0;\
	}\
\
	int pref##_full(const name *rb) {\
		return rb->occupancy >= rb->size;\
	}\
\
	int pref##_push(name *rb, const type *data) {\
		if (rb->occupancy >= rb->size) {\
			return 1;\
		}\
		\
		rb->head = (rb->head + 1) % rb->size;\
		rb->occupancy += 1;\
		\
		if (data != NULL) {\
			*pref##_head(rb) = *data;\
		}\
		\
		return 0;\
	}\
\
	int pref##_pop(name *rb, type *data) {\
		if (rb->occupancy <= 0) {\
			return 1;\
		}\
		\
		if (data != NULL) {\
			*data = *pref##_tail(rb);\
		}\
		\
		rb->tail = (rb->tail + 1) % rb->size;\
		rb->occupancy -= 1;\
		\
		return 0;\
	}\

