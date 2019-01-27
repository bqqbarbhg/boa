#pragma once

#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

#include <stdint.h>

// -- Platform

// Compiler
#define BOA_MSVC 0  // < Miscosoft Visual C++
#define BOA_GNUC 0  // GCC or Clang
#define BOA_CLANG 0 // Clang only

// Architecture
#define BOA_64BIT 0 // < 64-bit arch
#define BOA_X86 0   // < x86 or x64 depending on BOA_64BIT

// Operating system
#define BOA_WINDOWS 0 // < Windows
#define BOA_LINUX 0   // < Linux (or Linux like)

#if !defined(BOA_SINGLETHREADED)
	#define BOA_SINGLETHREADED 0
#else
	#undef BOA_SINGLETHREADED
	#define BOA_SINGLETHREADED 1
#endif

#if !defined(BOA_RELEASE)
	#define BOA_RELEASE 0
#else
	#undef BOA_RELEASE
	#define BOA_RELEASE 1
#endif

#if defined(_MSC_VER)
	#undef BOA_MSVC
	#define BOA_MSVC 1
#elif defined(__GNUC__) || defined(__clang__)
	#undef BOA_GNUC
	#define BOA_GNUC 1
#else
	#error "Unsupported compiler"
#endif

#if defined(__clang__)
	#undef BOA_CLANG
	#define BOA_CLANG 1
#endif

#if UINTPTR_MAX == UINT64_MAX
	#undef BOA_64BIT
	#define BOA_64BIT 1
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_IX86)
	#undef BOA_X86
	#define BOA_X86 1
#else
	#error "Unsupported architecture"
#endif

#if defined(_WIN32)
	#undef BOA_WINDOWS
	#define BOA_WINDOWS 1
#elif defined(__linux__)
	#undef BOA_LINUX
	#define BOA_LINUX 1
#else
	#error "Unsupported OS"
#endif

#if BOA_MSVC
	#include <intrin.h>
#endif

// -- Language

#include <stddef.h>
#include <string.h>

#define boa_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))
#define boa_arrayend(arr) (arr + (sizeof(arr) / sizeof(*(arr))))

#if defined(__cplusplus)
	#define boa_inline inline
#else
	#define boa_inline static
#endif

#if BOA_MSVC
	#define boa_forceinline static __forceinline
#elif BOA_GNUC
	#define boa_forceinline static __attribute__((always_inline))
#else
	#define boa_forceinline static boa_inline
#endif

#if BOA_MSVC
	#define boa_noinline __declspec(noinline)
#elif BOA_GNUC
	#define boa_noinline __attribute__((noinline))
#else
	#define boa_noinline
#endif

#if BOA_SINGLETHREADED
	#define boa_threadlocal
#elif __cplusplus >= 201103L
	#define boa_threadlocal thread_local
#elif __STDC_VERSION__ >= 201112L
	#define boa_threadlocal _Thread_local
#elif BOA_MSVC
	#define boa_threadlocal __declspec(thread)
#elif BOA_GNUC
	#define boa_threadlocal __thread
#else
	#error "Unsupported platform"
#endif

#if __cplusplus >= 201103L
	#define boa_alignof(type) alignof(type)
#elif __STDC_VERSION__ >= 201112L
	#define boa_alignof(type) _Alignof(type)
#elif BOA_GNUC
	#define boa_alignof(type) __alignof__(type)
#elif BOA_MSVC
	#define boa_alignof(type) __alignof(type)
#else
	#define boa_alignof(type) ((sizeof(type) & 7) == 0 ? 8 : ((sizeof(type) & 3) == 0 ? 4 : ((sizeof(type) & 1) == 0) ? 2 : 1))
#endif

#define BOA__CONCAT_STEP(x, y) x ## y
#define BOA_CONCAT(x, y) BOA__CONCAT_STEP(x, y)

// -- boa_assert

#if !defined(boa_assert) && !defined(boa_assertf)

#if BOA_RELEASE && BOA_CLANG
	#define boa_assert(cond) __builtin_assume(cond)
	#define boa_assertf(cond, ...) __builtin_assume(cond)
#elif BOA_RELEASE
	#define boa_assert(cond) (void)0
	#define boa_assertf(cond, ...) (void)0
#elif BOA_MSVC
	#define boa_assert(cond) if (!(cond)) __debugbreak()
	#define boa_assertf(cond, ...) if (!(cond)) __debugbreak()
