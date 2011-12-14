/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/wprintf.c
 * PURPOSE:         Implementation of wprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
__cdecl
wprintf(const wchar_t *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vwprintf(format, argptr);
    va_end(argptr);
    return result;
}

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
