/*++

  Copyright (c) Microsoft Corporation

  Module Name:

  FxRequestBufferKm.hpp

  Abstract:

  This module implements km specific functions for FxRequestBuffer.

  Author:



  Environment:

  Kernel mode only

  Revision History:

  --*/

#ifndef _FXREQUESTBUFFERKM_HPP_
#define _FXREQUESTBUFFERKM_HPP_

__inline
VOID
FxRequestBuffer::SetMdl(
    __in PMDL Mdl,
    __in ULONG Length
    )
{
    DataType = FxRequestBufferMdl;
    u.Mdl.Mdl = Mdl;
    u.Mdl.Length = Length;
}

FORCEINLINE
NTSTATUS
FxRequestBuffer::GetOrAllocateMdlWorker(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __deref_out PMDL*       Mdl,
    __in BOOLEAN *          ReuseMdl,
    __in LONG               Length,
    __in PVOID              Buffer,
    __inout size_t*         SizeOfMdl,
    __in BOOLEAN            UnlockWhenFreed,
    __deref_out_opt PMDL*   MdlToFree
        )
{
    size_t sizeofCurrentMdl;
    sizeofCurrentMdl = MmSizeOfMdl(Buffer, Length);

    //
    // Caller of this function (GetOrAllocateMdl) verifies that pages
    // are already unlocked. Asserting here, in case we start using this
    // function elsewhere.
    //
    // This is why we don't unlock pages either in reuse or non-reuse case.
    //
    ASSERT(UnlockWhenFreed == FALSE);
     UNREFERENCED_PARAMETER(UnlockWhenFreed); //for fre build

    //
    // If ReuseMdl is TRUE then the Mdl to be reused is passed in.
    //
    if (*ReuseMdl && sizeofCurrentMdl <= *SizeOfMdl) {
        MmPrepareMdlForReuse(*MdlToFree);
        *Mdl = *MdlToFree;
    }
    else {
        *ReuseMdl = FALSE;

        //
        // Since *Mdl may have the original IRP Mdl
        // free *MdlToFree and not *Mdl
        //
        if (*MdlToFree != NULL) {
            FxMdlFree(FxDriverGlobals, *MdlToFree);
            *MdlToFree = NULL;
             if (SizeOfMdl != NULL) {
                *SizeOfMdl = 0;
            }
        }

        *Mdl = FxMdlAllocate(FxDriverGlobals,
                             NULL, // owning FxObject
                             Buffer,
                             Length,
                             FALSE,
                             FALSE);

        if (*Mdl == NULL) {

            ASSERT(SizeOfMdl ? (*SizeOfMdl == 0) : TRUE);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (SizeOfMdl != NULL) {
            *SizeOfMdl  = sizeofCurrentMdl;
        }
    }

    return STATUS_SUCCESS;
}


#endif // _FXREQUESTBUFFERKM_HPP_
