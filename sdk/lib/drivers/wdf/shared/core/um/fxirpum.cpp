//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxmin.hpp"

extern "C" {
#include "FxIrpUm.tmh"

extern IWudfHost2 *g_IWudfHost2;
}

#define TraceEvents(a,b,c,d,e) UNREFERENCED_PARAMETER(d);

MdIrp
FxIrp::GetIrp(
    VOID
    )
{
    return m_Irp;
}


VOID
FxIrp::CompleteRequest(
    __in_opt CCHAR PriorityBoost
    )
{
    UNREFERENCED_PARAMETER(PriorityBoost);

    m_Irp->CompleteRequest();
    m_Irp = NULL;
}


NTSTATUS
FxIrp::CallDriver(
    __in MdDeviceObject DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    m_Irp->Forward();
    return STATUS_SUCCESS;
}


NTSTATUS
FxIrp::PoCallDriver(
    __in MdDeviceObject DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    m_Irp->Forward();
    return STATUS_SUCCESS;
}



VOID
FxIrp::StartNextPowerIrp(
    )
{



    DO_NOTHING();
}


VOID
FxIrp::SetCompletionRoutine(
    __in MdCompletionRoutine CompletionRoutine,
    __in PVOID Context,
    __in BOOLEAN InvokeOnSuccess,
    __in BOOLEAN InvokeOnError,
    __in BOOLEAN InvokeOnCancel
    )
{
    UNREFERENCED_PARAMETER(InvokeOnSuccess);
    UNREFERENCED_PARAMETER(InvokeOnError);
    UNREFERENCED_PARAMETER(InvokeOnCancel);

    //
    // In UMDF completion callback is invoked in all three cases, there isn't an option
    // to invoke it selectively




    FX_VERIFY(INTERNAL, CHECK(
        "UMDF completion routine can't be invoked selectively on Success/Error/Cancel",
        (TRUE == InvokeOnSuccess) &&
        (TRUE == InvokeOnError) &&
        (TRUE == InvokeOnCancel)));

    m_Irp->SetCompletionRoutine(
        CompletionRoutine,
        Context
        );
}

VOID
FxIrp::SetCompletionRoutineEx(
    __in MdDeviceObject DeviceObject,
    __in MdCompletionRoutine CompletionRoutine,
    __in PVOID Context,
    __in BOOLEAN InvokeOnSuccess,
    __in BOOLEAN InvokeOnError,
    __in BOOLEAN InvokeOnCancel
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    SetCompletionRoutine(
        CompletionRoutine,
        Context,
        InvokeOnSuccess,
        InvokeOnError,
        InvokeOnCancel);
}

MdCancelRoutine
FxIrp::SetCancelRoutine(
    __in_opt MdCancelRoutine  CancelRoutine
    )
{
    return m_Irp->SetCancelRoutine(CancelRoutine);
}


