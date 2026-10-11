/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Boot, paging and hibernation devices behind a hub
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef
BOOLEAN
(NTAPI *PFN_HUB_BOOT_DEVICE_READY)(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PVOID Context);

/* What ExRegisterBootDevice takes; the layout is the kernel's */
struct HubBootRegistration
{
    ULONG Version;
    ULONG Flags;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    PFN_HUB_BOOT_DEVICE_READY IsBootDeviceReady;
    PVOID Context;
};

#ifdef _WIN64
C_ASSERT(sizeof(HubBootRegistration) == 0x28);
#else
C_ASSERT(sizeof(HubBootRegistration) == 0x18);
#endif

#define HUB_BOOT_REGISTRATION_VERSION 1

/* FUNCTIONS ******************************************************************/

/* A device outside the machine's own container is external; no answer counts as external */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubBootIsExternal(
    _In_ WDFDEVICE Device)
{
    BOOLEAN InMachine = FALSE;
    DEVPROPTYPE Type;
    ULONG Size;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoGetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Device),
                                     &DEVPKEY_Device_InLocalMachineContainer,
                                     LOCALE_NEUTRAL,
                                     0,
                                     sizeof(InMachine),
                                     &InMachine,
                                     &Size,
                                     &Type);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("No container answer for %p (0x%lx), treated as external\n", Device, Status);
        return TRUE;
    }

    return InMachine == DEVPROP_FALSE;
}

/* Runs once a recovered boot device was counted as surprise removed */
static
VOID
NTAPI
HubBootRecoveryWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(WorkItem);

    DPRINT("Boot device recovered after a surprise removal\n");
    HubWmiNotifyBootSurpriseRemoval();
    HubBumpBootSurpriseRemovalCount();
}

/* The work item is freed with its parent PDO */
_IRQL_requires_max_(DISPATCH_LEVEL)
static
VOID
NTAPI
HubBootAfterRecovery(
    _In_ HubChild* Child)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFWORKITEM WorkItem;
    NTSTATUS Status;

    if (!Child->m_Port->HasFlag(PortFlag::PendingRecoveryUpdate))
        return;

    Child->m_Port->ClearFlag(PortFlag::PendingRecoveryUpdate);

    WDF_WORKITEM_CONFIG_INIT(&Config, HubBootRecoveryWorkItem);
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Child->m_Pdo->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Boot recovery work item failed 0x%lx\n", Status);
        return;
    }

    WdfWorkItemEnqueue(WorkItem);
}

/* Must run at PASSIVE_LEVEL since it sends a synchronous transfer */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubBootDeviceReady(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PVOID Context)
{
    HubChild* Child = (HubChild*)Context;
    HubPort* Port = Child->m_Port;
    ULONG PortStatus;
    USHORT Status;
    USHORT Change;
    NTSTATUS ReadStatus;
    BOOLEAN Ready = FALSE;

    UNREFERENCED_PARAMETER(Pdo);

    PAGED_CODE();

    /* The hub reset that is running finishes the recovery */
    if (Child->m_Hub->IsHubResetInProgress())
    {
        DPRINT("Boot device on port %u counted present during a hub reset\n", Port->Number());
        Ready = TRUE;
        goto Done;
    }

    InterlockedExchange(&Child->m_BootReportedMissing, 0);

    /* A failed status read answers not ready; a hub reset follows */
    ReadStatus = HubReadPortStatusForBootDevice(Child, &PortStatus);
    if (!NT_SUCCESS(ReadStatus))
    {
        DPRINT1("Boot device port %u status read failed 0x%lx\n", Port->Number(), ReadStatus);
        goto Done;
    }

    Status = (USHORT)(PortStatus & 0xFFFF);
    Change = (USHORT)(PortStatus >> 16);

    if (Child->HasState(ChildState::DifferentDeviceOnBootPort) && !(Change & PC_CONNECT))
    {
        DPRINT("Boot port %u still holds a different device\n", Port->Number());
        goto Done;
    }

    if (Port->m_ConnectionStatus == DeviceFailedEnumeration)
    {
        DPRINT("Boot device on port %u failed its last enumeration\n", Port->Number());
        goto Done;
    }

    if (Status & PS_CONNECTED)
    {
        DPRINT("Boot device on port %u is connected\n", Port->Number());
        Ready = TRUE;
        goto Done;
    }

    /* A real disconnect passes through SS.Inactive for at most about 20 ms */
    if ((Child->m_Kind & DSM_KIND_SUPER_SPEED) && HubLinkState(Status) == LINK_SS_INACTIVE)
    {
        if (Port->HasFlag(PortFlag::SsInactiveSeenForBoot))
        {
            DPRINT("Boot port %u stayed in SS.Inactive, device counted present\n", Port->Number());
            Ready = TRUE;
            goto Done;
        }

        DPRINT("Boot port %u in SS.Inactive for the first time\n", Port->Number());
        Port->SetFlag(PortFlag::SsInactiveSeenForBoot);
        goto Done;
    }

    Port->ClearFlag(PortFlag::SsInactiveSeenForBoot);
    DPRINT("Boot device on port %u still missing, status 0x%lx\n", Port->Number(), PortStatus);

Done:
    /* Another thread reported it missing again meanwhile */
    if (!Ready && InterlockedExchange(&Child->m_BootReportedMissing, 1) == 1)
    {
        DPRINT("Boot device on port %u reported missing again, answering ready\n", Port->Number());
        Ready = TRUE;
    }

    if (Ready)
    {
        Port->ClearFlag(PortFlag::SsInactiveSeenForBoot);
        HubBootAfterRecovery(Child);
    }

    return Ready;
}

