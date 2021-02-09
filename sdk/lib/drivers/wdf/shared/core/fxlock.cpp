/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxLock.cpp

Abstract:

    This module contains the implementation of FxLock

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

VOID
FxLock::Initialize(
    __in FxObject * ParentObject
    )
/*++

Routine Description:
    This is called to initialize the verifier with the object type so it can
    track lock order and sequence.

Arguments:
    ParentObject - the owning object

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = ParentObject->GetDriverGlobals();

    if (pFxDriverGlobals->FxVerifierLock) {
        //
        // Allocation failure is not fatal, we just won't track anything
        //

        (void) FxVerifierLock::CreateAndInitialize(&m_Verifier,
                                                   pFxDriverGlobals,
                                                   ParentObject);
    }
}
