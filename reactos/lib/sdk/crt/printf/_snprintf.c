/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_snprintf.c
 * PURPOSE:         Implementation of _snprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
_cdecl
_snprintf(char *buffer, size_t count, const char *format, ...)
{
    va_list argptr;
    int result;
    FILE stream;

    stream._base = buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
    stream._bufsiz = count;
    stream._cnt = stream._bufsiz;
    stream._flag = 0;
    stream._tmpfname = 0;

    va_start(argptr, format);
    result = streamout(&stream, format, argptr);
    va_end(argptr);

    *stream._ptr = '\0';
    return result;
}


