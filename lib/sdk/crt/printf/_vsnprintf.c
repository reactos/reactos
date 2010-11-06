/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vsnprintf.c
 * PURPOSE:         Implementation of _vsnprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
__cdecl
_vsnprintf(
   char *buffer,
   size_t count,
   const char *format,
   va_list argptr)
{
    int result;
    FILE stream;

    stream._base = buffer;
    stream._ptr = stream._base;
    stream._bufsiz = count;
    stream._cnt = stream._bufsiz;
    stream._flag = _IOSTRG | _IOWRT;
    stream._tmpfname = 0;
    stream._charbuf = 0;

    result = streamout(&stream, format, argptr);
    *stream._ptr = '\0';

    return result;
}
