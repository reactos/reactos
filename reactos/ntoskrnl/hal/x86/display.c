/* $Id: display.c,v 1.4 1999/12/12 03:48:47 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/display.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 */

#include <ddk/ntddk.h>
#include <internal/hal.h>

/* only needed for screen synchronization */
#include <internal/halio.h>


#define SCREEN_SYNCHRONIZATION


#ifdef SCREEN_SYNCHRONIZATION
#define CRTC_COMMAND 0x3d4
#define CRTC_DATA 0x3d5
#define CRTC_CURLO 0x0f
#define CRTC_CURHI 0x0e
#endif


#define CHAR_ATTRIBUTE 0x17  /* grey on blue */


/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX = 80;       /* Display size */
static ULONG SizeY = 50;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static WORD *VideoBuffer = NULL;

static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;


/* STATIC FUNCTIONS *********************************************************/

static VOID
HalClearDisplay (VOID)
{
    WORD *ptr = (WORD*)VideoBuffer;
    ULONG i;

    for (i = 0; i < SizeX * SizeY; i++, ptr++)
        *ptr = ((CHAR_ATTRIBUTE << 8) + ' ');

    CursorX = 0;
    CursorY = 0;
}


VOID
HalScrollDisplay (VOID)
{
   WORD *ptr;
   int i;

   ptr = VideoBuffer + SizeX;
   RtlMoveMemory (VideoBuffer,
           ptr,
           SizeX * (SizeY - 1) * 2);

   ptr = VideoBuffer  + (SizeX * (SizeY - 1));
   for (i = 0; i < SizeX; i++, ptr++)
   {
       *ptr = (CHAR_ATTRIBUTE << 8) + ' ';
   }
}


static VOID
HalPutCharacter (CHAR Character)
{
   WORD *ptr;

   ptr = VideoBuffer + ((CursorY * SizeX) + CursorX);
   *ptr = (CHAR_ATTRIBUTE << 8) + Character;
}


/* PRIVATE FUNCTIONS ********************************************************/

VOID
HalInitializeDisplay (boot_param *bp)
/*
 * FUNCTION: Initalize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
    if (DisplayInitialized == FALSE)
    {
        VideoBuffer = (WORD *)(0xd0000000 + 0xb8000);
//        VideoBuffer = HalMapPhysicalMemory (0xb8000, 2);

        /* Set cursor position */
        CursorX = bp->cursorx;
        CursorY = bp->cursory;
        SizeX = 80;
        SizeY = 50;

        HalClearDisplay ();

        DisplayInitialized = TRUE;
    }
}


VOID
HalResetDisplay (VOID)
/*
 * FUNCTION: Reset the display
 * ARGUMENTS:
 *         None
 */
{
    if (HalResetDisplayParameters == NULL)
        return;

    if (HalOwnsDisplay == TRUE)
        return;

    if (HalResetDisplayParameters(SizeX, SizeY) == TRUE)
    {
        HalOwnsDisplay = TRUE;
        HalClearDisplay ();
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID
HalAcquireDisplayOwnership (
        IN  PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
        )
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


VOID
HalDisplayString(PCH String)
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

    pch = String;

    if (HalOwnsDisplay == FALSE)
    {
        HalResetDisplay ();
    }

#ifdef SCREEN_SYNCHRONIZATION
    outb_p(CRTC_COMMAND, CRTC_CURHI);
    offset = inb_p(CRTC_DATA)<<8;
    outb_p(CRTC_COMMAND, CRTC_CURLO);
    offset += inb_p(CRTC_DATA);

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
        else
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

    outb_p(CRTC_COMMAND, CRTC_CURLO);
    outb_p(CRTC_DATA, offset);
    outb_p(CRTC_COMMAND, CRTC_CURHI);
    offset >>= 8;
    outb_p(CRTC_DATA, offset);
#endif
}


VOID
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
HalSetDisplayParameters (ULONG CursorPosX,
                         ULONG CursorPosY)
{
    CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
    CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}

/* EOF */
