/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXCONTROLLER object
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class UcxRootHub;
class UcxUsbDevice;
class UcxEndpoint;

/* Callbacks carried in the Reserved slots of UCX_CONTROLLER_CONFIG */
typedef
NTSTATUS
(NTAPI *PFN_UCX_HCD_GET_BANDWIDTH_INFORMATION)(
    _In_ UCXCONTROLLER Controller);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCX_HCD_ENABLE_FORWARD_PROGRESS)(
    _In_ UCXCONTROLLER Controller);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCX_HCD_GET_DUMP_DATA)(
    _In_ UCXCONTROLLER Controller,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ PVOID DumpDeviceInfo,
    _Out_ PVOID DumpControllerInfo);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
VOID
(NTAPI *PFN_UCX_HCD_FREE_DUMP_DATA)(
    _In_ UCXCONTROLLER Controller,
    _Inout_ PVOID DumpControllerInfo);

/* Accepted UCX_CONTROLLER_CONFIG sizes, oldest first */
#define UCX_CONFIG_SIZE_V1 FIELD_OFFSET(UCX_CONTROLLER_CONFIG, ManufacturerNameString)
#define UCX_CONFIG_SIZE_V3 FIELD_OFFSET(UCX_CONTROLLER_CONFIG, EvtControllerGetTransportCharacteristics)
#define UCX_CONFIG_SIZE_V4 sizeof(UCX_CONTROLLER_CONFIG)

/* Forward progress work item, also used by the reset machine to reach PASSIVE_LEVEL */
struct _UCXHUB_WORKITEM
{
    LIST_ENTRY ThreadQueueEntry;
    UcxController* Controller;
    PDEVICE_OBJECT DeviceObject;
    PIO_WORKITEM IoWorkItem;
    PUCXHUB_WORKITEM_ROUTINE Routine;
    PVOID RoutineContext;
    KSPIN_LOCK Lock;
    ULONG RunningCount;
    KEVENT Idle;
    BOOLEAN NeedsFlush;
    BOOLEAN Queued;

    static
    PUCXHUB_WORKITEM
    Allocate(
        _In_ UcxController* Controller,
        _In_ PDEVICE_OBJECT DeviceObject,
        _In_ ULONG Flags);

    VOID
    Free();

    VOID
    Enqueue(
        _In_ PUCXHUB_WORKITEM_ROUTINE NewRoutine,
        _In_opt_ PVOID NewContext,
        _In_ UCXHUB_WORKITEM_ENQUEUE_OPTIONS Options);

    VOID
    Flush();

    VOID
    Run();
};

typedef struct _UCXHUB_WORKITEM UcxWorkItem;

/** Context of the address 0 ownership queue: who holds address 0 right now. */
struct UcxAddress0QueueContext
{
    UcxController* Controller;
    UCXUSBDEVICE OwnerHub;
    UCXUSBDEVICE OwnerDevice;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxAddress0QueueContext, UcxGetAddress0QueueContext);

/** Context ucx01000 attaches to the controller driver's FDO. */
struct UcxFdoContext
{
    UcxController* Controller;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxFdoContext, UcxGetFdoContext);

/* Tree walk callback, run under the topology lock */
typedef
VOID
(NTAPI *PFN_UCX_DEVICE_VISITOR)(
    _In_ UcxUsbDevice* Device,
    _In_opt_ PVOID Context);

class UcxController
{
public:
    /* Exports */

    _Must_inspect_result_
    static
    NTSTATUS
    Create(
        _In_ PUCX_DRIVER_GLOBALS Globals,
        _In_ WDFDEVICE Device,
        _In_ PUCX_CONTROLLER_CONFIG Config,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _Out_ UCXCONTROLLER* Controller);

    VOID
    NeedsReset();

    VOID
    ResetComplete(
        _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO Info);

    VOID
    SetFailed();

    NTSTATUS
    SetIdStrings(
        _In_ PUNICODE_STRING Manufacturer,
        _In_ PUNICODE_STRING ModelName,
        _In_ PUNICODE_STRING ModelNumber);

