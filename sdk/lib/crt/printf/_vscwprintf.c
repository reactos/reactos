/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vscwprintf.c
 * PURPOSE:         Implementation of _vscprintf
 */

#include <stdio.h>
#include <stdarg.h>

#ifdef _LIBCNT_
#include <ntddk.h>
#endif

int __cdecl wstreamout(FILE *stream, const wchar_t *format, va_list argptr);

int
__cdecl
_vscwprintf(
   const wchar_t *format,
   va_list argptr)
{
    int ret;
#ifndef _LIBCNT_
    FILE nulfile;
    nulfile._tmpfname = nulfile._ptr = nulfile._base = NULL;
    nulfile._bufsiz = nulfile._charbuf = nulfile._cnt = 0;
    nulfile._flag = _IOSTRG | _IOWRT;
    ret = wstreamout(&nulfile, format, argptr);
#else
    ret = -1;
#endif
    return ret;
}
