/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager volumes and Device Object Power Extension (DOPE) support routines
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 *              Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KGUARDED_MUTEX PopVolumeLock;
KSPIN_LOCK PopDopeGlobalLock;
LIST_ENTRY PopVolumeDevices;
ULONG PopVolumeFlushPolicy = 0;
PDEVICE_NODE PopSystemPowerDeviceNode = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopMasterFlushVolume(
    _In_ PVOID Context)
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

    /* Do not allow anybody to add power volumes (other than us) while we do a flush operation */
    PopAcquireVolumeLock();

    /* Iterate over the flush list so that we can flush volume entries */
    while (!IsListEmpty(&FlushContext->List))
    {
        /* Grab the entry from the flush context and add it */
        NextEntry = FlushContext->List.Flink;
        RemoveEntryList(NextEntry);
        InsertTailList(&PopVolumeDevices, NextEntry);

        /* We are done touching the volume list */
        PopReleaseVolumeLock();

        /* Grab the DOPE from the volume link and volume name for flushing */
        Dope = CONTAINING_RECORD(NextEntry, DEVICE_OBJECT_POWER_EXTENSION, Volume);
        Status = ObQueryNameString(Dope->DeviceObject,
                                   NameInfo,
                                   sizeof(Buffer),
                                   &Length);
        if ((NT_SUCCESS(Status)) && (NameInfo->Name.Buffer))
        {
            /*
             * We queried the volume name, this is required to open a
             * file handle of the said volume so that we can flush its data.
             */
            DPRINT("Opening volume: %wZ\n", &NameInfo->Name);
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
                /* Flush it finally */
                DPRINT("Sending flush to: %p\n", VolumeHandle);
                ZwFlushBuffersFile(VolumeHandle, &IoStatusBlock);
                ZwClose(VolumeHandle);
            }
        }

        /* We are going to touch the volume list so protect it */
        PopAcquireVolumeLock();
    }

    /* If this was the last flush then signal such event to the caller */
    if (!--FlushContext->Count)
    {
        KeSetEvent(&FlushContext->Wait, IO_NO_INCREMENT, FALSE);
    }

    /* Give everybody access to the volume list now */
    PopReleaseVolumeLock();
}

/* PUBLIC FUNCTIONS ***********************************************************/

PDEVICE_OBJECT_POWER_EXTENSION
NTAPI
PopGetDope(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    KIRQL OldIrql;

    PAGED_CODE();

    /* This device already has a DOPE, just return that instead */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    if (DeviceExtension->Dope)
    {
        return (PDEVICE_OBJECT_POWER_EXTENSION)DeviceExtension->Dope;
    }

    /* Allocate memory for the DOPE */
    Dope = PopAllocatePool(sizeof(DEVICE_OBJECT_POWER_EXTENSION), FALSE, TAG_PO_DOPE);
    if (Dope == NULL)
    {
        /* Bail out */
        return NULL;
    }

    /* Initialize the necessary data to this DOPE */
    Dope->DeviceObject = DeviceObject;
    Dope->IdleState = PowerDeviceUnspecified;
    Dope->CurrentState = PowerDeviceUnspecified;
    InitializeListHead(&Dope->IdleList);

    /* Initialize the busy counters */
    Dope->BusyCount = 0;
    Dope->BusyReference = 0;
    Dope->TotalBusyCount = 0;

    /*
     * Acquire the DOPE lock so that we can safely assign this DOPE
     * to the device. Note that if somebody else already went fast
     * enough to assign it before us, then we have discard whatever
     * we allocated.
     */
    PopAcquireDopeLock(&OldIrql);
    if (!DeviceExtension->Dope)
    {
        /* OK, we are good to assign our DOPE to this device */
        DeviceExtension->Dope = (PVOID)Dope;
        Dope = NULL;
    }

    /* Someone assigned a DOPE before us, trash our DOPE */
    PopReleaseDopeLock(OldIrql);
    if (Dope)
    {
        PopFreePool(Dope, TAG_PO_DOPE);
    }

    return (PDEVICE_OBJECT_POWER_EXTENSION)DeviceExtension->Dope;
}

VOID
NTAPI
PopRemoveVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    KIRQL OldIrql;

    PAGED_CODE();

    /* This device never had a DOPE, consider our job as done */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    if (!DeviceExtension->Dope)
    {
        return;
    }

    /* Remove the volume while guarded with lock guarded */
    PopAcquireVolumeLock();
    Dope = (PDEVICE_OBJECT_POWER_EXTENSION)DeviceExtension->Dope;
    if (Dope->Volume.Flink)
    {
        RemoveEntryList(&Dope->Volume);
    }

    /* Release the lock */
    PopReleaseVolumeLock();

    /* Now tear the DOPE from this DO apart */
    PopAcquireDopeLock(&OldIrql);
    DeviceExtension->Dope = NULL;
    PopReleaseDopeLock(OldIrql);

    /* And free it from memory */
    PopFreePool(Dope, TAG_PO_DOPE);
}

