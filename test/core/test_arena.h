
#include <boa_test.h>
#include <boa_core.h>

BOA_TEST(arena_simple, "Push some values to an arena")
{
	boa_arena arena = { 0 };

	int *a = boa_arena_push(int, &arena);
	int *b = boa_arena_push(int, &arena);
	boa_assert(a != b);
	boa_assert(a != NULL);
	boa_assert(b != NULL);

	boa_arena_reset(&arena);
}

BOA_TEST(arena_simple_init, "Push some values to an arena initialized with init function")
{
	boa_arena arena;
	boa_arena_init(&arena);

	int *a = boa_arena_push(int, &arena);
	int *b = boa_arena_push(int, &arena);
	boa_assert(a != b);
	boa_assert(a != NULL);
	boa_assert(b != NULL);

	boa_arena_reset(&arena);
}

BOA_TEST(arena_many_pages, "Arena with more than one page")
{
	boa_arena arena = { 0 };
	boa_buf pointers = boa_empty_buf();

	for (uint32_t i = 0; i < 10000; i++) {
		uint32_t *ptr = boa_arena_push(uint32_t, &arena);
		boa_assert(ptr != NULL);
		*ptr = i;
		boa_push_data(&pointers, &ptr);
	}

	for (uint32_t i = 0; i < 10000; i++) {
		uint32_t *ptr = boa_get(uint32_t*, &pointers, i);
		boa_assert(*ptr == i);
	}

	boa_arena_reset(&arena);
	boa_reset(&pointers);
}

BOA_TEST(arena_multiple_allocators, "Using different allocators during the lifetime of an arena")
{
	boa_arena arena;
	boa_buf pointers = boa_empty_buf();

	boa_test_allocator a = boa_test_allocator_make();
	boa_test_allocator b = boa_test_allocator_make();

	boa_arena_init_ator(&arena, &a.ator);

	int *value = boa_arena_push(int, &arena);
	boa_assert(value != NULL);

	arena.ator = &b.ator;

	for (uint32_t i = 0; i < 10000; i++) {
		uint32_t *ptr = boa_arena_push(uint32_t, &arena);
		boa_assert(ptr != NULL);
		*ptr = i;
		boa_push_data(&pointers, &ptr);
	}

	for (uint32_t i = 0; i < 10000; i++) {
		uint32_t *ptr = boa_get(uint32_t*, &pointers, i);
		boa_assert(*ptr == i);
	}

	boa_arena_reset(&arena);
	boa_reset(&pointers);

	boa_assert(a.allocs == 1);
	boa_assert(a.frees == 1);
	boa_assert(b.allocs >= 1);
	boa_assert(b.frees == b.allocs);
}

BOA_TEST(arena_reset_empty, "Reset an empty arena")
{
	boa_arena arena = { 0 };
	boa_arena_reset(&arena);
}

BOA_TEST(arena_reset_empty_init, "Reset an empty arena initialized with init function")
{
	boa_arena arena;
	boa_arena_init(&arena);
	boa_arena_reset(&arena);
}

BOA_TEST(arena_alloc_fail, "Arena should handle allocation failure gracefully")
{
	boa_arena arena = { 0 };

	int *ptr = boa_arena_push(int, &arena);
	boa_assert(ptr != NULL);

	boa_test_fail_next_allocation();

	int failed = 0;
	for (uint32_t i = 0; i < 10000; i++) {
		uint32_t *ptr = boa_arena_push(uint32_t, &arena);

		if (ptr == NULL) {
			failed = 1;
			break;
		}
	}

	boa_assert(failed != 0);
	
	boa_arena_reset(&arena);
}

BOA_TEST(arena_align, "Arena should respect type alignment")
{
	boa_arena arena = { 0 };

	boa_arena_push(uint8_t, &arena);
	uintptr_t a16 = (uintptr_t)boa_arena_push(uint16_t, &arena);
	uintptr_t b16 = (uintptr_t)boa_arena_push(uint16_t, &arena);
	uintptr_t a32 = (uintptr_t)boa_arena_push(uint32_t, &arena);
	boa_arena_push(uint8_t, &arena);
	uintptr_t c16 = (uintptr_t)boa_arena_push(uint16_t, &arena);
	boa_arena_push(uint8_t, &arena);
	uintptr_t b32 = (uintptr_t)boa_arena_push(uint32_t, &arena);
	uintptr_t a64 = (uintptr_t)boa_arena_push(uint64_t, &arena);
	boa_arena_push(uint8_t, &arena);
	uintptr_t b64 = (uintptr_t)boa_arena_push(uint64_t, &arena);

	boa_assert(a16 % 2 == 0);
	boa_assert(b16 % 2 == 0);
	boa_assert(c16 % 2 == 0);
	boa_assert(a32 % 4 == 0);
	boa_assert(b32 % 4 == 0);
	boa_assert(a64 % 8 == 0);
	boa_assert(b64 % 8 == 0);

	boa_arena_reset(&arena);
}
