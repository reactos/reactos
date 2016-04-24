/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vprintf_s.c
 * PURPOSE:         Implementation of vprintf
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
vprintf_s(const char *format, va_list valist)
{
    return vfprintf_s(stdout,format,valist);
}
