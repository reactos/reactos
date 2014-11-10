/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/printf_s.c
 * PURPOSE:         Implementation of printf_s
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
printf_s(const char *format, ...)
{
    va_list argptr;
    int res;

    va_start(argptr, format);
    res = vfprintf_s(stdout, format, argptr);
    va_end(argptr);
    return res;
}
