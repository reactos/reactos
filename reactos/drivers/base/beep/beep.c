/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/base/beep/beep.c
 * PURPOSE:         Beep Device Driver
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <ntddk.h>
#include <ntddbeep.h>
#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

/* TYPES *********************************************************************/

typedef struct _BEEP_DEVICE_EXTENSION
{
    LONG ReferenceCount;
    FAST_MUTEX Mutex;
    KTIMER Timer;
    LONG TimerActive;
    PVOID SectionHandle;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
BeepDPC(IN PKDPC Dpc,
        IN PDEVICE_OBJECT DeviceObject,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    /* Stop the beep */
    HalMakeBeep(0);

    /* Disable the timer */
    InterlockedDecrement(&DeviceExtension->TimerActive);
}

NTSTATUS
NTAPI
BeepCreate(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    /* Acquire the mutex and increase reference count */
    ExAcquireFastMutex(&DeviceExtension->Mutex);
    if (++DeviceExtension->ReferenceCount == 1)
    {
        /* First reference, lock the data section */
        DeviceExtension->SectionHandle = MmLockPagableDataSection(BeepCreate);
    }

    /* Release it */
    ExReleaseFastMutex(&DeviceExtension->Mutex);

    /* Complete the request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
BeepClose(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    /* Acquire the mutex and decrease reference count */
    ExAcquireFastMutex(&DeviceExtension->Mutex);
    if (!(--DeviceExtension->ReferenceCount))
    {
        /* Check for active timer */
        if (DeviceExtension->TimerActive)
        {
            /* Cancel it */
            if (KeCancelTimer(&DeviceExtension->Timer))
            {
                /* Mark it as cancelled */
                InterlockedDecrement(&DeviceExtension->TimerActive);
            }
        }

        /* Page the driver */
        MmUnlockPagableImageSection(DeviceExtension->SectionHandle);
    }

    /* Release the lock */
    ExReleaseFastMutex(&DeviceExtension->Mutex);

    /* Complete the request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID
NTAPI
BeepCancel(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    /* Check if this is the current request */
    if (Irp == DeviceObject->CurrentIrp)
    {
        /* Clear it */
        DeviceObject->CurrentIrp = NULL;

        /* Release the cancel lock and start the next packet */
        IoReleaseCancelSpinLock(Irp->CancelIrql);
        IoStartNextPacket(DeviceObject, TRUE);
    }
    else
    {
        /* Otherwise, remove the packet from the queue and relelase the lock */
        KeRemoveEntryDeviceQueue(&DeviceObject->DeviceQueue,
                                 &Irp->Tail.Overlay.DeviceQueueEntry);
        IoReleaseCancelSpinLock(Irp->CancelIrql);
    }

    /* Complete the request */
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
BeepCleanup(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    KIRQL OldIrql, CancelIrql;
    PKDEVICE_QUEUE_ENTRY Packet;
    PIRP CurrentIrp;

    /* Raise IRQL and acquire the cancel lock */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    IoAcquireCancelSpinLock(&CancelIrql);

    /* Get the current IRP */
    CurrentIrp = DeviceObject->CurrentIrp;
    DeviceObject->CurrentIrp = NULL;
    while (CurrentIrp)
    {
        /* Clear its cancel routine */
        (VOID)IoSetCancelRoutine(CurrentIrp, NULL);

        /* Cancel the IRP */
        CurrentIrp->IoStatus.Status = STATUS_CANCELLED;
        CurrentIrp->IoStatus.Information = 0;

        /* Release the cancel lock and complete it */
        IoReleaseCancelSpinLock(CancelIrql);
        IoCompleteRequest(CurrentIrp, IO_NO_INCREMENT);

        /* Reacquire the lock and get the next queue packet */
        IoAcquireCancelSpinLock(&CancelIrql);
        Packet = KeRemoveDeviceQueue(&DeviceObject->DeviceQueue);
        if (Packet)
        {
            /* Get the IRP */
            CurrentIrp = CONTAINING_RECORD(Packet,
                                           IRP,
                                           Tail.Overlay.DeviceQueueEntry);
        }
        else
        {
            /* No more IRPs */
            CurrentIrp = NULL;
        }
    }

    /* Release lock and go back to low IRQL */
    IoReleaseCancelSpinLock(CancelIrql);
    KeLowerIrql(OldIrql);

    /* Complete the IRP */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* Stop and beep and return */
    HalMakeBeep(0);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
BeepDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PBEEP_SET_PARAMETERS BeepParam;
    NTSTATUS Status;

    /* Get the stack location and parameters */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    BeepParam = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

    /* We only support one IOCTL */
    if (Stack->Parameters.DeviceIoControl.IoControlCode != IOCTL_BEEP_SET)
    {
        /* Unsupported command */
        Status = STATUS_NOT_IMPLEMENTED;
    }
    else
    {
        /* Validate the input buffer length */
        if (Stack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(BEEP_SET_PARAMETERS))
        {
            /* Invalid buffer */
            Status = STATUS_INVALID_PARAMETER;
        }
        else if ((BeepParam->Frequency != 0) && !(BeepParam->Duration))
        {
            /* No duration, return imemdiately */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* We'll queue this request */
            Status = STATUS_PENDING;
        }
    }

    /* Set packet information */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    /* Check if we're completing or queuing a packet */
    if (Status == STATUS_PENDING)
    {
        /* Start the queue */
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, BeepCancel);
    }
    else
    {
        /* Complete the request */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Return */
    return Status;
}

VOID
NTAPI
BeepUnload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;

    /* Get DO and DE */
    DeviceObject = DriverObject->DeviceObject;
    DeviceExtension = DeviceObject->DeviceExtension;

    /* Check if the timer is active */
    if (DeviceExtension->TimerActive)
    {
        /* Cancel it */
        if (KeCancelTimer(&DeviceExtension->Timer))
        {
            /* All done */
            InterlockedDecrement(&DeviceExtension->TimerActive);
        }
    }

    /* Delete the object */
    IoDeleteDevice(DeviceObject);
}

VOID
NTAPI
BeepStartIo(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL CancelIrql;
    PIO_STACK_LOCATION IoStack;
    PBEEP_SET_PARAMETERS BeepParam;
    LARGE_INTEGER DueTime;
    NTSTATUS Status;

    /* Acquire the cancel lock and make sure the IRP is valid */
    IoAcquireCancelSpinLock(&CancelIrql);
    if (!Irp)
    {
        /* It's not, release the lock and quit */
        IoReleaseCancelSpinLock(CancelIrql);
        return;
    }

    /* Remove the cancel routine and release the lock */
    (VOID)IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(CancelIrql);

    /* Get the I/O Stack and make sure the request is valid */
    BeepParam = (PBEEP_SET_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_BEEP_SET)
    {
        /* Check if we have an active timer */
        if (DeviceExtension->TimerActive)
        {
            /* Cancel it */
            if (KeCancelTimer(&DeviceExtension->Timer))
            {
                /* Set the state */
                InterlockedDecrement(&DeviceExtension->TimerActive);
            }
        }

        /* Make the beep */
        if (HalMakeBeep(BeepParam->Frequency))
        {
            /* Beep successful, queue a DPC to stop it */
            Status = STATUS_SUCCESS;
            DueTime.QuadPart = BeepParam->Duration * -10000;
            InterlockedIncrement(&DeviceExtension->TimerActive);
            KeSetTimer(&DeviceExtension->Timer, DueTime, &DeviceObject->Dpc);
        }
        else
        {
            /* Beep has failed */
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Invalid request */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Complete the request and start the next packet */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoStartNextPacket(DeviceObject, TRUE);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Beep");
    NTSTATUS Status;

    /* Create the device */
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_BEEP,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make it use buffered I/O */
    DeviceObject->Flags |= DO_BUFFERED_IO;

    /* Setup the Driver Object */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = BeepCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BeepClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = BeepCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BeepDeviceControl;
    DriverObject->DriverUnload = BeepUnload;
    DriverObject->DriverStartIo = BeepStartIo;

    /* Set up device extension */
    DeviceExtension = DeviceObject->DeviceExtension;
    DeviceExtension->ReferenceCount = 0;
    DeviceExtension->TimerActive = FALSE;
    IoInitializeDpcRequest(DeviceObject, BeepDPC);
    KeInitializeTimer(&DeviceExtension->Timer);
    ExInitializeFastMutex(&DeviceExtension->Mutex);

    /* Page the entire driver */
    MmPageEntireDriver(DriverEntry);
    return STATUS_SUCCESS;
}

/* EOF */
