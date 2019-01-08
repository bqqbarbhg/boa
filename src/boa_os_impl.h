#pragma once

#ifndef BOA__OS_IMPLEMENTED
#define BOA__OS_IMPLEMENTED

#include "boa_core.h"
#include "boa_os.h"

#if !defined(BOA_NO_FILESYSTEM)
	#define BOA_NO_FILESYSTEM 0
#endif

#if BOA_MSVC
	#include <intrin.h>
#elif BOA_GNUC
	#include <x86intrin.h>
#endif

#if BOA_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include "boa_unicode.h"
#elif BOA_LINUX
	#include <time.h>
	#include <sys/time.h>
#endif

const boa_error boa_err_no_filesystem = { "Built without filesystem support" };
const boa_error boa_err_file_not_found = { "File not found" };

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

// File IO

int boa_has_filesystem()
{
	return BOA_NO_FILESYSTEM != 0 ? 0 : 1;
}

#if BOA_NO_FILESYSTEM

boa_dir_iterator *boa_dir_open(const char *path, const char *path_end, boa_result *result)
{
	if (result) *result = &boa_err_no_filesystem;
	return NULL;
}

int boa_dir_next(boa_dir_iterator *it, boa_dir_entry *entry)
{
	boa_assert(0 && "No filesystem support");
	return 0;
}

void boa_dir_close(boa_dir_iterator *it)
{
	boa_assert(0 && "No filesystem support");
	return 0;
}

#elif BOA_WINDOWS

struct boa_dir_iterator {
	WIN32_FIND_DATAW data;
	HANDLE handle;
	int begin;
	char name_buf_data[256];
	boa_buf name_buf;
};

boa_dir_iterator *boa_dir_open(const char *path, const char *path_end, boa_result *result)
{
	boa_result res;
	boa_dir_iterator *it = (boa_dir_iterator*)boa_alloc(sizeof(boa_dir_iterator));
	it->name_buf = boa_array_buf(it->name_buf_data);

	const char *ptr = path;
	res = boa_convert_utf8_to_utf16(boa_clear(&it->name_buf), &ptr, path_end);
	if (res != boa_ok) {
		if (result) *result = res;
		boa_dir_close(it);
		return NULL;
	}

	it->begin = 1;
	it->handle = FindFirstFileW((WCHAR*)it->name_buf.data, &it->data);

	if (it->handle == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			if (result) *result = &boa_err_file_not_found;
			boa_dir_close(it);
			return NULL;
		}
	}

	return it;
}

int boa_dir_next(boa_dir_iterator *it, boa_dir_entry *entry)
{
	boa_assert(it != NULL);
	boa_assert(it->handle != INVALID_HANDLE_VALUE);

	while (it->begin || FindNextFileW(it->handle, &it->data)) {
		it->begin = 0;

		const uint16_t *ptr = (const uint16_t*)it->data.cFileName;
		boa_result res = boa_convert_utf16_to_utf8(boa_clear(&it->name_buf), &ptr, NULL);
		if (res == boa_ok) {
			entry->name = (const char*)it->name_buf.data;
			return 1;
		}
	}

	return 0;
}

void boa_dir_close(boa_dir_iterator *it)
{
	boa_assert(it != NULL);
	CloseHandle(it->handle);
	boa_reset(&it->name_buf);
	boa_free(it);
}

#endif

#endif