#elif BOA_GNUC
	#define boa_assert(cond) if (!(cond)) __builtin_trap()
	#define boa_assertf(cond, ...) if (!(cond)) __builtin_trap()
#else
	#include <assert.h>
	#define boa_assert(cond) assert((cond))
	#define boa_assertf(cond, ...) assert((cond))
#endif

#elif !defined(boa_assert)
	#error "Custom assert defined without boa_assert()"
#elif !defined(boa_assertf)
	#error "Custom assert defined without boa_assertf()"
#endif

boa_forceinline void *boa_check_ptr(const void *ptr)
{
	boa_assert(ptr != NULL);
	return (void*)ptr;
}

// -- Utility

boa_forceinline uint32_t boa_align_up(uint32_t value, uint32_t align)
{
	boa_assert(align != 0 && (align & align - 1) == 0);
	return (value + align - 1) & ~(align - 1);
}

uint32_t boa_round_pow2_up(uint32_t value);
uint32_t boa_round_pow2_down(uint32_t value);

boa_forceinline uint32_t boa_highest_bit(uint32_t value)
{
	boa_assert(value != 0);

#if BOA_MSVC
	unsigned long result;
	_BitScanReverse(&result, value);
	return result;
#elif BOA_GNUC
	return 31 - __builtin_clz(value);
#else
	#error "Unimplemented"
#endif
}

boa_forceinline void boa_swap_inline(void *a, void *b, uint32_t size)
{
	char *pa = (char*)a, *pb = (char*)b;

#if BOA_64BIT
	if ((size & 7) == 0) {
		while (size >= 8) {
			uint64_t temp = *(uint64_t*)pa;
			*(uint64_t*)pa = *(uint64_t*)pb;
			*(uint64_t*)pb = temp;

			size -= 8;
			pa += 8;
			pb += 8;
		}
	}
#endif

	if ((size & 3) == 0) {
		while (size >= 4) {
			uint32_t temp = *(uint32_t*)pa;
			*(uint32_t*)pa = *(uint32_t*)pb;
			*(uint32_t*)pb = temp;

			size -= 4;
			pa += 4;
			pb += 4;
		}
	}

	while (size > 0) {
		char temp = *pa;
		*pa = *pb;
		*pb = temp;

		size -= 1;
		pa += 1;
		pb += 1;
	}
}

void boa_swap(void *a, void *b, uint32_t size);

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

#define boa_make(type) (type*)boa_alloc(sizeof(type))
#define boa_make_ator(type, ator) (type*)boa_alloc_ator((ator), sizeof(type))
#define boa_make_n(type, n) (type*)boa_alloc((n) * sizeof(type))
#define boa_make_n_ator(type, n, ator) (type*)boa_alloc_ator((ator), (n) * sizeof(type))

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


boa_forceinline boa_buf
boa_buf_make(void *data, uint32_t cap, boa_allocator *ator)
{
	boa_buf b = { (uintptr_t)ator, data, 0, cap };
	return b;
}

boa_forceinline boa_buf *
boa_clear(boa_buf *buf)
{
	buf->end_pos = 0;
	return buf;
}

boa_forceinline boa_buf *
boa_reset(boa_buf *buf)
{
	extern void boa__buf_reset_heap(boa_buf *buf);
	buf->end_pos = 0;
	if (buf->ator_flags & BOA_BUF_FLAG_ALLOCATED)
		boa__buf_reset_heap(buf);
	return buf;
}

boa_forceinline void *
boa_buf_reserve(boa_buf *buf, uint32_t size)
{
	extern void *boa__buf_grow(boa_buf *buf, uint32_t req_cap);
	uint32_t end = buf->end_pos, cap = buf->cap_pos;
	uint32_t req_cap = end + size;
	if (req_cap <= cap) return (char*)buf->data + end;
	return boa__buf_grow(buf, req_cap);
}

boa_forceinline void *
boa_buf_reserve_cap(boa_buf *buf, uint32_t size)
{
	extern void *boa__buf_grow(boa_buf *buf, uint32_t req_cap);
	uint32_t cap = buf->cap_pos;
	uint32_t req_cap = cap + size;
	return boa__buf_grow(buf, req_cap);
}

boa_forceinline void
boa_buf_bump(boa_buf *buf, uint32_t size)
{
	boa_assert(buf->end_pos + size <= buf->cap_pos);
	buf->end_pos += size;
}

