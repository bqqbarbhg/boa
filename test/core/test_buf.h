
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL

static int buf_equal(const boa_buf *a, const boa_buf *b)
{
	if (a->data != b->data) return 0;
	if (a->end_pos != b->end_pos) return 0;
	if (a->cap_pos != b->cap_pos) return 0;
	return 1;
}

#endif

BOA_TEST(buf_make, "Can manually define buffer")
{
	boa_allocator dummy;
	uint32_t array[4];
	boa_buf buf = boa_buf_make(array, 16, &dummy);
	boa_assert(boa_buf_ator(&buf) == &dummy);
	boa_assert(buf.data == array);
	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 16);
}

BOA_TEST(buf_zeroinit, "Zero initialized buffer should equal to boa_empty_buf()")
{
	boa_buf a = { 0 };
	boa_buf b = boa_empty_buf();
	boa_assert(buf_equal(&a, &b));
}

BOA_TEST(buf_alloc_empty, "Allocation from empty buffer")
{
	boa_buf buf = { 0 };
	void *ptr = boa_buf_reserve(&buf, 32);
	boa_assert(ptr != NULL);
	boa_assert(buf.data == ptr);
	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 32);
	boa_reset(&buf);
}

BOA_TEST(buf_alloc_small, "Smaller allocation from buffer should not malloc()")
{
	char data[64];
	boa_buf buf = boa_array_buf(data);
	void *ptr = boa_buf_reserve(&buf, 32);
	boa_assert(ptr != NULL);
	boa_assert(buf.data == ptr);
	boa_assert(buf.data == data);
	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 64);
}

BOA_TEST(buf_alloc_equal, "Equal sized allocation from buffer should not malloc()")
{
	char data[64];
	boa_buf buf = boa_array_buf(data);
	void *ptr = boa_buf_reserve(&buf, 64);
	boa_assert(ptr != NULL);
	boa_assert(buf.data == ptr);
	boa_assert(buf.data == data);
	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 64);
}


BOA_TEST(buf_alloc_large, "Larger allocation from buffer should malloc()")
{
	char data[64];
	boa_buf buf = boa_array_buf(data);
	void *ptr = boa_buf_reserve(&buf, 128);
	boa_assert(ptr != NULL);
	boa_assert(buf.data != data);
	boa_assert(buf.data == ptr);
	boa_assert(buf.cap_pos == 128);
	boa_reset(&buf);
}

BOA_TEST(buf_alloc_grow, "Allocation should be able to grow")
{
	boa_buf buf = boa_empty_buf();
	boa_buf_reserve(&buf, 64);
	boa_buf prev = buf;
	boa_buf_reserve(&buf, 256);
	boa_assert(!buf_equal(&prev, &buf));
	boa_reset(&buf);
}

BOA_TEST(buf_reset_empty, "reset() on empty buffer should work")
{
	boa_buf zero = { 0 };
	boa_reset(&zero);
}

BOA_TEST(buf_reset_stack, "reset() on a stack buffer should work")
{
	char data[32];
	boa_buf buf = boa_array_buf(data);
	boa_reset(&buf);
}

BOA_TEST(buf_reset_to_zero, "reset() should reset the buffer back to zero")
{
	boa_buf zero = { 0 };
	boa_buf buf = zero;
	boa_assert(boa_buf_reserve(&buf, 16) != NULL);
	boa_assert(!buf_equal(&zero, &buf));
	boa_reset(&buf);
	boa_assert(buf_equal(&zero, &buf));
}

BOA_TEST(buf_reset_to_buffer, "reset() should reset the buffer back to preallocated")
{
	char data[16];
	boa_buf original = boa_array_buf(data);
	boa_buf buf = original;
	boa_assert(buf.cap_pos == 16);
	boa_assert(boa_buf_reserve(&buf, 64) != NULL);
	boa_assert(!buf_equal(&original, &buf));
	boa_reset(&buf);
	boa_assert(buf_equal(&original, &buf));
}

BOA_TEST(buf_alloc_fail, "Failure on boa_buf_reserve()")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_reserve(&buf, 64) != NULL);
	boa_buf prev = buf;

	boa_test_fail_next_allocation();

	boa_assertf(boa_buf_reserve(&buf, 256) == NULL, "Should return NULL");
	boa_assertf(buf_equal(&prev, &buf), "Should leave the buffer unchanged");
	boa_reset(&buf);
}

