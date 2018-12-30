#pragma once

#ifndef BOA__CORE_CPP_INCLUDED
#define BOA__CORE_CPP_INCLUDED

#include "boa_core.h"
#include <type_traits>
#include <utility>
#include <functional>

extern boa_allocator boa__null_ator;
extern boa_allocator boa__default_ator;

namespace boa {

// -- boa_pod

template <typename T>
struct pod {
	alignas(T) char data[sizeof(T)];

	T& operator*() { return *(T*)data; }
	const T& operator*() const { return *(const T*)data; }
	T* operator->() { return (T*)data; }
	const T* operator->() const { return (const T*)data; }
};

// -- boa_allocator

struct allocator : boa_allocator
{
	void *alloc(size_t size) { return alloc_fn(this, size); }
	void *realloc(void *ptr, size_t size) { return realloc_fn(this, ptr, size); }
	void free(void *ptr) { return free_fn(this, ptr); }
};

inline allocator *boa_null_ator()
{
	return (allocator*)&boa__null_ator;
}

inline allocator *boa_default_ator()
{
	return (allocator*)&boa__default_ator;
}

inline void *alloc(size_t size) { return boa_alloc(size); }
inline void *realloc(void *ptr, size_t size) { return boa_realloc(ptr, size); }
inline void free(void *ptr) { boa_free(ptr); }

// -- boa_buf

template <typename T>
struct buf: boa_buf {
	static_assert(std::is_trivially_destructible<T>::value, "boa::buf doesn't run destructors, use boa::obj_buf instead");

	buf(): boa_buf(boa_empty_buf()) { }
	~buf() { reset(); }
	explicit buf(const boa_buf &b) : boa_buf(b) { }

	T *begin() { return boa_begin(T, this); }
	T *end() { return boa_end(T, this); }
	T *cap() { return boa_cap(T, this); }
	const T *begin() const { return boa_begin(const T, this); }
	const T *end() const { return boa_end(const T, this); }
	const T *cap() const { return boa_cap(const T, this); }
	buf<T>& clear() { boa_clear(this); return *this; }
	buf<T>& reset() { boa_reset(this); return *this; }
	const buf<T>& clear() const { boa_clear(this); return *this; }
	const buf<T>& reset() const { boa_reset(this); return *this; }

	T *reserve() { return boa_reserve(T, this); }
	T *push() { return boa_push(T, this); }
	void bump() { return boa_bump(T, this); }
	T *insert(uint32_t pos) { return boa_insert(T, this, pos); }
	T &pop() { *boa_pop(T, this); }

	bool try_push(const T &value) { return (bool)boa_buf_push_data(this, &value, sizeof(T)); }
	void push(const T &value) { boa_push_val(T, this, value); }
	void insert(uint32_t pos, const T &value) { boa_insert_val(T, this, pos, value); }

	T *reserve_n(uint32_t count) { return boa_reserve_n(T, this, count); }
	T *push_n(uint32_t count) { return boa_push_n(T, this, count); }
	void bump_n(uint32_t count) { boa_bump_n(T, this, count); }
	T *insert_n(uint32_t pos, uint32_t count) { return boa_insert_n(T, this, pos, count); }
	T *pop_n(uint32_t count) { boa_pop_n(T, this, count); }

	void remove(uint32_t pos) { boa_remove(T, this, pos); }

	void erase(uint32_t pos) { boa_erase(T, this, pos); }
	void erase_n(uint32_t pos, uint32_t count) { boa_erase_n(T, this, pos, count); }

	T &operator[](uint32_t index) { return boa_get(T, this, index); }
	const T &operator[](uint32_t index) const { return boa_get(const T, (boa_buf*)this, index); }

	uint32_t count() const { return boa_count(T, this); }
	bool is_empty() const { return (bool)boa_is_empty(this); }
	bool non_empty() const { return (bool)boa_non_empty(this); }

