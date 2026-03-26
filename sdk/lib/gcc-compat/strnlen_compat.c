/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     strnlen/wcsnlen shims for GCC 15 libmingwex
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 *
 * GCC 15's libmingwex references strnlen and wcsnlen, which ReactOS msvcrt
 * only exports for DLL_EXPORT_VERSION >= 0x600.
 */

#include <stddef.h>

#undef strnlen
#undef wcsnlen

size_t __cdecl strnlen(const char *s, size_t maxlen)
{
    size_t i;
    for (i = 0; i < maxlen && s[i]; i++)
        ;
    return i;
}

size_t __cdecl wcsnlen(const wchar_t *s, size_t maxlen)
{
    size_t i;
    for (i = 0; i < maxlen && s[i]; i++)
        ;
    return i;
}

#ifdef _M_IX86
void *_imp__strnlen = strnlen;
void *_imp__wcsnlen = wcsnlen;
#else
void *__imp_strnlen = strnlen;
void *__imp_wcsnlen = wcsnlen;
#endif
