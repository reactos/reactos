/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vfprintf.c
 * PURPOSE:         Implementation of vfprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

void _cdecl _lock_file(FILE* file);
void _cdecl _unlock_file(FILE* file);
int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
_cdecl
vfprintf(FILE *stream, const char *format, va_list argptr)
{
    int result;

    _lock_file(stream);
    
    result = streamout(stream, format, argptr);
    
    _unlock_file(stream);

    return result;
}
