/* $Id: fputs.c,v 1.6 2003/07/11 21:58:09 royce Exp $
 *
 *  ReactOS msvcrt library
 *
 *  fputs.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  Based on original work Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details
 *                         28/12/1998: Appropriated for Reactos
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>
#include <msvcrt/string.h>

int
fputs(const char *s, FILE *f)
{
  int r = 0;
  int c;
  int unbuffered;
  char localbuf[BUFSIZ];

  unbuffered = f->_flag & _IONBF;
  if (unbuffered)
  {
    f->_flag &= ~_IONBF;
    f->_ptr = f->_base = localbuf;
    f->_bufsiz = BUFSIZ;
  }

  while ((c = *s++))
    r = putc(c, f);

  if (unbuffered)
  {
    fflush(f);
    f->_flag |= _IONBF;
    f->_base = NULL;
    f->_bufsiz = 0;
    f->_cnt = 0;
  }

  return(r);
}

/*
 * @implemented
 */
int
fputws(const wchar_t* s, FILE* f)
{
  int r = 0;
  int unbuffered;
  wchar_t c;
  wchar_t localbuf[BUFSIZ];

  unbuffered = f->_flag & _IONBF;
  if (unbuffered)
  {
    f->_flag &= ~_IONBF;
    f->_ptr = f->_base = localbuf;
    f->_bufsiz = BUFSIZ;
  }

  while ((c = *s++))
    r = putwc(c, f);

  if (unbuffered)
  {
    fflush(f);
    f->_flag |= _IONBF;
    f->_base = NULL;
    f->_bufsiz = 0;
    f->_cnt = 0;
  }
  return(r);
}
