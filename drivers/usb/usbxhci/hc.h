/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller object, errata, recovery and the UCX controller callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/** What a fatal error asks the recovery worker to do. */
enum class XhciRecovery : ULONG
{
    Ignore,
    ResetHost,
    Disable,
    Gone,
    HoldStopped,
    InternalReset
};

#define XHCI_FIRMWARE_UNKNOWN           0xFFFFFFFFFFFFFFFFULL

/** Parent bus identity. PCI ids are 0x7FFFFFFF for ACPI parents. */
struct XhciIdentity
{
    UCX_CONTROLLER_PARENT_BUS_TYPE ParentBus;
    ULONG PciVendorId;
    ULONG PciDeviceId;
    UCHAR PciRevisionId;
    USHORT PciSubsystemVendorId;
    USHORT PciSubsystemId;
    ULONG PciBus;
    ULONG PciDevice;
    ULONG PciFunction;
    CHAR AcpiVendorId[MAX_VENDOR_ID_STRING_LENGTH];
    CHAR AcpiDeviceId[MAX_DEVICE_ID_STRING_LENGTH];
    CHAR AcpiRevisionId[MAX_REVISION_ID_STRING_LENGTH];

    /** All ones while unknown. */
    ULONG64 FirmwareVersion;

    /** Kept for disabling the device; Context is NULL when absent. */
    BUS_INTERFACE_STANDARD PciInterface;
};

/** Lives in the UCXCONTROLLER context and embeds every module object. */
class XhciController
{
public:
    static XhciController*
    FromDevice(
        _In_ WDFDEVICE Device);
    static XhciController*
    FromUcx(
        _In_ UCXCONTROLLER UcxController);

    BOOLEAN
    HasErrata(
        _In_ XhciErrata Bit) const
    {
        return (m_Errata & XhciErrataBit(Bit)) != 0;
    }

    /** Only bits 0 and 12 may be set after population. */
    VOID
    SetErrata(
        _In_ XhciErrata Bit);

    /** Registers mapped and the controller not marked gone. */
    BOOLEAN IsAccessible() const;
    VOID MarkGone();

    /** Any IRQL up to DISPATCH_LEVEL; the work runs on the recovery work item. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    RaiseControllerFault(
        _In_ XhciRecovery Action,
        _In_ ULONG Reason);

    WDFDEVICE m_Device;
    PDEVICE_OBJECT m_WdmDevice;
    UCXCONTROLLER m_Ucx;
    ULONG64 m_Errata;

    XhciRegisters m_Registers;
    XhciCommonBuffer m_Buffers;
    XhciSlotTable m_Slots;
    XhciInterrupters m_Interrupters;
    XhciCommandRing m_Commands;
    XhciRootHub m_RootHub;


    /** Who the controller is. */
    XhciIdentity m_Identity;

    /** Internal fixes keyed by PCI id, bits of m_InternalFlags. */
    enum : ULONG64
    {
        FixFrescoShadow = 0x01,
        FixFrescoControl = 0x02,
        FixEtronSet = 0x04,
        FixEtronClear = 0x08,
        FixAsmediaResetDelay = 0x10
    };
    ULONG64 m_InternalFlags;

    /** Set by UCX through EvtControllerEnableForwardProgress; read by transfer code. */
    BOOLEAN m_ReservedIoArmed;

    /** D0 only after a successful D0 entry; the target state from D0 exit start. */
    WDF_POWER_DEVICE_STATE m_PowerState;

    /** Tracked system power action. */
    enum class SystemAction : ULONG
    {
        Boot = 0,
        ResumeFromSleep = 1,
        ResumeFromHibernate = 2,
        Sleep = 3,
        HybridSleep = 4,
        Hibernate = 5,
        Shutdown = 6,
        None = 7
    };
    SystemAction m_SystemAction;

    /** Errata bit 34 pad buffer, NULL when absent. */
    XhciDmaBuffer* m_SplitPadBuffer;

