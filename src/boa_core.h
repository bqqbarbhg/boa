#pragma once

#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

// -- General helpers

#include <stdint.h>

#define boa_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))
#define boa_arrayend(arr) (arr + (sizeof(arr) / sizeof(*(arr))))

#define BOA_RESTRICT

// -- boa_assert

#define boa_assert(cond)

// -- boa_malloc

void *boa_malloc(size_t size);
void *boa_realloc(void *ptr, size_t size);
void boa_free(void *ptr);

// -- boa_buf

enum {
	BOA_BUF_GROW = 0,
	BOA_BUF_FIXED = 1,
	BOA_BUF_HEAP_ONLY = 2,
	BOA_BUF_HEAP_GROW = 3,
};

#define BOA_BUF_IN_HEAP_BIT 2u
#define BOA_BUF_FLAG_SHIFT 2u
#define BOA_BUF_POS_MASK (~3u)
#define BOA_BUF_FLAG_MASK (3u)

typedef struct boa_buf {

	void *data;
	uint32_t end_pos;
	uint32_t cap_pos;

} boa_buf;

static inline boa_buf boa_buf_make(void *data, uint32_t cap, uint32_t flags)
{
	boa_buf b = { data, flags, cap };
	return b;
}

static inline boa_buf *boa_clear(boa_buf *buf)
{
	buf->flags_pos &= BOA_BUF_POS_MASK;
	return buf;
}

static inline boa_buf *boa_reset(boa_buf *buf)
{
	extern void boa__buf_reset_heap(boa_buf *buf);
	uint32_t flags_pos = buf->flags_pos;
	buf->flags_pos = flags_pos & BOA_BUF_FLAG_MASK;
	if (flags_pos & BOA_BUF_IN_HEAP_BIT)
		boa__buf_reset_heap(buf);
	return buf;
}

#define boa_pos(buf) ((buf)->flags_pos >> BOA_BUF_FLAG_SHIFT)
#define boa_flags(buf) ((buf)->flags_pos & BOA_BUF_FLAG_MASK)

static inline void *boa_buf_reserve(boa_buf *buf, uint32_t size)
{
	extern void *boa__buf_grow(boa_buf *buf, size_t req_cap);
	uint32_t pos = boa_pos(buf), cap = buf->cap;
	uint32_t req_cap = pos + size;
	if (req_cap <= cap) return (char*)buf->data + pos;
	return boa__buf_grow(buf, req_cap);
}

static inline void boa_buf_bump(boa_buf *buf, uint32_t size)
{
	boa_assert(boa_pos(buf) + size <= buf->cap);
	buf->flags_pos += size << BOA_BUF_FLAG_SHIFT;
}

static inline void *boa_buf_push(boa_buf *buf, uint32_t size)
{
	void *ptr = boa_buf_reserve(buf, size);
	if (ptr) boa_buf_bump(buf, size);
	return ptr;
}

char *boa_format(boa_buf *buf, const char *fmt, ...);

#define boa_empty_buf() boa_buf_make(0, 0, BOA_BUF_GROW)
#define boa_range_buf(begin, end) boa_buf_make(begin, (char*)end - (char*)begin, BOA_BUF_GROW)
#define boa_slice_buf(begin, count) boa_buf_make(begin, count * sizeof(*(begin)), BOA_BUF_GROW)
#define boa_bytes_buf(begin, size) boa_buf_make(begin, size, BOA_BUF_GROW)
#define boa_array_buf(array) boa_buf_make(array, sizeof(array), BOA_BUF_GROW)

#define boa_empty_view() boa_buf_make(0, 0, BOA_BUF_FIXED)
#define boa_range_view(begin, end) boa_buf_make(begin, (char*)end - (char*)begin, BOA_BUF_FIXED)
#define boa_slice_view(begin, count) boa_buf_make(begin, count * sizeof(*(begin)), BOA_BUF_FIXED)
#define boa_bytes_view(begin, size) boa_buf_make(begin, size, BOA_BUF_FIXED)
#define boa_array_view(array) boa_buf_make(array, sizeof(array), BOA_BUF_FIXED)

#define boa_begin(type, buf) ((type*)(buf)->data)
#define boa_end(type, buf) ((type*)((char*)(buf)->data + boa_pos(buf)))
#define boa_cap(type, buf) ((type*)((char*)(buf)->data + (buf)->cap))
#define boa_count(type, buf) (type*)(boa_pos(buf) / sizeof(type))
#define boa_reserve(type, buf) (type*)boa_buf_reserve((buf), sizeof(type))
#define boa_bump(type, buf) boa_buf_bump((buf), sizeof(type))
#define boa_push(type, buf) (type*)boa_buf_push((buf), sizeof(type))
#define boa_reserve_n(type, buf, n) (type*)boa_buf_reserve((buf), (n) * sizeof(type))
#define boa_push_n(type, buf, n) (type*)boa_buf_push((buf), (n) * sizeof(type))

#define boa_bytesleft(buf) ((buf)->cap - boa_pos(buf))

#endif