boa_forceinline void *
boa_buf_push(boa_buf *buf, uint32_t size)
{
	void *ptr = boa_buf_reserve(buf, size);
	if (ptr) boa_buf_bump(buf, size);
	return ptr;
}

boa_forceinline int
boa_buf_push_data(boa_buf *buf, const void *data, uint32_t size)
{
	void *ptr = boa_buf_push(buf, size);
	if (ptr) {
		memcpy(ptr, data, size);
		return 1;
	} else {
		return 0;
	}
}

boa_forceinline int
boa_buf_push_buf(boa_buf *dst, const boa_buf *src)
{
	return boa_buf_push_data(dst, src->data, src->end_pos);
}

boa_forceinline int
boa_buf_push_str(boa_buf *dst, const char *str)
{
	return boa_buf_push_data(dst, src, strlen(str));
}

boa_forceinline boa_allocator *
boa_buf_ator(boa_buf *buf)
{
	return (boa_allocator*)(buf->ator_flags & ~(uintptr_t)BOA_BUF_FLAG_MASK);
}

boa_forceinline void *
boa_buf_get(boa_buf *buf, uint32_t offset, uint32_t size)
{
	boa_assert(offset + size <= buf->end_pos);
	return (char*)buf->data + offset;
}

boa_forceinline void
boa_buf_remove(boa_buf *buf, uint32_t offset, uint32_t size)
{
	char *data = (char*)buf->data;
	uint32_t end_pos = buf->end_pos;
	boa_assert(offset + size <= end_pos);
	boa_assert(offset == end_pos - size || offset + size <= end_pos - size);
	if (offset + size < end_pos) {
		memcpy(data + offset, data + end_pos - size, size);
	}
	buf->end_pos = end_pos - size;
}

boa_forceinline void
boa_buf_erase(boa_buf *buf, uint32_t offset, uint32_t size)
{
	char *data = (char*)buf->data;
	uint32_t end_pos = buf->end_pos;
	boa_assert(offset + size <= end_pos);
	uint32_t end = offset + size;
	uint32_t shift = end_pos - end;
	if (shift > 0) {
		memmove(data + offset, data + end, shift);
	}
	buf->end_pos = end_pos - size;
}

boa_forceinline void *
boa_buf_pop(boa_buf *buf, uint32_t size)
{
	char *data = (char*)buf->data;
	boa_assert(size <= buf->end_pos);
	buf->end_pos -= size;
	return data + buf->end_pos;
}

int boa_buf_set_ator(boa_buf *buf, boa_allocator *ator);

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size);

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
#define boa_reserve_cap(type, buf) (type*)boa_buf_reserve_cap((buf), sizeof(type))
#define boa_bump(type, buf) boa_buf_bump((buf), sizeof(type))
#define boa_push(type, buf) (type*)boa_buf_push((buf), sizeof(type))
#define boa_push_data(buf, data) boa_buf_push_data((buf), (data), sizeof(*(data)))
#define boa_insert(type, buf, pos) (type*)boa_buf_insert((buf), (pos) * sizeof(type), sizeof(type))
#define boa_pop(type, buf) (*(type*)boa_buf_pop((buf), sizeof(type)))
#define boa_get(type, buf, pos) (*(type*)boa_buf_get((buf), (pos) * sizeof(type), sizeof(type)))

#define boa_reserve_n(type, buf, n) (type*)boa_buf_reserve((buf), (n) * sizeof(type))
#define boa_reserve_cap_n(type, buf, n) (type*)boa_buf_reserve_cap((buf), (n) * sizeof(type))
#define boa_bump_n(type, buf, n) boa_buf_bump((buf), (n) * sizeof(type))
#define boa_push_n(type, buf, n) (type*)boa_buf_push((buf), (n) * sizeof(type))
#define boa_push_data_n(buf, data, n) boa_buf_push_data((buf), (data), (n) * sizeof(*(data)))
#define boa_insert_n(type, buf, pos, n) (type*)boa_buf_insert((buf), (pos) * sizeof(type), (n) * sizeof(type))
#define boa_pop_n(type, buf, n) (type*)boa_buf_pop((buf), (n) * sizeof(type))
#define boa_get_n(type, buf, pos, n) ((type*)boa_buf_get((buf), (pos) * sizeof(type), (n) * sizeof(type)))

#define boa_is_empty(buf) ((buf)->end_pos == 0)
#define boa_non_empty(buf) ((buf)->end_pos > 0)

