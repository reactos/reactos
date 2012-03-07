/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vcwprintf.c
 * PURPOSE:         Implementation of _vcwprintf
 * PROGRAMMER:      Samuel Serapión
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
_vcwprintf(const wchar_t* format, va_list va)
{
    return vfwprintf(stdout, format, va);
}
