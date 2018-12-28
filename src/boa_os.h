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

#endif

