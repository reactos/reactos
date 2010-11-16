/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/swprintf.c
 * PURPOSE:         Implementation of swprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

int _cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
_cdecl
swprintf(wchar_t *buffer, const wchar_t *format, ...)
{
    va_list argptr;
    int result;
    FILE stream;

    stream._base = (char*)buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
    stream._bufsiz = INT_MAX;
    stream._cnt = stream._bufsiz;
    stream._flag = 0;
    stream._tmpfname = 0;

    va_start(argptr, format);
    result = wstreamout(&stream, format, argptr);
    va_end(argptr);
    
    *(wchar_t*)stream._ptr = '\0';
    return result;
}


