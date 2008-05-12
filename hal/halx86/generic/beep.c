/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/beep.c
 * PURPOSE:         Speaker support (beeping)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* CONSTANTS *****************************************************************/

#define TIMER2      (PUCHAR)0x42
#define TIMER3      (PUCHAR)0x43
#define PORT_B      (PUCHAR)0x61
#define CLOCKFREQ   1193167

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalMakeBeep(IN ULONG Frequency)
{
    UCHAR Data;
    ULONG Divider;
    BOOLEAN Result = TRUE;

    /* FIXME: Acquire CMOS Lock */

    /* Turn the register off */
    Data = READ_PORT_UCHAR(PORT_B);
    WRITE_PORT_UCHAR(PORT_B, Data & 0xFC);

    /* Check if we have a frequency */
    if (Frequency)
    {
        /* Set the divider */
        Divider = CLOCKFREQ / Frequency;

        /* Check if it's too large */
        if (Divider > 0x10000)
        {
            /* Fail */
            Result = FALSE;
            goto Cleanup;
        }

        /* Set timer divider */
        WRITE_PORT_UCHAR(TIMER3, 0xB6);
        WRITE_PORT_UCHAR(TIMER2, (UCHAR)(Divider & 0xFF));
        WRITE_PORT_UCHAR(TIMER2, (UCHAR)((Divider>>8) & 0xFF));

        /* Turn speaker on */
        WRITE_PORT_UCHAR(PORT_B, READ_PORT_UCHAR(PORT_B) | 0x03);
    }

Cleanup:
    /* FIXME: Release hardware lock */

    /* Return result */
    return Result;
}


