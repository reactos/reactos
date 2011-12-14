/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/fwprintf.c
 * PURPOSE:         Implementation of fwprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
__cdecl
fwprintf(FILE* file, const wchar_t *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vfwprintf(file, format, argptr);
    va_end(argptr);
    return result;
}

int
__cdecl
fwprintf_s(FILE* file, const wchar_t *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfwprintf_s(file, format, valist);
    va_end(valist);
    return res;
}
