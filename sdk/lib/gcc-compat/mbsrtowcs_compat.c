/*
 * PROJECT:     GCC C++ support library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     mbsrtowcs shim for GCC 15 libwinpthread
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 *
 * GCC 15's libwinpthread references mbsrtowcs, which is only exported by
 * ReactOS msvcrt for DLL_EXPORT_VERSION >= 0x600. Provide a shim that
 * delegates to mbstowcs (always available).
 *
 * ReactOS CRT uses mbstate_t = int and only supports single-byte locales,
 * so the state parameter is effectively unused.
 */

#include <stdlib.h>

#undef mbsrtowcs

size_t __cdecl mbsrtowcs(wchar_t *dst, const char **src, size_t len, void *ps)
{
    size_t ret;
    const char *s;

    (void)ps;

    if (!src || !*src)
        return 0;

    s = *src;
    ret = mbstowcs(dst, s, len);

    if (ret == (size_t)-1)
        return (size_t)-1;

    if (dst) {
        if (ret < len)
            *src = NULL; /* null terminator was converted */
        else
            *src = s + ret;
    }

    return ret;
}

#ifdef _M_IX86
void *_imp__mbsrtowcs = mbsrtowcs;
#else
void *__imp_mbsrtowcs = mbsrtowcs;
#endif
