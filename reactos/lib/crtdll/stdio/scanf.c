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
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <stdarg.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/alloc.h>


/* The function `vscanf' is not defined in ISO C.  Therefore we must
   use the protected form here.  In stdio it is called `__vscanf' and
   in libio `_IO_vscanf'.  */
#ifdef USE_IN_LIBIO
# include <libioP.h>
# define VSCANF _IO_vscanf
#else
# define VSCANF __vscanf
#endif

int __vscanf (const char *format, va_list arg);

/* Read formatted input from stdin according to the format string FORMAT.  */
/* VARARGS1 */
int scanf (const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = VSCANF (format, arg);
  va_end (arg);

  return done;
}

int
wscanf(const wchar_t *fmt, ...)
{
  va_list arg;
  int done;
  char *f;
  int i, len = wcslen(fmt);

  f = malloc(len+1);
  for(i=0;i<len;i++)
	f[i] = fmt[i];
  f[i] = 0;  
  va_start (arg, fmt);
  done = VSCANF (f, arg);
  va_end (arg);
  free(f);

  return done;
}
