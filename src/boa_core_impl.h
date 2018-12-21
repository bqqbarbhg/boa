#pragma once

#include "boa_core.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef BOA__CORE_IMPLEMENTED
#define BOA__CORE_IMPLEMENTED

// -- boa_allocator

#ifndef BOA_ALLOC
	#define BOA_ALLOC malloc
#endif

#ifndef BOA_REALLOC
	#define BOA_REALLOC realloc
#endif

#ifndef BOA_FREE
	#define BOA_FREE free
#endif

void *boa_alloc(size_t size) { return BOA_ALLOC(size); }
void *boa_realloc(void *ptr, size_t size) { return BOA_REALLOC(ptr, size); }
void boa_free(void *ptr) { BOA_FREE(ptr); }

static void *boa__heap_alloc(boa_allocator *ator, size_t size) { return BOA_ALLOC(size); }
static void *boa__heap_realloc(boa_allocator *ator, void *ptr, size_t size) { return BOA_REALLOC(ptr, size); }
static void boa__heap_free(boa_allocator *ator, void *ptr) { BOA_FREE(ptr); }

boa_allocator boa__heap_ator = {
	boa__heap_alloc,
	boa__heap_realloc,
	boa__heap_free,
};

static void *boa__null_alloc(boa_allocator *ator, size_t size){ return NULL; }
static void *boa__null_realloc(boa_allocator *ator, void *ptr, size_t size){ return NULL; }
static void boa__null_free(boa_allocator *ator, void *ptr) { }

boa_allocator boa__null_ator = {
	boa__null_alloc,
	boa__null_realloc,
	boa__null_free,
};

// -- boa_buf

static inline boa_allocator *boa__buf_ator(uintptr_t ator_flags)
{
	return (boa_allocator*)(ator_flags & ~(uintptr_t)BOA_BUF_FLAG_MASK);
}

#define BOA_MIN_BUF_CAP 16

typedef struct boa__buf_grow_header {
	void *fixed_data;
	uint32_t fixed_cap;
} boa__buf_grow_header;

void boa__buf_reset_heap(boa_buf *buf)
{
	void *data = buf->data;
	size_t ator_flags = buf->ator_flags;
	boa_allocator *ator = boa__buf_ator(ator_flags);
	if (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) {
		// Heap -> Fixed: Return original buffer
		boa__buf_grow_header *header = (boa__buf_grow_header*)data - 1;
		buf->data = header->fixed_data;
		buf->cap_pos = header->fixed_cap;
		data = header;
	} else {
		// Heap -> Zero: Return to empty buffer
		buf->data = NULL;
		buf->cap_pos = 0;
	}
	boa_free_ator(ator, data);
}

void *boa__buf_grow(boa_buf *buf, uint32_t new_cap)
{
	uint32_t old_cap = buf->cap_pos;
	void *old_data = buf->data;
	uintptr_t ator_flags = buf->ator_flags;
	boa_allocator *ator = boa__buf_ator(ator_flags);

	void *new_data;

	// This function should only be called when needed
	boa_assert(new_cap > old_cap);

	// Ensure geometric growth and minimum size
	if (new_cap < BOA_MIN_BUF_CAP) new_cap = BOA_MIN_BUF_CAP;
	if (new_cap < old_cap * 2) new_cap = old_cap * 2;

	if (ator_flags & BOA_BUF_FLAG_ALLOCATED) {
		// Heap -> Heap: Just realloc() with optional header offset
		uint32_t offset = (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) ? sizeof(boa__buf_grow_header) : 0;
		new_data = boa_realloc_ator(ator, (char*)old_data - offset, new_cap + offset);
	} else {
		if (old_data != NULL) {
			// Fixed -> Heap: Create header and memcpy() data
			new_data = boa_alloc_ator(ator, new_cap + sizeof(boa__buf_grow_header));
			ator_flags |= BOA_BUF_FLAG_HAS_BUFFER;
			if (new_data) {
				boa__buf_grow_header *header = (boa__buf_grow_header*)new_data;
				header->fixed_data = old_data;
				header->fixed_cap = old_cap;
				new_data = header + 1;
				memcpy(new_data, old_data, old_cap);
			}
		} else {
			// Zero -> Heap: Just allocate a new buffer
			new_data = boa_alloc_ator(ator, new_cap);
		}
		buf->ator_flags = ator_flags | BOA_BUF_FLAG_ALLOCATED;
	}

	if (!new_data) return NULL;
	buf->data = new_data;
	buf->cap_pos = new_cap;

	return (char*)new_data + buf->end_pos;
}

