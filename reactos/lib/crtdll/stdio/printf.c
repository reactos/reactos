/* Copyright (C) 1991, 1995, 1996 Free Software Foundation, Inc.
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
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <msvcrt/stdarg.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>

/* Write formatted output to stdout from the format string FORMAT.  */
/* VARARGS1 */
/*
 * @implemented
 */
int
printf (const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vprintf (format, arg);
  va_end (arg);
  return done;
}

/*
 * @implemented
 */
int
wprintf (const wchar_t *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vwprintf (format, arg);
  va_end (arg);
  return done;
}

#ifdef USE_IN_LIBIO
# undef _IO_printf
/* This is for libg++.  */
strong_alias (printf, _IO_printf);
#endif

