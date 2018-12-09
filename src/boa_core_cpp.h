#pragma once

#ifndef BOA__CORE_CPP_INCLUDED
#define BOA__CORE_CPP_INCLUDED

#include "boa_core.h"
#include <type_traits>

namespace boa {

inline void *malloc(size_t size) { return boa_malloc(size); }
inline void *realloc(void *ptr, size_t size) { return boa_realloc(ptr, size); }
inline void free(void *ptr) { return boa_free(ptr); }

template <typename T>
struct scoped_buf;

template <typename T>
struct buf: boa_buf {
	static_assert(std::is_trivially_destructible<T>::value, "boa::buf doesn't run descturors, use boa::obj_buf instead")

	buf() { }
	~buf() { reset(); }
	explicit buf(const boa_buf &b) : boa_buf(b) { }

	T *begin() { return boa_begin(T, this); }
	T *end() { return boa_begin(T, this); }
	T *cap() { return boa_cap(T, this); }
	const T *begin() const { return boa_begin(const T, this); }
	const T *end() const { return boa_begin(const T, this); }
	const T *cap() const { return boa_cap(const T, this); }
	buf<T>& clear() const { boa_clear(this); return *this; }
	buf<T>& reset() const { boa_reset(this); return *this; }

	T *reserve() { return boa_reserve(T, this); }
	T *reserve(uint32_t count) { return boa_reserve_n(T, this, count); }
	T *push() { return boa_push(T, this); }
	T *push(uint32_t count) { return boa_push_n(T, this, count); }
	void bump() { return boa_bump(T, this); }
	void bump(uint32_t count) { return boa_bump_n(T, this, count); }
};

template <typename T>
struct obj_buf {
	boa_buf buf;

	~obj_buf() {
		for (auto &t : *this) t.~T();
	}

	T *begin() { return boa_begin(T, &buf); }
	T *end() { return boa_begin(T, &buf); }
	T *cap() { return boa_cap(T, &buf); }
	const T *begin() const { return boa_begin(const T, &buf); }
	const T *end() const { return boa_begin(const T, &buf); }
	const T *cap() const { return boa_cap(const T, &buf); }

	obj_buf<T>& clear() const {
		for (auto &t : *this) t.~T();
		boa_clear(this);
		return *this;
	}

	obj_buf<T>& reset() const {
		for (auto &t : *this) t.~T();
		boa_reset(this);
		return *this;
	}

	T *push() { return boa_push(T, this); }
	T *push(uint32_t count) { return boa_push_n(T, this, count); }
	void bump() { return boa_bump(T, this); }
	void bump(uint32_t count) { return boa_bump_n(T, this, count); }
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
empty_buf() { return buf<T>(boa_empty_buf()); }
template <typename T> inline buf<T>
range_buf(T *begin, T *end) { return buf<T>(boa_range_buf(begin, end)); }
template <typename T> inline buf<T>
slice_buf(T *begin, uint32_t count) { return buf<T>(boa_slice_buf(begin, count)); }
template <typename T> inline buf<T>
bytes_buf(T *begin, uint32_t size) { return buf<T>(boa_bytes_buf(begin, size)); }
template <typename T, int N> inline buf<T>
array_buf(T(&arr)[N]) { return buf<T>(boa_slice_buf(arr, N)); }

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

