/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vwprintf_s.c
 * PURPOSE:         Implementation of vwprintf
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
vwprintf_s(const wchar_t *format, va_list valist)
{
    return vfwprintf_s(stdout, format, valist);
}
