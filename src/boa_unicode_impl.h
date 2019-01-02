#pragma once

#ifndef BOA__UNICODE_IMPLEMENTED
#define BOA__UNICODE_IMPLEMENTED

#include "boa_unicode.h"

static uint32_t boa__utf16_size_in_utf8(unsigned word) {
	if (word < 0x80) return 1;
	else if (word < 0x800) return 2;
	else if (word - 0xD800 >= 0x800) return 3;
	else return 4;
}

static uint32_t boa__utf8_size_in_utf16(unsigned byte) {
	if ((byte & 0xF8) != 0xF0) return 1;
	else return 2;
}

int boa_convert_utf16_to_utf8(boa_buf *buf, const uint16_t **ptr, const uint16_t *end)
{
	int result = 1;
	const uint16_t *p = *ptr;
	uint8_t *dst_ptr = boa_end(uint8_t, buf);
	uint32_t dst_left = (uint32_t)(boa_cap(uint8_t, buf) - dst_ptr);

	while (p != end) {
		unsigned word = *p;
		if (!end && word == 0) {
			// U+0000: Null termination (if not ranged)
			break;
		} else if (word < 0x80 && dst_left >= 1) {
			// U+0000..U+007F: Single byte UTF8, single word UTF16
			dst_ptr[0] = (uint8_t)word;
			dst_ptr += 1;
			dst_left -= 1;
			p += 1;
		} else if (word < 0x800 && dst_left >= 2) {
			// U+0080..U+07FF: Double byte UTF8, single word UTF16
			dst_ptr[0] = (uint8_t)(0xC0 | (word >> 6));
			dst_ptr[1] = (uint8_t)(0x80 | (word & 0x3F));
			dst_ptr += 2;
			dst_left -= 2;
			p += 1;
		} else if (word - 0xD800 >= 0x800 && dst_left >= 3) {
			// U+0800..U+D7FF, U+0E00..U+FFFF: Triple byte UTF8, single word UTF16
			dst_ptr[0] = (uint8_t)(0xE0 | (word >> 12));
			dst_ptr[1] = (uint8_t)(0x80 | ((word >> 6) & 0x3F));
			dst_ptr[2] = (uint8_t)(0x80 | (word & 0x3F));
			dst_ptr += 3;
			dst_left -= 3;
			p += 1;
		} else if (p + 1 != end && dst_left >= 4) {
			// U+D800..U+0DFF: Quad byte UTF8, double word UTF16 surrogate pair
			unsigned hi = word - 0xD800;
			unsigned lo = p[1] - 0xDC00;

			if (((hi | lo) >> 10) == 0) {
				// Valid surrogate pair: hi, lo < 0x400
				uint32_t point = 0x10000 + ((hi << 10) | lo);
				dst_ptr[0] = (uint8_t)(0xF0 | (point >> 18));
				dst_ptr[1] = (uint8_t)(0x80 | ((point >> 12) & 0x3F));
				dst_ptr[2] = (uint8_t)(0x80 | ((point >> 6) & 0x3F));
				dst_ptr[3] = (uint8_t)(0x80 | (point & 0x3F));
				dst_ptr += 4;
				dst_left -= 4;
				p += 2;
			} else {
				// Bad surrogate pair
				result = 0;
				break;
			}
		} else {
			// Destination buffer too small or bad surrogate pair
			uint32_t size = boa__utf16_size_in_utf8(word);
			size_t offset = dst_ptr - boa_begin(uint8_t, buf);
			if (size > dst_left && boa_reserve_cap_n(uint8_t, buf, size)) {
				dst_ptr = boa_begin(uint8_t, buf) + offset;
				dst_left = (uint32_t)(boa_cap(uint8_t, buf) - dst_ptr);
			} else {
				result = 0;
				break;
			}
		}
	}

	buf->end_pos = (uint32_t)(dst_ptr - boa_begin(uint8_t, buf));
	*ptr = p;

	// Add trailing null byte after end_pos
	if (dst_left >= 1) {
		*dst_ptr++ = '\0';
	} else {
		uint8_t *zero = boa_reserve(uint8_t, buf);
		if (zero) {
			*zero = '\0';
		} else {
			result = 0;
		}
	}

	return result;
}

