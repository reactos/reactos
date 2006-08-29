/* $Id$
 *
 *  FreeLoader
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

#include <freeldr.h>

static unsigned CurrentCursorX = 0;
static unsigned CurrentCursorY = 0;
static unsigned CurrentAttr = 0x0f;

VOID
XboxConsPutChar(int c)
{
  ULONG Width;
  ULONG Height;
  ULONG Depth;

  if ('\r' == c)
    {
      CurrentCursorX = 0;
    }
  else if ('\n' == c)
    {
      CurrentCursorX = 0;
      CurrentCursorY++;
    }
  else if ('\t' == c)
    {
      CurrentCursorX = (CurrentCursorX + 8) & ~ 7;
    }
  else
    {
      XboxVideoPutChar(c, CurrentAttr, CurrentCursorX, CurrentCursorY);
      CurrentCursorX++;
    }
  XboxVideoGetDisplaySize(&Width, &Height, &Depth);
  if (Width <= CurrentCursorX)
    {
      CurrentCursorX = 0;
      CurrentCursorY++;
    }
}

BOOLEAN
XboxConsKbHit(VOID)
{
  /* No keyboard support yet */
  return FALSE;
}

int
XboxConsGetCh(void)
{
  /* No keyboard support yet */
  while (1)
    {
      ;
    }

  return 0;
}

/* EOF */
