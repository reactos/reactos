#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_IO_QUEUE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out_opt
    WDFQUEUE* Queue
    )

/*++

Routine Description:

    Creates an IoQueue object and returns its handle to the caller.

    The newly created IoQueue object is associated with the IoPackage
    instance for the device.

    The IoQueue object is automatically dereferenced when its
    associated device is removed. This driver does not normally have
    to manually manage IoQueue object reference counts.

    An IoQueue object is created in the WdfIoQueuePause state, and is
    configured for WdfIoQueueDispatchSynchronous;

Arguments:

    Device      - Handle to the Device the I/O Package registered with
                   at EvtDeviceFileCreate time.

    Config     - WDF_IO_QUEUE_CONFIG structure

    pQueue      - Pointer to location to store the returned IoQueue handle.

Return Value:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoQueueStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    )

/*++

Routine Description:

    Set the Queues state to start accepting and dispatching new requests.

Arguments:

    Queue - Handle to Queue object

    A Queue may not go into a specific state right away, since it may have to
    wait for requests to be completed or cancelled. This is reflected
    in the status returned by WdfIoQueueGetState.

    See the Queue event API's for asynchronous notification of
    specific status states.

Returns:

    NTSTATUS
--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoQueueStopSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    )

/*++

Routine Description:

    Set the Queue state to accept and queue (not dispatch) incoming new requests and wait
    for 1) all the dispatch callbacks to return and 2) all the driver-owned request to complete.

Arguments:

    Queue - Handle to Queue object

Returns:

    NTSTATUS
--*/

{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
