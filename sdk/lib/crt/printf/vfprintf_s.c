/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfprintf_s.c
 * PURPOSE:         Implementation of vfprintf_s
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>
#include <internal/safecrt.h>

int __cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
__cdecl
vfprintf_s(FILE* file, const char *format, va_list argptr)
{
    int result;

    if(!MSVCRT_CHECK_PMT(format != NULL)) {
        _set_errno(EINVAL);
        return -1;
    }

    _lock_file(file);
    result = streamout(file, format, argptr);
    _unlock_file(file);

    return result;
}
