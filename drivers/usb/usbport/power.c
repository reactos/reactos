/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort power handling functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
USBPORT_CompletePdoWaitWake(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PIRP Irp;
    KIRQL OldIrql;

    DPRINT("USBPORT_CompletePdoWaitWake: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    PdoDevice = FdoExtension->RootHubPdo;
    PdoExtension = PdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->PowerWakeSpinLock, &OldIrql);

    Irp = PdoExtension->WakeIrp;

    if (Irp && IoSetCancelRoutine(Irp, NULL))
    {
        PdoExtension->WakeIrp = NULL;
        KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);

        DPRINT("USBPORT_CompletePdoWaitWake: Complete Irp - %p\n", Irp);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return;
    }

    KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);
}

VOID
NTAPI
USBPORT_HcWakeDpc(IN PRKDPC Dpc,
                  IN PVOID DeferredContext,
                  IN PVOID SystemArgument1,
                  IN PVOID SystemArgument2)
{
    DPRINT("USBPORT_HcWakeDpc: ... \n");
    USBPORT_CompletePdoWaitWake((PDEVICE_OBJECT)DeferredContext);
}

VOID
NTAPI
USBPORT_HcQueueWakeDpc(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT("USBPORT_HcQueueWakeDpc: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    KeInsertQueueDpc(&FdoExtension->HcWakeDpc, NULL, NULL);
}

VOID
NTAPI
USBPORT_CompletePendingIdleIrp(IN PDEVICE_OBJECT PdoDevice)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIRP Irp;

    DPRINT("USBPORT_CompletePendingIdleIrp: ... \n");

    PdoExtension = PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    Irp = IoCsqRemoveNextIrp(&FdoExtension->IdleIoCsq, 0);

    if (Irp)
    {
        InterlockedDecrement(&FdoExtension->IdleLockCounter);

        DPRINT("USBPORT_CompletePendingIdleIrp: Complete Irp - %p\n", Irp);

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

VOID
NTAPI
USBPORT_DoSetPowerD0(IN PDEVICE_OBJECT FdoDevice)
{
    DPRINT("USBPORT_DoSetPowerD0: FIXME!\n");
    return;
}

VOID
NTAPI
USBPORT_SuspendController(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;

    DPRINT1("USBPORT_SuspendController \n");

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    FdoExtension->TimerFlags |= USBPORT_TMFLAG_RH_SUSPENDED;

    USBPORT_FlushController(FdoDevice);

    if (FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND)
    {
        return;
    }

    FdoExtension->TimerFlags |= USBPORT_TMFLAG_HC_SUSPENDED;

    if (FdoExtension->MiniPortFlags & USBPORT_MPFLAG_INTERRUPTS_ENABLED)
    {
        FdoExtension->MiniPortFlags |= USBPORT_MPFLAG_SUSPENDED;

        USBPORT_Wait(FdoDevice, 10);
        Packet->SuspendController(FdoExtension->MiniPortExt);
    }

    FdoExtension->Flags |= USBPORT_FLAG_HC_SUSPEND;
}

NTSTATUS
NTAPI
USBPORT_ResumeController(IN PDEVICE_OBJECT FdoDevice)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    KIRQL OldIrql;
    MPSTATUS MpStatus;

    DPRINT1("USBPORT_ResumeController: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    if (!(FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND))
    {
        return Status;
    }

    KeAcquireSpinLock(&FdoExtension->TimerFlagsSpinLock, &OldIrql);

    FdoExtension->TimerFlags &= ~(USBPORT_TMFLAG_HC_SUSPENDED |
                                  USBPORT_TMFLAG_RH_SUSPENDED);

    KeReleaseSpinLock(&FdoExtension->TimerFlagsSpinLock, OldIrql);

    if (!(FdoExtension->MiniPortFlags & USBPORT_MPFLAG_SUSPENDED))
    {
        FdoExtension->Flags &= ~USBPORT_FLAG_HC_SUSPEND;
        return Status;
    }

    FdoExtension->MiniPortFlags &= ~USBPORT_MPFLAG_SUSPENDED;

    if (!Packet->ResumeController(FdoExtension->MiniPortExt))
    {
        Status = USBPORT_Wait(FdoDevice, 100);

        FdoExtension->Flags &= ~USBPORT_FLAG_HC_SUSPEND;
        return Status;
    }

    KeAcquireSpinLock(&FdoExtension->TimerFlagsSpinLock, &OldIrql);
    FdoExtension->TimerFlags |= (USBPORT_TMFLAG_HC_SUSPENDED |
                                 USBPORT_TMFLAG_HC_RESUME);
    KeReleaseSpinLock(&FdoExtension->TimerFlagsSpinLock, OldIrql);

    USBPORT_MiniportInterrupts(FdoDevice, FALSE);

    Packet->StopController(FdoExtension->MiniPortExt, 1);

    USBPORT_NukeAllEndpoints(FdoDevice);

    RtlZeroMemory(FdoExtension->MiniPortExt, Packet->MiniPortExtensionSize);

    RtlZeroMemory((PVOID)FdoExtension->UsbPortResources.StartVA,
                  Packet->MiniPortResourcesSize);

    FdoExtension->UsbPortResources.IsChirpHandled = TRUE;

    MpStatus = Packet->StartController(FdoExtension->MiniPortExt,
                                       &FdoExtension->UsbPortResources);

    FdoExtension->UsbPortResources.IsChirpHandled = FALSE;

    if (!MpStatus)
    {
        USBPORT_MiniportInterrupts(FdoDevice, TRUE);
    }

    KeAcquireSpinLock(&FdoExtension->TimerFlagsSpinLock, &OldIrql);

    FdoExtension->TimerFlags &= ~(USBPORT_TMFLAG_HC_SUSPENDED |
                                  USBPORT_TMFLAG_HC_RESUME |
                                  USBPORT_TMFLAG_RH_SUSPENDED);

    KeReleaseSpinLock(&FdoExtension->TimerFlagsSpinLock, OldIrql);

    Status = USBPORT_Wait(FdoDevice, 100);

    FdoExtension->Flags &= ~USBPORT_FLAG_HC_SUSPEND;

    return Status;
}

NTSTATUS
NTAPI
USBPORT_PdoDevicePowerState(IN PDEVICE_OBJECT PdoDevice,
                            IN PIRP Irp)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;
    POWER_STATE State;

    PdoExtension = PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    State = IoStack->Parameters.Power.State;

    DPRINT1("USBPORT_PdoDevicePowerState: Irp - %p, State - %x\n",
            Irp,
            State.DeviceState);

    if (State.DeviceState == PowerDeviceD0)
    {
        if (FdoExtension->CommonExtension.DevicePowerState == PowerDeviceD0)
        {
            // FIXME FdoExtension->Flags
            while (FdoExtension->SetPowerLockCounter)
            {
                USBPORT_Wait(FdoDevice, 10);
            }

            USBPORT_ResumeController(FdoDevice);

            PdoExtension->CommonExtension.DevicePowerState = PowerDeviceD0;

            USBPORT_CompletePdoWaitWake(FdoDevice);
            USBPORT_CompletePendingIdleIrp(PdoDevice);
        }
        else
        {
            DPRINT1("USBPORT_PdoDevicePowerState: FdoExtension->Flags - %lx\n",
                    FdoExtension->Flags);

            DbgBreakPoint();
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else if (State.DeviceState  == PowerDeviceD1 ||
             State.DeviceState  == PowerDeviceD2 ||
             State.DeviceState  == PowerDeviceD3)
    {
        FdoExtension->TimerFlags |= USBPORT_TMFLAG_WAKE;
        USBPORT_SuspendController(FdoDevice);
        PdoExtension->CommonExtension.DevicePowerState = State.DeviceState;
    }

    return Status;
}

VOID
NTAPI
USBPORT_CancelPendingWakeIrp(IN PDEVICE_OBJECT PdoDevice,
                             IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    KIRQL OldIrql;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBPORT_CancelPendingWakeIrp: ... \n");

    IoReleaseCancelSpinLock(Irp->CancelIrql);
    PdoExtension = PdoDevice->DeviceExtension;
    FdoExtension = PdoExtension->FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->PowerWakeSpinLock, &OldIrql);

    if (PdoExtension->WakeIrp == Irp)
    {
        PdoExtension->WakeIrp = NULL;
    }

    KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
USBPORT_PdoPower(IN PDEVICE_OBJECT PdoDevice,
                 IN PIRP Irp)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PIO_STACK_LOCATION IoStack;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    NTSTATUS Status;
    KIRQL OldIrql;

    DPRINT("USBPORT_PdoPower: Irp - %p\n", Irp);

    PdoExtension = PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Status = Irp->IoStatus.Status;

    switch (IoStack->MinorFunction)
    {
      case IRP_MN_WAIT_WAKE:
          DPRINT("USBPORT_PdoPower: IRP_MN_WAIT_WAKE\n");

          if (!(FdoExtension->Flags & USBPORT_FLAG_HC_STARTED))
          {
              /* The device does not support wake-up */
              Status = STATUS_NOT_SUPPORTED;
              break;
          }

          KeAcquireSpinLock(&FdoExtension->PowerWakeSpinLock, &OldIrql);

          IoSetCancelRoutine(Irp, USBPORT_CancelPendingWakeIrp);

          /* Check if the IRP has been cancelled */
          if (Irp->Cancel)
          {
              if (IoSetCancelRoutine(Irp, NULL))
              {
                  /* IRP has been cancelled, release cancel spinlock */
                  KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);

                  DPRINT("USBPORT_PdoPower: IRP_MN_WAIT_WAKE - STATUS_CANCELLED\n");

                  /* IRP is cancelled */
                  Status = STATUS_CANCELLED;
                  break;
              }
          }

          if (!PdoExtension->WakeIrp)
          {
              /* The driver received the IRP
                 and is waiting for the device to signal wake-up. */

              DPRINT("USBPORT_PdoPower: IRP_MN_WAIT_WAKE - No WakeIrp\n");

              IoMarkIrpPending(Irp);
              PdoExtension->WakeIrp = Irp;

              KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);
              return STATUS_PENDING;
          }
          else
          {
              /* An IRP_MN_WAIT_WAKE request is already pending and must be
                 completed or canceled before another IRP_MN_WAIT_WAKE request
                 can be issued. */

              if (IoSetCancelRoutine(Irp, NULL))
              {
                  DPRINT("USBPORT_PdoPower: IRP_MN_WAIT_WAKE - STATUS_DEVICE_BUSY\n");

                  KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);
                  PoStartNextPowerIrp(Irp);
                  Status = STATUS_DEVICE_BUSY;
                  break;
              }
              else
              {
                  ASSERT(FALSE);
                  KeReleaseSpinLock(&FdoExtension->PowerWakeSpinLock, OldIrql);
                  return Status;
              }
          }

      case IRP_MN_POWER_SEQUENCE:
          DPRINT("USBPORT_PdoPower: IRP_MN_POWER_SEQUENCE\n");
          PoStartNextPowerIrp(Irp);
          break;

      case IRP_MN_SET_POWER:
          DPRINT("USBPORT_PdoPower: IRP_MN_SET_POWER\n");

          if (IoStack->Parameters.Power.Type == DevicePowerState)
          {
              DPRINT("USBPORT_PdoPower: IRP_MN_SET_POWER/DevicePowerState\n");
              Status = USBPORT_PdoDevicePowerState(PdoDevice, Irp);
              PoStartNextPowerIrp(Irp);
              break;
          }

          DPRINT("USBPORT_PdoPower: IRP_MN_SET_POWER/SystemPowerState \n");

          if (IoStack->Parameters.Power.State.SystemState == PowerSystemWorking)
          {
              FdoExtension->TimerFlags |= USBPORT_TMFLAG_WAKE;
          }
          else
          {
              FdoExtension->TimerFlags &= ~USBPORT_TMFLAG_WAKE;
          }

          Status = STATUS_SUCCESS;

          PoStartNextPowerIrp(Irp);
          break;

      case IRP_MN_QUERY_POWER:
          DPRINT("USBPORT_PdoPower: IRP_MN_QUERY_POWER\n");
          Status = STATUS_SUCCESS;
          PoStartNextPowerIrp(Irp);
          break;

      default:
          DPRINT1("USBPORT_PdoPower: unknown IRP_MN_POWER!\n");
          PoStartNextPowerIrp(Irp);
          break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
USBPORT_HcWake(IN PDEVICE_OBJECT FdoDevice,
               IN PIRP Irp)
{
    DPRINT1("USBPORT_HcWake: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_DevicePowerState(IN PDEVICE_OBJECT FdoDevice,
                         IN PIRP Irp)
{
    DPRINT1("USBPORT_DevicePowerState: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_SystemPowerState(IN PDEVICE_OBJECT FdoDevice,
                         IN PIRP Irp)
{
    DPRINT1("USBPORT_SystemPowerState: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_FdoPower(IN PDEVICE_OBJECT FdoDevice,
                 IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    DPRINT("USBPORT_FdoPower: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MinorFunction)
    {
      case IRP_MN_WAIT_WAKE:
          DPRINT("USBPORT_FdoPower: IRP_MN_WAIT_WAKE\n");
          Status = USBPORT_HcWake(FdoDevice, Irp);
          return Status;

      case IRP_MN_POWER_SEQUENCE:
          DPRINT("USBPORT_FdoPower: IRP_MN_POWER_SEQUENCE\n");
          break;

      case IRP_MN_SET_POWER:
          DPRINT("USBPORT_FdoPower: IRP_MN_SET_POWER\n");
          if (IoStack->Parameters.Power.Type == DevicePowerState)
          {
              Status = USBPORT_DevicePowerState(FdoDevice, Irp);
          }
          else
          {
              Status = USBPORT_SystemPowerState(FdoDevice, Irp);
          }

          if (Status != STATUS_PENDING)
              break;

          return Status;

      case IRP_MN_QUERY_POWER:
          DPRINT("USBPORT_FdoPower: IRP_MN_QUERY_POWER\n");
          Irp->IoStatus.Status = STATUS_SUCCESS;
          break;

      default:
          DPRINT1("USBPORT_FdoPower: unknown IRP_MN_POWER!\n");
          break;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    PoStartNextPowerIrp(Irp);
    return PoCallDriver(FdoExtension->CommonExtension.LowerDevice, Irp);
}

VOID
NTAPI
USBPORT_DoIdleNotificationCallback(IN PVOID Context)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PIRP NextIrp;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    PTIMER_WORK_QUEUE_ITEM IdleQueueItem;
    PDEVICE_OBJECT PdoDevice;
    PUSB_IDLE_CALLBACK_INFO IdleCallbackInfo;
    KIRQL OldIrql;

    DPRINT("USBPORT_DoIdleNotificationCallback \n");

    IdleQueueItem = Context;

    FdoDevice = IdleQueueItem->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;
    PdoDevice = FdoExtension->RootHubPdo;
    PdoExtension = PdoDevice->DeviceExtension;

    KeQuerySystemTime(&CurrentTime);

    if ((FdoExtension->IdleTime.QuadPart == 0) ||
        (((CurrentTime.QuadPart - FdoExtension->IdleTime.QuadPart) / 10000) >= 500))
    {
        if (PdoExtension->CommonExtension.DevicePowerState == PowerDeviceD0 &&
            FdoExtension->CommonExtension.DevicePowerState == PowerDeviceD0)
        {
            NextIrp = IoCsqRemoveNextIrp(&FdoExtension->IdleIoCsq, NULL);

            if (NextIrp)
            {
                IoStack = IoGetCurrentIrpStackLocation(NextIrp);
                IdleCallbackInfo = IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

                if (IdleCallbackInfo && IdleCallbackInfo->IdleCallback)
                {
                    IdleCallbackInfo->IdleCallback(IdleCallbackInfo->IdleContext);
                }

                if (NextIrp->Cancel)
                {
                    InterlockedDecrement(&FdoExtension->IdleLockCounter);

                    NextIrp->IoStatus.Status = STATUS_CANCELLED;
                    NextIrp->IoStatus.Information = 0;
                    IoCompleteRequest(NextIrp, IO_NO_INCREMENT);
                }
                else
                {
                    IoCsqInsertIrp(&FdoExtension->IdleIoCsq, NextIrp, NULL);
                }
            }
        }
    }

    KeAcquireSpinLock(&FdoExtension->TimerFlagsSpinLock, &OldIrql);
    FdoExtension->TimerFlags &= ~USBPORT_TMFLAG_IDLE_QUEUEITEM_ON;
    KeReleaseSpinLock(&FdoExtension->TimerFlagsSpinLock, OldIrql);

    ExFreePoolWithTag(IdleQueueItem, USB_PORT_TAG);
}

NTSTATUS
NTAPI
USBPORT_IdleNotification(IN PDEVICE_OBJECT PdoDevice,
                         IN PIRP Irp)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    LONG LockCounter;
    NTSTATUS Status = STATUS_PENDING;

    DPRINT("USBPORT_IdleNotification: Irp - %p\n", Irp);

    PdoExtension = PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    LockCounter = InterlockedIncrement(&FdoExtension->IdleLockCounter);

    if (LockCounter != 0)
    {
        if (Status != STATUS_PENDING)
        {
            InterlockedDecrement(&FdoExtension->IdleLockCounter);

            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return Status;
        }

        Status = STATUS_DEVICE_BUSY;
    }

    if (Status != STATUS_PENDING)
    {
        InterlockedDecrement(&FdoExtension->IdleLockCounter);

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);

    KeQuerySystemTime(&FdoExtension->IdleTime);

    IoCsqInsertIrp(&FdoExtension->IdleIoCsq, Irp, 0);

    return Status;
}

VOID
NTAPI
USBPORT_AdjustDeviceCapabilities(IN PDEVICE_OBJECT FdoDevice,
                                 IN PDEVICE_OBJECT PdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_CAPABILITIES Capabilities;

    DPRINT("USBPORT_AdjustDeviceCapabilities: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;
    PdoExtension = PdoDevice->DeviceExtension;
    Capabilities = &PdoExtension->Capabilities;

    RtlCopyMemory(Capabilities,
                  &FdoExtension->Capabilities,
                  sizeof(DEVICE_CAPABILITIES));

    Capabilities->DeviceD1 = FALSE;
    Capabilities->DeviceD2 = TRUE;

    Capabilities->Removable = FALSE;
    Capabilities->UniqueID = FALSE;

    Capabilities->WakeFromD0 = TRUE;
    Capabilities->WakeFromD1 = FALSE;
    Capabilities->WakeFromD2 = TRUE;
    Capabilities->WakeFromD3 = FALSE;

    Capabilities->Address = 0;
    Capabilities->UINumber = 0;

    if (Capabilities->SystemWake == PowerSystemUnspecified)
        Capabilities->SystemWake = PowerSystemWorking;

    Capabilities->DeviceWake = PowerDeviceD2;

    Capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
    Capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    Capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
    Capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
}
