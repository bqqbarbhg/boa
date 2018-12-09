#pragma once

#ifndef BOA__CORE_CPP_INCLUDED
#define BOA__CORE_CPP_INCLUDED

#include "boa_core.h"
#include <type_traits>

extern boa_allocator boa__null_ator;
extern boa_allocator boa__default_ator;

namespace boa {

// -- boa_allocator

struct allocator : boa_allocator
{
	void *alloc(size_t size) { return alloc_fn(this, size); }
	void *realloc(void *ptr, size_t size) { return realloc_fn(this, ptr, size); }
	void free(void *ptr) { return free_fn(this, ptr); }
};

static inline allocator *boa_null_ator()
{
	return (allocator*)&boa__null_ator;
}

static inline allocator *boa_default_ator()
{
	return (allocator*)&boa__default_ator;
}

template <typename T>
struct buf: boa_buf {
	static_assert(std::is_trivially_destructible<T>::value, "boa::buf doesn't run descturors, use boa::obj_buf instead");

	buf() { }
	~buf() { reset(); }
	explicit buf(const boa_buf &b) : boa_buf(b) { }

	T *begin() { return boa_begin(T, this); }
	T *end() { return boa_begin(T, this); }
	T *cap() { return boa_cap(T, this); }
	const T *begin() const { return boa_begin(const T, this); }
	const T *end() const { return boa_begin(const T, this); }
	const T *cap() const { return boa_cap(const T, this); }
	buf<T>& clear() { boa_clear(this); return *this; }
	buf<T>& reset() { boa_reset(this); return *this; }
	const buf<T>& clear() const { boa_clear(this); return *this; }
	const buf<T>& reset() const { boa_reset(this); return *this; }

	T *reserve() { return boa_reserve(T, this); }
	T *reserve(uint32_t count) { return boa_reserve_n(T, this, count); }
	T *push() { return boa_push(T, this); }
	T *push(uint32_t count) { return boa_push_n(T, this, count); }
	void bump() { return boa_bump(T, this); }
	void bump(uint32_t count) { return boa_bump_n(T, this, count); }
	T *insert(uint32_t pos) { return boa_insert(T, this, pos); }
	T *insert(uint32_t pos, uint32_t count) { return boa_insert_n(T, this, pos, count); }
};

template <typename T> inline buf<T>
empty_view() { return buf<T>(boa_empty_view()); }
template <typename T> inline buf<T>
range_view(T *begin, T *end) { return buf<T>(boa_range_view(begin, end)); }
template <typename T> inline buf<T>
slice_view(T *begin, uint32_t count) { return buf<T>(boa_slice_view(begin, count)); }
template <typename T> inline buf<T>
bytes_view(T *begin, uint32_t size) { return buf<T>(boa_bytes_view(begin, size)); }
template <typename T, int N> inline buf<T>
array_view(T(&arr)[N]) { return buf<T>(boa_slice_view(arr, N)); }

template <typename T> inline buf<T>
empty_buf(boa_allocator *ator) { return buf<T>(boa_empty_buf(ator)); }
template <typename T> inline buf<T>
range_buf(T *begin, T *end, boa_allocator *ator) { return buf<T>(boa_range_buf(begin, end, ator)); }
template <typename T> inline buf<T>
slice_buf(T *begin, uint32_t count, boa_allocator *ator) { return buf<T>(boa_slice_buf(begin, count, ator)); }
template <typename T> inline buf<T>
bytes_buf(T *begin, uint32_t size, boa_allocator *ator) { return buf<T>(boa_bytes_buf(begin, size, ator)); }
template <typename T, int N> inline buf<T>
array_buf(T(&arr)[N], boa_allocator *ator) { return buf<T>(boa_slice_buf(arr, N, ator)); }

template <typename T> inline buf<T>
empty_buf_default() { return buf<T>(boa_empty_buf_default()); }
template <typename T> inline buf<T>
range_buf_default(T *begin, T *end) { return buf<T>(boa_range_buf_default(begin, end)); }
template <typename T> inline buf<T>
slice_buf_default(T *begin, uint32_t count) { return buf<T>(boa_slice_buf_default(begin, count)); }
template <typename T> inline buf<T>
bytes_buf_default(T *begin, uint32_t size) { return buf<T>(boa_bytes_buf_default(begin, size)); }
template <typename T, int N> inline buf<T>
array_buf_default(T(&arr)[N]) { return buf<T>(boa_slice_buf_default(arr, N)); }

template<typename... Args>
inline const char *format(const char *fmt, Args... args) {
	return boa_format(NULL, fmt, args...);
}

template<typename... Args>
inline const char *format(boa_buf &buf, const char *fmt, Args... args) {
	return boa_format(&buf, fmt, args...);
}

}

#endif

