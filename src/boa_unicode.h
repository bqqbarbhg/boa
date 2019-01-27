#pragma once

#ifndef BOA__UNICODE_INCLUDED
#define BOA__UNICODE_INCLUDED

#include "boa_core.h"

int boa_convert_utf16_to_utf8(boa_buf *buf, const uint16_t **ptr, const uint16_t *end, boa_error **error);
int boa_convert_utf8_to_utf16(boa_buf *buf, const char **ptr, const char *end, boa_error **error);

int boa_convert_utf16_to_utf8_replace(boa_buf *buf, const uint16_t **ptr, const uint16_t *end, const char *replace, uint32_t replace_len);
int boa_convert_utf8_to_utf16_replace(boa_buf *buf, const char **ptr, const char *end, const uint16_t *replace, uint32_t replace_len);

extern const uint16_t boa_utf16_replacement_character[1];
extern const char boa_utf8_replacement_character[3];

boa_forceinline boa_result boa_convert_utf16_to_utf8_replace_default(boa_buf *buf, const uint16_t **ptr, const uint16_t *end) {
	return boa_convert_utf16_to_utf8_replace(buf, ptr, end, boa_utf8_replacement_character, 3);
}
boa_forceinline boa_result boa_convert_utf8_to_utf16_replace_default(boa_buf *buf, const char **ptr, const char *end) {
	return boa_convert_utf8_to_utf16_replace(buf, ptr, end, boa_utf16_replacement_character, 1);
}

boa_forceinline boa_result boa_convert_utf16_to_utf8_replace_empty(boa_buf *buf, const uint16_t **ptr, const uint16_t *end) {
	return boa_convert_utf16_to_utf8_replace(buf, ptr, end, NULL, 0);
}
boa_forceinline boa_result boa_convert_utf8_to_utf16_replace_empty(boa_buf *buf, const char **ptr, const char *end) {
	return boa_convert_utf8_to_utf16_replace(buf, ptr, end, NULL, 0);
}

#endif
