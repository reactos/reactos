/* Copyright (C) 1991 Free Software Foundation, Inc.
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


int __vfscanf (FILE *s, const char *format, va_list argptr);
/* Read formatted input from STREAM according to the format string FORMAT.  */
/* VARARGS2 */
int fscanf(FILE *stream,const char *format, ...)
{
  va_list arg;
  int done;

  va_start(arg, format);
  done = __vfscanf(stream, format, arg);
  va_end(arg);

  return done;
}

int
fwscanf(FILE *stream, const wchar_t *fmt, ...)
{
  va_list arg;
  int done;
  char *cf;
  int i,len = wcslen(fmt);

  cf = malloc(len+1);
  for(i=0;i<len;i++)
	cf[i] = fmt[i];
  cf[i] = 0;  

  va_start(arg, fmt);
  done = __vfscanf(stream, cf, arg);
  va_end(arg);
  free(cf);
  return done;
}

