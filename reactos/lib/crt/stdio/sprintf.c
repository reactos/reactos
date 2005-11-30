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

#include <precomp.h>

#include <wchar.h>
#include <tchar.h>

#undef sprintf
#undef wsprintf

/*
 * @implemented
 */
int
crt_sprintf(_TCHAR *str, const _TCHAR *fmt, ...)
{
  va_list arg;
  int done;

  va_start (arg, fmt);
  done = _vstprintf (str, fmt, arg);
  va_end (arg);
  return done;
}


/* Write formatted output into S, according to the format
   string FORMAT, writing no more than MAXLEN characters.  */
/* VARARGS3 */
int
crt__snprintf (_TCHAR *s, size_t maxlen,const _TCHAR *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = _vsntprintf(s, maxlen, format, arg);
  va_end (arg);

  return done;
}


