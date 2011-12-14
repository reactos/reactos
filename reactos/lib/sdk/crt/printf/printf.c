/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/printf.c
 * PURPOSE:         Implementation of printf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
_cdecl
printf(const char *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vprintf(format, argptr);
    va_end(argptr);

    return result;
}

int
_cdecl
printf_s(const char *format, ...)
{
    va_list argptr;
    int res;

    va_start(argptr, format);
    res = vprintf_s(format, argptr);
    va_end(argptr);
    return res;
}

