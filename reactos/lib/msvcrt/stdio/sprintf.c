/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

/* Copyright (C) 1991, 1995 Free Software Foundation, Inc.
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

#include <msvcrt/stdarg.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <limits.h>
#include <msvcrt/internal/file.h>

#undef sprintf
#undef wsprintf
int
sprintf(char *str, const char *fmt, ...)
{
  va_list arg;
  int done;

  va_start (arg, fmt);
  done = vsprintf (str, fmt, arg);
  va_end (arg);
  return done;
}

int
swprintf(wchar_t *str, const wchar_t *fmt, ...)
{
  va_list arg;
  int done;

  va_start (arg, fmt);
  done = vswprintf (str, fmt, arg);
  va_end (arg);
  return done;
}



/* Write formatted output into S, according to the format
   string FORMAT, writing no more than MAXLEN characters.  */
/* VARARGS3 */
int
_snprintf (char *s, size_t maxlen,const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = _vsnprintf (s, maxlen, format, arg);
  va_end (arg);

  return done;
}

int
_snwprintf (wchar_t *s, size_t maxlen,const wchar_t *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = _vsnwprintf (s, maxlen, format, arg);
  va_end (arg);

  return done;
}


