/*
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
/*
 * Xen virtualization of the console isn't that great, it doesn't properly
 * virtualize full-screen access. We simulate it by sending ANSI control
 * codes and assume a display size of 80x25. This should be enough for proper
 * display on e.g. linux consoles and xterms.
 */

#include "freeldr.h"
#include "machxen.h"

#define ROWS 25
#define COLS 80

static unsigned CurrentX = (unsigned) -1;
static unsigned CurrentY = (unsigned) -1;
static UCHAR CurrentAttr = 0x07;


static VOID
AnsiMoveToPos(unsigned X, unsigned Y)
{
  CurrentX = X;
  CurrentY = Y;
  printf("\033[%d;%dH", Y + 1, X + 1);
}

static VOID
AnsiSetAttr(UCHAR Attr)
{
  /* Convert color from BIOS (RGB) to ANSI (BGR) */
  static UCHAR ConvertColor[] =
    { 0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07 };
  char Bright[3];

  CurrentAttr = Attr;
  /* Check for bright foreground */
  if (0 != (Attr & 0x08))
    {
      strcpy(Bright, "1;");
    }
  else
    {
      Bright[0] = '\0';
    }
  printf("\033[%s3%d;4%dm", Bright, ConvertColor[Attr & 0x07],
         ConvertColor[(Attr >> 4) & 0x07]);
}

static VOID
AnsiClearScreen()
{
  printf("\033[H\033[J\n");
}

VOID
XenVideoClearScreen(UCHAR Attr)
{
  if (Attr != CurrentAttr)
    {
      AnsiSetAttr(Attr);
    }
  AnsiClearScreen();
}

VOID
XenVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  /* Don't write outside the screen or on the bottom-right position,
     it might trigger a scroll operation */
  if (COLS <= X || ROWS <= Y
      || (X == COLS - 1 && Y == ROWS - 1))
    {
      return;
    }

  if (X != CurrentX || Y != CurrentY)
    {
      AnsiMoveToPos(X, Y);
    }
  if (Attr != CurrentAttr)
    {
      AnsiSetAttr(Attr);
    }
  XenConsPutChar(Ch);
  CurrentX++;
}

/* EOF */
