/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/battery/battc/battc.c
 * PURPOSE:         Battery Class Driver
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <battc.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    DPRINT("Battery class driver initialized\n");

    return STATUS_SUCCESS;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassUnload(PVOID ClassData)
{
    PBATTERY_CLASS_DATA BattClass;

    DPRINT("Battery %p is being unloaded\n", ClassData);

    BattClass = ClassData;
    if (BattClass->InterfaceName.Length != 0)
    {
        IoSetDeviceInterfaceState(&BattClass->InterfaceName, FALSE);
        RtlFreeUnicodeString(&BattClass->InterfaceName);
    }

    ExFreePoolWithTag(BattClass,
                      BATTERY_CLASS_DATA_TAG);

    return STATUS_SUCCESS;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassSystemControl(PVOID ClassData,
                          PVOID WmiLibContext,
                          PDEVICE_OBJECT DeviceObject,
                          PIRP Irp,
                          PVOID Disposition)
{
    UNIMPLEMENTED;

    return STATUS_WMI_GUID_NOT_FOUND;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassQueryWmiDataBlock(PVOID ClassData,
                              PDEVICE_OBJECT DeviceObject,
                              PIRP Irp,
                              ULONG GuidIndex,
                              PULONG InstanceLengthArray,
                              ULONG OutBufferSize,
                              PUCHAR Buffer)
{
    UNIMPLEMENTED;

    return STATUS_WMI_GUID_NOT_FOUND;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassStatusNotify(PVOID ClassData)
{
    PBATTERY_CLASS_DATA BattClass;
    PBATTERY_WAIT_STATUS BattWait;
    BATTERY_STATUS BattStatus;
    NTSTATUS Status;

    DPRINT("Received battery status notification from %p\n", ClassData);

    BattClass = ClassData;
    BattWait = BattClass->EventTriggerContext;

    ExAcquireFastMutex(&BattClass->Mutex);
    if (!BattClass->Waiting)
    {
        ExReleaseFastMutex(&BattClass->Mutex);
        return STATUS_SUCCESS;
    }

    switch (BattClass->EventTrigger)
    {
        case EVENT_BATTERY_TAG:
            ExReleaseFastMutex(&BattClass->Mutex);
            DPRINT1("Waiting for battery is UNIMPLEMENTED!\n");
            break;

        case EVENT_BATTERY_STATUS:
            ExReleaseFastMutex(&BattClass->Mutex);
            Status = BattClass->MiniportInfo.QueryStatus(BattClass->MiniportInfo.Context,
                                                         BattWait->BatteryTag,
                                                         &BattStatus);
            if (!NT_SUCCESS(Status))
                return Status;

            ExAcquireFastMutex(&BattClass->Mutex);

            if (BattWait->PowerState != BattStatus.PowerState ||
                BattWait->HighCapacity < BattStatus.Capacity ||
                BattWait->LowCapacity > BattStatus.Capacity)
            {
                KeSetEvent(&BattClass->WaitEvent, IO_NO_INCREMENT, FALSE);
            }

            ExReleaseFastMutex(&BattClass->Mutex);
            break;

        default:
            ExReleaseFastMutex(&BattClass->Mutex);
            ASSERT(FALSE);
            break;
    }

    return STATUS_SUCCESS;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassInitializeDevice(PBATTERY_MINIPORT_INFO MiniportInfo,
                             PVOID *ClassData)
{
    NTSTATUS Status;
    PBATTERY_CLASS_DATA BattClass;

    BattClass = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(BATTERY_CLASS_DATA),
                                      BATTERY_CLASS_DATA_TAG);
    if (BattClass == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(BattClass, sizeof(BATTERY_CLASS_DATA));

    RtlCopyMemory(&BattClass->MiniportInfo,
                  MiniportInfo,
                  sizeof(BattClass->MiniportInfo));

    KeInitializeEvent(&BattClass->WaitEvent, SynchronizationEvent, FALSE);

    ExInitializeFastMutex(&BattClass->Mutex);

    if (MiniportInfo->Pdo != NULL)
    {
        Status = IoRegisterDeviceInterface(MiniportInfo->Pdo,
                                           &GUID_DEVICE_BATTERY,
                                           NULL,
                                           &BattClass->InterfaceName);
        if (NT_SUCCESS(Status))
        {
            DPRINT("Initialized battery interface: %wZ\n", &BattClass->InterfaceName);
            Status = IoSetDeviceInterfaceState(&BattClass->InterfaceName, TRUE);
            if (Status == STATUS_OBJECT_NAME_EXISTS)
            {
                DPRINT1("Got STATUS_OBJECT_NAME_EXISTS for SetDeviceInterfaceState\n");
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            DPRINT1("IoRegisterDeviceInterface failed (0x%x)\n", Status);
        }
    }

    *ClassData = BattClass;

    return STATUS_SUCCESS;
}

BCLASSAPI
NTSTATUS
NTAPI
BatteryClassIoctl(PVOID ClassData,
                  PIRP Irp)
{
    PBATTERY_CLASS_DATA BattClass;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG WaitTime;
    BATTERY_WAIT_STATUS BattWait;
    PBATTERY_QUERY_INFORMATION BattQueryInfo;
    PBATTERY_SET_INFORMATION BattSetInfo;
    LARGE_INTEGER Timeout;
    PBATTERY_STATUS BattStatus;
    BATTERY_NOTIFY BattNotify;
    ULONG ReturnedLength;

    DPRINT("BatteryClassIoctl(%p %p)\n", ClassData, Irp);

    BattClass = ClassData;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0;

    DPRINT("Received IOCTL %x for %p\n", IrpSp->Parameters.DeviceIoControl.IoControlCode,
           ClassData);

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_BATTERY_QUERY_TAG:
            if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG) && IrpSp->Parameters.DeviceIoControl.InputBufferLength != 0) ||
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            WaitTime = IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(ULONG) ? *(PULONG)Irp->AssociatedIrp.SystemBuffer : 0;

            Timeout.QuadPart = Int32x32To64(WaitTime, -1000);

            Status = BattClass->MiniportInfo.QueryTag(BattClass->MiniportInfo.Context,
                                                      (PULONG)Irp->AssociatedIrp.SystemBuffer);
            if (!NT_SUCCESS(Status))
            {
                ExAcquireFastMutex(&BattClass->Mutex);
                BattClass->EventTrigger = EVENT_BATTERY_TAG;
                BattClass->Waiting = TRUE;
                ExReleaseFastMutex(&BattClass->Mutex);

                Status = KeWaitForSingleObject(&BattClass->WaitEvent,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               WaitTime != -1 ? &Timeout : NULL);

                ExAcquireFastMutex(&BattClass->Mutex);
                BattClass->Waiting = FALSE;
                ExReleaseFastMutex(&BattClass->Mutex);

                if (Status == STATUS_SUCCESS)
                {
                    Status = BattClass->MiniportInfo.QueryTag(BattClass->MiniportInfo.Context,
                                                              (PULONG)Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status))
                        Irp->IoStatus.Information = sizeof(ULONG);
                }
                else
                {
                    Status = STATUS_NO_SUCH_DEVICE;
                }
            }
            else
            {
                Irp->IoStatus.Information = sizeof(ULONG);
            }
            break;

        case IOCTL_BATTERY_QUERY_STATUS:
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(BattWait) ||
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BATTERY_STATUS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            BattWait = *(PBATTERY_WAIT_STATUS)Irp->AssociatedIrp.SystemBuffer;

            Timeout.QuadPart = Int32x32To64(BattWait.Timeout, -1000);

            BattStatus = Irp->AssociatedIrp.SystemBuffer;
            Status = BattClass->MiniportInfo.QueryStatus(BattClass->MiniportInfo.Context,
                                                         BattWait.BatteryTag,
                                                         BattStatus);

            if (!NT_SUCCESS(Status) ||
                (BattWait.PowerState == BattStatus->PowerState &&
                 BattWait.HighCapacity >= BattStatus->Capacity &&
                 BattWait.LowCapacity <= BattStatus->Capacity))
            {
                BattNotify.PowerState = BattWait.PowerState;
                BattNotify.HighCapacity = BattWait.HighCapacity;
                BattNotify.LowCapacity = BattWait.LowCapacity;

                BattClass->MiniportInfo.SetStatusNotify(BattClass->MiniportInfo.Context,
                                                        BattWait.BatteryTag,
                                                        &BattNotify);

                ExAcquireFastMutex(&BattClass->Mutex);
                BattClass->EventTrigger = EVENT_BATTERY_STATUS;
                BattClass->EventTriggerContext = &BattWait;
                BattClass->Waiting = TRUE;
                ExReleaseFastMutex(&BattClass->Mutex);

                Status = KeWaitForSingleObject(&BattClass->WaitEvent,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               BattWait.Timeout != -1 ? &Timeout : NULL);
                if (Status == STATUS_TIMEOUT)
                    Status = STATUS_SUCCESS;

                ExAcquireFastMutex(&BattClass->Mutex);
                BattClass->Waiting = FALSE;
                ExReleaseFastMutex(&BattClass->Mutex);

                BattClass->MiniportInfo.DisableStatusNotify(BattClass->MiniportInfo.Context);
            }
            else
            {
                Irp->IoStatus.Information = sizeof(BATTERY_STATUS);
            }
            break;

        case IOCTL_BATTERY_QUERY_INFORMATION:
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*BattQueryInfo))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            BattQueryInfo = Irp->AssociatedIrp.SystemBuffer;

            Status = BattClass->MiniportInfo.QueryInformation(BattClass->MiniportInfo.Context,
                                                              BattQueryInfo->BatteryTag,
                                                              BattQueryInfo->InformationLevel,
                                                              BattQueryInfo->AtRate,
                                                              Irp->AssociatedIrp.SystemBuffer,
                                                              IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                                              &ReturnedLength);
            Irp->IoStatus.Information = ReturnedLength;
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("QueryInformation failed (0x%x)\n", Status);
            }
            break;

        case IOCTL_BATTERY_SET_INFORMATION:
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*BattSetInfo))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            BattSetInfo = Irp->AssociatedIrp.SystemBuffer;

            Status = BattClass->MiniportInfo.SetInformation(BattClass->MiniportInfo.Context,
                                                            BattSetInfo->BatteryTag,
                                                            BattSetInfo->InformationLevel,
                                                            BattSetInfo->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SetInformation failed (0x%x)\n", Status);
            }
            break;

        default:
            DPRINT1("Received unsupported IRP %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            /* Do NOT complete the irp */
            return STATUS_NOT_SUPPORTED;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
