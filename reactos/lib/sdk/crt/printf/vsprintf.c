#include "myfunc.h"
/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vsprintf.c
 * PURPOSE:         Implementation of vsprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

int _cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
__cdecl
vsprintf(
   char *buffer,
   const char *format,
   va_list argptr)
{
    int result;
    FILE stream;

    stream._base = buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
    stream._bufsiz = INT_MAX;
    stream._cnt = stream._bufsiz;
    stream._flag = _IOSTRG|_IOWRT|_IOMYBUF;
    stream._tmpfname = 0;

    result = streamout(&stream, format, argptr);
    *stream._ptr = '\0';

    return result;
}
