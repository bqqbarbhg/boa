
#ifdef BOA_TEST
	#undef BOA_TEST
#endif

#if BOA_TEST_MODE == BOA_TEST_IMPLEMENT
	#define BOA_TEST(name, desc) void boa_test__##name()
#elif BOA_TEST_MODE == BOA_TEST_GATHER
	#define BOA_TEST(name, desc) boa_test_add(&boa_test__##name, #name, desc); if (0)
#elif defined(BOA_TEST_MODE)
	#error "Invalid BOA_TEST_MODE"
#endif

#ifndef BOA__TEST_INCLUDED
#define BOA__TEST_INCLUDED

#define BOA_TEST_IMPLEMENT 0x1000
#define BOA_TEST_GATHER 0x2000

#define BOA_TEST_IMPL (BOA_TEST_MODE == BOA_TEST_IMPLEMENT)

typedef struct boa_test {
	void (*test_fn)();
	const char *name, *description;
} boa_test;

typedef struct boa_test_fail {
	const char *file;
	int line;
	const char *expression;
	char *description;
} boa_test_fail;

void boa_test_add(void (*test_fn)(), const char *name, const char *desc);
const boa_test *boa_test_get_all(size_t *count);
int boa_test_run(const boa_test *test, boa_test_fail *fail);

void boa_test_fail_allocations(int delay, int count);
#define boa_test_fail_next_allocation() boa_test_fail_allocations(0, 1)

void boa__test_do_fail(const char *file, int line, const char *expr, char *desc);
void boa__test_do_failf(const char *file, int line, const char *expr, ...);
#define boa_expect(cond) do { if (!(cond)) boa__test_do_fail(__FILE__, __LINE__, #cond, 0); } while (0)
#define boa_expectf(cond, ...) do { if (!(cond)) boa__test_do_fail(__FILE__, __LINE__, #cond, boa_format(0, __VA_ARGS__)); } while (0)

#endif

#if defined(BOA_IMPLEMENTATION)
#ifndef BOA__TEST_IMPLEMENTED
#define BOA__TEST_IMPLEMENTED

#include <stdlib.h>

#if defined(BOA_USE_TEST_ALLOCATORS)

#define BOA__HAS_TEST_ALLOCATORS

static inline void *boa__original_malloc(size_t size)
{
#ifdef BOA_MALLOC
	return BOA_MALLOC(size);
	#undef BOA_MALLOC
#else
	return malloc(size);
#endif
}

static inline void *boa__original_realloc(void *ptr, size_t size)
{
#ifdef BOA_REALLOC
	return BOA_REALLOC(ptr, size);
	#undef BOA_REALLOC
#else
	return realloc(ptr, size);
#endif
}

static inline void boa__original_free(void *ptr)
{
#ifdef BOA_FREE
	return BOA_FREE(ptr);
	#undef BOA_FREE
#else
	free(ptr);
#endif
}

void *boa_test_malloc(size_t size);
void *boa_test_realloc(void *ptr, size_t size);
void boa_test_free(void *ptr);

#define BOA_MALLOC boa_test_malloc
#define BOA_REALLOC boa_test_realloc
#define BOA_FREE boa_test_free

#endif // BOA_USE_TEST_ALLOCATORS

#include "boa_core.h"
#include <setjmp.h>

static jmp_buf boa__test_jmp;
static boa_test_fail boa__current_fail;

static boa_vec boa__all_tests;

static int boa__test_alloc_fail_delay = 0;
static int boa__test_alloc_fail_count = 0;

void boa_test_add(void (*test_fn)(), const char *name, const char *desc)
{
	boa_test *test = boa_vpush(boa_test, boa__all_tests);
	if (test) {
		test->test_fn = test_fn;
		test->name = name;
		test->description = desc;
	}
}

const boa_test *boa_test_get_all(size_t *count)
{
	*count = boa_vcount(boa_test, boa__all_tests);
	return boa_vbegin(boa_test, boa__all_tests);
}

int boa_test_run(const boa_test *test, boa_test_fail *fail)
{
	int status = 0;
	if (!setjmp(boa__test_jmp)) {
		test->test_fn();
		status = 1;
	} else {
		if (fail) *fail = boa__current_fail;
	}

	boa__test_alloc_fail_delay = 0;
	boa__test_alloc_fail_count = 0;

	return status;
}

void boa_test_fail_allocations(int delay, int count)
{
	boa__test_alloc_fail_delay = delay;
	boa__test_alloc_fail_count = count;
}

void boa__test_do_fail(const char *file, int line, const char *expr, char *desc)
{
	boa__current_fail.file = file;
	boa__current_fail.line = line;
	boa__current_fail.expression = expr;
	boa__current_fail.description = desc;
	longjmp(boa__test_jmp, 1);
}

static int boa__test_alloc_should_fail()
{
	if (boa__test_alloc_fail_delay > 0)
		boa__test_alloc_fail_delay--;
	else if (boa__test_alloc_fail_delay == 0 && boa__test_alloc_fail_count > 0) {
		boa__test_alloc_fail_count--;
		return 1;
	}
	return 0;
}

void *boa_test_malloc(size_t size)
{
	if (boa__test_alloc_should_fail()) return NULL;
	void *ptr = boa__original_malloc(size);
	memset(ptr, 0xFE, size);
	return ptr;
}

void *boa_test_realloc(void *ptr, size_t size)
{
	if (boa__test_alloc_should_fail()) return NULL;
	return boa__original_realloc(ptr, size);
}

void boa_test_free(void *ptr)
{
	boa__original_free(ptr);
}

#endif
#endif

