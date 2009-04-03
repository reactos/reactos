/* $Id: reboot.c 23907 2006-09-04 05:52:23Z arty $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/reboot.c
 * PURPOSE:         Reboot functions.
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 11/10/99
 */

#include <hal.h>
#define NDEBUG
#include <debug.h>

typedef void (*void_fun)();
static VOID
HalReboot (VOID)
{
    void_fun reset_vector = (void_fun)0xfff00100;
    reset_vector();
}


VOID NTAPI
HalReturnToFirmware (
	FIRMWARE_REENTRY	Action
	)
{
    if (Action == HalHaltRoutine)
    {
        DbgPrint ("HalReturnToFirmware called!\n");
        DbgBreakPoint ();
    }
    else if (Action == HalRebootRoutine)
    {
        HalReboot ();
    }
}

/* EOF */
