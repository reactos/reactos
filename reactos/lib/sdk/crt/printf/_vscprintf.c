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
_vscprintf(
   const char *format,
   va_list argptr)
{
    int ret;
    FILE* nulfile = fopen("nul", "w");
    if(nulfile == NULL)
    {
        /* This should never happen... */
        return -1;
    }
    ret = streamout(nulfile, format, argptr);
    fclose(nulfile);
    return ret;
}
