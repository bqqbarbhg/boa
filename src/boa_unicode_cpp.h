#pragma once

#ifndef BOA__UNICODE_CPP_INCLUDED
#define BOA__UNICODE_CPP_INCLUDED

#include "boa_core_cpp.h"
#include "boa_unicode.h"

namespace boa {

constexpr boa_result err_unicode_conversion = &boa_err_unicode_conversion;

constexpr const uint16_t *utf16_replacement_character = boa_utf16_replacement_character;
constexpr const char *utf8_replacement_character = boa_utf8_replacement_character;

boa_forceinline result convert_utf16_to_utf8(buf<char> &buf, const uint16_t *&ptr, const uint16_t *end) {
	return boa_convert_utf16_to_utf8(&buf, &ptr, end);
}
boa_forceinline result convert_utf8_to_utf16(buf<uint16_t> &buf, const char *&ptr, const char *end) {
	return boa_convert_utf8_to_utf16(&buf, &ptr, end);
}

boa_forceinline result convert_utf16_to_utf8_replace(buf<char> &buf, const uint16_t *&ptr, const uint16_t *end, const char *replace, uint32_t replace_len) {
	return boa_convert_utf16_to_utf8_replace(&buf, &ptr, end, replace, replace_len);
}
boa_forceinline result convert_utf8_to_utf16_replace(buf<uint16_t> &buf, const char *&ptr, const char *end, const uint16_t *replace, uint32_t replace_len) {
	return boa_convert_utf8_to_utf16_replace(&buf, &ptr, end, replace, replace_len);
}

boa_forceinline boa_result boa_convert_utf16_to_utf8_replace_default(buf<char> &buf, const uint16_t *&ptr, const uint16_t *end) {
	return boa_convert_utf16_to_utf8_replace_default(&buf, &ptr, end);
}
boa_forceinline boa_result boa_convert_utf8_to_utf16_replace_default(buf<uint16_t> &buf, const char *&ptr, const char *end) {
	return boa_convert_utf8_to_utf16_replace_default(&buf, &ptr, end);
}

boa_forceinline boa_result boa_convert_utf16_to_utf8_replace_empty(buf<char> &buf, const uint16_t *&ptr, const uint16_t *end) {
	return boa_convert_utf16_to_utf8_replace_empty(&buf, &ptr, end);
}
boa_forceinline boa_result boa_convert_utf8_to_utf16_replace_empty(buf<uint16_t> &buf, const char *&ptr, const char *end) {
	return boa_convert_utf8_to_utf16_replace_empty(&buf, &ptr, end);
}

}

#endif


