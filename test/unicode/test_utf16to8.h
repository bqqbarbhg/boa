
#include <boa_test.h>
#include <boa_unicode.h>
#include <string.h>

BOA_TEST(utf16to8_ascii, "Converting UTF-16 to UTF-8 ASCII")
{
	uint16_t data[] = { 0x41, 0x42, 0x43, 0 };
	const uint16_t *ptr = data;
	boa_buf dst = { 0 };
	int res;

	ptr = data;
	res = boa_convert_utf16_to_utf8(&dst, &ptr, data + boa_arraycount(data) - 1);

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
	res = boa_convert_utf16_to_utf8(&dst, &ptr, data + boa_arraycount(data) - 1);

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
	res = boa_convert_utf16_to_utf8(&dst, &ptr, data + boa_arraycount(data) - 1);

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
	res = boa_convert_utf16_to_utf8(&dst, &ptr, data + boa_arraycount(data));

	boa_assert(res != 0);
	boa_assert(dst.end_pos == 4);
	boa_assert(!memcmp((char*)dst.data, "\0\0\0A\0", 5));

	boa_reset(&dst);
}
