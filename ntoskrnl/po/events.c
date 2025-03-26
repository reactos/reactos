/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/po/events.c
 * PURPOSE:         Power Manager
 * PROGRAMMERS:     Herv� Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

typedef struct _SYS_BUTTON_CONTEXT
{
    PDEVICE_OBJECT DeviceObject;
    PIO_WORKITEM WorkItem;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG SysButton;
} SYS_BUTTON_CONTEXT, *PSYS_BUTTON_CONTEXT;

static VOID
NTAPI
PopGetSysButton(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context);

PKWIN32_POWEREVENT_CALLOUT PopEventCallout;
extern PCALLBACK_OBJECT SetSystemTimeCallback;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
PoNotifySystemTimeSet(VOID)
{
    KIRQL OldIrql;

    /* Check if Win32k registered a notification callback */
    if (PopEventCallout)
    {
        /* Raise to dispatch */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        /* Notify the callback */
        ExNotifyCallback(SetSystemTimeCallback, NULL, NULL);

        /* Lower IRQL back */
        KeLowerIrql(OldIrql);
    }
}

static NTSTATUS
NTAPI
PopGetSysButtonCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PSYS_BUTTON_CONTEXT SysButtonContext = Context;
    ULONG SysButton;

    /* The DeviceObject can be NULL, so use the one we stored */
    DeviceObject = SysButtonContext->DeviceObject;

    /* FIXME: What do do with the sys button event? */
    SysButton = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
    {
        DPRINT1("A device reported the event 0x%x (", SysButton);
        if (SysButton & SYS_BUTTON_POWER) DbgPrint(" POWER");
        if (SysButton & SYS_BUTTON_SLEEP) DbgPrint(" SLEEP");
        if (SysButton & SYS_BUTTON_LID) DbgPrint(" LID");
        if (SysButton == 0) DbgPrint(" WAKE");
        DbgPrint(" )\n");

        if (SysButton & SYS_BUTTON_POWER)
        {
            /* FIXME: Read registry for the action we should perform here */
            DPRINT1("Initiating shutdown after power button event\n");

            ZwShutdownSystem(ShutdownNoReboot);
        }
    }

    /* Allocate a new workitem to send the next IOCTL_GET_SYS_BUTTON_EVENT */
    SysButtonContext->WorkItem = IoAllocateWorkItem(DeviceObject);
    if (!SysButtonContext->WorkItem)
    {
        DPRINT("IoAllocateWorkItem() failed\n");
        ExFreePoolWithTag(SysButtonContext, 'IWOP');
        return STATUS_SUCCESS;
    }
    IoQueueWorkItem(SysButtonContext->WorkItem,
                    PopGetSysButton,
                    DelayedWorkQueue,
                    SysButtonContext);

    return STATUS_SUCCESS /* STATUS_CONTINUE_COMPLETION */;
}

