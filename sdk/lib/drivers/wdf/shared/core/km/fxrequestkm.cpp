/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestKm.cpp

Abstract:

    This module implements KM specific FxRequest object routines

Author:



Environment:

    Kernel mode only

Revision History:


--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxRequestKm.tmh"
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
    PWDF_REQUEST_PARAMETERS pWdfRequestParameters = NULL;
    PIO_STACK_LOCATION pIoStackLocation = NULL;

    UNREFERENCED_PARAMETER(pWdfRequestParameters);
    UNREFERENCED_PARAMETER(pIoStackLocation);

    //
    // FxRequest::GetParameters relies on these being exactly the same size
    //
    WDFCASSERT(sizeof(pWdfRequestParameters->Parameters) ==
               sizeof(pIoStackLocation->Parameters));

    //
    // The address of the offset in the structure needs to be 8 bit aligned
    // so that we can store flags in the bottom 3 bits
    //
    WDFCASSERT((FIELD_OFFSET(FxRequest, m_SystemBufferOffset) & FxHandleFlagMask) == 0);
    WDFCASSERT((FIELD_OFFSET(FxRequest, m_OutputBufferOffset) & FxHandleFlagMask) == 0);

    WDFCASSERT(FIELD_OFFSET(IO_STACK_LOCATION,  Parameters.Read.Length) ==
               FIELD_OFFSET(IO_STACK_LOCATION,  Parameters.Write.Length));
}


