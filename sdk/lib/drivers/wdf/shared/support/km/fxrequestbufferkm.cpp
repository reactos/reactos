/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestBufferKm.cpp

Abstract:

    This module implements a memory union object

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxsupportpch.hpp"

extern "C" {
// #include "FxRequestBufferKm.tmh"
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
    PVOID pBuf;
    NTSTATUS status;
    ULONG length;
    BOOLEAN oldUnlockValue;

    pBuf = NULL;

    oldUnlockValue =  *UnlockWhenFreed;

    //
    // Format functions that use this helper call
    // FxRequestBase::ValidateTarget which calls ContextReleaseAndRestore
    // which unlocks any locked pages.
    //
    // Hence pages must already be unlocked now. Let's assert that.
    //
    // This condition needs to be true since we unconditionally set
    // *UnlockWhenFreed to FALSE just below.
    //
    ASSERT (oldUnlockValue == FALSE);

    *UnlockWhenFreed = FALSE;

    //
    // Even if ReuseMdl is not true, SizeOfMdl may be supplied to store
    // the size of allocated MDL to be used later
    //
    ASSERT(ReuseMdl ? (SizeOfMdl != NULL && *MdlToFree != NULL) : TRUE);

    switch (DataType) {
    case FxRequestBufferUnspecified:
        *Mdl = NULL;
        //
        // We should not set *MdlToFree to NULL as *MdlToFree might have a valid
        // MDL which we should not overwrite with NULL without freeing it
        //
        return STATUS_SUCCESS;

    case FxRequestBufferMemory:
        if (u.Memory.Offsets != NULL) {
            pBuf = WDF_PTR_ADD_OFFSET(u.Memory.Memory->GetBuffer(),
                                      u.Memory.Offsets->BufferOffset);
        }
        else {
            pBuf = u.Memory.Memory->GetBuffer();
        }
        //   ||   ||
        //   \/   \/  fall through

    case FxRequestBufferBuffer:
        if (pBuf == NULL) {
            pBuf = u.Buffer.Buffer;
        }

        length = GetBufferLength();
        status = GetOrAllocateMdlWorker(FxDriverGlobals,
                               Mdl,
                               &ReuseMdl,
                               length,
                               pBuf,
                               SizeOfMdl,
                               oldUnlockValue,
                               MdlToFree
                               );
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Couldn't allocate memory for MDL of length 0x%x %!STATUS!", length, status);
            return status;
        }

        //
        // If we are reusing the MDL we need to initialize it with current
        // buffer.
        //
        if (ReuseMdl == TRUE) {
            Mx::MxInitializeMdl(*Mdl, pBuf, length);
        }

        status = FxProbeAndLockWithAccess(*Mdl, KernelMode, Operation);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Couldn't lock pages for MDL 0x%p %!STATUS!", *Mdl, status);

            //
            // Free MDL only if it was not reused.
            //
            if (ReuseMdl == FALSE) {
                FxMdlFree(FxDriverGlobals, *Mdl);
            }

            *Mdl = NULL;
            return status;
        }

        *UnlockWhenFreed = TRUE;
        *MdlToFree = *Mdl;

        return STATUS_SUCCESS;

    case FxRequestBufferMdl:
        *Mdl = u.Mdl.Mdl;
        //
        // We should not set *MdlToFree to NULL as *MdlToFree might have a valid
        // MDL which we should not overwrite with NULL without freeing it
        //
        return STATUS_SUCCESS;

    case FxRequestBufferReferencedMdl:
        if (u.RefMdl.Offsets == NULL ||
            (u.RefMdl.Offsets->BufferOffset == 0 && u.RefMdl.Offsets->BufferLength == 0)) {
            *Mdl = u.RefMdl.Mdl;
            //
            // We should not set *MdlToFree to NULL as *MdlToFree might have a valid
            // MDL which we should not overwrite with NULL without freeing it
            //
        }
        else {
            //
            // Do not use MmGetSystemAddressForMdlSafe because StartVa could be
            // in UM while MappedVa (obviously) is in KM.   Since
            // IoBuildPartial Mdl basically uses
            // pBuf - MmGetMdlVirtualAddress(SrcMdl) to compute offset, if one
            // VA is in UM (e.g. MmGetMdlVirtualAddress(SrcMdl)), you get the
            // (drastically) wrong offset.
            //
            pBuf =  Mx::MxGetMdlVirtualAddress(u.RefMdl.Mdl);
            ASSERT(pBuf != NULL);

            pBuf = WDF_PTR_ADD_OFFSET(pBuf, u.RefMdl.Offsets->BufferOffset);

            //
            // GetBufferLength will compute the correct length with the given offsets
            //
            length = GetBufferLength();
            status = GetOrAllocateMdlWorker(FxDriverGlobals,
                                   Mdl,
                                   &ReuseMdl,
                                   length,
                                   pBuf,
                                   SizeOfMdl,
                                   oldUnlockValue,
                                   MdlToFree
                                   );
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "Couldn't allocate memory for MDL of length 0x%x %!STATUS!", length, status);
                return status;
            }

            Mx::MxBuildPartialMdl(
                u.RefMdl.Mdl,
                *Mdl,
                pBuf,
                length
                );

            *MdlToFree = *Mdl;
        }

        return STATUS_SUCCESS;

    default:
        *Mdl = NULL;
        //
        // We should not set *MdlToFree to NULL as *MdlToFree might have a valid
        // MDL which we should not overwrite with NULL without freeing it
        //
        return STATUS_INVALID_PARAMETER;
    }
}

VOID
FxRequestBuffer::SetMemory(
    __in IFxMemory* Memory,
    __in PWDFMEMORY_OFFSET Offsets
    )
{
    PMDL pMdl;

    pMdl = Memory->GetMdl();
    if (pMdl != NULL) {
        DataType = FxRequestBufferReferencedMdl;
        u.RefMdl.Memory = Memory;
        u.RefMdl.Offsets = Offsets;
        u.RefMdl.Mdl = pMdl;
    }
    else {
        DataType = FxRequestBufferMemory;
        u.Memory.Memory = Memory;
        u.Memory.Offsets = Offsets;
    }
}

