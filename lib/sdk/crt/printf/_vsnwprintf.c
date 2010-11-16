/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vsnwprintf.c
 * PURPOSE:         Implementation of _vsnwprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <stdarg.h>

int _cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
_vsnwprintf(
   wchar_t *buffer,
   size_t count,
   const wchar_t *format,
   va_list argptr)
{
    int result;
    FILE stream;

    stream._base = (char*)buffer;
    stream._ptr = stream._base;
    stream._bufsiz = count * sizeof(wchar_t);
    stream._cnt = stream._bufsiz;
    stream._flag = _IOSTRG | _IOWRT;
    stream._tmpfname = 0;
    stream._charbuf = 0;

    result = wstreamout(&stream, format, argptr);
    *(wchar_t*)stream._ptr = L'\0';

    return result;
}
