/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/sprintf.c
 * PURPOSE:         Implementation of sprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
_cdecl
sprintf(char *buffer, const char *format, ...)
{
    va_list argptr;
    int result;
    FILE stream;

    stream._base = buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
    stream._bufsiz = INT_MAX;
    stream._cnt = stream._bufsiz;
    stream._flag = 0;
    stream._tmpfname = 0;

    va_start(argptr, format);
    result = streamout(&stream, format, argptr);
    va_end(argptr);
    
    *stream._ptr = '\0';
    return result;
}

