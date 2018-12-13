#pragma once

#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

// -- General helpers

#include <stdint.h>
#include <stddef.h>

#define boa_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))
#define boa_arrayend(arr) (arr + (sizeof(arr) / sizeof(*(arr))))

// -- Platform

#define boa_inline inline

#if __cplusplus  >= 201103L
	#define boa_threadlocal thread_local
#elif __STDC_VERSION__ >= 201112L
	#define boa_threadlocal _Thread_local
#elif defined(_MSC_VER)
	#define boa_threadlocal __declspec(thread)
#else
	#error "Unsupported platform/"
#endif

// -- boa_assert

#if !defined(boa_assert) && !defined(boa_assertf)

	#define boa_assert(cond) if (!(cond)) __debugbreak()
	#define boa_assertf(cond, ...) if (!(cond)) __debugbreak()

#elif !defined(boa_assert)
	#error "Custom assert defined without boa_assert()"
#elif !defined(boa_assertf)
	#error "Custom assert defined without boa_assertf()"
#endif

boa_inline void *boa_check_ptr(const void *ptr)
{
	boa_assert(ptr != NULL);
	return (void*)ptr;
}

// -- boa_allocator

typedef struct boa_allocator boa_allocator;
struct boa_allocator {
	void *(*alloc_fn)(boa_allocator *ator, size_t size);
	void *(*realloc_fn)(boa_allocator *ator, void *ptr, size_t size);
	void (*free_fn)(boa_allocator *ator, void *ptr);
};

void *boa_alloc(size_t size);
void *boa_realloc(void *ptr, size_t size);
void boa_free(void *ptr);

boa_inline void *boa_alloc_ator(boa_allocator *ator, size_t size) {
	return ator ? ator->alloc_fn(ator, size) : boa_alloc(size);
}
boa_inline void *boa_realloc_ator(boa_allocator *ator, void *ptr, size_t size) {
	return ator ? ator->realloc_fn(ator, ptr, size) : boa_realloc(ptr, size);
}
boa_inline void boa_free_ator(boa_allocator *ator, void *ptr) {
	if (ator) ator->free_fn(ator, ptr); else boa_free(ptr);
}

boa_inline boa_allocator *boa_heap_ator() { extern boa_allocator boa__heap_ator; return &boa__heap_ator; }
boa_inline boa_allocator *boa_null_ator() { extern boa_allocator boa__null_ator; return &boa__null_ator; }

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


boa_inline boa_buf boa_buf_make(void *data, uint32_t cap, boa_allocator *ator)
{
	boa_buf b = { (uintptr_t)ator, data, 0, cap };
	return b;
}

boa_inline boa_buf *boa_clear(boa_buf *buf)
{
	buf->end_pos = 0;
	return buf;
}

boa_inline boa_buf *boa_reset(boa_buf *buf)
{
	extern void boa__buf_reset_heap(boa_buf *buf);
	buf->end_pos = 0;
	if (buf->ator_flags & BOA_BUF_FLAG_ALLOCATED)
		boa__buf_reset_heap(buf);
	return buf;
}

boa_inline void *boa_buf_reserve(boa_buf *buf, uint32_t size)
{
	extern void *boa__buf_grow(boa_buf *buf, uint32_t req_cap);
	uint32_t end = buf->end_pos, cap = buf->cap_pos;
	uint32_t req_cap = end + size;
	if (req_cap <= cap) return (char*)buf->data + end;
	return boa__buf_grow(buf, req_cap);
}

boa_inline void boa_buf_bump(boa_buf *buf, uint32_t size)
{
	boa_assert(buf->end_pos + size <= buf->cap_pos);
	buf->end_pos += size;
}

boa_inline void *boa_buf_push(boa_buf *buf, uint32_t size)
{
	void *ptr = boa_buf_reserve(buf, size);
	if (ptr) boa_buf_bump(buf, size);
	return ptr;
}

