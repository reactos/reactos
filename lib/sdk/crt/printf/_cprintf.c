/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vcprintf.c
 * PURPOSE:         Implementation of _vcprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdarg.h>

int _vcprintf(const char* format, va_list argptr);

int
_cdecl
_cprintf(const char * format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = _vcprintf(format, argptr);
    va_end(argptr);
    return result;
}