#define boa_push_val(type, buf, val) (*(type*)boa_check_ptr(boa_push(type, buf)) = (val))
#define boa_insert_val(type, buf, pos, val) (*(type*)boa_check_ptr(boa_insert(type, buf, pos)) = (val))


#define boa_remove(type, buf, pos) boa_buf_remove((buf), (pos) * sizeof(type), sizeof(type))
#define boa_erase(type, buf, pos) boa_buf_erase((buf), (pos) * sizeof(type), sizeof(type))
#define boa_erase_n(type, buf, pos, n) boa_buf_erase((buf), (pos) * sizeof(type), (n) * sizeof(type))

#define boa_bytesleft(buf) ((buf)->cap_pos - (buf)->end_pos)

#define boa_for(type, name, buf) for (type *name = boa_begin(type, buf), *name##__end = boa_end(type, buf); name != name##__end; name++)

// -- boa_format

char *boa_format(boa_buf *buf, const char *fmt, ...);

typedef int (*boa_format_fn)(boa_buf *buf, const void *data, size_t size);
int boa_format_null(boa_buf *buf, const void *data, size_t size);
int boa_format_u32(boa_buf *buf, const void *data, size_t size);

/*
	-- boa_map: General purpose hash container.
	A boa_map consists of multiple entries: values in a set or key-value pairs in
	a map. The map is initialized with the desired entry size, to insert and find
	entries you need to define a hash and a comparison function.

	Implementation details:
	A map consists of one or more equally sized blocks. A block containing N entries
	has 2N slots that either point to an entry or are free. If a block is filled
	before the map reaches its load factor an auxilary block is allocated and linked
	to the current block. When inserting and finding entries the block is determined
	by the high bits of the hash and slot inside the block by the low bits. Hash
	collisions are resolved using Robin Hood hashing.
*/

#define BOA__MAP_LOWBITS 10
#define BOA__MAP_LOWMASK ((1 << BOA__MAP_LOWBITS) - 1)
#define BOA__MAP_HIGHMASK (~(uint32_t)BOA__MAP_LOWMASK)
#define BOA__MAP_BLOCK_SHIFT BOA__MAP_LOWBITS
#define BOA__MAP_BLOCK_MAX_SLOTS 128
#define BOA__MAP_BLOCK_MAX_ENTRIES 64

typedef struct boa__map_impl {
	uint32_t num_hash_blocks;   // < Number of primary blocks in the map, must be a power of two.
	uint32_t num_total_blocks;  // < Total number of (primary + auxilary) blocks
	uint32_t num_used_blocks;   // < Number of active (primary + auxilary) blocks

	uint8_t block_num_entries; // < Number of user visible entries in a block
	uint8_t entry_block_shift; // < How much to shift by to get from an entry to its block

	// Block metadata, also the root of the allocation to be freed
	struct boa__map_block *blocks;

	// Mapping from hashes to entry indices
	// [0:LOWBITS]  Low bits of the hash (original insert slot)
	// [LOWBITS:16] Entry data index
	uint16_t *entry_slot;

	// Mapping from elements to hash indices
	// [0:LOWBITS]  Current slot index
	// [LOWBITS:32] High bits of the element hash
	uint32_t *hash_cur_slot;

	// Entries per block, each block begins at `block_index * block_num_elements`
	// Entries within the block are in contiguous memory.
	void *entries;

} boa__map_impl;

typedef struct boa_map {
	boa_allocator *ator; // < Allocator to use
	uint32_t entry_size; // < Size of an entry in bytes
	uint32_t count;    // < Number of elements in the map
	uint32_t capacity; // < Maximum amount of elements in the map

	// Implementation details
	boa__map_impl impl;
} boa_map;

typedef struct boa__map_block {
	uint32_t next_aux; // < Index of the next aux block, 0 if none
	uint32_t count;    // < Number of elements in this block
} boa__map_block;

// Return non-zero if `key` is equal to `entry`
typedef int (*boa_map_cmp_fn)(const void *key, const void *entry, void *user);

// Returns the same integer for two `key` values if they are equal
typedef uint32_t (*boa_map_hash_fn)(const void *key, void *user);

typedef struct boa_map_insert_result {
	void *entry;
	int inserted;
} boa_map_insert_result;

typedef struct boa_map_iterator {
	void *entry;
	void *impl_block_end;
} boa_map_iterator;

