/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vprintf.c
 * PURPOSE:         Implementation of vprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
vprintf(const char *format, va_list argptr)
{
    return vfprintf(stdout, format, argptr);
}