BOA_TEST(buf_bump, "Bump should advance the buffer position")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_reserve(&buf, 16) != NULL);
	boa_buf_bump(&buf, 16);
	boa_reset(&buf);
}

BOA_TEST(buf_bump_assert, "Bump should assert if there's no space")
{
	boa_buf buf = boa_empty_buf();
	boa_expect_assert( boa_buf_bump(&buf, 16) );
	boa_assert(boa_buf_reserve(&buf, 16) != NULL);
	boa_buf_bump(&buf, 16);
	boa_expect_assert( boa_buf_bump(&buf, 16) );
	boa_reset(&buf);
}

BOA_TEST(buf_push, "Push should reserve and bump at once")
{
	boa_buf buf = boa_empty_buf();
	void *ptr = boa_buf_push(&buf, 16);
	boa_assert(ptr == buf.data);
	boa_assert(buf.end_pos == 16);
	boa_assert(buf.cap_pos == 16);
	boa_reset(&buf);
}

BOA_TEST(buf_push_no_space, "Push should leave the buffer unmodified on fail")
{
	boa_buf buf = boa_empty_buf();

	boa_assert(boa_buf_push(&buf, 16) != NULL);

	boa_test_fail_next_allocation();
	boa_assert(boa_buf_push(&buf, 16) == NULL);

	boa_assert(buf.end_pos == 16);
	boa_assert(buf.cap_pos == 16);
	boa_reset(&buf);
}

BOA_TEST(buf_clear, "Clear should only reset the position")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_push(&buf, 16) != NULL);

	boa_assert(buf.end_pos == 16);
	boa_assert(buf.cap_pos == 16);

	boa_clear(&buf);

	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 16);

	boa_reset(&buf);

	boa_assert(buf.end_pos == 0);
	boa_assert(buf.cap_pos == 0);
}

BOA_TEST(buf_insert, "Insert should be able to push to the front")
{
	boa_buf buf = boa_empty_buf();
	int *ptr = (int*)boa_buf_push(&buf, sizeof(int));
	boa_assert(ptr != NULL);
	*ptr = 15;
	int *head = (int*)boa_buf_insert(&buf, 0, sizeof(int));
	boa_assert(head != NULL);
	*head = 3;

	boa_assert(((int*)buf.data)[0] == 3);
	boa_assert(((int*)buf.data)[1] == 15);
	boa_assert(buf.end_pos == 2 * sizeof(int));
	boa_assert(buf.cap_pos >= 2 * sizeof(int));

	boa_reset(&buf);
}

BOA_TEST(buf_insert_middle, "Insert should be able to push to the middle")
{
	boa_buf buf = boa_empty_buf();
	int *ptr = (int*)boa_buf_push(&buf, 2 * sizeof(int));
	boa_assert(ptr != NULL);
	ptr[0] = 15;
	ptr[1] = 27;
	int *head = (int*)boa_buf_insert(&buf, sizeof(int), sizeof(int));
	boa_assert(head != NULL);
	*head = 3;

	boa_assert(((int*)buf.data)[0] == 15);
	boa_assert(((int*)buf.data)[1] == 3);
	boa_assert(((int*)buf.data)[2] == 27);
	boa_assert(buf.end_pos == 3 * sizeof(int));
	boa_assert(buf.cap_pos >= 3 * sizeof(int));

	boa_reset(&buf);
}

BOA_TEST(buf_insert_realloc, "Insert should handle reallocating")
{
	boa_buf buf = boa_empty_buf();
	int *ptr = (int*)boa_buf_push(&buf, 2 * sizeof(int));
	boa_assert(ptr != NULL);
	ptr[0] = 15;
	ptr[1] = 27;
	boa_assert(boa_buf_insert(&buf, sizeof(int), 100 * sizeof(int)) != NULL);
	boa_assert(((int*)buf.data)[0] == 15);
	boa_assert(((int*)buf.data)[101] == 27);
	boa_assert(buf.end_pos == 102 * sizeof(int));
	boa_assert(buf.cap_pos >= 102 * sizeof(int));
	boa_reset(&buf);
}