uint32_t boa__map_find_fallback(boa_map *map, uint32_t block_ix);
boa_map_iterator boa__map_find_block_start(const boa_map *map, uint32_t block_ix);
void *boa__map_remove_non_iter(boa_map *map, void *value);
boa_map_iterator boa__map_find_next(const boa_map *map, const void *value);

#define boa__map_block_num_slots(map) ((map)->impl.block_num_entries << 1)

#define boa__es_make(element, hash) (uint16_t)((element) << BOA__MAP_LOWBITS | (hash) & BOA__MAP_LOWMASK)
#define boa__es_entry_offset(es) ((es) >> BOA__MAP_LOWBITS)
#define boa__es_original_slot(es) ((es) & BOA__MAP_LOWMASK)
#define boa__es_scan_distance(es, slot, mask) (((slot) - (es)) & mask)

#define boa__hcs_make(hash, slot) (uint32_t)((hash) & BOA__MAP_HIGHMASK | (slot))
#define boa__hcs_current_slot(hcs) (uint32_t)((hcs) & BOA__MAP_LOWMASK)
#define boa__hcs_set_slot(hcs, slot) (uint32_t)((hcs) & ~(uint32_t)BOA__MAP_LOWMASK | (slot))

#define boa__map_entry_index_from_block(map, block, offset) ((block) * (map)->impl.block_num_entries + (offset))
#define boa__map_entry_from_index(map, entry_index) ((void*)((char*)(map)->impl.entries + (entry_index) * (map)->entry_size))

// Set the lowest bit of the hash to 1 if bits 1:LOWBITS are 0
#define boa__map_hash_canonicalize(hash) ((hash) | ((uint32_t)((hash) & BOA__MAP_LOWMASK) - 1) >> 31)

// Initialize `map` to hold entries of size `entry_size` using `ator` for alloctions
boa_inline void boa_map_init_ator(boa_map *map, size_t entry_size, boa_allocator *ator) {
	map->ator = ator;
	map->count = 0;
	map->capacity = 0;
	map->entry_size = (uint32_t)entry_size;
	map->impl.blocks = NULL;
}

// Initialize `map` to hold entries of size `entry_size`
boa_inline void boa_map_init(boa_map *map, size_t entry_size) {
	map->ator = NULL;
	map->count = 0;
	map->capacity = 0;
	map->entry_size = (uint32_t)entry_size;
	map->impl.blocks = NULL;
}

// Reserve `capacity` entries to insert into. Note: Does not guarantee that the map doesn't
// reallocate in pathological cases.
int boa_map_reserve(boa_map *map, uint32_t capacity);

// Insert a value into the map.
// Important: This function is a low-level primitive and doesn't copy any data to the
// resulting entry, but it needs to be done by the calling code.
boa_noinline boa_map_insert_result boa_map_insert(boa_map *map, const void *key_ptr, uint32_t hash, boa_map_cmp_fn cmp, void *user);

// Find a value from the map.
boa_noinline void *boa_map_find(const boa_map *map, const void *key_ptr, uint32_t hash, boa_map_cmp_fn cmp, void *user);

// Remove an entry from the map.
boa_forceinline void boa_map_remove(boa_map *map, void *entry) {
	boa__map_remove_non_iter(map, entry);
}

// Remove an entry from the map and return an iterator to the next entry.
boa_forceinline boa_map_iterator
boa_map_remove_iter(boa_map *map, void *entry)
{
	boa_map_iterator result;
	result.entry = entry;
	result.impl_block_end = boa__map_remove_non_iter(map, entry);
	if (result.impl_block_end == NULL) {
		result = boa__map_find_next(map, entry);
	}
	return result;
}

// Convert an entry pointer to an iterator.
boa_map_iterator boa_map_iterate_from(const boa_map *map, const void *entry);

// Get the iterator to the first entry in the map.
boa_forceinline boa_map_iterator boa_map_begin(const boa_map *map) {
	return boa__map_find_block_start(map, 0);
}

// Advance the iterator to the next entry.
boa_forceinline void boa_map_advance(const boa_map *map, boa_map_iterator *it)
{
	boa_assert(it->entry != NULL);
	void *next = (char*)it->entry + map->entry_size;
	if (next == it->impl_block_end) {
		*it = boa__map_find_next(map, it->entry);
	} else {
		it->entry = next;
	}
}

boa_forceinline void *boa__map_begin_for(const boa_map *map, void **impl_end) {
	boa_map_iterator it = boa__map_find_block_start(map, 0);
	*impl_end = it.impl_block_end;
	return it.entry;
}