/* The kernel routines are looked up at the first registration */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubBootRegister(
    _In_ HubChild* Child)
{
    HubBootRegistration Registration;
    UNICODE_STRING Name;

    PAGED_CODE();

    if (HubDriver.RegisterBootDevice == NULL)
    {
        RtlInitUnicodeString(&Name, L"ExRegisterBootDevice");
        HubDriver.RegisterBootDevice = (PFN_HUB_REGISTER_BOOT_DEVICE)MmGetSystemRoutineAddress(&Name);

        RtlInitUnicodeString(&Name, L"ExNotifyBootDeviceRemoval");
        HubDriver.NotifyBootDeviceRemoval = (PFN_HUB_NOTIFY_BOOT_DEVICE_REMOVAL)MmGetSystemRoutineAddress(&Name);
    }

    if (HubDriver.RegisterBootDevice == NULL)
        return STATUS_NOT_SUPPORTED;

    RtlZeroMemory(&Registration, sizeof(Registration));
    Registration.Version = HUB_BOOT_REGISTRATION_VERSION;
    Registration.DriverObject = HubDriver.DriverObject;
    Registration.IsBootDeviceReady = HubBootDeviceReady;
    Registration.Context = Child;

    return HubDriver.RegisterBootDevice(&Registration, &Child->m_BootHandle);
}

/* Special files are never disabled once enabled, so a removal notice is handled like an add; under WinPE each notice prepares again */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubBootAddToPath(
    _In_ HubPdo* Pdo,
    _In_ BOOLEAN WinPe,
    _In_ BOOLEAN External)
{
    HubChild* Child = Pdo->m_Child;
    HubFdo* Hub = Pdo->m_Hub;
    NTSTATUS Status;

    PAGED_CODE();

    if (Pdo->HasFlag(PdoFlag::InBootPath))
        return STATUS_SUCCESS;

    if (Child->m_BootControl.Request == NULL)
    {
        Status = Child->m_BootControl.Create(Child->m_Object, WdfDeviceGetIoTarget(Hub->m_Device));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Boot request on port %u not created 0x%lx\n", Pdo->m_PortNumber, Status);
            return Status;
        }
    }

    if (!Child->m_Port->HasFlag(PortFlag::DeviceConnected))
    {
        DPRINT1("Boot device on port %u left before its usage notice\n", Pdo->m_PortNumber);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (!WinPe)
    {
        /* Internal devices are not registered; saves memory where SATA was replaced by USB */
        if (External)
        {
            Status = HubBootRegister(Child);
            if (!NT_SUCCESS(Status))
                DPRINT1("Boot device registration failed 0x%lx\n", Status);

            if (!Child->HasProperty(ChildProperty::IsHub))
                HubWmiRegisterBootSurpriseRemoval(Pdo->m_Device);
        }
        else
        {
            DPRINT1("Boot device on port %u is internal, not registered\n", Pdo->m_PortNumber);
        }

        Pdo->SetFlag(PdoFlag::InBootPath);
        Child->m_Port->SetFlag(PortFlag::SupportsReattach);
    }

    /* The port machine follows the hub's forward progress setting */
    Child->m_NeedsForwardProgress = TRUE;
    DPRINT("Port %u device is now in the boot path\n", Pdo->m_PortNumber);
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPdoEvtUsageNotificationEx(
    _In_ WDFDEVICE Device,
    _In_ WDF_SPECIAL_FILE_TYPE NotificationType,
    _In_ BOOLEAN IsInNotificationPath)
{
    HubPdo* Pdo = HubGetPdoContext(Device);
    HubChild* Child = Pdo->m_Child;
    BOOLEAN WinPe;
    BOOLEAN External = FALSE;

    PAGED_CODE();

    WinPe = HubIsWinPe();

    DPRINT("Port %u usage notice %d, in path %u\n", Pdo->m_PortNumber, (int)NotificationType, IsInNotificationPath);

    if (!(Child->m_SqmFlags & (LONG)ChildSqm::HoldsPagingFiles))
    {
        InterlockedOr(&Child->m_SqmFlags, (LONG)ChildSqm::HoldsPagingFiles);
        HubFlushSqmFlags(Child);
    }

    switch (NotificationType)
    {
        /* Paging on an external disk that is not the boot disk is refused */
        case WdfSpecialFilePaging:
            if (!WinPe)
            {
                External = HubBootIsExternal(Device);
                if (External)
                {
                    if (Pdo->HasFlag(PdoFlag::InBootPath))
                        return STATUS_SUCCESS;

                    DPRINT1("Paging refused on external port %u device\n", Pdo->m_PortNumber);
                    return STATUS_NOT_SUPPORTED;
                }
            }
            return HubBootAddToPath(Pdo, WinPe, External);

        case WdfSpecialFileBoot:
            if (!WinPe && !Pdo->HasFlag(PdoFlag::InBootPath))
                External = HubBootIsExternal(Device);
            return HubBootAddToPath(Pdo, WinPe, External);

        case WdfSpecialFileHibernation:
            if (IsInNotificationPath)
                Child->m_NeedsForwardProgress = TRUE;
            return STATUS_SUCCESS;

        default:
            return STATUS_SUCCESS;
    }
}

VOID
DeviceMachine::NotifyWrongDevice()
{
    DPRINT1("A different device is on boot port %u\n", m_Device->m_Port->Number());
    m_Device->SetState(ChildState::DifferentDeviceOnBootPort);
    HubNotifyBootDeviceRemoval(m_Device->m_BootHandle);
}
