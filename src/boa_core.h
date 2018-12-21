#pragma once

#ifndef BOA__CORE_INCLUDED
#define BOA__CORE_INCLUDED

// -- General helpers

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <intrin.h>

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

// -- Utility

boa_inline uint32_t boa_align_up(uint32_t value, uint32_t align)
{
	return (value + align - 1) & ~(align - 1);
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

boa_inline void boa_buf_remove(boa_buf *buf, uint32_t offset, uint32_t size)
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

boa_inline void boa_buf_erase(boa_buf *buf, uint32_t offset, uint32_t size)
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

#define boa_remove(type, buf, pos) boa_buf_remove((buf), (pos) * sizeof(type), sizeof(type))
#define boa_erase(type, buf, pos) boa_buf_erase((buf), (pos) * sizeof(type), sizeof(type))
#define boa_erase_n(type, buf, pos, n) boa_buf_erase((buf), (pos) * sizeof(type), (n) * sizeof(type))

#define boa_get(type, buf, pos) (*(type*)boa_buf_get((buf), (pos) * sizeof(type), sizeof(type)))
#define boa_get_n(type, buf, pos, n) ((type*)boa_buf_get((buf), (pos) * sizeof(type), (n) * sizeof(type)))

#define boa_bytesleft(buf) ((buf)->cap_pos - (buf)->end_pos)

// -- boa_map

#if 0

typedef struct boa_map {
	void *data;
	uint32_t size;
	uint32_t kv_size;
	uint32_t chunk_size;
} boa_map;

typedef struct boa__map_chunk {
	uint32_t link;
	uint32_t next_present;
} boa__map_chunk;

boa_inline uint32_t boa__chunk_present(boa__map_chunk *chunk, uint32_t slot)
{
	return (chunk->next_present >> slot) & 1;
}

boa_inline uint32_t boa__chunk_link(boa__map_chunk *chunk, uint32_t slot)
{
	return chunk->link >> (slot * 4) & 0xf;
}

boa_inline void boa__chunk_set_link(boa__map_chunk *chunk, uint32_t slot, uint32_t link)
{
	chunk->link = (chunk->link & ~(0xf << slot)) | (link << slot);
}

boa_inline uint32_t boa__chunk_next_index(boa__map_chunk *chunk)
{
	return chunk->next_present >> 8;
}

boa_inline void *boa_map_insert(boa_map *map, const void *key, uint32_t hash, int (*cmp)(const void *a, const void *b))
{
	uint32_t mask = map->size - 1;
	uint32_t index = hash & mask;

	uint32_t chunk_ix = index / 8;
	uint32_t slot_ix = index % 8;

	boa__map_chunk *chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);
	char *kv_base = (char*)(chunk + 1);

	void *kv = kv_base + slot_ix * map->kv_size;
	uint32_t present = boa__chunk_present(chunk, slot_ix);

	if (cmp(key, kv) && present) {
		return NULL;
	} else if (!present) {
		return kv;
	} else {
		for (;;) {
			uint32_t link = boa__chunk_link(chunk, slot_ix);
			if (link == slot_ix) {
				uint32_t free = ~(chunk->next_present & 0xff);
				if (free) {
					uint32_t ix = boa_find_and_clear_bit(&free);
					chunk->next_present = (chunk->next_present & ~0xFF) | (~free & 0xFF);
					boa__chunk_set_link(chunk, slot_ix, ix);
					return kv_base + ix * map->kv_size;
				} else {
				}
			}

			slot_ix = link & 7;
			if (link >= 8) {
				chunk_ix = boa__chunk_next_index(chunk);
				chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);
				kv_base = (char*)(chunk + 1);
			}
			kv = kv_base + slot_ix * map->kv_size;
			if (cmp(key, kv)) {
				return NULL;
			}
		}
	}
}

boa_inline void *boa_map_find(boa_map *map, const void *key, uint32_t hash, int (*cmp)(const void *a, const void *b))
{
	uint32_t mask = map->size - 1;
	uint32_t index = hash & mask;

	uint32_t chunk_ix = index / 8;
	uint32_t slot_ix = index % 8;

	boa__map_chunk *chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);
	char *kv_base = (char*)(chunk + 1);

	void *kv = kv_base + slot_ix * map->kv_size;
	uint32_t present = boa__chunk_present(chunk, slot_ix);

	if (cmp(key, kv) && present) {
		return kv;
	} else if (!present) {
		return NULL;
	} else {
		for (;;) {
			uint32_t link = boa__chunk_link(chunk, slot_ix);
			if (link == slot_ix) return NULL;

			slot_ix = link & 7;
			if (link >= 8) {
				chunk_ix = boa__chunk_next_index(chunk);
				chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);
				kv_base = (char*)(chunk + 1);
			}
			kv = kv_base + slot_ix * map->kv_size;
			if (cmp(key, kv)) {
				return kv;
			}
		}
	}
}

boa_inline void *boa_map_erase(boa_map *map, uint32_t index, uint32_t hash)
{
	uint32_t chunk_ix = index / 8;
	uint32_t slot_ix = index % 8;

	boa__map_chunk *chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);
	char *kv_base = (char*)(chunk + 1);

	boa_assert(boa__chunk_present(chunk, slot_ix));

}

#endif

#define BOA__MAP_LOWBITS 9
#define BOA__MAP_LOWMASK ((1 << BOA__MAP_LOWBITS) - 1)
#define BOA__MAP_HIGHMASK (~(uint32_t)BOA__MAP_LOWMASK)