	T &from_pos(uint32_t pos) {
		boa_assert(pos % sizeof(T) == 0);
		boa_assert(pos < end_pos);
		return *(T*)((char*)data + pos);
	}
};

template <typename T> inline buf<T>
empty_buf_ator(boa_allocator *ator) { return buf<T>(boa_empty_buf_ator(ator)); }
template <typename T> inline buf<T>
range_buf_ator(T *begin, T *end, boa_allocator *ator) { return buf<T>(boa_range_buf_ator(begin, end, ator)); }
template <typename T> inline buf<T>
slice_buf_ator(T *begin, uint32_t count, boa_allocator *ator) { return buf<T>(boa_slice_buf_ator(begin, count, ator)); }
template <typename T> inline buf<T>
bytes_buf_ator(T *begin, uint32_t size, boa_allocator *ator) { return buf<T>(boa_bytes_buf_ator(begin, size, ator)); }
template <typename T, int N> inline buf<T>
array_buf_ator(T(&arr)[N], boa_allocator *ator) { return buf<T>(boa_slice_buf_ator(arr, N, ator)); }

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

template<typename... Args>
inline char *format(const char *fmt, Args... args) {
	return boa_format(NULL, fmt, args...);
}

template<typename... Args>
inline char *format(boa_buf &buf, const char *fmt, Args... args) {
	return boa_format(&buf, fmt, args...);
}

template <typename T>
T &check_ptr(T *t) { return *(T*)boa_check_ptr(t); }

// -- boa_map

struct blit_hasher: boa_map {
	uint32_t blit_key_size;

	template <typename T> static constexpr
	bool hasher_compatible() { return true; }

	template <typename T>
	void hasher_init() {
		blit_key_size = sizeof(T);
	}

	boa_map_insert_result hasher_insert(const void *key) {
		return boa_blit_map_insert(this, key, blit_key_size);
	}
	void *hasher_find(const void *key) {
		return boa_blit_map_find(this, key, blit_key_size);
	}
};

struct ptr_hasher: boa_map {
	template <typename T> static constexpr
	bool hasher_compatible() { return sizeof(T) == sizeof(void*); }

	template <typename T>
	void hasher_init() { }

	boa_map_insert_result hasher_insert(const void *key) {
		return boa_ptr_map_insert(this, *(const void**)key);
	}
	void *hasher_find(const void *key) {
		return boa_ptr_map_find(this, *(const void**)key);
	}
};

struct u32_hasher: boa_map {
	template <typename T> static constexpr
	bool hasher_compatible() { return sizeof(T) == sizeof(uint32_t); }

	template <typename T>
	void hasher_init() { }

	boa_map_insert_result hasher_insert(const void *key) {
		return boa_u32_map_insert(this, *(const uint32_t*)key);
	}
	void *hasher_find(const void *key) {
		return boa_u32_map_find(this, *(const uint32_t*)key);
	}
};

struct virtual_hasher: boa_map {
	boa_map_cmp_fn virtual_cmp_fn;
	boa_map_hash_fn virtual_hash_fn;

	template <typename T> static constexpr
	bool hasher_compatible() { return true; }

	template <typename T>
	static int virtual_equal(const void *a, const void *b, void *user) {
		return *(const T*)a == *(const T*)b;
	}

	template <typename T>
	static int virtual_hash(const void *a, void *user) {
		return hash(*(const T*)a);
	}

	template <typename T>
	void hasher_init() {
		virtual_cmp_fn = &virtual_equal<T>;
		virtual_hash_fn = &virtual_hash<T>;
	}

	boa_map_insert_result hasher_insert(const void *key) {
		uint32_t hash = virtual_hash_fn(key, NULL);
		return boa_map_insert(this, key, hash, virtual_cmp_fn, NULL);
	}
	void *hasher_find(const void *key) {
		uint32_t hash = virtual_hash_fn(key, NULL);
		return boa_map_find(this, key, hash, virtual_cmp_fn, NULL);
	}
};

template <typename T>
struct inline_hasher: boa_map {
	template <typename Tref> static constexpr
	bool hasher_compatible() { return std::is_same<T, Tref>::value; }

	static int inline_equal(const void *a, const void *b, void *user) {
		return *(const T*)a == *(const T*)b;
	}

