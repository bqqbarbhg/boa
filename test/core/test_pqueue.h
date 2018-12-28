
#include <boa_test.h>
#include <boa_core.h>

#if BOA_TEST_IMPL

int int_before(const void *a, const void *b, void *user) { return *(int*)a < *(int*)b; }

int enqueue_int(boa_buf *buf, int value)
{
	return boa_pqueue_enqueue_inline(buf, &value, sizeof(int), &int_before, NULL);
}

int dequeue_int(boa_buf *buf)
{
	int result;
	boa_pqueue_dequeue_inline(buf, &result, sizeof(int), &int_before, NULL);
	return result;
}

#endif

BOA_TEST(pqueue_ints, "Simple manual integer priority queue test")
{
	boa_buf buf = boa_empty_buf();

	boa_assert(enqueue_int(&buf, 5) != 0);
	boa_assert(enqueue_int(&buf, 1) != 0);
	boa_assert(enqueue_int(&buf, 3) != 0);
	boa_assert(enqueue_int(&buf, 2) != 0);
	boa_assert(enqueue_int(&buf, 4) != 0);
	boa_assert(enqueue_int(&buf, 3) != 0);

	boa_assert(boa_count(int, &buf) == 6);

	boa_assert(dequeue_int(&buf) == 1);
	boa_assert(dequeue_int(&buf) == 2);
	boa_assert(dequeue_int(&buf) == 3);
	boa_assert(dequeue_int(&buf) == 3);
	boa_assert(dequeue_int(&buf) == 4);
	boa_assert(dequeue_int(&buf) == 5);

	boa_assert(boa_count(int, &buf) == 0);

	boa_reset(&buf);
}

BOA_TEST(pqueue_reverse_ints, "Priority queue with integers inserted in reverse order")
{
	boa_buf buf = boa_empty_buf();

	int count = 1000;
	for (int i = count - 1; i >= 0; i--) {
		boa_test_hint_u32(i);
		boa_assert(enqueue_int(&buf, i) != 0);
	}

	boa_assert(boa_count(int, &buf) == count);

	for (int i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assert(dequeue_int(&buf) == i);
	}

	boa_assert(boa_count(int, &buf) == 0);

	boa_reset(&buf);
}

BOA_TEST(pqueue_reverse_correct_ints, "Priority queue with integers inserted in reverse and correct order")
{
	boa_buf buf = boa_empty_buf();

	int count = 1000;
	for (int i = count - 1; i >= 0; i--) {
		boa_test_hint_u32(i);
		boa_assert(enqueue_int(&buf, i) != 0);
	}
	for (int i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assert(enqueue_int(&buf, i) != 0);
	}

	boa_assert(boa_count(int, &buf) == 2 * count);

	for (int i = 0; i < count; i++) {
		boa_test_hint_u32(i);
		boa_assert(dequeue_int(&buf) == i);
		boa_assert(dequeue_int(&buf) == i);
	}

	boa_assert(boa_count(int, &buf) == 0);

	boa_reset(&buf);
}

BOA_TEST(pqueue_out_of_space, "Priority queue should handle out of space gracefully")
{
	int arr[3];
	boa_buf buf = boa_array_view(arr);

	boa_assert(enqueue_int(&buf, 5) != 0);
	boa_assert(enqueue_int(&buf, 1) != 0);
	boa_assert(enqueue_int(&buf, 3) != 0);
	boa_assert(enqueue_int(&buf, 2) == 0);

	boa_assert(boa_count(int, &buf) == 3);

	boa_assert(dequeue_int(&buf) == 1);
	boa_assert(boa_count(int, &buf) == 2);

	boa_assert(enqueue_int(&buf, 2) != 0);
	boa_assert(enqueue_int(&buf, 4) == 0);

	boa_assert(boa_count(int, &buf) == 3);

	boa_assert(dequeue_int(&buf) == 2);

	boa_reset(&buf);
}

BOA_TEST(pqueue_empty_assert, "Priority queue should assert if empty")
{
	boa_buf buf = boa_empty_buf();

	boa_expect_assert( dequeue_int(&buf) );

	boa_assert(enqueue_int(&buf, 5) != 0);
	boa_assert(dequeue_int(&buf) == 5);
	boa_expect_assert( dequeue_int(&buf) );

	boa_reset(&buf);
}
