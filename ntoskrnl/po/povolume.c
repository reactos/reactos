/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/po/povolume.c
 * PURPOSE:         Power Manager DOPE and Volume Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

typedef struct _POP_FLUSH_VOLUME
{
    LIST_ENTRY List;
    LONG Count;
    KEVENT Wait;
} POP_FLUSH_VOLUME, *PPOP_FLUSH_VOLUME;

ULONG PopFlushPolicy = 0;

KGUARDED_MUTEX PopVolumeLock;
LIST_ENTRY PopVolumeDevices;
KSPIN_LOCK PopDopeGlobalLock;

/* PRIVATE FUNCTIONS *********************************************************/

PDEVICE_OBJECT_POWER_EXTENSION
NTAPI
PopGetDope(IN PDEVICE_OBJECT DeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    KIRQL OldIrql;
    PAGED_CODE();

    /* If the device already has the dope, return it */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    if (DeviceExtension->Dope) goto Return;

    /* Allocate some dope for the device */
    Dope = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(DEVICE_OBJECT_POWER_EXTENSION),
                                 TAG_PO_DOPE);
    if (!Dope) goto Return;

    /* Initialize the initial contents of the dope */
    RtlZeroMemory(Dope, sizeof(DEVICE_OBJECT_POWER_EXTENSION));
    Dope->DeviceObject = DeviceObject;
    Dope->State = PowerDeviceUnspecified;
    InitializeListHead(&Dope->IdleList);

    /* Make sure only one caller can assign dope to a device */
    KeAcquireSpinLock(&PopDopeGlobalLock, &OldIrql);

    /* Make sure the device still has no dope */
    if (!DeviceExtension->Dope)
    {
        /* Give the local dope to this device, and remember we won the race */
        DeviceExtension->Dope = (PVOID)Dope;
        Dope = NULL;
    }

    /* Allow other dope transactions now */
    KeReleaseSpinLock(&PopDopeGlobalLock, OldIrql);

    /* Check if someone other than us already assigned the dope, so free ours */
    if (Dope) ExFreePoolWithTag(Dope, TAG_PO_DOPE);

    /* Return the dope to the caller */
Return:
    return (PDEVICE_OBJECT_POWER_EXTENSION)DeviceExtension->Dope;
}

VOID
NTAPI
PoVolumeDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    PAGED_CODE();

    /* Get dope from the device (if the device has no dope, it will receive some) */
    Dope = PopGetDope(DeviceObject);
    if (Dope)
    {
        /* Make sure we can flush safely */
        KeAcquireGuardedMutex(&PopVolumeLock);

        /* Add this volume into the list of power-manager volumes */
        if (!Dope->Volume.Flink) InsertTailList(&PopVolumeDevices, &Dope->Volume);

        /* Allow flushes to go through */
        KeReleaseGuardedMutex(&PopVolumeLock);
    }
}

VOID
NTAPI
PoRemoveVolumeDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    KIRQL OldIrql;
    PAGED_CODE();

    /* If the device already has the dope, return it */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    if (!DeviceExtension->Dope)
    {
        /* no dope */
        return;
    }

    /* Make sure we can flush safely */
    KeAcquireGuardedMutex(&PopVolumeLock);

    /* Get dope from device */
    Dope = (PDEVICE_OBJECT_POWER_EXTENSION)DeviceExtension->Dope;

    if (Dope->Volume.Flink)
    {
        /* Remove from volume from list */
        RemoveEntryList(&Dope->Volume);
    }

    /* Allow flushes to go through */
    KeReleaseGuardedMutex(&PopVolumeLock);

    /* Now remove dope from device object */
    KeAcquireSpinLock(&PopDopeGlobalLock, &OldIrql);

    /* remove from dev obj */
    DeviceExtension->Dope = NULL;

    /* Release lock */
    KeReleaseSpinLock(&PopDopeGlobalLock, OldIrql);

    /* Free dope */
    ExFreePoolWithTag(Dope, TAG_PO_DOPE);
}

VOID
NTAPI
PopFlushVolumeWorker(IN PVOID Context)
{
    PPOP_FLUSH_VOLUME FlushContext = (PPOP_FLUSH_VOLUME)Context;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    PLIST_ENTRY NextEntry;
    NTSTATUS Status;
    UCHAR Buffer[sizeof(OBJECT_NAME_INFORMATION) + 512];
    POBJECT_NAME_INFORMATION NameInfo = (PVOID)Buffer;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE VolumeHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Acquire the flush lock since we're messing with the list */
    KeAcquireGuardedMutex(&PopVolumeLock);

    /* Loop the flush list */
    while (!IsListEmpty(&FlushContext->List))
    {
        /* Grab the next (ie: current) entry and remove it */
        NextEntry = FlushContext->List.Flink;
        RemoveEntryList(NextEntry);

        /* Add it back on the volume list */
        InsertTailList(&PopVolumeDevices, NextEntry);

        /* Done touching the volume list */
        KeReleaseGuardedMutex(&PopVolumeLock);

        /* Get the dope from the volume link */
        Dope = CONTAINING_RECORD(NextEntry, DEVICE_OBJECT_POWER_EXTENSION, Volume);

        /* Get the name */
        Status = ObQueryNameString(Dope->DeviceObject,
                                   NameInfo,
                                   sizeof(Buffer),
                                   &Length);
        if ((NT_SUCCESS(Status)) && (NameInfo->Name.Buffer))
        {
            /* Open the volume */
            DPRINT("Opening: %wZ\n", &NameInfo->Name);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &NameInfo->Name,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       0,
                                       0);
            Status = ZwCreateFile(&VolumeHandle,
                                  SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                  &ObjectAttributes,
                                  &IoStatusBlock,
                                  NULL,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  FILE_OPEN,
                                  0,
                                  NULL,
                                  0);
            if (NT_SUCCESS(Status))
            {
                /* Flush it and close it */
                DPRINT("Sending flush to: %p\n", VolumeHandle);
                ZwFlushBuffersFile(VolumeHandle, &IoStatusBlock);
                ZwClose(VolumeHandle);
            }
        }

        /* Acquire the flush lock again since we'll touch the list */
        KeAcquireGuardedMutex(&PopVolumeLock);
    }

    /* One more flush completed... if it was the last, signal the caller */
    if (!--FlushContext->Count) KeSetEvent(&FlushContext->Wait, IO_NO_INCREMENT, FALSE);

    /* Serialize with flushers */
    KeReleaseGuardedMutex(&PopVolumeLock);
}

