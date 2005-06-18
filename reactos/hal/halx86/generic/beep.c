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

#define NDEBUG
#include <hal.h>


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
STDCALL
HalMakeBeep (
	ULONG	Frequency
	)
{
    UCHAR b;
    ULONG flags;
   
    /* save flags and disable interrupts */
    Ki386SaveFlags(flags);
    Ki386DisableInterrupts();

    /* speaker off */
    b = READ_PORT_UCHAR((PUCHAR)PORT_B);
    WRITE_PORT_UCHAR((PUCHAR)PORT_B, (UCHAR)(b & 0xFC));

    if (Frequency)
    {
        DWORD Divider = CLOCKFREQ / Frequency;

        if (Divider > 0x10000)
        {
            /* restore flags */
            Ki386RestoreFlags(flags);

            return FALSE;
        }

        /* set timer divider */
        WRITE_PORT_UCHAR((PUCHAR)TIMER3, 0xB6);
        WRITE_PORT_UCHAR((PUCHAR)TIMER2, (UCHAR)(Divider & 0xFF));
        WRITE_PORT_UCHAR((PUCHAR)TIMER2, (UCHAR)((Divider>>8) & 0xFF));

        /* speaker on */
        WRITE_PORT_UCHAR((PUCHAR)PORT_B, (UCHAR)(READ_PORT_UCHAR((PUCHAR)PORT_B) | 0x03));
    }

    /* restore flags */
    Ki386RestoreFlags(flags);

    return TRUE;
}

