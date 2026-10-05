/*
 * PROJECT:     ReactOS USB Driver Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USBD handle, capability query and URB allocation for client drivers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbdex.h"

#define USBDEX_HANDLE_SIGNATURE 'DBSU'

/* Handle modes; the UCX value is the client interface version it negotiated */
#define USBDEX_MODE_UCX     USBD_INTERFACE_VERSION_602
#define USBDEX_MODE_LEGACY  USBD_INTERFACE_VERSION_600
#define USBDEX_MODE_NONE    0xFFFFFFFF

/* POOL_TYPE value for no-execute nonpaged pool on NT 6.2 and later */
#define USBDEX_NONPAGED_POOL_NX ((POOL_TYPE)0x200)

#define USBDEX_PIPE_MAX_TRANSFER_DEFAULT 0xFFFFFFFF
#define USBDEX_SELECT_INTERFACE_MAX_PIPES 32
#define USBDEX_COMPOSITE_VERSION 0x0100

typedef
NTSTATUS
(NTAPI *PFN_USBDEX_QUERY_REGISTRY)(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _Inout_ PRTL_QUERY_REGISTRY_TABLE QueryTable,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Environment);

/* What a USBD_HANDLE points to; private to each client binary */
typedef struct _USBDEX_HANDLE_OBJECT
{
    ULONG Signature;
    USBD_CLIENT_INTERFACE Client;
    PDEVICE_OBJECT TargetDeviceObject;
    ULONG Mode;
    volatile LONG ReferenceCount;
    BOOLEAN HighSpeed;
    BOOLEAN CloseRequested;
} USBDEX_HANDLE_OBJECT, *PUSBDEX_HANDLE_OBJECT;

static POOL_TYPE UsbdexPoolType = NonPagedPool;
static BOOLEAN UsbdexPoolTypeKnown = FALSE;

static IO_COMPLETION_ROUTINE UsbdexSyncCompletion;
static RTL_QUERY_REGISTRY_ROUTINE UsbdexVerifierValueCallback;

FORCEINLINE
PUSBDEX_HANDLE_OBJECT
UsbdexHandleObject(
    _In_opt_ USBD_HANDLE USBDHandle)
{
    return (PUSBDEX_HANDLE_OBJECT)USBDHandle;
}

FORCEINLINE
BOOLEAN
UsbdexIsUcxMode(
    _In_ PUSBDEX_HANDLE_OBJECT Handle)
{
    return Handle->Mode != USBDEX_MODE_NONE && Handle->Mode >= USBDEX_MODE_UCX;
}

static
VOID
UsbdexDetectPoolType(VOID)
{
    RTL_OSVERSIONINFOW OsVersion;

    if (UsbdexPoolTypeKnown)
        return;

    UsbdexPoolType = NonPagedPool;

    RtlZeroMemory(&OsVersion, sizeof(OsVersion));
    OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
    if (NT_SUCCESS(RtlGetVersion(&OsVersion)))
    {
        if (OsVersion.dwMajorVersion > 6 ||
            (OsVersion.dwMajorVersion == 6 && OsVersion.dwMinorVersion >= 2))
        {
            UsbdexPoolType = USBDEX_NONPAGED_POOL_NX;
        }
    }

    UsbdexPoolTypeKnown = TRUE;
}