BOA_TEST(buf_insert_no_space, "Insert should handle allocation failure")
{
	boa_buf buf = boa_empty_buf();
	void *data = boa_buf_push(&buf, 16);
	boa_assert(data != NULL);

	boa_test_fail_next_allocation();
	void *ptr = boa_buf_insert(&buf, 0, 1024);
	boa_assert(ptr == NULL);

	boa_assert(buf.data == data);
	boa_assert(buf.end_pos == 16);
	boa_assert(buf.cap_pos >= 16);
	boa_reset(&buf);
}

BOA_TEST(buf_insert_out_of_bounds, "Insert should assert out of bounds")
{
	boa_buf buf = boa_empty_buf();
	boa_expect_assert( boa_buf_insert(&buf, 8, 16) );
	boa_assert(boa_buf_push(&buf, 16) != NULL);
	boa_expect_assert( boa_buf_insert(&buf, 64, 16) );
	boa_reset(&buf);
}

BOA_TEST(buf_allocator, "Using an allocator with a buffer")
{
	boa_test_allocator ator = boa_test_allocator_make();

	boa_assert(ator.allocs == 0);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 0);

	boa_buf buf = boa_empty_buf_ator(&ator.ator);
	boa_assert(boa_buf_reserve(&buf, 16) != NULL);
	boa_assert(boa_buf_reserve(&buf, 64) != NULL);
	boa_reset(&buf);

	boa_assert(ator.allocs == 1);
	boa_assert(ator.reallocs == 1);
	boa_assert(ator.frees == 1);
}

BOA_TEST(buf_set_ator, "Setting allocator to empty buffer")
{
	boa_test_allocator ator = boa_test_allocator_make();
	boa_buf buf = boa_empty_buf();

	boa_assert(boa_buf_set_ator(&buf, &ator.ator));
	boa_assert(boa_buf_ator(&buf) == &ator.ator);

	boa_assert(boa_buf_reserve(&buf, 64) != NULL);
	boa_reset(&buf);

	boa_assert(ator.allocs == 1);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 1);
}

BOA_TEST(buf_set_ator_data, "Setting allocator should copy data")
{
	int arr[1];
	boa_test_allocator ator = boa_test_allocator_make();
	boa_buf buf = boa_array_buf(arr);

	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 20);

	boa_assert(boa_buf_set_ator(&buf, &ator.ator));
	boa_assert(boa_buf_ator(&buf) == &ator.ator);

	int *data = boa_begin(int, &buf);
	boa_assert(data[0] == 10);
	boa_assert(data[1] == 20);

	boa_reset(&buf);

	boa_assert(buf.data == (void*)arr);

	boa_assert(ator.allocs == 1);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 1);
}

BOA_TEST(buf_set_ator_array, "Setting allocator should allocate unnecessarily")
{
	int arr[2];
	boa_test_allocator ator = boa_test_allocator_make();
	boa_buf buf = boa_array_buf(arr);

	boa_assert(boa_buf_set_ator(&buf, &ator.ator));
	boa_assert(boa_buf_ator(&buf) == &ator.ator);

	boa_assert(ator.allocs == 0);
}

BOA_TEST(buf_set_ator_fail, "Setting allocator should fail gracefully")
{
	boa_test_allocator ator = boa_test_allocator_make();
	boa_buf buf = boa_empty_buf();

	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 20);

	boa_test_fail_next_allocation();
	boa_assert(boa_buf_set_ator(&buf, &ator.ator) == 0);
	boa_assert(boa_buf_ator(&buf) == NULL);

	int *data = boa_begin(int, &buf);
	boa_assert(data[0] == 10);
	boa_assert(data[1] == 20);

	boa_reset(&buf);

	boa_assert(ator.allocs == 1);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 0);
}

BOA_TEST(buf_remove, "Remove should swap out the last element")
{
	boa_buf buf = boa_empty_buf();
	int *data = (int*)boa_buf_push(&buf, 4 * sizeof(int));
	data[0] = 10; data[1] = 20; data[2] = 30; data[3] = 40;

	boa_buf_remove(&buf, 3 * sizeof(int), sizeof(int));
	boa_assert(buf.end_pos == 3 * sizeof(int));
	boa_assert(data[0] == 10 && data[1] == 20 && data[2] == 30);
	boa_assert(data[3] == 40);

	boa_buf_remove(&buf, 0, sizeof(int));
	boa_assert(data[0] == 30 && data[1] == 20);

	boa_reset(&buf);
}

