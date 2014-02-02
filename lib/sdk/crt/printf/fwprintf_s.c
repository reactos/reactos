/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/fwprintf_s.c
 * PURPOSE:         Implementation of fwprintf_s
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

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