typedef struct boa_map {

	// Configuration
	boa_allocator *ator; // < Allocator to use
	uint32_t key_size;   // < Size of the key type in bytes
	uint32_t val_size;   // < Value of the key type in bytes

	uint32_t count;    // < Number of elements in the map
	uint32_t capacity; // < Maximum amount of elements in the map

	uint32_t kv_size;            // < Size of a key-value pair in bytes
	uint32_t val_offset;         // < Offset of the value from the key in bytes
	uint32_t num_hash_blocks;    // < Number of primary blocks in the map, must be a power of two.
	uint32_t num_total_blocks;   // < Total number of (primary + auxilary) blocks
	uint32_t num_used_blocks;    // < Number of used (primary + auxilary) blocks
	uint32_t block_num_slots;    // < Number of slots (hash locations) in a block
	uint32_t block_num_elements; // < Number of elements (key-value pairs) in a block
	uint32_t block_element_bits; // < Number of element index bits inside a block: Log2 of block_num_elements

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

} boa_map;

typedef struct boa__map_block {
	uint32_t next_aux; // < Index of the next aux block, 0 if none
	uint32_t count;    // < Number of elements in this block
} boa__map_block;

// Return non-zero if `a` is equal to `b`
typedef int (*boa_cmp_fn)(const void *a, const void *b);

int boa__map_rehash(boa_map *map);
uint32_t boa__map_find_fallback(boa_map *map, uint32_t block_ix);

boa_inline uint32_t boa_map_insert(boa_map *map, const void *key, uint32_t hash, boa_cmp_fn cmp)
{
	// Rehash before insert for simplicity, also handles edge case of empty map
	if (map->count >= map->capacity) {
		if (!boa__map_rehash(map)) return ~0u;
	}

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->num_hash_blocks - 1;
	uint32_t slot_mask = map->block_num_slots - 1;
	uint32_t block_ix = (hash >> 8) & block_mask;
	uint32_t slot_ix = hash & slot_mask;

	// Retrieve the actual block to insert to, falling back to auxilary blocks when
	// the blocks get full.
	// Note: `boa__map_find_fallback()` may invalidate any pointers to the map!
	uint32_t count = map->blocks[block_ix].count;
	if (count >= map->block_num_elements) {
		block_ix = boa__map_find_fallback(map, block_ix);
		if (block_ix == ~0u) return ~0u;
		count = map->blocks[block_ix].count;
	}

	uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;
	uint32_t scan = 0;
	uint16_t to_insert;
	uint32_t result;

	for (;;) {
		uint32_t es = element_slot[slot_ix];

		// Match `LOWMASK` bits of the hash to the element-slot value
		if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
			uint32_t element_offset = es >> BOA__MAP_LOWBITS;
			uint32_t element = block_ix * map->block_num_elements + element_offset;
			void *kvp = (char*)map->data_blocks + element * map->kv_size;
			if (cmp(key, kvp)) {
				return element | 0x80000000;
			}
		}

		// If we find an empty slot or one with lower scan distance insert here
		uint32_t ref_scan = (slot_ix - es) & slot_mask;
		if (es == 0xFFFF || ref_scan < scan) {
			element_slot[slot_ix] = count << BOA__MAP_LOWBITS | (hash & BOA__MAP_LOWMASK);
			map->blocks[block_ix].count = count + 1;
			result = block_ix * map->block_num_elements + count;
			map->hash_cur_slot[result] = slot_ix | (hash & BOA__MAP_HIGHMASK);
			to_insert = es;
			map->count++;
			break;
		}

		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;
	}

	// While we're holding a displaced slot insert it somewhere
	while (to_insert != 0xFFFF) {
		uint32_t es = element_slot[slot_ix];
		uint32_t diff = es - slot_ix;
		uint32_t ref_scan = diff & slot_mask;
		if (es == 0xFFFF || ref_scan < scan) {
			element_slot[slot_ix] = to_insert;
			to_insert = es;
		}

		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;
	}

	return result;
}

boa_inline uint32_t boa_map_find(boa_map *map, const void *key, uint32_t hash, boa_cmp_fn cmp)
{
	// Edge case: Other values may not be valid when empty!
	if (map->count == 0) return ~0u;

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->num_hash_blocks - 1;
	uint32_t slot_mask = map->block_num_slots - 1;
	uint32_t block_ix = (hash >> 8) & block_mask;
	uint32_t slot_ix = hash & slot_mask;

	uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;
	uint32_t scan = 0;

	for (;;) {
		uint32_t es = element_slot[slot_ix];

		// Match `LOWMASK` bits of the hash to the element-slot value
		if (((es ^ hash) & BOA__MAP_LOWMASK) == 0) {
			uint32_t element_offset = es >> BOA__MAP_LOWBITS;
			uint32_t element = block_ix * map->block_num_elements + element_offset;
			void *kvp = (char*)map->data_blocks + element * map->kv_size;
			if (cmp(key, kvp)) {
				return element;
			}
		}

		// If we find an empty slot or one with lower scan fail find
		uint32_t ref_scan = (slot_ix - es) & slot_mask;
		if (es == 0xFFFF || ref_scan < scan) {
			return ~0u;
		}

		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;
	}
}

uint32_t boa_map_erase(boa_map *map, uint32_t element);

uint32_t boa_map_begin(boa_map *map);

uint32_t boa__map_find_next(boa_map *map, uint32_t element);

boa_inline uint32_t boa_map_next(boa_map *map, uint32_t element)
{
	uint32_t block_ix = element >> map->block_element_bits;
	boa__map_block *block = &map->blocks[block_ix];
	if (block_ix < block->count) {
		return element + 1;
	} else {
		return boa__map_find_next(map, element);
	}
}

void boa_map_reset(boa_map *map);

#define boa_key(type, map, elem) (type*)((char*)(map)->data_blocks + (elem) * (map)->kv_size)
#define boa_val(type, map, elem) (type*)((char*)(map)->data_blocks + (elem) * (map)->kv_size + (map)->val_offset)

#endif
