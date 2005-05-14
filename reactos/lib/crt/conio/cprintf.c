/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/conio/cprintf.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Eric Kohl (Imported from DJGPP)
 */

#include <stdio.h>
#include <stdarg.h>
#include <conio.h>

/*
 * @unimplemented
 */
int
_cprintf(const char *fmt, ...)
{
  int     cnt;
  char    buf[ 2048 ];		/* this is buggy, because buffer might be too small. */
  va_list ap;

  va_start(ap, fmt);
  cnt = vsprintf(buf, fmt, ap);
  va_end(ap);

  _cputs(buf);
  return cnt;
}
