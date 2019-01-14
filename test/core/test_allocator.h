
#include <boa_test.h>
#include <boa_core.h>

BOA_TEST(default_alloc, "boa_alloc()")
{
	int *ptr = (int*)boa_alloc(sizeof(int) * 4);
	boa_assert(ptr != NULL);
	ptr[0] = 1; ptr[1] = 2; ptr[2] = 3; ptr[3] = 4;
	boa_free(ptr);
}

BOA_TEST(default_realloc, "boa_realloc()")
{
	int *ptr = (int*)boa_alloc(sizeof(int) * 2);
	boa_assert(ptr != NULL);
	ptr[0] = 1; ptr[1] = 2;
	ptr = (int*)boa_realloc(ptr, sizeof(int) * 4);
	boa_assert(ptr[0] == 1);
	boa_assert(ptr[1] == 2);
	ptr[2] = 3; ptr[3] = 4;
	boa_free(ptr);
}

BOA_TEST(default_alloc_align, "boa_alloc() should be always aligned to 8 bytes")
{
	void *pointers[64];

	for (uint32_t i = 0; i < boa_arraycount(pointers); i++) {
		void *ptr = boa_alloc(1);
		boa_assert((uintptr_t)ptr % 8 == 0);
		pointers[i] = ptr;
	}

	for (uint32_t i = 0; i < boa_arraycount(pointers); i++) {
		boa_free(pointers[i]);
	}
}

BOA_TEST(null_allocator, "Null allocator should return NULL for alloc")
{
	boa_allocator *ator = boa_null_ator();
	boa_assert(boa_alloc_ator(ator, 16) == NULL);
}

BOA_TEST(heap_allocator, "boa_heap_ator() should be compatible with allocators")
{
	boa_allocator *ator = boa_heap_ator();
	void *free_ptr = boa_alloc(32);
	void *ator_ptr = boa_alloc_ator(ator, 32);
	boa_free(ator_ptr);
	boa_free_ator(ator, free_ptr);
}

BOA_TEST(free_zero, "free(NULL) should be a no-op")
{
	boa_free(NULL);
}

