/* $Id: fgets.c,v 1.3 2002/11/24 18:42:17 robd Exp $
 *
 *  ReactOS msvcrt library
 *
 *  fgets.c
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

#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>


char* fgets(char* s, int n, FILE* f)
{
  int c=0;
  char *cs;

  cs = s;
  while (--n>0 && (c = getc(f)) != EOF) {
    *cs++ = c;
    if (c == '\n')
      break;
  }
  if (c == EOF && cs == s)
    return NULL;
  *cs++ = '\0';
  return s;
}
