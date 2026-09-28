/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry point
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

/* SystemProcessorBrandString */
#define UACPINT_SYSTEM_PROCESSOR_BRAND_STRING 105

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ ULONG SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength);

PUACPINT_FDO GlobalAcpiFdo;
PDRIVER_OBJECT GlobalAcpiDriverObj;
UNICODE_STRING GlobalDriverRegPath;
CHAR UacpiNtProcessorString[128];
CHAR UacpiNtProcessorBrand[64];

NTSTATUS
NTAPI
UacpiNtCompleteIrp(
    _In_ PIRP Irp,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR Information)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
UacpiNtForwardAndForget(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp)
{
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

static
NTSTATUS
NTAPI
UacpiNtSignalCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
UacpiNtForwardAndWait(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, UacpiNtSignalCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(LowerDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

static
NTSTATUS
NTAPI
UacpiNtDispatchFdo(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return UacpiNtFdoPnp(Fdo, Irp);

        case IRP_MJ_POWER:
            return UacpiNtFdoPower(Fdo, Irp);

        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        case IRP_MJ_CLEANUP:
            return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, 0);

        default:
            return UacpiNtForwardAndForget(Fdo->LowerDevice, Irp);
    }
}

static
NTSTATUS
NTAPI
UacpiNtDispatchFilter(
    _In_ PUACPINT_FLT Filter,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return UacpiNtFilterPnp(Filter, Irp);

        case IRP_MJ_POWER:
            return UacpiNtFilterPower(Filter, Irp);

        /* Evaluation IOCTLs, anything unknown goes down the stack */
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return UacpiNtFilterDeviceControl(Filter, Irp);

        default:
            return UacpiNtForwardAndForget(Filter->LowerDevice, Irp);
    }
}

static
NTSTATUS
NTAPI
UacpiNtDispatchPdo(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN Handled = FALSE;
    NTSTATUS Status;

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return UacpiNtPdoPnp(Pdo, Irp);

        case IRP_MJ_POWER:
            return UacpiNtPdoPower(Pdo, Irp);

        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return UacpiNtPdoDeviceControl(Pdo, Irp);

        /* Only the EC has an address space to transfer */
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
            if (Pdo->Ec)
                return UacpiNtEcReadWrite(Pdo, Irp);
            return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);

        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        case IRP_MJ_CLEANUP:
            return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, 0);

        /* Thermal zones answer WMI, everyone else completes it untouched */
        case IRP_MJ_SYSTEM_CONTROL:
            Status = UacpiNtThermalSystemControl(Pdo, Irp, &Handled);
            if (Handled)
                return Status;
            return UacpiNtCompleteIrp(Irp, Irp->IoStatus.Status, 0);

        default:
            return UacpiNtCompleteIrp(Irp, STATUS_NOT_SUPPORTED, 0);
    }
}

static
NTSTATUS
NTAPI
UacpiNtDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PUACPINT_SHARED Shared = DeviceObject->DeviceExtension;

    switch (Shared->Type)
    {
        case UacpiNtDevObjFdo:
            return UacpiNtDispatchFdo((PUACPINT_FDO)Shared, Irp);

        case UacpiNtDevObjFilter:
            return UacpiNtDispatchFilter((PUACPINT_FLT)Shared, Irp);

        case UacpiNtDevObjPdo:
            return UacpiNtDispatchPdo((PUACPINT_PDO)Shared, Irp);

        default:
            return UacpiNtCompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
    }
}

