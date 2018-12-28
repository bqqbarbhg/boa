#include <boa_benchmark_impl.h>
#include <boa_core_impl.h>
#include <boa_os_impl.h>

#include <stdio.h>
#include <string.h>

#include <boa_benchmark.h>

#define BOA_BENCHMARK_MODE BOA_BENCHMARK_IMPLEMENT
#include "benchmark_all.h"
#undef BOA_BENCHMARK_MODE

void gather_benchmarks()
{
#define BOA_BENCHMARK_MODE BOA_BENCHMARK_GATHER
#include "benchmark_all.h"
#undef BOA_BENCHMARK_MODE
}

int main(int argc, char **argv)
{
	int num_pass = 0;
	size_t i, num;
	boa_benchmark *benchmarks;

	const char *benchmark_filter = NULL;
	int permutation_filter = -1;

	if (argc > 1) {
		benchmark_filter = argv[1];
	}

	if (argc > 2) {
		permutation_filter = atoi(argv[2]);
	}
	
	gather_benchmarks();

	int num_ran = 0;
	benchmarks = boa_benchmark_get_all(&num);
	for (i = 0; i < num; i++) {
		boa_benchmark *benchmark = &benchmarks[i];

		if (benchmark_filter) {
			if (strcmp(benchmark->name, benchmark_filter)) continue;
		}
		num_ran++;

		printf("[%s] %s:\n", benchmark->name, benchmark->description);
		fflush(stdout);
		boa_benchmark_prepare(benchmark);
		while (boa_benchmark_run(benchmark, permutation_filter)) {
			uint32_t count = boa_benchmark_count();
			double ns = benchmark->avg_time / (double)count * 1000.0 * 1000.0 * 1000.0;
			double cy = benchmark->avg_cycles / (double)count;
			double ghz = cy / ns;
			printf("  %12u: %10.2fns %10.2fcy %10u runs\n", count, ns, cy, benchmark->num_runs);
		}
	}

	printf("\n");

	return num_pass == num_ran ? 0 : 1;
}

