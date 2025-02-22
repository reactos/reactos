/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_scwprintf.c
 * PURPOSE:         Implementation of _scwprintf
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
_scwprintf(
    const wchar_t *format,
    ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = _vscwprintf(format, args);
    va_end(args);

    return len;
}
