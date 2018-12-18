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

enum boa__map_color {
	boa__map_empty = 0, // < The node is not in use in any way
	boa__map_black = 1, // < The node is a part of an intra-chunk linked list
	boa__map_white = 2, // < The node is the head of an intra-chunk linked list
};

typedef struct boa_map {
	void *data;
	uint32_t size;
	uint32_t kv_size;
	uint32_t chunk_size;
	uint32_t aux_chunk_begin;
	uint32_t num_aux;
} boa_map;

typedef struct boa__map_chunk {
	uint32_t link;
	uint16_t color;
	uint16_t next;
} boa__map_chunk;

#define boa_map_assert(x) (void)0

// Find any set bit in `*mask`.
static inline uint32_t boa_find_bit(uint32_t mask)
{
	unsigned long index;
	_BitScanForward(&index, mask);
	return index;
}

boa_inline uint32_t boa_mask_nibbles_to_msb(uint32_t mask)
{
    // 0x8 for bytes with bit 4 set, 0x00 otherwise
    uint32_t high_bits = mask & 0x88888888;
    // 0x8 for bytes with bits 1-3 set, 0x00 otherwise
    uint32_t all_low_bits = (mask & ~0x88888888) + 0x11111111;
    // 0x8 for bytes with all bits set
    return high_bits & all_low_bits;
}

boa_inline boa__map_color boa__map_get_color(boa__map_chunk *chunk, uint32_t slot)
{
	boa_map_assert(slot < 8);
	uint32_t col = (chunk->color >> (slot * 2)) & 3;
	boa_map_assert(col != 3);
	return (boa__map_color)col;
}

// Retrieve the next node this one is linked to
boa_inline uint32_t boa__map_get_link(boa__map_chunk *chunk, uint32_t slot)
{
	boa_map_assert(slot < 8);
	boa_map_assert(boa__map_get_color(chunk, slot) != boa__map_empty);
	uint32_t link = (chunk->link >> (slot * 4)) & 0x7;
	return link;
}

// Change the node this one is linked to
boa_inline void boa__map_set_link(boa__map_chunk *chunk, uint32_t slot, uint32_t link)
{
	boa_map_assert(slot < 8);
	boa_map_assert(boa__map_get_color(chunk, slot) != boa__map_empty);
	chunk->link = chunk->link & ~(0xfu << 4u * slot) | (link << 4u * slot);
}

boa_inline void boa__map_init_node(boa__map_chunk *chunk, uint32_t slot, boa__map_color color)
{
	boa_map_assert(slot < 8);
	chunk->color = chunk->color & ~(0x3 << (slot * 2)) | ((uint32_t)color << (slot * 2));
	boa__map_set_link(chunk, slot, slot);
}

// Retrieve the previous node in this chunk leading to this node.
boa_inline uint32_t boa__map_get_prev(boa__map_chunk *chunk, uint32_t slot)
{
	boa_map_assert(slot < 8);
	boa_map_assert(boa__map_get_color(chunk, slot) == boa__map_black);
	uint32_t eq_mask = (chunk->link ^ (slot * 0x11111111)) | (0x1 << slot * 4);
	uint32_t eq_bits = boa_mask_nibbles_to_msb(~eq_mask) >> 3;
	boa_map_assert(eq_bits != 0);
	boa_map_assert((eq_bits & eq_bits - 1) == 0);
	uint32_t eq_ix = boa_find_bit(eq_bits);
	return eq_ix;
}

boa_inline boa__map_chunk *boa__map_next_chunk(boa_map *map, boa__map_chunk *chunk)
{
	uint32_t offset = map->aux_chunk_begin + chunk->next * map->chunk_size;
	return (boa__map_chunk*)((char*)map->data + offset);
}

boa_inline uint32_t boa__map_empty_mask(boa_map *map, boa__map_chunk *chunk)
{
	return ~((uint32_t)chunk->color + 0x5555) & 0xaaaa;
}

boa_inline uint32_t boa__map_non_white_mask(boa_map *map, boa__map_chunk *chunk)
{
	return ~((uint32_t)chunk->color) & 0xaaaa;
}

boa_inline void *boa__map_get_kv(boa_map *map, boa__map_chunk *chunk, uint32_t slot)
{
	return (char*)(chunk + 1) + slot * map->kv_size;
}

void *boa__map_create(boa_map *map, boa__map_chunk *chunk, uint32_t slot);
void boa__map_unlink_and_insert(boa_map *map, boa__map_chunk *chunk, uint32_t slot);

boa_inline void *boa_map_insert(boa_map *map, const void *key, uint32_t hash, int (*cmp)(const void *a, const void *b))
{
	uint32_t mask = map->size - 1;
	uint32_t index = hash & mask;

	uint32_t chunk_ix = index / 8;
	uint32_t slot = index % 8;

	boa__map_chunk *chunk = (boa__map_chunk*)((char*)map->data + chunk_ix * map->chunk_size);

	for (;;) {
		boa__map_color color = boa__map_get_color(chunk, slot);
		void *kv = boa__map_get_kv(map, chunk, slot);

		if (cmp(key, kv) && color == boa__map_white) {
			return NULL;
		} else if (color == boa__map_empty) {
			boa__map_init_node(chunk, slot, boa__map_white);
			return kv;
		} else if (color == boa__map_black) {
			boa__map_unlink_and_insert(map, chunk, slot);
			return kv;
		} else {
			for (;;) {
				uint32_t link = boa__map_get_link(chunk, slot);
				if (link == slot) {
					return boa__map_create(map, chunk, slot);
				} else if (link >= 8) {
					chunk = boa__map_next_chunk(map, chunk);
					slot -= 8;
				}
			}
		}
	}
}

#endif