static
NTSTATUS
NTAPI
UacpiNtAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    PUACPINT_FDO Fdo;
    NTSTATUS Status;

    Status = IoCreateDevice(DriverObject,
                            sizeof(UACPINT_FDO),
                            NULL,
                            FILE_DEVICE_ACPI,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: IoCreateDevice for the FDO failed 0x%lx\n", Status);
        return Status;
    }

    Fdo = DeviceObject->DeviceExtension;
    RtlZeroMemory(Fdo, sizeof(*Fdo));
    Fdo->Shared.Type = UacpiNtDevObjFdo;
    Fdo->Shared.Self = DeviceObject;
    Fdo->PhysicalDeviceObject = PhysicalDeviceObject;
    ExInitializeFastMutex(&Fdo->ChildLock);
    InitializeListHead(&Fdo->ChildList);
    InitializeListHead(&Fdo->FilterList);

    Fdo->LowerDevice = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if (!Fdo->LowerDevice)
    {
        DPRINT1("uACPI-NT: could not attach to the root PDO\n");
        IoDeleteDevice(DeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DPRINT("uACPI-NT: FDO %p over PDO %p\n", DeviceObject, PhysicalDeviceObject);
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UacpiNtUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

/*
 * REG_DWORD overrides from Services\ACPI\Parameters.
 * A missing value keeps the compiled default.
 */
static
VOID
NTAPI
UacpiNtReadParameters(
    _In_ PUNICODE_STRING RegistryPath)
{
    static const struct
    {
        PCWSTR Name;
        PULONG Value;
    } Parameters[] =
    {
        { L"IrqArbEnabled",        &UacpiNtIrqArbEnabled },
        { L"IrqArbVerbose",        &UacpiNtIrqArbVerbose },
        { L"IrqLibHalOverrides",   &UacpiNtIrqLibHalOverrides },
        { L"ResArbEnabled",        &UacpiNtResArbEnabled },
        { L"ResVerbose",           &UacpiNtResVerbose },
        { L"MsiDiagEnabled",       &UacpiNtMsiDiagEnabled },
        { L"MsiDiagDelaySeconds",  &UacpiNtMsiDiagDelaySeconds },
        { L"EnumDiagEnabled",      &UacpiNtEnumDiagEnabled },
        { L"EnumDiagDelaySeconds", &UacpiNtEnumDiagDelaySeconds },
        { L"HostVerbose",          &UacpiNtHostVerbose },
    };
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ServiceKey;
    HANDLE ParametersKey;
    ULONG i;

    if (!RegistryPath || !RegistryPath->Length)
        return;

    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(ZwOpenKey(&ServiceKey, KEY_READ, &ObjectAttributes)))
        return;

    if (NT_SUCCESS(UacpiNtRegOpenKey(ServiceKey, L"Parameters", KEY_READ, &ParametersKey)))
    {
        for (i = 0; i < RTL_NUMBER_OF(Parameters); i++)
            UacpiNtRegQueryUlong(ParametersKey, Parameters[i].Name, Parameters[i].Value);

        ZwClose(ParametersKey);
    }

    ZwClose(ServiceKey);
}

/*
 * Legacy Processor() objects are named from CentralProcessor\0, e.g.
 * ACPI\GenuineIntel_-_Intel64_Family_6_Model_158
 * Windows 10 and later append the CPUID brand string on top of that.
 */
static
VOID
NTAPI
UacpiNtSetProcessorInformation(VOID)
{
    CHAR Identifier[96];
    CHAR Vendor[64];
    HANDLE Key;
    PCHAR Stepping;
#if (NTDDI_VERSION >= NTDDI_WIN10)
    ULONG Length;
    ULONG i;
#endif

    PAGED_CODE();

    UacpiNtProcessorString[0] = ANSI_NULL;
    UacpiNtProcessorBrand[0] = ANSI_NULL;

    if (!NT_SUCCESS(UacpiNtRegOpenKey(NULL,
                                      L"\\Registry\\Machine\\Hardware\\Description\\System\\CentralProcessor\\0",
                                      KEY_READ,
                                      &Key)))
    {
        DPRINT1("uACPI-NT: no CentralProcessor\\0, processor devices get no IDs\n");
        return;
    }

    if (UacpiNtRegQueryAnsiString(Key, L"Identifier", Identifier, sizeof(Identifier)) &&
        UacpiNtRegQueryAnsiString(Key, L"VendorIdentifier", Vendor, sizeof(Vendor)))
    {
        /* The stepping is not part of the ID */
        Stepping = strstr(Identifier, " Stepping");
        if (Stepping)
            *Stepping = ANSI_NULL;

        RtlStringCbPrintfA(UacpiNtProcessorString,
                           sizeof(UacpiNtProcessorString),
                           "%s - %s",
                           Vendor,
                           Identifier);
    }

    ZwClose(Key);

#if (NTDDI_VERSION >= NTDDI_WIN10)
    if (NT_SUCCESS(ZwQuerySystemInformation(UACPINT_SYSTEM_PROCESSOR_BRAND_STRING,
                                            UacpiNtProcessorBrand,
                                            sizeof(UacpiNtProcessorBrand) - 1,
                                            &Length)))
    {
        UacpiNtProcessorBrand[sizeof(UacpiNtProcessorBrand) - 1] = ANSI_NULL;

        /* PnP IDs cannot carry commas or control characters */
        for (i = 0; UacpiNtProcessorBrand[i] != ANSI_NULL; i++)
        {
            if (UacpiNtProcessorBrand[i] < ' ' ||
                UacpiNtProcessorBrand[i] > '~' ||
                UacpiNtProcessorBrand[i] == ',')
            {
                UacpiNtProcessorBrand[i] = ' ';
            }
        }
    }
    else
    {
        UacpiNtProcessorBrand[0] = ANSI_NULL;
    }
#endif

    DPRINT("uACPI-NT: processor ID string \"%s\"\n", UacpiNtProcessorString);
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    ULONG i;

    DPRINT1("uACPI-NT: uACPI based ACPI driver for %s\n", NT_TARGET_NAME);

    GlobalAcpiDriverObj = DriverObject;

    /* WmiLib wants the service path long after DriverEntry returns */
    if (RegistryPath && RegistryPath->Length)
    {
        GlobalDriverRegPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
        GlobalDriverRegPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                           GlobalDriverRegPath.MaximumLength,
                                                           UACPINT_POOL_TAG);
        if (GlobalDriverRegPath.Buffer)
        {
            RtlCopyUnicodeString(&GlobalDriverRegPath, RegistryPath);
            GlobalDriverRegPath.Buffer[GlobalDriverRegPath.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            GlobalDriverRegPath.MaximumLength = 0;
        }
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = UacpiNtDispatch;

    DriverObject->DriverExtension->AddDevice = UacpiNtAddDevice;
    DriverObject->DriverUnload = UacpiNtUnload;

    UacpiNtReadParameters(RegistryPath);
    UacpiNtSetProcessorInformation();

    return STATUS_SUCCESS;
}
