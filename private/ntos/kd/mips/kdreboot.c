/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdreboot.c

Abstract:

    System reboot function.  Currently part of the debugger because
    that's the only place it's used.

Author:

    Bryan M. Willman (bryanwi) 4-Dec-90

Revision History:

--*/

#include "kdp.h"

VOID
KdpReboot (
    VOID
    )

/*++

Routine Description:

    Reboot the system via the Hal.

--*/

{
    HalReturnToFirmware(HalRebootRoutine);
}