boa_forceinline void *boa__map_advance_for_block(const boa_map *map, void *entry, void **impl_end) {
	boa_assert(entry != NULL);
	boa_map_iterator it = boa__map_find_next(map, entry);
	*impl_end = it.impl_block_end;
	return it.entry;
}

// Remove all elements from the map. Doesn't free memory.
void boa_map_clear(boa_map *map);

// Reset the map to its initial state. Frees any allocated memory.
void boa_map_reset(boa_map *map);

// Combine two hash values
boa_forceinline uint32_t boa_hash_combine(uint32_t hash, uint32_t value)
{
	return hash ^ (value + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}

// Hash a 32-bit value
boa_forceinline uint32_t boa_u32_hash(uint32_t value)
{
	uint32_t x = value;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

// Inline implementation of `boa_map_insert()`, wrap in a specialized function for better map performance
boa_forceinline boa_map_insert_result
boa_map_insert_inline(boa_map *map, const void *key_ptr, uint32_t hash, boa_map_cmp_fn cmp, void *user)
{
	boa_map_insert_result result;
	result.entry = NULL;
	result.inserted = 0;

	// Rehash before insert for simplicity, also handles edge case of empty map
	if (map->count >= map->capacity) {
		if (!boa_map_reserve(map, map->capacity * 2)) return result;
	}

	hash = boa__map_hash_canonicalize(hash);

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->impl.num_hash_blocks - 1;
	uint32_t block_ix = (hash >> BOA__MAP_BLOCK_SHIFT) & block_mask;
	uint32_t count;
	uint32_t block_num_slots = boa__map_block_num_slots(map);
	uint32_t slot_mask = block_num_slots - 1;

	uint16_t *entry_slot;
	uint32_t slot_ix, es, scan;

	for (;;) {
		count = map->impl.blocks[block_ix].count;
		slot_ix = hash & slot_mask;
		entry_slot = map->impl.entry_slot + block_ix * block_num_slots;
		scan = 0;  // < Number of slots scanned from insertion point

		for (;;) {
			es = entry_slot[slot_ix];

			// Match `LOWMASK` bits of the hash to the element-slot value
			if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
				uint32_t entry_offset = boa__es_entry_offset(es);
				uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, entry_offset);
				void *entry = boa__map_entry_from_index(map, entry_index);
				if (cmp(key_ptr, entry, user)) {
					result.entry = entry;
					return result;
				}
			}

			// If we find an empty slot or one with lower scan distance insert here
			uint32_t ref_scan = boa__es_scan_distance(es, slot_ix, slot_mask);
			if (es == 0 || ref_scan < scan) {
				break;
			}

			slot_ix = (slot_ix + 1) & slot_mask;
			scan++;
		}

		uint32_t next_ix = map->impl.blocks[block_ix].next_aux;
		if (next_ix == 0) {
			if (count >= map->impl.block_num_entries) {
				next_ix = boa__map_find_fallback(map, block_ix);
				if (next_ix == ~0u) return result;
			}
		}

		if (next_ix) {
			block_ix = next_ix;
		} else {
			break;
		}
	}

	// Insert as last entry of the block
	entry_slot[slot_ix] = boa__es_make(count, hash);
	uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, count);
	void *entry = boa__map_entry_from_index(map, entry_index);
	map->impl.hash_cur_slot[entry_index] = boa__hcs_make(hash, slot_ix);
	map->impl.blocks[block_ix].count = count + 1;
	map->count++;

	result.entry = entry;
	result.inserted = 1;

	// While we're holding a displaced slot insert it somewhere
	while (es != 0) {
		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;

		uint32_t next_es = entry_slot[slot_ix];
		uint32_t ref_scan = (slot_ix - next_es) & slot_mask;
		if (next_es == 0 || ref_scan < scan) {
			uint32_t entry_offset = boa__es_entry_offset(es);
			uint32_t entry = boa__map_entry_index_from_block(map, block_ix, entry_offset);
			uint32_t hcs = map->impl.hash_cur_slot[entry];
			map->impl.hash_cur_slot[entry] = boa__hcs_set_slot(hcs, slot_ix);

			entry_slot[slot_ix] = es;
			es = next_es;
		}
	}

	return result;
}

