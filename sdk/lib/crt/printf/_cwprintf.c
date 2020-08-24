/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * PURPOSE:         Implementation of _cwprintf
 * PROGRAMMER:      Samuel Serapión
 */

#include <conio.h>
#include <stdarg.h>

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
