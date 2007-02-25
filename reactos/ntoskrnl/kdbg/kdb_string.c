/*
 *  ReactOS kernel
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_string.c
 * PURPOSE:         Kernel debugger string functions
 * PROGRAMMER:      Gregor Anich (blight@blight.eu.org)
 * UPDATE HISTORY:
 *                  Created 17/01/2005
 */

/* INCLUDES ******************************************************************/
#include <ntoskrnl.h>
#include <ctype.h>

/* FUNCTIONS *****************************************************************/

#if 0
int
_stricmp(
   const char *s1,
   const char *s2)
{
   char c1, c2;
   for (;;)
   {
      c1 = tolower(*s1++);
      c2 = tolower(*s2++);
      if (c1 < c2)
         return -1;
      else if (c1 > c2)
         return 1;
      if (c1 == '\0')
         break;
   }
   return 0;
}
#endif /* unused */

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do {
    c = *s++;
  } while (isspace(c));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long)0xffffffff / (unsigned long)base;
  cutlim = (unsigned long)0xffffffff % (unsigned long)base;
  for (acc = 0, any = 0;; c = *s++)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = 0xffffffff;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? (char *)((size_t)(s - 1)) : (char *)((size_t)nptr);
  return acc;
}

