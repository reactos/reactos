/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/beep.c
 * PURPOSE:         Speaker function (it's only one)
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 31/01/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#include <internal/debug.h>
#include <internal/i386/io.h>

/* CONSTANTS *****************************************************************/

#define TIMER2     0x42
#define TIMER3     0x43
#define PORT_B     0x61
#define CLOCKFREQ  1193167


/* FUNCTIONS *****************************************************************/
/*
 * FUNCTION: Beeps the speaker.
 * ARGUMENTS:
 *       Frequency = If 0, the speaker will be switched off, otherwise
 *                   the speaker beeps with the specified frequency.
 */

BOOLEAN
HalMakeBeep(ULONG Frequency)
{
    /* save flags and disable interrupts */
    __asm__("pushf\n\t" \
            "cli\n\t");

    /* speaker off */
    outb_p(PORT_B, inb_p(PORT_B) & 0xFC);

    if (Frequency)
    {
        DWORD Divider = CLOCKFREQ / Frequency;

        if (Divider > 0x10000)
        {
            /* restore flags */
            __asm__("popf\n\t");

            return FALSE;
        }

        /* set timer divider */
        outb_p(TIMER3, 0xB6);
        outb_p(TIMER2, (UCHAR)(Divider & 0xFF));
        outb_p(TIMER2, (UCHAR)((Divider>>8) & 0xFF));

        /* speaker on */
        outb_p(PORT_B, inb_p(PORT_B) | 0x03);
    }

    /* restore flags */
    __asm__("popf\n\t");

    return TRUE;
}

