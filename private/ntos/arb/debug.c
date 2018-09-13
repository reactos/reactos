/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains support routines for the Pnp resource arbiters.

Author:

    Andrew Thornton (andrewth) 19-June-1998


Environment:

    Kernel mode

Revision History:

--*/

#include "arbp.h"


//
// Debugging support
//

#if ARB_DBG

//
// Debug print level:
//    -1 = no messages
//     0 = vital messages only
//     1 = call trace
//     2 = verbose messages
//

LONG ArbDebugLevel = -1;

//
// ArbStopOnError works just like a debug level variable except
// instead of controlling whether a message is printed, it controls
// whether we breakpoint on an error or not.  Likewise ArbReplayOnError
// controls if we replay fail arbitrations so we can debug them.
//

ULONG ArbStopOnError;
ULONG ArbReplayOnError;

PCHAR ArbpActionStrings[] = {
    "ArbiterActionTestAllocation",
    "ArbiterActionRetestAllocation",
    "ArbiterActionCommitAllocation",
    "ArbiterActionRollbackAllocation",
    "ArbiterActionQueryAllocatedResources",
    "ArbiterActionWriteReservedResources",
    "ArbiterActionQueryConflict",
    "ArbiterActionQueryArbitrate",
    "ArbiterActionAddReserved",
    "ArbiterActionBootAllocation"
};

VOID
ArbDumpArbiterRange(
    LONG Level,
    PRTL_RANGE_LIST List,
    PUCHAR RangeText
    )

/*++

Routine Description:

    This dumps the contents of a range list to the debugger.

Parameters:

    Level     - The debug level at or above which the data should be displayed.
    List      - The range list to be displayed.
    RangeText - Informative text to go with the display.

Return Value:

    None

--*/

{
    PRTL_RANGE current;
    RTL_RANGE_LIST_ITERATOR iterator;
    BOOLEAN headerDisplayed = FALSE;

    PAGED_CODE();

    FOR_ALL_RANGES(List, &iterator, current) {

        if (headerDisplayed == FALSE) {
            headerDisplayed = TRUE;
            ARB_PRINT(Level, ("  %s:\n", RangeText));
        }

        ARB_PRINT(Level,
                    ("    %I64x-%I64x %s%s O=0x%08x U=0x%08x\n",
                    current->Start,
                    current->End,
                    current->Flags & RTL_RANGE_SHARED ? "S" : " ",
                    current->Flags & RTL_RANGE_CONFLICT ? "C" : " ",
                    current->Owner,
                    current->UserData
                   ));
    }
    if (headerDisplayed == FALSE) {
        ARB_PRINT(Level, ("  %s: <None>\n", RangeText));
    }
}

VOID
ArbDumpArbiterInstance(
    LONG Level,
    PARBITER_INSTANCE Arbiter
    )

/*++

Routine Description:

    This dumps the state of the arbiter to the debugger.

Parameters:

    Level - The debug level at or above which the data should be displayed.

    Arbiter - The arbiter instance to display

Return Value:

    None

--*/

{

    PAGED_CODE();

    ARB_PRINT(Level,
                ("---%S Arbiter State---\n",
                Arbiter->Name
                ));

    ArbDumpArbiterRange(
        Level,
        Arbiter->Allocation,
        "Allocation"
        );

    ArbDumpArbiterRange(
        Level,
        Arbiter->PossibleAllocation,
        "PossibleAllocation"
        );
}

VOID
ArbDumpArbitrationList(
    LONG Level,
    PLIST_ENTRY ArbitrationList
    )

/*++

Routine Description:

    Display the contents of an arbitration list.  That is, the
    set of resources (possibilities) we are trying to get.

Parameters:

    Level           - The debug level at or above which the data
                      should be displayed.
    ArbitrationList - The arbitration list to be displayed.

Return Value:

    None

--*/

{
    PARBITER_LIST_ENTRY current;
    PIO_RESOURCE_DESCRIPTOR alternative;
    PDEVICE_OBJECT previousOwner = NULL;
    UCHAR andOr = ' ';

    PAGED_CODE();

    ARB_PRINT(Level, ("Arbitration List\n"));

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        if (previousOwner != current->PhysicalDeviceObject) {

            previousOwner = current->PhysicalDeviceObject;

            ARB_PRINT(
                Level,
                ("  Owning object 0x%08x\n",
                current->PhysicalDeviceObject
                ));
            ARB_PRINT(
                Level,
                ("    Length  Alignment   Minimum Address - Maximum Address\n"
                ));

        }

        FOR_ALL_IN_ARRAY(current->Alternatives,
                         current->AlternativeCount,
                         alternative) {

            ARB_PRINT(
                Level,
                ("%c %8x   %8x  %08x%08x - %08x%08x  %s\n",
                andOr,
                alternative->u.Generic.Length,
                alternative->u.Generic.Alignment,
                alternative->u.Generic.MinimumAddress.HighPart,
                alternative->u.Generic.MinimumAddress.LowPart,
                alternative->u.Generic.MaximumAddress.HighPart,
                alternative->u.Generic.MaximumAddress.LowPart,
                alternative->Type == CmResourceTypeMemory ?
                  "Memory"
                : "Port"
                ));
            andOr = '|';
        }
        andOr = '&';
    }
}

#endif // ARB_DBG