NTSTATUS
FxIrp::_IrpSynchronousCompletion(
    __in MdDeviceObject DeviceObject,
    __in MdIrp /*OriginalIrp*/,
    __in PVOID Context
    )
{
    HANDLE event = (HANDLE) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    SetEvent(event);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


_Must_inspect_result_
NTSTATUS
FxIrp::SendIrpSynchronously(
    __in MdDeviceObject DeviceObject
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE event;

    UNREFERENCED_PARAMETER(DeviceObject);

    event = CreateEvent(
                    NULL,
                    TRUE,  //bManualReset
                    FALSE, //bInitialState
                    NULL   //Name
                    );

    if (NULL == event)
    {
#pragma prefast(suppress:__WARNING_MUST_USE, "we convert all Win32 errors into a generic failure currently")
        DWORD err = GetLastError();

        //
        // As such event creation would fail only for resource reasons if we pass
        // correct parameters
        //

        status = STATUS_INSUFFICIENT_RESOURCES;

        TraceEvents( TRACE_LEVEL_ERROR,
                    FX_TRACE_IO,
                    "Failed to create event, error: %!WINERROR!, "
                    "returning %!STATUS!",
                    err, status);

    }

    if (NT_SUCCESS(status))
    {
        SetCompletionRoutine(_IrpSynchronousCompletion,
                             event,
                             TRUE,
                             TRUE,
                             TRUE);

        m_Irp->Forward();

        DWORD retval = WaitForSingleObject(event, INFINITE);
        FX_VERIFY(INTERNAL, CHECK("INFNITE wait failed",
                                (retval == WAIT_OBJECT_0)));

        status = this->GetStatus();
    }

    return status;
}


VOID
FxIrp::CopyCurrentIrpStackLocationToNext(
    VOID
    )
{
    m_Irp->CopyCurrentIrpStackLocationToNext();
}

UCHAR
FxIrp::GetMajorFunction(
    VOID
    )
{
    UCHAR majorFunction = IRP_MJ_MAXIMUM_FUNCTION;
    IWudfIoIrp * pIoIrp = NULL;
    IWudfPnpIrp * pPnpIrp = NULL;

    //
    // IWudfIrp does not expose a method to get major function code. So we
    // find out if it's an I/O irp or pnp irp. If I/O irp then we use GetType
    // method to retrieve request tyoe and then map it to major function code,
    // otherwise if it is pnp irp then we just use GetMajorFunction method
    // exposed by IWudfPnpIrp.
    //
    HRESULT hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    if (SUCCEEDED(hrQI)) {
        UMINT::WDF_REQUEST_TYPE type;

        //
        // for Io irp, map request type to major funcction
        //
        type = (UMINT::WDF_REQUEST_TYPE) pIoIrp->GetType();
        switch(type) {
        case  UMINT::WdfRequestCreate:
            majorFunction = IRP_MJ_CREATE;
            break;
        case  UMINT::WdfRequestCleanup:
            majorFunction = IRP_MJ_CLEANUP;
            break;
        case  UMINT::WdfRequestRead:
            majorFunction = IRP_MJ_READ;
            break;
        case  UMINT::WdfRequestWrite:
            majorFunction = IRP_MJ_WRITE;
            break;
        case  UMINT::WdfRequestDeviceIoControl:
            majorFunction = IRP_MJ_DEVICE_CONTROL;
            break;
        case  UMINT::WdfRequestClose:
            majorFunction = IRP_MJ_CLOSE;
            break;
        case  UMINT::WdfRequestInternalIoctl:
            majorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            break;
        case UMINT::WdfRequestFlushBuffers:
            majorFunction = IRP_MJ_FLUSH_BUFFERS;
            break;
        case UMINT::WdfRequestQueryInformation:
            majorFunction = IRP_MJ_QUERY_INFORMATION;
            break;
        case UMINT::WdfRequestSetInformation:
            majorFunction = IRP_MJ_SET_INFORMATION;
            break;
        case  UMINT::WdfRequestUsb:
        case  UMINT::WdfRequestOther:
            // fall through
        default:
            FX_VERIFY(INTERNAL, TRAPMSG("The request type is not expected"));
        }

        pIoIrp->Release();
    }
    else {
        FX_VERIFY(INTERNAL, CHECK_NULL(pIoIrp));

        //
        // see if it is a pnp irp
        //
        hrQI = m_Irp->QueryInterface(IID_IWudfPnpIrp, (PVOID*)&pPnpIrp);
        FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pPnpIrp));

        majorFunction = pPnpIrp->GetMajorFunction();
        pPnpIrp->Release();
    }

    return majorFunction;
}

UCHAR
FxIrp::GetMinorFunction(
    VOID
    )
{
    UCHAR minorFunction;
    IWudfPnpIrp * pPnpIrp = NULL;
    IWudfIoIrp * pIoIrp = NULL;

    HRESULT hrQI = m_Irp->QueryInterface(IID_IWudfPnpIrp, (PVOID*)&pPnpIrp);
    if (SUCCEEDED(hrQI)) {
        minorFunction = pPnpIrp->GetMinorFunction();
        pPnpIrp->Release();
    }
    else {
        //
        // If this is not PnP irp then this must be Io irp.
        //
        hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
        FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIoIrp));
        pIoIrp->Release();

        //
        // Minor function is 0 for I/O irps (create/cleanup/close/read/write/
        // ioctl).
        //
        minorFunction = 0;
    }

    return minorFunction;
}

KPROCESSOR_MODE
FxIrp::GetRequestorMode(
    VOID
    )
{
    IWudfIoIrp * pIoIrp = NULL;

    HRESULT hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);

    if (SUCCEEDED(hrQI)) {
        KPROCESSOR_MODE requestorMode;

        requestorMode = pIoIrp->GetRequestorMode();
        pIoIrp->Release();

        return requestorMode;
    }
    else {
        return KernelMode;
    }
}

VOID
FxIrp::SetContext(
    __in ULONG Index,
    __in PVOID Value
    )
{
    m_Irp->SetContext(Index, Value);
}


