/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: display.c,v 1.13 2004/01/18 22:35:26 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/display.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 */

/*
 * Portions of this code are from the XFree86 Project and available from the
 * following license:
 *
 * Copyright (C) 1994-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
 * NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
*/  

/* DISPLAY OWNERSHIP
 *
 * So, who owns the physical display and is allowed to write to it?
 *
 * In MS NT, upon boot HAL owns the display. Somewhere in the boot
 * sequence (haven't figured out exactly where or by who), some
 * component calls HalAcquireDisplayOwnership. From that moment on,
 * the display is owned by that component and is switched to graphics
 * mode. The display is not supposed to return to text mode, except
 * in case of a bug check. The bug check will call HalDisplayString
 * to output a string to the text screen. HAL will notice that it
 * currently doesn't own the display and will re-take ownership, by
 * calling the callback function passed to HalAcquireDisplayOwnership.
 * After the bugcheck, execution is halted. So, under NT, the only
 * possible sequence of display modes is text mode -> graphics mode ->
 * text mode (the latter hopefully happening very infrequently).
 *
 * Things are a little bit different in the current state of ReactOS.
 * We want to have a functional interactive text mode. We should be
 * able to switch from text mode to graphics mode when a GUI app is
 * started and switch back to text mode when it's finished. Then, when
 * another GUI app is started, another switch to and from graphics mode
 * is possible. Also, when the system bugchecks in graphics mode we want
 * to switch back to text mode to show the registers and stack trace.
 * Last but not least, HalDisplayString is used a lot more in ReactOS,
 * e.g. to print debug messages when the /DEBUGPORT=SCREEN boot option
 * is present.
 * 3 Components are involved in Reactos: HAL, BLUE.SYS and VIDEOPRT.SYS.
 * As in NT, on boot HAL owns the display. When entering the text mode
 * command interpreter, BLUE.SYS kicks in. It will write directly to the
 * screen, more or less behind HALs back.
 * When a GUI app is started, WIN32K.SYS will open the DISPLAY device.
 * This open call will end up in VIDEOPRT.SYS. That component will then
 * take ownership of the display by calling HalAcquireDisplayOwnership.
 * When the GUI app terminates (WIN32K.SYS will close the DISPLAY device),
 * we want to give ownership of the display back to HAL. Using the
 * standard exported HAL functions, that's a bit of a problem, because
 * there is no function defined to do that. In NT, this is handled by
 * HalDisplayString, but that solution isn't satisfactory in ReactOS,
 * because HalDisplayString is (in some cases) also used to output debug
 * messages. If we do it the NT way, the first debug message output while
 * in graphics mode would switch the display back to text mode.
 * So, instead, if HalDisplayString detects that HAL doesn't have ownership
 * of the display, it doesn't do anything.
 * To return ownership to HAL, a new function is exported,
 * HalReleaseDisplayOwnership. This function is called by the DISPLAY
 * device Close routine in VIDEOPRT.SYS. It is also called at the beginning
 * of a bug check, so HalDisplayString is activated again.
 * Now, while the display is in graphics mode (not owned by HAL), BLUE.SYS
 * should also refrain from writing to the screen buffer. The text mode
 * screen buffer might overlap the graphics mode screen buffer, so changing
 * something in the text mode buffer might mess up the graphics screen. To
 * allow BLUE.SYS to detect if HAL owns the display, another new function is
 * exported, HalQueryDisplayOwnership. BLUE.SYS will call this function to
 * check if it's allowed to touch the text mode buffer.
 *
 * In an ideal world, when HAL takes ownership of the display, it should set
 * up the CRT using real-mode (actually V86 mode, but who cares) INT 0x10
 * calls. Unfortunately, this will require HAL to setup a real-mode interrupt
 * table etc. So, we chickened out of that by having the loader set up the
 * display before switching to protected mode. If HAL is given back ownership
 * after a GUI app terminates, the INT 0x10 calls are made by VIDEOPRT.SYS,
 * since there is already support for them via the VideoPortInt10 routine.
 */

#include <ddk/ntddk.h>
#include <mps.h>

#define SCREEN_SYNCHRONIZATION

#define VGA_AC_INDEX            0x3c0
#define VGA_AC_READ             0x3c1
#define VGA_AC_WRITE            0x3c0

#define VGA_MISC_WRITE          0x3c2

