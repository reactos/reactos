/* $Id: display_xbox.c,v 1.1 2004/12/04 21:43:37 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/xbox/display_xbox.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 *                  Modified for Xbox 2004/12/02 GvG
 */

/* For an explanation about display ownership see generic/display.c */

#include <ddk/ntddk.h>
#include <hal.h>
#include "halxbox.h"

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

/* Default to grey on blue */
#define DEFAULT_FG_COLOR MAKE_COLOR(127, 127, 127)
#define DEFAULT_BG_COLOR MAKE_COLOR(0, 0, 127)

/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX;            /* Display size (characters) */
static ULONG SizeY;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

static PVOID FrameBuffer;
static ULONG BytesPerPixel;
static ULONG Delta;

/* PRIVATE FUNCTIONS *********************************************************/

static VOID FASTCALL
HalpXboxOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
  PUCHAR FontPtr;
  PULONG Pixel;
  UCHAR Mask;
  unsigned Line;
  unsigned Col;

  FontPtr = XboxFont8x16 + Char * 16;
  Pixel = (PULONG) ((char *) FrameBuffer + Y * CHAR_HEIGHT * Delta
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

static ULONG FASTCALL
HalpXboxAttrToSingleColor(UCHAR Attr)
{
  UCHAR Intensity;

  Intensity = (0 == (Attr & 0x08) ? 127 : 255);

  return 0xff000000 |
         (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
         (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
         (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID FASTCALL
HalpXboxAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
  *FgColor = HalpXboxAttrToSingleColor(Attr & 0xf);
  *BgColor = HalpXboxAttrToSingleColor((Attr >> 4) & 0xf);
}

static VOID FASTCALL
HalpXboxClearScreenColor(ULONG Color)
{
  ULONG Line, Col;
  PULONG p;

  for (Line = 0; Line < SizeY * CHAR_HEIGHT; Line++)
    {
      p = (PULONG) ((char *) FrameBuffer + Line * Delta);
      for (Col = 0; Col < SizeX * CHAR_WIDTH; Col++)
        {
          *p++ = Color;
        }
    }
}

VOID FASTCALL
HalClearDisplay(UCHAR CharAttribute)
{
  ULONG FgColor, BgColor;

  HalpXboxAttrToColors(CharAttribute, &FgColor, &BgColor);

  HalpXboxClearScreenColor(BgColor);

  CursorX = 0;
  CursorY = 0;
}

VOID STATIC
HalScrollDisplay (VOID)
{
  ULONG Line, Col;
  PULONG p;

  p = (PULONG) ((char *) FrameBuffer + (Delta * CHAR_HEIGHT));
  RtlMoveMemory(FrameBuffer,
		p,
		(Delta * CHAR_HEIGHT) * (SizeY - 1));

  for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
      p = (PULONG) ((char *) FrameBuffer + (CHAR_HEIGHT * (SizeY - 1 ) + Line) * Delta);
      for (Col = 0; Col < SizeX * CHAR_WIDTH; Col++)
        {
          *p++ = DEFAULT_BG_COLOR;
        }
    }
}

static VOID FASTCALL
HalPutCharacter(UCHAR Character)
{
  HalpXboxOutputChar(Character, CursorX, CursorY, DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
}

VOID FASTCALL
HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Initalize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
  ULONG ScreenWidthPixels;
  ULONG ScreenHeightPixels;
  PHYSICAL_ADDRESS PhysBuffer;

  if (! DisplayInitialized)
    {
      PhysBuffer.u.HighPart = 0;
      if (0 != (LoaderBlock->Flags & MB_FLAGS_MEM_INFO))
        {
          PhysBuffer.u.LowPart = (LoaderBlock->MemHigher + 1024) * 1024;
        }
      else
        {
          /* Assume a 64Mb Xbox */
          PhysBuffer.u.LowPart = 0x03c00000;
        }
      PhysBuffer.u.LowPart |= 0xf0000000;
      FrameBuffer = MmMapIoSpace(PhysBuffer, 0x400000, MmNonCached);
      if (NULL == FrameBuffer)
        {
          return;
        }
      ScreenWidthPixels = 720;
      ScreenHeightPixels = 480;
      BytesPerPixel = 4;

      SizeX = ScreenWidthPixels / CHAR_WIDTH;
      SizeY = ScreenHeightPixels / CHAR_HEIGHT;
      Delta = (ScreenWidthPixels * BytesPerPixel + 3) & ~ 0x3;

      HalpXboxClearScreenColor(MAKE_COLOR(0, 0, 0));

      DisplayInitialized = TRUE;
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID STDCALL
HalReleaseDisplayOwnership(VOID)
/*
 * FUNCTION: Release ownership of display back to HAL
 */
{
  if (HalOwnsDisplay || NULL == HalResetDisplayParameters)
    {
      return;
    }

  HalResetDisplayParameters(SizeX, SizeY);

  HalOwnsDisplay = TRUE;
  HalpXboxClearScreenColor(DEFAULT_BG_COLOR);
}


VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
/*
 * FUNCTION: 
 * ARGUMENTS:
 *         ResetDisplayParameters = Pointer to a driver specific
 *         reset routine.
 */
{
  HalOwnsDisplay = FALSE;
  HalResetDisplayParameters = ResetDisplayParameters;
}

VOID STDCALL
HalDisplayString(IN PCH String)
/*
 * FUNCTION: Switches the screen to HAL console mode (BSOD) if not there
 * already and displays a string
 * ARGUMENT:
 *        string = ASCII string to display
 * NOTE: Use with care because there is no support for returning from BSOD
 * mode
 */
{
  PCH pch;
  static KSPIN_LOCK Lock;
  KIRQL OldIrql;
  ULONG Flags;

  if (! HalOwnsDisplay || ! DisplayInitialized)
    {
      return;
    }

  pch = String;

  OldIrql = KfRaiseIrql(HIGH_LEVEL);
  KiAcquireSpinLock(&Lock);

  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();

  while (*pch != 0)
    {
      if (*pch == '\n')
	{
	  CursorY++;
	  CursorX = 0;
	}
      else if (*pch == '\b')
	{
	  if (CursorX > 0)
	    {
	      CursorX--;
	    }
	}
      else if (*pch != '\r')
	{
	  HalPutCharacter(*pch);
	  CursorX++;
	  
	  if (SizeX <= CursorX)
	    {
	      CursorY++;
	      CursorX = 0;
	    }
	}

      if (SizeY <= CursorY)
	{
	  HalScrollDisplay ();
	  CursorY = SizeY - 1;
	}
  
      pch++;
    }
  
  Ki386RestoreFlags(Flags);

  KiReleaseSpinLock(&Lock);
  KfLowerIrql(OldIrql);
}

VOID STDCALL
HalQueryDisplayParameters(OUT PULONG DispSizeX,
			  OUT PULONG DispSizeY,
			  OUT PULONG CursorPosX,
			  OUT PULONG CursorPosY)
{
  if (DispSizeX)
    *DispSizeX = SizeX;
  if (DispSizeY)
    *DispSizeY = SizeY;
  if (CursorPosX)
    *CursorPosX = CursorX;
  if (CursorPosY)
    *CursorPosY = CursorY;
}


VOID STDCALL
HalSetDisplayParameters(IN ULONG CursorPosX,
			IN ULONG CursorPosY)
{
  CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
  CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}


BOOLEAN STDCALL
HalQueryDisplayOwnership(VOID)
{
  return ! HalOwnsDisplay;
}

/* EOF */
