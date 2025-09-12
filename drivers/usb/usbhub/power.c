/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBHub power handling functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbhub.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBHUB_POWER
#include "dbg_uhub.h"

VOID
NTAPI
USBH_CompletePowerIrp(IN PUSBHUB_FDO_EXTENSION HubExtension,
                      IN PIRP Irp,
                      IN NTSTATUS NtStatus)
{
    DPRINT("USBH_CompletePowerIrp: HubExtension - %p, Irp - %p, NtStatus - %lX\n",
           HubExtension,
           Irp,
           NtStatus);

    Irp->IoStatus.Status = NtStatus;

    PoStartNextPowerIrp(Irp);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
NTAPI
USBH_HubCancelWakeIrp(IN PUSBHUB_FDO_EXTENSION HubExtension,
                      IN PIRP Irp)
{
    DPRINT("USBH_HubCancelWakeIrp: HubExtension - %p, Irp - %p\n",
           HubExtension,
           Irp);

    IoCancelIrp(Irp);

    if (InterlockedExchange((PLONG)&HubExtension->FdoWaitWakeLock, 1))
    {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

VOID
NTAPI
USBH_HubESDRecoverySetD3Completion(IN PDEVICE_OBJECT DeviceObject,
                                   IN UCHAR MinorFunction,
                                   IN POWER_STATE PowerState,
                                   IN PVOID Context,
                                   IN PIO_STATUS_BLOCK IoStatus)
{
    DPRINT("USBH_HubESDRecoverySetD3Completion ... \n");

    KeSetEvent((PRKEVENT)Context,
               EVENT_INCREMENT,
               FALSE);
}

NTSTATUS
NTAPI
USBH_HubSetD0(IN PUSBHUB_FDO_EXTENSION HubExtension)
{
    PUSBHUB_FDO_EXTENSION RootHubDevExt;
    NTSTATUS Status;
    KEVENT Event;
    POWER_STATE PowerState;

    DPRINT("USBH_HubSetD0: HubExtension - %p\n", HubExtension);

    RootHubDevExt = USBH_GetRootHubExtension(HubExtension);

    if (RootHubDevExt->SystemPowerState.SystemState != PowerSystemWorking)
    {
        Status = STATUS_INVALID_DEVICE_STATE;
        return Status;
    }

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST)
    {
        DPRINT("USBH_HubSetD0: HubFlags - %lX\n", HubExtension->HubFlags);

        KeWaitForSingleObject(&HubExtension->IdleEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    PowerState.DeviceState = PowerDeviceD0;

    Status = PoRequestPowerIrp(HubExtension->LowerPDO,
                               IRP_MN_SET_POWER,
                               PowerState,
                               USBH_HubESDRecoverySetD3Completion,
                               &Event,
                               NULL);

    if (Status == STATUS_PENDING)
    {
       Status = KeWaitForSingleObject(&Event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);
    }

    while (HubExtension->HubFlags & USBHUB_FDO_FLAG_WAKEUP_START)
    {
        USBH_Wait(10);
    }

    return Status;
}

VOID
NTAPI
USBH_IdleCancelPowerHubWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                              IN PVOID Context)
{
    PUSBHUB_IDLE_PORT_CANCEL_CONTEXT WorkItemIdlePower;
    PIRP Irp;

    DPRINT("USBH_IdleCancelPowerHubWorker: ... \n");

    WorkItemIdlePower = Context;

    if (HubExtension &&
        HubExtension->CurrentPowerState.DeviceState != PowerDeviceD0 &&
        HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_HubSetD0(HubExtension);
    }

    Irp = WorkItemIdlePower->Irp;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
NTAPI
USBH_HubQueuePortWakeIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                          IN PLIST_ENTRY ListIrps)
{
    PDEVICE_OBJECT PortDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    USHORT NumPorts;
    USHORT Port;
    PIRP WakeIrp;
    KIRQL OldIrql;

    DPRINT("USBH_HubQueuePortWakeIrps ... \n");

    NumPorts = HubExtension->HubDescriptor->bNumberOfPorts;

    InitializeListHead(ListIrps);

    IoAcquireCancelSpinLock(&OldIrql);

    for (Port = 0; Port < NumPorts; ++Port)
    {
        PortDevice = HubExtension->PortData[Port].DeviceObject;

        if (PortDevice)
        {
            PortExtension = PortDevice->DeviceExtension;

            WakeIrp = PortExtension->PdoWaitWakeIrp;
            PortExtension->PdoWaitWakeIrp = NULL;

            if (WakeIrp)
            {
                DPRINT1("USBH_HubQueuePortWakeIrps: UNIMPLEMENTED. FIXME\n");
                DbgBreakPoint();
            }
        }
    }

    IoReleaseCancelSpinLock(OldIrql);
}

VOID
NTAPI
USBH_HubCompleteQueuedPortWakeIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                   IN PLIST_ENTRY ListIrps,
                                   IN NTSTATUS NtStatus)
{
    DPRINT("USBH_HubCompleteQueuedPortWakeIrps ... \n");

    while (!IsListEmpty(ListIrps))
    {
        DPRINT1("USBH_HubCompleteQueuedPortWakeIrps: UNIMPLEMENTED. FIXME\n");
        DbgBreakPoint();
    }
}

VOID
NTAPI
USBH_HubCompletePortWakeIrps(IN PUSBHUB_FDO_EXTENSION HubExtension,
                             IN NTSTATUS NtStatus)
{
    LIST_ENTRY ListIrps;

    DPRINT("USBH_HubCompletePortWakeIrps: NtStatus - %x\n", NtStatus);

    if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
    {
        USBH_HubQueuePortWakeIrps(HubExtension, &ListIrps);

        USBH_HubCompleteQueuedPortWakeIrps(HubExtension,
                                           &ListIrps,
                                           NtStatus);
    }
}

VOID
NTAPI
USBH_FdoPoRequestD0Completion(IN PDEVICE_OBJECT DeviceObject,
                              IN UCHAR MinorFunction,
                              IN POWER_STATE PowerState,
                              IN PVOID Context,
                              IN PIO_STATUS_BLOCK IoStatus)
{
    PUSBHUB_FDO_EXTENSION HubExtension;

    DPRINT("USBH_FdoPoRequestD0Completion ... \n");

    HubExtension = Context;

    USBH_HubCompletePortWakeIrps(HubExtension, STATUS_SUCCESS);

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_WAKEUP_START;

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }
}

VOID
NTAPI
USBH_CompletePortWakeIrpsWorker(IN PUSBHUB_FDO_EXTENSION HubExtension,
                                IN PVOID Context)
{
    DPRINT1("USBH_CompletePortWakeIrpsWorker: UNIMPLEMENTED. FIXME\n");
    DbgBreakPoint();
}

NTSTATUS
NTAPI
USBH_FdoWWIrpIoCompletion(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp,
                          IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    NTSTATUS Status;
    KIRQL OldIrql;
    POWER_STATE PowerState;
    PIRP WakeIrp;

    DPRINT("USBH_FdoWWIrpIoCompletion: DeviceObject - %p, Irp - %p\n",
            DeviceObject,
            Irp);

    HubExtension = Context;

    Status = Irp->IoStatus.Status;

    IoAcquireCancelSpinLock(&OldIrql);

    HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_PENDING_WAKE_IRP;

    WakeIrp = InterlockedExchangePointer((PVOID *)&HubExtension->PendingWakeIrp,
                                         NULL);

    if (!InterlockedDecrement(&HubExtension->PendingRequestCount))
    {
        KeSetEvent(&HubExtension->PendingRequestEvent,
                   EVENT_INCREMENT,
                   FALSE);
    }

    IoReleaseCancelSpinLock(OldIrql);

    DPRINT("USBH_FdoWWIrpIoCompletion: Status - %lX\n", Status);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBH_FdoWWIrpIoCompletion: DbgBreakPoint() \n");
        DbgBreakPoint();
    }
    else
    {
        PowerState.DeviceState = PowerDeviceD0;

        HubExtension->HubFlags |= USBHUB_FDO_FLAG_WAKEUP_START;
        InterlockedIncrement(&HubExtension->PendingRequestCount);

        Status = STATUS_SUCCESS;

        PoRequestPowerIrp(HubExtension->LowerPDO,
                          IRP_MN_SET_POWER,
                          PowerState,
                          USBH_FdoPoRequestD0Completion,
                          (PVOID)HubExtension,
                          NULL);
    }

    if (!WakeIrp)
    {
        if (!InterlockedExchange(&HubExtension->FdoWaitWakeLock, 1))
        {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    DPRINT("USBH_FdoWWIrpIoCompletion: Status - %lX\n", Status);

    if (Status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        PoStartNextPowerIrp(Irp);
    }

    return Status;
}

NTSTATUS
NTAPI
USBH_PowerIrpCompletion(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp,
                        IN PVOID Context)
{
    PUSBHUB_FDO_EXTENSION HubExtension;
    PIO_STACK_LOCATION IoStack;
    DEVICE_POWER_STATE OldDeviceState;
    NTSTATUS Status;
    POWER_STATE PowerState;

    DPRINT("USBH_PowerIrpCompletion: DeviceObject - %p, Irp - %p\n",
           DeviceObject,
           Irp);

    HubExtension = Context;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    PowerState = IoStack->Parameters.Power.State;

    Status = Irp->IoStatus.Status;
    DPRINT("USBH_PowerIrpCompletion: Status - %lX\n", Status);

    if (!NT_SUCCESS(Status))
    {
        if (PowerState.DeviceState == PowerDeviceD0)
        {
            PoStartNextPowerIrp(Irp);
            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_SET_D0_STATE;
        }
    }
    else if (PowerState.DeviceState == PowerDeviceD0)
    {
        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_SET_D0_STATE;

        OldDeviceState = HubExtension->CurrentPowerState.DeviceState;
        HubExtension->CurrentPowerState.DeviceState = PowerDeviceD0;

        DPRINT("USBH_PowerIrpCompletion: OldDeviceState - %x\n", OldDeviceState);

        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_HIBERNATE_STATE)
        {
            DPRINT1("USBH_PowerIrpCompletion: USBHUB_FDO_FLAG_HIBERNATE_STATE. FIXME\n");
            DbgBreakPoint();
        }

        HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_HIBERNATE_STATE;

        if (OldDeviceState == PowerDeviceD3)
        {
            DPRINT1("USBH_PowerIrpCompletion: PowerDeviceD3. FIXME\n");
            DbgBreakPoint();
        }

        if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPED) &&
            HubExtension->HubFlags & USBHUB_FDO_FLAG_DO_ENUMERATION)
        {
            USBH_SubmitStatusChangeTransfer(HubExtension);
        }

        DPRINT("USBH_PowerIrpCompletion: Status - %lX\n", Status);

        if (Status != STATUS_MORE_PROCESSING_REQUIRED)
        {
            PoStartNextPowerIrp(Irp);
            return Status;
        }
    }

    return Status;
}

