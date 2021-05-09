/*++

  Copyright (c) Microsoft Corporation

  Module Name:

  FxRequestBufferUm.hpp

  Abstract:

  This module implements um specific functions for FxRequestBuffer.

  Author:



  Environment:

  User mode only

  Revision History:

  --*/

#ifndef _FXREQUESTBUFFERUM_HPP_
#define _FXREQUESTBUFFERUM_HPP_

__inline
VOID
FxRequestBuffer::SetMdl(
    __in PMDL Mdl,
    __in ULONG Length
    )
{
    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(Length);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
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
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    UNREFERENCED_PARAMETER(Mdl);
    UNREFERENCED_PARAMETER(ReuseMdl);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(SizeOfMdl);
    UNREFERENCED_PARAMETER(UnlockWhenFreed);
    UNREFERENCED_PARAMETER(MdlToFree);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_UNSUCCESSFUL;
}


#endif // _FXREQUESTBUFFERUM_HPP_