BOA_TEST(buf_remove_single, "Remove should work on single element")
{
	boa_buf buf = boa_empty_buf();
	int *data = (int*)boa_buf_push(&buf, 2 * sizeof(int));
	data[0] = 10;
	data[1] = 20;

	boa_buf_remove(&buf, 0, sizeof(int));
	boa_assert(data[0] == 20);
	boa_assert(buf.end_pos == sizeof(int));
	boa_buf_remove(&buf, 0, sizeof(int));
	boa_assert(buf.end_pos == 0);

	boa_reset(&buf);
}

BOA_TEST(buf_remove_assert, "Remove should assert when out of bounds or invalid")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_push(&buf, 4 * sizeof(int)) != NULL);

	boa_expect_assert( boa_buf_remove(&buf, 4 * sizeof(int), sizeof(int)) );
	boa_expect_assert( boa_buf_remove(&buf, 3 * sizeof(int) - 2, sizeof(int)) );

	boa_reset(&buf);
}

BOA_TEST(buf_erase, "Erase should shift the elements")
{
	boa_buf buf = boa_empty_buf();
	int *data = (int*)boa_buf_push(&buf, 4 * sizeof(int));
	data[0] = 10; data[1] = 20; data[2] = 30; data[3] = 40;

	boa_buf_erase(&buf, 3 * sizeof(int), sizeof(int));
	boa_assert(buf.end_pos == 3 * sizeof(int));
	boa_assert(data[0] == 10 && data[1] == 20 && data[2] == 30);
	boa_assert(data[3] == 40);

	boa_buf_erase(&buf, 0, sizeof(int));
	boa_assert(data[0] == 20 && data[1] == 30);

	boa_reset(&buf);
}

BOA_TEST(buf_erase_single, "Erase should work on single element")
{
	boa_buf buf = boa_empty_buf();
	int *data = (int*)boa_buf_push(&buf, 2 * sizeof(int));
	data[0] = 10;
	data[1] = 20;

	boa_buf_erase(&buf, 0, sizeof(int));
	boa_assert(data[0] == 20);
	boa_assert(buf.end_pos == sizeof(int));
	boa_buf_erase(&buf, 0, sizeof(int));
	boa_assert(buf.end_pos == 0);

	boa_reset(&buf);
}

BOA_TEST(buf_erase_assert, "Erase should assert when out of bounds but not have remove's limitation")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_push(&buf, 4 * sizeof(int)) != NULL);

	boa_expect_assert( boa_buf_erase(&buf, 4 * sizeof(int), sizeof(int)) );
	boa_buf_erase(&buf, 3 * sizeof(int) - 2, sizeof(int));

	boa_reset(&buf);
}

BOA_TEST(buf_get, "Buf get should return bounds checked pointer")
{
	boa_buf buf = boa_empty_buf();
	boa_assert(boa_buf_push(&buf, 16) != NULL);
	boa_assert(boa_buf_get(&buf, 8, 8) == (char*)buf.data + 8);
	boa_expect_assert( boa_buf_get(&buf, 8, 16) );
	boa_expect_assert( boa_buf_get(&buf, 16, 8) );
	boa_reset(&buf);
}

BOA_TEST(buf_shorthands_simple, "Simple buffer shorthand test")
{
	int arr[2];
	boa_buf buf = boa_array_buf(arr);

	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 30);
	boa_push_val(int, &buf, 40);
	boa_insert_val(int, &buf, 1, 20);

	int *ptr = boa_begin(int, &buf);
	int *end = boa_end(int, &buf);
	int *cap = boa_cap(int, &buf);

	boa_assert(boa_count(int, &buf) == 4);
	boa_assert(end == ptr + 4);
	boa_assert(cap >= end);
	boa_assert(ptr[0] == 10);
	boa_assert(ptr[1] == 20);
	boa_assert(ptr[2] == 30);
	boa_assert(ptr[3] == 40);

	boa_reset(&buf);
}