    /** 32 bit frame number, Increment microframes ahead. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    ULONG
    GetFrameNumber(
        _In_ ULONG Increment);

    /** DPRINT1 a hardware verifier condition; breaks in when that flag is enabled. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    VerifierCheck(
        _In_ ULONG Flag);

    /** Root hub PDO reached D0: switch to the short idle timeout. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID OnRootHubD0Entry();

    /** Port connect state for the power engine. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    NotifyPortConnectState(
        _In_ BOOLEAN AnyConnected);

    /* Event handlers; the KMDF and UCX callbacks in hc.cpp forward here */
    NTSTATUS
    Initialize(
        _In_ WDFDEVICE Device,
        _In_ UCXCONTROLLER Ucx,
        _In_ const XhciIdentity* Identity);
    NTSTATUS
    PrepareHardware(
        _In_ WDFCMRESLIST Raw,
        _In_ WDFCMRESLIST Translated);
    VOID ReleaseHardware();
    NTSTATUS
    D0Entry(
        _In_ WDF_POWER_DEVICE_STATE PreviousState);
    NTSTATUS
    D0EntryPostInterruptsEnabled(
        _In_ WDF_POWER_DEVICE_STATE PreviousState);
    VOID D0ExitPreInterruptsDisabled();
    VOID
    D0Exit(
        _In_ WDF_POWER_DEVICE_STATE TargetState);
    VOID SelfManagedIoInit();
    VOID SelfManagedIoCleanup();
    VOID
    UsageNotification(
        _In_ WDF_SPECIAL_FILE_TYPE Type,
        _In_ BOOLEAN InPath);
    VOID
    SetPortWake(
        _In_ BOOLEAN Arm);
    VOID
    TrackSystemPowerIrp(
        _In_ PIO_STACK_LOCATION Stack);
    VOID WatchdogTick();
    VOID RecoveryWorker();
    VOID IdleTimeoutWorker();
    VOID UcxReset();
    NTSTATUS
    QueryUsbCapability(
        _In_ const GUID* Capability,
        _In_ ULONG OutputBufferLength,
        _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
        _Out_ PULONG ResultLength);
    VOID FdoCleanup();

    BOOLEAN m_WaitWakeQueued;
    BOOLEAN m_TestMode;

private:
    enum class IdleStatus : ULONG
    {
        NotConfigured,
        NoS0Wake,
        Configured
    };

    NTSTATUS CreateWdfObjects();
    VOID PopulateErrata();
    VOID
    QueryErrataDatabase(
        _In_ BOOLEAN AtAddDevice);
    VOID ReadRegistryErrata();
    VOID SetInternalFlags();
    VOID ReadVerifierFlags();
    VOID ConfigureS0Idle();
    VOID
    SetIdleTimeout(
        _In_ ULONG Milliseconds);
    VOID
    EvaluateDsm(
        _In_ ULONG Function);
    VOID EvaluatePrmi();
    VOID UpdateRegistryCounters();
    VOID RunFirmwareCommands();

    NTSTATUS StartController();
    NTSTATUS
    RunD0Pass(
        _In_ BOOLEAN Restore);
    VOID InitFrameSnapshot();

    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    QueueRecovery(
        _In_ ULONG Actions,
        _In_ ULONG Reason);
    VOID
    BootRecovery(
        _In_ ULONG Actions);
    VOID
    RunRecovery(
        _In_ ULONG Actions);
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    SetGone(
        _In_ BOOLEAN ReportToPnp);
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID DisableController();
    VOID InternalReset();

    ULONG64
    NowMs() const;

    ULONG m_VerifierFlags;
    BOOLEAN m_Gone;
    BOOLEAN m_SaveFailed;
    BOOLEAN m_FrameSnapshotReady;
    LONG64 m_FrameSnapshot;
    ULONG m_TimeIncrement;
    ULONG m_SpecialFiles;
    ULONG m_RestoreFailures;
    ULONG m_RecoveryCount;
    LONG m_ResetCount;
    LONG m_ResetInterval;
    LONG m_ResetLock;
    IdleStatus m_IdleStatus;
    ULONG m_LastIdleTimeout;

    WDFTIMER m_Watchdog;
    WDFWORKITEM m_RecoveryItem;
    WDFWORKITEM m_IdleItem;

    /* Recovery state */
    KSPIN_LOCK m_RecoveryLock;
    ULONG m_RecoveryActions;
    KMUTEX m_RecoveryMutex;
    BOOLEAN m_GoneHandled;
    BOOLEAN m_PnpToldRemoved;
};

/** Driver wide state set up in DriverEntry. */
struct XhciDriverData
{
    PDRIVER_OBJECT DriverObject;
    PVOID QueryDeviceFlags;
    BOOLEAN TestMode;
};

extern XhciDriverData XhciDriver;