static VOID
NTAPI
PopGetSysButton(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PSYS_BUTTON_CONTEXT SysButtonContext = Context;
    PIO_WORKITEM CurrentWorkItem = SysButtonContext->WorkItem;
    PIRP Irp;

    /* Get button pressed (IOCTL_GET_SYS_BUTTON_EVENT) */
    KeInitializeEvent(&SysButtonContext->Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_GET_SYS_BUTTON_EVENT,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &SysButtonContext->SysButton,
                                        sizeof(SysButtonContext->SysButton),
                                        FALSE,
                                        &SysButtonContext->Event,
                                        &SysButtonContext->IoStatusBlock);
    if (Irp)
    {
        IoSetCompletionRoutine(Irp,
                               PopGetSysButtonCompletion,
                               SysButtonContext,
                               TRUE,
                               FALSE,
                               FALSE);
        IoCallDriver(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("IoBuildDeviceIoControlRequest() failed\n");
        ExFreePoolWithTag(SysButtonContext, 'IWOP');
    }

    IoFreeWorkItem(CurrentWorkItem);
}

NTSTATUS
NTAPI
PopAddRemoveSysCapsCallback(IN PVOID NotificationStructure,
                            IN PVOID Context)
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;
    PSYS_BUTTON_CONTEXT SysButtonContext;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    BOOLEAN Arrival;
    ULONG Caps;
    NTSTATUS Status;
    POP_POLICY_DEVICE_TYPE DeviceType = (POP_POLICY_DEVICE_TYPE)(ULONG_PTR)Context;

    DPRINT("PopAddRemoveSysCapsCallback(%p %p)\n",
        NotificationStructure, Context);

    Notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
    if (Notification->Version != 1)
        return STATUS_REVISION_MISMATCH;
    if (Notification->Size != sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION))
        return STATUS_INVALID_PARAMETER;
    if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID)) == sizeof(GUID))
        Arrival = TRUE;
    else if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID)) == sizeof(GUID))
        Arrival = FALSE;
    else
        return STATUS_INVALID_PARAMETER;

    if (Arrival && DeviceType == PolicyDeviceBattery)
    {
        PopCapabilities.SystemBatteriesPresent = TRUE;
        return STATUS_SUCCESS;
    }

    if (Arrival)
    {
        DPRINT("Arrival of %wZ\n", Notification->SymbolicLinkName);

        /* Open the device */
        InitializeObjectAttributes(&ObjectAttributes,
                                   Notification->SymbolicLinkName,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        Status = ZwOpenFile(&FileHandle,
                            FILE_READ_DATA,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwOpenFile() failed with status 0x%08lx\n", Status);
            return Status;
        }
        Status = ObReferenceObjectByHandle(FileHandle,
                                           FILE_READ_DATA,
                                           IoFileObjectType,
                                           KernelMode,
                                           (PVOID*)&FileObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ObReferenceObjectByHandle() failed with status 0x%08lx\n", Status);
            ZwClose(FileHandle);
            return Status;
        }
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
        ObDereferenceObject(FileObject);

        /* Get capabilities (IOCTL_GET_SYS_BUTTON_CAPS) */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_GET_SYS_BUTTON_CAPS,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            &Caps,
                                            sizeof(Caps),
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            DPRINT1("IoBuildDeviceIoControlRequest() failed\n");
            ZwClose(FileHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            DPRINT("IOCTL_GET_SYS_BUTTON_CAPS pending\n");
            KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Sending IOCTL_GET_SYS_BUTTON_CAPS failed with status 0x%08x\n", Status);
            ZwClose(FileHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DPRINT("Device capabilities: 0x%x\n", Caps);
        if (Caps & SYS_BUTTON_POWER)
        {
            DPRINT("POWER button present\n");
            PopCapabilities.PowerButtonPresent = TRUE;
        }

        if (Caps & SYS_BUTTON_SLEEP)
        {
            DPRINT("SLEEP button present\n");
            PopCapabilities.SleepButtonPresent = TRUE;
        }

        if (Caps & SYS_BUTTON_LID)
        {
            DPRINT("LID present\n");
            PopCapabilities.LidPresent = TRUE;
        }

        SysButtonContext = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(SYS_BUTTON_CONTEXT),
                                                 'IWOP');
        if (!SysButtonContext)
        {
            DPRINT1("ExAllocatePoolWithTag() failed\n");
            ZwClose(FileHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Queue a work item to get sys button event */
        SysButtonContext->WorkItem = IoAllocateWorkItem(DeviceObject);
        SysButtonContext->DeviceObject = DeviceObject;
        if (!SysButtonContext->WorkItem)
        {
            DPRINT1("IoAllocateWorkItem() failed\n");
            ZwClose(FileHandle);
            ExFreePoolWithTag(SysButtonContext, 'IWOP');
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        IoQueueWorkItem(SysButtonContext->WorkItem,
                        PopGetSysButton,
                        DelayedWorkQueue,
                        SysButtonContext);

        ZwClose(FileHandle);
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Removal of a power capable device not implemented\n");
        return STATUS_NOT_IMPLEMENTED;
    }
}
