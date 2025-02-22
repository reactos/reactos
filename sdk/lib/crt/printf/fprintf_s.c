/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/fprintf_s.c
 * PURPOSE:         Implementation of fprintf_s
 * PROGRAMMER:      Samuel Serapión
 */

#define MINGW_HAS_SECURE_API 1

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
fprintf_s(FILE* file, const char *format, ...)
{
    va_list argptr;
    int result;
    va_start(argptr, format);
    result = vfprintf_s(file, format, argptr);
    va_end(argptr);
    return result;
}
