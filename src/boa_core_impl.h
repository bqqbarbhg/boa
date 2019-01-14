#pragma once

#ifndef BOA__CORE_IMPLEMENTED
#define BOA__CORE_IMPLEMENTED

#include "boa_core.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// -- boa_error

const boa_error boa_err_no_space = { "Allocator is out of space" };

// -- Utility

uint32_t boa_round_pow2_up(uint32_t value)
{
	uint32_t x = value - 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

uint32_t boa_round_pow2_down(uint32_t value)
{
	uint32_t x = value;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
	return x - (x >> 1);
}

void boa_swap(void *a, void *b, uint32_t size)
{
	boa_swap_inline(a, b, size);
}

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
		if (new_data) new_data = (char*)new_data + offset;
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
		ator_flags |= BOA_BUF_FLAG_ALLOCATED;
		if (new_data) buf->ator_flags = ator_flags;
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

int boa_format_null(boa_buf *buf, const void *data, size_t size)
{
	return 1;
}

int boa_format_u32(boa_buf *buf, const void *data, size_t size)
{
	boa_assert(size == sizeof(uint32_t));
	return boa_format(buf, "%u", *(const uint32_t*)data) ? 1 : 0;
}

static int boa__map_allocate(boa_map *map, uint32_t prev_blocks)
{
	uint32_t num_total_blocks = map->impl.num_total_blocks;
	uint32_t block_num_entries = map->impl.block_num_entries;
	uint32_t block_num_slots = map->impl.block_num_entries << 1;

	uint32_t block_size = boa_align_up(num_total_blocks * sizeof(boa__map_block), 8);
	uint32_t es_size = boa_align_up(num_total_blocks * block_num_slots * sizeof(uint16_t), 8);
	uint32_t hcs_size = boa_align_up(num_total_blocks * block_num_entries * sizeof(uint32_t), 8);
	uint32_t entry_size = boa_align_up(num_total_blocks * block_num_entries * map->entry_size, 8);

	uint32_t block_offset = 0;
	uint32_t es_offset = block_offset + block_size;
	uint32_t hcs_offset = es_offset + es_size;
	uint32_t entry_offset = hcs_offset + hcs_size;
	uint32_t total_size = entry_offset + entry_size;

	char *ptr = (char*)boa_alloc_ator(map->ator, total_size);
	if (!ptr) return 0;

	if (prev_blocks) {
		memcpy(ptr + block_offset, map->impl.blocks, prev_blocks * sizeof(boa__map_block));
		memcpy(ptr + es_offset, map->impl.entry_slot, prev_blocks * block_num_slots * sizeof(uint16_t));
		memcpy(ptr + hcs_offset, map->impl.hash_cur_slot, prev_blocks * block_num_entries * sizeof(uint32_t));
		memcpy(ptr + entry_offset, map->impl.entries, prev_blocks * block_num_entries * map->entry_size);
		boa_free_ator(map->ator, map->impl.blocks);
	}

	map->impl.blocks = (boa__map_block*)(ptr + block_offset);
	map->impl.entry_slot = (uint16_t*)(ptr + es_offset);
	map->impl.hash_cur_slot = (uint32_t*)(ptr + hcs_offset);
	map->impl.entries = (void*)(ptr + entry_offset);

	return 1;
}

static void boa__map_insert_no_find(boa_map *map, uint32_t hash, const void *data)
{
	// Inserted hashes should come from the table and be already canonicalized
	boa_assert(hash == boa__map_hash_canonicalize(hash));

	// Calculate block and slot indices from the hash
	uint32_t block_mask = map->impl.num_hash_blocks - 1;
	uint32_t block_ix = (hash >> BOA__MAP_BLOCK_SHIFT) & block_mask;
	uint32_t block_num_slots = boa__map_block_num_slots(map);
	uint32_t slot_mask = block_num_slots - 1;
	uint32_t slot_ix = hash & slot_mask;

	// Retrieve the actual block to insert to, falling back to auxilary blocks when
	// the blocks get full.
	// Note: `boa__map_find_fallback()` may invalidate any pointers to the map!
	uint32_t count = map->impl.blocks[block_ix].count;
	if (count >= map->impl.block_num_entries) {
		block_ix = boa__map_find_fallback(map, block_ix);
		boa_assert(block_ix != ~0u);
		count = map->impl.blocks[block_ix].count;
	}

	uint16_t *entry_slot = map->impl.entry_slot + block_ix * block_num_slots;
	uint32_t scan = 0;  // < Number of slots scanned from insertion point
	uint16_t displaced; // < Displaced element-slot value that needs to be inserted

	for (;;) {
		uint32_t es = entry_slot[slot_ix];

		// If we find an empty slot or one with lower scan distance insert here
		uint32_t ref_scan = boa__es_scan_distance(es, slot_ix, slot_mask);
		if (es == 0 || ref_scan < scan) {
			// Insert as last element of the block
			entry_slot[slot_ix] = boa__es_make(count, hash);
			uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, count);
			map->impl.hash_cur_slot[entry_index] = boa__hcs_make(hash, slot_ix);
			map->impl.blocks[block_ix].count = count + 1;
			displaced = es;

			void *dst = boa__map_entry_from_index(map, entry_index);
			memcpy(dst, data, map->entry_size);
			break;
		}

		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;
	}

	// While we're holding a displaced slot insert it somewhere
	while (displaced != 0) {
		slot_ix = (slot_ix + 1) & slot_mask;
		scan++;

		uint32_t es = entry_slot[slot_ix];
		uint32_t ref_scan = (slot_ix - es) & slot_mask;
		if (es == 0 || ref_scan < scan) {
			uint32_t entry_offset = boa__es_entry_offset(displaced);
			uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, entry_offset);
			uint32_t hcs = map->impl.hash_cur_slot[entry_index];
			map->impl.hash_cur_slot[entry_index] = boa__hcs_set_slot(hcs, slot_ix);

			entry_slot[slot_ix] = displaced;
			displaced = es;
		}
	}
}

