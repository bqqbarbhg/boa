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

static void *boa__default_alloc(boa_allocator *ator, size_t size)
{
	return malloc(size);
}

static void *boa__default_realloc(boa_allocator *ator, void *ptr, size_t size)
{
	return realloc(ptr, size);
}

static void boa__default_free(boa_allocator *ator, void *ptr)
{
	return free(ptr);
}

boa_allocator boa__default_ator = {
	boa__default_alloc,
	boa__default_realloc,
	boa__default_free,
};

static void *boa__null_alloc(boa_allocator *ator, size_t size)
{
	return NULL;
}

static void *boa__null_realloc(boa_allocator *ator, void *ptr, size_t size)
{
	return NULL;
}

static void boa__null_free(boa_allocator *ator, void *ptr)
{
}

boa_allocator boa__null_ator = {
	boa__null_alloc,
	boa__null_realloc,
	boa__null_free,
};

// -- boa_arena

enum {
	BOA__ARENA_FLAG_HUGE = 1,
};

typedef struct boa__arena_page {
	struct boa__arena_page *next;
	uint32_t capacity;
} boa__arena_page;

typedef struct boa__arena_header {
	uint32_t size;
	uint32_t flags;
} boa__arena_header;

typedef struct boa__arena_huge_header {
	boa__arena_huge_header *prev, *next;
	boa__arena_header header;
} boa__arena_huge_header;

struct boa_arena {
	boa_allocator self_ator;
	boa_allocator *inner_ator;
	boa_allocator *huge_ator;
	int allocated;
	struct boa__arena_page first_page;
	
};

typedef struct boa__arena_node {
	uint8_t children;
} boa__arena_node;

typedef struct boa__arena_pool {
	uint32_t size;
	uint32_t mask;
} boa__arena_pool;

static boa__arena_node *boa__arena_find_node(boa__arena_node *root, uint32_t size, uint32_t offset)
{
	boa__arena_node *node = root;
	uint32_t children;
	while (children = node->children) {
		uint32_t half = size >> 1;
		uint32_t ix = (offset < half) ? 0 : 1;
		node = node + children + ix;
	}
	return node;
}

void *boa__arena_alloc(boa_allocator *ator, size_t size)
{
	boa_arena *arena = (boa_arena*)ator;

	if (size >= arena->huge_size) {
		void *ptr = (void*)boa_alloc(arena->huge_ator, size + sizeof(boa__arena_huge_header));
		if (ptr) {
			boa__arena_huge_header *header = (boa__arena_huge_header*)ptr;
			boa__arena_huge_header *head = &arena->huge_ends[0], *next = head->next;
			header->header.size = size;
			header->header.flags = BOA__ARENA_FLAG_HUGE;
			header->prev = head;
			header->next = next;
			head->next = header;
			next->prev = header;
			return header + 1;
		}
	}

	uint32_t sz = 31 - boa_clz32(size);
	uint32_t avail = arena->sz_bitmask & ((1 << sz) - 1);
	if (avail) {
		uint32_t first = boa_clz32(avail);
		void *ptr = arena->sz_head[first];
		void *next = *(void**)ptr;
		if (next) {
			arena->sz_head[first] = next;
		} else {
			arena->sz_bitmask &= ~(1u << first);
		}

		boa__arena_header *header = (boa__arena_header*)ptr;
		header->size = size;
		header->flags = 0;

		return header + 1;
	} else {
	}
}

void *boa__arena_realloc(boa_allocator *ator, void *ptr, size_t size)
{
	void *next = boa__arena_alloc(ator, size);
	if (next) {
		boa__arena_header *header = (boa__arena_header*)ptr -1;
		memcpy(next, ptr, header->size);
		boa__arena_free(ator, ptr);
	}
	return next;
}

void boa__arena_free(boa_allocator *ator, void *ptr)
{
	boa_arena *arena = (boa_arena*)ator;
	boa__arena_header *header = (boa__arena_header*)ptr - 1;
	if (header->flags & BOA__ARENA_FLAG_HUGE) {
		boa__arena_huge_header *huge_header = (boa__arena_huge_header*)ptr - 1;
		huge_header->prev->next = huge_header->next;
		huge_header->next->prev = huge_header->prev;
		boa_free(arena->huge_ator, huge_header);
	} else {
	}
}

boa_arena *boa_arena_make_buffer(boa_allocator *ator, uint32_t capacity, void *buffer, uint32_t size)
{
	int allocated = 0;
	if (!buffer || size < sizeof(boa_arena)) {
		buffer = boa_alloc(ator, sizeof(boa_arena) + capacity);
		allocated = 1;
		if (!buffer) return NULL;
	} else { 
		capacity = size - sizeof(boa_arena);
	}

	boa_arena *arena = (boa_arena*)buffer;

	arena->self_ator.alloc_fn = &boa__arena_alloc;
	arena->self_ator.realloc_fn = &boa__arena_realloc;
	arena->self_ator.free_fn = &boa__arena_free;
	arena->inner_ator = ator;
	arena->huge_ator = ator;
	arena->allocated = allocated;

	arena->first_page.next = NULL;
	arena->first_page.capacity = capacity;

	return arena;
}

void boa_arena_free(boa_arena *arena)
{
	boa_allocator *inner_ator = arena->inner_ator;
	boa__arena_page *page = arena->first_page.next;
	while (page) {
		boa__arena_page *to_free = page;
		page = page->next;
		boa_free(inner_ator, page);
	}

	if (arena->allocated) {
		boa_free(inner_ator, arena);
	}
}

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
	boa_free(ator, data);
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
		new_data = boa_realloc(ator, (char*)old_data - offset, new_cap + offset);
	} else {
		if (old_data != NULL) {
			// Fixed -> Heap: Create header and memcpy() data
			new_data = boa_alloc(ator, new_cap + sizeof(boa__buf_grow_header));
			if (new_data) {
				boa__buf_grow_header *header = (boa__buf_grow_header*)new_data;
				header->fixed_data = old_data;
				header->fixed_cap = old_cap;
				new_data = header + 1;
				memcpy(new_data, old_data, old_cap);
			}
		} else {
			// Zero -> Heap: Just allocate a new buffer
			new_data = boa_alloc(ator, new_cap);
		}
		buf->ator_flags = ator_flags | BOA_BUF_FLAG_ALLOCATED;
	}

	if (!new_data) return NULL;
	buf->data = new_data;
	buf->cap_pos = new_cap;

	return (char*)new_data + buf->end_pos;
}

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size)
{
	boa_assert(buf != NULL);
	boa_assert(size > 0);
	boa_assert(pos + size <= buf->end_pos);

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
		zerobuf = boa_empty_buf_default();
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
