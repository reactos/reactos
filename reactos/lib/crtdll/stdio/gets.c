/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdio/gets.c
 * PURPOSE:     Get a character string from stdin
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>

/*
 * @implemented
 */
char* gets(char* s)
{
  int c;
  char *cs;

  cs = s;
  while ((c = getchar()) != '\n' && c != EOF)
    *cs++ = c;
  if (c == EOF && cs==s)
    return NULL;
  *cs++ = '\0';
  return s;
}

#if 0
/* Copyright (C) 1991, 1994, 1995, 1996 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <errno.h>
#include <string.h>

link_warning (gets, "the `gets' function is dangerous and should not be used.")


/* Read a newline-terminated multibyte string from stdin into S,
   removing the trailing newline.  Return S or NULL.  */
  
char *
gets (s)
     char *s;
{
  register char *p = s;
  register int c;
  FILE *stream = stdin;
  int l;

  if (!__validfp (stream) || p == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  if (feof (stream) || ferror (stream))
    return NULL;

  while ((c = getc(stdin)) != EOF) {
    if (c == '\n')
	break;
    if ( isascii(c) ) 
    	*cs++ = c;
#ifdef _MULTIBYTE
    else if ( isleadbyte(c) ) {
    	l = mblen(c);
    	while(l > 0 ) {
    		c = getchar();
    		if ( isleadbyte(c) || c == EOF )
    			return NULL; // encoding error
    		*cs++ = c;
    		l--;
    	}
    }
#endif
    else
    	return NULL; // suspicious input
  }

  *p = '\0';

  /* Return null if we had an error, or if we got EOF
     before writing any characters.  */

  if (ferror (stream) || (feof (stream) && p == s))
    return NULL;

  return s;
}
  
#endif
