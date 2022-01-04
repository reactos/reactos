/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/pnp.c
 * PURPOSE:        Plug N Play routines
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "processr.h"

#include <stdio.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
GetDeviceId(
    PDEVICE_OBJECT DeviceObject,
    BUS_QUERY_ID_TYPE IdType,
    PWSTR *DeviceId)
{
    PIO_STACK_LOCATION IrpStack;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT TargetObject;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    PAGED_CODE();

    /* Initialize the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    TargetObject = IoGetAttachedDeviceReference(DeviceObject);

    /* Build the IRP */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       TargetObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (Irp == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* PNP IRPs all begin life as STATUS_NOT_SUPPORTED */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    /* Get the top of stack */
    IrpStack = IoGetNextIrpStackLocation(Irp);

    /* Set the top of stack */
    RtlZeroMemory(IrpStack, sizeof(IO_STACK_LOCATION));
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_ID;
    IrpStack->Parameters.QueryId.IdType = IdType;

    /* Call the driver */
    Status = IoCallDriver(TargetObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        Status = IoStatus.Status;
    }

    if (NT_SUCCESS(Status))
    {
        *DeviceId = (PWSTR)IoStatus.Information;
    }

done:
    /* Dereference the target device object */
    ObDereferenceObject(TargetObject);

    return Status;
}



static
VOID
ProcessorSetFriendlyName(
    PDEVICE_OBJECT DeviceObject)
{
    KEY_VALUE_PARTIAL_INFORMATION *Buffer = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING HardwareKeyName, ValueName, EnumKeyName;
    HANDLE KeyHandle = NULL;
    ULONG DataLength = 0;
    ULONG BufferLength = 0;
    NTSTATUS Status;
    PWSTR KeyNameBuffer = NULL;
    PWSTR DeviceId = NULL;
    PWSTR InstanceId = NULL;
    PWSTR pszPrefix = L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum";

    RtlInitUnicodeString(&HardwareKeyName,
                         L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    InitializeObjectAttributes(&ObjectAttributes,
                               &HardwareKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       KEY_READ,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenKey() failed (Status 0x%08lx)\n", Status);
        return;
    }

    RtlInitUnicodeString(&ValueName,
                         L"ProcessorNameString");
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &DataLength);
    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_SUCCESS)
    {
        DPRINT1("ZwQueryValueKey() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Buffer = ExAllocatePool(PagedPool,
                            DataLength + sizeof(KEY_VALUE_PARTIAL_INFORMATION));
    if (Buffer == NULL)
    {
        DPRINT1("ExAllocatePool() failed\n");
        goto done;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             Buffer,
                             DataLength + sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                             &DataLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQueryValueKey() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DPRINT("ProcessorNameString: %S\n", (PWSTR)&Buffer->Data[0]);

    ZwClose(KeyHandle);
    KeyHandle = NULL;

    Status = GetDeviceId(DeviceObject,
                         BusQueryDeviceID,
                         &DeviceId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetDeviceId() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DPRINT("DeviceId: %S\n", DeviceId);

    Status = GetDeviceId(DeviceObject,
                         BusQueryInstanceID,
                         &InstanceId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetDeviceId() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DPRINT("InstanceId: %S\n", InstanceId);

    BufferLength = wcslen(pszPrefix) + 1 + wcslen(DeviceId) + 1 + wcslen(InstanceId) + 1;

    KeyNameBuffer = ExAllocatePool(PagedPool, BufferLength * sizeof(WCHAR));
    if (KeyNameBuffer == NULL)
    {
        DPRINT1("ExAllocatePool() failed\n");
        goto done;
    }

    swprintf(KeyNameBuffer, L"%s\\%s\\%s", pszPrefix, DeviceId, InstanceId);

    RtlInitUnicodeString(&EnumKeyName, KeyNameBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &EnumKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       KEY_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenKey() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    RtlInitUnicodeString(&ValueName,
                         L"FriendlyName");
    Status = ZwSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)&Buffer->Data[0],
                           Buffer->DataLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetValueKey() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

done:
    if (KeyHandle != NULL)
        ZwClose(KeyHandle);

    if (KeyNameBuffer != NULL)
        ExFreePool(KeyNameBuffer);

    if (InstanceId != NULL)
        ExFreePool(InstanceId);

    if (DeviceId != NULL)
        ExFreePool(DeviceId);

    if (Buffer != NULL)
        ExFreePool(Buffer);
}


static
NTSTATUS
ProcessorStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PCM_RESOURCE_LIST ResourceListTranslated)
{
    DPRINT("ProcessorStartDevice()\n");

    ProcessorSetFriendlyName(DeviceObject);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
ProcessorPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    ULONG_PTR Information = 0;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    DPRINT("ProcessorPnp()\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DPRINT("  IRP_MN_START_DEVICE received\n");

            /* Call lower driver */
            DeviceExtension = DeviceObject->DeviceExtension;
            Status = STATUS_UNSUCCESSFUL;

            if (IoForwardIrpSynchronously(DeviceExtension->LowerDevice, Irp))
            {
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                {
                    Status = ProcessorStartDevice(DeviceObject,
                        IrpSp->Parameters.StartDevice.AllocatedResources,
                        IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated);
                }
            }
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            DPRINT("  IRP_MN_QUERY_REMOVE_DEVICE\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_REMOVE_DEVICE:
            DPRINT("  IRP_MN_REMOVE_DEVICE received\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DPRINT("  IRP_MN_CANCEL_REMOVE_DEVICE\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_STOP_DEVICE:
            DPRINT("  IRP_MN_STOP_DEVICE received\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_QUERY_STOP_DEVICE:
            DPRINT("  IRP_MN_QUERY_STOP_DEVICE received\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_CANCEL_STOP_DEVICE:
            DPRINT("  IRP_MN_CANCEL_STOP_DEVICE\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT("  IRP_MN_QUERY_DEVICE_RELATIONS\n");

            switch (IrpSp->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);
                    break;

                case RemovalRelations:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                default:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            IrpSp->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DPRINT("  IRP_MN_SURPRISE_REMOVAL received\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* (optional) 0xd */
            DPRINT("  IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            return ForwardIrpAndForget(DeviceObject, Irp);

        default:
            DPRINT("  Unknown IOCTL 0x%lx\n", IrpSp->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
ProcessorAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo)
{
    PDEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT Fdo = NULL;
    NTSTATUS Status;

    DPRINT("ProcessorAddDevice()\n");

    ASSERT(DriverObject);
    ASSERT(Pdo);

    /* Create functional device object */
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (NT_SUCCESS(Status))
    {
        DeviceExtension = (PDEVICE_EXTENSION)Fdo->DeviceExtension;
        RtlZeroMemory(DeviceExtension, sizeof(DEVICE_EXTENSION));

        DeviceExtension->DeviceObject = Fdo;

        Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
            IoDeleteDevice(Fdo);
            return Status;
        }

        Fdo->Flags |= DO_DIRECT_IO;
        Fdo->Flags |= DO_POWER_PAGABLE;

        Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return Status;
}

/* EOF */