	static int inline_hash(const void *a) {
		return hash(*(const T*)a);
	}

	template <typename Tref>
	void hasher_init() { }

	boa_map_insert_result hasher_insert(const void *key) {
		uint32_t hash = virtual_hash_fn(key);
		return boa_map_insert(this, key, hash, &inline_equal);
	}
	void *hasher_find(const void *key) {
		uint32_t hash = inline_hash(key);
		return boa_map_find(this, key, hash, &inline_equal);
	}
};

template <typename Key, typename Val>
struct key_val {
	Key key;
	Val val;
};

template <typename T>
struct insert_result {
	explicit insert_result(const boa_map_insert_result &ires)
		: entry((T*)ires.entry), inserted((bool)ires.inserted)
	{ }

	T *entry;
	bool inserted;

	T &operator*() { return *(T*)entry; }
	T *operator->() { return (T*)entry; }
	const T &operator*() const { return *(const T*)entry; }
	const T *operator->() const { return (const T*)entry; }
};

template <typename T>
struct map_iterator: boa_map_iterator {

	map_iterator<T> &operator++() {
		boa_map_advance(this);
		return *this;
	}

	map_iterator<T> operator++(int) {
		map_iterator<T> res = *this;
		boa_map_advance(this);
		return res;
	}

	T &operator*() { return *(T*)entry; }
	T *operator->() { return (T*)entry; }
	const T &operator*() const { return *(const T*)entry; }
	const T *operator->() const { return (const T*)entry; }
};

template <typename Hasher, typename T>
struct set: Hasher {
	static_assert(Hasher::template hasher_compatible<T>(), "Hasher is incompatible with the type");

	typedef map_iterator<T> iterator;

	set() {
		boa_map_init(this, sizeof(T));
		hasher_init<T>();
	}

	explicit set(boa_allocator *ator) {
		boa_map_init_ator(this, sizeof(T), ator);
		hasher_init<T>();
	}

	~set() {
		boa_map_reset(this);
	}

	void reserve(uint32_t capacity) {
		boa_map_reserve(this, capacity);
	}

	insert_result<T> insert_uninitialized(const T &t) {
		return hasher_insert(&t);
	}

	insert_result<T> insert(const T &t) {
		insert_result<T> ires { hasher_insert(&t) };
		boa_assert(ires.entry);
		if (ires.inserted) *ires.entry = t;
		return ires;
	}

	insert_result<T> try_insert(const T &t) {
		insert_result<T> ires { hasher_insert(&t) };
		if (ires.inserted) *ires.entry = t;
		return ires;
	}

	T *find(const T &t) {
		return (T*)hasher_find(&t);
	}

	iterator begin() { return boa_map_begin(this); }
	iterator end() { return iterator(); }
	iterator iterate_from(T *entry) { return boa_map_iterate_from(this, entry); }
};

template <typename Hasher, typename Key, typename Val>
struct map: Hasher {
	static_assert(Hasher::template hasher_compatible<Key>(), "Hasher is incompatible with the key type");

	typedef key_val<Key, Val> key_val;
	typedef map_iterator<key_val> iterator;

	map() {
		boa_map_init(this, sizeof(key_val));
		hasher_init<Key>();
	}

	explicit map(boa_allocator *ator) {
		boa_map_init_ator(this, sizeof(key_val), ator);
		hasher_init<Key>();
	}

	~map() {
		boa_map_reset(this);
	}

	void reserve(uint32_t capacity) {
		boa_map_reserve(this, capacity);
	}

	insert_result<key_val> insert_uninitialized(const Key &key) {
		insert_result<key_val> ires { hasher_insert(&key) };
		boa_assert(ires.entry);
		return ires;
	}

	insert_result<key_val> try_insert_uninitialized(const Key &key) {
		insert_result<key_val> ires { hasher_insert(&key) };
		return ires;
	}

	insert_result<key_val> insert(const Key &key, const Val &val) {
		insert_result<key_val> ires { hasher_insert(&key) };
		boa_assert(ires.entry);
		if (ires.inserted) {
			ires.entry->key = key;
			ires.entry->val = val;
		}
		return ires;
	}