// Inline implementation of `boa_map_find()`, wrap in a specialized function for better map performance
boa_forceinline void *
boa_map_find_inline(const boa_map *map, const void *key_ptr, uint32_t hash, boa_map_cmp_fn cmp, void *user)
{
	// Edge case: Rest of the boa_map struct can be invalid when empty
	if (map->count == 0) return NULL;

	hash = boa__map_hash_canonicalize(hash);

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->impl.num_hash_blocks - 1;
	uint32_t block_ix = (hash >> BOA__MAP_BLOCK_SHIFT) & block_mask;
	uint32_t block_num_slots = boa__map_block_num_slots(map);
	uint32_t slot_mask = block_num_slots - 1;

	do {
		uint32_t slot_ix = hash & slot_mask;
		uint16_t *entry_slot = map->impl.entry_slot + block_ix * block_num_slots;
		uint32_t scan = 0;

		for (;;) {
			uint32_t es = entry_slot[slot_ix];

			// Match `LOWMASK` bits of the hash to the element-slot value
			if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
				uint32_t entry_offset = boa__es_entry_offset(es);
				uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, entry_offset);
				void *entry = boa__map_entry_from_index(map, entry_index);
				if (cmp(key_ptr, entry, user)) {
					return entry;
				}
			}

			// If we find an empty slot or one with lower scan fail find
			uint32_t ref_scan = boa__es_scan_distance(es, slot_ix, slot_mask);
			if (es == 0 || ref_scan < scan) {
				break;
			}

			slot_ix = (slot_ix + 1) & slot_mask;
			scan++;
		}

		block_ix = map->impl.blocks[block_ix].next_aux;
	} while (block_ix != 0);

	return NULL;
}