VOID
NTAPI
PopFlushVolumes(
    _In_ BOOLEAN ShuttingDown)
{
    ULONG FlushPolicy;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RegistryHandle;
    PLIST_ENTRY NextEntry;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG ThreadCount;
    ULONG VolumeCount = 0;
    POP_FLUSH_VOLUME FlushContext = {{0}};
    static UNICODE_STRING RegistryName = RTL_CONSTANT_STRING(L"\\Registry");

    /* Prepare the flush context */
    InitializeListHead(&FlushContext.List);
    KeInitializeEvent(&FlushContext.Wait, NotificationEvent, FALSE);

    /*
     * Determine what should we flush exactly. During a shutdown power
     * action we flush the registry and every volume that is writable.
     * Note that if the system was not shutting down but is put to sleep
     * or goes to hibernation path, only flush what the flush policy allows.
     */
    FlushPolicy = ShuttingDown ? POP_FLUSH_REGISTRY | POP_FLUSH_NON_REM_DEVICES : PopVolumeFlushPolicy;

    /* A flush of the registry is requested, do the deed */
    if ((FlushPolicy & POP_FLUSH_REGISTRY))
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegistryName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&RegistryHandle, KEY_READ, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            ZwFlushKey(RegistryHandle);
            ZwClose(RegistryHandle);
        }
    }

    /* Block any incoming volumes as we do a flush */
    PopAcquireVolumeLock();

    /* Iterate over the volume list */
    NextEntry = PopVolumeDevices.Flink;
    while (NextEntry != &PopVolumeDevices)
    {
        /* Grab the DOPE from the volume link */
        Dope = CONTAINING_RECORD(NextEntry, DEVICE_OBJECT_POWER_EXTENSION, Volume);

        /* Grab the next volume entry */
        NextEntry = NextEntry->Flink;

        /*
         * The device object owning this volume has to meet the following
         * criteria for a successful flush:
         *
         * - The volume of the DO is mounted;
         * - The DO points to a real device;
         * - The DO currently exists and is not a floppy device;
         * - The volume is writable.
         */
        if (!(Dope->DeviceObject->Vpb->Flags & VPB_MOUNTED) ||
            (Dope->DeviceObject->Characteristics & FILE_FLOPPY_DISKETTE) ||
            (Dope->DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE) ||
            ((Dope->DeviceObject->Vpb->RealDevice) &&
             (Dope->DeviceObject->Vpb->RealDevice->Characteristics & FILE_FLOPPY_DISKETTE)))
        {
            /*
             * At least one of the conditions above is not met,
             * skip this volume.
             */
            continue;
        }

        /*
         * The caller requested to skip non-removable devices.
         * We will simply ignore volume flushes for these devices.
         */
        if (!(FlushPolicy & POP_FLUSH_NON_REM_DEVICES))
        {
            if (!(Dope->DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA))
            {
                continue;
            }
        }

        /* Insert the volume to the flust context list and count it too */
        RemoveEntryList(&Dope->Volume);
        InsertTailList(&FlushContext.List, &Dope->Volume);
        VolumeCount++;
    }

    /* No volumes have been found, just quit */
    if (!VolumeCount)
    {
        PopReleaseVolumeLock();
        return;
    }

    /* Allocate up to 8 flusher threads */
    ThreadCount = min(VolumeCount, 8);
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /*
     * We will become a flusher thread ourselves so setup the
     * thread count context now.
     */
    FlushContext.Count = 1;
    ThreadCount--;

    /* We still have other volumes to process so create threads for them */
    while (ThreadCount > 0)
    {
        ThreadCount--;
        Status = PsCreateSystemThread(&ThreadHandle,
                                      THREAD_ALL_ACCESS,
                                      &ObjectAttributes,
                                      0L,
                                      NULL,
                                      PopMasterFlushVolume,
                                      &FlushContext);
        if (NT_SUCCESS(Status))
        {
            FlushContext.Count++;
            ZwClose(ThreadHandle);
        }
    }

    /* Allow flushes to go through */
    PopReleaseVolumeLock();

    /* Jump into the flusher and wait for other flushes to be over */
    PopMasterFlushVolume(&FlushContext);
    KeWaitForSingleObject(&FlushContext.Wait, Executive, KernelMode, FALSE, NULL);
}

VOID
NTAPI
PoInitializeDeviceObject(
    _Inout_ PDEVOBJ_EXTENSION DeviceObjectExtension)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension = (PVOID)DeviceObjectExtension;

    PAGED_CODE();

    /* Initialize the power flags and DOPE */
    DeviceExtension->PowerFlags = PowerSystemUnspecified & POP_DOE_SYSTEM_POWER_FLAG_BIT;
    DeviceExtension->PowerFlags |= ((PowerDeviceUnspecified << 4) & POP_DOE_DEVICE_POWER_FLAG_BIT);
    DeviceExtension->Dope = NULL;
}

VOID
NTAPI
PoVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    PAGED_CODE();

    /*
     * Retrieve the DOPE from this device. It will allocate a newly
     * fresh DOPE if the device never had one.
     */
    Dope = PopGetDope(DeviceObject);
    if (Dope)
    {
        /* Insert this volume now */
        PopAcquireVolumeLock();
        if (!Dope->Volume.Flink)
        {
            InsertTailList(&PopVolumeDevices, &Dope->Volume);
        }

        PopReleaseVolumeLock();
    }
}

/* EOF */
