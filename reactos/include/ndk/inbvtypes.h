/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    inbvtypes.h

Abstract:

    Type definitions for the Boot Video Driver.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Created - 02-Feb-2007

--*/

#ifndef _INBVTYPES_H
#define _INBVTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#ifndef NTOS_MODE_USER

//
// Boot Video Display Ownership Status
//
typedef enum _INBV_DISPLAY_STATE
{
    INBV_DISPLAY_STATE_OWNED,
    INBV_DISPLAY_STATE_DISABLED,
    INBV_DISPLAY_STATE_LOST
} INBV_DISPLAY_STATE;

//
// Function Callbacks
//
typedef
BOOLEAN
(NTAPI *INBV_RESET_DISPLAY_PARAMETERS)(
    ULONG Cols,
    ULONG Rows
);

typedef
VOID
(NTAPI *INBV_DISPLAY_STRING_FILTER)(
    PCHAR *Str
);

#endif
#endif
