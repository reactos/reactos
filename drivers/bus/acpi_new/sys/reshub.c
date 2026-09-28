/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Resource hub client for ACPI connection descriptors
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <haltypes.h>
#include <reshub_downlevel.h>
#include <drivers/acpi/restrans.h>
#include <debug.h>

/* Initial translation buffer, the hub asks for more when it needs it */
#define UACPINT_TRANSLATION_BUFFER_SIZE 120

/* The hub interface, guarded by a fast mutex. All callers run <= APC_LEVEL. */
static UACPINT_RH_TRANSLATION_INTERFACE HubInterface;
static BOOLEAN HubInterfaceValid;
static FAST_MUTEX HubInterfaceLock;

/* The hub passes a NULL name */
static
NTSTATUS
NTAPI
UacpiNtRhAllocateSecondaryGsiv(
    _In_ PCHAR DescriptorName,
    _In_ ULONG DescriptorNameSize,
    _Out_ PULONG Gsiv)
{
#if (NTDDI_VERSION >= NTDDI_WIN8)
    NTSTATUS Status;

    /* A GSIV made up here could never be connected */
    if (!HALPRIVATEDISPATCH->HalAllocateGsivForSecondaryInterrupt)
    {
        DPRINT1("uACPI-NT: the HAL has no secondary GSIV allocator\n");
        return STATUS_NOT_SUPPORTED;
    }

    Status = HALPRIVATEDISPATCH->HalAllocateGsivForSecondaryInterrupt((PCCHAR)DescriptorName,
                                                                      (USHORT)DescriptorNameSize,
                                                                      Gsiv);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: secondary GSIV allocation failed 0x%lx\n", Status);
        return Status;
    }

    DPRINT("uACPI-NT: secondary GSIV %lu allocated\n", *Gsiv);
    return STATUS_SUCCESS;
#else
    UNREFERENCED_PARAMETER(DescriptorName);
    UNREFERENCED_PARAMETER(DescriptorNameSize);
    UNREFERENCED_PARAMETER(Gsiv);

    return STATUS_NOT_SUPPORTED;
#endif
}

static
uacpi_namespace_node *
NTAPI
UacpiNtNodeFromDeviceObject(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PUACPINT_SHARED Shared = DeviceObject->DeviceExtension;

    /* Only our own device objects point back at themselves */
    if (!Shared || Shared->Self != DeviceObject)
        return NULL;

    switch (Shared->Type)
    {
        case UacpiNtDevObjPdo:
            return ((PUACPINT_PDO)Shared)->Node;

        case UacpiNtDevObjFilter:
            return ((PUACPINT_FLT)Shared)->Node;

        default:
            return NULL;
    }
}

/* GpioInt trigger modes from the hub go into the irqlib GSIV tables */
static
NTSTATUS
NTAPI
UacpiNtRhSetInterruptProperties(
    _In_ ULONG Gsiv,
    _In_ KINTERRUPT_MODE Mode,
    _In_ KINTERRUPT_POLARITY Polarity)
{
    if (Mode == Latched)
        UacpiNtIrqLibNoteEdgeGsiv(Gsiv);
    else
        UacpiNtIrqLibNoteLevelGsiv(Gsiv);

    DPRINT("uACPI-NT: GSIV %lu is %s, polarity %lu\n",
           Gsiv,
           (Mode == Latched) ? "edge" : "level",
           (ULONG)Polarity);

    return STATUS_SUCCESS;
}

