/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP and power handling for ACPI enumerated PDOs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <wdmguid.h>
#include <debug.h>

/* _STA bit 2, the device should be shown in the UI */
#define UACPINT_STA_SHOW_IN_UI  0x00000004u

/*
 * Leaf devices without a function driver start raw. A missing HID here
 * fails with Code 28, an extra one never gets its driver loaded.
 */
static
BOOLEAN
NTAPI
UacpiNtPdoIsRawDevice(
    _In_ PUACPINT_PDO Pdo)
{
    static PCSTR RawHids[] =
    {
        "ACPI0004",                    /* module container */
        "PNP0A05", "PNP0A06",          /* generic containers */
        "PNP0B00",                     /* RTC */
        "PNP0C09",                     /* embedded controller */
        "PNP0C0B",                     /* fan */
        "PNP0C0C", "PNP0C0D", "PNP0C0E",
        "PNP0C32",                     /* application launch button */
        "PNP0D80", "TOS6200",
    };
    ULONG Index;

    if (Pdo->ButtonCaps || Pdo->IsThermalZone)
        return TRUE;

    for (Index = 0; Index < RTL_NUMBER_OF(RawHids); Index++)
    {
        if (!_stricmp(Pdo->Hid, RawHids[Index]))
            return TRUE;
    }

    return FALSE;
}

