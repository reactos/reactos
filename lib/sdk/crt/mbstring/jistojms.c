/*
 * MSVCRT string functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <precomp.h>
#include <mbstring.h>
#include <locale.h>

/*
 * @implemented
 */
unsigned int _mbcjistojms(unsigned int c)
{
 /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(MSVCRT___lc_codepage == 932)
  {
    if(HIBYTE(c) >= 0x21 && HIBYTE(c) <= 0x7e &&
       LOBYTE(c) >= 0x21 && LOBYTE(c) <= 0x7e)
    {
      if(HIBYTE(c) % 2)
        c += 0x1f;
      else
        c += 0x7d;

      if(LOBYTE(c) > 0x7F)
        c += 0x1;

      c = (((HIBYTE(c) - 0x21)/2 + 0x81) << 8) | LOBYTE(c);

      if(HIBYTE(c) > 0x9f)
        c += 0x4000;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

