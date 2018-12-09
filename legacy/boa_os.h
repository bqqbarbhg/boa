
#include "boa_core.h"

typedef struct boa_dir_entry {

	// UTF8 encoded name relative to the parent
	const char *name;

} boa_dir_entry;

typedef struct boa__os_dir_iterator boa_dir_iterator;

boa_dir_iterator boa_dir_open(const char *path, int path_length);
int boa_dir_next(boa_dir_iterator it, boa_dir_entry *entry);
void boa_dir_close(boa_dir_iterator it);

#if defined(BOA_IMPLEMENTATION)

#if BOA_OS == BOA_WINDOWS

int boa__utf16_to_utf8_len(boa_buf *buf, const WCHAR *source, int source_len)
{
	char *ptr = (char*)buf->ptr;
	size_t cap = buf->ptr_cap;
	int len, res;

	if (source_len < 0) source_len = -1;

	if (source_len == 0 || source[0] == 0) {
		ptr = boa__buf_alloc(buf, 1);
		if (ptr == NULL) return -1;
		ptr[0] = '\0';
		return 0;
	}

	len = WideCharToMultiByte(CP_UTF8, 0, source, source_len, ptr, cap, NULL, NULL);
	if (source_len < 0) len -= 1;

	if (len > 0 && cap > 0 && len < cap) {
		ptr[len] = '\0';
		return len;
	} else if (len <= 0) {
		// Didn't fit to buffer, calculate required space
		len = WideCharToMultiByte(CP_UTF8, 0, source, source_len, NULL, 0, NULL, NULL);
		if (source_len < 0) len -= 1;
	}

	// Edge case: Failed to convert
	if (len <= 0) return -1;

	// Allocate required space and convert
	ptr = (WCHAR*)boa__buf_alloc(buf, (size_t)len + 1);
	if (ptr == NULL) return -1;

	res = WideCharToMultiByte(CP_UTF8, 0, source, source_len, ptr, len + 1, NULL, NULL);
	if (res <= 0) return -1;

	ptr[len] = '\0';

	return len;
}

int boa__utf16_to_utf8(boa_buf *buf, const WCHAR *source)
{
	return boa__utf16_to_utf8_len(buf, source, -1);
}

int boa__utf8_to_utf16_len(boa_buf *buf, const char *source, int source_len)
{
	WCHAR *ptr = (WCHAR*)buf->ptr;
	size_t cap = buf->ptr_cap;
	int len, res;

	if (source_len < 0) source_len = -1;

	if (source_len == 0 || source[0] == '\0') {
		ptr = (WCHAR*)boa__buf_alloc(buf, 2);
		if (ptr == NULL) return -1;
		ptr[0] = 0;
		return 0;
	}

	len = MultiByteToWideChar(CP_UTF8, 0, source, source_len, buf->ptr, cap / 2, NULL, NULL);
	if (source_len < 0) len -= 2;

	if (len > 0 && cap > 0 && len < cap) {
		len /= 2;
		ptr[len] = 0;
		return len;
	} else if (len <= 0) {
		// Didn't fit to buffer, calculate required space
		len = MultiByteToWideChar(CP_UTF8, 0, source, source_len, NULL, 0, NULL, NULL);
		if (source_len < 0) len -= 2;
	}

	// Edge case: Failed to convert
	if (len <= 0) return -1;

	// Allocate required space and convert
	ptr = (WCHAR*)boa__buf_alloc(buf, (size_t)len + 2);
	if (ptr == NULL) return -1;

	res = MultiByteToWideChar(CP_UTF8, 0, source, source_len, ptr, len + 2, NULL, NULL);
	if (res <= 0) return -1;

	len /= 2;
	ptr[len] = 0;
	return len;
}

int boa__utf8_to_utf16(boa_buf *buf, const char *source)
{
	return boa__utf16_to_utf8_len(buf, source, -1);
}

struct boa__os_dir_iterator {
	WIN32_FIND_DATAW data;
	HANDLE handle;
	int begin;
	char name_buf_data[256];
	boa_buf name_buf;
};

boa_dir_iterator boa_dir_open(const char *path, int path_length)
{
	int len;
	boa_dir_iterator it = (boa_dir_iterator)malloc(sizeof(boa__os_dir_iterator));
	it->name_buf = boa__buf_make(it->name_buf_data, sizeof(it->name_buf_data));

	len = boa__utf8_to_utf16_len(&it->name_buf, path, path_length);

	if (len < 0) {
		boa_dir_close(it);
		return NULL;
	}

	it->begin = 1;
	it->handle = FindFirstFileW((WCHAR)it->name_buf.ptr, &it->data);

	if (it->handle == INVALID_HANDLE_VALUE) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			boa_dir_close(it);
			return NULL;
		}
	}

	return it;
}

int boa_dir_next(boa_dir_iterator it, boa_dir_entry *entry)
{
	if (it == NULL) return 0;
	if (it->handle == INVALID_HANDLE_VALUE) return 0;

	while (it->begin || FindNextFileW(it->handle, &it->data)) {
		int len;
		it->begin = 0;

		len = boa__utf8_to_utf16_len(&it->name_buf, path, path_length);
		if (len < 0) continue;

		entry->name = (char*)it->name_buf->ptr;
		return 1;
	}

	return 0;
}

void boa_dir_close(boa_dir_iterator it)
{
	if (it == NULL) return 0;
	boa__buf_reset(&it->name_buf);
	free(it);
}

#endif

#endif

