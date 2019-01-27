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

#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif

	#include <time.h>
	#include <sys/time.h>

#if !defined(BOA_PTHREAD)
	#define BOA_PTHREAD 1
#endif

	// TEMP
	#undef BOA_NO_FILESYSTEM
	#define BOA_NO_FILESYSTEM 1

#endif

#if !defined(BOA_PTHREAD)
	#define BOA_PTHREAD 0
#endif

#if BOA_PTHREAD
	#include <sys/types.h>
	#include <pthread.h>
#endif

const boa_error_type boa_err_no_filesystem = { "boa_err_no_filesystem", "Built without filesystem support" };
const boa_error_type boa_err_file_not_found = { "boa_err_file_not_found", "File not found" };

#if BOA_WINDOWS

void boa__win32_push_last_error(boa_error **error, const char *context)
{
	boa_err_win32_data *data = (boa_err_win32_data*)boa_error_push(error, &boa_err_win32, context);
	if (data) {
		data->code = (uint32_t)GetLastError();
	}
}

void boa__win32_push_error_code(boa_error **error, uint32_t code, const char *context)
{
	boa_err_win32_data *data = (boa_err_win32_data*)boa_error_push(error, &boa_err_win32, context);
	data->code = code;
}

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

// -- File IO

int boa_has_filesystem()
{
	return BOA_NO_FILESYSTEM != 0 ? 0 : 1;
}

#if BOA_NO_FILESYSTEM

boa_dir_iterator *boa_dir_open(const char *path, const char *path_end, boa_error **error)
{
	boa_error_push(error, &boa_err_no_filesystem, "boa_dir_open()");
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
}

#elif BOA_WINDOWS

struct boa_dir_iterator {
	WIN32_FIND_DATAW data;
	HANDLE handle;
	int begin;
	char name_buf_data[256];
	boa_buf name_buf;
};

boa_dir_iterator *boa_dir_open(const char *path, boa_error **error)
{
	boa_error_type *errtype = NULL;
	boa_dir_iterator *it = NULL;

	it = boa_make(boa_dir_iterator);
	if (it == NULL) {
		errtype = &boa_err_no_space;
		goto error;
	}

	it->handle = INVALID_HANDLE_VALUE;
	it->name_buf = boa_array_buf(it->name_buf_data);

	const char *ptr = path;
	boa_convert_utf8_to_utf16(boa_clear(&it->name_buf), &ptr, NULL, error);
	if (*error) {
		errtype = &boa_err_bad_filename;
		goto error;
	}

	it->begin = 1;
	it->handle = FindFirstFileW((WCHAR*)it->name_buf.data, &it->data);

	if (it->handle == INVALID_HANDLE_VALUE) {
		DWORD code = GetLastError();
		boa__win32_push_error_code(error, code, "FindFirstFileW()");
		errtype = code == ERROR_FILE_NOT_FOUND ? &boa_err_file_not_found : &boa_err_external;
		goto error;
	}

	return it;

error:
	boa_error_push(error, errtype, "boa_dir_open()");
	if (it) boa_dir_close(it);
	return NULL;
}

