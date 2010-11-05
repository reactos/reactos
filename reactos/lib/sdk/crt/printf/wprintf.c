/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wprintf.c
 * PURPOSE:         Implementation of wprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int _cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
wprintf(const wchar_t *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = wstreamout(stdout, format, argptr);
    va_end(argptr);
    return result;
}
