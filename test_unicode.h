
#include "boa_test.h"
#include "boa_unicode.h"

BOA_TEST(utf16_decode_single, "Decode a single word UTF-16 character")
{
	uint16_t data[] = { 0x41 };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data)) == 0x41);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0x41);
}

BOA_TEST(utf16_decode_double, "Decode a double word UTF-16 character")
{
	uint16_t data[] = { 0xD83D, 0xDC4C };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data)) == 0x1F44C);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0x1F44C);
}

BOA_TEST(utf16_decode_null, "Decode NULL character succesfully")
{
	uint16_t data[] = { 0x0000 };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data)) == 0);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0);
}

BOA_TEST(utf16_decode_max, "Decode the maximum character succesfully")
{
	uint16_t data[] = { 0xDBFF, 0xDFFF };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data)) == 0x10FFFF);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0x10FFFF);
}

BOA_TEST(utf16_decode_unexpected_low, "Fail on unexpected low surrogate")
{
	uint16_t data[] = { 0xDC4C, 0x0000 };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data) - 1) == ~0u);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == ~0u);
}

BOA_TEST(utf16_decode_unpaired_high, "Fail on unpaired high surrogate")
{
	uint16_t data[] = { 0xD83D, 0x0000 };
	uint16_t *ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, data + boa_arraycount(data)) == ~0u);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == ~0u);
}

BOA_TEST(utf16_decode_edges, "Handle characters on the edges of surrogates")
{
	uint16_t data[] = { 0xD7FF, 0xE000, 0xFFFF };
	uint16_t *ptr = data, *end = data + boa_arraycount(data);
	boa_expect(boa_decode_point_utf16(&ptr, end) == 0xD7FF);
	boa_expect(boa_decode_point_utf16(&ptr, end) == 0xE000);
	boa_expect(boa_decode_point_utf16(&ptr, end) == 0xFFFF);
	ptr = data;
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0xD7FF);
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0xE000);
	boa_expect(boa_decode_point_utf16(&ptr, NULL) == 0xFFFF);
}

BOA_TEST(utf8_encode_ascii, "UTF-8 encoding should work with ASCII")
{
	char data[32], *ptr = data, *end = boa_arrayend(data);
	boa_expect(boa_encode_point_utf8(&ptr, end, 0x41) != 0);
	boa_expect(boa_encode_point_utf8(&ptr, end, 0x42) != 0);
	boa_expect(boa_encode_point_utf8(&ptr, end, 0x43) != 0);
	boa_expect(ptr < end);
	*ptr = '\0';
	boa_expect(!strcmp(data, "ABC"));
}

BOA_TEST(utf8_encode_null, "Should be able to endcode a NULL character")
{
	char data[32], *ptr = data, *end = boa_arrayend(data);
	data[0] = 'A';
	boa_expect(boa_encode_point_utf8(&ptr, end, 0x00) != 0);
	boa_expect(data[0] == '\0');
}

BOA_TEST(utf8_encode_bmp, "Should be able to endcode BMP text")
{
	uint32_t points[] = { 0x00E4, 0x65E5, 0x672C, 0x306B, 0x307B, 0x3093 };
	uint32_t *pi, *pe = boa_arrayend(points);
	char data[32], *ptr = data, *end = boa_arrayend(data);
	for (pi = points; pi != pe; pi++) {
		boa_expect(boa_encode_point_utf8(&ptr, end, *pi) != 0);
	}
	boa_expect(ptr < end);
	*ptr = '\0';
	boa_expect(!strcmp(data, "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3"
		"\x81\xAB\xE3\x81\xBB\xE3\x82\x93"));
}

BOA_TEST(utf8_encode_edges, "UTF-8 encode should handle range edges")
{
	uint32_t points[] = { 0x7F, 0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x10FFFF };
	uint32_t *pi, *pe = boa_arrayend(points);
	char data[32], *ptr = data, *end = boa_arrayend(data);
	for (pi = points; pi != pe; pi++) {
		boa_expect(boa_encode_point_utf8(&ptr, end, *pi) != 0);
	}
	boa_expect(ptr < end);
	*ptr = '\0';
	boa_expect(!strcmp(data, "\x7F\xC2\x80\xDF\xBF\xE0\xA0\x80\xEF"
		"\xBF\xBF\xF0\x90\x80\x80\xF4\x8F\xBF\xBF"));
}

BOA_TEST(utf16to8_ascii, "Converting UTF-16 to UTF-8 ASCII")
{
	uint16_t data[] = { 0x41, 0x42, 0x43, 0 };
	boa_vec dst = { 0 };
	int res;

	res = boa_convert_utf16_to_utf8(&dst, data, data + boa_arraycount(data) - 1);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 3);
	boa_expect(!strcmp(dst.buf.ptr, "ABC"));

	dst.byte_pos = 0;
	res = boa_convert_utf16_to_utf8(&dst, data, NULL);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 3);
	boa_expect(!strcmp(dst.buf.ptr, "ABC"));

	dst.byte_pos = 0;
}

BOA_TEST(utf16to8_bmp, "Convert a basic multilingual plane text")
{
	uint16_t data[] = { 0x00E4, 0x65E5, 0x672C, 0x306B, 0x307B, 0x3093, 0 };
	boa_vec dst = { 0 };
	int res;

	res = boa_convert_utf16_to_utf8(&dst, data, data + boa_arraycount(data) - 1);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 17);
	boa_expect(!strcmp(dst.buf.ptr, "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3"
		"\x81\xAB\xE3\x81\xBB\xE3\x82\x93"));

	dst.byte_pos = 0;
	res = boa_convert_utf16_to_utf8(&dst, data, NULL);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 17);
	boa_expect(!strcmp(dst.buf.ptr, "\xC3\xA4\xE6\x97\xA5\xE6\x9C\xAC\xE3"
		"\x81\xAB\xE3\x81\xBB\xE3\x82\x93"));
}

BOA_TEST(utf16to8_emoji, "Convert a few emojis")
{
	uint16_t data[] = { 0xD83D, 0xDC4C, 0xD83D, 0xDC40, 0 };
	boa_vec dst = { 0 };
	int res;

	res = boa_convert_utf16_to_utf8(&dst, data, data + boa_arraycount(data) - 1);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 8);
	boa_expect(!strcmp(dst.buf.ptr, "\xF0\x9F\x91\x8C\xF0\x9F\x91\x80"));

	dst.byte_pos = 0;
	res = boa_convert_utf16_to_utf8(&dst, data, NULL);

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 8);
	boa_expect(!strcmp(dst.buf.ptr, "\xF0\x9F\x91\x8C\xF0\x9F\x91\x80"));
}

BOA_TEST(utf16to8_nulls, "Should be able to encode multiple NULL characters")
{
	uint16_t data[] = { 0, 0, 0, (uint16_t)'A' };
	boa_vec dst = { 0 };
	int res;

	res = boa_convert_utf16_to_utf8(&dst, data, data + boa_arraycount(data));

	boa_expect(res != 0);
	boa_expect(dst.byte_pos == 4);
	boa_expect(!memcmp(dst.buf.ptr, "\0\0\0A\0", 5));
}
