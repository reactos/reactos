/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestUm.cpp

Abstract:

    This module implements KM specific FxRequest object routines

Author:



Environment:

    User mode only

Revision History:


--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxRequestUm.tmh"
}

VOID
FxRequest::CheckAssumptions(
    VOID
    )
/*++

Routine Description:
    This routine is never actually called by running code, it just has
    WDFCASSERTs who upon failure, would not allow this file to be compiled.

    DO NOT REMOVE THIS FUNCTION just because it is not called by any running
    code.

Arguments:
    None

Return Value:
    None

  --*/
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}


_Must_inspect_result_
NTSTATUS
FxRequest::GetMdl(
    __out PMDL *pMdl
    )
{
    UNREFERENCED_PARAMETER(pMdl);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}


_Must_inspect_result_
NTSTATUS
FxRequest::GetDeviceControlOutputMdl(
    __out PMDL *pMdl
    )
/*++

Routine Description:

    Return the IRP_MJ_DEVICE_CONTROL OutputBuffer as an MDL

    The MDL is automatically released when the request is completed.

    The MDL is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or for any request other than IRP_MJ_DEVICE_CONTROL.

    The MDL is as follows for each buffering mode:

    METHOD_BUFFERED:

        MmBuildMdlForNonPagedPool(IoAllocateMdl(Irp->UserBuffer, ... ))

    METHOD_IN_DIRECT:

        Irp->MdlAddress

    METHOD_OUT_DIRECT:

        Irp->MdlAddress

    METHOD_NEITHER:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The MDL is only valid until the request is completed.

Arguments:

    pMdl- Pointer location to return MDL ptr

Returns:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(pMdl);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxRequest::ProbeAndLockForRead(
    __in  PVOID Buffer,
    __in  ULONG Length,
    __deref_out FxRequestMemory** MemoryObject
    )

/*++

Routine Description:

    Probe and lock a memory buffer for reading.

    This is to be called in the proper process context, and
    will generate an FxRequestMemory object if successful.

    The FxRequestMemory object will be associated with the FxRequest
    object, and is automatically released when the FxRequest is completed.

    This function performs validation to ensure that the current
    thread is in the same process as the thread that originated
    the I/O request.

Arguments:


    Buffer - Buffer to lock down

    Length - Length of buffer

    MemoryObject - FxRequestMemory object to return

Returns:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(MemoryObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxRequest::ProbeAndLockForWrite(
    __in  PVOID Buffer,
    __in  ULONG Length,
    __deref_out FxRequestMemory** MemoryObject
    )

/*++

Routine Description:

    Probe and lock a memory buffer for writing.

    This is to be called in the proper process context, and
    will generate an FxRequestMemory object if successful.

    The FxRequestMemory object will be associated with the FxRequest
    object, and is automatically released when the FxRequest is completed.

    This function performs validation to ensure that the current
    thread is in the same process as the thread that originated
    the I/O request.

Arguments:


    Buffer - Buffer to lock down

    Length - Length of buffer

    MemoryObject - FxRequestMemory object to return

Returns:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(MemoryObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FxRequest::Impersonate(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_ PFN_WDF_REQUEST_IMPERSONATE EvtRequestImpersonate,
    _In_opt_ PVOID Context
    )
{
    DDI_ENTRY();

    NTSTATUS status;
    HRESULT hr;
    MdIrp irp;

    status = GetIrp(&irp);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST is already completed 0x%p, %!STATUS!",
                            GetHandle(), status);
        FxVerifierDbgBreakPoint(GetDriverGlobals());
        return status;
    }

    hr = irp->ImpersonateRequestorProcess(ImpersonationLevel);

    if (SUCCEEDED(hr)) {
        status = STATUS_SUCCESS;
        EvtRequestImpersonate(GetHandle(), Context);
        irp->RevertImpersonation();
    }
    else {
        status = GetDevice()->NtStatusFromHr(hr);
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST 0x%p, Could not impersonate, %!STATUS!",
                            GetHandle(), status);
    }

    return status;
}


