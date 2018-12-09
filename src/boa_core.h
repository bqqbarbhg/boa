#pragma once

#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

// -- General helpers

#include <stdint.h>

#define boa_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))
#define boa_arrayend(arr) (arr + (sizeof(arr) / sizeof(*(arr))))

// -- Platform specific

#include <intrin.h>
static uint32_t boa_clz32(uint32_t val)
{
	unsigned long index;
	_BitScanReverse(&index, val);
	return (uint32_t)index;
}

// -- boa_assert

#define boa_assert(cond)

// -- boa_allocator

typedef struct boa_allocator {

	void *(*alloc_fn)(boa_allocator *ator, size_t size);
	void *(*realloc_fn)(boa_allocator *ator, void *ptr, size_t size);
	void (*free_fn)(boa_allocator *ator, void *ptr);

} boa_allocator;

static inline void *boa_alloc(boa_allocator *ator, size_t size)
{
	return ator->alloc_fn(ator, size);
}

static inline void *boa_realloc(boa_allocator *ator, void *ptr, size_t size)
{
	return ator->realloc_fn(ator, ptr, size);
}

static inline void boa_free(boa_allocator *ator, void *ptr)
{
	return ator->free_fn(ator, ptr);
}

static inline boa_allocator *boa_default_ator()
{
	extern boa_allocator boa__default_ator;
	return &boa__default_ator;
}

static inline boa_allocator *boa_null_ator()
{
	extern boa_allocator boa__null_ator;
	return &boa__null_ator;
}

// -- boa_arena

typedef struct boa_arena boa_arena;

static inline boa_allocator *boa_arena_ator(boa_arena *arena)
{
	return (boa_allocator*)arena;
}

boa_arena *boa_arena_make_buffer(boa_allocator *ator, uint32_t capacity, void *buffer, uint32_t size);

static inline boa_arena *boa_arena_make(boa_allocator *ator, uint32_t capacity)
{
	return boa_arena_make_buffer(ator, capacity, 0, 0);
}

void boa_arena_free(boa_arena *arena);

// -- boa_buf

#define BOA_BUF_FLAG_ALLOCATED 1
#define BOA_BUF_FLAG_HAS_BUFFER 2
#define BOA_BUF_FLAG_MASK 3

typedef struct boa_buf {

	uintptr_t ator_flags;
	void *data;
	uint32_t end_pos;
	uint32_t cap_pos;

} boa_buf;


static inline boa_buf boa_buf_make(void *data, uint32_t cap, boa_allocator *ator)
{
	boa_buf b = { (uintptr_t)ator, data, 0, cap };
	return b;
}

static inline boa_buf *boa_clear(boa_buf *buf)
{
	buf->end_pos = 0;
	return buf;
}

static inline boa_buf *boa_reset(boa_buf *buf)
{
	extern void boa__buf_reset_heap(boa_buf *buf);
	buf->end_pos = 0;
	if (buf->ator_flags & BOA_BUF_FLAG_ALLOCATED)
		boa__buf_reset_heap(buf);
	return buf;
}

static inline void *boa_buf_reserve(boa_buf *buf, uint32_t size)
{
	extern void *boa__buf_grow(boa_buf *buf, uint32_t req_cap);
	uint32_t end = buf->end_pos, cap = buf->cap_pos;
	uint32_t req_cap = end + size;
	if (req_cap <= cap) return (char*)buf->data + end;
	return boa__buf_grow(buf, req_cap);
}

static inline void boa_buf_bump(boa_buf *buf, uint32_t size)
{
	boa_assert(buf->end_pos + size <= buf->cap_pos);
	buf->end_pos += size;
}

static inline void *boa_buf_push(boa_buf *buf, uint32_t size)
{
	void *ptr = boa_buf_reserve(buf, size);
	if (ptr) boa_buf_bump(buf, size);
	return ptr;
}

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size);

char *boa_format(boa_buf *buf, const char *fmt, ...);

#define boa_empty_buf(ator) boa_buf_make(0, 0, ator)
#define boa_range_buf(begin, end, ator) boa_buf_make(begin, (char*)end - (char*)begin, ator)
#define boa_slice_buf(begin, count, ator) boa_buf_make(begin, count * sizeof(*(begin)), ator)
#define boa_bytes_buf(begin, size, ator) boa_buf_make(begin, size, ator)
#define boa_array_buf(array, ator) boa_buf_make(array, sizeof(array), ator)

#define boa_empty_buf_default() boa_empty_buf(boa_default_ator())
#define boa_range_buf_default(begin, end) boa_range_buf(begin, end, boa_default_ator())
#define boa_slice_buf_default(begin, count) boa_slice_buf(begin, count, boa_default_ator())
#define boa_bytes_buf_default(begin, size) boa_bytes_buf(begin, size, boa_default_ator())
#define boa_array_buf_default(array) boa_array_buf(array, boa_default_ator())

#define boa_empty_view() boa_buf_make(0, 0, &oa_null_allocator())
#define boa_range_view(begin, end) boa_buf_make(begin, (char*)end - (char*)begin, boa_null_ator())
#define boa_slice_view(begin, count) boa_buf_make(begin, count * sizeof(*(begin)), boa_null_ator())
#define boa_bytes_view(begin, size) boa_buf_make(begin, size, boa_null_ator())
#define boa_array_view(array) boa_buf_make(array, sizeof(array), boa_null_ator())

#define boa_begin(type, buf) ((type*)(buf)->data)
#define boa_end(type, buf) ((type*)((char*)(buf)->data + (buf)->end_pos))
#define boa_cap(type, buf) ((type*)((char*)(buf)->data + (buf)->cap_pos))
#define boa_count(type, buf) (type*)((buf)->end_pos / sizeof(type))
#define boa_reserve(type, buf) (type*)boa_buf_reserve((buf), sizeof(type))
#define boa_bump(type, buf) boa_buf_bump((buf), sizeof(type))
#define boa_push(type, buf) (type*)boa_buf_push((buf), sizeof(type))
#define boa_insert(type, buf, pos) (type*)boa_buf_insert((buf), (pos) * sizeof(type), sizeof(type))
#define boa_reserve_n(type, buf, n) (type*)boa_buf_reserve((buf), (n) * sizeof(type))
#define boa_push_n(type, buf, n) (type*)boa_buf_push((buf), (n) * sizeof(type))
#define boa_insert_n(type, buf, pos, n) (type*)boa_buf_insert((buf), (pos) * sizeof(type), (n) * sizeof(type))

#define boa_bytesleft(buf) ((buf)->cap_pos - (buf)->end_pos)

#endif