    VOID
    NotifyTransportCharacteristicsChange(
        _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS Characteristics);

    /* Lookups */

    static
    UcxController*
    FromHandle(
        _In_ UCXCONTROLLER Handle);

    static
    UcxController*
    FromFdo(
        _In_ WDFDEVICE Fdo);

    /* Reset references: keep a controller reset from starting while held */

    BOOLEAN
    BlockReset();

    VOID
    UnblockReset();

    VOID
    MarkResetInProgress();

    VOID
    MarkResetFinished();

    BOOLEAN
    IsResetInProgress() const
    {
        return m_ResetInProgress;
    }

    BOOLEAN
    HasFailed() const
    {
        return m_Failed != 0;
    }

    NTSTATUS
    GetCurrentFrameNumber(
        _Out_ PULONG FrameNumber);

    /* HCD capability queries */

    BOOLEAN
    QueryClearTtBufferOnCancel();

    BOOLEAN
    QueryStreamsSupported();

    /* Controller reset machine glue */

    VOID
    PostResetEvent(
        _In_ CrEvent Event)
    {
        m_ResetMachine.SmPost(Event);
    }

    VOID
    PrepareDevicesForReset();

    VOID
    NotifyDevicesResetDone();

    /** An endpoint finished preparing for the reset. */
    VOID
    PrepareForResetOperationDone();

    /* Device tree, walked under the topology lock */

    VOID
    WalkDevices(
        _In_ PFN_UCX_DEVICE_VISITOR Visitor,
        _In_opt_ PVOID Context,
        _In_ BOOLEAN IncludeDisconnected);

    /* Hub interface functions that act on the controller */

    VOID
    Address0Release(
        _In_ UCXUSBDEVICE Hub);

    NTSTATUS
    StopIdle(
        _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context);

    VOID
    ResumeIdle(
        _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context);

    VOID
    GetInfo(
        _Out_ PVOID Info,
        _In_ BOOLEAN Grown);

    /* NOTIFY_FORWARD_PROGRESS */

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    EnableForwardProgress(
        _In_ PUCXHUB_FORWARD_PROGRESS_INFO Info);

    VOID
    QueueOnWorkerThread(
        _In_ UcxWorkItem* WorkItem);

    BOOLEAN
    HasWorkerThread() const
    {
        return m_WorkerThread != NULL;
    }

    /* Private config slots */

    PFN_UCX_HCD_GET_BANDWIDTH_INFORMATION
    HcdGetBandwidthInformation() const
    {
        return (PFN_UCX_HCD_GET_BANDWIDTH_INFORMATION)m_Config.Reserved1;
    }

    PFN_UCX_HCD_ENABLE_FORWARD_PROGRESS
    HcdEnableForwardProgress() const
    {
        return (PFN_UCX_HCD_ENABLE_FORWARD_PROGRESS)m_Config.Reserved2;
    }

    PFN_UCX_HCD_GET_DUMP_DATA
    HcdGetDumpData() const
    {
        return (PFN_UCX_HCD_GET_DUMP_DATA)m_Config.Reserved3;
    }

    PFN_UCX_HCD_FREE_DUMP_DATA
    HcdFreeDumpData() const
    {
        return (PFN_UCX_HCD_FREE_DUMP_DATA)m_Config.Reserved4;
    }

public:
    UCXCONTROLLER m_Handle;
    WDFDEVICE m_Fdo;
    UCX_CONTROLLER_CONFIG m_Config;
    LIST_ENTRY m_DriverListEntry;

    /* Queues on the controller driver's FDO, see hostqueues.cpp */
    WDFQUEUE m_Address0Queue;
    WDFQUEUE m_DeviceMgmtQueue;
    WDFQUEUE m_TreePurgeQueue;
    WDFQUEUE m_PendDuringResetQueue;
    WDFQUEUE m_DefaultQueue;