PVOID
FxIrp::GetContext(
    __in ULONG Index
    )
{
    return m_Irp->GetContext(Index);
}


PIO_STACK_LOCATION
FxIrp::GetCurrentIrpStackLocation(
    VOID
    )
{




    // The Km implementation does some verifier checks in this function so
    // mode agnostic code uses it and therefore we provide the um version as a
    // stub. The return value is NULL and is not used by the caller.
    //
    return NULL;
}


PIO_STACK_LOCATION
FxIrp::GetNextIrpStackLocation(
    VOID
    )
{




    FX_VERIFY(INTERNAL, TRAPMSG("Common code using io stack location directly"));
    return NULL;
}

VOID
FxIrp::SkipCurrentIrpStackLocation(
    VOID
    )
{
    //
    // Earler we always used to copy because the framework always set a
    // completion routine to notify other packages that we completed. However,
    // since some I/O paths relied on Skip to revert the action taken by
    // SetNextStackLocation in failure paths, we now skip instead of copy. The
    // same behavior applies to KMDF as well.
    //
    m_Irp->SkipCurrentIrpStackLocation();
}

VOID
FxIrp::MarkIrpPending(
    VOID
    )
{





    m_Irp->MarkIrpPending();
}


BOOLEAN
FxIrp::PendingReturned(
    VOID
    )
{





    return m_Irp->PendingReturned();
}


VOID
FxIrp::PropagatePendingReturned(
    VOID
    )
{





    m_Irp->PropagatePendingReturned();
}


VOID
FxIrp::SetStatus(
    __in NTSTATUS Status
    )
{
    m_Irp->SetStatus(Status);
}


NTSTATUS
FxIrp::GetStatus(
    VOID
    )
{
    return m_Irp->GetStatus();
}


BOOLEAN
FxIrp::Cancel(
    VOID
    )
{
    return (m_Irp->Cancel() ? TRUE: FALSE);
}


BOOLEAN
FxIrp::IsCanceled(
    )
{
    return (m_Irp->IsCanceled() ? TRUE: FALSE);
}


KIRQL
FxIrp::GetCancelIrql(
    )
{
    //
    // CancelIrql is used to pass in to IoReleaseCancelSpinLock
    // hence it is not applicable to UMDF.
    //
    return PASSIVE_LEVEL;
}


VOID
FxIrp::SetInformation(
    __in ULONG_PTR Information
    )
{
    m_Irp->SetInformation(Information);
}


ULONG_PTR
FxIrp::GetInformation(
    )
{
    return m_Irp->GetInformation();
}


CCHAR
FxIrp::GetCurrentIrpStackLocationIndex(
    )
{




    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));

    return -1;
}


PLIST_ENTRY
FxIrp::ListEntry(
    )
{
    return m_Irp->GetListEntry();
}


PVOID
FxIrp::GetSystemBuffer(
    )
{
    IWudfIoIrp * ioIrp = NULL;
    PVOID systemBuffer = NULL;
    HRESULT hr;

    ioIrp = GetIoIrp();

    switch (GetMajorFunction()) {
    case IRP_MJ_WRITE:
        //
        // For write host provides the buffer as input buffer
        //
        hr = ioIrp->RetrieveBuffers(NULL,         // InputBufferCb
                                    &systemBuffer,// InputBuffer
                                    NULL,         // OutputBufferCb
                                    NULL          // OutputBuffer
                                    );
        break;
    case IRP_MJ_READ:
        //
        // For read host provides the buffer as output buffer
        //
        hr = ioIrp->RetrieveBuffers(NULL,         // InputBufferCb
                                    NULL,         // InputBuffer
                                    NULL,         // OutputBufferCb
                                    &systemBuffer // OutputBuffer
                                    );
        break;
    case IRP_MJ_DEVICE_CONTROL:
        hr = ioIrp->RetrieveBuffers(NULL,         // InputBufferCb
                                    &systemBuffer,// InputBuffer
                                    NULL,         // OutputBufferCb
                                    NULL          // OutputBuffer
                                    );
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
        hr = E_NOTIMPL;
        break;
    }

    if (FAILED(hr)) {
        systemBuffer = NULL;
    }

    return systemBuffer;
}