int boa_buf_set_ator(boa_buf *buf, boa_allocator *ator)
{
	uintptr_t ator_flags = buf->ator_flags;
	boa_allocator *old_ator = boa__buf_ator(ator_flags);
	if (old_ator == ator) return 1;
	
	if (!(ator_flags & BOA_BUF_FLAG_ALLOCATED)) {
		buf->ator_flags = (ator_flags & (uintptr_t)BOA_BUF_FLAG_MASK) | (uintptr_t)ator;
		return 1;
	}

	uint32_t offset = (ator_flags & BOA_BUF_FLAG_HAS_BUFFER) ? sizeof(boa__buf_grow_header) : 0;
	void *old_data = (char*)buf->data - offset;
	uint32_t total_size = buf->cap_pos + offset;

	void *new_data = boa_alloc_ator(ator, total_size);
	if (!new_data) return 0;

	memcpy(new_data, old_data, total_size);

	buf->data = (char*)new_data + offset;
	buf->ator_flags = (ator_flags & (uintptr_t)BOA_BUF_FLAG_MASK) | (uintptr_t)ator;
	boa_free_ator(old_ator, old_data);
	return 1;
}

void *boa_buf_insert(boa_buf *buf, uint32_t pos, uint32_t size)
{
	boa_assert(buf != NULL);
	boa_assert(size > 0);
	boa_assert(pos <= buf->end_pos);

	uint32_t req_cap = buf->end_pos + size;
	if (req_cap > buf->cap_pos) {
		void *ptr = boa__buf_grow(buf, req_cap);
		if (!ptr) return NULL;
	}

	char *begin = (char*)buf->data + pos;
	memmove(begin + size, begin, buf->end_pos - pos);
	buf->end_pos += size;
	return begin;
}

char *boa_format(boa_buf *buf, const char *fmt, ...)
{
	boa_buf zerobuf;
	if (!buf) {
		zerobuf = boa_empty_buf();
		buf = &zerobuf;
	}

	char *ptr = boa_end(char, buf);
	uint32_t cap = boa_bytesleft(buf);
	int len;
	va_list args;

	va_start(args, fmt);
	len = vsnprintf(ptr, cap, fmt, args);
	va_end(args);

	if (len < 0) return NULL;

	if ((uint32_t)len >= cap) {
		ptr = (char*)boa_buf_reserve(buf, (uint32_t)len + 1);
		cap = boa_bytesleft(buf);
		if (!ptr) return NULL;

		va_start(args, fmt);
		vsnprintf(ptr, cap, fmt, args);
		va_end(args);
	}

	ptr[len] = '\0';
	boa_buf_bump(buf, len);
	return ptr;
}

