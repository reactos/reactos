/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of fwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

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

