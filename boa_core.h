
#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

#include <stdint.h>

#define boa_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))

typedef struct boa_buf {
	void *buf_ptr, *ptr;
	size_t buf_cap, cap;
} boa_buf;

static inline boa_buf boa_buf_empty()
{
	boa_buf buf = { 0 };
	return buf;
}

static inline boa_buf boa_buf_make(void *buffer, size_t cap)
{
	boa_buf buf = { buffer, buffer, cap, cap };
	return buf;
}

#define boa_buf_array(arr) boa_buf_make((arr), sizeof(arr))

static inline void *boa_buf_alloc(boa_buf *buf, size_t size) {
	extern void *boa__internal_buf_alloc(boa_buf *buf, size_t size);
	if (size <= buf->cap) return buf->ptr;
	return boa__internal_buf_alloc(buf, size);
}

static inline void *boa_buf_realloc(boa_buf *buf, size_t size) {
	extern void *boa__internal_buf_realloc(boa_buf *buf, size_t size);
	if (size <= buf->cap) return buf->ptr;
	return boa__internal_buf_realloc(buf, size);
}

static inline void boa_buf_reset(boa_buf *buf) {
	extern void boa__internal_buf_reset(boa_buf *buf);
	if (buf->buf_ptr == buf->ptr) return;
	boa__internal_buf_reset(buf);
}

typedef struct boa_vec {
	boa_buf buf;
	size_t byte_pos;
} boa_vec;

static inline boa_vec boa_vec_empty()
{
	boa_vec vec = { 0 };
	return vec;
}

static inline boa_vec boa_vec_make(void *buffer, size_t cap)
{
	boa_vec vec = { buffer, buffer, cap, cap, 0 };
	return vec;
}

#define boa_vec_array(arr) boa_vec_make((arr), sizeof(arr))

static inline void *boa_vec_push(boa_vec *vec, size_t size) {
	extern void *boa__internal_vec_resize(boa_vec *vec, size_t size);
	size_t pos = vec->byte_pos;
	size_t cap = pos + size;
	if (cap <= vec->buf.cap) {
		vec->byte_pos = cap;
		return (char*)vec->buf.ptr + pos;
	} else {
		return boa__internal_vec_resize(vec, cap);
	}
}

#define boa_vpush(type, vec) (type*)boa_vec_push(&(vec), sizeof(type))
#define boa_vadd(type, vec, value) (*(type*)boa_vec_push(&(vec), sizeof(type)) = (value))
#define boa_vcount(type, vec) ((vec).byte_pos / sizeof(type))
#define boa_vbegin(type, vec) ((type*)((vec).buf.ptr))
#define boa_vend(type, vec) (type*)((char*)(vec).buf.ptr + (vec).byte_pos)
#define boa_vbytesleft(vec) ((vec).buf.cap - (vec).byte_pos)

static inline boa_vec_reset(boa_vec *vec)
{
	boa_buf_reset(&vec->buf);
	vec->byte_pos = 0;
}

char *boa_format(boa_buf *buf, const char *fmt, ...);
void boa_format_push(boa_vec *vec, const char *fmt, ...);

void *boa_malloc(size_t size);
void *boa_realloc(void *ptr, size_t size);
void *boa_try_realloc(void *ptr, size_t size);
void boa_free(void *ptr);

#endif

#if defined(BOA_IMPLEMENTATION)
#ifndef BOA__CORE_IMPLEMENTED
#define BOA__CORE_IMPLEMENTED

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void *boa__internal_buf_alloc(boa_buf *buf, size_t size) {
	char *ptr;
	if (size < buf->cap * 2)
		size = buf->cap * 2;

	ptr = boa_malloc(size);

	if (ptr) {
		if (buf->ptr != buf->buf_ptr) {
			boa_free(buf->ptr);
		}

		buf->ptr = ptr;
		buf->cap = size;
		return buf->ptr;
	} else {
		return NULL;
	}
}

void *boa__internal_buf_realloc(boa_buf *buf, size_t size) {
	char *ptr;
	if (size < buf->cap * 2)
		size = buf->cap * 2;

	if (buf->ptr != buf->buf_ptr) {
		ptr = boa_try_realloc(buf->ptr, size);
	} else {
		ptr = boa_malloc(size);
		if (ptr && buf->buf_cap > 0) {
			memcpy(ptr, buf->buf_ptr, buf->buf_cap);
		}
	}

	if (ptr) {
		buf->ptr = ptr;
		buf->cap = size;
		return buf->ptr;
	} else {
		return NULL;
	}
}

void boa__internal_buf_reset(boa_buf *buf)
{
	boa_free(buf->ptr);
	buf->cap = buf->buf_cap;
	buf->ptr = buf->buf_ptr;
}

extern void *boa__internal_vec_resize(boa_vec *vec, size_t cap)
{
	char *ptr = (char*)boa__internal_buf_realloc(&vec->buf, cap);
	size_t pos = vec->byte_pos;
	if (!ptr) return NULL;
	vec->byte_pos = cap;
	return ptr + pos;
}

char *boa_format(boa_buf *buf, const char *fmt, ...)
{
	boa_buf zerobuf;
	if (!buf) {
		zerobuf = boa_buf_empty();
		buf = &zerobuf;
	}

	char *ptr = (char*)buf->ptr;
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsnprintf(ptr, buf->cap, fmt, args);
	va_end(args);

	if (len >= (int)buf->cap) {
		ptr = boa_buf_alloc(buf, len + 1);
		if (!ptr) {
			return NULL;
		}

		va_start(args, fmt);
		vsnprintf(ptr, buf->cap, fmt, args);
		va_end(args);
	}

	return ptr;
}

void boa_format_push(boa_vec *vec, const char *fmt, ...)
{
	char *ptr = boa_vend(char, *vec);
	va_list args;
	int len;
	int space = (int)boa_vbytesleft(*vec);

	va_start(args, fmt);
	len = vsnprintf(ptr, space, fmt, args);
	va_end(args);

	if (len >= space) {
		ptr = (char*)boa_vec_push(vec, len + 1);
		if (!ptr) {
			return;
		}

		va_start(args, fmt);
		vsnprintf(ptr, boa_vend(char, *vec) - ptr, fmt, args);
		va_end(args);

		vec->byte_pos--;
	} else {
		vec->byte_pos += len;
	}

	return;
}

#if defined(BOA_USE_TEST_ALLOCATORS) && !defined(BOA__HAS_TEST_ALLOCATORS)
	#error "Requesting test allocators, but they didn't get enabled in time, include boa_test.h first!"
#endif

#ifndef BOA_MALLOC
	#define BOA_MALLOC malloc
#endif

#ifndef BOA_REALLOC
	#define BOA_REALLOC realloc
#endif

#ifndef BOA_FREE
	#define BOA_FREE free
#endif

void *boa_malloc(size_t size)
{
	return BOA_MALLOC(size);
}

void *boa_realloc(void *ptr, size_t size)
{
	void *new_ptr = boa_try_realloc(ptr, size);
	if (!new_ptr) boa_free(ptr);
	return new_ptr;
}

void *boa_try_realloc(void *ptr, size_t size)
{
	return BOA_REALLOC(ptr, size);
}

void boa_free(void *ptr)
{
	BOA_FREE(ptr);
}

#endif
#endif

