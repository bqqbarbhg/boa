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

boa_inline void *boa_check_ptr(const void *ptr)
{
	boa_assert(ptr != NULL);
	return (void*)ptr;
}

// -- Utility

boa_inline uint32_t boa_align_up(uint32_t value, uint32_t align)
{
	boa_assert(align != 0 && (align & align - 1) == 0);
	return (value + align - 1) & ~(align - 1);
}

uint32_t boa_round_pow2_up(uint32_t value);
uint32_t boa_highest_bit(uint32_t value);

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
#define boa_bump(type, buf) boa_buf_bump((buf), sizeof(type))
#define boa_push(type, buf) (type*)boa_buf_push((buf), sizeof(type))
#define boa_insert(type, buf, pos) (type*)boa_buf_insert((buf), (pos) * sizeof(type), sizeof(type))
#define boa_pop(type, buf) (*(type*)boa_buf_pop((buf), sizeof(type)))
#define boa_get(type, buf, pos) (*(type*)boa_buf_get((buf), (pos) * sizeof(type), sizeof(type)))

#define boa_reserve_n(type, buf, n) (type*)boa_buf_reserve((buf), (n) * sizeof(type))
#define boa_bump_n(type, buf, n) boa_buf_bump((buf), (n) * sizeof(type))
#define boa_push_n(type, buf, n) (type*)boa_buf_push((buf), (n) * sizeof(type))
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

// -- boa_map

#define BOA__MAP_LOWBITS 10
#define BOA__MAP_LOWMASK ((1 << BOA__MAP_LOWBITS) - 1)
#define BOA__MAP_HIGHMASK (~(uint32_t)BOA__MAP_LOWMASK)
#define BOA__MAP_BLOCK_SHIFT BOA__MAP_LOWBITS
#define BOA__MAP_BLOCK_MAX_SLOTS 128
#define BOA__MAP_BLOCK_MAX_ELEMENTS 64

typedef struct boa__map_impl {
	uint32_t kv_size;             // < Size of a key-value pair in bytes
	uint32_t val_offset;          // < Offset of the value from the key in bytes
	uint32_t num_hash_blocks;     // < Number of primary blocks in the map, must be a power of two.
	uint32_t num_total_blocks;    // < Total number of (primary + auxilary) blocks
	uint32_t num_used_blocks;     // < Number of used (primary + auxilary) blocks
	uint32_t block_num_slots;     // < Number of slots (hash locations) in a block
	uint32_t block_num_elements;  // < Number of elements (key-value pairs) in a block
	uint32_t element_block_shift; // < How much to shift by to get from element its block

	// Combined allocation pointer
	void *allocation;

	// Block metadata
	struct boa__map_block *blocks;

	// Mapping from hashes to element indices
	// [0:LOWBITS]  Original slot index (low BOA__MAP_LOWBITS bits of hash)
	// [LOWBITS:16] Element data index
	uint16_t *element_slot;

	// Mapping from elements to hash indices
	// [0:LOWBITS]  Current slot index
	// [LOWBITS:32] High bits of the element hash
	uint32_t *hash_cur_slot;

	// Data elements per block, each block begins at `block_index * block_num_elements`
	// Elements within the block are in contiguous memory
	void *data_blocks;

} boa__map_impl;

typedef struct boa_map {

	// Configuration
	boa_allocator *ator; // < Allocator to use
	uint32_t key_size;   // < Size of the key type in bytes
	uint32_t val_size;   // < Value of the key type in bytes

	uint32_t count;    // < Number of elements in the map
	uint32_t capacity; // < Maximum amount of elements in the map

	// Implementation details
	boa__map_impl impl;
} boa_map;

typedef struct boa__map_block {
	uint32_t next_aux; // < Index of the next aux block, 0 if none
	uint32_t count;    // < Number of elements in this block
} boa__map_block;

// Return non-zero if `a` is equal to `b`
typedef int (*boa_map_cmp_fn)(const void *a, const void *b, boa_map *map);

typedef struct boa_map_insert_result {
	void *value;
	int inserted;
} boa_map_insert_result;

typedef struct boa_map_iterator {
	void *value;
	void *block_end;
} boa_map_iterator;

uint32_t boa__map_find_fallback(boa_map *map, uint32_t block_ix);
boa_map_iterator boa__map_find_block_start(boa_map *map, uint32_t block_ix);
void *boa__map_remove_non_iter(boa_map *map, void *value);
boa_map_iterator boa__map_find_next(boa_map *map, void *value);

#define boa__es_make(element, hash) (uint16_t)((element) << BOA__MAP_LOWBITS | (hash) & BOA__MAP_LOWMASK)
#define boa__es_element_offset(es) ((es) >> BOA__MAP_LOWBITS)
#define boa__es_original_slot(es) ((es) & BOA__MAP_LOWMASK)
#define boa__es_scan_distance(es, slot, mask) (((slot) - (es)) & mask)

#define boa__hcs_make(hash, slot) (uint32_t)((hash) & BOA__MAP_HIGHMASK | (slot))
#define boa__hcs_current_slot(hcs) (uint32_t)((hcs) & BOA__MAP_LOWMASK)
#define boa__hcs_set_slot(hcs, slot) (uint32_t)((hcs) & ~(uint32_t)BOA__MAP_LOWMASK | (slot))

#define boa__map_element_from_block(map, block, offset) ((block) * (map)->impl.block_num_elements + (offset))
#define boa__map_kv_from_element(map, element) ((void*)((char*)(map)->impl.data_blocks + (element) * (map)->impl.kv_size))

// Set the lowest bit of the hash to 1 if bits 1:LOWBITS are 0
#define boa__map_hash_canonicalize(hash) ((hash) | ((uint32_t)((hash) & BOA__MAP_LOWMASK) - 1) >> 31)

