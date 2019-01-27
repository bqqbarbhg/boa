#pragma once

#ifndef BOA__OS_INCLUDED
#define BOA__OS_INCLUDED

#include "boa_core.h"

#if BOA_X86
	#include <emmintrin.h>
#endif

uint64_t boa_cycle_timestamp();

boa_forceinline void boa_yield_cpu()
{
#if BOA_X86
	_mm_pause();
#endif
}

uint64_t boa_perf_timer();
uint64_t boa_perf_freq();
double boa_perf_sec(uint64_t delta);

typedef struct boa_dir_entry {
	// UTF8 encoded name relative to the parent
	const char *name;

	// If non-zero this entry is a directory
	int directory;
} boa_dir_entry;

// -- File IO

extern const boa_error boa_err_no_filesystem;
extern const boa_error boa_err_file_not_found;

typedef struct boa_dir_iterator boa_dir_iterator;

int boa_has_filesystem();

boa_dir_iterator *boa_dir_open(const char *path, const char *path_end, boa_result *result);
int boa_dir_next(boa_dir_iterator *it, boa_dir_entry *entry);
void boa_dir_close(boa_dir_iterator *it);

// -- Threading

typedef struct boa_thread boa_thread;

typedef void (*boa_thread_entry)(void *user);

typedef struct boa_thread_opts {
	boa_allocator *ator;
	boa_thread_entry entry;
	void *user;
	const char *debug_name;
	size_t stack_size;
} boa_thread_opts;

boa_thread *boa_create_thread(const boa_thread_opts *opts);
void boa_join_thread(boa_thread *thread);

#endif

