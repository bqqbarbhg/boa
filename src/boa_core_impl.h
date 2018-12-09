#pragma once

#include "boa_core.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef BOA__CORE_IMPLEMENTED
#define BOA__CORE_IMPLEMENTED

// -- boa_malloc

void *boa_malloc(size_t size)
{
	return malloc(size);
}

void *boa_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void boa_free(void *ptr)
{
	free(ptr);
}

#define BOA_MIN_BUF_CAP 16

typedef struct boa__buf_grow_header {
	void *fixed_data;
	uint32_t fixed_cap;
} boa__buf_grow_header;

void boa__buf_reset_heap(boa_buf *BOA_RESTRICT buf)
{
	void *data = buf->data;
	uint32_t flags_pos = buf->flags_pos;
	if ((flags_pos & BOA_BUF_FLAG_MASK) == BOA_BUF_HEAP_GROW) {
		// Heap -> Fixed: Return original buffer
		boa__buf_grow_header *header = (boa__buf_grow_header*)data - 1;
		buf->data = header->fixed_data;
		buf->cap = header->fixed_cap;
		data = header;
	} else {
		// Heap -> Zero: Return to empty buffer
		buf->data = NULL;
		buf->cap = 0;
	}
	buf->flags_pos = BOA_BUF_GROW;
	boa_free(data);
}

void *boa__buf_grow(boa_buf *BOA_RESTRICT buf, uint32_t new_cap)
{
	uint32_t old_cap = buf->cap;
	uint32_t flags_pos = buf->flags_pos;
	void *old_data = buf->data;

	void *new_data;

	// This function should only be called when needed
	boa_assert(new_cap > old_cap);

	// If the buffer is fixed all allocations must fail
	if ((flags_pos & BOA_BUF_FLAG_MASK) == BOA_BUF_FIXED) return NULL;

	// Ensure geometric growth and minimum size
	if (new_cap < BOA_MIN_BUF_CAP) new_cap = BOA_MIN_BUF_CAP;
	if (new_cap < old_cap * 2) new_cap = old_cap * 2;

	if ((flags_pos & BOA_BUF_FLAG_MASK) != BOA_BUF_GROW) {
		// Heap -> Heap: Just realloc() with optional header offset
		uint32_t offset = (flags_pos & 1) ? sizeof(boa__buf_grow_header) : 0;
		new_data = boa_realloc((char*)old_data - offset, new_cap + offset);
	} else {
		if (old_data != NULL) {
			// Fixed -> Heap: Create header and memcpy() data
			new_data = boa_malloc(new_cap + sizeof(boa__buf_grow_header));
			if (new_data) {
				boa__buf_grow_header *header = (boa__buf_grow_header*)new_data;
				header->fixed_data = old_data;
				header->fixed_cap = old_cap;
				new_data = header + 1;
				memcpy(new_data, old_data, old_cap);
			}
			flags_pos = (flags_pos & BOA_BUF_POS_MASK) | BOA_BUF_HEAP_GROW;
		} else {
			// Zero -> Heap: Just allocate a new buffer
			new_data = boa_malloc(new_cap);
			flags_pos = (flags_pos & BOA_BUF_POS_MASK) | BOA_BUF_HEAP_ONLY;
		}
	}

	if (!new_data) return NULL;
	buf->data = new_data;
	buf->flags_pos = flags_pos;
	buf->cap = new_cap;

	return (char*)new_data + boa_pos(buf);
}

char *boa_format(boa_buf *BOA_RESTRICT buf, const char *fmt, ...)
{
	boa_buf zerobuf;
	if (!buf) {
		zerobuf = boa_empty_buf();
		buf = &zerobuf;
	}

	char *ptr = boa_end(char, buf);
	uint32_t cap = boa_bytesleft(buf);
	int len;
	va_list args;

	va_start(args, fmt);
	len = vsnprintf(ptr, cap, fmt, args);
	va_end(args);

	if (len < 0) return NULL;

	if ((uint32_t)len >= cap) {
		ptr = (char*)boa_buf_reserve(buf, (uint32_t)len + 1);
		cap = boa_bytesleft(buf);
		if (!ptr) return NULL;

		va_start(args, fmt);
		vsnprintf(ptr, cap, fmt, args);
		va_end(args);
	}

	ptr[len] = '\0';
	boa_buf_bump(buf, len + 1);
	return ptr;
}

#endif
