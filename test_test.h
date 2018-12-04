
#include "boa_test.h"

BOA_TEST(test_malloc_fail, "boa_malloc() should fail in tests when requested")
{
	void *allocs[2];

	boa_test_fail_next_allocation();

	allocs[0] = boa_malloc(16);
	allocs[1] = boa_malloc(16);
	boa_expect(allocs[0] == NULL);
	boa_expect(allocs[1] != NULL);

	boa_free(allocs[1]);
}

BOA_TEST(test_malloc_fail_complex, "boa_malloc() should fail with complex pattern")
{
	void *allocs[4];

	boa_test_fail_allocations(1, 2);

	allocs[0] = boa_malloc(16);
	allocs[1] = boa_malloc(16);
	allocs[2] = boa_malloc(16);
	allocs[3] = boa_malloc(16);
	boa_expect(allocs[0] != NULL);
	boa_expect(allocs[1] == NULL);
	boa_expect(allocs[2] == NULL);
	boa_expect(allocs[3] != NULL);

	boa_free(allocs[0]);
	boa_free(allocs[3]);
}

BOA_TEST(test_realloc_fail, "boa_realloc() should fail in tests when requested")
{
	void *allocs[2];

	allocs[0] = boa_malloc(16);
	allocs[1] = boa_malloc(16);
	boa_expect(allocs[0] != NULL);
	boa_expect(allocs[1] != NULL);

	boa_test_fail_next_allocation();

	allocs[0] = boa_realloc(allocs[0], 64);
	allocs[1] = boa_realloc(allocs[1], 64);
	boa_expect(allocs[0] == NULL);
	boa_expect(allocs[1] != NULL);

	boa_free(allocs[1]);
}

BOA_TEST(test_realloc_fail_complex, "boa_realloc() should fail with complex pattern")
{
	void *allocs[4];
	allocs[0] = boa_malloc(16);
	allocs[1] = boa_malloc(16);
	allocs[2] = boa_malloc(16);
	allocs[3] = boa_malloc(16);

	boa_test_fail_allocations(1, 2);

	allocs[0] = boa_realloc(allocs[0], 64);
	allocs[1] = boa_realloc(allocs[1], 64);
	allocs[2] = boa_realloc(allocs[2], 64);
	allocs[3] = boa_realloc(allocs[3], 64);
	boa_expect(allocs[0] != NULL);
	boa_expect(allocs[1] == NULL);
	boa_expect(allocs[2] == NULL);
	boa_expect(allocs[3] != NULL);

	boa_free(allocs[0]);
	boa_free(allocs[3]);
}