static
NTSTATUS
NTAPI
UacpiNtPdoQueryCapabilities(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;
    uacpi_u32 Sta;
    uacpi_u64 Sun;

    if (!Capabilities || Capabilities->Version < 1)
        return STATUS_UNSUCCESSFUL;

    Capabilities->SilentInstall = TRUE;
    Capabilities->RawDeviceOK = UacpiNtPdoIsRawDevice(Pdo);
    Capabilities->SurpriseRemovalOK = FALSE;
    Capabilities->Removable = FALSE;
    Capabilities->UniqueID = FALSE;

    UacpiNtPowerBuildStateMap(Pdo, Capabilities);

    if (Pdo->HasAdr)
        Capabilities->Address = (ULONG)Pdo->Adr;

    if (!Pdo->Node)
        return STATUS_SUCCESS;

    if (uacpi_likely_success(uacpi_eval_simple_integer(Pdo->Node, "_SUN", &Sun)))
        Capabilities->UINumber = (ULONG)Sun;

    Sta = UACPINT_STA_PRESENT | UACPINT_STA_FUNCTIONING | UACPINT_STA_SHOW_IN_UI;
    uacpi_eval_sta(Pdo->Node, &Sta);
    if (!(Sta & UACPINT_STA_SHOW_IN_UI))
        Capabilities->NoDisplayInUI = TRUE;

    /* The instance ID is the namespace path, so _UID makes it unique */
    Capabilities->UniqueID = UacpiNtNodeHasChild(Pdo->Node, "_UID");
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtPdoTargetRelation(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PDEVICE_RELATIONS Relations;

    Relations = ExAllocatePoolWithTag(PagedPool, sizeof(*Relations), UACPINT_POOL_TAG);
    if (!Relations)
        return STATUS_INSUFFICIENT_RESOURCES;

    ObReferenceObject(Pdo->Shared.Self);
    Relations->Count = 1;
    Relations->Objects[0] = Pdo->Shared.Self;

    Irp->IoStatus.Information = (ULONG_PTR)Relations;
    return STATUS_SUCCESS;
}

/* ACPI children first, then filters over the bus driver's _ADR matches */
static
NTSTATUS
NTAPI
UacpiNtPdoBusRelations(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    NTSTATUS Status;

    UacpiNtBuildChildPdosForNode(Pdo->Parent, Pdo->Node);

    Status = UacpiNtMergeChildRelations(Pdo->Parent, Pdo->Node, NULL, Irp);
    if (NT_SUCCESS(Status))
    {
        UacpiNtDetectFilterDevices(Pdo->Parent,
                                   Pdo->Node,
                                   (PDEVICE_RELATIONS)Irp->IoStatus.Information);
    }

    return Status;
}

/* A device with no _CRS simply needs no resources */
static
NTSTATUS
NTAPI
UacpiNtPdoQueryResources(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _In_ BOOLEAN Requirements)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementList = NULL;
    PCM_RESOURCE_LIST ResourceList = NULL;
    NTSTATUS Status;

    /* PCI link _PRS and _SRS belong to irqarb.c, only _CRS is used here */
    if (Requirements)
    {
        Status = UacpiNtPrsToRequirements(Pdo->Node, FALSE, Pdo->Shared.Self, &RequirementList);
        if (NT_SUCCESS(Status))
            Irp->IoStatus.Information = (ULONG_PTR)RequirementList;
    }
    else
    {
        Status = UacpiNtCrsToCmList(Pdo->Node, Pdo->Shared.Self, &ResourceList);
        if (NT_SUCCESS(Status))
            Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
    }

    if (NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    if (Status == STATUS_NOT_FOUND)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    return Status;
}

static
ULONG_PTR
NTAPI
UacpiNtPdoDeviceState(
    _In_ PUACPINT_PDO Pdo)
{
    uacpi_u32 Sta = UACPINT_STA_PRESENT | UACPINT_STA_FUNCTIONING;
    ULONG_PTR State = 0;

    /* Node-less PDOs are always there */
    if (Pdo->Node)
    {
        uacpi_eval_sta(Pdo->Node, &Sta);

        if (!(Sta & UACPINT_STA_PRESENT))
            State |= PNP_DEVICE_DISABLED;
        else if (!(Sta & UACPINT_STA_FUNCTIONING))
            State |= PNP_DEVICE_FAILED;

        if (!(Sta & UACPINT_STA_SHOW_IN_UI))
            State |= PNP_DEVICE_DONT_DISPLAY_IN_UI;
    }

    /* Paging, hibernate and dump devices cannot be disabled */
    if (Pdo->UsageCount > 0)
        State |= PNP_DEVICE_NOT_DISABLEABLE;

    return State;
}

/* _STR is a Unicode buffer, it may lack its terminator */
static
NTSTATUS
NTAPI
UacpiNtPdoQueryDescription(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    uacpi_object *Result = NULL;
    uacpi_data_view View;
    PWSTR Text;
    SIZE_T Size;

    if (uacpi_unlikely_error(uacpi_eval(Pdo->Node, "_STR", NULL, &Result)) || !Result)
        return Status;

    if (uacpi_object_get_type(Result) == UACPI_OBJECT_BUFFER &&
        uacpi_likely_success(uacpi_object_get_buffer(Result, &View)) &&
        View.length >= sizeof(WCHAR))
    {
        Size = View.length + sizeof(WCHAR);
        Text = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
        if (Text)
        {
            RtlZeroMemory(Text, Size);
            RtlCopyMemory(Text, View.data, View.length);
            Irp->IoStatus.Information = (ULONG_PTR)Text;
            Status = STATUS_SUCCESS;
        }
    }

    uacpi_object_unref(Result);
    return Status;
}

static
PPNP_BUS_INFORMATION
NTAPI
UacpiNtPdoBusInformation(VOID)
{
    PPNP_BUS_INFORMATION BusInformation;

    BusInformation = ExAllocatePoolWithTag(PagedPool, sizeof(*BusInformation), UACPINT_POOL_TAG);
    if (!BusInformation)
        return NULL;

    BusInformation->BusTypeGuid = GUID_BUS_TYPE_ACPI;
    BusInformation->LegacyBusType = PNPBus;
    BusInformation->BusNumber = 0;
    return BusInformation;
}

/* Runs a one argument control method such as _EJ0 or _LCK */
static
VOID
NTAPI
UacpiNtPdoEvalWithInteger(
    _In_ PUACPINT_PDO Pdo,
    _In_z_ PCSTR Method,
    _In_ uacpi_u64 Value)
{
    uacpi_object_array Arguments;
    uacpi_object *Argument;

    if (!Pdo->Node)
        return;

    Argument = uacpi_object_create_integer(Value);
    if (!Argument)
        return;

    Arguments.objects = &Argument;
    Arguments.count = 1;
    uacpi_eval(Pdo->Node, Method, &Arguments, NULL);
    uacpi_object_unref(Argument);
}

/* The PDO is the bottom of the stack, every IRP is completed here */
NTSTATUS
NTAPI
UacpiNtPdoPnp(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = Irp->IoStatus.Status;
    PPNP_BUS_INFORMATION BusInformation;

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Pdo->Started = TRUE;
            UacpiNtDriversStartDevice(Pdo);

            /* Connection() descriptors reparse through the resource hub */
            UacpiNtRegisterBiosNameForPdo(Pdo);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            Status = (Pdo->UsageCount > 0) ? STATUS_DEVICE_BUSY : STATUS_SUCCESS;
            break;

        case IRP_MN_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            UacpiNtWakeTeardown(&Pdo->Wake);
            UacpiNtDriversRemoveDevice(Pdo);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            UacpiNtWakeTeardown(&Pdo->Wake);
            UacpiNtDriversRemoveDevice(Pdo);
            UacpiNtResArbiterTeardown(Pdo);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            Status = UacpiNtPdoQueryId(Pdo, Irp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = UacpiNtPdoQueryCapabilities(Pdo, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
                Status = UacpiNtPdoTargetRelation(Pdo, Irp);
            else if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
                Status = UacpiNtPdoBusRelations(Pdo, Irp);
            break;

        case IRP_MN_QUERY_RESOURCES:
            Status = UacpiNtPdoQueryResources(Pdo, Irp, FALSE);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            Status = UacpiNtPdoQueryResources(Pdo, Irp, TRUE);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Irp->IoStatus.Information = UacpiNtPdoDeviceState(Pdo);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            if (IoStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription &&
                !Irp->IoStatus.Information &&
                Pdo->Node)
            {
                Status = UacpiNtPdoQueryDescription(Pdo, Irp, Status);
            }
            break;

        case IRP_MN_QUERY_INTERFACE:
            Status = UacpiNtPdoQueryInterface(Pdo, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            if (IoStack->Parameters.UsageNotification.InPath)
                InterlockedIncrement(&Pdo->UsageCount);
            else if (Pdo->UsageCount > 0)
                InterlockedDecrement(&Pdo->UsageCount);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            BusInformation = UacpiNtPdoBusInformation();
            if (BusInformation)
            {
                Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;

        case IRP_MN_EJECT:
            UacpiNtPdoEvalWithInteger(Pdo, "_EJ0", 1);
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SET_LOCK:
            UacpiNtPdoEvalWithInteger(Pdo, "_LCK", IoStack->Parameters.SetLock.Lock ? 1 : 0);
            Status = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    return UacpiNtCompleteIrp(Irp, Status, Irp->IoStatus.Information);
}

NTSTATUS
NTAPI
UacpiNtPdoPower(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    return UacpiNtPdoSetPower(Pdo, Irp);
}
