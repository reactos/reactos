#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxpkgio.h"
#include "common/fxvalidatefunctions.h"
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    CfxDevice*  pDevice;
    FxPkgIo*   pPkgIo;
    FxIoQueue* pQueue;
    NTSTATUS status;

    //
    // Validate the I/O Package handle, and get the FxPkgIo*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    pPkgIo = NULL;
    pQueue = NULL;

    FxPointerNotNull(pFxDriverGlobals, Config);

    status = FxValidateObjectAttributes(pFxDriverGlobals, 
                                        QueueAttributes, 
                                        (FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
                                         FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Validate Config structure    
    if (Config->Size != sizeof(WDF_IO_QUEUE_CONFIG) &&
        Config->Size != sizeof(WDF_IO_QUEUE_CONFIG_V1_9) &&
        Config->Size != sizeof(WDF_IO_QUEUE_CONFIG_V1_7))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "WDF_IO_QUEUE_CONFIG Size 0x%x, "
                            "expected for v1.7 size 0x%x or v1.9 size 0x%x or "
                            "current version size 0x%x, %!STATUS!",
                            Config->Size, sizeof(WDF_IO_QUEUE_CONFIG_V1_7),
                            sizeof(WDF_IO_QUEUE_CONFIG_V1_9),
                            sizeof(WDF_IO_QUEUE_CONFIG), status);
        return status;
    }
      
    //
    // If the queue is not a default queue then the out parameter is not optional
    //
    if (!Config->DefaultQueue && Queue == NULL)
    {
        status = STATUS_INVALID_PARAMETER_4;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Parameter to receive WDFQUEUE handle is not optional "
            "for non default queue %!STATUS!", status);

        return status;
    }

    //pPkgIo = (FxPkgIo*)pDevice->m_PkgIo;
    pPkgIo = pDevice->m_PkgIo;

    //
    // If the queue is a default queue, then we restrict the creation to happen
    // a) before the device is started for pnp devices (FDO or PDO)
    // b) before the call to WdfControlDeviceFinishInitializing for controldevices.
    // This is done to prevent some unknown race conditions and reduce
    // the test matrix.
    //
    if (Config->DefaultQueue)
    {
        if (pDevice->IsLegacy())
        {
            //
            // This is a controldevice. Make sure the create is called after the device
            // is initialized and ready to accept I/O.
            //
            if ((pDevice->GetDeviceObjectFlags() & DO_DEVICE_INITIALIZING) == FALSE)
            {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Default queue can only be created before WdfControlDeviceFinishInitializing"
                    "on the WDFDEVICE %p is called %!STATUS!",
                    Device,
                    STATUS_INVALID_DEVICE_STATE);
                return STATUS_INVALID_DEVICE_STATE;
            }
        }
        else
        {
            //
            // This is either FDO or PDO. Make sure it's not started yet.
            //
            if (pDevice->GetDevicePnpState() != WdfDevStatePnpInit)
            {
                status = STATUS_INVALID_DEVICE_STATE;
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Default queue can only be created before the WDFDEVICE 0x%p "
                    "is started, %!STATUS!", Device, status);
                return status;
            }
        }
    }

    //
    // Create the Queue for the I/O package
    //
    status = pPkgIo->CreateQueue(Config, 
                                 QueueAttributes,
                                 GetFxDriverGlobals(DriverGlobals)->Driver,
                                 &pQueue);
    if (!NT_SUCCESS(status))
    {
       DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Queue Creation failed "
                           "for WDFDEVICE 0x%p, %!STATUS!", Device, status);
       return status;
    }

    if (Config->DefaultQueue)
    {
        //
        // Make this a default queue.   The default queue receives any
        // I/O requests that have not been otherwise forwarded to another
        // queue by WdfDeviceConfigureRequestDispatching .
        //

        status = pPkgIo->InitializeDefaultQueue(
                          pDevice,
                          pQueue
                          );

        if (!NT_SUCCESS(status))
        {
           DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                               "Create failed "
                               "for FxPkgIo 0x%p, WDFDEVICE 0x%p",pPkgIo, Device);
           //
           // Delete the queue *without* invoking driver defined callbacks
           //
           pQueue->DeleteFromFailedCreate();

           return status;
       }
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                       "Created WDFQUEUE 0x%p", pQueue->GetObjectHandle());

    if (Queue != NULL)
    {
        *Queue = (WDFQUEUE)pQueue->GetObjectHandle();
    }

    return STATUS_SUCCESS;
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
    DDI_ENTRY();

    FxIoQueue* pQueue;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                          Queue,
                          FX_TYPE_QUEUE,
                          (PVOID*)&pQueue);

    pQueue->QueueStart();

    return;
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
