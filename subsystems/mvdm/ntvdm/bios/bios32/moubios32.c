/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.c
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"

#include "moubios32.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/mouse.h"

// HACK: For the PS/2 bypass and MOUSE.COM driver direct call
#include "dos/mouse32.h"

/* PRIVATE VARIABLES **********************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

// Mouse IRQ 12
static VOID WINAPI BiosMouseIrq(LPWORD Stack)
{
    // HACK!! Call directly the MOUSE.COM driver instead of going
    // through the regular interfaces!!
    extern COORD DosNewPosition;
    extern WORD  DosButtonState;
    DosMouseUpdatePosition(&DosNewPosition);
    DosMouseUpdateButtons(DosButtonState);

    PicIRQComplete(Stack);
}

VOID BiosMousePs2Interface(LPWORD Stack)
{
    DPRINT1("INT 15h, AH = C2h must be implemented in order to support vendor mouse drivers\n");

    switch (getAL())
    {
        /* Enable / Disable */
        case 0x00:
        {
            break;
        }

        /* Reset */
        case 0x01:
        {
            break;
        }

        /* Set Sampling Rate */
        case 0x02:
        {
            break;
        }

        /* Set Resolution */
        case 0x03:
        {
            break;
        }

        /* Get Type */
        case 0x04:
        {
            break;
        }

        /* Initialize */
        case 0x05:
        {
            break;
        }

        /* Extended Commands */
        case 0x06:
        {
            break;
        }

        /* Set Device Handler Address */
        case 0x07:
        {
            break;
        }

        /* Write to Pointer Port */
        case 0x08:
        {
            break;
        }

        /* Read from Pointer Port */
        case 0x09:
        {
            break;
        }

        default:
        {
            DPRINT1("INT 15h, AH = C2h, AL = 0x%02X NOT IMPLEMENTED\n",
                    getAL());
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN MouseBios32Initialize(VOID)
{
    /* Set up the HW vector interrupts */
    EnableHwIRQ(12, BiosMouseIrq);
    return TRUE;
}

VOID MouseBios32Cleanup(VOID)
{
}
