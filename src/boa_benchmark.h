
#include <stdint.h>

#ifdef BOA_BENCHMARK
	#undef BOA_BENCHMARK
	#undef BOA_BENCHMARK_BEGIN_PERMUTATION
	#undef BOA_BENCHMARK_END_PERMUTATION
	#undef BOA_BENCHMARK_BEGIN_COUNT
	#undef BOA_BENCHMARK_END_COUNT
	#undef BOA_BENCHMARK_BEGIN_COMPILE_PERMUTATION
	#undef BOA_BENCHMARK_END_COMPILE_PERMUTATION
#endif

#if BOA_BENCHMARK_MODE == BOA_BENCHMARK_IMPLEMENT
	extern uint32_t boa__benchmark_count;
	#define BOA_BENCHMARK(name, desc) void boa__benchmark_##name()
	#define BOA_BENCHMARK_BEGIN_PERMUTATION(name, values, format)
	#define BOA_BENCHMARK_END_PERMUTATION(name)
	#define BOA_BENCHMARK_BEGIN_COUNT(values)
	#define BOA_BENCHMARK_END_COUNT()
	#define BOA_BENCHMARK_BEGIN_COMPILE_PERMUTATION(name)
	#define BOA_BENCHMARK_END_COMPILE_PERMUTATION(name)
#elif BOA_BENCHMARK_MODE == BOA_BENCHMARK_GATHER
	#define BOA_BENCHMARK(name, desc) boa_benchmark_add(&boa__benchmark_##name, #name, desc, __FILE__, __LINE__); if (0)
	#define BOA_BENCHMARK_BEGIN_PERMUTATION(name, values, format) boa_benchmark_set_permutation(&(name), #name, (values), sizeof(values) / sizeof(*(values)), sizeof(*(values)), format);
	#define BOA_BENCHMARK_END_PERMUTATION(name) boa_benchmark_set_permutation(&(name), #name, 0, 0, 0, 0);
	#define BOA_BENCHMARK_BEGIN_COUNT(values) boa_benchmark_set_permutation(&boa__benchmark_count, "count", (values), sizeof(values) / sizeof(*(values)), sizeof(*(values)), &boa_format_u32);
	#define BOA_BENCHMARK_END_COUNT() boa_benchmark_set_permutation(&boa__benchmark_count, "count", 0, 0, 0, 0);
	#define BOA_BENCHMARK_BEGIN_COMPILE_PERMUTATION(name) boa_benchmark_set_compile_permutation(name, #name);
	#define BOA_BENCHMARK_END_COMPILE_PERMUTATION(name) boa_benchmark_reset_compile_permutation(#name);
#elif defined(BOA_BENCHMARK_MODE)
	#error "Invalid BOA_BENCHMARK_MODE"
#endif

#ifndef BOA__BENCHMARK_INCLUDED
#define BOA__BENCHMARK_INCLUDED

#define BOA_BENCHMARK_IMPLEMENT 0x1000
#define BOA_BENCHMARK_GATHER 0x2000

#define BOA_BENCHMARK_IMPL (BOA_BENCHMARK_MODE == BOA_BENCHMARK_IMPLEMENT)

#define BOA_BENCHMARK_BEGIN_PERMUTATION_U32(name, values) BOA_BENCHMARK_BEGIN_PERMUTATION(name, values, &boa_format_u32)

#define BOA__BENCHMARK_P_STEP(name, desc) BOA_BENCHMARK(name, desc)
#define BOA_BENCHMARK_P(name, desc) BOA__BENCHMARK_P_STEP(P(name), desc)

#include "boa_core.h"

extern uint32_t boa__benchmark_count;
#define boa_benchmark_count() (boa__benchmark_count)

#define boa_benchmark_assert(x) do { if (!(x)) __debugbreak(); } while (0)

typedef struct boa_benchmark {
	void (*benchmark_fn)();
	const char *name, *description, *file;
	int line;

	boa_buf permutations;
	int permutation_index;
	int finished;

	uint32_t num_runs;
	double avg_time;
	double avg_cycles;
} boa_benchmark;

typedef struct boa_benchmark_permutation {
	void *ptr;
	const char *name;
	const void *values;
	uint32_t num, value_size, index;
	boa_format_fn format_fn;
} boa_benchmark_permutation;

void boa_benchmark_add(void (*benchmark_fn)(), const char *name, const char *desc, const char *file, int line);
void boa_benchmark_set_permutation(void *ptr, const char *name, const void *values, size_t num, size_t value_size, boa_format_fn format);

void boa_benchmark_set_compile_permutation(uint32_t value, const char *name);
void boa_benchmark_reset_compile_permutation(const char *name);

boa_benchmark *boa_benchmark_get_all(size_t *count);

void boa_benchmark_prepare(boa_benchmark *benchmark);
int boa_benchmark_run(boa_benchmark *benchmark, int permutation_filter);

uint32_t boa_benchmark_begin();
uint32_t boa_benchmark_end();

#define boa_benchmark_for() for ( \
	uint32_t boa__benchmark_state = boa_benchmark_begin(); \
	boa__benchmark_state > 0 || (boa__benchmark_state = boa_benchmark_end()); \
	--boa__benchmark_state)

#endif


