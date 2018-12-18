#pragma once

#include "boa_core.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef BOA__CORE_IMPLEMENTED
#define BOA__CORE_IMPLEMENTED

// -- boa_allocator

#ifndef BOA_MALLOC
	#define BOA_MALLOC malloc
#endif

#ifndef BOA_REALLOC
	#define BOA_REALLOC realloc
#endif

#ifndef BOA_FREE
	#define BOA_FREE free
#endif

void *boa_alloc(size_t size) { return BOA_MALLOC(size); }
void *boa_realloc(void *ptr, size_t size) { return BOA_REALLOC(ptr, size); }
void boa_free(void *ptr) { BOA_FREE(ptr); }

static void *boa__heap_alloc(boa_allocator *ator, size_t size) { return BOA_MALLOC(size); }
static void *boa__heap_realloc(boa_allocator *ator, void *ptr, size_t size) { return BOA_REALLOC(ptr, size); }
static void boa__heap_free(boa_allocator *ator, void *ptr) { BOA_FREE(ptr); }

boa_allocator boa__heap_ator = {
	boa__heap_alloc,
	boa__heap_realloc,
	boa__heap_free,
};

static void *boa__null_alloc(boa_allocator *ator, size_t size){ return NULL; }
static void *boa__null_realloc(boa_allocator *ator, void *ptr, size_t size){ return NULL; }
static void boa__null_free(boa_allocator *ator, void *ptr) { }

boa_allocator boa__null_ator = {
	boa__null_alloc,
	boa__null_realloc,
	boa__null_free,
};

// -- boa_buf

static inline boa_allocator *boa__buf_ator(uintptr_t ator_flags)
{
	return (boa_allocator*)(ator_flags & ~(uintptr_t)BOA_BUF_FLAG_MASK);
}

#define BOA_MIN_BUF_CAP 16

typedef struct boa__buf_grow_header {
	void *fixed_data;
	uint32_t fixed_cap;
} boa__buf_grow_header;

void boa__buf_reset_heap(boa_buf *buf)
{
	void *data = buf->data;
	size_t ator_flags = buf->ator_flags;
	boa_allocator *ator = boa__buf_ator(ator_flags);
	if (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) {
		// Heap -> Fixed: Return original buffer
		boa__buf_grow_header *header = (boa__buf_grow_header*)data - 1;
		buf->data = header->fixed_data;
		buf->cap_pos = header->fixed_cap;
		data = header;
	} else {
		// Heap -> Zero: Return to empty buffer
		buf->data = NULL;
		buf->cap_pos = 0;
	}
	boa_free_ator(ator, data);
}

void *boa__buf_grow(boa_buf *buf, uint32_t new_cap)
{
	uint32_t old_cap = buf->cap_pos;
	void *old_data = buf->data;
	uintptr_t ator_flags = buf->ator_flags;
	boa_allocator *ator = boa__buf_ator(ator_flags);

	void *new_data;

	// This function should only be called when needed
	boa_assert(new_cap > old_cap);

	// Ensure geometric growth and minimum size
	if (new_cap < BOA_MIN_BUF_CAP) new_cap = BOA_MIN_BUF_CAP;
	if (new_cap < old_cap * 2) new_cap = old_cap * 2;

	if (ator_flags & BOA_BUF_FLAG_ALLOCATED) {
		// Heap -> Heap: Just realloc() with optional header offset
		uint32_t offset = (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) ? sizeof(boa__buf_grow_header) : 0;
		new_data = boa_realloc_ator(ator, (char*)old_data - offset, new_cap + offset);
	} else {
		if (old_data != NULL) {
			// Fixed -> Heap: Create header and memcpy() data
			new_data = boa_alloc_ator(ator, new_cap + sizeof(boa__buf_grow_header));
			ator_flags |= BOA_BUF_FLAG_HAS_BUFFER;
			if (new_data) {
				boa__buf_grow_header *header = (boa__buf_grow_header*)new_data;
				header->fixed_data = old_data;
				header->fixed_cap = old_cap;
				new_data = header + 1;
				memcpy(new_data, old_data, old_cap);
			}
		} else {
			// Zero -> Heap: Just allocate a new buffer
			new_data = boa_alloc_ator(ator, new_cap);
		}
		buf->ator_flags = ator_flags | BOA_BUF_FLAG_ALLOCATED;
	}

	if (!new_data) return NULL;
	buf->data = new_data;
	buf->cap_pos = new_cap;

	return (char*)new_data + buf->end_pos;
}

