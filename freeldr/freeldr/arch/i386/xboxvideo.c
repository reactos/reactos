/* $Id: xboxvideo.c,v 1.2 2004/11/10 23:45:37 gvg Exp $
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
 *
 * Note: much of this code was based on knowledge and/or code developed
 * by the Xbox Linux group: http://www.xbox-linux.org
 */

#include "freeldr.h"
#include "debug.h"
#include "rtl.h"
#include "machine.h"
#include "machxbox.h"

static PVOID FrameBuffer;
static U32 ScreenWidth;
static U32 ScreenHeight;
static U32 BytesPerPixel;
static U32 Delta;

static unsigned CurrentCursorX;
static unsigned CurrentCursorY;
static unsigned CurrentFgColor;
static unsigned CurrentBgColor;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define TOP_BOTTOM_LINES 40

#define FB_SIZE_MB 4

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

static VOID
XboxVideoOutputChar(U8 Char, unsigned X, unsigned Y, U32 FgColor, U32 BgColor)
{
  PU8 FontPtr;
  PU32 Pixel;
  U8 Mask;
  unsigned Line;
  unsigned Col;

  FontPtr = XboxFont8x16 + Char * 16;
  Pixel = (PU32) ((char *) FrameBuffer + (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta
                  + X * CHAR_WIDTH * BytesPerPixel);
  for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
      Mask = 0x80;
      for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
          Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
          Mask = Mask >> 1;
        }
      Pixel = (PU32) ((char *) Pixel + Delta);
    }
}

static U32
XboxVideoAttrToSingleColor(U8 Attr)
{
  U8 Intensity;

  Intensity = (0 == (Attr & 0x08) ? 127 : 255);

  return 0xff000000 |
         (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
         (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
         (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
XboxVideoAttrToColors(U8 Attr, U32 *FgColor, U32 *BgColor)
{
  *FgColor = XboxVideoAttrToSingleColor(Attr & 0xf);
  *BgColor = XboxVideoAttrToSingleColor((Attr >> 4) & 0xf);
}

static VOID
XboxVideoClearScreen(U32 Color, BOOL FullScreen)
{
  U32 Line, Col;
  PU32 p;

  for (Line = 0; Line < ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
      p = (PU32) ((char *) FrameBuffer + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
      for (Col = 0; Col < ScreenWidth; Col++)
        {
          *p++ = Color;
        }
    }
}

VOID
XboxVideoClearScreenAttr(U8 Attr)
{
  U32 FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoClearScreen(BgColor, FALSE);
}

VOID
XboxVideoPutChar(int c)
{
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
      XboxVideoOutputChar(c, CurrentCursorX, CurrentCursorY, CurrentFgColor, CurrentBgColor);
      CurrentCursorX++;
    }
  if (ScreenWidth / CHAR_WIDTH <= CurrentCursorX)
    {
      CurrentCursorX = 0;
      CurrentCursorY++;
    }
}

VOID
XboxVideoPutCharAttrAtLoc(int Ch, U8 Attr, unsigned X, unsigned Y)
{
  U32 FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoOutputChar(Ch, X, Y, FgColor, BgColor);
}

VOID
XboxVideoInit(VOID)
{
  FrameBuffer = (PVOID)((U32) XboxMemReserveMemory(FB_SIZE_MB) | 0xf0000000);
  ScreenWidth = 640;
  ScreenHeight = 480;
  BytesPerPixel = 4;
  Delta = (ScreenWidth * BytesPerPixel + 3) & ~ 0x3;

  CurrentCursorX = 0;
  CurrentCursorY = 0;
  CurrentFgColor = MAKE_COLOR(192, 192, 192);
  CurrentBgColor = MAKE_COLOR(0, 0, 0);

  XboxVideoClearScreen(CurrentBgColor, TRUE);

  /* Tell the nVidia controller about the framebuffer */
  *((PU32) 0xfd600800) = (U32) FrameBuffer;
}

/* EOF */
