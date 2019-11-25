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
#include <section_attribs.h>

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
    _In_ INBV_RESET_DISPLAY_PARAMETERS Callback
);

//
// Installation Functions
//
VOID
NTAPI
InbvEnableBootDriver(
    _In_ BOOLEAN Enable
);

VOID
NTAPI
InbvInstallDisplayStringFilter(
    _In_ INBV_DISPLAY_STRING_FILTER DisplayFilter
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
    _In_ PCHAR String
);

BOOLEAN
NTAPI
InbvEnableDisplayString(
    _In_ BOOLEAN Enable
);

BOOLEAN
NTAPI
InbvResetDisplay(
    VOID
);

VOID
NTAPI
InbvSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom
);

VOID
NTAPI
InbvSetTextColor(
    _In_ ULONG Color
);

VOID
NTAPI
InbvSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ ULONG Color
);

#endif
#endif
