
#include <boa_test.h>

BOA_TEST(expect_assert, "Expect assert should allow assert inside it")
{
	boa_expect_assert( boa_assert(1 > 2) );
}

BOA_TEST(expect_assert_fail, "Expect assert without assertion")
{
	int x;
	boa_expect_assert( boa_expect_assert( x = 5 ) );
}

BOA_TEST(alloc_fail, "boa_alloc() should fail in tests when requested")
{
	void *allocs[2];

	boa_test_fail_next_allocation();

	allocs[0] = boa_alloc(16);
	allocs[1] = boa_alloc(16);
	boa_assert(allocs[0] == NULL);
	boa_assert(allocs[1] != NULL);

	boa_free(allocs[1]);
}

BOA_TEST(alloc_fail_complex, "boa_alloc() should fail with complex pattern")
{
	void *allocs[4];

	boa_test_fail_allocations(1, 2);

	allocs[0] = boa_alloc(16);
	allocs[1] = boa_alloc(16);
	allocs[2] = boa_alloc(16);
	allocs[3] = boa_alloc(16);
	boa_assert(allocs[0] != NULL);
	boa_assert(allocs[1] == NULL);
	boa_assert(allocs[2] == NULL);
	boa_assert(allocs[3] != NULL);

	boa_free(allocs[0]);
	boa_free(allocs[3]);
}

BOA_TEST(realloc_fail, "boa_realloc() should fail in tests when requested")
{
	void *allocs[2], *reallocs[2];

	allocs[0] = boa_alloc(16);
	allocs[1] = boa_alloc(16);
	boa_assert(allocs[0] != NULL);
	boa_assert(allocs[1] != NULL);

	boa_test_fail_next_allocation();

	reallocs[0] = boa_realloc(allocs[0], 64);
	reallocs[1] = boa_realloc(allocs[1], 64);
	boa_assert(reallocs[0] == NULL);
	boa_assert(reallocs[1] != NULL);

	boa_free(allocs[0]);
	boa_free(reallocs[1]);
}

BOA_TEST(realloc_fail_complex, "boa_realloc() should fail with complex pattern")
{
	void *allocs[4], *reallocs[4];
	allocs[0] = boa_alloc(16);
	allocs[1] = boa_alloc(16);
	allocs[2] = boa_alloc(16);
	allocs[3] = boa_alloc(16);

	boa_test_fail_allocations(1, 2);

	reallocs[0] = boa_realloc(allocs[0], 64);
	reallocs[1] = boa_realloc(allocs[1], 64);
	reallocs[2] = boa_realloc(allocs[2], 64);
	reallocs[3] = boa_realloc(allocs[3], 64);
	boa_assert(reallocs[0] != NULL);
	boa_assert(reallocs[1] == NULL);
	boa_assert(reallocs[2] == NULL);
	boa_assert(reallocs[3] != NULL);

	boa_free(reallocs[0]);
	boa_free(allocs[1]);
	boa_free(allocs[2]);
	boa_free(reallocs[3]);
}

BOA_TEST(memory_leak, "Tests should catch memory leaks")
{
	boa_test_expect_fail();
	boa_alloc(16);
}

BOA_TEST(memory_leak_realloc, "Tests should catch memory leaks after realloc")
{
	boa_test_expect_fail();
	void *ptr = boa_alloc(16);
	ptr = boa_realloc(ptr, 128);
}

BOA_TEST(allocator, "Test allocator should count allocations")
{
	boa_test_allocator ator = boa_test_allocator_make();

	boa_assert(ator.allocs == 0);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 0);

	void *ptr = boa_alloc_ator(&ator.ator, 64);
	boa_assert(ptr != NULL);
	ptr = boa_realloc_ator(&ator.ator, ptr, 64);
	boa_assert(ptr != NULL);
	boa_free_ator(&ator.ator, ptr);

	boa_free_ator(&ator.ator, boa_alloc_ator(&ator.ator, 64));

	boa_assert(ator.allocs == 2);
	boa_assert(ator.reallocs == 1);
	boa_assert(ator.frees == 2);
}

BOA_TEST(allocator_fail, "Test allocator count failed allocations")
{
	boa_test_allocator ator = boa_test_allocator_make();

	boa_assert(ator.allocs == 0);
	boa_assert(ator.reallocs == 0);
	boa_assert(ator.frees == 0);

	boa_test_fail_next_allocation();
	boa_assert(boa_alloc_ator(&ator.ator, 64) == NULL);

	void *ptr = boa_alloc_ator(&ator.ator, 64);
	boa_assert(ptr != NULL);

	boa_test_fail_next_allocation();
	void *re_ptr = boa_realloc_ator(&ator.ator, ptr, 64);
	boa_assert(re_ptr == NULL);

	boa_free_ator(&ator.ator, ptr);

	boa_assert(ator.allocs == 2);
	boa_assert(ator.reallocs == 1);
	boa_assert(ator.frees == 1);
}