int boa_map_reserve(boa_map *map, uint32_t capacity);

boa_forceinline boa_map_insert_result
boa_map_insert_inline(boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp)
{
	boa_map_insert_result result;
	result.value = NULL;
	result.inserted = 0;

	// Rehash before insert for simplicity, also handles edge case of empty map
	if (map->count >= map->capacity) {
		if (!boa_map_reserve(map, map->capacity * 2)) return result;
	}

	hash = boa__map_hash_canonicalize(hash);

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->impl.num_hash_blocks - 1;
	uint32_t slot_mask = map->impl.block_num_slots - 1;
	uint32_t block_ix = (hash >> BOA__MAP_BLOCK_SHIFT) & block_mask;
	uint32_t count = map->impl.blocks[block_ix].count;

	uint16_t *element_slot;
	uint32_t slot_ix, es, scan;

	for (;;) {
		slot_ix = hash & slot_mask;
		element_slot = map->impl.element_slot + block_ix * map->impl.block_num_slots;
		scan = 0;  // < Number of slots scanned from insertion point

		for (;;) {
			es = element_slot[slot_ix];

			// Match `LOWMASK` bits of the hash to the element-slot value
			if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
				uint32_t element_offset = boa__es_element_offset(es);
				uint32_t element = boa__map_element_from_block(map, block_ix, element_offset);
				void *kv = boa__map_kv_from_element(map, element);
				if (cmp(key, kv, map)) {
					result.value = kv;
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
			if (count >= map->impl.block_num_elements) {
				next_ix = boa__map_find_fallback(map, block_ix);
				if (next_ix == ~0u) return result;
				count = map->impl.blocks[next_ix].count;
			}
		}

		if (next_ix) {
			block_ix = next_ix;
		} else {
			break;
		}
	}

	// Insert as last element of the block
	element_slot[slot_ix] = boa__es_make(count, hash);
	uint32_t element = boa__map_element_from_block(map, block_ix, count);
	void *kv = boa__map_kv_from_element(map, element);
	map->impl.hash_cur_slot[element] = boa__hcs_make(hash, slot_ix);
	map->impl.blocks[block_ix].count = count + 1;
	map->count++;

	result.value = kv;
	result.inserted = 1;

	// While we're holding a displaced slot insert it somewhere
	while (es != 0) {
		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;

		uint32_t next_es = element_slot[slot_ix];
		uint32_t ref_scan = (slot_ix - next_es) & slot_mask;
		if (next_es == 0 || ref_scan < scan) {
			uint32_t element_offset = boa__es_element_offset(es);
			uint32_t element = boa__map_element_from_block(map, block_ix, element_offset);
			uint32_t hcs = map->impl.hash_cur_slot[element];
			map->impl.hash_cur_slot[element] = boa__hcs_set_slot(hcs, slot_ix);

			element_slot[slot_ix] = es;
			es = next_es;
		}
	}

	return result;
}

boa_forceinline void *
boa_map_find_inline(boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp)
{
	// Edge case: Other values may not be valid when empty!
	if (map->count == 0) return NULL;

	hash = boa__map_hash_canonicalize(hash);

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->impl.num_hash_blocks - 1;
	uint32_t slot_mask = map->impl.block_num_slots - 1;
	uint32_t block_ix = (hash >> BOA__MAP_BLOCK_SHIFT) & block_mask;

	do {
		uint32_t slot_ix = hash & slot_mask;
		uint16_t *element_slot = map->impl.element_slot + block_ix * map->impl.block_num_slots;
		uint32_t scan = 0;

		for (;;) {
			uint32_t es = element_slot[slot_ix];

			// Match `LOWMASK` bits of the hash to the element-slot value
			if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
				uint32_t element_offset = boa__es_element_offset(es);
				uint32_t element = boa__map_element_from_block(map, block_ix, element_offset);
				void *kv = boa__map_kv_from_element(map, element);
				if (cmp(key, kv, map)) {
					return kv;
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

boa_noinline boa_map_insert_result boa_map_insert(boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp);
boa_noinline void *boa_map_find(boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp);

boa_forceinline
void boa_map_remove(boa_map *map, void *value)
{
	boa__map_remove_non_iter(map, value);
}

boa_forceinline boa_map_iterator
boa_map_remove_iter(boa_map *map, void *value)
{
	boa_map_iterator result;
	result.value = value;
	result.block_end = boa__map_remove_non_iter(map, value);
	if (result.block_end == NULL) {
		result = boa__map_find_next(map, value);
	}
	return result;
}

boa_map_iterator boa_map_iterate(boa_map *map, void *value);

boa_forceinline boa_map_iterator
boa_map_begin(boa_map *map)
{
	return boa__map_find_block_start(map, 0);
}

boa_forceinline void
boa_map_advance(boa_map *map, boa_map_iterator *it)
{
	boa_assert(it->value != NULL);
	void *next = (char*)it->value + map->impl.kv_size;
	if (next == it->block_end) {
		*it = boa__map_find_next(map, it->value);
	} else {
		it->value = next;
	}
}

void boa_map_clear(boa_map *map);

void boa_map_reset(boa_map *map);

boa_forceinline uint32_t boa_hash_combine(uint32_t hash, uint32_t value)
{
	return hash ^ (value + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}

#define boa_key(type, map, elem) (*(type*)boa_check_ptr(elem))
#define boa_val(type, map, elem) (*(type*)((char*)boa_check_ptr(elem) + (map)->impl.val_offset))

// -- boa_bmap

boa_noinline boa_map_insert_result boa_bmap_insert(boa_map *map, const void *key);
boa_noinline void *boa_bmap_find(boa_map *map, const void *key);

#endif
