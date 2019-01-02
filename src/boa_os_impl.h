#pragma once

#ifndef BOA__OS_IMPLEMENTED
#define BOA__OS_IMPLEMENTED

#include <boa_core.h>

#if BOA_MSVC
	#include <intrin.h>
#elif BOA_GNUC
	#include <x86intrin.h>
#endif

#if BOA_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#elif BOA_LINUX
	#include <time.h>
	#include <sys/time.h>
#endif

uint64_t boa_cycle_timestamp()
{
#if BOA_MSVC || BOA_GNUC
	return __rdtsc();
#else
	#error "Unimplemented"
#endif
}

#if BOA_WINDOWS
uint64_t boa_perf_timer()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (uint64_t)counter.QuadPart;
}

static uint64_t boa__win_perf_freq;
uint64_t boa_perf_freq()
{
	if (boa__win_perf_freq == 0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		boa__win_perf_freq = (uint64_t)freq.QuadPart;
	}
	return boa__win_perf_freq;
}
#elif BOA_LINUX
uint64_t boa_perf_timer()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1000000000ull;
}

uint64_t boa_perf_freq()
{
	return 1000000000ull;
}
#endif

double boa_perf_sec(uint64_t delta)
{
	uint64_t freq = boa_perf_freq();
	return (double)delta / (double)freq;
}

#endif

