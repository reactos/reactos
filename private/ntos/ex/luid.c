/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    luid.c

Abstract:

    This module implements the NT locally unique identifier services.

Author:

    Jim Kelly (JimK) 7-June-1990

Revision History:

--*/

#include "exp.h"

//
//  Global variables needed to support locally unique IDs.
//

LARGE_INTEGER ExpLuid;
LARGE_INTEGER ExpLuidIncrement;
extern KSPIN_LOCK ExpLuidLock;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExLuidInitialization)
#pragma alloc_text(PAGE, NtAllocateLocallyUniqueId)
#endif

BOOLEAN
ExLuidInitialization (
    VOID
    )

/*++

Routine Description:

    This function initializes the locally unique identifier allocation.

    NOTE:  THE LUID ALLOCATION SERVICES ARE NEEDED BY SECURITY IN PHASE 0
           SYSTEM INITIALIZATION.  FOR THIS REASON, LUID INITIALIZATION IS
           PERFORMED AS PART OF PHASE 0 SECURITY INITIALIZATION.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is successfully
    completed.  Otherwise, a value of FALSE is returned.

--*/

{

    //
    // Initialize the LUID source value.
    //
    // The first 1000 values are reserved for static definition. This
    // value can be increased with later releases with no adverse impact.
    //
    // N.B. The LUID source always refers to the "next" allocatable LUID.
    //

    ExpLuid.LowPart = 1001;
    ExpLuid.HighPart = 0;
    KeInitializeSpinLock(&ExpLuidLock);
    ExpLuidIncrement.QuadPart = 1;
    return TRUE;
}

NTSTATUS
NtAllocateLocallyUniqueId (
    OUT PLUID Luid
    )

/*++

Routine Description:

    This function returns an LUID value that is unique since the system
    was last rebooted.  It is unique on the system it is generated on
    only (not network wide).

    There are no restrictions on who can allocate LUIDs.  The LUID space
    is large enough that this will never present a problem.  If one LUID
    is allocated every 100ns, they will not be exhausted for roughly
    15,000 years (100ns * 2^63).

Arguments:

    Luid - Supplies the address of a variable that will receive the
        new LUID.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        LUID cannot be written.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler and attempt to write the Luid
    // to the specified variable. If the write attempt fails, then return
    // the exception code as the service status. Otherwise return success
    // as the service status.
    //

    try {

        //
        // Get previous processor mode and probe argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWrite((PVOID)Luid, sizeof(LUID), sizeof(ULONG));
        }

        //
        // Allocate and store a locally unique Id.
        //

        ExAllocateLocallyUniqueId(Luid);

    } except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}
