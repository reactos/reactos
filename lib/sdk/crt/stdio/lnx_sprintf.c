/*
 * PROJECT:         ReactOS CRT library
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            lib/sdk/crt/stdio/lnx_sprintf.c
 * PURPOSE:         Base implementation of sprintf
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

#include <precomp.h>

#include <wchar.h>
#include <tchar.h>

/*
 * @implemented
 */
int
lnx_sprintf(_TCHAR *str, const _TCHAR *fmt, ...)
{
  va_list arg;
  int done;

  va_start (arg, fmt);
  done = lnx__vstprintf (str, fmt, arg);
  va_end (arg);
  return done;
}

