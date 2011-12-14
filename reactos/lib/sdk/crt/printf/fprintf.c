/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/fprintf.c
 * PURPOSE:         Implementation of fprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <precomp.h>

int
__cdecl
fprintf(FILE *file, const char *format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = vfprintf(file, format, argptr);
    va_end(argptr);
    return result;
}

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
