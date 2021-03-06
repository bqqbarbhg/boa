
#include <boa_test_impl.h>
#include <boa_core_impl.h>
#include <boa_os_impl.h>
#include <boa_unicode_impl.h>

#include <stdio.h>
#include <string.h>

#include <boa_test.h>

#define BOA_TEST_MODE BOA_TEST_IMPLEMENT
#include "test_all.h"
#undef BOA_TEST_MODE

void gather_tests()
{
#define BOA_TEST_MODE BOA_TEST_GATHER
#include "test_all.h"
#undef BOA_TEST_MODE
}

int main(int argc, char **argv)
{
	int num_pass = 0;
	size_t i, num;
	boa_test *tests;

	const char *test_filter = NULL;
	int permutation_filter = -1;

	if (argc > 1) {
		test_filter = argv[1];
	}

	if (argc > 2) {
		permutation_filter = atoi(argv[2]);
	}
	
	gather_tests();

	boa_allocator *original = boa_test_original_ator();

	char fmtbuf_data[128];
	boa_buf fmtbuf = boa_array_buf_ator(fmtbuf_data, original);

	boa_test_fail fail = { 0 };

	int num_ran = 0;
	tests = boa_test_get_all(&num);
	for (i = 0; i < num; i++) {
		boa_test *test = &tests[i];

		if (test_filter) {
			if (strcmp(test->name, test_filter)) continue;
		}
		num_ran++;

		printf("%s %s: ", boa_format(boa_clear(&fmtbuf), "[%s]", test->name), test->description);
		fflush(stdout);
		int status = boa_test_run(test, &fail, permutation_filter);
		if (status) {
			num_pass++;
			if (boa_non_empty(&test->permutations)) {
				uint32_t count = 1;
				boa_for (boa_test_permutation, p, &test->permutations) {
					count *= p->num;
				}
				printf("OK x%u\n", count);
			} else {
				printf("OK\n");
			}
		} else {
			const char *slash = strrchr(fail.file, '/');
			const char *backslash = strrchr(fail.file, '\\');
			const char *file = fail.file;
			printf("FAIL\n");
			if (slash != NULL && slash > file) file = slash + 1;
			if (backslash != NULL && backslash > file) file = backslash + 1;
			if (fail.description) {
				printf("    %s\n", fail.description);
				boa_free_ator(original, fail.description);
			}
			printf("    %s:%d: Assertion failed: %s\n", file, fail.line, fail.expression);

			if (boa_non_empty(&test->permutations)) {
				printf("    Permutation index %d\n", fail.permutation_index);
			}

			boa_for (boa_test_permutation, p, &test->permutations) {
				const void *value = (const char*)p->values + p->index * p->value_size;
				if (p->format_fn(boa_clear(&fmtbuf), value, p->value_size)) {
					printf("    ... %s = %s (index %u)\n", p->name, boa_begin(char, &fmtbuf), p->index);
				}
			}

			boa_for (boa_test_hint, h, &fail.hints) {
				if (h->format_fn(boa_clear(&fmtbuf), h->value, h->size)) {
					printf("    ... %s = %s\n", h->name, boa_begin(char, &fmtbuf));
				}
			}

			printf("\n");
		}
	}

	boa_reset(&fail.hints);
	boa_reset(&fmtbuf);

	printf("\n");
	printf("Tests passed: %d / %d\n", num_pass, num_ran);

	return num_pass == num_ran ? 0 : 1;
}
