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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoQueue* pQueue;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Queue,
                                   FX_TYPE_QUEUE,
                                   (PVOID*)&pQueue,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status))
    {
        return;
    }

    status = pQueue->QueueIdleSynchronously(FALSE);

    if (!NT_SUCCESS(status))
    {
        pQueue->FatalError(status);
        return;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_IO_QUEUE_STATE
WDFEXPORT(WdfIoQueueGetState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __out_opt
    PULONG QueueCount,
    __out_opt
    PULONG DriverCount
    )

/*++

Routine Description:

    Return the Queues status

Arguments:

    Queue        - Handle to Queue object

    pQueueCount  - Count of requests in the Queue not presented
                  to the driver.

    pDriverCount - Count of requests the driver is operating
                   on that are associated with this queue.

Returns:

    WDF_IO_QUEUE_STATE

--*/

{
    WDFNOTIMPLEMENTED();
    return WDF_IO_QUEUE_STATE::WdfIoQueueNoRequests;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoQueueStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE StopComplete,
    __drv_when(StopComplete != 0, __in)
    __drv_when(StopComplete == 0, __in_opt)
    WDFCONTEXT Context
    )

/*++

Routine Description:

    Set the Queue state to accept and queue (not dispatch) incoming new requests.

Arguments:

    Queue - Handle to Queue object

    A Queue may not go into a specific state right away, since it may have to
    wait for requests to be completed or cancelled. This is reflected
    in the status returned by WdfIoQueueGetState.

    See the Queue event API's for asynchronous notification of
    specific status states.

Returns:

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfIoQueueGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    )

/*++

Routine Description:
    Returns the device handle that the queue is associated with

Arguments:
    Queue - Handle to queue object

Return Value:
    WDFDEVICE handle

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveNextRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __out
    WDFREQUEST *OutRequest
    )

/*++

WdfIoQueueRetrieveNextRequest:

Routine Description:

    Returns a request from the head of the queue.

    On successful return the driver owns the request, and must
    eventually call WdfRequestComplete.

    The driver does not need to release any extra reference counts,
    other than calling WdfRequestComplete.


Arguments:

    Queue - Queue handle

    pOutRequest - Pointer to location to return new request handle

Returns:


    STATUS_NO_MORE_ENTRIES -     The queue is empty

    STATUS_SUCCESS        -     A request was returned in pOutRequest

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveRequestByFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in
    WDFFILEOBJECT FileObject,
    __out
    WDFREQUEST *OutRequest
    )

/*++

WdfIoQueueRetrieveNextRequest:

Routine Description:

    Returns a request from the queue that matches the fileobject.

    Requests are dequeued in a first in, first out manner.

    If there are no requests that match the selection criteria,
    STATUS_NO_MORE_ENTRIES is returned.

    On successful return the driver owns the request, and must
    eventually call WdfRequestComplete.

    The driver does not need to release any extra reference counts,
    other than calling WdfRequestComplete.

Arguments:

    Queue - Queue handle

    FileObject - FileObject to match in the request

    pOutRequest - Pointer to location to return new request handle

Returns:


    STATUS_NO_MORE_ENTRIES -     The queue is empty, or no more requests
                                match the selection criteria of TagRequest
                                and FileObject specified above.

    STATUS_SUCCESS        -     A request context was returned in
                                pOutRequest

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueFindRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in_opt
    WDFREQUEST TagRequest,
    __in_opt
    WDFFILEOBJECT FileObject,
    __inout_opt
    PWDF_REQUEST_PARAMETERS Parameters,
    __out
    WDFREQUEST * OutRequest
    )

/*++

WdfIoQueueFindRequest:

Routine Description:

    PeekRequest allows a caller to enumerate through requests in
    a queue, optionally only returning requests that match a specified
    FileObject.

    The first call specifies TagContext == NULL, and the first request
    in the queue that matches the FileObject is returned.

    Subsequent requests specify the previous request value as the
    TagContext, and searching will continue at the request that follows.

    If the queue is empty, there are no requests after TagContext, or no
    requests match the FileObject, NULL is returned.

    If FileObject == NULL, this matches any FileObject in a request.

    If a WDF_REQUEST_PARAMETERS structure is supplied, the information
    from the request is returned to allow the driver to further examine
    the request to decide whether to service it.

    If a TagRequest is specified, and it is not found, the return
    status STATUS_NOT_FOUND means that the queue should
    be re-scanned. This is because the TagRequest was cancelled from
    the queue, or if the queue was active, delivered to the driver.
    There may still be un-examined requests on the queue that match
    the drivers search criteria, but the search marker has been lost.

    Re-scanning the queue starting with TagRequest == NULL and
    continuing until STATUS_NO_MORE_ENTRIES is returned will ensure
    all requests have been examined.

    Enumerating an active queue with this API could result in the
    driver frequently having to re-scan.

    If a successful return of a Request object handle occurs, the driver
    *must* call WdfObjectDereference when done with it, otherwise an
    object leak will result.

    Returned request objects are not owned by the driver, and may not be
    used by I/O or completed. The only valid operations are
    WdfRequestGetParameters, passing it as the TagRequest parameter to
    WdfIoQueueFindRequest, WdfIoQueueRetrieveFoundRequest, or calling
    WdfObjectDereference. All other actions are undefined.

    The request could be cancelled without a EvtIoCancel callback while the
    driver has the handle. The request then becomes invalid, and
    WdfObjectDereference must be called to release its resources.

    The caller should not use any buffer pointers in the returned parameters
    structure until it successfully receives ownership of the request by
    calling WdfIoQueueRetrieveFoundRequest with the request tag. This is because
    the I/O can be cancelled and completed at any time without the driver
    being notified until it has received ownership, thus invalidating
    the buffer pointers and releasing the memory, even if the request
    object itself it still valid due to the reference.

    The driver should hold the reference on the object until after
    a call to WdfIoQueueFindRequest using it as the TagRequest
    parameter, or WdfIoQueueRetrieveFoundtRequest. Otherwise, a race could
    result in which the object is cancelled, its memory re-used for
    a new request, and then an attempt to use it as a tag would result
    in a different state than intended. The driver verifier will
    catch this, but it is a rare race.

    Calling this API on an operating Queue can lead to confusing results. This
    is because the request could be presented to EvtIoDefault, causing it to be
    invalid as a TagRequest in calls to WdfIoQueueFindRequest, and
    WdfIoQueueRetrieveFoundRequest. The extra reference taken on the object by its return
    from this call must still be released by WdfObjectDereference.

    The intention of the API is to only hold onto the TagRequest long enough
    for the driver to make a decision whether to service the request, or
    to continue searching for requests.

Arguments:

    Queue       - Queue handle

    TagRequest  - If !NULL, request to begin search at

    FileObject  - If !NULL, FileObject to match in the request

    Parameters  - If !NULL, pointer to buffer to return the requests
                  parameters to aid in selection.

    pOutRequest - Pointer to location to return request handle

Returns:

    STATUS_NOT_FOUND - TagContext was specified, but not
                                found in the queue. This could be
                                because the request was cancelled,
                                or is part of an active queue and
                                the request was passed to the driver
                                or forwarded to another queue.

    STATUS_NO_MORE_ENTRIES -     The queue is empty, or no more requests
                                match the selection criteria of TagRequest
                                and FileObject specified above.

    STATUS_SUCCESS        -     A request context was returned in
                                pOutRequest

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueRetrieveFoundRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST TagRequest,
    __out
    WDFREQUEST * OutRequest
    )

/*++

WdfIoQueueRetrieveNextRequest:

Routine Description:

    Returns a request from the queue.

    Requests are dequeued in a first in, first out manner.

    The queue is searched for specific peeked
    request, and if it is not found, STATUS_NOT_FOUND is
    returned. A TagRequest value is returned
    from WdfIoQueueFindRequest().

    If there are no requests that match the selection criteria,
    STATUS_NO_MORE_ENTRIES is returned.

    On successful return the driver owns the request, and must
    eventually call WdfRequestComplete.

    The driver does not need to release any extra reference counts,
    other than calling WdfRequestComplete.

Arguments:

    Queue - Queue handle

    TagRequest - Request to look for in queue

    pOutRequest - Pointer to location to return new request handle

Returns:

    STATUS_NOT_FOUND - TagContext was specified, but not
                                found in the queue. This could be
                                because the request was cancelled,
                                or is part of an active queue and
                                the request was passed to the driver
                                or forwarded to another queue.

    STATUS_NO_MORE_ENTRIES -     The queue is empty, or no more requests
                                match the selection criteria of TagRequest
                                specified above.

    STATUS_SUCCESS        -     A request context was returned in
                                pOutRequest

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoQueueDrainSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    )

/*++

    Set the Queue to fail new request with STATUS_CANCELLED,
    dispatches pending requests, and waits for the all the pending and
    inflight requests to complete before returning.

    Should be called at PASSIVE_LEVEL.

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoQueueDrain)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE DrainComplete,
    __drv_when(DrainComplete != 0, __in)
    __drv_when(DrainComplete == 0, __in_opt)
    WDFCONTEXT Context
    )

/*++

     Set the Queue to reject new requests, with newly arriving
     requests being completed with STATUS_CANCELLED.

     If the optional DrainComplete callback is specified, the
     callback will be invoked with the supplied context when
     there are no requests owned by the driver and no request pending in the
     queue.

     Only one callback registration for WdfIoQueueIdle, WdfIoQueuePurge,
     or WdfIoQueueDrain may be outstanding at a time.

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoQueuePurgeSynchronously)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFQUEUE Queue
   )

/*++

    Sets the queue state to fail incoming requests,
    cancels all the pending requests, cancels in-flights requests
    (if they are marked cancelable), and waits for all the requests
    to complete before returning.

    Should be called at PASSIVE_LEVEL.

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoQueuePurge)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    __drv_when(PurgeComplete != 0, __in)
    __drv_when(PurgeComplete == 0, __in_opt)
    WDFCONTEXT Context
    )

/*++

 Set the Queue to reject new requests, with newly arriving
 requests being completed with STATUS_CANCELLED.

 A Queue may not purge immediately, since the device driver
 could be operating on non-cancelable requests.


 If the optional PurgeComplete callback is specified, the
 callback will be invoked with the supplied context when
 the Queue is in the WDF_IO_QUEUE_PURGED state.
 (Reject new requests, no current requests in Queue or device driver)

 Only one callback registration for WdfIoQueueIdle, WdfIoQueuePurge,
 or WdfIoQueueDrain may be outstanding at a time.

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueReadyNotify)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
     __in_opt
    PFN_WDF_IO_QUEUE_STATE QueueReady,
     __in_opt
    WDFCONTEXT Context
    )

/*++

 This API notifies the device driver when the Queue
 has one or more requests that can be processed by
 the device driver.

 This event registration continues until cancelled with
 by calling with NULL for QueueReady.

 One event is generated for each transition from
 not ready, to ready. An event is not generated for each
 new request, only request arrival on an empty Queue.

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueAssignForwardProgressPolicy)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE   Queue,
    __in
    PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY ForwardProgressPolicy
   )

/*++

 This DDI  is used by the driver to configure a queue to have forward
  progress. 
--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
