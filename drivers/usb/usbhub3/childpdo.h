/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Child PDO of a hub
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Child PDO state flags */
enum class PdoFlag : ULONG
{
    ReportPortDisabled       = 0x00000001,
    ProgrammingLostOnReset   = 0x00000002,
    InBootPath               = 0x00000004,

    /* devxfer */
    ArmedForWake             = 0x00000008,

    /* hubpdo */
    InD0                     = 0x00000010,
    WakeArmRequested         = 0x00000020,
    WaitWakePending          = 0x00000040,
    ClientDoesFunctionSuspend = 0x00000080,
    WasResetAtResume        = 0x00000100,
    PowerLostOnReset         = 0x00000200,
    DeviceGone               = 0x00000400,
    QueryStopped             = 0x00000800,
    QueryRemoved             = 0x00001000,
    IdleMachineStarted       = 0x00002000,
    WakeNotificationSent     = 0x00004000,
    FailGetStatus            = 0x00008000,
    Started                  = 0x00010000
};

/* hubpdo */

/* Number of LPM power setting registrations per child PDO */
#define HUB_PDO_LPM_SETTINGS            5

/* Client contract version before a USBD client identified itself */
#define HUB_PDO_CONTRACT_UNKNOWN        0xFFFFFFFF

class HubPdo
{
public:
    static
    HubPdo*
    FromDevice(
        _In_ WDFDEVICE Device);

public:
    IdleMachine m_Idle;

    WDFDEVICE m_Device;
    HubFdo* m_Hub;
    HubChild* m_Child;
    USHORT m_PortNumber;

    /* PdoFlag bits, interlocked */
    volatile LONG m_Flags;

    /* hubpdo */

    BOOLEAN
    HasFlag(
        _In_ PdoFlag Flag) const
    {
        return (m_Flags & (LONG)Flag) != 0;
    }

    VOID
    SetFlag(
        _In_ PdoFlag Flag)
    {
        InterlockedOr(&m_Flags, (LONG)Flag);
    }

    VOID
    ClearFlag(
        _In_ PdoFlag Flag)
    {
        InterlockedAnd(&m_Flags, ~(LONG)Flag);
    }

    /** Clears Flag and says whether it was set. */
    BOOLEAN
    TestAndClearFlag(
        _In_ PdoFlag Flag)
    {
        return (InterlockedAnd(&m_Flags, ~(LONG)Flag) & (LONG)Flag) != 0;
    }

    /* Gate for client IOCTLs; plain stores */
    BOOLEAN m_FailRequests;

    /* FALSE until the PDO is a static child; cleanup then skips the device machine */
    BOOLEAN m_Reported;

    /* Set once by the first accepted cycle port request */
    volatile LONG m_CycleQueued;

    /* WDM device object right below the root hub FDO; NULL for a placeholder */
    PDEVICE_OBJECT m_ControllerTarget;

    WDFQUEUE m_Queue;
    WDFWORKITEM m_IdleWorkItem;

    /* Written by select configuration validation, in mA */
    ULONG m_ConfigPowerDraw;

    /* Copy of the client's USB_START_FAILDATA */
    PVOID m_StartFailData;

    /* ChildHubObject of a child hub that took the parent interface; debug only */
    PVOID m_ChildHubFdo;

    /* Remote wake notification sent to UCX for SuperSpeed devices */
    WDFREQUEST m_WakeRequest;
    REQUEST_REMOTE_WAKE_NOTIFICATION m_WakeBlock;
    KEVENT m_WakeDone;

    PVOID m_LpmSettings[HUB_PDO_LPM_SETTINGS];

    /* What an ACPI filter put into the D3cold interface; zero without one */
    D3COLD_SUPPORT_INTERFACE m_AcpiD3Cold;

    WDF_POWER_DEVICE_STATE m_WdfPowerState;
    ULONG m_ClientContractVersion;

    /* Device interface symbolic link, fetched at prepare hardware */
    WDFSTRING m_SymbolicLink;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubPdo, HubGetPdoContext);

/* hubpdo */

/* childpdo.cpp */

/** Waits for the device machine to answer a PnP or power event; returns the PnP status. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPdoPostAndWait(
    _In_ HubChild* Child,
    _In_ DsmEvent Event,
    _In_ PCSTR What);

/* suspend.cpp */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPdoCreateIdleWorkItem(
    _In_ HubPdo* Pdo);

/** IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION; the idle machine owns the IRP afterwards. */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
HubPdoSubmitIdleRequest(
    _In_ HubPdo* Pdo,
    _In_ PIRP Irp);

/* bootdev.cpp */

EVT_WDF_DEVICE_USAGE_NOTIFICATION_EX HubPdoEvtUsageNotificationEx;