int boa_buf_set_ator(boa_buf *buf, boa_allocator *ator)
{
	uintptr_t ator_flags = buf->ator_flags;
	boa_allocator *old_ator = boa__buf_ator(ator_flags);
	if (old_ator == ator) return 1;
	
	if (!(ator_flags & BOA_BUF_FLAG_ALLOCATED)) {
		buf->ator_flags = (ator_flags & (uintptr_t)BOA_BUF_FLAG_MASK) | (uintptr_t)ator;
		return 1;
	}

	uint32_t offset = (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) ? sizeof(boa__buf_grow_header) : 0;
	void *old_data = (char*)buf->data - offset;
	uint32_t total_size = buf->cap_pos + offset;

	void *new_data = boa_alloc_ator(ator, total_size);
	if (!new_data) return 0;

	memcpy(new_data, old_data, total_size);

	buf->data = (char*)new_data + offset;
	buf->ator_flags = (ator_flags & (uintptr_t)BOA_BUF_FLAG_MASK) | (uintptr_t)ator;
	boa_free_ator(old_ator, old_data);
	return 1;
}

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size)
{
	boa_assert(buf != NULL);
	boa_assert(size > 0);
	boa_assert(pos <= buf->end_pos);

	uint32_t req_cap = buf->end_pos + size;
	if (req_cap > buf->cap_pos) {
		void *ptr = boa__buf_grow(buf, req_cap);
		if (!ptr) return NULL;
	}

	char *begin = (char*)buf->data + pos;
	memmove(begin + size, begin, buf->end_pos - pos);
	buf->end_pos += size;
	return begin;
}

char *boa_format(boa_buf *buf, const char *fmt, ...)
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
	boa_buf_bump(buf, len);
	return ptr;
}

void boa__map_unlink_and_insert(boa_map *map, boa__map_chunk *chunk, uint32_t slot);

boa__map_chunk *boa__map_make_next_chunk(boa_map *map, boa__map_chunk *chunk)
{
	if (chunk->next == 0) {
		map->num_aux++;
		chunk->next = map->num_aux;
	}
	return boa__map_next_chunk(map, chunk);
}

// Create a new node coming to the chunk
uint32_t boa__map_create_white(boa_map *map, boa__map_chunk *chunk, void **kv)
{
	uint32_t ix;
	uint32_t free = boa__map_empty_mask(map, chunk);
	if (free) {
		ix = boa_find_bit(free) / 2;
		boa__map_init_node(chunk, ix, boa__map_white);
	} else {
		free = boa__map_non_white_mask(map, chunk);
		boa_map_assert(free != 0);
		ix = boa_find_bit(free) / 2;
		boa__map_unlink_and_insert(map, chunk, ix);
	}

	*kv = boa__map_get_kv(map, chunk, ix);

	return ix;
}

// Create a new node and add it after `slot`
void *boa__map_create(boa_map *map, boa__map_chunk *chunk, uint32_t slot)
{
	void *kv;
	uint32_t free = boa__map_empty_mask(map, chunk);
	if (free) {
		uint32_t ix = boa_find_bit(free) / 2;
		boa__map_set_link(chunk, slot, ix);
		boa__map_init_node(chunk, ix, boa__map_black);
		kv = boa__map_get_kv(map, chunk, ix);
	} else {
		boa__map_chunk *nextc = boa__map_make_next_chunk(map, chunk);
		uint32_t ix = boa__map_create_white(map, nextc, &kv) | 8;
		boa__map_set_link(chunk, slot, ix);
	}
	return kv;
}

// Follow the linked list from this node to end and insert this node to the end
// Note: Supports resolving links to next chunks for `slot`
void *boa__map_insert(boa_map *map, boa__map_chunk *chunk, uint32_t slot)
{
	if (slot >= 8) {
		chunk = boa__map_next_chunk(map, chunk);
		slot -= 8;
	}

	for (;;) {
		uint32_t link = boa__map_get_link(chunk, slot);
		if (link == slot) {
			return boa__map_create(map, chunk, slot);
		} else if (link >= 8) {
			chunk = boa__map_next_chunk(map, chunk);
			slot -= 8;
		}
	}
}

// Override a black node with a white one. Replace the intra-chunk link coming to
// this node with the black node's link and re-insert into the previous node.
void boa__map_unlink_and_insert(boa_map *map, boa__map_chunk *chunk, uint32_t slot)
{
	uint32_t prev = boa__map_get_prev(chunk, slot);
	uint32_t link = boa__map_get_link(chunk, slot);
	uint32_t next;
	if (link != slot) {
		next = link;
	} else {
		next = prev;
	}
	boa__map_set_link(chunk, prev, next);

	boa__map_init_node(chunk, slot, boa__map_white);

	void *dst = boa__map_insert(map, chunk, next);
	void *src = boa__map_get_kv(map, chunk, slot);
	memcpy(dst, src, map->kv_size);
}

#endif
