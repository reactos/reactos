/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXUSBDEVICE object
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Device type bits */
#define UCX_DEVICE_TYPE_HUB        0x1
#define UCX_DEVICE_TYPE_ROOT_HUB   0x2

/* Newer controller drivers pass one more slot than the 1.6 structure */
struct UcxUsbDeviceCallbacks
{
    UCX_USBDEVICE_EVENT_CALLBACKS Public;
    PVOID EndpointAddEx;
};

/* Opaque to the HCD; lives on the UCX stack while EvtControllerUsbDeviceAdd runs */
struct _UCXUSBDEVICE_INIT
{
    UCXUSBDEVICE ParentHub;
    UCXUSBDEVICE Created;
    UCXUSBDEVICE_INFO Info;
    UCXHUB_DEVICE_CONTEXT HubDeviceContext;
    UcxUsbDeviceCallbacks Callbacks;
};

typedef struct _UCXUSBDEVICE_INIT UcxUsbDeviceInit;

/* One armed remote wake notification */
struct UcxRemoteWakeSlot
{
    WDFREQUEST Request;
    PREQUEST_REMOTE_WAKE_NOTIFICATION Params;
};

/* One function of a composite device; Index must stay the first member */
struct UcxFunctionRecord
{
    ULONG Index;
    UcxUsbDevice* Device;
    PDEVICE_OBJECT ClientPdo;
    UcxRemoteWakeSlot Wake;
};

class UcxUsbDevice
{
public:
    /* Run when the last endpoint finished its part of a hub operation */
    typedef VOID (UcxUsbDevice::*PFN_UCX_OPERATION_DONE)();

    /* Exports */

    static
    VOID
    InitSetEventCallbacks(
        _Inout_ PUCXUSBDEVICE_INIT Init,
        _In_ PUCX_USBDEVICE_EVENT_CALLBACKS Callbacks);

    _Must_inspect_result_
    static
    NTSTATUS
    Create(
        _In_ UCXCONTROLLER Controller,
        _Inout_ PUCXUSBDEVICE_INIT* Init,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _Out_ UCXUSBDEVICE* UsbDevice);

    VOID
    RemoteWakeNotification(
        _In_ ULONG Interface);

    /* Hub interface */

    _Must_inspect_result_
    static
    NTSTATUS
    CreateFromHub(
        _In_ UCXUSBDEVICE Hub,
        _In_ PUCXHUB_DEVICE_CREATE_INFO Info,
        _Out_ UCXUSBDEVICE* UsbDevice);

    VOID
    DeleteFromHub();

    /** Port 0 means this hub itself went away. */
    VOID
    DisconnectPort(
        _In_ ULONG PortNumber);

    VOID
    SetPdo(
        _In_ PDEVICE_OBJECT Pdo);

    ULONG64
    Timestamp() const
    {
        return m_Timestamp;
    }

    /* Lookups and flags */

    static
    UcxUsbDevice*
    FromHandle(
        _In_ UCXUSBDEVICE Handle);

    BOOLEAN
    IsRootHub() const
    {
        return (m_Type & UCX_DEVICE_TYPE_ROOT_HUB) != 0;
    }

    BOOLEAN
    IsHub() const
    {
        return (m_Type & UCX_DEVICE_TYPE_HUB) != 0;
    }

    USB_DEVICE_SPEED
    Speed() const
    {
        return m_Info.DeviceSpeed;
    }

    BOOLEAN
    AllowsEndpointStart() const
    {
        return !m_Disconnected && !m_DeprogrammedByControllerReset &&
               m_Enabled && m_EndpointsMayLeavePurge;
    }

    /** Root hub setup; the device context lives on the UCXROOTHUB object. */
    VOID
    InitializeAsRootHub(
        _In_ UCXROOTHUB Handle,
        _In_ UcxController* Controller);

    /* Tree walks, called with the topology lock held */

    VOID
    WalkSubtree(
        _In_ PFN_UCX_DEVICE_VISITOR Visitor,
        _In_opt_ PVOID Context,
        _In_ BOOLEAN IncludeDisconnected);

    /* Device management, see devicemgmt.cpp */

    VOID
    DispatchManagement(
        _In_ WDFREQUEST Request,
        _In_ ULONG IoControlCode);

    /** Controller reset in progress or device deprogrammed. */
    VOID
    FailManagement(
        _In_ WDFREQUEST Request,
        _In_ ULONG IoControlCode);

    NTSTATUS
    OnHcdEnableDone(
        _In_ PIRP Irp,
        _In_ PUSBDEVICE_ENABLE Enable);

    NTSTATUS
    OnHcdResetDone(
        _In_ PIRP Irp,
        _In_ PUSBDEVICE_RESET Reset);

    NTSTATUS
    OnHcdEndpointsConfigureDone(
        _In_ PIRP Irp,
        _In_ PENDPOINTS_CONFIGURE Configure);

    /** An endpoint finished its part of the current hub operation. */
    VOID
    CompleteHubOperation();

    /* UCX default queue requests */

    VOID
    RegisterComposite(
        _In_ WDFREQUEST Request);

    VOID
    UnregisterComposite(
        _In_ WDFREQUEST Request);

    VOID
    SetFunctionData(
        _In_ WDFREQUEST Request);

