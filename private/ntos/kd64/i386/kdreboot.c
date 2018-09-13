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

    John Vert (jvert) 03-Jun-1991
        Tweaked magic address to skip memory check on reboot.

    John Vert (jvert) 13-Aug-1991
        Code moved to HalReturnToFirmware, which this calls.

--*/

#include "kdp.h"

#define CMOS_CTRL   (PUCHAR )0x70
#define CMOS_DATA   (PUCHAR )0x71

#define RESET       0xfe
#define KEYBPORT    (PUCHAR )0x64


VOID
KdpReboot (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpReboot)
#endif

VOID
KdpReboot (
    VOID
    )

/*++

Routine Description:

    Just calls the HalReturnToFirmware function.

Arguments:

    None

Return Value:

    Does not return

--*/

{
    //
    // Never returns from HAL
    //

    KeReturnToFirmware(HalRebootRoutine);
}

