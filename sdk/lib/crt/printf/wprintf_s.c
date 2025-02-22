/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wprintf_s.c
 * PURPOSE:         Implementation of wprintf
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
wprintf_s(const wchar_t *format, ...)
{
    va_list argptr;
    int res;
    va_start(argptr, format);
    res = vwprintf_s(format, argptr);
    va_end(argptr);
    return res;
}
