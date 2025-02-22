/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/reboot.c
 * PURPOSE:         Reboot Function
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* PUBLIC FUNCTIONS **********************************************************/

#ifndef _MINIHAL_
/*
 * @implemented
 */
VOID
NTAPI
HalReturnToFirmware(
    _In_ FIRMWARE_REENTRY Action)
{
    /* Check what kind of action this is */
    switch (Action)
    {
        /* All recognized actions */
        case HalHaltRoutine:
        case HalPowerDownRoutine:
        case HalRestartRoutine:
        case HalRebootRoutine:
        {
            /* Acquire the display */
            InbvAcquireDisplayOwnership();
            // TODO: Reboot
        }

        /* Anything else */
        default:
        {
            /* Print message and break */
            DbgPrint("HalReturnToFirmware called!\n");
            DbgBreakPoint();
        }
    }
}
#endif // _MINIHAL_

/* EOF */
