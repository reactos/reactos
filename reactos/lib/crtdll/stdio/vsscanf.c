/* Copyright (C) 1991, 1992, 1995, 1996 Free Software Foundation, Inc.
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

#include <crtdll/errno.h>
#include <crtdll/stdarg.h>
#include <crtdll/stdio.h>
#include <crtdll/string.h>
#include <crtdll/internal/file.h>

#undef  vsscanf

int __vfscanf (FILE *s, const char *format, va_list argptr);

/* Read formatted input from S according to the format
   string FORMAT, using the argument list in ARG.  */
int
__vsscanf (const char *s,const char *format,va_list arg)
{
  FILE f;

  if (s == NULL)
    {
      __set_errno (EINVAL);
      return -1;
    }

  memset ((void *) &f, 0, sizeof (f));



  f._flag = _IOREAD;
  f._ptr = (char *)s;
  f._base = (char *)s;
  f._bufsiz = strlen(s);
  f._cnt = f._bufsiz;
 

  return __vfscanf (&f, format, arg);
}


//weak_alias (__vsscanf, vsscanf)