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

static UCHAR ShadowBuffer[ROWS * COLS * 2];

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
  unsigned X, Y;

  if (Attr != CurrentAttr)
    {
      AnsiSetAttr(Attr);
    }
  AnsiClearScreen();
  for (Y = 0; Y < ROWS; Y++)
    {
      for (X = 0; X < COLS; X++)
        {
          ShadowBuffer[2 * (Y * COLS + X)] = (UCHAR) ' ';
          ShadowBuffer[2 * (Y * COLS + X) + 1] = Attr;
        }
    }
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

  /* Translate some line-drawing characters */
  if (0x80 <= Ch)
    {
      switch(Ch)
        {
        case 0xda: /* top-left single corner */
        case 0xbf: /* top-right single corner */
        case 0xc0: /* bottom-left single corner */
        case 0xd9: /* bottom-right single corner */
        case 0xc9: /* top-left double corner */
        case 0xbb: /* top-right double corner */
        case 0xc8: /* bottom-left double corner */
        case 0xbc: /* bottom-right double corner */
        case 0xd5: /* top-left single/double corner */
        case 0xb8: /* top-right single/double corner */
        case 0xd4: /* bottom-left single/double corner */
        case 0xbe: /* bottom-right single/double corner */
        case 0xd6: /* top-left single/double corner */
        case 0xb7: /* top-right single/double corner */
        case 0xd3: /* bottom-left single/double corner */
        case 0xbd: /* bottom-right single/double corner */
          Ch = '+';
          break;
        case 0xc4: /* horizontal single */
        case 0xcd: /* horizontal double */
          Ch = '-';
          break;
        case 0xb3: /* vertical single */
        case 0xba: /* vertical double */
          Ch = '|';
          break;
        case 0xb1: /* dotted pattern */
          Ch = ' ';
          break;
        case 0xdb: /* progress bar, completed */
          Ch = '*';
          break;
        case 0xb2: /* progress bar, to do */
          Ch = ' ';
          break;
        }
    }

  if ((UCHAR) Ch != ShadowBuffer[2 * (Y * COLS + X)]
      || Attr != ShadowBuffer[2 * (Y * COLS + X) + 1])
    {
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

      ShadowBuffer[2 * (Y * COLS + X)] = (UCHAR) Ch;
      ShadowBuffer[2 * (Y * COLS + X) + 1] = Attr;
    }
}

VIDEODISPLAYMODE
XenVideoSetDisplayMode(char *DisplayMode, BOOL Init)
{
  /* We only have one mode: text */
  return VideoTextMode;
}

VOID
XenVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  *Width = COLS;
  *Height = ROWS;
  *Depth = 0;
}

VOID
XenVideoHideShowTextCursor(BOOL Show)
{
  /* We can't hide the cursor */
}

ULONG
XenVideoGetBufferSize(VOID)
{
  return COLS * ROWS * 2;
}

VOID
XenVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  unsigned X, Y;
  PUCHAR BufPtr = (PUCHAR) Buffer;

  for (Y = 0; Y < ROWS; Y++)
    {
      for (X = 0; X < COLS; X++)
        {
          XenVideoPutChar(BufPtr[0], BufPtr[1], X, Y);
          BufPtr += 2;
        }
    }
  if (COLS - 1 != CurrentX || ROWS - 1 != CurrentY)
    {
      AnsiMoveToPos(COLS - 1, ROWS - 1);
    }

  XenConsFlush();
}
VOID
XenVideoSetTextCursorPosition(ULONG X, ULONG Y)
  {
    AnsiMoveToPos(X, Y);
  }

BOOL
XenVideoIsPaletteFixed(VOID)
  {
    return TRUE;
  }

VOID
XenVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
  {
    /* Should never be called */
  }

VOID
XenVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
  {
    /* Should never be called */
  }

VOID
XenVideoSync(VOID)
  {
    /* Nothing to do */
  }

/* EOF */
