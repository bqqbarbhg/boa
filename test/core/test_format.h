
#include <boa_test.h>
#include <string.h>

BOA_TEST(format, "boa_format() should work like sprintf()")
{
	boa_buf buf = { 0 };
	const char *str = boa_format(&buf, "Hello %s", "World");
	boa_assert(strcmp(str, "Hello World") == 0);
	boa_assert(buf.end_pos == strlen("Hello World"));
	boa_reset(&buf);
}

BOA_TEST(format_stack, "Format should work to stack")
{
	char data[128];
	boa_buf buf = boa_array_buf(data);
	char *str = boa_format(&buf, "Hello %s", "World");
	boa_assert(str == data);
	boa_assert(strcmp(data, "Hello World") == 0);
	boa_assert(buf.end_pos == strlen("Hello World"));
}

BOA_TEST(format_stack_exact, "Format should work byte exactly to stack")
{
	char data[4];
	boa_buf buf = boa_array_buf(data);
	char *str = boa_format(&buf, "abc");
	boa_assert(str == data);
	boa_assert(strcmp(data, "abc") == 0);
	boa_assert(buf.end_pos == 3);
}

BOA_TEST(format_stack_overflow, "Format should allocate when overflowing")
{
	char data[4];
	boa_buf buf = boa_array_buf(data);
	char *str = boa_format(&buf, "abcd");
	boa_assert(str != data);
	boa_assert(strcmp(buf.data, "abcd") == 0);
	boa_assert(buf.end_pos == 4);
	boa_reset(&buf);
}

BOA_TEST(format_stack_no_space, "Format should fail gracefully when not enough space")
{
	char data[4];
	boa_buf buf = boa_array_view(data);
	char *str = boa_format(&buf, "abcd");
	boa_assert(str == NULL);
	boa_assert(buf.end_pos == 0);
}

BOA_TEST(format_cat, "Format should concatenate consecutive calls")
{
	char data[64];
	boa_buf buf = boa_array_view(data);
	boa_assert(boa_format(&buf, "abcd") == data + 0);
	boa_assert(boa_format(&buf, "efgh") == data + 4);
	boa_assert(boa_format(&buf, "ijkl") == data + 8);
	boa_assert(strcmp(data, "abcdefghijkl") == 0);
	boa_assert(buf.end_pos == 12);
}

BOA_TEST(format_cat_realloc, "Format should handle reallocation in the middle of concatenation")
{
	char data[6];
	boa_buf buf = boa_array_buf(data);
	boa_assert(boa_format(&buf, "abcd") != NULL);
	boa_assert(boa_format(&buf, "efgh") != NULL);
	boa_assert(boa_format(&buf, "ijkl") != NULL);
	boa_assert(strcmp((char*)buf.data, "abcdefghijkl") == 0);
	boa_assert(buf.end_pos == 12);
	boa_reset(&buf);
}