#define boa_map_for(type, name, map) for ( \
	type *name##__end, *name = (type*)boa__map_begin_for(map, (void**)&name##__end); name; \
	name = (name + 1 != name##__end ? name + 1 : (type*)boa__map_advance_for_block(map, name, (void**)&name##__end)))

// Blit map: Bitwise hash and compare key
boa_noinline boa_map_insert_result boa_blit_map_insert(boa_map *map, const void *key_ptr, uint32_t key_size);
boa_noinline void *boa_blit_map_find(const boa_map *map, const void *key_ptr, uint32_t key_size);

// Pointer map
boa_noinline boa_map_insert_result boa_ptr_map_insert(boa_map *map, const void *key);
boa_noinline void *boa_ptr_map_find(const boa_map *map, const void *key);

// uint32_t map
boa_noinline boa_map_insert_result boa_u32_map_insert(boa_map *map, uint32_t key);
boa_noinline void *boa_u32_map_find(const boa_map *map, uint32_t key);

// -- boa_heap

typedef int (*boa_before_fn)(const void *a, const void *b, void *user);

boa_forceinline void boa_upheap_inline(void *values, uint32_t index, uint32_t size, boa_before_fn before, void *user)
{
	char *data = (char*)values;
	void *index_v = data + index * size;
	while (index > 0) {
		uint32_t parent = (index - 1) >> 1;
		void *parent_v = data + parent * size;
		if (before(index_v, parent_v, user)) {
			boa_swap_inline(parent_v, index_v, size);

			index_v = parent_v;
			index = parent;
		} else {
			break;
		}
	}
}

boa_forceinline void boa_downheap_inline(void *values, uint32_t end, uint32_t index, uint32_t size, boa_before_fn before, void *user)
{
	char *data = (char*)values;
	uint32_t end_o = end;
	uint32_t index_o = index * size;
	uint32_t child_o = index_o * 2 + size;
	while (child_o < end_o) {
		void *index_v = data + index_o;
		char *child_v = data + child_o;

		if (child_o + size < end_o && before(child_v + size, child_v, user)) {
			child_o += size;
			child_v += size;
		}

		if (before(child_v, index_v, user)) {
			boa_swap_inline(index_v, child_v, size);

			index_o = child_o;
			child_o = index_o * 2 + size;
		} else {
			break;
		}
	}
}

void boa_upheap(void *values, uint32_t index, uint32_t size, boa_before_fn before, void *user);
void boa_downheap(void *values, uint32_t end, uint32_t index, uint32_t size, boa_before_fn before, void *user);

// -- boa_pqueue

boa_forceinline int boa_pqueue_enqueue_inline(boa_buf *buf, const void *value, uint32_t size, boa_before_fn before, void *user)
{
	uint32_t pos = buf->end_pos / size;
	if (!boa_buf_push_data(buf, value, size)) return 0;
	boa_upheap_inline(buf->data, pos, size, before, user);
	return 1;
}

boa_forceinline void boa_pqueue_dequeue_inline(boa_buf *buf, void *value, uint32_t size, boa_before_fn before, void *user)
{
	boa_assert(buf->end_pos >= size);
	memcpy(value, buf->data, size);
	boa_buf_remove(buf, 0, size);
	boa_downheap_inline(buf->data, buf->end_pos, 0, size, before, user);
}

boa_inline int boa_pqueue_enqueue(boa_buf *buf, const void *value, uint32_t size, boa_before_fn before, void *user)
{
	uint32_t pos = buf->end_pos / size;
	if (!boa_buf_push_data(buf, value, size)) return 0;
	boa_upheap(buf->data, pos, size, before, user);
	return 1;
}

boa_inline void boa_pqueue_dequeue(boa_buf *buf, void *value, uint32_t size, boa_before_fn before, void *user)
{
	boa_assert(buf->end_pos >= size);
	memcpy(value, buf->data, size);
	boa_buf_remove(buf, 0, size);
	boa_downheap(buf->data, buf->end_pos, 0, size, before, user);
}

// -- boa_arena

typedef struct boa__arena_impl {
	void *data;
	uint32_t pos, cap;
} boa__arena_impl;

typedef struct boa_arena {
	boa_allocator *ator;
	boa__arena_impl impl;
} boa_arena;

boa_forceinline void boa_arena_init(boa_arena *arena)
{
	arena->ator = NULL;
	arena->impl.data = NULL;
	arena->impl.pos = 0;
	arena->impl.cap = 0;
}

boa_forceinline void boa_arena_init_ator(boa_arena *arena, boa_allocator *ator)
{
	arena->ator = ator;
	arena->impl.data = NULL;
	arena->impl.pos = 0;
	arena->impl.cap = 0;
}

boa_arena *boa_arena_make_ator(uint32_t initial_cap, boa_allocator *ator);

boa_forceinline boa_arena *boa_arena_make(uint32_t initial_cap)
{
	return boa_arena_make_ator(initial_cap, NULL);
}

void *boa__arena_push_page(boa_arena *arena, uint32_t size);

boa_forceinline void *boa_arena_push_size(boa_arena *arena, uint32_t size, uint32_t align) {
	uint32_t pos = arena->impl.pos, cap = arena->impl.cap;
	pos = boa_align_up(pos, align);
	if (pos + size <= cap) {
		arena->impl.pos = pos + size;
		return (char*)arena->impl.data + pos;
	} else {
		return boa__arena_push_page(arena, size);
	}
}

void boa_arena_reset(boa_arena *arena);

#define boa_arena_push(type, arena) (type*)boa_arena_push_size((arena), sizeof(type), boa_alignof(type))
#define boa_arena_push_n(type, arena, n) (type*)boa_arena_push_size((arena), sizeof(type) * (n), boa_alignof(type))

// -- boa_error

typedef struct boa_error_type {
	const char *name;
	const char *description;
	boa_error_format_fn *format;
	size_t size;
} boa_error_type;

typedef struct boa_error {
	const boa_error_type *type;
	boa_error *cause;
	const char *context;

} boa_error;

typedef struct boa_error_buf {
	boa_error *error;
	boa_arena *arena;
} boa_error_buf;

typedef int (*boa_error_format_fn)(boa_buf *buf, const boa_error *error);

boa_forceinline int boa_error_format(boa_buf *buf, const boa_error *error)
{
	boa_assert(buf != NULL && error != NULL);
	return error->type->format(buf, error);
}

boa_forceinline int boa_has_error(boa_error **error)
{
	return error && *error;
}

boa_forceinline void boa_error_reset(boa_error **error)
{
	if (error) {
		boa_arena_reset((*error)->arena);
		*error = NULL;
	}
}

void *boa_error_push(boa_error **error, const boa_error_type *type, const char *context);

int boa_error_simple_format(boa_buf *buf, const boa_error *error);

extern const boa_error_type boa_err_no_space;
extern const boa_error_type boa_err_external;

extern const boa_error_type boa_err_errno;
typedef struct boa_err_errno_data {
	boa_error error;
	int errno;
};

extern const boa_error_type boa_err_win32;
typedef struct boa_err_win32_data {
	boa_error error;
	uint32_t code;
};

extern const boa_error_type boa_err_hresult;
typedef struct boa_err_hresult_data {
	boa_error error;
	uint32_t hresult;
};

#endif