    VOID
    RequestRemoteWakeNotification(
        _In_ WDFREQUEST Request);

    /* Other services */

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    EnableForwardProgress(
        _In_ PUCXHUB_FORWARD_PROGRESS_INFO Info);

    VOID
    TrackStaleHandles();

    /** Called by failing transfer completions; tells the parent hub once per error. */
    VOID
    ReportNoPingResponseIfPending();

public:
    UCXUSBDEVICE m_Handle;
    UcxController* m_Controller;
    UcxUsbDevice* m_ParentHub;
    UCXHUB_DEVICE_CONTEXT m_HubDeviceContext;
    ULONG m_Type;
    USHORT m_BcdUsb;
    UCXUSBDEVICE_INFO m_Info;
    UcxUsbDeviceCallbacks m_Callbacks;
    PDEVICE_OBJECT m_Pdo;
    ULONG64 m_Timestamp;

    /* Topology lock */
    LIST_ENTRY m_ChildList;
    LIST_ENTRY m_ChildLink;
    LIST_ENTRY m_EndpointList;
    LIST_ENTRY m_StaleEndpointList;
    BOOLEAN m_Disconnected;
    BOOLEAN m_ChildrenMayLeavePurge;
    BOOLEAN m_EndpointsMayLeavePurge;

    /* Strict pipe handle validation, its own lock inside the topology lock */
    KSPIN_LOCK m_TrackingLock;
    LIST_ENTRY m_TrackingList;
    BOOLEAN m_StalePipeIoAllowed;

    UcxEndpoint* m_DefaultEndpoint;
    UcxPipe* m_DefaultPipe;

    BOOLEAN m_Enabled;
    BOOLEAN m_DeprogrammedByControllerReset;
    BOOLEAN m_PendingDelete;

    /* Set by the fail path, consumed by the device management completion routine */
    BOOLEAN m_ResetFailedByControllerReset;
    BOOLEAN m_EndpointResetFailedByControllerReset;
    BOOLEAN m_FailNextStreamClose;

    /* One hub operation at a time; the hub serializes them per device */
    PVOID m_PendingOperation;
    PFN_UCX_OPERATION_DONE m_PendingOperationDone;
    LONG m_PendingOperationCount;

    LONG m_PendingNoPingResponse;
    ULONG m_TransferFailureCount;

    /* What the hub gave us in its stack interface query, on hub devices */
    UCXHUB_HUB_CONTEXT m_HubContext;
    PFN_UCXHUB_CLEAR_TT_BUFFER m_HubClearTtBuffer;
    PFN_UCXHUB_NO_PING_RESPONSE m_HubNoPingResponse;
    BOOLEAN m_HubUsesGrownInterface;

    /* Composite functions and remote wake */
    KSPIN_LOCK m_RemoteWakeLock;
    UcxRemoteWakeSlot m_RemoteWake;
    ULONG m_FunctionCount;
    UcxFunctionRecord* m_Functions;
    WDFMEMORY m_FunctionsMemory;

    /* USBD client handles registered against this device */
    KSPIN_LOCK m_UsbdHandleLock;
    LIST_ENTRY m_UsbdHandleList;

private:
    VOID
    InitializeLists();

    /* Pending operation helpers */

    VOID
    StartOperation(
        _In_ PVOID Pending,
        _In_ PFN_UCX_OPERATION_DONE Done,
        _In_ LONG Count);

    PVOID
    TakeOperation()
    {
        PVOID Pending = m_PendingOperation;

        m_PendingOperation = NULL;
        return Pending;
    }

    VOID
    FanOutToEndpoints(
        _In_ EpEvent Event);

    VOID
    HandleHubPurgeIo(
        _In_ WDFREQUEST Request,
        _In_ BOOLEAN OnSuspend);

    VOID
    HandleHubStartIo(
        _In_ WDFREQUEST Request);

    VOID
    HandleHubAbortIo(
        _In_ WDFREQUEST Request);

    VOID
    HandleHubTreePurge(
        _In_ WDFREQUEST Request);

    VOID
    HandleHubReset(
        _In_ WDFREQUEST Request,
        _In_ BOOLEAN AfterReset);

    VOID
    HandleHubDisable(
        _In_ WDFREQUEST Request,
        _In_ BOOLEAN AfterReset);

    VOID
    HandleHubEndpointsConfigure(
        _In_ WDFREQUEST Request,
        _In_ BOOLEAN AfterReset);

    VOID
    UpdateFromHub(
        _In_ WDFREQUEST Request);

    /* Pending operation completions */

    VOID SendResetToController();
    VOID PendReset();
    VOID SendDisableToController();
    VOID PendDisable();
    VOID SendEndpointsConfigureToController();
    VOID PendEndpointsConfigure();
    VOID CompleteHubRequest();
    VOID CompleteEnableIrp();
    VOID CompleteEndpointsConfigureIrp();
    VOID FinishEndpointResetRequest();

    static
    EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;

    static
    EVT_WDF_REQUEST_CANCEL EvtRemoteWakeCancel;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxUsbDevice, UcxGetUsbDeviceContext);

/* Controller queue handlers owned by this module */
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtDeviceMgmtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtTreePurgeIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtPendDuringResetIoInternalDeviceControl;
