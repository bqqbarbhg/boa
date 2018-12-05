
#include "boa_test.h"

#if BOA_TEST_IMPL

static int buf_equal(const boa_buf *a, const boa_buf *b)
{
	if (a->buf_ptr != b->buf_ptr) return 0;
	if (a->buf_cap != b->buf_cap) return 0;
	if (a->ptr != b->ptr) return 0;
	if (a->cap != b->cap) return 0;
	return 1;
}

static int vec_equal(const boa_vec *a, const boa_vec *b)
{
	if (!buf_equal(&a->buf, &b->buf)) return 0;
	if (a->byte_pos != b->byte_pos) return 0;
	return 1;
}

#endif

BOA_TEST(buf_empty, "Empty buffer should be empty")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(buf.ptr == NULL);
	boa_expectf(buf.cap == 0, "buf.cap = %d", (int)buf.cap);
}

BOA_TEST(buf_zeroinit, "Zero initialized buffer should equal to boa_buf_empty()")
{
	boa_buf a = { 0 };
	boa_buf b = boa_buf_empty();
	boa_expect(buf_equal(&a, &b));
}

BOA_TEST(buf_make, "Initialized buffer should point to buffer")
{
	char data[64];
	boa_buf buf = boa_buf_make(data, sizeof(data));
	boa_expect(buf.ptr == data);
	boa_expect(buf.buf_ptr == data);
}

BOA_TEST(buf_array, "Array shorthand should be equivalent to make")
{
	char data[64];
	boa_buf a = boa_buf_make(data, sizeof(data));
	boa_buf b = boa_buf_array(data);
	boa_expect(buf_equal(&a, &b));
}

BOA_TEST(buf_alloc_empty, "Allocation from empty buffer")
{
	boa_buf buf = { 0 };
	void *ptr = boa_buf_alloc(&buf, 32);
	boa_expect(ptr != NULL);
	boa_expect(buf.ptr == ptr);
	boa_expect(buf.cap == 32);
	boa_expect(buf.buf_ptr == NULL);
	boa_expect(buf.buf_cap == 0);
}

BOA_TEST(buf_alloc_small, "Smaller allocation from buffer should not malloc()")
{
	char data[64];
	boa_buf buf = boa_buf_array(data);
	void *ptr = boa_buf_alloc(&buf, 32);
	boa_expect(ptr != NULL);
	boa_expect(buf.ptr == ptr);
	boa_expect(buf.buf_ptr == ptr);
	boa_expect(buf.cap == 64);
}

BOA_TEST(buf_alloc_equal, "Equal sized allocation from buffer should not malloc()")
{
	char data[64];
	boa_buf buf = boa_buf_array(data);
	void *ptr = boa_buf_alloc(&buf, 64);
	boa_expect(ptr != NULL);
	boa_expect(buf.ptr == ptr);
	boa_expect(buf.buf_ptr == ptr);
	boa_expect(buf.cap == 64);
}

BOA_TEST(buf_alloc_large, "Larger allocation from buffer should malloc()")
{
	char data[64];
	boa_buf buf = boa_buf_array(data);
	void *ptr = boa_buf_alloc(&buf, 128);
	boa_expect(ptr != NULL);
	boa_expect(buf.ptr == ptr);
	boa_expect(buf.buf_ptr != ptr);
	boa_expect(buf.cap == 128);
	boa_expect(buf.buf_cap == 64);
}

BOA_TEST(buf_alloc_grow, "Allocation should be able to grow")
{
	boa_buf buf = boa_buf_empty();
	boa_buf_alloc(&buf, 64);
	boa_buf prev = buf;
	boa_buf_alloc(&buf, 128);
	boa_expect(!buf_equal(&prev, &buf));
}

BOA_TEST(buf_realloc_zero, "Realloc should work from zero")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(boa_buf_realloc(&buf, 32) != NULL);
	boa_buf_reset(&buf);
}

BOA_TEST(buf_realloc_heap, "Realloc should preserve data from heap")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(boa_buf_alloc(&buf, 64) != NULL);
	for (int i = 0; i < 64; i++)
		((char*)buf.ptr)[i] = (char)i;
	boa_expect(boa_buf_realloc(&buf, 128) != NULL);
	for (int i = 0; i < 64; i++)
		boa_expect(((char*)buf.ptr)[i] == (char)i);
	boa_buf_reset(&buf);
}

BOA_TEST(buf_realloc_stack, "Realloc should preserve data from stack")
{
	char data[] = "Hello world!";
	boa_buf buf = boa_buf_make(data, sizeof(data));
	boa_expect(boa_buf_realloc(&buf, 128) != NULL);
	boa_expect(buf.ptr != data);
	boa_expect(strcmp(buf.ptr, data) == 0);
	boa_buf_reset(&buf);
}

BOA_TEST(buf_realloc_nop, "Realloc shuold be a nop if it doesn't grow")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(boa_buf_alloc(&buf, 64) != NULL);
	boa_buf prev = buf;
	boa_expect(boa_buf_realloc(&buf, 32) != NULL);
	boa_expect(buf_equal(&prev, &buf));
	boa_buf_reset(&buf);
}

BOA_TEST(buf_reset_empty, "reset() on empty buffer should work")
{
	boa_buf zero = { 0 };
	boa_buf_reset(&zero);
}

BOA_TEST(buf_reset_stack, "reset() on a stack buffer should work")
{
	char data[32];
	boa_buf buf = boa_buf_array(data);
	boa_buf_reset(&buf);
}

BOA_TEST(buf_reset_to_zero, "reset() should reset the buffer back to zero")
{
	boa_buf zero = { 0 };
	boa_buf buf = zero;
	boa_expect(boa_buf_alloc(&buf, 16) != NULL);
	boa_expect(!buf_equal(&zero, &buf));
	boa_buf_reset(&buf);
	boa_expect(buf_equal(&zero, &buf));
}

BOA_TEST(buf_reset_to_buffer, "reset() should reset the buffer back to preallocated")
{
	char data[16];
	boa_buf original = boa_buf_array(data);
	boa_buf buf = original;
	boa_expect(buf.cap == 16);
	boa_expect(boa_buf_alloc(&buf, 64) != NULL);
	boa_expect(!buf_equal(&original, &buf));
	boa_buf_reset(&buf);
	boa_expect(buf_equal(&original, &buf));
}

BOA_TEST(buf_alloc_fail, "Failure on boa_buf_alloc()")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(boa_buf_alloc(&buf, 64) != NULL);
	boa_buf prev = buf;

	boa_test_fail_next_allocation();

	boa_expectf(boa_buf_alloc(&buf, 128) == NULL, "Should return NULL");
	boa_expectf(buf_equal(&prev, &buf), "Should leave the buffer unchanged");
	boa_buf_reset(&buf);
}

BOA_TEST(buf_realloc_fail, "Failure on boa_buf_realloc()")
{
	boa_buf buf = boa_buf_empty();
	boa_expect(boa_buf_alloc(&buf, 64) != NULL);
	boa_buf prev = buf;

	boa_test_fail_next_allocation();

	boa_expectf(boa_buf_realloc(&buf, 128) == NULL, "Should return NULL");
	boa_expectf(buf_equal(&prev, &buf), "Should leave the buffer unchanged");
	boa_buf_reset(&buf);
}

BOA_TEST(vec_push, "Vec push should append data to the buffer")
{
	boa_vec vec = { 0 };
	*(int32_t*)boa_vec_push(&vec, 4) = 1;
	*(int32_t*)boa_vec_push(&vec, 4) = 2;
	*(int32_t*)boa_vec_push(&vec, 4) = 3;

	boa_expect(vec.byte_pos == 4 * 3);
	boa_expect(vec.buf.cap >= 4 * 3);
	boa_expect(((int32_t*)vec.buf.ptr)[0] == 1);
	boa_expect(((int32_t*)vec.buf.ptr)[1] == 2);
	boa_expect(((int32_t*)vec.buf.ptr)[2] == 3);

	boa_buf_reset(&vec.buf);
}

BOA_TEST(vec_push_stack, "Vec push should work to an existing buffer")
{
	int32_t data[4];
	boa_vec vec = boa_vec_array(data);
	*(int32_t*)boa_vec_push(&vec, 4) = 1;
	*(int32_t*)boa_vec_push(&vec, 4) = 2;
	*(int32_t*)boa_vec_push(&vec, 4) = 3;

	boa_expect(vec.byte_pos == 4 * 3);
	boa_expect(data[0] == 1);
	boa_expect(data[1] == 2);
	boa_expect(data[2] == 3);

	boa_vec_reset(&vec);
}

BOA_TEST(vec_push_stack_over, "Vec push should work overflowing an buffer")
{
	int32_t data[2];
	boa_vec vec = boa_vec_array(data);
	*(int32_t*)boa_vec_push(&vec, 4) = 1;
	*(int32_t*)boa_vec_push(&vec, 4) = 2;
	*(int32_t*)boa_vec_push(&vec, 4) = 3;

	boa_expect(vec.byte_pos == 4 * 3);
	boa_expect(data[0] == 1);
	boa_expect(data[1] == 2);
	boa_expect(boa_vbegin(int32_t, vec)[2] == 3);

	boa_vec_reset(&vec);
}

BOA_TEST(vec_push_fail, "Failing vector push should return NULL")
{
	boa_vec vec = boa_vec_empty();

	boa_test_fail_next_allocation();
	boa_expect(boa_vec_push(&vec, 4) == NULL);
}

BOA_TEST(vec_push_fail_change, "Failing vector push should not modify the vector")
{
	int32_t data[2];
	boa_vec vec = boa_vec_array(data);
	*(int32_t*)boa_vec_push(&vec, 4) = 1;
	*(int32_t*)boa_vec_push(&vec, 4) = 2;
	boa_expect(vec.buf.ptr == data);

	boa_vec prev = vec;

	boa_test_fail_next_allocation();

	boa_expect(boa_vec_push(&vec, 4) == NULL);
	boa_expect(vec_equal(&vec, &prev));

	boa_expect(data[0] == 1);
	boa_expect(data[1] == 2);

	boa_buf_reset(&vec.buf);
}

