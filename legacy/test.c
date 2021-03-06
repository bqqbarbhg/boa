#include <stdio.h>
#include <string.h>

#define BOA_IMPLEMENTATION
#define BOA_USE_TEST_ALLOCATORS

#include "boa_test.h"

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
	const boa_test *tests;
	
	gather_tests();

	char namebuf_data[128];
	boa_buf namebuf = boa_buf_array(namebuf_data);

	tests = boa_test_get_all(&num);
	for (i = 0; i < num; i++) {
		boa_test_fail fail;

		printf("%s %s: ", boa_format(&namebuf, "[%s]", tests[i].name), tests[i].description);
		fflush(stdout);
		int status = boa_test_run(&tests[i], &fail);
		if (status) {
			num_pass++;
			printf("OK\n");
		} else {
			const char *slash = strrchr(fail.file, '/');
			const char *backslash = strrchr(fail.file, '\\');
			const char *file = fail.file;
			printf("FAIL\n");
			if (slash != NULL && slash > file) file = slash + 1;
			if (backslash != NULL && backslash > file) file = backslash + 1;
			if (fail.description) {
				printf("    X %s\n", fail.description);
				free(fail.description);
			}
			printf("    %s:%d: Assertion failed: %s\n", file, fail.line, fail.expression);
			printf("\n");
		}
	}

	boa_buf_reset(&namebuf);

	printf("\n");
	printf("Tests passed: %d / %d\n", num_pass, (int)num);

	return num_pass == (int)num ? 0 : 1;
}

