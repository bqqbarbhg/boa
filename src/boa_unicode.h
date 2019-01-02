#pragma once

#ifndef BOA__UNICODE_INCLUDED
#define BOA__UNICODE_INCLUDED

#include "boa_core.h"

int boa_convert_utf16_to_utf8(boa_buf *buf, const uint16_t **ptr, const uint16_t *end);
int boa_convert_utf8_to_utf16(boa_buf *buf, const char **ptr, const char *end);

#endif