PVOID
FxIrp::GetOutputBuffer(
    )
{
    IWudfIoIrp * ioIrp = NULL;
    PVOID outputBuffer = NULL;
    HRESULT hr;

    ioIrp = GetIoIrp();

    switch (GetMajorFunction()) {
    case IRP_MJ_DEVICE_CONTROL:
        hr = ioIrp->RetrieveBuffers(NULL,         // InputBufferCb
                                    NULL,         // InputBuffer
                                    NULL,         // OutputBufferCb
                                    &outputBuffer // OutputBuffer
                                    );
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
        hr = E_NOTIMPL;
        break;
    }

    if (FAILED(hr)) {
        outputBuffer = NULL;
    }

    return outputBuffer;
}

PMDL
FxIrp::GetMdl(
    )
{
    return NULL;
}


PVOID
FxIrp::GetUserBuffer(
    )
{
    //
    // UserBuffer is used with METHOD_NEITHER
    // For METHOD_NEITHER reflector still makes a copy of the buffer
    // in common buffer (if METHOD_NEITHER is enabled via regkey)
    //

    IWudfIoIrp * ioIrp = GetIoIrp();
    PVOID userBuffer;
    HRESULT hr;

    hr = ioIrp->RetrieveBuffers(NULL,         // InputBufferCb
                                NULL,         // InputBuffer
                                NULL,         // OutputBufferCb
                                &userBuffer   // OutputBuffer
                                );
    if (FAILED(hr))
    {
        userBuffer = NULL;
    }

    return userBuffer;
}


VOID
FxIrp::Reuse(
    __in NTSTATUS Status
    )
{
    GetIoIrp()->Reuse(Status);
    return;
}


SYSTEM_POWER_STATE_CONTEXT
FxIrp::GetParameterPowerSystemPowerStateContext(
    )
{
    SYSTEM_POWER_STATE_CONTEXT systemPwrStateContext = {0};

    systemPwrStateContext.ContextAsUlong = GetPnpIrp()->GetSystemPowerStateContext();

    return systemPwrStateContext;
}


POWER_STATE_TYPE
FxIrp::GetParameterPowerType(
    )
{
    POWER_STATE_TYPE powerType;

    powerType = GetPnpIrp()->GetPowerType();

    return powerType;
}


DEVICE_POWER_STATE
FxIrp::GetParameterPowerStateDeviceState(
    )
{
    DEVICE_POWER_STATE devicePowerState;

    devicePowerState = GetPnpIrp()->GetPowerStateDeviceState();

    return devicePowerState;
}


SYSTEM_POWER_STATE
FxIrp::GetParameterPowerStateSystemState(
    )
{
    SYSTEM_POWER_STATE systemPowerState;

    systemPowerState = GetPnpIrp()->GetPowerStateSystemState();

    return systemPowerState;
}


POWER_ACTION
FxIrp::GetParameterPowerShutdownType(
    )
{
    POWER_ACTION powerAction;

    powerAction = GetPnpIrp()->GetPowerAction();

    return powerAction;
}


DEVICE_RELATION_TYPE
FxIrp::GetParameterQDRType(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return BusRelations;
}

//
// Get Methods for IO_STACK_LOCATION.Parameters.QueryInterface




PINTERFACE
FxIrp::GetParameterQueryInterfaceInterface(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return NULL;
}

const GUID*
FxIrp::GetParameterQueryInterfaceType(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return NULL;
}


USHORT
FxIrp::GetParameterQueryInterfaceVersion(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return 0;
}


USHORT
FxIrp::GetParameterQueryInterfaceSize(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return 0;
}


PVOID
FxIrp::GetParameterQueryInterfaceInterfaceSpecificData(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return NULL;
}

//
// Get Method for IO_STACK_LOCATION.Parameters.UsageNotification





DEVICE_USAGE_NOTIFICATION_TYPE
FxIrp::GetParameterUsageNotificationType(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return DeviceUsageTypeUndefined;
}


BOOLEAN
FxIrp::GetParameterUsageNotificationInPath(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return FALSE;
}


