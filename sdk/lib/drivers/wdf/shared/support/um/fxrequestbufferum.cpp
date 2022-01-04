/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestBufferUm.cpp

Abstract:

    This module implements a memory union object

Author:



Environment:

    User mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxRequestBufferUm.tmh"
}

_Must_inspect_result_
NTSTATUS
FxRequestBuffer::GetOrAllocateMdl(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __deref_out_opt PMDL*   Mdl,
    __inout PMDL*           MdlToFree,
    __inout PBOOLEAN        UnlockWhenFreed,
    __in LOCK_OPERATION     Operation,
    __in BOOLEAN            ReuseMdl,
    __inout_opt size_t*     SizeOfMdl
    )
/*++

Routine Description:

    This function attempts to reuse the passed-in MDL (if any) or allocates
    a new MDL if reuse flag isn't passed-in or if the existing MDL isn't
    big enough.

Arguments:

Return Value:
    FxDriverGlobals - Driver globals

    Mdl - on return it contains the MDL allocated/reused

    MdlToFree - pointer to any MDL
                  * to be reused, if the size is <= current size, or
                  * freed and set to newly allocated MDL

    UnlockWhenFreed - whether to unlock pages when freeing MDL
                      (if FALSE, MDL may represent just MDL buffer but the pages
                      might have already been unlocked)

    Operation - Operation to pass to MmLockPages

    ReuseMdl - whether to reuse *MdlToFree
               Please note that this can be FALSE even when MDL is supplied

    SizeOfMdl - on input contains size of *MdlToFree,
                on return contains size of *Mdl

Remarks:

    *MdlToFree is modified only when this function frees the passed in MDL
    Otherwise it leaves it untouched. Caller is responsible for storing
    properly initialized value and/or freeing what's stored in the value.

  --*/
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(MdlToFree);
    UNREFERENCED_PARAMETER(UnlockWhenFreed);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(ReuseMdl);
    UNREFERENCED_PARAMETER(SizeOfMdl);

    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxRequestBuffer::SetMemory(
    __in IFxMemory* Memory,
    __in PWDFMEMORY_OFFSET Offsets
    )
{
    DataType = FxRequestBufferMemory;
    u.Memory.Memory = Memory;
    u.Memory.Offsets = Offsets;
}

