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

#include <hal.h>
#define NDEBUG
#include <debug.h>


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
NTAPI
HalMakeBeep (
	ULONG	Frequency
	)
{
    return TRUE;
}