VOID
FxIrp::SetParameterUsageNotificationInPath(
    __in BOOLEAN /*InPath*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}


BOOLEAN
FxIrp::GetNextStackParameterUsageNotificationInPath(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

    return FALSE;
}

//
// Get/Set Methods for IO_STACK_LOCATION.Parameters.StartDevice
//

PCM_RESOURCE_LIST
FxIrp::GetParameterAllocatedResources(
    )
{
    IWudfPnpIrp * pnpIrp = NULL;
    PCM_RESOURCE_LIST res = NULL;






    pnpIrp = static_cast<IWudfPnpIrp *>(m_Irp);

    res = pnpIrp->GetParameterAllocatedResources();

    //
    // Release the ref even though we are returning a memory pointer from
    // IWudfIrp object. This is fine because we know irp is valid for the
    // lifetime of the caller who is calling this interface).
    //


    return res;
}

VOID
FxIrp::SetParameterAllocatedResources(
    __in PCM_RESOURCE_LIST /*AllocatedResources*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
}

PCM_RESOURCE_LIST
FxIrp::GetParameterAllocatedResourcesTranslated(
    )
{
    IWudfPnpIrp * pnpIrp = NULL;
    PCM_RESOURCE_LIST res = NULL;






    pnpIrp = static_cast<IWudfPnpIrp *>(m_Irp);

    res = pnpIrp->GetParameterAllocatedResourcesTranslated();


    return res;
}

VOID
FxIrp::SetParameterAllocatedResourcesTranslated(
    __in PCM_RESOURCE_LIST /*AllocatedResourcesTranslated*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
}

VOID
FxIrp::SetMajorFunction(
    __in UCHAR MajorFunction
    )
{
    IWudfIoIrp * pIoIrp = NULL;

    //
    // IWudfIrp does not expose a method to set major function code directly so
    // map it to WdfRequestType.
    //
    HRESULT hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    if (SUCCEEDED(hrQI)) {
        UMINT::WDF_REQUEST_TYPE type;

        //
        // for Io irp, map major function to request type
        //
        switch(MajorFunction) {
        case IRP_MJ_CREATE:
            type = UMINT::WdfRequestCreate;
            break;
        case IRP_MJ_CLEANUP:
            type = UMINT::WdfRequestCleanup;
            break;
        case IRP_MJ_READ:
            type = UMINT::WdfRequestRead;
            break;
        case IRP_MJ_WRITE:
            type = UMINT::WdfRequestWrite;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            type = UMINT::WdfRequestDeviceIoControl;
            break;
        case IRP_MJ_CLOSE:
            type = UMINT::WdfRequestClose;
            break;
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            type = UMINT::WdfRequestInternalIoctl;
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            type = UMINT::WdfRequestFlushBuffers;
            break;
        case IRP_MJ_QUERY_INFORMATION:
            type = UMINT::WdfRequestQueryInformation;
            break;
        case IRP_MJ_SET_INFORMATION:
            type = UMINT::WdfRequestSetInformation;
            break;
        default:
            FX_VERIFY(INTERNAL, TRAPMSG("The request type is not expected"));
            type = UMINT::WdfRequestUndefined;
        }

        pIoIrp->SetTypeForNextStackLocation(type);
        pIoIrp->Release();
    }
    else {
        FX_VERIFY(INTERNAL, TRAPMSG("Not expected"));
    }
}

VOID
FxIrp::SetMinorFunction(
    __in UCHAR /*MinorFunction*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not supported"));
}

LCID
FxIrp::GetParameterQueryDeviceTextLocaleId(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));

    return (LCID)(-1);
}

DEVICE_TEXT_TYPE
FxIrp::GetParameterQueryDeviceTextType(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));

    return (DEVICE_TEXT_TYPE)(-1);
}

BOOLEAN
FxIrp::GetParameterSetLockLock(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));

    return FALSE;
}

//
// Get Method for IO_STACK_LOCATION.Parameters.QueryId
//

BUS_QUERY_ID_TYPE
FxIrp::GetParameterQueryIdType(
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));

    return (BUS_QUERY_ID_TYPE)(-1);
}

POWER_STATE
FxIrp::GetParameterPowerState(
    )
{
    IWudfPnpIrp * pnpIrp = NULL;
    POWER_STATE powerState;






    pnpIrp = static_cast<IWudfPnpIrp *>(m_Irp);

    powerState = pnpIrp->GetPowerState();


    return powerState;
}

VOID
FxIrp::InitNextStackUsingStack(
    __in FxIrp* /*Irp*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
}




_Must_inspect_result_
NTSTATUS
FxIrp::RequestPowerIrp(
    __in MdDeviceObject  DeviceObject,
    __in UCHAR  MinorFunction,
    __in POWER_STATE  PowerState,
    __in PREQUEST_POWER_COMPLETE  CompletionFunction,
    __in PVOID  Context
    )
{
    HRESULT hr;
    IWudfDevice* deviceObject;
    IWudfDeviceStack *deviceStack;

    deviceObject = DeviceObject;
    deviceStack = deviceObject->GetDeviceStackInterface();

    hr = deviceStack->RequestPowerIrp(MinorFunction,
                                      PowerState,
                                      CompletionFunction,
                                      Context);

    if (S_OK == hr)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        PUMDF_VERSION_DATA driverVersion = deviceStack->GetMinDriverVersion();

        BOOL preserveCompat =
             deviceStack->ShouldPreserveIrpCompletionStatusCompatibility();

        return CHostFxUtil::NtStatusFromHr(
                                        hr,
                                        driverVersion->MajorNumber,
                                        driverVersion->MinorNumber,
                                        preserveCompat
                                        );
    }
}

_Must_inspect_result_
MdIrp
FxIrp::AllocateIrp(
    _In_ CCHAR StackSize,
    _In_opt_ FxDevice* Device
    )
{
    IWudfIoIrp* ioIrp;
    HRESULT hr;
    NTSTATUS status;

    ioIrp = NULL;

    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(Device));

    //
    // UMDF currently support allocating of I/O Irps only
    //
    hr = Device->GetDeviceStack()->AllocateIoIrp(Device->GetDeviceObject(),
                                                 StackSize,
                                                 &ioIrp);

    if (FAILED(hr)) {
        status = Device->NtStatusFromHr(hr);
        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
            "WDFDEVICE 0x%p Failed to allocate I/O request %!STATUS!",
            Device->GetHandle(), status);

        FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NULL(ioIrp),
            Device->GetDriverGlobals()->Public.DriverName);
    }
    else {
        FX_VERIFY_WITH_NAME(INTERNAL, CHECK_NOT_NULL(ioIrp),
            Device->GetDriverGlobals()->Public.DriverName);
    }

    return ioIrp;
}

//
// Get/Set Methods for IO_STACK_LOCATION.Parameters.DeviceCapabilities
//

PDEVICE_CAPABILITIES
FxIrp::GetParameterDeviceCapabilities(
    )
{
    IWudfPnpIrp * pQueryCapsIrp = GetPnpIrp();
    PDEVICE_CAPABILITIES deviceCapabilities;

    deviceCapabilities = pQueryCapsIrp->GetDeviceCapabilities();

    return deviceCapabilities;
}


VOID
FxIrp::SetParameterDeviceCapabilities(
    __in PDEVICE_CAPABILITIES /*DeviceCapabilities*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("Not implemented"));
}





VOID
FxIrp::SetParameterIoctlCode(
    __in ULONG /*DeviceIoControlCode*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}


VOID
FxIrp::SetParameterIoctlInputBufferLength(
    __in ULONG /*InputBufferLength*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}


VOID
FxIrp::SetParameterIoctlType3InputBuffer(
    __in PVOID /*Type3InputBuffer*/
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

VOID
FxIrp::ClearNextStack(
    )
{
    m_Irp->ClearNextStackLocation();
}

MdIrp
FxIrp::GetIrpFromListEntry(
    __in PLIST_ENTRY ListEntry
    )
{
    return g_IWudfHost2->GetIrpFromListEntry(ListEntry);
}

FxAutoIrp::~FxAutoIrp()
{
    if (m_Irp != NULL) {
        m_Irp->Release();
        m_Irp = NULL;
    }
}

MdCompletionRoutine
FxIrp::GetNextCompletionRoutine(
    VOID
    )
{

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

VOID
FxIrp::CopyToNextIrpStackLocation(
    __in PIO_STACK_LOCATION Stack
    )
{
    UNREFERENCED_PARAMETER(Stack);









    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));

}

