
#ifndef BOA__UNICODE_INCLUDED
#define BOA__UNICODE_INCLUDED

#include "boa_core.h"

uint32_t boa_decode_point_utf16(const uint16_t **ptr, const uint16_t *end);

int boa_encode_point_utf8(uint8_t **ptr, uint8_t *end, uint32_t point);

int boa_convert_utf16_to_utf8(boa_vec *vec, const uint16_t *src_ptr, uint16_t *src_end);

#endif

#ifndef BOA__UNICODE_IMPLEMENTED
#define BOA__UNICODE_IMPLEMENTED

static inline uint32_t boa__decode_point_utf16_inline(const uint16_t **ptr, const uint16_t *end)
{
	const uint16_t *p = *ptr;
	uint32_t hi;
	if (p == end) return ~0u;
	hi = *p;
	// Equivalent to hi < 0xD800 || hi >= 0xE000
	if (hi - 0xD800 >= 0x800) {
		*ptr = p + 1;
		return hi;
	} else {
		uint32_t lo;
		if (p + 1 == end) return ~0u; 

		hi -= 0xD800;
		lo = p[1] - 0xDC00;

		// Equivalent to max(hi, lo) >= 0x400
		if (((hi | lo) >> 10) != 0) return ~0u;

		*ptr = p + 2;
		return 0x10000 + ((hi << 10) | lo);
	}
}

static inline int boa__encode_point_utf8_inline(uint8_t **ptr, uint8_t *end, uint32_t point)
{
	uint8_t *p = *ptr;
	if (point < 0x80) {
		if (p == end) return 0;
		*p++ = (uint8_t)point;
		*ptr = p;
	} else if (point < 0x800) {
		if (end - p < 2) return 0;
		*p++ = (uint8_t)(0xC0 | (point >> 6));
		*p++ = (uint8_t)(0x80 | (point & 0x3F));
		*ptr = p;
	} else if (point < 0x10000) {
		if (end - p < 3) return 0;
		*p++ = (uint8_t)(0xE0 | (point >> 12));
		*p++ = (uint8_t)(0x80 | ((point >> 6) & 0x3F));
		*p++ = (uint8_t)(0x80 | (point & 0x3F));
		*ptr = p;
	} else if (point < 0x110000) {
		if (end - p < 4) return 0;
		*p++ = (uint8_t)(0xF0 | (point >> 18));
		*p++ = (uint8_t)(0x80 | ((point >> 12) & 0x3F));
		*p++ = (uint8_t)(0x80 | ((point >> 6) & 0x3F));
		*p++ = (uint8_t)(0x80 | (point & 0x3F));
		*ptr = p;
	} else {
		return 0;
	}
	return 1;
}

uint32_t boa_decode_point_utf16(const uint16_t **ptr, const uint16_t *end)
{
	return boa__decode_point_utf16_inline(ptr, end);
}

int boa_encode_point_utf8(uint8_t **ptr, uint8_t *end, uint32_t point)
{
	return boa__encode_point_utf8_inline(ptr, end, point);
}

int boa_convert_utf16_to_utf8(boa_vec *vec, const uint16_t *src_ptr, uint16_t *src_end) {
	char *dst_ptr = boa_vend(char, *vec);
	char *dst_end = boa_vcap(char, *vec);
	while (src_ptr != src_end) {
		const uint16_t *prev = src_ptr;
		uint32_t point = boa__decode_point_utf16_inline(&src_ptr, src_end);
		if (point == ~0u) return 0;

		if (point == 0 && !src_end) {
			break;
		} else if (point < 0x80 && dst_ptr < dst_end) {
			*dst_ptr++ = (uint8_t)point;
		} else if (!boa__encode_point_utf8_inline(&dst_ptr, dst_end, point)) {
			size_t offset = dst_ptr ? dst_ptr - (char*)vec->buf.ptr : 0;
			if (dst_end - dst_ptr >= 4) return 0;
			if (!boa_buf_realloc(&vec->buf, vec->buf.cap + 4)) return 0;
			dst_ptr = boa_vbegin(char, *vec) + offset;
			dst_end = boa_vcap(char, *vec);
			src_ptr = prev;
		}
	}

	if (dst_ptr) {
		vec->byte_pos = dst_ptr - (char*)vec->buf.ptr;
	} else {
		vec->byte_pos = 0;
	}

	if (dst_ptr < dst_end) {
		*dst_ptr++ = '\0';
	} else { 
		if (!boa_buf_realloc(&vec->buf, vec->buf.cap + 1)) return 0;
		*boa_vend(char, *vec) = '\0';
	}

	return 1;
}

#endif

