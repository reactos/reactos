/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    raise.c

Abstract:

    This module implements routines to raise datatype misalignment and
    access violation for probe code.

    N.B. These routines are provided as function to save space in the
        probe macros.

    N.B. Since these routines are *only* called from the probe macros,
        it is assumed that the calling code is pageable.

Author:

    David N. Cutler (davec) 29-Apr-1995

Environment:

    Kernel mode.

Revision History:

--*/

#include "exp.h"

//
// Define function sections.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ExRaiseAccessViolation)
#pragma alloc_text(PAGE, ExRaiseDatatypeMisalignment)
#endif

VOID
ExRaiseAccessViolation (
    VOID
    )

/*++

Routine Description:

    This function raises an access violation exception.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    return;
}

VOID
ExRaiseDatatypeMisalignment (
    VOID
    )

/*++

Routine Description:

    This function raises a datatype misalignment exception.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ExRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    return;
}