VOID
FxIrp::SetNextIrpStackLocation(
    VOID
    )
{
    m_Irp->SetNextIrpStackLocation();
}

UCHAR
FxIrp::GetCurrentStackFlags(
    VOID
    )
{







    return 0;
}

MdFileObject
FxIrp::GetCurrentStackFileObject(
    VOID
    )
{
    return GetIoIrp()->GetFile();
}

VOID
FxIrp::SetFlags(
    __in ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);






    FX_VERIFY(INTERNAL, CHECK_TODO(Flags == 0));
}

ULONG
FxIrp::GetFlags(
    VOID
    )
{





    return 0;
}

VOID
FxIrp::SetCancel(
    __in BOOLEAN Cancel
    )
{
    UNREFERENCED_PARAMETER(Cancel);

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

CCHAR
FxIrp::GetStackCount(
    )
{

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return 0;
}

VOID
FxIrp::SetSystemBuffer(
    __in PVOID Value
    )
{
    UNREFERENCED_PARAMETER(Value);

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

PMDL*
FxIrp::GetMdlAddressPointer(
    )
{
    return NULL;
}

VOID
FxIrp::SetMdlAddress(
    __in PMDL Value
    )
{
    //
    // MDL is not supported in UMDF so must be NULL.
    //
    FX_VERIFY(INTERNAL, CHECK_NULL(Value));
}

VOID
FxIrp::SetUserBuffer(
    __in PVOID Value
    )
{
    UNREFERENCED_PARAMETER(Value);


    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

MdDeviceObject
FxIrp::GetDeviceObject(
    VOID
    )
{

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

VOID
FxIrp::SetCurrentDeviceObject(
    __in MdDeviceObject DeviceObject
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);


    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

LONGLONG
FxIrp::GetParameterWriteByteOffsetQuadPart(
    )
{




    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return 0;
}

VOID
FxIrp::SetNextParameterWriteByteOffsetQuadPart(
    __in LONGLONG DeviceOffset
    )
{
    UNREFERENCED_PARAMETER(DeviceOffset);




    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

VOID
FxIrp::SetNextParameterWriteLength(
    __in ULONG IoLength
    )
{
    UNREFERENCED_PARAMETER(IoLength);




    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

VOID
FxIrp::SetNextStackParameterOthersArgument1(
    __in PVOID Argument1
    )
{
    UNREFERENCED_PARAMETER(Argument1);

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

PVOID*
FxIrp::GetNextStackParameterOthersArgument1Pointer(
    )
{





    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

PVOID*
FxIrp::GetNextStackParameterOthersArgument2Pointer(
    )
{





    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

PVOID*
FxIrp::GetNextStackParameterOthersArgument4Pointer(
    )
{





    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

MdFileObject
FxIrp::GetFileObject(
    VOID
    )
{
    IWudfIoIrp * pIoIrp = NULL;

    HRESULT hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIoIrp));
    pIoIrp->Release();

    return pIoIrp->GetFile();
}

ULONG
FxIrp::GetParameterIoctlCode(
    VOID
    )
{
    IWudfIoIrp * ioIrp = NULL;
    ULONG ioControlCode = 0;

    if (GetMajorFunction() == IRP_MJ_DEVICE_CONTROL) {
        ioIrp = GetIoIrp();
        ioIrp->GetDeviceIoControlParameters(&ioControlCode, NULL, NULL);
    }

    return ioControlCode;
}

ULONG
FxIrp::GetParameterIoctlCodeBufferMethod(
    VOID
    )
{
    //
    // For UMDF, always return METHOD_BUFFERED. This is because merged code
    // uses this info to decide how and where to fetch the buffers from, from
    // inside the irp, and for UMDF, the buffers are always fetched from host in
    // same manner irrespective of IOCTL type.
    //
    return METHOD_BUFFERED;
}

ULONG
FxIrp::GetParameterIoctlOutputBufferLength(
    VOID
    )
{
    IWudfIoIrp * ioIrp = NULL;
    ULONG outputBufferLength;

    ioIrp = GetIoIrp();
    ioIrp->GetDeviceIoControlParameters(NULL, NULL, &outputBufferLength);

    return outputBufferLength;
}

ULONG
FxIrp::GetParameterIoctlInputBufferLength(
    VOID
    )
{
    IWudfIoIrp * ioIrp = NULL;
    ULONG inputBufferLength;

    ioIrp = GetIoIrp();
    ioIrp->GetDeviceIoControlParameters(NULL, &inputBufferLength, NULL);

    return inputBufferLength;
}

VOID
FxIrp::SetParameterIoctlOutputBufferLength(
    __in ULONG OutputBufferLength
    )
{
    UNREFERENCED_PARAMETER(OutputBufferLength);

    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
}

PVOID
FxIrp::GetParameterIoctlType3InputBuffer(
    VOID
    )
{





    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

VOID
FxIrp::SetNextStackFlags(
    __in UCHAR Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);








    DO_NOTHING();
}

VOID
FxIrp::SetNextStackFileObject(
    _In_ MdFileObject FileObject
    )
{
    GetIoIrp()->SetFileForNextIrpStackLocation(FileObject);
}

VOID
FxIrp::ClearNextStackLocation(
    VOID
    )
{
    m_Irp->ClearNextStackLocation();
}

ULONG
FxIrp::GetParameterReadLength(
    VOID
    )
{
    ULONG length;

    GetIoIrp()->GetReadParameters(&length, NULL, NULL);

    return length;
}

ULONG
FxIrp::GetParameterWriteLength(
    VOID
    )
{
    ULONG length;

    GetIoIrp()->GetWriteParameters(&length, NULL, NULL);

    return length;
}

ULONG
FxIrp::GetCurrentFlags(
    VOID
     )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return 0;
}

PVOID
FxIrp::GetCurrentParametersPointer(
     VOID
     )
{
   FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
   return NULL;
}

MdEThread
FxIrp::GetThread(
    VOID
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return  NULL;
}

BOOLEAN
FxIrp::Is32bitProcess(
    VOID
    )
{
    return (GetIoIrp()->IsFrom32BitProcess() ? TRUE : FALSE);
}

VOID
FxIrp::FreeIrp(
     VOID
     )
{
    //
    // Release the um com irp creation ref
    //
    m_Irp->Release();

    //
    // This is equivalent to IoFreeIrp in km.
    //
    GetIoIrp()->Deallocate();
}

PIO_STATUS_BLOCK
FxIrp::GetStatusBlock(
     VOID
     )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

PVOID
FxIrp::GetDriverContext(
     VOID
     )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return NULL;
}

ULONG
FxIrp::GetDriverContextSize(
     VOID
     )
{
    FX_VERIFY(INTERNAL, TRAPMSG("To be implemented"));
    return 0;
}

VOID
FxIrp::CopyParameters(
    _Out_ PWDF_REQUEST_PARAMETERS Parameters
    )
{
    IWudfIoIrp* ioIrp;
    UCHAR majorFunction;

    ioIrp = GetIoIrp();
    majorFunction = GetMajorFunction();

    switch (majorFunction) {
    case IRP_MJ_CREATE:
        ioIrp->GetCreateParameters(
                                   &Parameters->Parameters.Create.Options,
                                   &Parameters->Parameters.Create.FileAttributes,
                                   &Parameters->Parameters.Create.ShareAccess,
                                   NULL // ACCESS_MASK*
                                   );
        break;
    case IRP_MJ_READ:
        ioIrp->GetReadParameters((ULONG*)&Parameters->Parameters.Read.Length,
                                 &Parameters->Parameters.Read.DeviceOffset,
                                 &Parameters->Parameters.Read.Key);
        break;
    case IRP_MJ_WRITE:
        ioIrp->GetWriteParameters((ULONG*)&Parameters->Parameters.Write.Length,
                                 &Parameters->Parameters.Write.DeviceOffset,
                                 &Parameters->Parameters.Write.Key);
        break;
    case IRP_MJ_DEVICE_CONTROL:
        ioIrp->GetDeviceIoControlParameters(
             &Parameters->Parameters.DeviceIoControl.IoControlCode,
             (ULONG*)&Parameters->Parameters.DeviceIoControl.InputBufferLength,
             (ULONG*)&Parameters->Parameters.DeviceIoControl.OutputBufferLength
             );
        break;
    default:
        FX_VERIFY(INTERNAL, TRAPMSG("Not expected"));
        break;
    }

    return;
}

VOID
FxIrp::CopyStatus(
    _Out_ PIO_STATUS_BLOCK StatusBlock
    )
{
    StatusBlock->Status = GetStatus();
    StatusBlock->Information = GetInformation();
}

BOOLEAN
FxIrp::HasStack(
    _In_ UCHAR StackCount
    )
{
    UNREFERENCED_PARAMETER(StackCount);







    return TRUE;
}

BOOLEAN
FxIrp::IsCurrentIrpStackLocationValid(
    VOID
    )
{








    return TRUE;
}

IWudfIoIrp*
FxIrp::GetIoIrp(
    VOID
    )
{
    IWudfIoIrp* pIoIrp;
    HRESULT hrQI;

    hrQI = m_Irp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIoIrp));
    pIoIrp->Release();

    //
    // Now that we confirmed the irp is an io irp, just return the underlying
    // irp.
    //
    return static_cast<IWudfIoIrp*>(m_Irp);
}


IWudfPnpIrp*
FxIrp::GetPnpIrp(
    VOID
    )
{
    IWudfPnpIrp* pPnpIrp;
    HRESULT hrQI;

    hrQI = m_Irp->QueryInterface(IID_IWudfPnpIrp, (PVOID*)&pPnpIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pPnpIrp));
    pPnpIrp->Release();

    //
    // Now that we confirmed the irp is a pnp irp, just return the underlying
    // irp.
    //
    return static_cast<IWudfPnpIrp*>(m_Irp);
}

