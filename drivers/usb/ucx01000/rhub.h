/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXROOTHUB object and the root hub PDO
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Port change bookkeeping behind the root hub interrupt (status change) endpoint */
struct UcxInterruptQueueState
{
    KSPIN_LOCK PortChangeLock;

    /* 16 bit generations, only compared for equality */
    USHORT PortChangeGeneration;
    USHORT PortChangeGenerationProcessed;

    /* The status change transfer UCX holds instead of passing it to the HCD */
    WDFREQUEST HeldTransfer;
    BOOLEAN LastTransferCanceled;

    /* S0 idle: port changes must wake the root hub PDO */
    BOOLEAN IndicateWakeEnabled;
    KEVENT NoIndicateWakeInProgress;

    UcxRootHub* RootHub;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxInterruptQueueState, UcxGetInterruptQueueState);

struct UcxControlQueueContext
{
    UcxRootHub* RootHub;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxControlQueueContext, UcxGetControlQueueContext);

struct UcxRootHubPdoContext
{
    UcxRootHub* RootHub;
    UcxController* Controller;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxRootHubPdoContext, UcxGetRootHubPdoContext);

class UcxRootHub
{
public:
    /* Exports */

    _Must_inspect_result_
    static
    NTSTATUS
    Create(
        _In_ UCXCONTROLLER Controller,
        _In_ PUCX_ROOTHUB_CONFIG Config,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _Out_ UCXROOTHUB* RootHub);

    /** Port change from the HCD, or one the reset machine simulates. */
    VOID
    PortChanged();

    static
    UcxRootHub*
    FromHandle(
        _In_ UCXROOTHUB Handle);

    UcxUsbDevice*
    Device() const
    {
        return m_Device;
    }

    /* Controller reset hooks */

    VOID
    FailIo();

    VOID
    ResumeIo();

    VOID
    FinishPortResetRequest(
        _In_ BOOLEAN Succeeded);

    /* Forward progress for the root hub needs nothing, its queues reserve a request */
    NTSTATUS
    EnableForwardProgress()
    {
        return STATUS_SUCCESS;
    }

    /* UCX default queue, ROOTHUB_GET_INFO */
    VOID
    DispatchGetInfo(
        _In_ WDFREQUEST Request,
        _Inout_ PUCXHUB_ROOTHUB_INFO Info);

    /* PDO state readers, for the user mode IOCTLs */

    NTSTATUS
    ReferenceSymbolicName(
        _Out_ WDFSTRING* Name);

    WDFDEVICE
    Pdo() const
    {
        return m_Pdo;
    }

public:
    UCXROOTHUB m_Handle;
    UcxController* m_Controller;
    UCX_ROOTHUB_CONFIG m_Config;

    /* The root hub as a USB device, the second context on the UCXROOTHUB */
    UcxUsbDevice* m_Device;

    UcxEndpoint* m_ControlEndpoint;
    UcxEndpoint* m_InterruptEndpoint;
    WDFQUEUE m_ControlQueue;
    WDFQUEUE m_InterruptQueue;
    UcxInterruptQueueState* m_InterruptState;

    USHORT m_NumberOf20Ports;
    USHORT m_NumberOf30Ports;

    WDF_DEVICE_PNP_CAPABILITIES m_PnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES m_PowerCaps;

    /* Guards the PDO handle, the started flag and the symbolic name */
    KSPIN_LOCK m_PdoInfoLock;
    WDFDEVICE m_Pdo;
    BOOLEAN m_PdoStarted;
    WDFSTRING m_SymbolicName;

    /* At most one IOCTL_UCXHUB_RESET_PORT_ASYNC */
    PIRP m_PendingAsyncReset;

    SYSTEM_POWER_STATE m_LastSystemSleepState;
    POWER_ACTION m_SystemPowerAction;

private:
    NTSTATUS
    Initialize(
        _In_ UcxController* Controller,
        _In_ PUCX_ROOTHUB_CONFIG Config);

    NTSTATUS
    CreateEndpoint(
        _In_ BOOLEAN Interrupt);

    NTSTATUS
    CreatePdo();

    VOID
    BuildPowerCapabilities();

    NTSTATUS
    AddQueryInterfaces();

    static
    EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanup;

    friend class UcxRootHubPdo;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxRootHub, UcxGetRootHubContext);

/* Root hub queues, rhub.cpp */
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtRootHubInterruptTransfer;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL UcxEvtRootHubControlTransfer;
EVT_WDF_REQUEST_CANCEL UcxEvtRootHubHeldTransferCancel;

/* Root hub PDO callbacks, rhubpdo.cpp */
EVT_WDF_DEVICE_PREPARE_HARDWARE UcxEvtRootHubPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE UcxEvtRootHubReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY UcxEvtRootHubD0Entry;
EVT_WDF_DEVICE_D0_EXIT UcxEvtRootHubD0Exit;
EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS UcxEvtRootHubEnableWakeAtBus;
EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS UcxEvtRootHubDisableWakeAtBus;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS UcxEvtRootHubPreprocessInternalIoctl;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS UcxEvtRootHubPreprocessQueryInterface;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS UcxEvtRootHubPreprocessSetPower;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL UcxEvtRootHubPdoIoDeviceControl;

/* Query interfaces, hubinterface.cpp */
EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST UcxEvtQueryParentInterface;
EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST UcxEvtQueryStackInterface;
EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST UcxEvtQueryUsbdiInterface;
EVT_WDF_DEVICE_PROCESS_QUERY_INTERFACE_REQUEST UcxEvtQueryUsbdClientInterface;
