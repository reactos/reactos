/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vwprintf.c
 * PURPOSE:         Implementation of vwprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
__cdecl
vwprintf(const wchar_t *format, va_list valist)
{
    return vfwprintf(stdout,format,valist);
}

int
__cdecl
vwprintf_s(const wchar_t *format, va_list valist)
{
    return vfwprintf_s(stdout,format,valist);
}
