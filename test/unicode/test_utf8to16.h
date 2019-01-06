
#include <boa_test.h>
#include <boa_unicode.h>
#include <string.h>
#include "test_codepoints.h"

BOA_TEST(utf8to16_ascii, "Converting UTF-8 to UTF-16 ASCII")
{
	uint16_t result[] = { 0x41, 0x42, 0x43, 0 };
	char data[] = { 'A', 'B', 'C', 0 };
	const char *ptr = data;
	boa_buf dst = { 0 };
	boa_result res;

	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 6);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, NULL);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 6);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_reset(&dst);
}

BOA_TEST(utf8to16_bmp, "Convert a basic multilingual plane text")
{
	uint16_t result[] = { 0x00E4, 0x65E5, 0x672C, 0x306B, 0x307B, 0x3093, 0 };
	char data[] = "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3\x81\xAB\xE3\x81\xBB\xE3\x82\x93";
	const char *ptr = data;
	boa_buf dst = { 0 };
	boa_result res;

	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 12);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, NULL);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 12);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_reset(&dst);
}

BOA_TEST(utf8to16_emoji, "Convert a few emojis")
{
	uint16_t result[] = { 0xD83D, 0xDC4C, 0xD83D, 0xDC40, 0 };
	char data[] = "\xF0\x9F\x91\x8C\xF0\x9F\x91\x80";
	const char *ptr = data;
	boa_buf dst = { 0 };
	boa_result res;

	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 8);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_clear(&dst);
	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, NULL);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 8);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_reset(&dst);
}

BOA_TEST(utf8to16_nulls, "Should be able to encode multiple NULL characters")
{
	uint16_t result[] = { 0, 0, 0, (uint16_t)'A', 0 };
	char data[] = "\0\0\0A";
	const char *ptr = data;
	boa_buf dst = { 0 };
	boa_result res;

	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, boa_arrayend(data) - 1);

	boa_assert_ok(res);
	boa_assert(dst.end_pos == 8);
	boa_assert(!memcmp((char*)dst.data, result, sizeof(result)));

	boa_reset(&dst);
}

BOA_TEST(utf8to16_test_codepoints, "Should be able to convert test codepoint set")
{
	boa_buf buf = boa_empty_buf();

	uint32_t count = boa_arraycount(test_codepoints);
	for (uint32_t i = 0; i < count; i++) {
		const test_codepoint *point = &test_codepoints[i];
		boa_test_hint_u32(i);

		const char *ptr;
		boa_result res;
		char local[5];
		memcpy(local, point->utf8, point->utf8_len * sizeof(char));
		local[point->utf8_len] = 0;

		boa_clear(&buf);
		ptr = (char*)point->utf8;
		res = boa_convert_utf8_to_utf16(&buf, &ptr, ptr + point->utf8_len);

		boa_assert_ok(res);
		boa_assert(buf.end_pos == point->utf16_len * sizeof(uint16_t));
		boa_assert(ptr == (char*)point->utf8 + point->utf8_len);
		boa_assert(!memcmp(buf.data, point->utf16, point->utf16_len));

		boa_clear(&buf);
		ptr = local;
		res = boa_convert_utf8_to_utf16(&buf, &ptr, NULL);

		boa_assert_ok(res);
		boa_assert(buf.end_pos == point->utf16_len * sizeof(uint16_t));
		boa_assert(ptr == local + point->utf8_len);
		boa_assert(!memcmp(buf.data, point->utf16, point->utf16_len));
	}

	boa_reset(&buf);
}

BOA_TEST(utf8to16_test_codepoints_concat, "Should be able to convert test codepoint set concatenated")
{
	boa_buf src = boa_empty_buf();
	boa_buf target = boa_empty_buf();
	boa_buf buf = boa_empty_buf();
	const char *ptr;
	boa_result res;

	uint32_t count = boa_arraycount(test_codepoints);
	for (uint32_t i = 0; i < count; i++) {
		const test_codepoint *point = &test_codepoints[i];
		res = boa_push_data_n(&src, point->utf8, point->utf8_len) ? boa_ok : &boa_err_no_space;
		boa_assert_ok(res);
		res = boa_push_data_n(&target, point->utf16, point->utf16_len) ? boa_ok : &boa_err_no_space;
		boa_assert_ok(res);
	}

	ptr = boa_begin(char, &src);
	res = boa_convert_utf8_to_utf16(boa_clear(&buf), &ptr, boa_end(char, &src));

	boa_assert_ok(res);
	boa_assert(buf.end_pos == target.end_pos);
	boa_assert(ptr == boa_end(char, &src));
	boa_assert(!memcmp(buf.data, target.data, buf.end_pos));

	boa_push_val(char, &src, 0);
	ptr = boa_begin(char, &src);
	res = boa_convert_utf8_to_utf16(boa_clear(&buf), &ptr, NULL);

	boa_assert_ok(res);
	boa_assert(buf.end_pos == target.end_pos);
	boa_assert(ptr == boa_end(char, &src) - 1);
	boa_assert(!memcmp(buf.data, target.data, buf.end_pos));

	boa_reset(&src);
	boa_reset(&target);
	boa_reset(&buf);
}

BOA_TEST(utf8to16_unexpected_continuation, "Unpaired surrogate due to end")
{
	uint16_t result[] = { 0x41 };
	char data[] = { 'A', (char)0xA4, 'A' };
	const char *ptr = data;
	boa_buf dst = { 0 };
	boa_result res;

	ptr = data;
	res = boa_convert_utf8_to_utf16(&dst, &ptr, boa_arrayend(data));

	boa_assert(res != boa_ok);
	boa_assert(ptr == data + 1);
	boa_assert(dst.end_pos == 2);
	boa_assert(!memcmp(dst.data, result, sizeof(result)));

	boa_reset(&dst);
}