    /* Symbolic link of GUID_DEVINTERFACE_USB_HOST_CONTROLLER */
    WDFSTRING m_HostControllerInterfaceName;

    UcxRootHub* m_RootHub;
    DEVICE_CAPABILITIES m_HcCaps;

    /* Cancel synchronization for parked abort pipe URBs */
    IO_CSQ m_AbortPipeCsq;
    KSPIN_LOCK m_AbortPipeCsqLock;

    /* Guards every device and endpoint list of the tree */
    KSPIN_LOCK m_TopologyLock;
    ULONG m_ChildDeviceCount;
    ULONG m_ChildEndpointCount;

    ControllerResetMachine m_ResetMachine;
    BOOLEAN m_ResetMachineReady;
    UcxWorkItem* m_ResetMachineWorkItem;

    KEVENT m_RootHubMayExitD0;
    KEVENT m_ResetCompleteProcessed;
    LONG m_DeviceContextsLost;
    LONG m_RootHubResetSeen;
    BOOLEAN m_WaitOnResetComplete;
    BOOLEAN m_RootHubInD0;

    KSPIN_LOCK m_ResetLock;
    ULONG m_ResetGeneration;
    LONG m_ResetBlockCount;
    BOOLEAN m_ResetInProgress;
    BOOLEAN m_ResetInitiated;
    LONG m_Failed;
    LONG m_PendingPrepareForReset;

    ULONG m_CachedFrameNumber;
    ULONG m_SyntheticFrame;

    BOOLEAN m_DriverVerifierEnabled;
    BOOLEAN m_ClearTtBufferOnAsyncCancel;

    /* Forward progress system thread */
    KSPIN_LOCK m_WorkerLock;
    LIST_ENTRY m_WorkerItems;
    PKTHREAD m_WorkerThread;
    KEVENT m_WorkerWake;
    BOOLEAN m_WorkerParked;
    BOOLEAN m_WorkerExit;
    KMUTEX m_ForwardProgressMutex;
    BOOLEAN m_HcdReservedIoReady;

    LONG m_UsbdInterfaceCount;

    /* One tree purge at a time, see devobj.cpp */
    WDFREQUEST m_PendingTreePurge;
    LONG m_PendingTreePurgeEndpoints;

    /* UcxControllerSetIdStrings, copies owned by the controller */
    UNICODE_STRING m_ManufacturerName;
    UNICODE_STRING m_ModelName;
    UNICODE_STRING m_ModelNumber;

private:
    NTSTATUS
    Initialize(
        _In_ PUCX_DRIVER_GLOBALS Globals,
        _In_ WDFDEVICE Fdo,
        _In_ const UCX_CONTROLLER_CONFIG* Config);

    NTSTATUS
    CreateQueues();

    NTSTATUS
    QueryHcCapabilities();

    NTSTATUS
    RegisterWmi();

    static
    EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;

    VOID
    Destroy();

    NTSTATUS
    StartWorkerThread();

    static
    KSTART_ROUTINE WorkerThreadRoutine;

    static
    UCXHUB_WORKITEM_ROUTINE ResetMachinePassiveRoutine;

    friend class ControllerResetMachine;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxController, UcxGetControllerContext);

/* Queue handlers, hostqueues.cpp */
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtAddress0IoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtDefaultIoInternalDeviceControl;
EVT_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS UcxEvtDefaultQueueExamineIrp;

/* Abort pipe cancel safe queue callbacks */
IO_CSQ_INSERT_IRP UcxCsqInsertAbortIrp;
IO_CSQ_REMOVE_IRP UcxCsqRemoveAbortIrp;
IO_CSQ_PEEK_NEXT_IRP UcxCsqPeekAbortIrp;
IO_CSQ_ACQUIRE_LOCK UcxCsqAcquireAbortLock;
IO_CSQ_RELEASE_LOCK UcxCsqReleaseAbortLock;
IO_CSQ_COMPLETE_CANCELED_IRP UcxCsqCompleteCanceledAbortIrp;
