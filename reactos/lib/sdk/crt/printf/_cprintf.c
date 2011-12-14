/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/_cprintf.c
 * PURPOSE:         Implementation of _cprintf
 * PROGRAMMER:      Timo Kreuzer
 *                  Samuel Serapión
 */

#include <conio.h>
#include <stdarg.h>

int
__cdecl
_cprintf(const char * format, ...)
{
    va_list argptr;
    int result;

    va_start(argptr, format);
    result = _vcprintf(format, argptr);
    va_end(argptr);
    return result;
}

int
__cdecl
_cwprintf(const wchar_t* format, ...)
{
  int retval;
  va_list valist;

  va_start( valist, format );
  retval = _vcwprintf(format, valist);
  va_end(valist);

  return retval;
}
