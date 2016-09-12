/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/firmware/fwutil.c
 * PURPOSE:         Boot Library Firmware Utility Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

VOID
BlFwReboot (
    VOID
    )
{
#ifdef BL_KD_SUPPORTED
    /* Stop the boot debugger*/
    BlBdStop();
#endif

    /* Reset the machine */
    EfiResetSystem(EfiResetCold);
}

