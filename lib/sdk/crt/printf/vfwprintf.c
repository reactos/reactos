/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfwprintf.c
 * PURPOSE:         Implementation of vfwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int _cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
vfwprintf(FILE* file, const wchar_t *format, va_list argptr)
{
    return wstreamout(file, format, argptr);
}