int boa_map_reserve(boa_map *map, uint32_t capacity)
{
	boa_map new_map = *map;

	if (capacity < 16) capacity = 16;

	uint32_t block_ix, elem_ix;
	uint32_t num_blocks = map->impl.num_used_blocks;
	uint32_t num_aux = map->impl.num_total_blocks - map->impl.num_hash_blocks;

	if (capacity <= BOA__MAP_BLOCK_MAX_ENTRIES) {
		capacity = boa_round_pow2_up(capacity);
		num_aux = 0;
		new_map.impl.num_hash_blocks = 1;
		new_map.impl.block_num_entries = capacity;
	} else {
		uint32_t cap = (capacity * 4 / 3 + BOA__MAP_BLOCK_MAX_ENTRIES - 1) / BOA__MAP_BLOCK_MAX_ENTRIES;
		new_map.impl.num_hash_blocks = boa_round_pow2_up(cap);
		new_map.impl.block_num_entries = BOA__MAP_BLOCK_MAX_ENTRIES;
	}

	// Alloacte at least 1/4 aux blocks per hash block
	uint32_t auto_aux = new_map.impl.num_hash_blocks / 4;
	if (num_aux < auto_aux) num_aux = auto_aux;
	if (new_map.impl.num_hash_blocks < 2) num_aux = 0;

	new_map.impl.entry_block_shift = boa_highest_bit(new_map.impl.block_num_entries);
	new_map.impl.num_total_blocks = new_map.impl.num_hash_blocks + num_aux;
	new_map.impl.num_used_blocks = new_map.impl.num_hash_blocks;
	if (new_map.impl.num_hash_blocks > 1) {
		new_map.capacity = new_map.impl.num_hash_blocks * new_map.impl.block_num_entries * 3 / 4;
	} else {
		new_map.capacity = new_map.impl.block_num_entries;
	}

	uint32_t block_num_slots = boa__map_block_num_slots(&new_map);
	boa_assert(new_map.impl.block_num_entries <= BOA__MAP_BLOCK_MAX_ENTRIES);
	boa_assert(block_num_slots <= BOA__MAP_BLOCK_MAX_SLOTS);

	if (!boa__map_allocate(&new_map, 0)) return 0;

	memset(new_map.impl.entry_slot, 0, sizeof(uint16_t) * block_num_slots * new_map.impl.num_hash_blocks);
	memset(new_map.impl.blocks, 0, sizeof(boa__map_block) * new_map.impl.num_hash_blocks);

	// Re-hash previous entries
	if (map->count > 0) {
		uint32_t old_block_num_slots = boa__map_block_num_slots(map);
		uint32_t *hash_cur_slot = map->impl.hash_cur_slot;
		uint16_t *element_slot = map->impl.entry_slot;
		for (block_ix = 0; block_ix < num_blocks; block_ix++)
		{
			const char *entry = (const char *)map->impl.entries + block_ix * map->impl.block_num_entries * map->entry_size;
			uint32_t num_elems = map->impl.blocks[block_ix].count;
			for (elem_ix = 0; elem_ix < num_elems; elem_ix++)
			{
				uint32_t hcs = hash_cur_slot[elem_ix];
				uint16_t es = element_slot[boa__hcs_current_slot(hcs)];
				uint32_t hash = (es & BOA__MAP_LOWMASK) | (hcs & BOA__MAP_HIGHMASK);
				boa__map_insert_no_find(&new_map, hash, entry);
				entry += map->entry_size;
			}

			hash_cur_slot += map->impl.block_num_entries;
			element_slot += old_block_num_slots;
		}
	}

	if (map->impl.blocks) {
		boa_free_ator(map->ator, map->impl.blocks);
	}

	*map = new_map;
	return 1;
}