VOID
NTAPI
USBH_FdoDeferPoRequestCompletion(IN PDEVICE_OBJECT DeviceObject,
                                 IN UCHAR MinorFunction,
                                 IN POWER_STATE PowerState,
                                 IN PVOID Context,
                                 IN PIO_STATUS_BLOCK IoStatus)
{
    PUSBHUB_FDO_EXTENSION Extension;
    PUSBHUB_FDO_EXTENSION HubExtension = NULL;
    PIRP PowerIrp;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBH_FdoDeferPoRequestCompletion ... \n");

    Extension = Context;

    PowerIrp = Extension->PowerIrp;

    if (Extension->Common.ExtensionType == USBH_EXTENSION_TYPE_HUB)
    {
        HubExtension = Context;
    }

    IoStack = IoGetCurrentIrpStackLocation(PowerIrp);

    if (IoStack->Parameters.Power.State.SystemState == PowerSystemWorking &&
        HubExtension && HubExtension->LowerPDO == HubExtension->RootHubPdo)
    {
        HubExtension->SystemPowerState.SystemState = PowerSystemWorking;
        USBH_CheckIdleDeferred(HubExtension);
    }

    IoCopyCurrentIrpStackLocationToNext(PowerIrp);
    PoStartNextPowerIrp(PowerIrp);
    PoCallDriver(Extension->LowerDevice, PowerIrp);
}

