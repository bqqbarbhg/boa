#pragma once

#ifndef BOA__BENCHMARK_IMPLEMENTED
#define BOA__BENCHMARK_IMPLEMENTED

#if defined(BOA__CORE_IMPLEMENTED)
	#error "boa_benchmark_impl.h needs to be included before boa_core_impl.h"
#endif

#include "boa_benchmark.h"
#include "boa_core.h"
#include "boa_os.h"
#include <stdlib.h>
#include <string.h>

uint32_t boa__benchmark_count;

typedef struct boa__benchmark_state {
	uint64_t begin_time;
	uint64_t begin_cycle;
	uint64_t first_run_time;
	int is_first_run;

	uint32_t total_runs;

	double avg_time;
	double avg_cycles;
} boa__benchmark_state;

typedef struct boa__benchmark_compile_permutation {
	const char *name;
	uint32_t *value;
	uint32_t *current_value;
} boa__benchmark_compile_permutation;

boa__benchmark_state boa__current_state;

static boa_buf boa__all_benchmarks;
boa_buf boa__benchmark_active_permutations;
boa_buf boa__benchmark_compile_permutations;

void boa_benchmark_add(void (*benchmark_fn)(), const char *name, const char *desc, const char *file, int line)
{
	boa_benchmark *benchmark = boa_push(boa_benchmark, &boa__all_benchmarks);
	if (benchmark) {
		benchmark->benchmark_fn = benchmark_fn;
		benchmark->name = name;
		benchmark->description = desc;
		benchmark->file = file;
		benchmark->line = line;

		benchmark->permutations = boa_empty_buf();
		boa_buf_push_buf(&benchmark->permutations, &boa__benchmark_active_permutations);
	}
}

void boa_benchmark_set_permutation(void *ptr, const char *name, const void *values, size_t num, size_t value_size, boa_format_fn format)
{
	boa_buf *active = &boa__benchmark_active_permutations;

	uint32_t i, num_active = boa_count(boa_benchmark_permutation, active);
	for (i = 0; i < num_active; i++) {
		boa_benchmark_permutation *p = &boa_get(boa_benchmark_permutation, active, i);
		if (p->ptr == ptr) break;
	}

	if (i < num_active) {
		if (values) {
			boa_benchmark_permutation *p = &boa_get(boa_benchmark_permutation, active, i); 
			p->values = values;
			p->num = (uint32_t)num;
			p->value_size = (uint32_t)value_size;
		} else {
			boa_erase(boa_benchmark_permutation, active, i);
		}
	} else if (values) {
		boa_benchmark_permutation *p = boa_push(boa_benchmark_permutation, active); 
		p->ptr = ptr;
		p->name = name;
		p->values = values;
		p->num = (uint32_t)num;
		p->value_size = (uint32_t)value_size;
		p->format_fn = format;
	}
}

boa__benchmark_compile_permutation *boa__benchmark_find_compile_permutation(const char *name)
{
	boa__benchmark_compile_permutation *perm = NULL;
	boa_for (boa__benchmark_compile_permutation, p, &boa__benchmark_compile_permutations) {
		if (!strcmp(p->name, name)) {
			perm = p;
			break;
		}
	}

	if (!perm) {
		perm = boa_push(boa__benchmark_compile_permutation, &boa__benchmark_compile_permutations);
		perm->name = name;
		perm->value = (uint32_t*)boa_alloc(sizeof(uint32_t));
		boa_assert(perm->value != NULL);
	}

	return perm;
}

void boa_benchmark_set_compile_permutation(uint32_t value, const char *name)
{
	boa__benchmark_compile_permutation *perm = boa__benchmark_find_compile_permutation(name);
	perm->current_value = (uint32_t*)boa_alloc(sizeof(uint32_t));
	boa_assert(perm->current_value != NULL);
	*perm->current_value = value;
	boa_benchmark_set_permutation(perm->value, name, perm->current_value, 1, sizeof(uint32_t), &boa_format_u32);
}

void boa_benchmark_reset_compile_permutation(const char *name)
{
	boa__benchmark_compile_permutation *perm = boa__benchmark_find_compile_permutation(name);
	boa_benchmark_set_permutation(perm->value, name, 0, 0, 0, 0);
}

static int boa__benchmark_next_permutation_values(boa_buf *permutations)
{
	boa_for (boa_benchmark_permutation, p, permutations) {
		p->index++;
		if (p->index < p->num) {
			return 1;
		} else {
			p->index = 0;
		}
	}
	return 0;
}

boa_benchmark *boa_benchmark_get_all(size_t *count)
{
	*count = boa_count(boa_benchmark, &boa__all_benchmarks);
	return boa_begin(boa_benchmark, &boa__all_benchmarks);
}

static void boa__benchmark_run_permutation(boa_benchmark *benchmark)
{
	benchmark->benchmark_fn();

	boa__benchmark_state *state = &boa__current_state;
	benchmark->avg_time = state->avg_time;
	benchmark->avg_cycles = state->avg_cycles;
	benchmark->num_runs = state->total_runs;
}

void boa_benchmark_prepare(boa_benchmark *benchmark)
{
	benchmark->permutation_index = -1;
	benchmark->finished = 0;
	boa_for (boa_benchmark_permutation, p, &benchmark->permutations) {
		p->index = 0;
	}
}

int boa_benchmark_run(boa_benchmark *benchmark, int permutation_filter)
{
	if (benchmark->finished) return 0;

	benchmark->permutation_index++;
	if (permutation_filter < 0 || permutation_filter == benchmark->permutation_index) {
		boa_for (boa_benchmark_permutation, p, &benchmark->permutations) {
			memcpy(p->ptr, (char*)p->values + p->index * p->value_size, p->value_size);
		}

		boa__benchmark_run_permutation(benchmark);
	}

	benchmark->finished = !boa__benchmark_next_permutation_values(&benchmark->permutations);
	return 1;
}

uint32_t boa_benchmark_begin()
{
	boa__benchmark_state *state = &boa__current_state;

	state->is_first_run = 1;
	state->begin_time = boa_perf_timer();
	state->begin_cycle = boa_cycle_timestamp();

	return 1;
}

uint32_t boa_benchmark_end()
{
	uint64_t end_time = boa_perf_timer();
	uint64_t end_cycle = boa_cycle_timestamp();
	boa__benchmark_state *state = &boa__current_state;

	uint64_t run_time = end_time - state->begin_time;
	uint64_t run_cycles = end_cycle - state->begin_cycle;
	double secs = boa_perf_sec(run_time);

	uint32_t runs = 0;
	if (state->is_first_run) {
		state->is_first_run = 0;

		double runsf = 0.25 / secs;
		if (runsf > 1000000.0)
			runsf = 1000000.0;

		runs = (uint32_t)runsf;
		state->is_first_run = 0;
	
		if (runs > 0) {
			state->begin_time = boa_perf_timer();
			state->begin_cycle = boa_cycle_timestamp();
			state->total_runs = runs;
		} else {
			state->total_runs = 1;
		}
	}

	if (runs == 0) {
		state->avg_time = secs / (double)state->total_runs;
		state->avg_cycles = (double)run_cycles / (double)state->total_runs;
	}

	return runs;
}

#endif