uint32_t boa__map_find_fallback(boa_map *map, uint32_t block_ix)
{
	boa__map_block *block = &map->impl.blocks[block_ix];
	do {
		if (block->next_aux) {
			block_ix = block->next_aux;
			block = &map->impl.blocks[block_ix];
		} else {
			if (map->impl.num_used_blocks == map->impl.num_total_blocks) {
				uint32_t num_aux = map->impl.num_total_blocks - map->impl.num_hash_blocks;
				uint32_t prev_blocks = map->impl.num_total_blocks;
				if (num_aux <= 1) num_aux = 2;

				map->impl.num_total_blocks += num_aux;
				if (!boa__map_allocate(map, prev_blocks)) return ~0u;

				// Reload invalidated the pointer
				block = &map->impl.blocks[block_ix];
			}
			block_ix = map->impl.num_used_blocks;
			map->impl.num_used_blocks++;

			block->next_aux = block_ix;
			block = &map->impl.blocks[block_ix];

			uint32_t block_num_slots = boa__map_block_num_slots(map);
			uint16_t *entry_slot = map->impl.entry_slot + block_ix * block_num_slots;
			block->count = 0;
			block->next_aux = 0;
			memset(entry_slot, 0, sizeof(uint16_t) * block_num_slots);
		}
	} while (block->count >= map->impl.block_num_entries);
	return block_ix;
}

boa_noinline boa_map_insert_result boa_map_insert(boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp, void *user)
{
	return boa_map_insert_inline(map, key, hash, cmp, user);
}

boa_noinline void *boa_map_find(const boa_map *map, const void *key, uint32_t hash, boa_map_cmp_fn cmp, void *user)
{
	return boa_map_find_inline(map, key, hash, cmp, user);
}

#define boa__map_index_from_entry(map, entry) (uint32_t)(((char*)(entry) - (char*)(map)->impl.entries) / ((map)->entry_size))

boa_map_iterator boa__map_find_block_start(const boa_map *map, uint32_t block_ix)
{
	boa__map_block *block = &map->impl.blocks[block_ix];
	uint32_t count = map->impl.num_used_blocks;
	boa_map_iterator result;
	result.entry = NULL;

	while (block_ix < count) {
		uint32_t block_count = block->count;
		if (block_count != 0) {
			uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, 0);
			result.entry = boa__map_entry_from_index(map, entry_index);
			result.impl_block_end = (char*)result.entry + map->entry_size * block_count;
			break;
		}
		block_ix++;
		block++;
	}

	return result;
}

