
#include <boa_test.h>
#include <boa_unicode_cpp.h>

BOA_TEST(unicode_cpp_8to16, "UTF-8 to UTF-16 conversion in C++")
{
	const char *utf8 = "ABC";
	boa::buf<uint16_t> utf16;
	boa::result res = boa::convert_utf8_to_utf16(utf16, utf8, nullptr);
	boa_assert(res == boa::ok);
	boa_assert(utf16[0] == (uint16_t)'A');
	boa_assert(utf16[1] == (uint16_t)'B');
	boa_assert(utf16[2] == (uint16_t)'C');
}