int boa_convert_utf8_to_utf16(boa_buf *buf, const char **ptr, const char *end)
{
	int result = 1;
	const uint8_t *p = (const uint8_t*)ptr;
	uint32_t p_left = end ? (uint32_t)((const uint8_t*)end - p) : ~0u;
	uint16_t *dst_ptr = boa_end(uint16_t, buf);
	uint32_t dst_left = (uint32_t)(boa_cap(uint16_t, buf) - dst_ptr);

	while (p_left > 0) {
		unsigned byte = *p;
		if (!end && byte == 0) {
			// U+0000: Null termination (if not ranged)
			break;
		} else if (byte < 0x80 && dst_left >= 1) {
			// U+0000..U+007F: Single byte UTF8, single word UTF16
			dst_ptr[0] = (uint16_t)byte;
			dst_ptr += 1;
			dst_left -= 1;
			p += 1;
			p_left -= 1;
		} else if ((byte & 0xE0) == 0xC0 && dst_left >= 1 && p_left >= 2) {
			// U+0080..U+07FF: Double byte UTF8, single word UTF16
			uint32_t result = ((uint32_t)byte << 6) ^ (p[1] ^ 0x80);
			if (result >> 6 == byte) {
				dst_ptr[0] = (uint16_t)result;
				dst_ptr += 1;
				dst_left -= 1;
				p += 2;
				p_left -= 2;
			} else {
				// Second byte wasn't 0b10xxxxxx
				result = 0;
				break;
			}
		} else if ((byte & 0xF0) == 0xE0 && dst_left >= 1 && p_left >= 3) {
			// U+0800..U+D7FF, U+0E00..U+FFFF: Triple byte UTF8, single word UTF16
			uint32_t swar = (uint32_t)p[1] << 8 | (uint32_t)p[2];
			uint32_t result = ((uint32_t)byte << 12) | (swar & 0x3F00) >> 2 | (swar & 0x3F);
			if ((swar & 0xC0C0) == 0x8080 && result - 0xD800 >= 0x800) {
				dst_ptr[0] = (uint16_t)result;
				dst_ptr += 1;
				dst_left -= 1;
				p += 3;
				p_left -= 3;
			} else {
				// Trailing bytes weren't 0b10xxxxxx or decoded into a surrogate codepoint
				result = 0;
				break;
			}
		} else if ((byte & 0xF8) == 0xF0 && dst_left >= 2 && p_left >= 4) {
			// U+D800..U+0DFF: Quad byte UTF8, double word UTF16 surrogate pair
			uint32_t swar = (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
			uint32_t result = ((uint32_t)byte << 18) | (swar & 0x3F0000) >> 4 | (swar & 0x3F00) >> 2 | (swar & 0x3F);
			if ((swar & 0xC0C0C0) == 0x808080) {
				uint32_t surrogate = result - 0x10000;
				uint32_t hi = (surrogate >> 10) + 0xD800;
				uint32_t lo = (surrogate & 0x3FF) + 0xDC00;
				dst_ptr[0] = (uint16_t)hi;
				dst_ptr[1] = (uint16_t)lo;
				dst_ptr += 2;
				dst_left -= 2;
				p += 4;
				p_left -= 4;
			} else {
				// Trailing bytes weren't 0b10xxxxxx
				result = 0;
				break;
			}
		} else {
			// Destination buffer too small or bad UTF8 input
			uint32_t size = boa__utf8_size_in_utf16(byte);
			size_t offset = dst_ptr - boa_begin(uint16_t, buf);
			if (size > dst_left && boa_reserve_cap_n(uint16_t, buf, size)) {
				dst_ptr = boa_begin(uint16_t, buf) + offset;
				dst_left = (uint32_t)(boa_cap(uint16_t, buf) - dst_ptr);
			} else {
				// Bad UTF8 input
				result = 0;
				break;
			}
		}
	}

	buf->end_pos = (uint32_t)((char*)dst_ptr - (char*)boa_begin(uint16_t, buf));
	*ptr = (const char*)p;

	// Add trailing null byte after end_pos
	if (dst_left >= 1) {
		*dst_ptr++ = 0;
	} else {
		uint16_t *zero = boa_reserve(uint16_t, buf);
		if (zero) {
			*zero = 0;
		} else {
			result = 0;
		}
	}

	return result;
}

#endif