static void boa__map_insert_no_find(boa_map *map, uint32_t hash, const void *data)
{
	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->num_hash_blocks - 1;
	uint32_t slot_mask = map->block_num_slots - 1;
	uint32_t block_ix = (hash >> 8) & block_mask;
	uint32_t slot_ix = hash & slot_mask;

	// Retrieve the actual block to insert to, falling back to auxilary blocks when
	// the blocks get full.
	// Note: `boa__map_find_fallback()` may invalidate any pointers to the map!
	uint32_t count = map->blocks[block_ix].count;
	if (count >= 128) {
		block_ix = boa__map_find_fallback(map, block_ix);
		count = map->blocks[block_ix].count;
	}

	uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;
	uint32_t scan = 0;
	uint16_t to_insert;

	for (;;) {
		uint32_t es = element_slot[slot_ix];

		// If we find an empty slot or one with lower scan distance insert here
		uint32_t ref_scan = (slot_ix - es) & slot_mask;
		if (es == 0xFFFF || ref_scan < scan) {
			uint32_t result = block_ix * map->block_num_elements + count;
			element_slot[slot_ix] = count << BOA__MAP_LOWBITS | (hash & BOA__MAP_LOWMASK);
			to_insert = es;
			map->blocks[block_ix].count = count + 1;

			void *dst = (char*)map->data_blocks + result * map->kv_size;
			memcpy(dst, data, map->kv_size);
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
}

int boa__map_allocate(boa_map *map)
{
	uint32_t block_size = boa_align_up(map->num_total_blocks * sizeof(boa__map_block), 8);
	uint32_t es_size = boa_align_up(map->num_total_blocks * map->block_num_slots * sizeof(uint16_t), 8);
	uint32_t hcs_size = boa_align_up(map->num_total_blocks * map->block_num_elements * sizeof(uint32_t), 8);
	uint32_t kv_size = boa_align_up(map->num_total_blocks * map->block_num_elements * map->kv_size, 8);

	uint32_t block_offset = 0;
	uint32_t es_offset = block_offset + block_size;
	uint32_t hcs_offset = es_offset + es_size;
	uint32_t kv_offset = hcs_offset + hcs_size;
	uint32_t total_size = kv_offset + kv_size;

	char *ptr;
	if (map->allocation) {
		ptr = (char*)boa_realloc_ator(map->ator, map->allocation, total_size);
	} else {
		ptr = (char*)boa_alloc_ator(map->ator, total_size);
	}

	if (!ptr) return 0;
	map->allocation = ptr;

	map->blocks = (boa__map_block*)(ptr + block_offset);
	map->element_slot = (uint16_t*)(ptr + es_offset);
	map->hash_cur_slot = (uint32_t*)(ptr + hcs_offset);
	map->data_blocks = (void*)(ptr + kv_offset);

	return 1;
}

int boa__map_rehash(boa_map *map)
{
	boa_map new_map = *map;

	uint32_t block_ix, elem_ix;
	uint32_t num_blocks = map->num_used_blocks, num_elems;
	uint32_t num_aux = map->num_total_blocks - map->num_hash_blocks;

	uint32_t max_align = map->key_size | map->val_size;
	uint32_t kv_align = max_align >= 8 ? 8 : 4;

	if (map->capacity == 0) {
		new_map.num_hash_blocks = 1;
		num_aux = 0;
		new_map.block_num_slots = 32;
		new_map.block_element_bits = 4; // 16
		new_map.val_offset = boa_align_up(new_map.key_size, kv_align);
		new_map.kv_size = boa_align_up(new_map.val_offset + map->val_size, kv_align);
	} else {
		new_map.block_num_slots = map->block_num_slots;
		new_map.block_element_bits = map->block_element_bits;

		if (new_map.block_num_slots < 256) {
			new_map.block_num_slots *= 2;
			new_map.block_element_bits += 1;
		}

		if (map->block_num_slots == 256)
			new_map.num_hash_blocks = map->num_hash_blocks * 2;
	}

	// Alloacte at least 1/2 aux blocks per hash block
	uint32_t auto_aux = new_map.num_hash_blocks / 2;
	if (num_aux < auto_aux) num_aux = auto_aux;
	if (new_map.num_hash_blocks < 2) num_aux = 0;

	new_map.allocation = NULL;
	new_map.num_total_blocks = new_map.num_hash_blocks + num_aux;
	new_map.block_num_elements = 1 << new_map.block_element_bits;
	new_map.num_used_blocks = new_map.num_hash_blocks;
	new_map.capacity = new_map.num_hash_blocks * new_map.block_num_slots / 2;

	boa_assert(new_map.block_num_slots <= 1 << BOA__MAP_LOWBITS);
	boa_assert(new_map.block_num_elements <= 1 << (16 - BOA__MAP_LOWBITS));

	if (!boa__map_allocate(&new_map)) return 0;

	memset(new_map.element_slot, 0xFF, sizeof(uint16_t) * new_map.block_num_slots * new_map.num_hash_blocks);
	memset(new_map.blocks, 0x00, sizeof(boa__map_block) * new_map.num_hash_blocks);

	for (block_ix = 0; block_ix < num_blocks; block_ix++)
	{
		uint32_t *hash_cur_slot = map->hash_cur_slot + block_ix * map->block_num_elements;
		uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;
		const char *data = (const char *)map->data_blocks + block_ix * map->block_num_elements * map->kv_size;
		num_elems = map->blocks[block_ix].count;
		for (elem_ix = 0; elem_ix < num_elems; elem_ix++)
		{
			uint32_t hcs = hash_cur_slot[elem_ix];
			uint16_t es = element_slot[hcs & BOA__MAP_LOWMASK];
			uint32_t hash = (es & BOA__MAP_LOWMASK) | (hcs & BOA__MAP_HIGHMASK);
			boa__map_insert_no_find(&new_map, hash, data);
			data += map->kv_size;
		}
	}

	if (map->allocation)
		boa_free_ator(map->ator, map->allocation);

	*map = new_map;
	return 1;
}

uint32_t boa__map_find_fallback(boa_map *map, uint32_t block_ix)
{
	boa__map_block *block;
	do {
		block = &map->blocks[block_ix];
		if (block->next_aux) {
			block_ix = block->next_aux;
		} else {
			if (map->num_used_blocks == map->num_total_blocks) {
				uint32_t num_aux = map->num_total_blocks - map->num_hash_blocks;
				if (num_aux == 0) num_aux = 2;
				map->num_total_blocks += num_aux;
				if (!boa__map_allocate(map)) return ~0u;
			}
			block_ix = map->num_used_blocks;
			map->num_used_blocks++;

			block->next_aux = block_ix;

			block = &map->blocks[block_ix];
			uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;

			block->count = 0;
			block->next_aux = 0;
			memset(element_slot, 0xFF, sizeof(uint16_t) * map->block_num_slots);
		}
	} while (block->count >= map->block_num_elements);
	return block_ix;
}

uint32_t boa_map_erase(boa_map *map, uint32_t element)
{
	uint32_t block_ix = element >> map->block_element_bits;
	uint32_t slot_ix = map->hash_cur_slot[element];
	uint32_t element_ix = element & (map->block_element_bits - 1);

	uint32_t *hash_cur_slot = map->hash_cur_slot + block_ix * map->block_num_elements;
	uint16_t *element_slot = map->element_slot + block_ix * map->block_num_slots;

	boa__map_block *block = &map->blocks[block_ix];
	uint32_t count = block->count - 1;
	block->count = count;

	element_slot[slot_ix] = 0xFFFF;

	// Swap the last element to the current element location
	if (element_ix != count) {
		uint32_t last_slot = hash_cur_slot[count] & BOA__MAP_LOWMASK;
		uint16_t ec = element_slot[last_slot];
		element_slot[last_slot] = (ec & BOA__MAP_LOWMASK) | (element_ix << BOA__MAP_LOWBITS);
		hash_cur_slot[element_ix] = hash_cur_slot[count];

		void *dst = (char*)map->data_blocks + element * map->kv_size;
		void *src = (char*)map->data_blocks + (block_ix * map->block_num_elements + count) * map->kv_size;
		memcpy(dst, src, map->kv_size);

		return element;
	} else {
		block_ix++;
		while (block_ix < map->num_used_blocks) {
			if (block[block_ix].count != 0) {
				return block_ix * map->block_num_elements;
			}
			block_ix++;
		}
		return ~0u;
	}
}

uint32_t boa_map_begin(boa_map *map)
{
	uint32_t block_ix = 0;
	boa__map_block *block = &map->blocks[block_ix];
	uint32_t count = map->num_used_blocks;
	while (block_ix < count) {
		if (block->count != 0) {
			return block_ix * map->block_num_elements;
		}
		block_ix++;
		block++;
	}
	return ~0u;
}

uint32_t boa__map_find_next(boa_map *map, uint32_t element)
{
	uint32_t block_ix = element >> map->block_element_bits;
	boa__map_block *block = &map->blocks[block_ix];
	uint32_t count = map->num_used_blocks;
	block_ix++;
	block++;
	while (block_ix < count) {
		if (block->count != 0) {
			return block_ix * map->block_num_elements;
		}
		block_ix++;
		block++;
	}
	return ~0u;
}

void boa_map_reset(boa_map *map)
{
	map->count = 0;
	map->capacity = 0;
	if (map->allocation) {
		boa_free_ator(map->ator, map->allocation);
	}
}

#endif