NTSTATUS
NTAPI
USBH_FdoPower(IN PUSBHUB_FDO_EXTENSION HubExtension,
              IN PIRP Irp,
              IN UCHAR Minor)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    POWER_STATE PowerState;
    POWER_STATE DevicePwrState;
    BOOLEAN IsAllPortsD3;
    PUSBHUB_PORT_DATA PortData;
    PDEVICE_OBJECT PdoDevice;
    PUSBHUB_PORT_PDO_EXTENSION PortExtension;
    ULONG Port;

    DPRINT_PWR("USBH_FdoPower: HubExtension - %p, Irp - %p, Minor - %X\n",
               HubExtension,
               Irp,
               Minor);

    switch (Minor)
    {
        case IRP_MN_WAIT_WAKE:
            DPRINT_PWR("USBH_FdoPower: IRP_MN_WAIT_WAKE\n");

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(Irp,
                                   USBH_FdoWWIrpIoCompletion,
                                   HubExtension,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            PoStartNextPowerIrp(Irp);
            IoMarkIrpPending(Irp);
            PoCallDriver(HubExtension->LowerDevice, Irp);

            return STATUS_PENDING;

        case IRP_MN_POWER_SEQUENCE:
            DPRINT_PWR("USBH_FdoPower: IRP_MN_POWER_SEQUENCE\n");
            break;

        case IRP_MN_SET_POWER:
            DPRINT_PWR("USBH_FdoPower: IRP_MN_SET_POWER\n");

            IoStack = IoGetCurrentIrpStackLocation(Irp);
            DPRINT_PWR("USBH_FdoPower: IRP_MN_SET_POWER/DevicePowerState\n");
            PowerState = IoStack->Parameters.Power.State;

            if (IoStack->Parameters.Power.Type == DevicePowerState)
            {
                DPRINT_PWR("USBH_FdoPower: PowerState - %x\n",
                           PowerState.DeviceState);

                if (HubExtension->CurrentPowerState.DeviceState == PowerState.DeviceState)
                {
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    PoStartNextPowerIrp(Irp);
                    IoMarkIrpPending(Irp);
                    PoCallDriver(HubExtension->LowerDevice, Irp);

                    return STATUS_PENDING;
                }

                switch (PowerState.DeviceState)
                {
                    case PowerDeviceD0:
                        if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_SET_D0_STATE))
                        {
                            HubExtension->HubFlags &= ~(USBHUB_FDO_FLAG_NOT_D0_STATE |
                                                        USBHUB_FDO_FLAG_DEVICE_STOPPING);

                            HubExtension->HubFlags |= USBHUB_FDO_FLAG_SET_D0_STATE;

                            IoCopyCurrentIrpStackLocationToNext(Irp);

                            IoSetCompletionRoutine(Irp,
                                                   USBH_PowerIrpCompletion,
                                                   HubExtension,
                                                   TRUE,
                                                   TRUE,
                                                   TRUE);
                        }
                        else
                        {
                            IoCopyCurrentIrpStackLocationToNext(Irp);
                            PoStartNextPowerIrp(Irp);
                        }

                        IoMarkIrpPending(Irp);
                        PoCallDriver(HubExtension->LowerDevice, Irp);
                        return STATUS_PENDING;

                    case PowerDeviceD1:
                    case PowerDeviceD2:
                    case PowerDeviceD3:
                        if (HubExtension->ResetRequestCount)
                        {
                            IoCancelIrp(HubExtension->ResetPortIrp);

                            KeWaitForSingleObject(&HubExtension->ResetEvent,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  NULL);
                        }

                        if (!(HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STOPPED))
                        {
                            HubExtension->HubFlags |= (USBHUB_FDO_FLAG_NOT_D0_STATE |
                                                       USBHUB_FDO_FLAG_DEVICE_STOPPING);

                            IoCancelIrp(HubExtension->SCEIrp);

                            KeWaitForSingleObject(&HubExtension->StatusChangeEvent,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  NULL);
                        }

                        HubExtension->CurrentPowerState.DeviceState = PowerState.DeviceState;

                        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DO_SUSPENSE &&
                            USBH_CheckIdleAbort(HubExtension, TRUE, TRUE) == TRUE)
                        {
                            HubExtension->HubFlags &= ~(USBHUB_FDO_FLAG_NOT_D0_STATE |
                                                        USBHUB_FDO_FLAG_DEVICE_STOPPING);

                            HubExtension->CurrentPowerState.DeviceState = PowerDeviceD0;

                            USBH_SubmitStatusChangeTransfer(HubExtension);

                            PoStartNextPowerIrp(Irp);

                            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);

                            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_DO_SUSPENSE;

                            KeReleaseSemaphore(&HubExtension->IdleSemaphore,
                                               LOW_REALTIME_PRIORITY,
                                               1,
                                               FALSE);

                            return STATUS_UNSUCCESSFUL;
                        }

                        IoCopyCurrentIrpStackLocationToNext(Irp);

                        IoSetCompletionRoutine(Irp,
                                               USBH_PowerIrpCompletion,
                                               HubExtension,
                                               TRUE,
                                               TRUE,
                                               TRUE);

                        PoStartNextPowerIrp(Irp);
                        IoMarkIrpPending(Irp);
                        PoCallDriver(HubExtension->LowerDevice, Irp);

                        if (HubExtension->HubFlags & USBHUB_FDO_FLAG_DO_SUSPENSE)
                        {
                            HubExtension->HubFlags &= ~USBHUB_FDO_FLAG_DO_SUSPENSE;

                            KeReleaseSemaphore(&HubExtension->IdleSemaphore,
                                               LOW_REALTIME_PRIORITY,
                                               1,
                                               FALSE);
                        }

                        return STATUS_PENDING;

                    default:
                        DPRINT1("USBH_FdoPower: Unsupported PowerState.DeviceState\n");
                        DbgBreakPoint();
                        break;
                }
            }
            else
            {
                if (PowerState.SystemState != PowerSystemWorking)
                {
                    USBH_GetRootHubExtension(HubExtension)->SystemPowerState.SystemState =
                                                            PowerState.SystemState;
                }

                if (PowerState.SystemState == PowerSystemHibernate)
                {
                    HubExtension->HubFlags |= USBHUB_FDO_FLAG_HIBERNATE_STATE;
                }

                PortData = HubExtension->PortData;

                IsAllPortsD3 = TRUE;

                if (PortData && HubExtension->HubDescriptor)
                {
                    for (Port = 0;
                         Port < HubExtension->HubDescriptor->bNumberOfPorts;
                         Port++)
                    {
                        PdoDevice = PortData[Port].DeviceObject;

                        if (PdoDevice)
                        {
                            PortExtension = PdoDevice->DeviceExtension;

                            if (PortExtension->CurrentPowerState.DeviceState != PowerDeviceD3)
                            {
                                IsAllPortsD3 = FALSE;
                                break;
                            }
                        }
                    }
                }

                if (PowerState.SystemState == PowerSystemWorking)
                {
                    DevicePwrState.DeviceState = PowerDeviceD0;
                }
                else if (HubExtension->HubFlags & USBHUB_FDO_FLAG_PENDING_WAKE_IRP ||
                         !IsAllPortsD3)
                {
                    DevicePwrState.DeviceState = HubExtension->DeviceState[PowerState.SystemState];

                    if (DevicePwrState.DeviceState == PowerDeviceUnspecified)
                    {
                        goto Exit;
                    }
                }
                else
                {
                    DevicePwrState.DeviceState = PowerDeviceD3;
                }

                if (DevicePwrState.DeviceState != HubExtension->CurrentPowerState.DeviceState &&
                    HubExtension->HubFlags & USBHUB_FDO_FLAG_DEVICE_STARTED)
                {
                    HubExtension->PowerIrp = Irp;

                    IoMarkIrpPending(Irp);

                    if (PoRequestPowerIrp(HubExtension->LowerPDO,
                                          IRP_MN_SET_POWER,
                                          DevicePwrState,
                                          USBH_FdoDeferPoRequestCompletion,
                                          (PVOID)HubExtension,
                                          NULL) == STATUS_PENDING)
                    {
                        return STATUS_PENDING;
                    }

                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    PoStartNextPowerIrp(Irp);
                    PoCallDriver(HubExtension->LowerDevice, Irp);

                    return STATUS_PENDING;
                }

            Exit:

                HubExtension->SystemPowerState.SystemState = PowerState.SystemState;

                if (PowerState.SystemState == PowerSystemWorking)
                {
                    USBH_CheckIdleDeferred(HubExtension);
                }

                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);

                return PoCallDriver(HubExtension->LowerDevice, Irp);
            }

            break;

        case IRP_MN_QUERY_POWER:
            DPRINT_PWR("USBH_FdoPower: IRP_MN_QUERY_POWER\n");
            break;

        default:
            DPRINT1("USBH_FdoPower: unknown IRP_MN_POWER!\n");
            break;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    PoStartNextPowerIrp(Irp);
    Status = PoCallDriver(HubExtension->LowerDevice, Irp);

    return Status;
}

