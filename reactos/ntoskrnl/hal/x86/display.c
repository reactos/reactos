/* $Id: display.c,v 1.1 1999/10/11 20:50:32 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/display.c
 * PURPOSE:         Text mode display functions (blue screen)
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 */

#include <ddk/ntddk.h>
#include <internal/hal.h>


/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX = 80;       /* Display size */
static ULONG SizeY = 50;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static BYTE *VideoBuffer = NULL;

static RESET_DISPLAY_ROUTINE ResetRoutine = NULL;


/* STATIC FUNCTIONS *********************************************************/

static VOID
HalClearDisplay (VOID)
{
    WORD *ptr = (WORD*)VideoBuffer;
    ULONG i;

    for (i=0; i < SizeX*SizeY; i++, ptr++)
        *ptr = 0x1720;
}



/* PRIVATE FUNCTIONS ********************************************************/

VOID
HalInitializeDisplay (VOID)
{
    if (DisplayInitialized == FALSE)
    {
        DisplayInitialized = TRUE;

        VideoBuffer = (BYTE *)(0xd0000000 + 0xb8000);
//        VideoBuffer = HalMapPhysicalMemory (0xb8000, 2);

        HalClearDisplay ();
    }
}


/* PUBLIC FUNCTIONS *********************************************************/


VOID
STDCALL
HalAcquireDisplayOwnership (RESET_DISPLAY_ROUTINE ResetDisplay)
{
    HalOwnsDisplay = FALSE;
    ResetRoutine = ResetDisplay;
}


VOID
STDCALL
HalDisplayString (VOID)
{


}


VOID
STDCALL
HalQueryDisplayParameters (PULONG DispSizeX,
                           PULONG DispSizeY,
                           PULONG CursorPosX,
                           PULONG CursorPosY)
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


VOID
STDCALL
HalSetDisplayParameters (ULONG CursorPosX,
                         ULONG CursorPosY)
{
    CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
    CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}

/* EOF */
