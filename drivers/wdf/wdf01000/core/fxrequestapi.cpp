#include "wdf.h"

extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFAPI
WDFEXPORT(WdfRequestGetIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Returns the queue handle that currently owns the request.


Arguments:

    Request - Handle to the Request object


Returns:

    WDFQUEUE

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestCompleteWithInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus,
    __in
    ULONG_PTR Information
    )

/*++

Routine Description:

    Complete the request with supplied status and information.

    Any default reference counts implied by handle are invalid after
    completion.

Arguments:

    Request        - Handle to the Request object

    RequestStatus  - Wdm Status to complete the request with

    Information    - Information to complete request with

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveOutputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    )

/*++

Routine Description:

    Return the WDFMEMORY buffer associated with the request.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Memory - Pointer location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestComplete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus
    )

/*++

Routine Description:

    Complete the request with supplied status.

    Any default reference counts implied by handle are invalid after
    completion.

Arguments:

    Request        - Handle to the Request object

    RequestStatus  - Wdm Status to complete the request with

Returns:

    None

--*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestSetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    ULONG_PTR Information
    )

/*++

Routine Description:

    Set the transfer information for the request.

    This sets the NT Irp->Status.Information field.

Arguments:

    Request     - Handle to the Request object

    Information - Value to be set

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestMarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   PFN_WDF_REQUEST_CANCEL  EvtRequestCancel
   )

/*++

Routine Description:

    Mark the specified request as cancelable

Arguments:

    Request - Request to mark as cancelable.
    
    EvtRequestCancel - cancel routine to be invoked when the
                            request is cancelled.

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveInputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    )
/*++

Routine Description:

    Return the WDFMEMORY buffer associated with the request.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Memory - Pointer location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestUnmarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   )

/*++

Routine Description:

    Unmark the specified request as cancelable

Arguments:

    Request - Request to unmark as cancelable.

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

} // extern "C"
