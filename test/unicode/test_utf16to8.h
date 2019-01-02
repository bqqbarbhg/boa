
#include <boa_test.h>
#include <boa_unicode.h>
#include <string.h>
#include "test_codepoints.h"

BOA_TEST(utf16to8_ascii, "Converting UTF-16 to UTF-8 ASCII")
{
	uint16_t data[] = { 0x41, 0x42, 0x43, 0 };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 3);
	boa_assert(!strcmp((char*)dst.data, "ABC"));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, NULL);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 3);
	boa_assert(!strcmp((char*)dst.data, "ABC"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_bmp, "Convert a basic multilingual plane text")
{
	uint16_t data[] = { 0x00E4, 0x65E5, 0x672C, 0x306B, 0x307B, 0x3093, 0 };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 17);
	boa_assert(!strcmp((char*)dst.data, "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3"
		"\x81\xAB\xE3\x81\xBB\xE3\x82\x93"));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, NULL);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 17);
	boa_assert(!strcmp((char*)dst.data, "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3"
		"\x81\xAB\xE3\x81\xBB\xE3\x82\x93"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_emoji, "Convert a few emojis")
{
	uint16_t data[] = { 0xD83D, 0xDC4C, 0xD83D, 0xDC40, 0 };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 8);
	boa_assert(!strcmp((char*)dst.data, "\xF0\x9F\x91\x8C\xF0\x9F\x91\x80"));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, NULL);

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 8);
	boa_assert(!strcmp((char*)dst.data, "\xF0\x9F\x91\x8C\xF0\x9F\x91\x80"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_nulls, "Should be able to encode multiple NULL characters")
{
	uint16_t data[] = { 0, 0, 0, (uint16_t)'A' };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 4);
	boa_assert(!memcmp((char*)dst.data, "\0\0\0A\0", 5));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_test_codepoints, "Should be able to convert test codepoint set")
{
	boa_buf buf = boa_empty_buf();

	uint32_t count = boa_arraycount(test_codepoints);
	for (uint32_t i = 0; i < count; i++) {
		const test_codepoint *point = &test_codepoints[i];
		boa_test_hint_u32(i);

		const uint16_t *ptr;
		int res;
		uint16_t local[3];
		memcpy(local, point->utf16, point->utf16_len * sizeof(uint16_t));
		local[point->utf16_len] = 0;

		boa_clear(&buf);
		ptr = point->utf16;
		res = boa_convert_utf16_to_utf8(&buf, &ptr, ptr + point->utf16_len);

		boa_assert(res != 0);
		boa_assert(buf.end_pos == point->utf8_len);
		boa_assert(ptr == point->utf16 + point->utf16_len);
		boa_assert(!memcmp(buf.data, point->utf8, point->utf8_len));

		boa_clear(&buf);
		ptr = local;
		res = boa_convert_utf16_to_utf8(&buf, &ptr, NULL);

		boa_assert(res != 0);
		boa_assert(buf.end_pos == point->utf8_len);
		boa_assert(ptr == local + point->utf16_len);
		boa_assert(!memcmp(buf.data, point->utf8, point->utf8_len));
	}

	boa_reset(&buf);
}

BOA_TEST(utf16to8_test_codepoints_concat, "Should be able to convert test codepoint set concatenated")
{
	boa_buf src = boa_empty_buf();
	boa_buf target = boa_empty_buf();
	boa_buf buf = boa_empty_buf();
	const uint16_t *ptr;
	int res;

	uint32_t count = boa_arraycount(test_codepoints);
	for (uint32_t i = 0; i < count; i++) {
		const test_codepoint *point = &test_codepoints[i];
		res = boa_push_data_n(&src, point->utf16, point->utf16_len);
		boa_assert(res != 0);
		res = boa_push_data_n(&target, point->utf8, point->utf8_len);
		boa_assert(res != 0);
	}

	ptr = boa_begin(uint16_t, &src);
	res = boa_convert_utf16_to_utf8(boa_clear(&buf), &ptr, boa_end(uint16_t, &src));

	boa_assert(res != 0);
	boa_assert(buf.end_pos == target.end_pos);
	boa_assert(ptr == boa_end(uint16_t, &src));
	boa_assert(!memcmp(buf.data, target.data, buf.end_pos));

	boa_push_val(uint16_t, &src, 0);
	ptr = boa_begin(uint16_t, &src);
	res = boa_convert_utf16_to_utf8(boa_clear(&buf), &ptr, NULL);

	boa_assert(res != 0);
	boa_assert(buf.end_pos == target.end_pos);
	boa_assert(ptr == boa_end(uint16_t, &src) - 1);
	boa_assert(!memcmp(buf.data, target.data, buf.end_pos));

	boa_reset(&src);
	boa_reset(&target);
	boa_reset(&buf);
}

BOA_TEST(utf16to8_surrogate_end, "Unpaired surrogate due to end")
{
	uint16_t data[] = { (uint16_t)'A', 0xD83D };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res == 0);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 1);
	boa_assert(!strcmp((char*)dst.data, "A"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_surrogate_doublehi, "Double high surrogate")
{
	uint16_t data[] = { (uint16_t)'A', 0xD83D, 0xD83D };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res == 0);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 1);
	boa_assert(!strcmp((char*)dst.data, "A"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_surrogate_wrongorder, "Surrogates in wrong order")
{
	uint16_t data[] = { (uint16_t)'A', 0xDC4C, 0xD83D };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res == 0);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 1);
	boa_assert(!strcmp((char*)dst.data, "A"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_surrogate_singleghi, "Single high surrogate")
{
	uint16_t data[] = { (uint16_t)'A', 0xD83D, (uint16_t)'B' };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res == 0);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 1);
	boa_assert(!strcmp((char*)dst.data, "A"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_surrogate_singlelo, "Single low surrogate")
{
	uint16_t data[] = { (uint16_t)'A', 0xDC4C, (uint16_t)'B' };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, boa_arrayend(data));

	boa_assert(res == 0);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 1);
	boa_assert(!strcmp((char*)dst.data, "A"));

	boa_reset(&dst);
}

BOA_TEST(utf16to8_truncate, "Converting should handle truncated output")
{
	uint16_t data[] = { (uint16_t)'A', (uint16_t)'B', (uint16_t)'C', (uint16_t)'D', 0 };
	const uint16_t *ptr = data;
	char result[2];
	boa_buf dst = boa_array_view(result);
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(boa_clear(&dst), &ptr, boa_arrayend(data) - 1);

	boa_assert(res == 0);
	boa_assert(ptr == data + 2);
	boa_assert(dst.end_pos == 2);
	boa_assert(!memcmp((char*)dst.data, "AB", 2));

	ptr = data;
	res = boa_convert_utf16_to_utf8(boa_clear(&dst), &ptr, NULL);

	boa_assert(res == 0);
	boa_assert(ptr == data + 2);
	boa_assert(dst.end_pos == 2);
	boa_assert(!memcmp((char*)dst.data, "AB", 2));

	boa_reset(&dst);
}