	insert_result<key_val> try_insert(const Key &key, const Val &val) {
		insert_result<key_val> ires { hasher_insert(&key) };
		if (ires.inserted) {
			ires.entry->key = key;
			ires.entry->val = val;
		}
		return ires;
	}

	insert_result<key_val> insert_or_assign(const Key &key, const Val &val) {
		insert_result<key_val> ires { hasher_insert(&key) };
		boa_assert(ires.entry);
		if (ires.inserted) ires.entry->key = key;
		ires.entry->val = val;
		return ires;
	}

	insert_result<key_val> try_insert_or_assign(const Key &key, const Val &val) {
		insert_result<key_val> ires { hasher_insert(&key) };
		if (ires.inserted) ires.entry->key = key;
		if (ires.entry) ires.entry->val = val;
		return ires;
	}

	key_val *find(const Key &key) {
		return (key_val*)hasher_find(&key);
	}

	iterator begin() { return boa_map_begin(this); }
	iterator end() { return map_iterator<T>(); }
	iterator iterate_from(key_val *entry) { return boa_map_iterate_from(this, entry); }
};

template <typename T> using blit_set = set<blit_hasher, T>;
template <typename T> using ptr_set = set<ptr_hasher, T>;
template <typename T> using u32_set = set<u32_hasher, T>;
template <typename T> using virtual_set = set<virtual_hasher, T>;
template <typename T> using inline_set = set<inline_hasher<T>, T>;
template <typename Key, typename Val> using blit_map = map<blit_hasher, Key, Val>;
template <typename Key, typename Val> using ptr_map = map<ptr_hasher, Key, Val>;
template <typename Key, typename Val> using u32_map = map<u32_hasher, Key, Val>;
template <typename Key, typename Val> using virtual_map = map<virtual_hasher, Key, Val>;
template <typename Key, typename Val> using inline_map = map<inline_hasher<Key>, Key, Val>;

// -- boa_pqueue

template <typename T, typename F>
boa_inline int boa__cpp_functor_before(const void *a, const void *b, void *user)
{
	return (*(F*)user)(*(const T*)a, *(const T*)b);
}

template <typename T, typename Before = std::less<T> >
struct pqueue {
	boa::buf<T> buf;
	Before before;

	pqueue() { }
	explicit pqueue(boa::buf<T> &&buf) : buf(std::move(buf)) { }
	pqueue(boa::buf<T> &&buf, Before before) : buf(std::move(buf)), before(before) { }

	uint32_t count() const { return boa_count(T, &buf); }
	bool is_empty() const { return (bool)boa_is_empty(&buf); }
	bool non_empty() const { return (bool)boa_non_empty(&buf); }

	void enqueue(const T &value) {
		int res = boa_pqueue_enqueue_inline(&buf, &value, sizeof(T), boa__cpp_functor_before<T, Before>, &before);
		boa_assert(res != 0);
	}

	bool try_enqueue(const T &value) {
		int res = boa_pqueue_enqueue_inline(&buf, &value, sizeof(T), boa__cpp_functor_before<T, Before>, &before);
		return res != 0;
	}

	T dequeue() {
		pod<T> result;
		boa_pqueue_dequeue_inline(&buf, &result, sizeof(T), boa__cpp_functor_before<T, Before>, &before);
		return *result;
	}
};

// -- Pod aliases

template <typename T> using pod_buf = pod<buf<T>>;
template <typename T> using pod_blit_set = pod<blit_set<T>>;
template <typename T> using pod_ptr_set = pod<ptr_set<T>>;
template <typename T> using pod_u32_set = pod<u32_set<T>>;
template <typename Key, typename Val> using pod_blit_map = pod<blit_map<Key, Val>>;
template <typename Key, typename Val> using pod_ptr_map = pod<ptr_map<Key, Val>>;
template <typename Key, typename Val> using pod_u32_map = pod<u32_map<Key, Val>>;
template <typename T> using pod_pqueue = pod<pqueue<T>>;

}

#endif