_Must_inspect_result_
NTSTATUS
FxRequest::GetMdl(
    __out PMDL *pMdl
    )
{
    WDF_DEVICE_IO_TYPE ioType;
    ULONG              length;
    PVOID              pBuffer;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    KIRQL irql;
    PMDL pNewMdl;
    UCHAR majorFunction;

    pBuffer = NULL;
    length = 0;
    pFxDriverGlobals = GetDriverGlobals();

    Lock(&irql);

    // Verifier
    if (pFxDriverGlobals->FxVerifierIO) {

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if(!NT_SUCCESS(status)) {
            Unlock(irql);
            return status;
        }
    }

    //
    // See if we have an IRP_MJ_DEVICE_CONTROL or IRP_MJ_INTERNAL_DEVICE_CONTROL
    //
    majorFunction = m_Irp.GetMajorFunction();

    if( (majorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) ) {

        switch (m_Irp.GetParameterIoctlCodeBufferMethod()) {
        case METHOD_BUFFERED:
            pBuffer = m_Irp.GetSystemBuffer();
            length = m_Irp.GetParameterIoctlInputBufferLength();

            // Fall through to common buffer handler
            break;

        case METHOD_IN_DIRECT:
            //
            // InputBuffer is in SystemBuffer
            // OutputBuffer is in MdlAddress with read access
            //
            pBuffer = m_Irp.GetSystemBuffer();
            length = m_Irp.GetParameterIoctlInputBufferLength();

            // Fall through to common buffer handler
            break;

        case METHOD_OUT_DIRECT:
            //
            // InputBuffer is in SystemBuffer
            // OutputBuffer is in MdlAddress with write access
            //
            pBuffer = m_Irp.GetSystemBuffer();
            length = m_Irp.GetParameterIoctlInputBufferLength();

            // Fall through to common buffer handler

            break;

        case METHOD_NEITHER:
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Attempt to get UserMode MDL for METHOD_NEITHER DeviceControl 0x%x, "
                "WDFDEVICE 0x%p, WDFREQUEST 0x%p, %!STATUS!",
                m_Irp.GetParameterIoctlCode(),
                GetDevice()->GetHandle(), GetHandle(), status);

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver must use METHOD_BUFFERED or METHOD_xx_DIRECT "
                "I/O for this call, or use "
                "WdfDeviceInitSetIoInCallerContextCallback to probe "
                "and lock user mode memory");

            *pMdl = NULL;
            Unlock(irql);

            return status;
        }
    }
    else {
        //
        // It's a non-Device Control request, we must know the devices I/O type in
        // order to safely return the correct buffer
        //
        ioType = GetDevice()->GetIoType();

        if (ioType == WdfDeviceIoBuffered) {

            pBuffer = m_Irp.GetSystemBuffer();

            //
            // Must get the length depending on the request type code
            //
            if (majorFunction == IRP_MJ_READ) {
                length = m_Irp.GetParameterReadLength();
            }
            else if (majorFunction == IRP_MJ_WRITE) {
                length = m_Irp.GetParameterWriteLength();
            }
            else {
                //
                // Report the error, we can support other IRP_MJ_xxx codes are required
                // later
                //
                status = STATUS_INVALID_PARAMETER;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST %p no Automatic MDL mapping for buffered request"
                    "major function 0x%x available for WDFDEVICE 0x%p, %!STATUS!",
                    GetHandle(), majorFunction, GetDevice()->GetHandle(),
                    status);

                *pMdl = NULL;

                Unlock(irql);

                return status;
            }

            // Fall through to common buffer handler
        }
        else if (ioType == WdfDeviceIoDirect) {

            //
            // If driver has used the default DO_DIRECT_IO I/O Mgr has done all the work
            //
            *pMdl = m_Irp.GetMdl();

            if (*pMdl == NULL) {
                status = STATUS_BUFFER_TOO_SMALL;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p for a direct io device, PMDL is NULL, "
                    "%!STATUS!", GetHandle(), status);
            }
            else {
                status = STATUS_SUCCESS;
            }

            Unlock(irql);

            return status;
        }
        else if (ioType == WdfDeviceIoNeither) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Attempt to get UserMode Buffer Pointer for "
                "WDFDEVICE 0x%p, WDFREQUEST 0x%p, %!STATUS!",
                GetDevice()->GetHandle(), GetHandle(), status);

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver must use buffered or direct I/O for this "
                "call, or use WdfDeviceInitSetIoInCallerContextCallback "
                "to probe and lock user mode memory");
            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            *pMdl = NULL;
            Unlock(irql);

            return status;
        }
        else {
            status = STATUS_INTERNAL_ERROR;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Unrecognized I/O Type %d on WDFDEVICE 0x%p, WDFREQUEST 0x%p, "
                "%!STATUS!",
                ioType, GetDevice()->GetHandle(), GetHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            *pMdl = NULL;
            Unlock(irql);

            return status;
        }
    }

    //
    // if pBuffer != NULL, attempt to generate an MDL for it
    //
    if( (pBuffer == NULL) || (length == 0) ) {
        *pMdl = NULL;
        Unlock(irql);

        status = STATUS_BUFFER_TOO_SMALL;

        if (pBuffer == NULL) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, SystemBuffer is NULL, %!STATUS!",
                GetHandle(), status);
        }
        else if (length == 0) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, SystemBuffer length is 0, %!STATUS!",
                GetHandle(), status);
        }

        return status;
    }

    //
    // See if we have already allocated an MDL for this
    //
    if( m_AllocatedMdl != NULL ) {
        *pMdl = m_AllocatedMdl;
        Unlock(irql);
        return STATUS_SUCCESS;
    }

    pNewMdl = FxMdlAllocate(pFxDriverGlobals,
                            this,
                            pBuffer,
                            length,
                            FALSE,
                            FALSE);

    if (pNewMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Could not allocate MDL for buffer 0x%p Length %d, %!STATUS!",
            pBuffer, length, status);
        *pMdl = NULL;
        Unlock(irql);

        return status;
    }

    Mx::MxBuildMdlForNonPagedPool(pNewMdl);

    //
    // We associated the Mdl with the request object
    // so it can be freed when completed.
    //
    m_AllocatedMdl = pNewMdl;

    *pMdl = pNewMdl;

    Unlock(irql);

    return STATUS_SUCCESS;
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
    ULONG              length;
    PVOID              pBuffer;
    KIRQL              irql;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS           status;
    PMDL pNewMdl;
    UCHAR majorFunction;

    length = 0;
    pBuffer = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    Lock(&irql);

    // Verifier
    if (pFxDriverGlobals->FxVerifierIO) {

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if (!NT_SUCCESS(status)) {
            Unlock(irql);
            return status;
        }
    }

    //
    // See if we have an IRP_MJ_DEVICE_CONTROL or IRP_MJ_INTERNAL_DEVICE_CONTROL
    //
    majorFunction = m_Irp.GetMajorFunction();

    if( !((majorFunction == IRP_MJ_DEVICE_CONTROL) ||
          (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)) ) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        //
        // It's a non-Device Control request, which is an error for this call
        //
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p (MajorFunction is 0x%x) this call is only valid for "
            "IOCTLs, %!STATUS!", GetHandle(), majorFunction, status);
        *pMdl = NULL;

        Unlock(irql);

        return status;
    }

    switch (m_Irp.GetParameterIoctlCodeBufferMethod()) {
    case METHOD_BUFFERED:

        pBuffer = m_Irp.GetSystemBuffer();
        length = m_Irp.GetParameterIoctlOutputBufferLength();

        // Fall through to common buffer handler
        break;

    case METHOD_IN_DIRECT:
        //
        // InputBuffer is in SystemBuffer
        // OutputBuffer is in MdlAddress with read access
        //
        *pMdl = m_Irp.GetMdl();

        if (*pMdl == NULL) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, METHOD_IN_DIRECT IOCTL, PMDL is NULL, "
                "%!STATUS!", GetHandle(), status);
        }
        else {
            status = STATUS_SUCCESS;
        }
        Unlock(irql);

        return status;

    case METHOD_OUT_DIRECT:
        //
        // InputBuffer is in SystemBuffer
        // OutputBuffer is in MdlAddress with write access
        //
        *pMdl = m_Irp.GetMdl();

        if (*pMdl == NULL) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, METHOD_OUT_DIRECT IOCTL, PMDL is NULL, "
                "%!STATUS!", GetHandle(), status);
        }
        else {
            status = STATUS_SUCCESS;
        }
        Unlock(irql);

        return status;

    case METHOD_NEITHER:
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Attempt to get UserMode Buffer Pointer for METHOD_NEITHER "
            "DeviceControl 0x%x, WDFDEVICE 0x%p, WDFREQUEST 0x%p, %!STATUS!",
            m_Irp.GetParameterIoctlCode(),
            GetDevice()->GetHandle(), GetHandle(), status);

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Driver must use METHOD_BUFFERED or METHOD_xx_DIRECT I/O for this "
            "call, or use WdfDeviceInitSetIoInCallerContextCallback to probe "
            "and lock user mode memory");

        *pMdl = NULL;
        Unlock(irql);

        return status;
    }

    //
    // if pBuffer != NULL, attempt to generate an MDL for it
    //
    if( (pBuffer == NULL) || (length == 0) ) {
        *pMdl = NULL;

        Unlock(irql);

        status = STATUS_BUFFER_TOO_SMALL;

        if (pBuffer == NULL) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, SystemBuffer is NULL, %!STATUS!",
                GetHandle(), status);
        }
        else if (length == 0) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, SystemBuffer length is 0, %!STATUS!",
                GetHandle(), status);
        }

        return status;
    }

    //
    // See if we have already allocated an MDL for this
    //
    if( m_AllocatedMdl != NULL ) {
        *pMdl = m_AllocatedMdl;
        Unlock(irql);

        return STATUS_SUCCESS;
    }

    pNewMdl= FxMdlAllocate(pFxDriverGlobals,
                           this,
                           pBuffer,
                           length,
                           FALSE,
                           FALSE);

    if (pNewMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p could not allocate MDL for buffer 0x%p Length %d, "
            "%!STATUS!", GetHandle(), pBuffer, length, status);
        *pMdl = NULL;
        Unlock(irql);

        return status;
    }

    Mx::MxBuildMdlForNonPagedPool(pNewMdl);

    //
    // We associated the Mdl with the request object
    // so it can be freed when completed.
    //
    m_AllocatedMdl = pNewMdl;

    *pMdl = pNewMdl;

    Unlock(irql);

    return STATUS_SUCCESS;
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
    NTSTATUS status;
    PMDL pMdl;
    PIRP irp;
    PVOID pVA;
    FxRequestMemory* pMemory;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pMdl = NULL;
    pMemory = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    if (Length == 0) {
        status = STATUS_INVALID_USER_BUFFER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Length of zero not allowed, %!STATUS!", status);

        return status;
    }

    irp = m_Irp.GetIrp();

    if (irp == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        // Error, completed request
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p has already been completed or has no PIRP assignment,"
            " %!STATUS!", GetObjectHandle(), status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return status;
    }

    //
    // Look at the Irp, and if there is a thread field, validate we are
    // actually in the correct process!
    //
    status = VerifyProbeAndLock(pFxDriverGlobals);
    if(!NT_SUCCESS(status) ) {
        return status;
    }

    pMdl = FxMdlAllocate(pFxDriverGlobals, this, Buffer, Length, FALSE, TRUE);

    if (pMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Failed to allocate MDL %!STATUS!", status);
        return status;
    }

    // Use a C utility function for the probe due to C++ exception handling issues
    status = FxProbeAndLockForRead(pMdl, UserMode);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Exception is raised for Address 0x%p, Length %d %!STATUS!",
                            Buffer, Length, status);
        FxMdlFree(pFxDriverGlobals, pMdl);
        return status;
    }

    //
    // Get a system address for the MDL
    //
    pVA = Mx::MxGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
    if (pVA == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // Build an FxRequestMemory object now
    status = FxRequestMemory::Create(pFxDriverGlobals, NULL, &pMemory);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    // Associate the FxRequestMemory object with the FxRequest
    status = pMemory->Commit(NULL, NULL, this);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

Done:
    if (NT_SUCCESS(status)) {
        //
        // If we re-define the WDFMEMORY interfaces to allow
        // failure for get buffer, we could delay the assignment
        // of SystemAddressForMdl and its PTE's until the driver
        // actually requests a double buffer mapping.
        //
        // Some DMA drivers may just retrieve the MDL from the WDFMEMORY
        // and not even attempt to access the underlying bytes, other
        // than through hardware DMA.
        //
        pMemory->SetMdl(
            this,
            pMdl,
            pVA,
            Length,
            TRUE // Readonly
            );

        *MemoryObject = pMemory;
    }
    else {
        if (pMemory != NULL) {
            pMemory->DeleteFromFailedCreate();
        }

        Mx::MxUnlockPages(pMdl);
        FxMdlFree(pFxDriverGlobals, pMdl);
    }

    return status;
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
    NTSTATUS status;
    PMDL pMdl;
    PIRP irp;
    PVOID pVA;
    FxRequestMemory* pMemory;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pMdl = NULL;
    pMemory = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    if (Length == 0) {
        status = STATUS_INVALID_USER_BUFFER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Length of zero not allowed, %!STATUS!", status);

        return status;
    }

    irp = m_Irp.GetIrp();

    if (irp == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        // Error, completed request
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p has already been completed or has no PIRP assignment,"
            " %!STATUS!", GetObjectHandle(), status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return status;
    }

    //
    // Look at the Irp, and if there is a thread field, validate we are
    // actually in the correct process!
    //
    status = VerifyProbeAndLock(pFxDriverGlobals);
    if(!NT_SUCCESS(status) ) {
        return status;
    }

    pMdl = FxMdlAllocate(pFxDriverGlobals, this, Buffer, Length, FALSE, TRUE);
    if (pMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Failed to allocate MDL %!STATUS!", status);
        return status;
    }

    // Use a C utility function for the probe due to C++ exception handling issues
    status = FxProbeAndLockForWrite(pMdl, UserMode);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Exception is raised for Address 0x%p, Length %d %!STATUS!",
                            Buffer, Length, status);
        FxMdlFree(pFxDriverGlobals, pMdl);
        return status;
    }

    //
    // Get a system address for the MDL
    //
    pVA = Mx::MxGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
    if (pVA == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    // Build an FxRequestMemory object now
    status = FxRequestMemory::Create(pFxDriverGlobals, NULL, &pMemory);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    // Associate the FxRequestMemory object with the FxRequest
    status = pMemory->Commit(NULL, NULL, this);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

Done:
    if (NT_SUCCESS(status)) {
        //
        // If we re-define the WDFMEMORY interfaces to allow
        // failure for get buffer, we could delay the assignment
        // of SystemAddressForMdl and its PTE's until the driver
        // actually requests a double buffer mapping.
        //
        // Some DMA drivers may just retrieve the MDL from the WDFMEMORY
        // and not even attempt to access the underlying bytes, other
        // than through hardware DMA.
        //
        pMemory->SetMdl(
            this,
            pMdl,
            pVA,
            Length,
            FALSE // FALSE indicates not a readonly buffer
            );

        *MemoryObject = pMemory;
    }
    else {
        if (pMemory != NULL) {
            pMemory->DeleteFromFailedCreate();
        }

        Mx::MxUnlockPages(pMdl);
        FxMdlFree(pFxDriverGlobals, pMdl);
    }

    return status;
}

