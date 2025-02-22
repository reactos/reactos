/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_vscprintf.c
 * PURPOSE:         Implementation of _vscprintf
 */

#include <stdio.h>
#include <stdarg.h>

int __cdecl streamout(FILE *stream, const char *format, va_list argptr);

int
__cdecl
_vscprintf(
    const char *format,
    va_list argptr)
{
    FILE nulfile;
    nulfile._tmpfname = nulfile._ptr = nulfile._base = NULL;
    nulfile._bufsiz = nulfile._charbuf = nulfile._cnt = 0;
    nulfile._flag = _IOSTRG | _IOWRT;
    return streamout(&nulfile, format, argptr);
}