VOID
NTAPI
PopFlushVolumes(IN BOOLEAN ShuttingDown)
{
    POP_FLUSH_VOLUME FlushContext = {{0}};
    ULONG FlushPolicy;
    UNICODE_STRING RegistryName = RTL_CONSTANT_STRING(L"\\Registry");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RegistryHandle;
    PLIST_ENTRY NextEntry;
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    ULONG VolumeCount = 0;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG ThreadCount;

    /* Setup the flush context */
    InitializeListHead(&FlushContext.List);
    KeInitializeEvent(&FlushContext.Wait, NotificationEvent, FALSE);

    /* What to flush */
    FlushPolicy = ShuttingDown ? 1 | 2 : PopFlushPolicy;
    if ((FlushPolicy & 1))
    {
        /* Registry flush requested, so open it */
        DPRINT("Opening registry\n");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegistryName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&RegistryHandle, KEY_READ, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Flush the registry */
            DPRINT("Flushing registry\n");
            ZwFlushKey(RegistryHandle);
            ZwClose(RegistryHandle);
        }
    }

    /* Serialize with other flushes */
    KeAcquireGuardedMutex(&PopVolumeLock);

    /* Scan the volume list */
    NextEntry = PopVolumeDevices.Flink;
    while (NextEntry != &PopVolumeDevices)
    {
        /* Get the dope from the link */
        Dope = CONTAINING_RECORD(NextEntry, DEVICE_OBJECT_POWER_EXTENSION, Volume);

        /* Grab the next entry now, since we'll be modifying the list */
        NextEntry = NextEntry->Flink;

        /* Make sure the object is mounted, writable, exists, and is not a floppy */
        if (!(Dope->DeviceObject->Vpb->Flags & VPB_MOUNTED) ||
            (Dope->DeviceObject->Characteristics & FILE_FLOPPY_DISKETTE) ||
            (Dope->DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE) ||
            ((Dope->DeviceObject->Vpb->RealDevice) &&
             (Dope->DeviceObject->Vpb->RealDevice->Characteristics & FILE_FLOPPY_DISKETTE)))
        {
            /* Not flushable */
            continue;
        }

        /* Remove it from the dope and add it to the flush context list */
        RemoveEntryList(&Dope->Volume);
        InsertTailList(&FlushContext.List, &Dope->Volume);

        /* Next */
        VolumeCount++;
    }

    /* Check if we should skip non-removable devices */
    if (!(FlushPolicy & 2))
    {
        /* ReactOS only implements this routine for shutdown, which requires it */
        UNIMPLEMENTED;
    }

    /* Check if there were no volumes at all */
    if (!VolumeCount)
    {
        /* Nothing to do */
        KeReleaseGuardedMutex(&PopVolumeLock);
        return;
    }

    /* Allocate up to 8 flusher threads */
    ThreadCount = min(VolumeCount, 8);
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* We will ourselves become a flusher thread */
    FlushContext.Count = 1;
    ThreadCount--;

    /* Look for any extra ones we might need */
    while (ThreadCount > 0)
    {
        /* Create a new one */
        ThreadCount--;
        DPRINT("Creating flush thread\n");
        Status = PsCreateSystemThread(&ThreadHandle,
                                      THREAD_ALL_ACCESS,
                                      &ObjectAttributes,
                                      0L,
                                      NULL,
                                      PopFlushVolumeWorker,
                                      &FlushContext);
        if (NT_SUCCESS(Status))
        {
            /* One more created... */
            FlushContext.Count++;
            ZwClose(ThreadHandle);
        }
    }

    /* Allow flushes to go through */
    KeReleaseGuardedMutex(&PopVolumeLock);

    /* Enter the flush work */
    DPRINT("Local flush\n");
    PopFlushVolumeWorker(&FlushContext);

    /* Wait for all flushes to be over */
    DPRINT("Waiting for flushes\n");
    KeWaitForSingleObject(&FlushContext.Wait, Executive, KernelMode, FALSE, NULL);
    DPRINT("Flushes have completed\n");
}

VOID
NTAPI
PoInitializeDeviceObject(IN OUT PDEVOBJ_EXTENSION DeviceObjectExtension)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension = (PVOID)DeviceObjectExtension;
    PAGED_CODE();

    /* Initialize the power flags */
    DeviceExtension->PowerFlags = PowerSystemUnspecified & 0xF;
    DeviceExtension->PowerFlags |= ((PowerDeviceUnspecified << 4) & 0xF0);

    /* The device object is not on drugs yet */
    DeviceExtension->Dope = NULL;
}

/* PUBLIC FUNCTIONS **********************************************************/

