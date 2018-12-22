
#include <boa_test_impl.h>
#include <boa_core_impl.h>

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
	
	gather_tests();

	boa_allocator *original = boa_test_original_ator();

	char namebuf_data[128];
	boa_buf namebuf = boa_array_buf_ator(namebuf_data, original);

	tests = boa_test_get_all(&num);
	for (i = 0; i < num; i++) {
		boa_test_fail fail;
		boa_test *test = &tests[i];

		printf("%s %s: ", boa_format(boa_clear(&namebuf), "[%s]", test->name), test->description);
		fflush(stdout);
		int status = boa_test_run(test, &fail);
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

			boa_for (boa_test_permutation, p, &test->permutations) {
				printf("    Permutation %s: index %u\n", p->name, p->index);
			}

			printf("\n");
		}
	}

	boa_reset(&namebuf);

	printf("\n");
	printf("Tests passed: %d / %d\n", num_pass, (int)num);

	return num_pass == (int)num ? 0 : 1;
}