#define VGA_SEQ_INDEX           0x3c4
#define VGA_SEQ_DATA            0x3c5

#define VGA_DAC_MASK            0x3c6
#define VGA_DAC_READ_INDEX      0x3c7
#define VGA_DAC_WRITE_INDEX     0x3c8
#define VGA_DAC_DATA            0x3c9
#define VGA_FEATURE_READ        0x3ca
#define VGA_MISC_READ           0x3cc

#define VGA_GC_INDEX            0x3ce
#define VGA_GC_DATA             0x3cf

#define VGA_CRTC_INDEX          0x3d4
#define VGA_CRTC_DATA           0x3d5

#define VGA_INSTAT_READ         0x3da

#define VGA_SEQ_NUM_REGISTERS   5
#define VGA_CRTC_NUM_REGISTERS  25
#define VGA_GC_NUM_REGISTERS    9
#define VGA_AC_NUM_REGISTERS    21

#define CRTC_COLUMNS       0x01
#define CRTC_OVERFLOW      0x07
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09

#define CRTC_CURHI         0x0e
#define CRTC_CURLO         0x0f


#define CHAR_ATTRIBUTE_BLACK  0x00  /* black on black */
#define CHAR_ATTRIBUTE        0x17  /* grey on blue */

#define FONT_AMOUNT        (8*8192)

/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX = 80;       /* Display size */
static ULONG SizeY = 25;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static WORD *VideoBuffer = NULL;
static PUCHAR GraphVideoBuffer = NULL;

static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;

static UCHAR SavedTextPalette[768];
static UCHAR SavedTextMiscOutReg;
static UCHAR SavedTextCrtcReg[VGA_CRTC_NUM_REGISTERS];
static UCHAR SavedTextAcReg[VGA_AC_NUM_REGISTERS];
static UCHAR SavedTextGcReg[VGA_GC_NUM_REGISTERS];
static UCHAR SavedTextSeqReg[VGA_SEQ_NUM_REGISTERS];
static UCHAR SavedTextFont[2][FONT_AMOUNT];
static BOOL TextPaletteEnabled = FALSE;

/* PRIVATE FUNCTIONS *********************************************************/

VOID FASTCALL
HalClearDisplay (UCHAR CharAttribute)
{
   WORD *ptr = (WORD*)VideoBuffer;
   ULONG i;

  for (i = 0; i < SizeX * SizeY; i++, ptr++)
    *ptr = ((CharAttribute << 8) + ' ');

  CursorX = 0;
  CursorY = 0;
}


/* STATIC FUNCTIONS *********************************************************/

VOID STATIC
HalScrollDisplay (VOID)
{
  WORD *ptr;
  int i;

  ptr = VideoBuffer + SizeX;
  RtlMoveMemory(VideoBuffer,
		ptr,
		SizeX * (SizeY - 1) * 2);

  ptr = VideoBuffer  + (SizeX * (SizeY - 1));
  for (i = 0; i < (int)SizeX; i++, ptr++)
    {
      *ptr = (CHAR_ATTRIBUTE << 8) + ' ';
    }
}

VOID STATIC FASTCALL
HalPutCharacter (CHAR Character)
{
  WORD *ptr;

  ptr = VideoBuffer + ((CursorY * SizeX) + CursorX);
  *ptr = (CHAR_ATTRIBUTE << 8) + Character;
}

VOID STATIC
HalDisablePalette(VOID)
{
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, 0x20);
  TextPaletteEnabled = FALSE;
}

VOID STATIC
HalEnablePalette(VOID)
{
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, 0x00);
  TextPaletteEnabled = TRUE;
}

UCHAR STATIC FASTCALL
HalReadGc(ULONG Index)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_GC_INDEX, (UCHAR)Index);
  return(READ_PORT_UCHAR((PUCHAR)VGA_GC_DATA));
}

VOID STATIC FASTCALL
HalWriteGc(ULONG Index, UCHAR Value)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_GC_INDEX, (UCHAR)Index);
  WRITE_PORT_UCHAR((PUCHAR)VGA_GC_DATA, Value);
}

UCHAR STATIC FASTCALL
HalReadSeq(ULONG Index)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_SEQ_INDEX, (UCHAR)Index);
  return(READ_PORT_UCHAR((PUCHAR)VGA_SEQ_DATA));
}

