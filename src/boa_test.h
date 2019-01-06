
#ifdef BOA_TEST
	#undef BOA_TEST
	#undef BOA_TEST_BEGIN_PERMUTATION
	#undef BOA_TEST_END_PERMUTATION
#endif

#if BOA_TEST_MODE == BOA_TEST_IMPLEMENT
	#define BOA_TEST(name, desc) void boa_test__##name()
	#define BOA_TEST_BEGIN_PERMUTATION(name, values, format)
	#define BOA_TEST_END_PERMUTATION(name)
#elif BOA_TEST_MODE == BOA_TEST_GATHER
	#define BOA_TEST(name, desc) boa_test_add(&boa_test__##name, #name, desc, __FILE__, __LINE__); if (0)
	#define BOA_TEST_BEGIN_PERMUTATION(name, values, format) boa_test_set_permutation(&(name), #name, (values), sizeof(values) / sizeof(*(values)), sizeof(*(values)), format);
	#define BOA_TEST_END_PERMUTATION(name) boa_test_set_permutation(&name, #name, 0, 0, 0, 0);
#elif defined(BOA_TEST_MODE)
	#error "Invalid BOA_TEST_MODE"
#endif

#ifndef BOA__TEST_INCLUDED
#define BOA__TEST_INCLUDED

#if defined(BOA__CORE_INCLUDED)
	#error "boa_test.h needs to be included before boa_core.h"
#endif

#include <setjmp.h>

#define boa_assert(cond) do { if (!(cond)) boa__test_do_fail(__FILE__, __LINE__, #cond, 0); } while (0)
#define boa_assertf(cond, ...) do { if (!(cond)) { \
		boa_buf fmtbuf = boa_empty_buf_ator(boa_test_original_ator()); \
		boa__test_do_fail(__FILE__, __LINE__, #cond, boa_format(&fmtbuf, __VA_ARGS__)); \
	} } while (0)
#define boa_assert_ok(cond) do { boa_result boa__assert_res = (cond); if (boa__assert_res != boa_ok) { \
		boa_buf fmtbuf = boa_empty_buf_ator(boa_test_original_ator()); \
		boa__test_do_fail(__FILE__, __LINE__, #cond, boa_format(&fmtbuf, "Result was not OK: %s", boa__assert_res->description)); \
	} } while (0)

#define boa_expect_assert(expr) do { \
	extern jmp_buf *boa__test_expect_assert_jmp; \
	jmp_buf boa__expect_assert_local_jmp_buf, *prev = boa__test_expect_assert_jmp; \
	int boa__expect_assert_fail = 1; \
	boa__test_expect_assert_jmp = &boa__expect_assert_local_jmp_buf; \
	if (!setjmp(boa__expect_assert_local_jmp_buf)) { expr; } \
	else boa__expect_assert_fail = 0; \
	boa__test_expect_assert_jmp = prev; \
	if (boa__expect_assert_fail) boa__test_do_fail(__FILE__, __LINE__, "boa_expect_assert()", 0); \
	} while (0)

#define BOA_TEST_IMPLEMENT 0x1000
#define BOA_TEST_GATHER 0x2000

#define BOA_TEST_IMPL (BOA_TEST_MODE == BOA_TEST_IMPLEMENT)

#define BOA_TEST_BEGIN_PERMUTATION_U32(name, values) BOA_TEST_BEGIN_PERMUTATION(name, values, &boa_format_u32)

void boa__test_do_fail(const char *file, int line, const char *expr, char *desc);

#include "boa_core.h"

typedef struct boa_test {
	void (*test_fn)();
	const char *name, *description, *file;
	int line;

	boa_buf permutations;
} boa_test;

typedef struct boa_test_permutation {
	void *ptr;
	const char *name;
	const void *values;
	uint32_t num, value_size, index;
	boa_format_fn format_fn;
} boa_test_permutation;

typedef struct boa_test_hint {
	const char *name;
	void *value;
	size_t size;
	boa_format_fn format_fn;
} boa_test_hint;

typedef struct boa_test_fail {
	const char *file;
	int line;
	const char *expression;
	char *description;
	int permutation_index;
	boa_buf hints;
} boa_test_fail;

void boa_test_add(void (*test_fn)(), const char *name, const char *desc, const char *file, int line);
void boa_test_set_permutation(void *ptr, const char *name, const void *values, size_t num, size_t value_size, boa_format_fn format);

boa_test *boa_test_get_all(size_t *count);
int boa_test_run(boa_test *test, boa_test_fail *fail, int permutation_filter);

void boa_test_expect_fail();

void boa_test_fail_allocations(int delay, int count);
#define boa_test_fail_next_allocation() boa_test_fail_allocations(0, 1)

void boa_test_add_hint(const char *name, const void *value, size_t size, boa_format_fn format);
#define boa_test_hint_u32(name) boa_test_add_hint(#name, &(name), sizeof(uint32_t), &boa_format_u32);

boa_inline boa_allocator *boa_test_original_ator()
{
	extern boa_allocator boa__test_original_ator;
	return &boa__test_original_ator;
}

typedef struct boa_test_allocator {
	boa_allocator ator;
	uint32_t allocs, reallocs, frees;
} boa_test_allocator;

boa_test_allocator boa_test_allocator_make();

#endif