void *boa__map_remove_non_iter(boa_map *map, void *entry)
{
	uint32_t entry_index = boa__map_index_from_entry(map, entry);
	uint32_t slot_ix = boa__hcs_current_slot(map->impl.hash_cur_slot[entry_index]);
	uint32_t block_ix = entry_index >> map->impl.entry_block_shift;
	uint32_t entry_offset = entry_index & (map->impl.block_num_entries - 1);

	uint32_t block_num_slots = boa__map_block_num_slots(map);
	uint32_t *hash_cur_slot = map->impl.hash_cur_slot + block_ix * map->impl.block_num_entries;
	uint16_t *entry_slot = map->impl.entry_slot + block_ix * block_num_slots;

	boa__map_block *block = &map->impl.blocks[block_ix];
	uint32_t count = block->count;
	boa_assert(map->count > 0);
	boa_assert(count > 0);
	count--;
	block->count = count;

	map->count--;

	uint32_t res_block;
	void *new_block_end;

	// Swap the last element to the current element location
	if (entry_offset != count) {
		uint32_t last_slot = hash_cur_slot[count] & BOA__MAP_LOWMASK;
		uint16_t es = entry_slot[last_slot];
		entry_slot[last_slot] = (es & BOA__MAP_LOWMASK) | (entry_offset << BOA__MAP_LOWBITS);
		hash_cur_slot[entry_offset] = hash_cur_slot[count];

		uint32_t last_entry_index = boa__map_entry_index_from_block(map, block_ix, count);

		void *src = boa__map_entry_from_index(map, last_entry_index);
		memcpy(entry, src, map->entry_size);

		new_block_end = src;
		res_block = block_ix;
	} else {
		new_block_end = NULL;
	}

	uint32_t slot_mask = block_num_slots - 1;

	// Shift back following entries
	for (;;) {
		uint32_t next_slot_ix = (slot_ix + 1) & slot_mask;
		uint16_t es = entry_slot[next_slot_ix];

		uint32_t scan = boa__es_scan_distance(es, next_slot_ix, slot_mask);
		if (es == 0 || scan == 0) {
			entry_slot[slot_ix] = 0;
			break;
		}

		entry_slot[slot_ix] = es;

		uint32_t entry_offset = boa__es_entry_offset(es);
		uint32_t entry_index = boa__map_entry_index_from_block(map, block_ix, entry_offset);
		uint32_t hcs = map->impl.hash_cur_slot[entry_index];
		map->impl.hash_cur_slot[entry_index] = boa__hcs_set_slot(hcs, slot_ix);

		slot_ix = next_slot_ix;
	}

	return new_block_end;
}

boa_map_iterator boa_map_iterate_from(const boa_map *map, const void *entry)
{
	uint32_t entry_index = boa__map_index_from_entry(map, entry);
	uint32_t block_ix = entry_index >> map->impl.entry_block_shift;
	return boa__map_find_block_start(map, block_ix);
}

boa_map_iterator boa__map_find_next(const boa_map *map, const void *entry)
{
	uint32_t entry_index = boa__map_index_from_entry(map, entry);
	uint32_t block_ix = entry_index >> map->impl.entry_block_shift;
	return boa__map_find_block_start(map, block_ix + 1);
}

void boa_map_clear(boa_map *map)
{
	map->count = 0;
	uint32_t block_num_slots = boa__map_block_num_slots(map);
	memset(map->impl.entry_slot, 0, sizeof(uint16_t) * block_num_slots * map->impl.num_hash_blocks);
	memset(map->impl.blocks, 0, sizeof(boa__map_block) * map->impl.num_hash_blocks);
}

void boa_map_reset(boa_map *map)
{
	map->count = 0;
	map->capacity = 0;
	if (map->impl.blocks) {
		boa_free_ator(map->ator, map->impl.blocks);
		map->impl.blocks = NULL;
	}
}

static uint32_t boa__blit_map_hash(const void *key, uint32_t size)
{
	uint32_t x = 1;
	const char *pa = (const char*)key;

	if ((size & 3) == 0) {
		while (size >= 4) {
			x = boa_hash_combine(x, *(uint32_t*)pa);

			size -= 4;
			pa += 4;
		}
	}

	while (size > 0) {
		x = boa_hash_combine(x, (uint32_t)*pa);

		size -= 1;
		pa += 1;
	}

	return boa_u32_hash(x);
}

static int boa__blit_map_cmp(const void *a, const void *b, void *user)
{
	uint32_t size = *(uint32_t*)user;
	const char *pa = (const char*)a, *pb = (const char*)b;

#if BOA_64BIT
	if ((size & 7) == 0) {
		while (size >= 8) {
			if (*(uint64_t*)pa != *(uint64_t*)pb) return 0;

			size -= 8;
			pa += 8;
			pb += 8;
		}
	}
#endif

	if ((size & 3) == 0) {
		while (size >= 4) {
			if (*(uint32_t*)pa != *(uint32_t*)pb) return 0;

			size -= 4;
			pa += 4;
			pb += 4;
		}
	}

	while (size > 0) {
		if (*pa != *pb) return 0;

		size -= 1;
		pa += 1;
		pb += 1;
	}

	return 1;
}

