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
#include <tchar.h>

#ifdef _UNICODE
#define _tstreamout wstreamout
#else
#define _tstreamout streamout
#endif

int _cdecl _tstreamout(FILE *stream, const TCHAR *format, va_list argptr);

int
#if defined(USER32_WSPRINTF) && defined(_M_IX86)
_stdcall
#else
_cdecl
#endif
_sxprintf(
    TCHAR *buffer,
#if USE_COUNT
   size_t count,
#endif
    const TCHAR *format,
#if USE_VARARGS
    va_list argptr)
#else
    ...)
#endif
{
#if !USE_VARARGS
    va_list argptr;
#endif
    int result;
    FILE stream;

    stream._base = (char*)buffer;
    stream._ptr = stream._base;
    stream._charbuf = 0;
#if USE_COUNT
    stream._cnt = (int)(count * sizeof(TCHAR));
#else
    stream._cnt = INT_MAX;
#endif
    stream._bufsiz = 0;
    stream._flag = _IOSTRG | _IOWRT;
    stream._tmpfname = 0;

#if !USE_VARARGS
    va_start(argptr, format);
#endif
    result = _tstreamout(&stream, format, argptr);
#if !USE_VARARGS
    va_end(argptr);
#endif

    /* Only zero terminate if there is enough space left */
    if (stream._cnt >= sizeof(TCHAR)) *(TCHAR*)stream._ptr = _T('\0');

    return result;
}


