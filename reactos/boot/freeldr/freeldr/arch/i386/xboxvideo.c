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
 *
 * Note: much of this code was based on knowledge and/or code developed
 * by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>

static PVOID FrameBuffer;
static ULONG ScreenWidth;
static ULONG ScreenHeight;
static ULONG BytesPerPixel;
static ULONG Delta;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

#define TOP_BOTTOM_LINES 0

#define FB_SIZE_MB 4

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

BOOLEAN I2CTransmitByteGetReturn(UCHAR bPicAddressI2cFormat, UCHAR bDataToWrite, ULONG *Return);

static VOID
XboxVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
  PUCHAR FontPtr;
  PULONG Pixel;
  UCHAR Mask;
  unsigned Line;
  unsigned Col;

  FontPtr = XboxFont8x16 + Char * 16;
  Pixel = (PULONG) ((char *) FrameBuffer + (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta
                  + X * CHAR_WIDTH * BytesPerPixel);
  for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
      Mask = 0x80;
      for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
          Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
          Mask = Mask >> 1;
        }
      Pixel = (PULONG) ((char *) Pixel + Delta);
    }
}

static ULONG
XboxVideoAttrToSingleColor(UCHAR Attr)
{
  UCHAR Intensity;

  Intensity = (0 == (Attr & 0x08) ? 127 : 255);

  return 0xff000000 |
         (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
         (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
         (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
XboxVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
  *FgColor = XboxVideoAttrToSingleColor(Attr & 0xf);
  *BgColor = XboxVideoAttrToSingleColor((Attr >> 4) & 0xf);
}

static VOID
XboxVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
  ULONG Line, Col;
  PULONG p;

  for (Line = 0; Line < ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
      p = (PULONG) ((char *) FrameBuffer + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
      for (Col = 0; Col < ScreenWidth; Col++)
        {
          *p++ = Color;
        }
    }
}

VOID
XboxVideoClearScreen(UCHAR Attr)
{
  ULONG FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoClearScreenColor(BgColor, FALSE);
}

VOID
XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  ULONG FgColor, BgColor;

  XboxVideoAttrToColors(Attr, &FgColor, &BgColor);

  XboxVideoOutputChar(Ch, X, Y, FgColor, BgColor);
}

VOID
XboxVideoInit(VOID)
{
  ULONG AvMode;

  FrameBuffer = (PVOID)((ULONG) XboxMemReserveMemory(FB_SIZE_MB) | 0xf0000000);

  if (I2CTransmitByteGetReturn(0x10, 0x04, &AvMode))
    {
      if (1 == AvMode) /* HDTV */
        {
          ScreenWidth = 720;
        }
      else
        {
          /* FIXME Other possible values of AvMode:
           * 0 - AV_SCART_RGB
           * 2 - AV_VGA_SOG
           * 4 - AV_SVIDEO
           * 6 - AV_COMPOSITE
           * 7 - AV_VGA
           * other AV_COMPOSITE
           */
          ScreenWidth = 640;
        }
    }
  else
    {
      ScreenWidth = 640;
    }

  ScreenHeight = 480;
  BytesPerPixel = 4;
  Delta = (ScreenWidth * BytesPerPixel + 3) & ~ 0x3;

  XboxVideoClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);

  /* Tell the nVidia controller about the framebuffer */
  *((PULONG) 0xfd600800) = (ULONG) FrameBuffer;
}

VIDEODISPLAYMODE
XboxVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
  /* We only have one mode, semi-text */
  return VideoTextMode;
}

VOID
XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  *Width = ScreenWidth / CHAR_WIDTH;
  *Height = (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
  *Depth = 0;
}

ULONG
XboxVideoGetBufferSize(VOID)
{
  return (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT * (ScreenWidth / CHAR_WIDTH) * 2;
}

VOID
XboxVideoSetTextCursorPosition(ULONG X, ULONG Y)
{
  /* We don't have a cursor yet */
}

VOID
XboxVideoHideShowTextCursor(BOOLEAN Show)
{
  /* We don't have a cursor yet */
}

VOID
XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  PUCHAR OffScreenBuffer = (PUCHAR) Buffer;
  ULONG Col, Line;

  for (Line = 0; Line < (ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
      for (Col = 0; Col < ScreenWidth / CHAR_WIDTH; Col++)
        {
          XboxVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
          OffScreenBuffer += 2;
        }
    }
}

BOOLEAN
XboxVideoIsPaletteFixed(VOID)
{
  return FALSE;
}

VOID
XboxVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
  /* Not supported */
}

VOID
XboxVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
{
  /* Not supported */
}

VOID
XboxVideoSync()
{
  /* Not supported */
}

VOID
XboxVideoPrepareForReactOS(IN BOOLEAN Setup)
{
  XboxVideoClearScreenColor(MAKE_COLOR(0, 0, 0), TRUE);
}

/* EOF */