VOID STATIC FASTCALL
HalWriteSeq(ULONG Index, UCHAR Value)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_SEQ_INDEX, (UCHAR)Index);
  WRITE_PORT_UCHAR((PUCHAR)VGA_SEQ_DATA, Value);
}

VOID STATIC FASTCALL
HalWriteAc(ULONG Index, UCHAR Value)
{
  if (TextPaletteEnabled)
    {
      Index &= ~0x20;
    }
  else
    {
      Index |= 0x20;
    }
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, (UCHAR)Index);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_WRITE, Value);
}

UCHAR STATIC FASTCALL
HalReadAc(ULONG Index)
{
  if (TextPaletteEnabled)
    {
      Index &= ~0x20;
    }
  else
    {
      Index |= 0x20;
    }
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  WRITE_PORT_UCHAR((PUCHAR)VGA_AC_INDEX, (UCHAR)Index);
  return(READ_PORT_UCHAR((PUCHAR)VGA_AC_READ));
}

VOID STATIC FASTCALL
HalWriteCrtc(ULONG Index, UCHAR Value)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, (UCHAR)Index);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, Value);
}

UCHAR STATIC FASTCALL
HalReadCrtc(ULONG Index)
{
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, (UCHAR)Index);
  return(READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA));
}

VOID STATIC FASTCALL
HalResetSeq(BOOL Start)
{
  if (Start)
    {
      HalWriteSeq(0x00, 0x01);
    }
  else
    {
      HalWriteSeq(0x00, 0x03);
    }
}

VOID STATIC FASTCALL
HalBlankScreen(BOOL On)
{
  UCHAR Scrn;

  Scrn = HalReadSeq(0x01);

  if (On)
    {
      Scrn &= ~0x20;
    }
  else
    {
      Scrn |= 0x20;
    }

  HalResetSeq(TRUE);
  HalWriteSeq(0x01, Scrn);
  HalResetSeq(FALSE);
}

VOID STATIC
HalSaveFont(VOID)
{
  UCHAR Attr10;
  UCHAR MiscOut, Gc4, Gc5, Gc6, Seq2, Seq4;
  ULONG i;

  /* Check if we are already in graphics mode. */
  Attr10 = HalReadAc(0x10);
  if (Attr10 & 0x01)
    {
      return;
    }

  /* Save registers. */
  MiscOut = READ_PORT_UCHAR((PUCHAR)VGA_MISC_READ);
  Gc4 = HalReadGc(0x04);
  Gc5 = HalReadGc(0x05);
  Gc6 = HalReadGc(0x06);
  Seq2 = HalReadSeq(0x02);
  Seq4 = HalReadSeq(0x04);

  /* Force colour mode. */
  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, (UCHAR)(MiscOut | 0x01));

  HalBlankScreen(FALSE);

  for (i = 0; i < 2; i++)
    {
      /* Save font 1 */
      HalWriteSeq(0x02, (UCHAR)(0x04 << i)); /* Write to plane 2 or 3 */
      HalWriteSeq(0x04, 0x06); /* Enable plane graphics. */
      HalWriteGc(0x04, (UCHAR)(0x02 + i)); /* Read plane 2 or 3 */
      HalWriteGc(0x05, 0x00); /* Write mode 0; read mode 0 */
      HalWriteGc(0x06, 0x05); /* Set graphics. */
      memcpy(SavedTextFont[i], GraphVideoBuffer, FONT_AMOUNT);
    }

  /* Restore registers. */
  HalWriteAc(0x10, Attr10);
  HalWriteSeq(0x02, Seq2);
  HalWriteSeq(0x04, Seq4);
  HalWriteGc(0x04, Gc4);
  HalWriteGc(0x05, Gc5);
  HalWriteGc(0x06, Gc6);
  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, MiscOut);

  HalBlankScreen(TRUE);
}

VOID STATIC
HalSaveMode(VOID)
{
  ULONG i;

  SavedTextMiscOutReg = READ_PORT_UCHAR((PUCHAR)VGA_MISC_READ);

  for (i = 0; i < VGA_CRTC_NUM_REGISTERS; i++)
    {
      SavedTextCrtcReg[i] = HalReadCrtc(i);
    }

  HalEnablePalette();
  for (i = 0; i < VGA_AC_NUM_REGISTERS; i++)
    {
      SavedTextAcReg[i] = HalReadAc(i);
    }
  HalDisablePalette();

  for (i = 0; i < VGA_GC_NUM_REGISTERS; i++)
    {
      SavedTextGcReg[i] = HalReadGc(i);
    }

  for (i = 0; i < VGA_SEQ_NUM_REGISTERS; i++)
    {
      SavedTextSeqReg[i] = HalReadSeq(i);
    }
}