static
NTSTATUS
NTAPI
UsbdexSyncCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Sends a prepared IRP, waits for it, frees it and returns its final status */
static
NTSTATUS
UsbdexCallAndWait(
    _In_ PDEVICE_OBJECT ClientDevice,
    _In_ PDEVICE_OBJECT Target,
    _In_ __drv_freesMem(Mem) PIRP Irp)
{
    KEVENT Done;
    NTSTATUS Status;

    KeInitializeEvent(&Done, NotificationEvent, FALSE);

    Status = IoSetCompletionRoutineEx(ClientDevice, Irp, UsbdexSyncCompletion, &Done, TRUE, TRUE, TRUE);
    if (!NT_SUCCESS(Status))
        IoSetCompletionRoutine(Irp, UsbdexSyncCompletion, &Done, TRUE, TRUE, TRUE);

    Status = IoCallDriver(Target, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Done, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    IoFreeIrp(Irp);
    return Status;
}

static
NTSTATUS
UsbdexQueryInterface(
    _In_ PDEVICE_OBJECT ClientDevice,
    _In_ PDEVICE_OBJECT Target,
    _In_ const GUID *InterfaceType,
    _Inout_ PINTERFACE Interface)
{
    PIO_STACK_LOCATION Stack;
    PIRP Irp;

    Irp = IoAllocateIrp(Target->StackSize, FALSE);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* PnP IRPs must start out as not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType = InterfaceType;
    Stack->Parameters.QueryInterface.Size = Interface->Size;
    Stack->Parameters.QueryInterface.Version = Interface->Version;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    return UsbdexCallAndWait(ClientDevice, Target, Irp);
}

static
NTSTATUS
UsbdexSendCapabilityRequest(
    _In_ PDEVICE_OBJECT ClientDevice,
    _In_ PDEVICE_OBJECT Target,
    _Inout_ PUCXHUB_QUERY_CAPABILITY Request,
    _Out_opt_ PVOID OutputBuffer)
{
    PIO_STACK_LOCATION Stack;
    PIRP Irp;

    Irp = IoAllocateIrp(Target->StackSize, FALSE);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->AssociatedIrp.SystemBuffer = OutputBuffer;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->MinorFunction = 0;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_UCXHUB_QUERY_USB_CAPABILITY;
    Stack->Parameters.Others.Argument1 = Request;

    return UsbdexCallAndWait(ClientDevice, Target, Irp);
}

static
NTSTATUS
NTAPI
UsbdexVerifierValueCallback(
    _In_z_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID EntryContext)
{
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(Context);

    if (ValueType != REG_DWORD)
        return STATUS_INVALID_PARAMETER;

    if (ValueLength != sizeof(ULONG))
        return STATUS_INTERNAL_ERROR;

    /* A missing value comes back as the default, which already is the destination */
    if (ValueData != EntryContext)
        *(PULONG)EntryContext = *(PULONG)ValueData;

    return STATUS_SUCCESS;
}

/** Turns "\Driver\Name" into "Name\Parameters"; NULL for any other form. */
static
PWSTR
UsbdexBuildParametersPath(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ ULONG PoolTag)
{
    static const WCHAR DriverPrefix[] = L"\\Driver\\";
    static const WCHAR ParametersSuffix[] = L"\\Parameters";
    const SIZE_T PrefixBytes = sizeof(DriverPrefix) - sizeof(WCHAR);
    PUNICODE_STRING DriverName = &DriverObject->DriverName;
    SIZE_T ServiceBytes;
    SIZE_T PathBytes;
    PWSTR Path;

    if (DriverName->Length <= PrefixBytes)
        return NULL;

    if (RtlCompareMemory(DriverName->Buffer, DriverPrefix, PrefixBytes) != PrefixBytes)
        return NULL;

    ServiceBytes = DriverName->Length - PrefixBytes;
    PathBytes = ServiceBytes + sizeof(ParametersSuffix);

    Path = ExAllocatePoolWithTag(UsbdexPoolType, PathBytes, PoolTag);
    if (!Path)
        return NULL;

    RtlZeroMemory(Path, PathBytes);
    RtlCopyMemory(Path, (PUCHAR)DriverName->Buffer + PrefixBytes, ServiceBytes);
    RtlCopyMemory((PUCHAR)Path + ServiceBytes, ParametersSuffix, sizeof(ParametersSuffix));
    return Path;
}

/* Loads the UsbVerifier* overrides from the client's Parameters key into its interface */
static
VOID
UsbdexReadVerifierSettings(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_z_ PCWSTR ParametersPath,
    _In_ ULONG PoolTag,
    _Inout_ PUSBD_CLIENT_INTERFACE Client)
{
    PCWSTR ValueNames[] =
    {
        L"UsbVerifierEnabled",
        L"UsbVerifierFailRegistration",
        L"UsbVerifierFailChainedMdlSupport",
        L"UsbVerifierFailStaticStreamSupport",
        L"UsbVerifierStaticStreamCountOverride",
        L"UsbVerifierFailEnableStaticStreams",
    };
    PULONG Fields[] =
    {
        &Client->VerifierEnabled,
        &Client->VerifierFailRegistration,
        &Client->VerifierFailChainedMdlSupport,
        &Client->VerifierFailStaticStreamSupport,
        &Client->VerifierStaticStreamCountOverride,
        &Client->VerifierFailEnableStaticStreams,
    };
    PRTL_QUERY_REGISTRY_TABLE Table;
    PFN_USBDEX_QUERY_REGISTRY QueryValues;
    UNICODE_STRING RoutineName;
    SIZE_T TableBytes;
    ULONG Index;

    C_ASSERT(RTL_NUMBER_OF(ValueNames) == RTL_NUMBER_OF(Fields));

    Client->VerifierEnabled = MmIsDriverVerifying(DriverObject) ? 1 : 0;

    /* One extra zeroed entry terminates the table */
    TableBytes = (RTL_NUMBER_OF(ValueNames) + 1) * sizeof(*Table);
    Table = ExAllocatePoolWithTag(UsbdexPoolType, TableBytes, PoolTag);
    if (!Table)
        return;

    RtlZeroMemory(Table, TableBytes);
    for (Index = 0; Index < RTL_NUMBER_OF(ValueNames); Index++)
    {
        Table[Index].QueryRoutine = UsbdexVerifierValueCallback;
        Table[Index].Flags = 0;
        Table[Index].Name = (PWSTR)ValueNames[Index];
        Table[Index].EntryContext = Fields[Index];
        Table[Index].DefaultType = REG_DWORD;
        Table[Index].DefaultData = Fields[Index];
        Table[Index].DefaultLength = sizeof(ULONG);
    }

    RtlInitUnicodeString(&RoutineName, L"RtlQueryRegistryValuesEx");
    QueryValues = (PFN_USBDEX_QUERY_REGISTRY)MmGetSystemRoutineAddress(&RoutineName);
    if (!QueryValues)
        QueryValues = RtlQueryRegistryValues;

    (VOID)QueryValues(RTL_REGISTRY_SERVICES, ParametersPath, Table, NULL, NULL);

    ExFreePoolWithTag(Table, PoolTag);
}

/* Asks a USB 2 era bus driver for its USBDI interface; returns the resulting handle mode */
static
ULONG
UsbdexProbeLegacyStack(
    _Inout_ PUSBDEX_HANDLE_OBJECT Handle)
{
    USB_BUS_INTERFACE_USBDI_V1 BusInterface;
    NTSTATUS Status;

    RtlZeroMemory(&BusInterface, sizeof(BusInterface));
    BusInterface.Size = sizeof(BusInterface);
    BusInterface.Version = USB_BUSIF_USBDI_VERSION_1;

    Status = UsbdexQueryInterface(Handle->Client.DeviceObject,
                                  Handle->TargetDeviceObject,
                                  &USB_BUS_INTERFACE_USBDI_GUID,
                                  (PINTERFACE)&BusInterface);
    if (!NT_SUCCESS(Status))
        return USBDEX_MODE_NONE;

    if (BusInterface.IsDeviceHighSpeed)
        Handle->HighSpeed = BusInterface.IsDeviceHighSpeed(BusInterface.BusContext);

    /* The bus driver referenced the interface while answering; this is the only use of it */
    if (BusInterface.InterfaceDereference)
        BusInterface.InterfaceDereference(BusInterface.BusContext);

    return USBDEX_MODE_LEGACY;
}

static
NTSTATUS
UsbdexReferenceHandle(
    _Inout_ PUSBDEX_HANDLE_OBJECT Handle)
{
    if (Handle->CloseRequested || Handle->ReferenceCount < 1)
        return STATUS_INVALID_DEVICE_STATE;

    InterlockedIncrement(&Handle->ReferenceCount);
    return STATUS_SUCCESS;
}

static
VOID
UsbdexDereferenceHandle(
    _Inout_ PUSBDEX_HANDLE_OBJECT Handle)
{
    if (InterlockedDecrement(&Handle->ReferenceCount) > 0)
        return;

    /* Only a closed handle goes away; anything else is a client imbalance */
    if (!Handle->CloseRequested)
        return;

    if (Handle->Client.Unregister)
        Handle->Client.Unregister(Handle->Client.Handle);

    ExFreePoolWithTag(Handle, Handle->Client.PoolTag);
}

/* Common front half of every URB allocator; takes the reference the URB will own */
static
NTSTATUS
UsbdexStartAllocation(
    _In_opt_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_ BOOLEAN ArgumentsValid,
    _Out_opt_ PURB *Urb)
{
    if (!Handle || !ArgumentsValid)
    {
        if (Urb)
            *Urb = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    if (!Urb)
        return STATUS_INVALID_PARAMETER;

    *Urb = NULL;

    if (!NT_SUCCESS(UsbdexReferenceHandle(Handle)))
        return STATUS_INVALID_DEVICE_STATE;

    return STATUS_SUCCESS;
}

/* Common back half; a failed allocation gives its reference back */
static
NTSTATUS
UsbdexFinishAllocation(
    _Inout_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_ NTSTATUS Status,
    _Inout_ PURB *Urb)
{
    if (!NT_SUCCESS(Status))
    {
        *Urb = NULL;
        UsbdexDereferenceHandle(Handle);
    }

    return Status;
}

static
PURB
UsbdexAllocatePoolUrb(
    _In_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_ SIZE_T Size)
{
    PURB Urb;

    Urb = ExAllocatePoolWithTag(UsbdexPoolType, Size, Handle->Client.PoolTag);
    if (Urb)
        RtlZeroMemory(Urb, Size);

    return Urb;
}

/* Fills one interface block the way the bus driver expects to receive it */
static
VOID
UsbdexInitInterfaceInformation(
    _Out_ PUSBD_INTERFACE_INFORMATION Information,
    _In_ PUSB_INTERFACE_DESCRIPTOR Descriptor,
    _In_ UCHAR PipeCount)
{
    PUSBD_PIPE_INFORMATION Pipe;
    ULONG Index;

    Information->Length = (USHORT)GET_USBD_INTERFACE_SIZE(PipeCount);
    Information->InterfaceNumber = Descriptor->bInterfaceNumber;
    Information->AlternateSetting = Descriptor->bAlternateSetting;
    Information->NumberOfPipes = PipeCount;

    Pipe = Information->Pipes;
    for (Index = 0; Index < PipeCount; Index++)
    {
        Pipe[Index].MaximumTransferSize = USBDEX_PIPE_MAX_TRANSFER_DEFAULT;
        Pipe[Index].PipeFlags = 0;
    }
}

static
NTSTATUS
UsbdexBuildSelectConfigUrb(
    _In_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_opt_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    _Out_ PURB *Urb)
{
    PUSBD_INTERFACE_LIST_ENTRY Entry;
    PUCHAR Cursor;
    PUCHAR End;
    PURB NewUrb;
    ULONG TotalSize;
    ULONG BlockSize;
    UCHAR PipeCount;

    /* The URB Length field is 16 bits, so the whole request has to fit */
    TotalSize = FIELD_OFFSET(struct _URB_SELECT_CONFIGURATION, Interface);
    for (Entry = InterfaceList; Entry->InterfaceDescriptor; Entry++)
    {
        TotalSize += (ULONG)GET_USBD_INTERFACE_SIZE(Entry->InterfaceDescriptor->bNumEndpoints);
        if (TotalSize > MAXUSHORT)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewUrb = UsbdexAllocatePoolUrb(Handle, TotalSize);
    if (!NewUrb)
        return STATUS_INSUFFICIENT_RESOURCES;

    Cursor = (PUCHAR)&NewUrb->UrbSelectConfiguration.Interface;
    End = (PUCHAR)NewUrb + TotalSize;

    /* The descriptors may change under us, so every block is checked against the buffer again */
    for (Entry = InterfaceList; Entry->InterfaceDescriptor; Entry++)
    {
        PipeCount = Entry->InterfaceDescriptor->bNumEndpoints;
        BlockSize = (ULONG)GET_USBD_INTERFACE_SIZE(PipeCount);
        if (BlockSize > (ULONG)(End - Cursor))
        {
            ExFreePoolWithTag(NewUrb, Handle->Client.PoolTag);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        UsbdexInitInterfaceInformation((PUSBD_INTERFACE_INFORMATION)Cursor,
                                       Entry->InterfaceDescriptor,
                                       PipeCount);
        Entry->Interface = (PUSBD_INTERFACE_INFORMATION)Cursor;
        Cursor += BlockSize;
    }

    NewUrb->UrbSelectConfiguration.Hdr.Length = (USHORT)TotalSize;
    NewUrb->UrbSelectConfiguration.Hdr.Function = URB_FUNCTION_SELECT_CONFIGURATION;
    NewUrb->UrbSelectConfiguration.ConfigurationDescriptor = ConfigurationDescriptor;

    *Urb = NewUrb;
    return STATUS_SUCCESS;
}

static
NTSTATUS
UsbdexBuildSelectInterfaceUrb(
    _In_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceListEntry,
    _Out_ PURB *Urb)
{
    PUSB_INTERFACE_DESCRIPTOR Descriptor;
    PURB NewUrb;
    ULONG TotalSize;
    UCHAR PipeCount;

    Descriptor = InterfaceListEntry->InterfaceDescriptor;
    if (!Descriptor)
        return STATUS_INVALID_PARAMETER;

    PipeCount = Descriptor->bNumEndpoints;
    if (PipeCount > USBDEX_SELECT_INTERFACE_MAX_PIPES)
        return STATUS_INVALID_PARAMETER;

    TotalSize = FIELD_OFFSET(struct _URB_SELECT_INTERFACE, Interface) +
                (ULONG)GET_USBD_INTERFACE_SIZE(PipeCount);

    NewUrb = UsbdexAllocatePoolUrb(Handle, TotalSize);
    if (!NewUrb)
        return STATUS_INSUFFICIENT_RESOURCES;

    UsbdexInitInterfaceInformation(&NewUrb->UrbSelectInterface.Interface, Descriptor, PipeCount);
    InterfaceListEntry->Interface = &NewUrb->UrbSelectInterface.Interface;

    NewUrb->UrbSelectInterface.Hdr.Length = (USHORT)TotalSize;
    NewUrb->UrbSelectInterface.Hdr.Function = URB_FUNCTION_SELECT_INTERFACE;
    NewUrb->UrbSelectInterface.ConfigurationHandle = ConfigurationHandle;

    *Urb = NewUrb;
    return STATUS_SUCCESS;
}

/* Capability answers for a stack that predates the capability IOCTL */
static
NTSTATUS
UsbdexLegacyCapability(
    _In_ PUSBDEX_HANDLE_OBJECT Handle,
    _In_ const GUID *CapabilityType,
    _In_opt_ PUCHAR OutputBuffer)
{
    if (RtlCompareMemory(CapabilityType,
                         &GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE,
                         sizeof(GUID)) == sizeof(GUID))
    {
        if (OutputBuffer)
            return STATUS_INVALID_PARAMETER;

        return Handle->HighSpeed ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
    }

    if (RtlCompareMemory(CapabilityType,
                         &GUID_USB_CAPABILITY_SELECTIVE_SUSPEND,
                         sizeof(GUID)) == sizeof(GUID))
    {
        return OutputBuffer ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
USBD_CreateHandle(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ ULONG USBDClientContractVersion,
    _In_ ULONG PoolTag,
    _Out_ USBD_HANDLE *USBDHandle)
{
    PUSBDEX_HANDLE_OBJECT Handle;
    PWSTR ParametersPath;
    NTSTATUS Status;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        if (USBDHandle)
            *USBDHandle = NULL;
        return STATUS_INVALID_LEVEL;
    }

    UsbdexDetectPoolType();

    if (!DeviceObject ||
        !TargetDeviceObject ||
        USBDClientContractVersion < USBD_CLIENT_CONTRACT_VERSION_602 ||
        PoolTag == 0)
    {
        if (USBDHandle)
            *USBDHandle = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    if (!USBDHandle)
        return STATUS_INVALID_PARAMETER;

    ParametersPath = UsbdexBuildParametersPath(DeviceObject->DriverObject, PoolTag);

    Handle = ExAllocatePoolWithTag(UsbdexPoolType, sizeof(*Handle), PoolTag);
    if (!Handle)
    {
        if (ParametersPath)
            ExFreePoolWithTag(ParametersPath, PoolTag);
        *USBDHandle = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Handle, sizeof(*Handle));
    if (ParametersPath)
        UsbdexReadVerifierSettings(DeviceObject->DriverObject, ParametersPath, PoolTag, &Handle->Client);

    Handle->Signature = USBDEX_HANDLE_SIGNATURE;
    Handle->Client.Header.Size = sizeof(Handle->Client);
    Handle->Client.Header.Version = USBD_CLIENT_INTERFACE_VERSION;
    Handle->Client.ClientContractVersion = USBDClientContractVersion;
    Handle->Client.DeviceObject = DeviceObject;
    Handle->Client.PoolTag = PoolTag;
    Handle->Client.ClientContext = Handle;
    Handle->TargetDeviceObject = TargetDeviceObject;
    Handle->Mode = USBDEX_MODE_UCX;
    Handle->ReferenceCount = 1;

    Status = UsbdexQueryInterface(DeviceObject,
                                  TargetDeviceObject,
                                  &GUID_USBD_CLIENT_INTERFACE,
                                  (PINTERFACE)&Handle->Client);
    if (!NT_SUCCESS(Status))
    {
        /* Nothing the lower stack may have left in the answer part is usable */
        Handle->Client.Handle = NULL;
        Handle->Client.Unregister = NULL;
        Handle->Client.AllocUrb = NULL;
        Handle->Client.AllocIsochUrb = NULL;
        Handle->Client.AllocSelectConfigUrb = NULL;
        Handle->Client.AllocSelectInterfaceUrb = NULL;
        Handle->Client.ReleaseUrb = NULL;

        /* Not a UCX stack; the handle still works, with pool URBs */
        Handle->Mode = UsbdexProbeLegacyStack(Handle);
    }

    if (ParametersPath)
        ExFreePoolWithTag(ParametersPath, PoolTag);

    *USBDHandle = (USBD_HANDLE)Handle;
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_CloseHandle(
    _In_ USBD_HANDLE USBDHandle)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);

    if (!Handle)
        return;

    /* Outstanding URBs keep the handle alive until their USBD_UrbFree */
    Handle->CloseRequested = TRUE;
    UsbdexDereferenceHandle(Handle);
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
USBD_QueryUsbCapability(
    _In_ USBD_HANDLE USBDHandle,
    _In_ const GUID *CapabilityType,
    _In_ ULONG OutputBufferLength,
    _When_(OutputBufferLength == 0, _Pre_null_)
    _When_(OutputBufferLength != 0 && ResultLength == NULL, _Out_writes_bytes_(OutputBufferLength))
    _When_(OutputBufferLength != 0 && ResultLength != NULL, _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultLength))
        PUCHAR OutputBuffer,
    _Out_opt_
    _When_(ResultLength != NULL, _Deref_out_range_(<=, OutputBufferLength))
        PULONG ResultLength)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);
    UCXHUB_QUERY_CAPABILITY Request;
    NTSTATUS Status;

    if (ResultLength)
        *ResultLength = 0;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        return STATUS_INVALID_PARAMETER;

    if (!Handle || !CapabilityType)
        return STATUS_INVALID_PARAMETER;

    /* A buffer and its length come together or not at all */
    if ((OutputBufferLength != 0) != (OutputBuffer != NULL))
        return STATUS_INVALID_PARAMETER;

    if (Handle->Mode == USBDEX_MODE_NONE)
        return STATUS_NOT_IMPLEMENTED;

    if (Handle->Mode == USBDEX_MODE_LEGACY)
        return UsbdexLegacyCapability(Handle, CapabilityType, OutputBuffer);

    RtlZeroMemory(&Request, sizeof(Request));
    Request.Version = UCXHUB_QUERY_CAPABILITY_VERSION;
    Request.Size = sizeof(Request);
    Request.UsbdHandle = Handle->Client.Handle;
    Request.CapabilityType = *CapabilityType;
    Request.OutputBufferLength = OutputBufferLength;

    Status = UsbdexSendCapabilityRequest(Handle->Client.DeviceObject,
                                         Handle->TargetDeviceObject,
                                         &Request,
                                         OutputBuffer);
    if (NT_SUCCESS(Status) && ResultLength)
        *ResultLength = Request.ResultLength;

    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
USBD_UrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _Outptr_result_bytebuffer_(sizeof(URB)) PURB *Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);
    NTSTATUS Status;

    Status = UsbdexStartAllocation(Handle, TRUE, Urb);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Handle->Client.AllocUrb)
    {
        Status = Handle->Client.AllocUrb(Handle->Client.Handle, Urb);
    }
    else
    {
        *Urb = UsbdexAllocatePoolUrb(Handle, sizeof(URB));
        Status = *Urb ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    }

    return UsbdexFinishAllocation(Handle, Status, Urb);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
USBD_IsochUrbAllocate(
    _In_ USBD_HANDLE USBDHandle,
    _In_ ULONG NumberOfIsochPackets,
    _Outptr_result_bytebuffer_(sizeof(struct _URB_ISOCH_TRANSFER)
                               + (NumberOfIsochPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR))
                               - sizeof(USBD_ISO_PACKET_DESCRIPTOR))
        PURB *Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);
    const ULONG MaxPackets = (MAXULONG - sizeof(URB)) / sizeof(USBD_ISO_PACKET_DESCRIPTOR);
    NTSTATUS Status;

    Status = UsbdexStartAllocation(Handle, TRUE, Urb);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Handle->Client.AllocIsochUrb)
    {
        Status = Handle->Client.AllocIsochUrb(Handle->Client.Handle, NumberOfIsochPackets, Urb);
    }
    else if (NumberOfIsochPackets > MaxPackets)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Same size as the UCX isoch URB region: a whole URB plus one descriptor per packet */
        *Urb = UsbdexAllocatePoolUrb(Handle,
                                     sizeof(URB) +
                                     NumberOfIsochPackets * sizeof(USBD_ISO_PACKET_DESCRIPTOR));
        Status = *Urb ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    }

    return UsbdexFinishAllocation(Handle, Status, Urb);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
