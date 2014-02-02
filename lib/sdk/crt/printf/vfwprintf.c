/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfwprintf.c
 * PURPOSE:         Implementation of vfwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int __cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
vfwprintf(FILE* file, const wchar_t *format, va_list argptr)
{
     int ret;

    _lock_file(file);
    ret = wstreamout(file, format, argptr);
    _unlock_file(file);

    return ret;
}
