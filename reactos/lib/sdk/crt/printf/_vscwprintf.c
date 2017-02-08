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
    FILE* nulfile;
    nulfile = fopen("nul", "w");
    if(nulfile == NULL)
    {
        /* This should never happen... */
        return -1;
    }
    ret = wstreamout(nulfile, format, argptr);
    fclose(nulfile);
#else
    ret = -1;
#endif
    return ret;
}
