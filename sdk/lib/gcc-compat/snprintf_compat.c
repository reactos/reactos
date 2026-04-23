/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Local snprintf to override GCC 15 libmsvcrt import stub
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 */

#include <stdio.h>
#include <stdarg.h>

#undef snprintf

int __cdecl snprintf(char *s, size_t n, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = _vsnprintf(s, n, fmt, ap);
    va_end(ap);
    if (s && n > 0)
        s[n - 1] = '\0';
    return ret;
}

#ifdef _M_IX86
void *_imp__snprintf = snprintf;
#else
void *__imp_snprintf = snprintf;
#endif