BOA_TEST(vec_shorthands, "Vector shorthands")
{
	boa_vec vec = { 0 };

	*boa_vpush(int32_t, vec) = 1;
	boa_vadd(int32_t, vec, 2);
	boa_vadd(int32_t, vec, 3);

	boa_expect(vec.byte_pos == 4 * 3);
	boa_expect(vec.buf.cap >= 4 * 3);
	boa_expect(((int32_t*)vec.buf.ptr)[0] == 1);
	boa_expect(((int32_t*)vec.buf.ptr)[1] == 2);
	boa_expect(((int32_t*)vec.buf.ptr)[2] == 3);
	boa_expect(boa_vcount(int32_t, vec) == 3);
	boa_expect(boa_vbegin(int32_t, vec) == (int32_t*)vec.buf.ptr);
	boa_expect(boa_vend(int32_t, vec) == (int32_t*)vec.buf.ptr + 3);

	boa_vec_reset(&vec);
}

BOA_TEST(vec_bytesleft, "Vector bytesleft shorthand")
{
	int32_t data[4];
	boa_vec vec = boa_vec_array(data);

	boa_vadd(int32_t, vec, 1);

	boa_expect(boa_vbytesleft(vec) == 12);
}

BOA_TEST(malloc, "Malloc should return a valid pointer")
{
	int *ptr = (int*)boa_malloc(sizeof(int));
	boa_expect(ptr != NULL);
	*ptr = 3;
	boa_free(ptr);
}

BOA_TEST(realloc, "Realloc should free an existing pointer on failure")
{
	int *a, *b;
	a = (int*)boa_malloc(sizeof(int));
	boa_test_fail_next_allocation();
	b = (int*)boa_realloc(a, sizeof(int));
	boa_expect(a != NULL);
	boa_expect(b == NULL);
}

BOA_TEST(free_zero, "free(NULL) should be a no-op")
{
	boa_free(NULL);
}

BOA_TEST(format, "boa_format() should work like sprintf()")
{
	boa_buf buf = { 0 };
	const char *str = boa_format(&buf, "Hello %s", "World");
	boa_expect(strcmp(str, "Hello World") == 0);
	boa_buf_reset(&buf);
}

BOA_TEST(format_stack, "Format should work to stack")
{
	char data[128];
	boa_buf buf = boa_buf_array(data);
	char *str = boa_format(&buf, "Hello %s", "World");
	boa_expect(str == data);
	boa_expect(strcmp(data, "Hello World") == 0);
}

BOA_TEST(format_stack_exact, "Format should work byte exactly to stack")
{
	char data[4];
	boa_buf buf = boa_buf_array(data);
	char *str = boa_format(&buf, "abc");
	boa_expect(str == data);
	boa_expect(strcmp(data, "abc") == 0);
}

BOA_TEST(format_stack_overflow, "Format should allocate when overflowing")
{
	char data[4];
	boa_buf buf = boa_buf_array(data);
	char *str = boa_format(&buf, "abcd");
	boa_expect(str != data);
	boa_expect(strcmp(buf.ptr, "abcd") == 0);
	boa_buf_reset(&buf);
}

BOA_TEST(format_vec, "Appending format() to a vector")
{
	boa_vec vec = { 0 };
	boa_format_push(&vec, "A: %d", 1);
	boa_format_push(&vec, ", ");
	boa_format_push(&vec, "B: %d", 2);

	boa_expectf(strcmp(boa_vbegin(char, vec), "A: 1, B: 2") == 0,
			"vec = %s", boa_vbegin(char, vec));
	boa_expect(vec.byte_pos == strlen("A: 1, B: 2"));
	boa_vec_reset(&vec);
}

BOA_TEST(format_vec_stack, "Appending format() works to a stack vector")
{
	char data[4];
	boa_vec vec = boa_vec_array(data);
	boa_format_push(&vec, "ABC");
	boa_expect(vec.buf.ptr == data);
	boa_format_push(&vec, "D");
	boa_expect(vec.buf.ptr != data);
	boa_format_push(&vec, "EFG");

	boa_expectf(strcmp(boa_vbegin(char, vec), "ABCDEFG") == 0,
			"vec = %s", boa_vbegin(char, vec));
	boa_expect(vec.byte_pos == 7);
	boa_vec_reset(&vec);
}

BOA_TEST(arraycount, "boa_arraycount() should return the number of elements in array")
{
	int arr[16];
	boa_expect(boa_arraycount(arr) == 16);
}

BOA_TEST(arrayend, "boa_arrayend() should return pointer past the last element")
{
	int arr[16];
	boa_expect(boa_arrayend(arr) == arr + 16);
}

