/* Copyright (C) 1991, 1993, 1995 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <stdarg.h>
#undef	__OPTIMIZE__	/* Avoid inline `vprintf' function.  */
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>

#undef	vprintf
#undef	vwprintf

/* Write formatted output to stdout according to the
   format string FORMAT, using the argument list in ARG.  */
int
vprintf (format, arg)
     const char *format;
     va_list arg;
{
  int ret = vfprintf (stdout, format, arg);
  fflush(stdout);
  return ret;
}

int
vwprintf (format, arg)
     const wchar_t *format;
     va_list arg;
{
  int ret = vfwprintf (stdout, format, arg);
  fflush(stdout);
  return ret;
}
