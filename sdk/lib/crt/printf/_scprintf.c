/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of _scprintf
 */

#include <stdio.h>
#include <stdarg.h>

int
__cdecl
_scprintf(
    const char *format,
    ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = _vscprintf(format, args);
    va_end(args);

    return len;
}
