/* $Id: display.c,v 1.2 1999/10/15 15:18:39 ekohl Exp $
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
#include <internal/halio.h>


//#define BOCHS_DEBUGGING
//#define SERIAL_DEBUGGING
#define SCREEN_DEBUGGING


#define SERIAL_PORT 0x03f8
#define SERIAL_BAUD_RATE 19200
#define SERIAL_LINE_CONTROL (SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO)





#ifdef BOCHS_DEBUGGING
#define BOCHS_LOGGER_PORT (0x3ed)
#endif

#ifdef SERIAL_DEBUGGING
#define   SER_RBR   SERIAL_PORT + 0
#define   SER_THR   SERIAL_PORT + 0
#define   SER_DLL   SERIAL_PORT + 0
#define   SER_IER   SERIAL_PORT + 1
#define   SER_DLM   SERIAL_PORT + 1
#define   SER_IIR   SERIAL_PORT + 2
#define   SER_LCR   SERIAL_PORT + 3
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR   SERIAL_PORT + 4
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR   SERIAL_PORT + 5
#define     SR_LSR_TBE 0x20
#define   SER_MSR   SERIAL_PORT + 6
#endif

#ifdef SCREEN_DEBUGGING
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
#ifdef SERIAL_DEBUGGING
        /*  turn on DTR and RTS  */
        outb_p(SER_MCR, SR_MCR_DTR | SR_MCR_RTS);
        /*  set baud rate, line control  */
        outb_p(SER_LCR, SERIAL_LINE_CONTROL | SR_LCR_DLAB);
        outb_p(SER_DLL, (115200 / SERIAL_BAUD_RATE) & 0xff);
        outb_p(SER_DLM, ((115200 / SERIAL_BAUD_RATE) >> 8) & 0xff);
        outb_p(SER_LCR, SERIAL_LINE_CONTROL);
#endif

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
#ifdef SCREEN_DEBUGGING
    int offset;
#endif

    pch = String;

    if (HalOwnsDisplay == FALSE)
    {
        HalResetDisplay ();
    }

#ifdef BOCHS_DEBUGGING
    outb_p(BOCHS_LOGGER_PORT,c);
#endif

#ifdef SERIAL_DEBUGGING
    while ((inb_p(SER_LSR) & SR_LSR_TBE) == 0)
        ;

    outb_p(SER_THR, c);
#endif

#ifdef SCREEN_DEBUGGING
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

#ifdef SCREEN_DEBUGGING   
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