boa_inline boa_allocator *boa_buf_ator(boa_buf *buf)
{
	return (boa_allocator*)(buf->ator_flags & ~(uintptr_t)BOA_BUF_FLAG_MASK);
}

boa_inline void *boa_buf_get(boa_buf *buf, uint32_t offset, uint32_t size)
{
	boa_assert(offset + size <= buf->end_pos);
	return (char*)buf->data + offset;
}

int boa_buf_set_ator(boa_buf *buf, boa_allocator *ator);

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size);

char *boa_format(boa_buf *buf, const char *fmt, ...);

#define boa_empty_buf_ator(ator) boa_buf_make(NULL, 0, (ator))
#define boa_range_buf_ator(begin, end, ator) boa_buf_make((begin), (uint32_t)((char*)(end) - (char*)(begin)), (ator))
#define boa_slice_buf_ator(begin, count, ator) boa_buf_make((begin), (count) * sizeof(*(begin)), (ator))
#define boa_bytes_buf_ator(begin, size, ator) boa_buf_make((begin), (size), (ator))
#define boa_array_buf_ator(array, ator) boa_buf_make((array), sizeof(array), (ator))

#define boa_empty_buf()             boa_empty_buf_ator(NULL)
#define boa_range_buf(begin, end)   boa_range_buf_ator(begin, end, NULL)
#define boa_slice_buf(begin, count) boa_slice_buf_ator(begin, count, NULL)
#define boa_bytes_buf(begin, size)  boa_bytes_buf_ator(begin, size, NULL)
#define boa_array_buf(array)        boa_array_buf_ator(array, NULL)

#define boa_empty_view()             boa_empty_buf_ator(boa_null_ator())
#define boa_range_view(begin, end)   boa_range_buf_ator(begin, end, boa_null_ator())
#define boa_slice_view(begin, count) boa_slice_buf_ator(begin, count, boa_null_ator())
#define boa_bytes_view(begin, size)  boa_bytes_buf_ator(begin, size, boa_null_ator())
#define boa_array_view(array)        boa_array_buf_ator(array, boa_null_ator())

#define boa_begin(type, buf) ((type*)(buf)->data)
#define boa_end(type, buf) ((type*)((char*)(buf)->data + (buf)->end_pos))
#define boa_cap(type, buf) ((type*)((char*)(buf)->data + (buf)->cap_pos))
#define boa_count(type, buf) ((buf)->end_pos / sizeof(type))
#define boa_reserve(type, buf) (type*)boa_buf_reserve((buf), sizeof(type))
#define boa_bump(type, buf) boa_buf_bump((buf), sizeof(type))
#define boa_push(type, buf) (type*)boa_buf_push((buf), sizeof(type))
#define boa_insert(type, buf, pos) (type*)boa_buf_insert((buf), (pos) * sizeof(type), sizeof(type))
#define boa_reserve_n(type, buf, n) (type*)boa_buf_reserve((buf), (n) * sizeof(type))
#define boa_push_n(type, buf, n) (type*)boa_buf_push((buf), (n) * sizeof(type))
#define boa_insert_n(type, buf, pos, n) (type*)boa_buf_insert((buf), (pos) * sizeof(type), (n) * sizeof(type))

#define boa_push_val(type, buf, val) (*(type*)boa_check_ptr(boa_push(type, buf)) = (val))
#define boa_insert_val(type, buf, pos, val) (*(type*)boa_check_ptr(boa_insert(type, buf, pos)) = (val))

#define boa_get(type, buf, pos) (*(type*)boa_buf_get((buf), (pos) * sizeof(type), sizeof(type)))
#define boa_get_n(type, buf, pos, n) ((type*)boa_buf_get((buf), (pos) * sizeof(type), (n) * sizeof(type)))

#define boa_bytesleft(buf) ((buf)->cap_pos - (buf)->end_pos)

#endif