USBD_SelectConfigUrbAllocateAndBuild(
    _In_ USBD_HANDLE USBDHandle,
    _In_opt_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    _Outptr_ PURB *Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);
    NTSTATUS Status;

    Status = UsbdexStartAllocation(Handle, InterfaceList != NULL, Urb);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Handle->Client.AllocSelectConfigUrb)
    {
        Status = Handle->Client.AllocSelectConfigUrb(Handle->Client.Handle,
                                                                ConfigurationDescriptor,
                                                                InterfaceList,
                                                                Urb);
    }
    else
    {
        Status = UsbdexBuildSelectConfigUrb(Handle, ConfigurationDescriptor, InterfaceList, Urb);
    }

    return UsbdexFinishAllocation(Handle, Status, Urb);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
USBD_SelectInterfaceUrbAllocateAndBuild(
    _In_ USBD_HANDLE USBDHandle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceListEntry,
    _Outptr_ PURB *Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);
    NTSTATUS Status;

    Status = UsbdexStartAllocation(Handle,
                                   ConfigurationHandle != NULL && InterfaceListEntry != NULL,
                                   Urb);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Handle->Client.AllocSelectInterfaceUrb)
    {
        Status = Handle->Client.AllocSelectInterfaceUrb(Handle->Client.Handle,
                                                                   ConfigurationHandle,
                                                                   InterfaceListEntry,
                                                                   Urb);
    }
    else
    {
        Status = UsbdexBuildSelectInterfaceUrb(Handle, ConfigurationHandle, InterfaceListEntry, Urb);
    }

    return UsbdexFinishAllocation(Handle, Status, Urb);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_UrbFree(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PURB Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);

    if (!Urb || !Handle)
        return;

    if (Handle->Client.ReleaseUrb)
        Handle->Client.ReleaseUrb(Urb);
    else
        ExFreePoolWithTag(Urb, Handle->Client.PoolTag);

    UsbdexDereferenceHandle(Handle);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_AssignUrbToIoStackLocation(
    _In_ USBD_HANDLE USBDHandle,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _In_ PURB Urb)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);

    IoStackLocation->Parameters.Others.Argument1 = Urb;

    /* UCX tells an XRB from a plain URB by finding the URB in FileObject too */
    if (UsbdexIsUcxMode(Handle))
        IoStackLocation->FileObject = (PFILE_OBJECT)Urb;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
NTAPI
USBD_IsInterfaceVersionSupported(
    _In_ USBD_HANDLE USBDHandle,
    _In_ ULONG USBDInterfaceVersion)
{
    return UsbdexHandleObject(USBDHandle)->Mode >= USBDInterfaceVersion;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_BuildRegisterCompositeDevice(
    _In_ USBD_HANDLE USBDHandle,
    _In_ COMPOSITE_DEVICE_CAPABILITIES CapabilityFlags,
    _In_ ULONG FunctionCount,
    _Out_ PREGISTER_COMPOSITE_DEVICE RegisterCompositeDevice)
{
    PUSBDEX_HANDLE_OBJECT Handle = UsbdexHandleObject(USBDHandle);

    RtlZeroMemory(RegisterCompositeDevice, sizeof(*RegisterCompositeDevice));
    RegisterCompositeDevice->Version = USBDEX_COMPOSITE_VERSION;
    RegisterCompositeDevice->Size = sizeof(*RegisterCompositeDevice);
    RegisterCompositeDevice->Reserved = (USBDI_HANDLE)Handle->Client.Handle;
    RegisterCompositeDevice->CapabilityFlags = CapabilityFlags;
    RegisterCompositeDevice->FunctionCount = FunctionCount;
}
