#pragma once

#ifndef BOA__TEST_IMPLEMENTED
#define BOA__TEST_IMPLEMENTED

#if defined(BOA__CORE_IMPLEMENTED)
	#error "boa_test_impl.h needs to be included before boa_core_impl.h"
#endif

#include "boa_test.h"
#include "boa_core.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

static inline void *boa__original_alloc(size_t size)
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

static void *boa__test_original_alloc(boa_allocator *ator, size_t size) { return boa__original_alloc(size); }
static void *boa__test_original_realloc(boa_allocator *ator, void *ptr, size_t size) { return boa__original_realloc(ptr, size); }
static void boa__test_original_free(boa_allocator *ator, void *ptr) { boa__original_free(ptr); }

boa_allocator boa__test_original_ator = {
	boa__test_original_alloc,
	boa__test_original_realloc,
	boa__test_original_free,
};

void *boa_test_alloc(size_t size);
void *boa_test_realloc(void *ptr, size_t size);
void boa_test_free(void *ptr);

#define BOA_MALLOC boa_test_alloc
#define BOA_REALLOC boa_test_realloc
#define BOA_FREE boa_test_free

static jmp_buf boa__test_jmp;
static boa_test_fail boa__current_fail;

static boa_buf boa__all_tests;

static int boa__test_alloc_fail_delay = 0;
static int boa__test_alloc_fail_count = 0;

static int boa__test_expect_fail_current = 0;

jmp_buf *boa__test_expect_assert_jmp;

typedef struct boa__test_alloc_hdr {
	struct boa__test_alloc_hdr *next, *prev;
	size_t size;
} boa__test_alloc_hdr;

boa__test_alloc_hdr boa__test_alloc_head;

void boa_test_add(void (*test_fn)(), const char *name, const char *desc, const char *file, int line)
{
	boa_buf_set_ator(&boa__all_tests, boa_test_original_ator());
	boa_test *test = boa_push(boa_test, &boa__all_tests);
	if (test) {
		test->test_fn = test_fn;
		test->name = name;
		test->description = desc;
		test->file = file;
		test->line = line;
	}
}

const boa_test *boa_test_get_all(size_t *count)
{
	*count = boa_count(boa_test, &boa__all_tests);
	return boa_begin(boa_test, &boa__all_tests);
}

int boa_test_run(const boa_test *test, boa_test_fail *fail)
{
	int status = 0;
	if (!setjmp(boa__test_jmp)) {
		test->test_fn();
		status = 1;
	} else {
		*fail = boa__current_fail;
	}

	boa__test_alloc_hdr *hdr = boa__test_alloc_head.next;
	if (hdr) {
		size_t allocs = 0, total_size = 0;

		while (hdr) {
			void *to_free = hdr;
			total_size += hdr->size;
			allocs++;
			hdr = hdr->next;
			boa__original_free(to_free);
		}

		boa__test_alloc_head.next = NULL;

		if (status) {
			boa_buf fmtbuf = boa_empty_buf_ator(boa_test_original_ator());
			fail->file = test->file;
			fail->line = test->line;
			fail->expression = "Memory leak";
			fail->description = boa_format(&fmtbuf, "Leaked %u allocations, %u bytes",
					(unsigned)allocs, (unsigned)total_size);

			status = 0;
		}
	}

	if (boa__test_expect_fail_current) {
		boa__test_expect_fail_current = 0;

		if (status) {
			fail->file = test->file;
			fail->line = test->line;
			fail->expression = "Expected test to fail";
			fail->description = NULL;
			status = 0;
		} else {
			status = 1;
		}
	}

	boa__test_alloc_fail_delay = 0;
	boa__test_alloc_fail_count = 0;

	return status;
}

void boa_test_expect_fail()
{
	boa__test_expect_fail_current = 1;
}

void boa_test_fail_allocations(int delay, int count)
{
	boa__test_alloc_fail_delay = delay;
	boa__test_alloc_fail_count = count;
}

void boa__test_do_fail(const char *file, int line, const char *expr, char *desc)
{
	if (boa__test_expect_assert_jmp) {
		longjmp(*boa__test_expect_assert_jmp, 1);
	} else {
		boa__current_fail.file = file;
		boa__current_fail.line = line;
		boa__current_fail.expression = expr;
		boa__current_fail.description = desc;
		longjmp(boa__test_jmp, 1);
	}
}

void *boa_test_alloc(size_t size)
{
	if (boa__test_alloc_fail_delay > 0)
		boa__test_alloc_fail_delay--;
	else if (boa__test_alloc_fail_delay == 0 && boa__test_alloc_fail_count > 0) {
		boa__test_alloc_fail_count--;
		return NULL;
	}

	void *ptr = boa__original_alloc(size + sizeof(boa__test_alloc_hdr));
	boa__test_alloc_hdr *hdr = (boa__test_alloc_hdr*)ptr;

	hdr->prev = &boa__test_alloc_head;
	hdr->next = boa__test_alloc_head.next;
	if (hdr->next) hdr->next->prev = hdr;
	boa__test_alloc_head.next = hdr;
	hdr->size = size;

	ptr = hdr + 1;
	memset(ptr, 0xAC, size);
	return ptr;
}

void *boa_test_realloc(void *ptr, size_t size)
{
	boa__test_alloc_hdr *hdr = (boa__test_alloc_hdr*)ptr - 1;
	void *new_ptr = boa_test_alloc(size);

	if (new_ptr) {
		size_t copy_size = hdr->size < size ? hdr->size : size;
		memcpy(new_ptr, ptr, copy_size);
		boa_test_free(ptr);
	}

	return new_ptr;
}

void boa_test_free(void *ptr)
{
	if (!ptr) return;
	boa__test_alloc_hdr *hdr = (boa__test_alloc_hdr*)ptr - 1;

	if (hdr->next) hdr->next->prev = hdr->prev;
	if (hdr->prev) hdr->prev->next = hdr->next;

	memset(ptr, 0xFE, hdr->size);

	boa__original_free(hdr);
}

void *boa__test_ator_alloc(boa_allocator *ator, size_t size)
{
	boa_test_allocator *ta = (boa_test_allocator*)ator;
	ta->allocs++;
	return boa_alloc(size);
}

void *boa__test_ator_realloc(boa_allocator *ator, void *ptr, size_t size)
{
	boa_test_allocator *ta = (boa_test_allocator*)ator;
	ta->reallocs++;
	return boa_realloc(ptr, size);
}

void boa__test_ator_free(boa_allocator *ator, void *ptr)
{
	boa_test_allocator *ta = (boa_test_allocator*)ator;
	ta->frees++;
	boa_free(ptr);
}

boa_test_allocator boa_test_allocator_make() 
{
	boa_test_allocator ator;
	ator.ator.alloc_fn = &boa__test_ator_alloc;
	ator.ator.realloc_fn = &boa__test_ator_realloc;
	ator.ator.free_fn = &boa__test_ator_free;
	ator.allocs = ator.reallocs = ator.frees = 0;
	return ator;
}

#endif

