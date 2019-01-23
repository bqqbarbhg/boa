#pragma once

#ifndef BOA__OS_CPP_INCLUDED
#define BOA__OS_CPP_INCLUDED

#include "boa_core.h"
#include "boa_os.h"

namespace boa {

boa_forceinline uint64_t cycle_timestamp() { return boa_cycle_timestamp(); }
boa_forceinline uint64_t yield_cpu() { return boa_yield_cpu(); }

boa_forceinline uint64_t perf_timer() { return boa_perf_timer(); }
boa_forceinline uint64_t perf_freq() { return boa_perf_freq(); }
double perf_sec(uint64_t delta) { return boa_perf_sec(delta); }

// -- Theading

struct thread {
	boa_thread *thread;

	void join() {
		boa_join_thread(thread);
		thread = NULL;
	}
};

typedef boa_thread_opts thread_opts;

thread create_thread(const thread_opts &opts) {
	thread t;
	t.thread = boa_create_thread(&opts);
	return t;
}

}

#endif