BOA_TEST(buf_shorthand_buf_ator, "Shorthands for bufs with allocator")
{
	boa_allocator dummy;
	int data[4];

	boa_buf empty = boa_empty_buf_ator(1 ? &dummy : &dummy);
	boa_buf range = boa_range_buf_ator(1 ? data : data, 1 ? data + 4 : NULL, 1 ? &dummy : &dummy);
	boa_buf slice = boa_slice_buf_ator(1 ? data : data, 1 ? 4 : 0, 1 ? &dummy : &dummy);
	boa_buf bytes = boa_bytes_buf_ator(1 ? data : data, 1 ? sizeof(int) * 4 : 0, 1 ? &dummy : &dummy);
	boa_buf array = boa_array_buf_ator(data, 1 ? &dummy : &dummy);

	boa_buf zero = boa_buf_make(NULL, 0, &dummy);
	boa_buf manual = boa_buf_make(data, 4 * sizeof(int), &dummy);

	boa_assert(buf_equal(&zero, &empty));
	boa_assert(buf_equal(&manual, &range));
	boa_assert(buf_equal(&manual, &slice));
	boa_assert(buf_equal(&manual, &bytes));
	boa_assert(buf_equal(&manual, &array));
}

BOA_TEST(buf_shorthand_buf, "Shorthands for bufs")
{
	int data[4];

	boa_buf empty = boa_empty_buf();
	boa_buf range = boa_range_buf(1 ? data : data, 1 ? data + 4 : NULL);
	boa_buf slice = boa_slice_buf(1 ? data : data, 1 ? 4 : 0);
	boa_buf bytes = boa_bytes_buf(1 ? data : data, 1 ? sizeof(int) * 4 : 0);
	boa_buf array = boa_array_buf(data);

	boa_buf zero = boa_buf_make(NULL, 0, NULL);
	boa_buf manual = boa_buf_make(data, 4 * sizeof(int), NULL);

	boa_assert(buf_equal(&zero, &empty));
	boa_assert(buf_equal(&manual, &range));
	boa_assert(buf_equal(&manual, &slice));
	boa_assert(buf_equal(&manual, &bytes));
	boa_assert(buf_equal(&manual, &array));
}

BOA_TEST(buf_shorthand_view, "Shorthands for views")
{
	int data[4];

	boa_buf empty = boa_empty_view();
	boa_buf range = boa_range_view(1 ? data : data, 1 ? data + 4 : NULL);
	boa_buf slice = boa_slice_view(1 ? data : data, 1 ? 4 : 0);
	boa_buf bytes = boa_bytes_view(1 ? data : data, 1 ? sizeof(int) * 4 : 0);
	boa_buf array = boa_array_view(data);

	boa_buf zero = boa_buf_make(NULL, 0, boa_null_ator());
	boa_buf manual = boa_buf_make(data, 4 * sizeof(int), boa_null_ator());

	boa_assert(buf_equal(&zero, &empty));
	boa_assert(buf_equal(&manual, &range));
	boa_assert(buf_equal(&manual, &slice));
	boa_assert(buf_equal(&manual, &bytes));
	boa_assert(buf_equal(&manual, &array));
}

BOA_TEST(buf_shorthand_get, "Shorthand for get")
{
	boa_buf buf = boa_empty_buf();
	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 20);

	boa_assert(boa_get(int, &buf, 0) == 10);
	boa_assert(boa_get(int, &buf, 1) == 20);
	boa_assert(boa_get_n(int, &buf, 0, 2) == (int*)buf.data);

	boa_expect_assert( boa_get(int, &buf, 2) );
	boa_expect_assert( boa_get(int, &buf, 2) );
	boa_expect_assert( boa_get_n(int, &buf, 0, 3) );

	boa_reset(&buf);
}

BOA_TEST(buf_shorthand_remove, "Shorthand for remove")
{
	boa_buf buf = boa_empty_buf();
	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 20);
	boa_push_val(int, &buf, 30);

	boa_remove(int, &buf, 0);

	boa_assert(boa_get(int, &buf, 0) == 30);

	boa_reset(&buf);
}

BOA_TEST(buf_shorthand_erase, "Shorthand for erase")
{
	boa_buf buf = boa_empty_buf();
	boa_push_val(int, &buf, 10);
	boa_push_val(int, &buf, 20);
	boa_push_val(int, &buf, 30);

	boa_erase(int, &buf, 0);

	boa_assert(boa_get(int, &buf, 0) == 20);

	boa_erase_n(int, &buf, 0, 2);

	boa_assert(buf.end_pos == 0);

	boa_reset(&buf);
}