boa_dir_status boa_dir_next(boa_dir_iterator *it, boa_dir_entry *entry, boa_error **error)
{
	boa_assert(it != NULL);
	boa_assert(it->handle != INVALID_HANDLE_VALUE);

	if (!it->begin) {
		BOOL result = FindNextFileW(it->handle, &it->data);
		if (!result) {
			DWORD code = GetLastError();
			if (code == ERROR_NO_MORE_FILES) {
				return boa_dir_end;
			} else {
				boa__win32_push_error_code(error, code, "FindNextFileW()");
				boa_error_push(error, &boa_err_external, "boa_dir_next()");
				return boa_dir_error;
			}
		}
	}

	while (it->begin || ) {
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
	if (it->handle != INVALID_HANDLE_VALUE)
		CloseHandle(it->handle);
	boa_reset(&it->name_buf);
	boa_free(it);
}

#else
	#error "No filesystem implementation for OS"
#endif

// -- Threading

#if BOA_SINGLETHREADED

boa_thread *boa_create_thread(const boa_thread_opts *opts)
{
	return nullptr;
}

void boa_join_thread(boa_thread *thread { }

#elif BOA_WINDOWS

// Windows thread name is set by throwing a special exception...
// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code

const DWORD MS_VC_EXCEPTION = 0x406D1388;  
#pragma pack(push,8)  
typedef struct { DWORD dwType; LPCSTR szName; DWORD dwThreadID; DWORD dwFlags; } THREADNAME_INFO;  
#pragma pack(pop)  
void SetThreadName(DWORD dwThreadID, const char* threadName) {  
    THREADNAME_INFO info = { 0x1000, threadName, dwThreadID, 0 };
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
    __try{  
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);  
    } __except (EXCEPTION_EXECUTE_HANDLER) { }  
#pragma warning(pop)  
}  

struct boa_thread {
	boa_allocator *ator;
	boa_thread_entry entry;
	void *user;

	HANDLE handle;
	DWORD thread_id;
};

DWORD WINAPI boa__win32_thread_entry(LPVOID arg)
{
	boa_thread *thread = (boa_thread*)arg;
	thread->entry(thread->user);
	return 0;
}

boa_thread *boa_create_thread(const boa_thread_opts *opts)
{
	boa_thread *thread = boa_make_ator(boa_thread, opts->ator);
	if (thread == NULL) return NULL;

	thread->ator = opts->ator;
	thread->entry = opts->entry;
	thread->user = opts->user;

	thread->handle = CreateThread(NULL,
			opts->stack_size,
			&boa__win32_thread_entry,
			thread,
			0,
			&thread->thread_id);

	if (thread->handle == NULL) {
		boa_free_ator(opts->ator, thread);
		return NULL;
	}

	if (opts->debug_name) {
		SetThreadName(thread->thread_id, opts->debug_name);
	}

	return thread;
}

void boa_join_thread(boa_thread *thread)
{
	boa_assert(thread != NULL);
	WaitForSingleObject(thread->handle, INFINITE);
	CloseHandle(thread->handle);
	boa_free_ator(thread->ator, thread);
}

#elif BOA_PTHREAD

struct boa_thread {
	boa_allocator *ator;
	boa_thread_entry entry;
	void *user;

	pthread_t pthread;
};

void *boa__pthread_thread_entry(void *arg)
{
	boa_thread *thread = (boa_thread*)arg;
	thread->entry(thread->user);
	return NULL;
}

boa_thread *boa_create_thread(const boa_thread_opts *opts)
{
	boa_thread *thread = boa_make_ator(boa_thread, opts->ator);
	if (thread == NULL) return NULL;
	int res = 0;

	thread->ator = opts->ator;
	thread->entry = opts->entry;
	thread->user = opts->user;

	pthread_attr_t attr;
	res = pthread_attr_init(&attr);

	if (res == 0 && opts->stack_size) {
		res = pthread_attr_setstacksize(&attr, opts->stack_size);
	}

	if (res != 0) {
		boa_free_ator(thread->ator, thread);
		return NULL;
	}

	res = pthread_create(&thread->pthread, &attr, &boa__pthread_thread_entry, thread);
	pthread_attr_destroy(&attr);

	if (res != 0) {
		boa_free_ator(thread->ator, thread);
		return NULL;
	}

#if BOA_LINUX
	if (opts->debug_name) {
		pthread_setname_np(thread->pthread, opts->debug_name);
	}
#endif

	return thread;
}

void boa_join_thread(boa_thread *thread)
{
	boa_assert(thread != NULL);
	pthread_join(thread->pthread, NULL);
	boa_free_ator(thread->ator, thread);
}

#else
	#error "No thread implementation for OS"
#endif

#endif