NTSTATUS
NTAPI
USBH_PdoPower(IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
              IN PIRP Irp,
              IN UCHAR Minor)
{
    NTSTATUS Status = Irp->IoStatus.Status;

    DPRINT_PWR("USBH_FdoPower: PortExtension - %p, Irp - %p, Minor - %X\n",
               PortExtension,
               Irp,
               Minor);

    switch (Minor)
    {
      case IRP_MN_WAIT_WAKE:
          DPRINT_PWR("USBHUB_PdoPower: IRP_MN_WAIT_WAKE\n");
          PoStartNextPowerIrp(Irp);
          break;

      case IRP_MN_POWER_SEQUENCE:
          DPRINT_PWR("USBHUB_PdoPower: IRP_MN_POWER_SEQUENCE\n");
          PoStartNextPowerIrp(Irp);
          break;

      case IRP_MN_SET_POWER:
          DPRINT_PWR("USBHUB_PdoPower: IRP_MN_SET_POWER\n");
          PoStartNextPowerIrp(Irp);
          break;

      case IRP_MN_QUERY_POWER:
          DPRINT_PWR("USBHUB_PdoPower: IRP_MN_QUERY_POWER\n");
          PoStartNextPowerIrp(Irp);
          break;

      default:
          DPRINT1("USBHUB_PdoPower: unknown IRP_MN_POWER!\n");
          PoStartNextPowerIrp(Irp);
          break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
