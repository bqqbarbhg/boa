
#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct {
	void *head;
	size_t size;
} debug_header;

void *debug_alloc(size_t size)
{
	size_t align8 = (size + 7) & ~7;
	align8 += 16;

	size_t align_page = (align8 + 4095) & ~4095;
	size_t slack = align_page - align8;
	char *ptr = (char*)VirtualAlloc(NULL, align_page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	debug_header *hdr = (debug_header*)(ptr + slack);
	hdr->head = ptr;
	hdr->size = size;
	return (char*)hdr + 16;
}

void debug_free(void *ptr)
{
	debug_header *hdr = (debug_header*)((char*)ptr - 16);
	VirtualFree(hdr->head, 0, MEM_RELEASE);
}

void *debug_realloc(void *ptr, size_t size)
{
	debug_header *hdr = (debug_header*)((char*)ptr - 16);
	void *p = debug_alloc(size);
	if (p && ptr) {
		memcpy(p, ptr, hdr->size);
		debug_free(ptr);
	}
	return p;
}

#define BOA_ALLOC debug_alloc
#define BOA_REALLOC debug_realloc
#define BOA_FREE debug_free

#endif

#include "test_main.h"

