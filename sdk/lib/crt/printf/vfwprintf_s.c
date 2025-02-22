/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfwprintf_s.c
 * PURPOSE:         Implementation of vfwprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>
#include <internal/safecrt.h>

int __cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
vfwprintf_s(FILE* file, const wchar_t *format, va_list argptr)
{
    int ret;

    if(!MSVCRT_CHECK_PMT( file != NULL)) {
        _set_errno(EINVAL);
        return -1;
    }

    _lock_file(file);
    ret = wstreamout(file, format, argptr);
    _unlock_file(file);

    return ret;
}
