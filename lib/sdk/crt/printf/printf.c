/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/printf.c
 * PURPOSE:         Implementation of printf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
_cdecl
printf(const char *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = streamout(stdout, format, argptr);
    va_end(argptr);

    return result;
}

