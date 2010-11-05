/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vswprintf.c
 * PURPOSE:         Implementation of vswprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

int
__cdecl
vswprintf(wchar_t *buffer, const wchar_t *format, va_list argptr)
{
    return _vsnwprintf(buffer, INT_MAX, format, argptr);
}
