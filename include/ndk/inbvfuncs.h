/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    inbvfuncs.h

Abstract:

    Function definitions for the Boot Video Driver.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _INBVFUNCS_H
#define _INBVFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <inbvtypes.h>

#ifndef NTOS_MODE_USER
//
// Ownership Functions
//
VOID
NTAPI
InbvAcquireDisplayOwnership(
    VOID
);

BOOLEAN
NTAPI
InbvCheckDisplayOwnership(
    VOID
);

VOID
NTAPI
InbvNotifyDisplayOwnershipLost(
    IN INBV_RESET_DISPLAY_PARAMETERS Callback
);

//
// Installation Functions
//
VOID
NTAPI
InbvEnableBootDriver(
    IN BOOLEAN Enable
);

VOID
NTAPI
InbvInstallDisplayStringFilter(
    IN INBV_DISPLAY_STRING_FILTER DisplayFilter
);

BOOLEAN
NTAPI
InbvIsBootDriverInstalled(
    VOID
);

//
// Display Functions
//
BOOLEAN
NTAPI
InbvDisplayString(
    IN PCHAR String
);

BOOLEAN
NTAPI
InbvEnableDisplayString(
    IN BOOLEAN Enable
);

BOOLEAN
NTAPI
InbvResetDisplay(
    VOID
);

VOID
NTAPI
InbvSetScrollRegion(
    IN ULONG Left,
    IN ULONG Top,
    IN ULONG Width,
    IN ULONG Height
);

VOID
NTAPI
InbvSetTextColor(
    IN ULONG Color
);

VOID
NTAPI
InbvSolidColorFill(
    IN ULONG Left,
    IN ULONG Top,
    IN ULONG Width,
    IN ULONG Height,
    IN ULONG Color
);

VOID
NTAPI
InbvSetProgressBarSubset(
    IN ULONG Floor,
    IN ULONG Ceiling
);

#endif
#endif
