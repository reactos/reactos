/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.c
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "moubios32.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/mouse.h"
#include "hardware/ps2.h"

// HACK: For the PS/2 bypass and MOUSE.COM driver direct call
#include "dos/mouse32.h"

/* PRIVATE VARIABLES **********************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

// Mouse IRQ 12
static VOID WINAPI BiosMouseIrq(LPWORD Stack)
{
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
    BYTE ControllerConfig;

    /* Clear the mouse queue */
    while (PS2PortQueueRead(1)) continue;

    /* Enable packet reporting */
    IOWriteB(PS2_CONTROL_PORT, 0xD4);
    IOWriteB(PS2_DATA_PORT, 0xF4);

    /* Read the mouse ACK reply */
    PS2PortQueueRead(1);

    /* Enable IRQ12 */
    IOWriteB(PS2_CONTROL_PORT, 0x20);
    ControllerConfig = IOReadB(PS2_DATA_PORT);
    IOWriteB(PS2_CONTROL_PORT, 0x60);
    IOWriteB(PS2_DATA_PORT, ControllerConfig | 0x02);

    /* Set up the HW vector interrupts */
    EnableHwIRQ(12, BiosMouseIrq);

    return TRUE;
}

VOID MouseBios32Cleanup(VOID)
{
}