boa_noinline boa_map_insert_result boa_blit_map_insert(boa_map *map, const void *key_ptr, uint32_t key_size)
{
	uint32_t hash = boa__blit_map_hash(key_ptr, key_size);
	return boa_map_insert_inline(map, key_ptr, hash, &boa__blit_map_cmp, &key_size);
}

boa_noinline void *boa_blit_map_find(const boa_map *map, const void *key_ptr, uint32_t key_size)
{
	uint32_t hash = boa__blit_map_hash(key_ptr, key_size);
	return boa_map_find_inline(map, key_ptr, hash, &boa__blit_map_cmp, &key_size);
}

static uint32_t boa__ptr_map_hash(const void *key)
{
	uintptr_t up = (uintptr_t)key;
#if BOA_64BIT
	uint32_t x = boa_hash_combine((uint32_t)up, (uint32_t)(up >> 32));
#else
	uint32_t x = (uint32_t)up;
#endif
	return boa_u32_hash(x);
}

static int boa__ptr_map_cmp(const void *a, const void *b, void *user)
{
	return *(const void**)a == *(const void**)b;
}

boa_noinline boa_map_insert_result boa_ptr_map_insert(boa_map *map, const void *key)
{
	uint32_t hash = boa__ptr_map_hash(key);
	return boa_map_insert_inline(map, &key, hash, &boa__ptr_map_cmp, NULL);
}

boa_noinline void *boa_ptr_map_find(const boa_map *map, const void *key)
{
	uint32_t hash = boa__ptr_map_hash(key);
	return boa_map_find_inline(map, &key, hash, &boa__ptr_map_cmp, NULL);
}

static int boa__u32_map_cmp(const void *a, const void *b, void *user)
{
	return *(const uint32_t*)a == *(const uint32_t*)b;
}

boa_noinline boa_map_insert_result boa_u32_map_insert(boa_map *map, uint32_t key)
{
	uint32_t hash = boa_u32_hash(key);
	return boa_map_insert_inline(map, &key, hash, &boa__u32_map_cmp, NULL);
}

boa_noinline void *boa_u32_map_find(const boa_map *map, uint32_t key)
{
	uint32_t hash = boa_u32_hash(key);
	return boa_map_find_inline(map, &key, hash, &boa__u32_map_cmp, NULL);
}

// -- boa_heap

void boa_upheap(void *values, uint32_t index, uint32_t size, boa_before_fn before, void *user)
{
	return boa_upheap_inline(values, index, size, before, user);
}

void boa_downheap(void *values, uint32_t end, uint32_t index, uint32_t size, boa_before_fn before, void *user)
{
	return boa_downheap_inline(values, end, index, size, before, user);
}

// -- boa_arena

typedef struct boa__arena_page {
	struct boa__arena_page *next;
	boa_allocator *ator;
} boa__arena_page;

void *boa__arena_push_page(boa_arena *arena, uint32_t size)
{
	uint32_t header_size = boa_align_up(sizeof(boa__arena_page), 8);
	size = header_size + boa_align_up(size, 8);

	uint32_t page_size = arena->impl.cap * 2;
	if (page_size < 1024) page_size = 1024;
	if (page_size < size * 2) page_size = size * 2;

	boa__arena_page *prev = (boa__arena_page*)arena->impl.data;
	boa__arena_page *page = (boa__arena_page*)boa_alloc_ator(arena->ator, page_size);
	if (page == NULL) return NULL;

	page->next = prev;
	page->ator = arena->ator;

	void *ptr = (char*)page + header_size;
	arena->impl.data = page;
	arena->impl.pos = size;
	arena->impl.cap = page_size;
	return ptr;
}

void boa_arena_reset(boa_arena *arena)
{
	boa__arena_page *page = (boa__arena_page*)arena->impl.data;
	while (page != NULL) {
		void *free_ptr = page;
		boa_allocator *ator = page->ator;
		page = page->next;
		boa_free_ator(ator, free_ptr);
	}
}

#endif
