/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdreboot.c

Abstract:

    This module implements the functions to support rebooting from
    the debugger.

Author:

    Joe Notarangelo 11-Mar-1993

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
