/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetSelf.cpp

Abstract:

    This module implements the IO Target APIs

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/


#include "../fxtargetsshared.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxIoTargetSelf.tmh"
#endif
}

FxIoTargetSelf::FxIoTargetSelf(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ USHORT ObjectSize
    ) :
    FxIoTarget(FxDriverGlobals, ObjectSize, FX_TYPE_IO_TARGET_SELF),
    m_DispatchQueue(NULL)
{
}

FxIoTargetSelf::~FxIoTargetSelf()
{
}

FxIoQueue*
FxIoTargetSelf::GetDispatchQueue(
    UCHAR MajorFunction
    )
/*++
Routine Description:
    Returns a pointer to the queue to which an IO sent to the Self
    io target must be sent to

Arguments:

    MajorFunction - IRP_MJ_READ, IRP_MJ_WRITE, or IRP_MJ_DEVICE_CONTROL

Returns:

    FxIoQueue*

--*/
{
    if (m_DispatchQueue != NULL) {
        return m_DispatchQueue;
    }

    return m_Device->m_PkgIo->GetDispatchQueue(MajorFunction);
}

VOID
FxIoTargetSelf::Send(
    _In_ MdIrp Irp
    )
/*++
Routine Description:
    send an MdIrp to the Self IO Target.

Arguments:

    MdIrp for IRP_MJ_READ, IRP_MJ_WRITE, or IRP_MJ_DEVICE_CONTROL

Returns:

    VOID

Implementation Note:

    Function body inspired by WdfDeviceWdmDispatchIrpToIoQueue API.

--*/
{
    FxIrp irp(Irp);
    FxIoQueue* queue;
    NTSTATUS status;
    UCHAR majorFunction;
    FxIoInCallerContext* ioInCallerCtx;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    //
    // Prepare the request to forward to the inteternal target.
    //
    (static_cast<IWudfIoIrp2*>(Irp))->PrepareToForwardToSelf();

#else
    //
    // Set Next Stack Location
    //
    irp.SetNextIrpStackLocation();

    //
    // Set Device Object.
    //
    irp.SetCurrentDeviceObject(m_Device->GetDeviceObject());
#endif

    majorFunction = irp.GetMajorFunction();

    //
    // Retrieve Queue
    //
    queue = GetDispatchQueue(majorFunction);

    if (queue == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Send WDFIOTARGET %p, No Dispatch Queue Found for Major Function %d",
            GetObjectHandle(), majorFunction);
        status = STATUS_INVALID_DEVICE_STATE;
        goto Fail;
    }

    //
    // Only read/writes/ctrls/internal_ctrls IRPs are allowed to be sent to
    // Self IO Target
    //
    if (m_Device->GetDispatchPackage(majorFunction) != m_Device->m_PkgIo) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Only Read/Write/Control/Internal-Control IRPs can be "
                "forwarded to Self IO Target 0x%p, %!IRPMJ!, "
                "IRP_MN %x, Device 0x%p, %!STATUS!",
                 GetHandle(), majorFunction, irp.GetMinorFunction(),
                 m_Device->GetObjectHandle(), status);
        FxVerifierDbgBreakPoint(GetDriverGlobals());
        goto Fail;
    }

    //
    // Retrieve the InContextCallback function
    //
    ioInCallerCtx = m_Device->m_PkgIo->GetIoInCallerContextCallback(
                                            queue->GetCxDeviceInfo());

    //
    // DispatchStep2 will convert the IRP into a WDFREQUEST, queue it and if
    // possible dispatch the request to the driver.
    // If a failure occurs, DispatchStep2 completes teh Irp
    //
    (VOID) m_Device->m_PkgIo->DispatchStep2(Irp, ioInCallerCtx, queue);
    return;

Fail:

    irp.SetStatus(status);
    irp.SetInformation(0);
    irp.CompleteRequest(IO_NO_INCREMENT);

    return;
}

