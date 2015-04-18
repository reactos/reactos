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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/display.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl
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
 * 3 Components are involved in ReactOS: HAL, BLUE.SYS and VIDEOPRT.SYS.
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

#include <hal.h>
#include <ppcboot.h>
#include <ppcdebug.h>

#define NDEBUG
#include <debug.h>

boot_infos_t PpcEarlybootInfo;

#define SCREEN_SYNCHRONIZATION

/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX = 80;       /* Display size */
static ULONG SizeY = 25;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;
static ULONG GraphVideoBuffer = 0;
static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;

extern UCHAR XboxFont8x16[];
extern void SetPhys( ULONG Addr, ULONG Data );
extern ULONG GetPhys( ULONG Addr );
extern void SetPhysByte( ULONG Addr, ULONG Data );

/* PRIVATE FUNCTIONS *********************************************************/

VOID FASTCALL
HalClearDisplay (UCHAR CharAttribute)
{
   ULONG i;
   ULONG deviceSize =
       PpcEarlybootInfo.dispDeviceRowBytes *
       PpcEarlybootInfo.dispDeviceRect[3];
   for(i = 0; i < deviceSize; i += sizeof(int) )
       SetPhys(GraphVideoBuffer + i, CharAttribute);

   CursorX = 0;
   CursorY = 0;
}


/* STATIC FUNCTIONS *********************************************************/

VOID STATIC
HalScrollDisplay (VOID)
{
    ULONG i, deviceSize =
	PpcEarlybootInfo.dispDeviceRowBytes *
	PpcEarlybootInfo.dispDeviceRect[3];
    ULONG Dest = (ULONG)GraphVideoBuffer,
	Src = (ULONG)(GraphVideoBuffer + (16 * PpcEarlybootInfo.dispDeviceRowBytes));
    ULONG End  = (ULONG)
	GraphVideoBuffer +
	(PpcEarlybootInfo.dispDeviceRowBytes *
	 (PpcEarlybootInfo.dispDeviceRect[3]-16));

    while( Src < End )
    {
	SetPhys((ULONG)Dest, GetPhys(Src));
	Src += 4; Dest += 4;
    }

    /* Clear the bottom row */
    for(i = End; i < deviceSize; i += sizeof(int) )
	SetPhys(GraphVideoBuffer + i, 1);
}

VOID STATIC FASTCALL
HalPutCharacter (CHAR Character)
{
    WRITE_PORT_UCHAR((PVOID)0x3f8, Character);
#if 0
    int i,j,k;
    ULONG Dest =
	(GraphVideoBuffer +
	 (16 * PpcEarlybootInfo.dispDeviceRowBytes * CursorY) +
	 (8 * (PpcEarlybootInfo.dispDeviceDepth / 8) * CursorX)), RowDest;
    UCHAR ByteToPlace;

    for( i = 0; i < 16; i++ ) {
	RowDest = Dest;
	for( j = 0; j < 8; j++ ) {
	    ByteToPlace = ((128 >> j) & (XboxFont8x16[(16 * Character) + i])) ? 0xff : 1;
	    for( k = 0; k < PpcEarlybootInfo.dispDeviceDepth / 8; k++, RowDest++ ) {
		SetPhysByte(RowDest, ByteToPlace);
	    }
	}
	Dest += PpcEarlybootInfo.dispDeviceRowBytes;
    }
#endif
}

/* PRIVATE FUNCTIONS ********************************************************/

VOID FASTCALL
HalInitializeDisplay (PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Initialize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
    if (! DisplayInitialized)
    {
      boot_infos_t *XBootInfo = (boot_infos_t *)LoaderBlock->ArchExtra;
      GraphVideoBuffer = (ULONG)XBootInfo->dispDeviceBase;
      memcpy(&PpcEarlybootInfo, XBootInfo, sizeof(*XBootInfo));

      /* Set cursor position */
      CursorX = 0;
      CursorY = 0;

      SizeX = XBootInfo->dispDeviceRowBytes / XBootInfo->dispDeviceDepth;
      SizeY = XBootInfo->dispDeviceRect[3] / 16;

      HalClearDisplay(1);

      DisplayInitialized = TRUE;
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID NTAPI
HalReleaseDisplayOwnership(VOID)
/*
 * FUNCTION: Release ownership of display back to HAL
 */
{
  if (HalResetDisplayParameters == NULL)
    return;

  if (HalOwnsDisplay == TRUE)
    return;

  HalOwnsDisplay = TRUE;
  HalClearDisplay(0);
}


VOID NTAPI
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

VOID NTAPI
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
  //static KSPIN_LOCK Lock;
  KIRQL OldIrql;
  BOOLEAN InterruptsEnabled = __readmsr();

  /* See comment at top of file */
  if (! HalOwnsDisplay || ! DisplayInitialized)
    {
      return;
    }

  pch = String;

  KeRaiseIrql(HIGH_LEVEL, &OldIrql);
  //KiAcquireSpinLock(&Lock);

  _disable();

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

  __writemsr(InterruptsEnabled);

  //KiReleaseSpinLock(&Lock);
  KeLowerIrql(OldIrql);
}

VOID NTAPI
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


VOID NTAPI
HalSetDisplayParameters(IN ULONG CursorPosX,
			IN ULONG CursorPosY)
{
  CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
  CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}


BOOLEAN NTAPI
HalQueryDisplayOwnership(VOID)
{
  return !HalOwnsDisplay;
}

/* EOF */