VOID STATIC
HalDacDelay(VOID)
{
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
  (VOID)READ_PORT_UCHAR((PUCHAR)VGA_INSTAT_READ);
}

VOID STATIC
HalSavePalette(VOID)
{
  ULONG i;
  WRITE_PORT_UCHAR((PUCHAR)VGA_DAC_MASK, 0xFF);
  WRITE_PORT_UCHAR((PUCHAR)VGA_DAC_READ_INDEX, 0x00);
  for (i = 0; i < 768; i++)
    {
      SavedTextPalette[i] = READ_PORT_UCHAR((PUCHAR)VGA_DAC_DATA);
      HalDacDelay();
    }
}

VOID STATIC
HalRestoreFont(VOID)
{
  UCHAR MiscOut, Attr10, Gc1, Gc3, Gc4, Gc5, Gc6, Gc8;
  UCHAR Seq2, Seq4;
  ULONG i;

  /* Save registers. */
  MiscOut = READ_PORT_UCHAR((PUCHAR)VGA_MISC_READ);
  Attr10 = HalReadAc(0x10);
  Gc1 = HalReadGc(0x01);
  Gc3 = HalReadGc(0x03);
  Gc4 = HalReadGc(0x04);
  Gc5 = HalReadGc(0x05);
  Gc6 = HalReadGc(0x06);
  Gc8 = HalReadGc(0x08);
  Seq2 = HalReadSeq(0x02);
  Seq4 = HalReadSeq(0x04);

  /* Force into colour mode. */
  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, (UCHAR)(MiscOut | 0x10));

  HalBlankScreen(FALSE);

  HalWriteGc(0x03, 0x00);  /* Don't rotate; write unmodified. */
  HalWriteGc(0x08, 0xFF);  /* Write all bits. */
  HalWriteGc(0x01, 0x00);  /* All planes from CPU. */

  for (i = 0; i < 2; i++)
    {
      HalWriteSeq(0x02, (UCHAR)(0x04 << i)); /* Write to plane 2 or 3 */
      HalWriteSeq(0x04, 0x06); /* Enable plane graphics. */
      HalWriteGc(0x04, (UCHAR)(0x02 + i)); /* Read plane 2 or 3 */
      HalWriteGc(0x05, 0x00); /* Write mode 0; read mode 0. */
      HalWriteGc(0x06, 0x05); /* Set graphics. */
      memcpy(GraphVideoBuffer, SavedTextFont[i], FONT_AMOUNT);
    }

  HalBlankScreen(TRUE);

  /* Restore registers. */
  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, MiscOut);
  HalWriteAc(0x10, Attr10);
  HalWriteGc(0x01, Gc1);
  HalWriteGc(0x03, Gc3);
  HalWriteGc(0x04, Gc4);
  HalWriteGc(0x05, Gc5);
  HalWriteGc(0x06, Gc6);
  HalWriteGc(0x08, Gc8);
  HalWriteSeq(0x02, Seq2);
  HalWriteSeq(0x04, Seq4);
}

VOID STATIC
HalRestoreMode(VOID)
{
  ULONG i;

  WRITE_PORT_UCHAR((PUCHAR)VGA_MISC_WRITE, SavedTextMiscOutReg);

  for (i = 1; i < VGA_SEQ_NUM_REGISTERS; i++)
    {
      HalWriteSeq(i, SavedTextSeqReg[i]);
    }

  /* Unlock CRTC registers 0-7 */
  HalWriteCrtc(17, (UCHAR)(SavedTextCrtcReg[17] & ~0x80));

  for (i = 0; i < VGA_CRTC_NUM_REGISTERS; i++)
    {
      HalWriteCrtc(i, SavedTextCrtcReg[i]);
    }

  for (i = 0; i < VGA_GC_NUM_REGISTERS; i++)
    {
      HalWriteGc(i, SavedTextGcReg[i]);
    }

  HalEnablePalette();
  for (i = 0; i < VGA_AC_NUM_REGISTERS; i++)
    {
      HalWriteAc(i, SavedTextAcReg[i]);
    }
  HalDisablePalette();
}