/* ResourceSource is resolved in the scope of the requesting device */
static
NTSTATUS
NTAPI
UacpiNtRhQualifyBiosName(
    _In_ PDEVICE_OBJECT ScopeDevice,
    _In_ PSTRING ReferenceName,
    _Out_ PUNICODE_STRING QualifiedName,
    _Out_ PULONG StringLength)
{
    uacpi_namespace_node *Scope = NULL;
    uacpi_namespace_node *Target = NULL;
    const uacpi_char *AbsolutePath;
    ANSI_STRING PathString;
    USHORT Required;
    NTSTATUS Status;

    *StringLength = 0;

    if (!ReferenceName || !ReferenceName->Buffer)
        return STATUS_INVALID_PARAMETER;

    if (ScopeDevice)
        Scope = UacpiNtNodeFromDeviceObject(ScopeDevice);

    if (!Scope)
        Scope = uacpi_namespace_root();

    if (uacpi_unlikely_error(uacpi_namespace_node_find(Scope, ReferenceName->Buffer, &Target)) || !Target)
    {
        DPRINT1("uACPI-NT: cannot resolve ResourceSource %s\n", ReferenceName->Buffer);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    AbsolutePath = uacpi_namespace_node_generate_absolute_path(Target);
    if (!AbsolutePath)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitAnsiString(&PathString, AbsolutePath);

    /* The hub retries once with a buffer of the reported size */
    Required = (USHORT)RtlAnsiStringToUnicodeSize(&PathString);
    *StringLength = Required;

    if (QualifiedName->MaximumLength < Required)
    {
        uacpi_free_absolute_path(AbsolutePath);
        return STATUS_BUFFER_TOO_SMALL;
    }

    Status = RtlAnsiStringToUnicodeString(QualifiedName, &PathString, FALSE);
    uacpi_free_absolute_path(AbsolutePath);

    return Status;
}

static
VOID
NTAPI
UacpiNtRhRelease(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    ExAcquireFastMutex(&HubInterfaceLock);
    HubInterfaceValid = FALSE;
    RtlZeroMemory(&HubInterface, sizeof(HubInterface));
    ExReleaseFastMutex(&HubInterfaceLock);
}

static
NTSTATUS
NTAPI
UacpiNtOpenResourceHub(
    _Out_ PHANDLE HubHandle)
{
    DECLARE_CONST_UNICODE_STRING(HubDeviceName, RESOURCE_HUB_DEVICE_NAME);
    DECLARE_CONST_UNICODE_STRING(HubServiceName,
                                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\acpiex");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)&HubDeviceName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenFile(HubHandle,
                        GENERIC_READ | GENERIC_WRITE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND)
        return Status;

    /* The hub is demand start, nothing has needed it before now */
    Status = ZwLoadDriver((PUNICODE_STRING)&HubServiceName);
    if (!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED)
        DPRINT1("uACPI-NT: loading acpiex failed 0x%lx\n", Status);

    return ZwOpenFile(HubHandle,
                      GENERIC_READ | GENERIC_WRITE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      FILE_NON_DIRECTORY_FILE);
}

/* Hand our half of the interface to the hub and take its half back */
static
NTSTATUS
NTAPI
UacpiNtExchangeHubInterface(
    _In_ PDEVICE_OBJECT HubDevice,
    _Out_ PUACPINT_RH_TRANSLATION_INTERFACE Interface)
{
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    RtlZeroMemory(Interface, sizeof(*Interface));
    Interface->Interface.Size = sizeof(*Interface);
    Interface->Interface.Version = UACPINT_RH_TRANSLATION_VERSION;
    Interface->HostContext = NULL;
    Interface->ReleaseInterface = UacpiNtRhRelease;
    Interface->QualifyBiosName = UacpiNtRhQualifyBiosName;
    Interface->AllocateSecondaryGsiv = UacpiNtRhAllocateSecondaryGsiv;
    Interface->SetInterruptProperties = UacpiNtRhSetInterruptProperties;

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_UACPINT_RH_QUERY_TRANSLATION,
                                        HubDevice,
                                        Interface,
                                        sizeof(*Interface),
                                        Interface,
                                        sizeof(*Interface),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = IoCallDriver(HubDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: translation interface query failed 0x%lx\n", Status);
        return Status;
    }

    /* Check what came back before calling through any of it */
    if (Interface->Interface.Version != UACPINT_RH_TRANSLATION_VERSION ||
        !Interface->TranslateDescriptor ||
        !Interface->AssociateBiosName)
    {
        DPRINT1("uACPI-NT: the resource hub returned an unusable interface\n");
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UacpiNtConnectResourceHub(VOID)
{
    UACPINT_RH_TRANSLATION_INTERFACE Interface;
    PDEVICE_OBJECT HubDevice;
    PFILE_OBJECT HubFile;
    HANDLE HubHandle;
    NTSTATUS Status;

    PAGED_CODE();

    ExInitializeFastMutex(&HubInterfaceLock);

    Status = UacpiNtOpenResourceHub(&HubHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: no resource hub (0x%lx), connection resources are not translated\n",
                Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(HubHandle,
                                       GENERIC_READ | GENERIC_WRITE,
                                       *IoFileObjectType,
                                       KernelMode,
                                       (PVOID *)&HubFile,
                                       NULL);
    ZwClose(HubHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    HubDevice = IoGetAttachedDeviceReference(HubFile->DeviceObject);

    Status = UacpiNtExchangeHubInterface(HubDevice, &Interface);
    if (NT_SUCCESS(Status))
    {
        ExAcquireFastMutex(&HubInterfaceLock);
        HubInterface = Interface;
        HubInterfaceValid = TRUE;
        ExReleaseFastMutex(&HubInterfaceLock);

        DPRINT("uACPI-NT: resource hub translation interface bound\n");
    }

    ObDereferenceObject(HubDevice);
    ObDereferenceObject(HubFile);
    return Status;
}

NTSTATUS
NTAPI
UacpiNtAddBiosNameDeviceAssociation(
    _In_ PCUNICODE_STRING ReferenceName,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    ExAcquireFastMutex(&HubInterfaceLock);

    if (HubInterfaceValid)
        Status = HubInterface.AssociateBiosName(HubInterface.Context, ReferenceName, DeviceObject);

    ExReleaseFastMutex(&HubInterfaceLock);
    return Status;
}

/* One retry when the hub reports a bigger size */
NTSTATUS
NTAPI
UacpiNtTranslateConnectionDescriptor(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_(DescriptorLength) PVOID Descriptor,
    _In_ ULONG DescriptorLength,
    _Out_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    PUACPINT_RH_TRANSLATION_RESULT Result;
    ULONG Size = UACPINT_TRANSLATION_BUFFER_SIZE;
    ULONG Attempt;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    ExAcquireFastMutex(&HubInterfaceLock);

    if (!HubInterfaceValid)
    {
        ExReleaseFastMutex(&HubInterfaceLock);
        return STATUS_NOT_SUPPORTED;
    }

    for (Attempt = 0; Attempt < 2; Attempt++)
    {
        Result = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
        if (!Result)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = HubInterface.TranslateDescriptor(HubInterface.Context,
                                                         DeviceObject,
                                                         Descriptor,
                                                         DescriptorLength,
                                                         0,
                                                         Result,
                                                         &Size);
        if (NT_SUCCESS(Status))
            *IoDescriptor = Result->Descriptor;

        ExFreePoolWithTag(Result, UACPINT_POOL_TAG);

        if (Status != STATUS_BUFFER_TOO_SMALL || Size == 0)
            break;
    }

    ExReleaseFastMutex(&HubInterfaceLock);
    return Status;
}

/* Every started PDO is offered to the hub as a possible connection provider */
VOID
NTAPI
UacpiNtRegisterBiosNameForPdo(
    _In_ PUACPINT_PDO Pdo)
{
    const uacpi_char *AbsolutePath;
    UNICODE_STRING PathUnicode;
    ANSI_STRING PathAnsi;
    BOOLEAN Bound;
    NTSTATUS Status;

    PAGED_CODE();

    if (!Pdo || !Pdo->Node || !Pdo->Shared.Self)
        return;

    ExAcquireFastMutex(&HubInterfaceLock);
    Bound = HubInterfaceValid;
    ExReleaseFastMutex(&HubInterfaceLock);

    if (!Bound)
        return;

    AbsolutePath = uacpi_namespace_node_generate_absolute_path(Pdo->Node);
    if (!AbsolutePath)
        return;

    RtlInitAnsiString(&PathAnsi, AbsolutePath);
    Status = RtlAnsiStringToUnicodeString(&PathUnicode, &PathAnsi, TRUE);
    uacpi_free_absolute_path(AbsolutePath);

    if (!NT_SUCCESS(Status))
        return;

    Status = UacpiNtAddBiosNameDeviceAssociation(&PathUnicode, Pdo->Shared.Self);
    if (NT_SUCCESS(Status))
        DPRINT("uACPI-NT: %wZ registered with the hub as %p\n", &PathUnicode, Pdo->Shared.Self);

    RtlFreeUnicodeString(&PathUnicode);
}
