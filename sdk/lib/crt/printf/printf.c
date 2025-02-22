/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/printf.c
 * PURPOSE:         Implementation of printf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
printf(const char *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vfprintf(stdout, format, argptr);
    va_end(argptr);

    return result;
}
