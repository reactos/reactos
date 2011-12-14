/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vprintf.c
 * PURPOSE:         Implementation of vprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
__cdecl
vprintf(const char *format, va_list argptr)
{
    return vfprintf(stdout, format, argptr);
}

int
__cdecl
vprintf_s(const char *format, va_list valist)
{
    return vfprintf_s(stdout,format,valist);
}