VOID STATIC
HalRestorePalette(VOID)
{
  ULONG i;
  WRITE_PORT_UCHAR((PUCHAR)VGA_DAC_MASK, 0xFF);
  WRITE_PORT_UCHAR((PUCHAR)VGA_DAC_WRITE_INDEX, 0x00);
  for (i = 0; i < 768; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)VGA_DAC_DATA, SavedTextPalette[i]);
      HalDacDelay();
    }
  HalDisablePalette();
}

/* PRIVATE FUNCTIONS ********************************************************/

VOID FASTCALL
HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Initalize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
  if (DisplayInitialized == FALSE)
    {
      ULONG ScanLines;
      ULONG Data;

      VideoBuffer = (WORD *)(0xd0000000 + 0xb8000);
      GraphVideoBuffer = (PUCHAR)(0xd0000000 + 0xa0000);
//      VideoBuffer = HalMapPhysicalMemory (0xb8000, 2);

      /* Set cursor position */
//      CursorX = LoaderBlock->cursorx;
//      CursorY = LoaderBlock->cursory;
      CursorX = 0;
      CursorY = 0;

      /* read screen size from the crtc */
      /* FIXME: screen size should be read from the boot parameters */
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_COLUMNS);
      SizeX = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA) + 1;
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_ROWS);
      SizeY = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_OVERFLOW);
      Data = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
      SizeY |= (((Data & 0x02) << 7) | ((Data & 0x40) << 3));
      SizeY++;
      WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_SCANLINES);
      ScanLines = (READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA) & 0x1F) + 1;
      SizeY = SizeY / ScanLines;

#ifdef BOCHS_30ROWS
      SizeY=30;
#endif
      HalClearDisplay(CHAR_ATTRIBUTE_BLACK);

      DisplayInitialized = TRUE;

      /* 
	 Save the VGA state at this point so we can restore it on a bugcheck.
      */
      HalSavePalette();
      HalSaveMode();
      HalSaveFont();
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID STDCALL
HalReleaseDisplayOwnership()
/*
 * FUNCTION: Release ownership of display back to HAL
 */
{
  if (HalResetDisplayParameters == NULL)
    return;

  if (HalOwnsDisplay == TRUE)
    return;

  if (!HalResetDisplayParameters(SizeX, SizeY))
    {
      HalRestoreMode();
      HalRestoreFont();
      HalRestorePalette();
    }
  HalOwnsDisplay = TRUE;
  HalClearDisplay(CHAR_ATTRIBUTE);
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
#ifdef SCREEN_SYNCHRONIZATION
  int offset;
#endif
  static KSPIN_LOCK Lock;
  ULONG Flags;

  /* See comment at top of file */
  if (! HalOwnsDisplay)
    {
      return;
    }

  pch = String;

  pushfl(Flags);

#if defined(__GNUC__)
  __asm__ ("cli\n\t");
#elif defined(_MSC_VER)
  __asm cli
#else
#error Unknown compiler for inline assembler
#endif

  KiAcquireSpinLock(&Lock);

#if 0  
  if (HalOwnsDisplay == FALSE)
    {
      HalReleaseDisplayOwnership();
    }
#endif
  
#ifdef SCREEN_SYNCHRONIZATION
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURHI);
  offset = READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA)<<8;
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURLO);
  offset += READ_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA);
  
  CursorY = offset / SizeX;
  CursorX = offset % SizeX;
#endif
  
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
	  HalPutCharacter (*pch);
	  CursorX++;
	  
	  if (CursorX >= SizeX)
	    {
	      CursorY++;
	      CursorX = 0;
	    }
	}
  
      if (CursorY >= SizeY)
	{
	  HalScrollDisplay ();
	  CursorY = SizeY - 1;
	}
  
      pch++;
    }
  
#ifdef SCREEN_SYNCHRONIZATION
  offset = (CursorY * SizeX) + CursorX;
  
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURLO);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, (UCHAR)(offset & 0xff));
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_INDEX, CRTC_CURHI);
  WRITE_PORT_UCHAR((PUCHAR)VGA_CRTC_DATA, (UCHAR)((offset >> 8) & 0xff));
#endif
  KiReleaseSpinLock(&Lock);
  popfl(Flags);
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
HalQueryDisplayOwnership()
{
  return ! HalOwnsDisplay;
}

/* EOF */
