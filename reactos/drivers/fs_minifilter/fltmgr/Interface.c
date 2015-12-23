/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/fs_minifilter/fltmgr/interface.c
* PURPOSE:         Implements the driver interface
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"

//#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

static
NTSTATUS
SetupDispatchAndCallbacksTables(
    _In_ PDRIVER_OBJECT DriverObject
);

static
NTSTATUS
FltpAttachDeviceObject(
    _In_ PDEVICE_OBJECT SourceDevice,
    _In_ PDEVICE_OBJECT Targetevice,
    _Out_ PDEVICE_OBJECT *AttachedToDeviceObject
);

static
BOOLEAN
FltpIsAttachedToDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PDEVICE_OBJECT *AttachedDeviceObject
);

static
NTSTATUS
FltpEnumerateFileSystemVolumes(
    _In_ PDEVICE_OBJECT DeviceObject
);

static
NTSTATUS
FltpAttachToFileSystemDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PUNICODE_STRING DeviceName
);

static
LONG_PTR
FltpDetachFromFileSystemDevice(
    _In_ PDEVICE_OBJECT DeviceObject
);

DRIVER_FS_NOTIFICATION FltpFsNotification;
VOID
FltpFsNotification(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN FsActive
);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, SetupDispatchAndCallbacksTables)
#pragma alloc_text(PAGE, FltpAttachDeviceObject)
#pragma alloc_text(PAGE, FltpIsAttachedToDevice)
#pragma alloc_text(PAGE, FltpEnumerateFileSystemVolumes)
#pragma alloc_text(PAGE, FltpAttachToFileSystemDevice)
#pragma alloc_text(PAGE, FltpDetachFromFileSystemDevice)
#pragma alloc_text(PAGE, FltpFsNotification)
//#pragma alloc_text(PAGE, )
#endif

#define MAX_DEVNAME_LENGTH  64

DRIVER_DATA DriverData;

typedef struct _FLTMGR_DEVICE_EXTENSION
{
    /* The file system we're attached to */
    PDEVICE_OBJECT AttachedToDeviceObject;

    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    /* The storage stack(disk) accociated with the file system device object we're attached to */
    PDEVICE_OBJECT StorageStackDeviceObject;

    /* Either physical drive for volume device objects otherwise
     * it's the name of the control device we're attached to */
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];

} FLTMGR_DEVICE_EXTENSION, *PFLTMGR_DEVICE_EXTENSION;




/* DISPATCH ROUTINES **********************************************/

NTSTATUS
NTAPI
FltpPreFsFilterOperation(_In_ PFS_FILTER_CALLBACK_DATA Data,
                         _Out_ PVOID *CompletionContext)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(CompletionContext);
    __debugbreak();
    return STATUS_SUCCESS;
}

VOID
NTAPI
FltpPostFsFilterOperation(_In_ PFS_FILTER_CALLBACK_DATA Data,
                          _In_ NTSTATUS OperationStatus,
                          _In_ PVOID CompletionContext)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(OperationStatus);
    UNREFERENCED_PARAMETER(CompletionContext);
    __debugbreak();
}

NTSTATUS
NTAPI
FltpDispatch(_In_ PDEVICE_OBJECT DeviceObject,
             _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    __debugbreak();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FltpCreate(_In_ PDEVICE_OBJECT DeviceObject,
           _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    __debugbreak();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FltpFsControl(_In_ PDEVICE_OBJECT DeviceObject,
              _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    __debugbreak();
    return STATUS_SUCCESS;
}



/* FUNCTIONS **********************************************/

VOID
FltpCleanupDeviceObject(_In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension)
    {
        // cleanup device extension
    }
}

static
NTSTATUS
FltpAttachDeviceObject(_In_ PDEVICE_OBJECT SourceDevice,
                       _In_ PDEVICE_OBJECT TargetDevice,
                       _Out_ PDEVICE_OBJECT *AttachedToDeviceObject)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* Before attaching, copy the flags from the device we're going to attach to */
    if (FlagOn(SourceDevice->Flags, DO_BUFFERED_IO))
    {
        SetFlag(TargetDevice->Flags, DO_BUFFERED_IO);
    }
    if (FlagOn(SourceDevice->Flags, DO_DIRECT_IO))
    {
        SetFlag(TargetDevice->Flags, DO_DIRECT_IO);
    }
    if (FlagOn(SourceDevice->Flags, DO_SYSTEM_BOOT_PARTITION))
    {
        SetFlag(TargetDevice->Characteristics, FILE_DEVICE_SECURE_OPEN);
    }

    /* Attach this device to the top of the driver stack */
    Status = IoAttachDeviceToDeviceStackSafe(SourceDevice,
                                             TargetDevice,
                                             AttachedToDeviceObject);

    return Status;
}

static
BOOLEAN
FltpIsAttachedToDevice(_In_ PDEVICE_OBJECT DeviceObject,
                       _In_opt_ PDEVICE_OBJECT *AttachedDeviceObject)
{
    PDEVICE_OBJECT CurrentDeviceObject;
    PDEVICE_OBJECT NextDeviceObject;

    PAGED_CODE();

    /* Initialize the return pointer */
    if (AttachedDeviceObject) *AttachedDeviceObject = NULL;

    /* Start by getting the top level device in the chain */
    CurrentDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    /* Loop while there are attached devices */
    while (CurrentDeviceObject)
    {
        /* Check if this device driver matches ours */
        if (CurrentDeviceObject->DriverObject == DriverData.DriverObject)
        {
            FLT_ASSERT(CurrentDeviceObject->DeviceExtension != NULL);

            /* We're attached, return the device object if the caller asked for it */
            if (AttachedDeviceObject)
            {
                *AttachedDeviceObject = CurrentDeviceObject;
            }
            else
            {
                /* We aren't returning the reference, so decrement the count */
                ObDereferenceObject(CurrentDeviceObject);
            }

            return TRUE;
        }

        /* Get the next device in the chain */
        NextDeviceObject = IoGetLowerDeviceObject(CurrentDeviceObject);

        /* Decrement the count on the last device before we update the pointer */
        ObDereferenceObject(CurrentDeviceObject);
        CurrentDeviceObject = NextDeviceObject;
    } 

    return FALSE;
}

static
NTSTATUS
FltpEnumerateFileSystemVolumes(_In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION NewDeviceExtension;
    PDEVICE_OBJECT BaseDeviceObject;
    PDEVICE_OBJECT NewDeviceObject;
    PDEVICE_OBJECT *DeviceList;
    PDEVICE_OBJECT StorageStackDeviceObject;
    UNICODE_STRING DeviceName;
    ULONG NumDevices;
    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the base device */
    BaseDeviceObject = IoGetDeviceAttachmentBaseRef(DeviceObject);

    /* get the number of device object linked to the base file system */
    Status = IoEnumerateDeviceObjectList(BaseDeviceObject->DriverObject,
                                         NULL,
                                         0,
                                         &NumDevices);
    if (Status != STATUS_BUFFER_TOO_SMALL) return Status;

    /* Add a few more slots in case the size changed between calls and allocate some memory to hold the pointers */
    NumDevices += 4;
    DeviceList = ExAllocatePoolWithTag(NonPagedPool,
                                       (NumDevices * sizeof(PDEVICE_OBJECT)),
                                       FM_TAG_DEV_OBJ_PTRS);
    if (DeviceList == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Now get all the device objects that this base driver has created */
    Status = IoEnumerateDeviceObjectList(BaseDeviceObject->DriverObject,
                                         DeviceList,
                                         (NumDevices * sizeof(PDEVICE_OBJECT)),
                                         &NumDevices);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(DeviceList, FM_TAG_DEV_OBJ_PTRS);
        return Status;
    }

    /* Loop through all the devices looking for ones to attach to */
    for (i = 0; i < NumDevices; i++)
    {
        RtlInitUnicodeString(&DeviceName, NULL);
        StorageStackDeviceObject = NULL;
        NewDeviceObject = NULL;

        /* Ignore the device we passed in, and devices of the wrong type */
        if ((DeviceList[i] == BaseDeviceObject) ||
            (DeviceList[i]->DeviceType != BaseDeviceObject->DeviceType))
        {
            goto CleanupAndNext;
        }

        /* Ignore this device if we're already attached to it */
        if (FltpIsAttachedToDevice(DeviceList[i], NULL) == FALSE)
        {
            goto CleanupAndNext;
        }


        /*
         * If the device has a name, it must be a control device.
         * This handles drivers with more then one control device (like FastFat)
         */
        FltpGetBaseDeviceObjectName(DeviceList[i], &DeviceName);
        if (NT_SUCCESS(Status) && DeviceName.Length > 0)
        {
            goto CleanupAndNext;
        }

        /*
         * Try to get the storage stack (disk) device object accociated with
         * this file system device object. Ignore the device if we don't have one
         */
        Status = IoGetDiskDeviceObject(DeviceList[i],
                                       &StorageStackDeviceObject);
        if (!NT_SUCCESS(Status))
        {
            goto CleanupAndNext;
        }


        /*
         * TODO: Don't attach to shadow copy volumes,
         * ros doesn't have any so it's not an issues yet
         */

        /*
         * We're far enough to be ready to attach, create a device
         * object which we'll use to do so
         */
        Status = IoCreateDevice(DriverData.DriverObject,
                                sizeof(FLTMGR_DEVICE_EXTENSION),
                                NULL,
                                DeviceList[i]->DeviceType,
                                0,
                                FALSE,
                                &NewDeviceObject);
        if (!NT_SUCCESS(Status))
        {
            goto CleanupAndNext;
        }

        /* Get the device extension for this new object and store our disk object there */
        NewDeviceExtension = NewDeviceObject->DeviceExtension;
        NewDeviceExtension->StorageStackDeviceObject = StorageStackDeviceObject;

        /* Lookup and store the device name for the storage stack */
        RtlInitEmptyUnicodeString(&NewDeviceExtension->DeviceName,
                                  NewDeviceExtension->DeviceNameBuffer,
                                  sizeof(NewDeviceExtension->DeviceNameBuffer));
        FltpGetObjectName(StorageStackDeviceObject,
                          &NewDeviceExtension->DeviceName);


        /* Grab the attach lock before we attempt to attach */
        ExAcquireFastMutex(&DriverData.FilterAttachLock);

        /* Check again that we aren't already attached. It may have changed since our last check */
        if (FltpIsAttachedToDevice(DeviceList[i], NULL) == FALSE)
        {
            FLT_ASSERT(NewDeviceObject->DriverObject == DriverData.DriverObject);

            /* Finally, attach to the volume */
            Status = FltpAttachDeviceObject(DeviceList[i],
                                            NewDeviceObject,
                                            &NewDeviceExtension->AttachedToDeviceObject);
            if (NT_SUCCESS(Status))
            {
                /* Clean the initializing flag so other filters can attach to our device object */
                ClearFlag(NewDeviceObject->Flags, DO_DEVICE_INITIALIZING);
            }
        }
        else
        {
            /* We're already attached. Just cleanup */
            Status = STATUS_DEVICE_ALREADY_ATTACHED;
        }

        ExReleaseFastMutex(&DriverData.FilterAttachLock);

CleanupAndNext:

        if (!NT_SUCCESS(Status))
        {
            if (NewDeviceObject)
            {
                FltpCleanupDeviceObject(NewDeviceObject);
                IoDeleteDevice(NewDeviceObject);
            }
        }

        if (StorageStackDeviceObject)
        {
            /* A ref was added for us when we attached, so we can deref ours now */
            ObDereferenceObject(StorageStackDeviceObject);
        }

        /* Remove the ref which was added by IoEnumerateDeviceObjectList */
        ObDereferenceObject(DeviceList[i]);

        /* Free the buffer that FltpGetBaseDeviceObjectName added */
        FltpFreeUnicodeString(&DeviceName);

    }

    /* Free the memory we allocated for the list */
    ExFreePoolWithTag(DeviceList, FM_TAG_DEV_OBJ_PTRS);

    return STATUS_SUCCESS;
}

static
NTSTATUS
FltpAttachToFileSystemDevice(_In_ PDEVICE_OBJECT DeviceObject,
                             _In_ PUNICODE_STRING DeviceName)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT NewDeviceObject;
    WCHAR Buffer[MAX_DEVNAME_LENGTH];
    UNICODE_STRING FileSystemDeviceName;
    UNICODE_STRING FsRecDeviceName;
    NTSTATUS Status;

    PAGED_CODE();

    /* Only handle device types we're interested in */
    if (DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM &&
        DeviceObject->DeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM &&
        DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM)
    {
        return STATUS_SUCCESS;
    }

    /* Setup the buffer to hold the device name */
    RtlInitEmptyUnicodeString(&FileSystemDeviceName,
                              Buffer,
                              MAX_DEVNAME_LENGTH * sizeof(WCHAR));

    /* Get the the name of the file system device */
    Status = FltpGetObjectName(DeviceObject->DriverObject, &FileSystemDeviceName);
    if (!NT_SUCCESS(Status)) return Status;

    DPRINT("Found device %wZ, checking if we need to attach...", &FileSystemDeviceName);

    /* Build up the name of the file system recognizer device */
    RtlInitUnicodeString(&FsRecDeviceName, L"\\FileSystem\\Fs_Rec");

    /* We don't attach to recognizer devices, so bail if this is one */
    if (RtlCompareUnicodeString(&FileSystemDeviceName, &FsRecDeviceName, TRUE) == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Create a device object which we can attach to this file system */
    Status = IoCreateDevice(DriverData.DriverObject,
                            sizeof(FLTMGR_DEVICE_EXTENSION),
                            NULL,
                            DeviceObject->DeviceType,
                            0,
                            FALSE,
                            &NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a DO for attatching to a FS : 0x%X", Status);
        return Status;
    }

    /* Cast the device extension to something we understand */
    DeviceExtension = NewDeviceObject->DeviceExtension;

    /* Attach this device to the top of the driver stack and store the DO we attached to in the DE */
    Status = FltpAttachDeviceObject(NewDeviceObject,
                                    DeviceObject,
                                    &DeviceExtension->AttachedToDeviceObject);
    if (NT_SUCCESS(Status))
    {
        DPRINT("Attached to %wZ", &FileSystemDeviceName);
    }
    else
    {
        DPRINT1("Failed to attach to the driver stack : 0x%X", Status);
        goto Cleanup;
    }

    /* Setup the unicode string buffer and copy the device name to the device extension */
    RtlInitEmptyUnicodeString(&DeviceExtension->DeviceName,
                              DeviceExtension->DeviceNameBuffer,
                              MAX_DEVNAME_LENGTH * sizeof(WCHAR));
    RtlCopyUnicodeString(&DeviceExtension->DeviceName, DeviceName);

    /* We're done, remove the initializing flag */
    ClearFlag(NewDeviceObject->Flags, DO_DEVICE_INITIALIZING);

    /* Look for existing mounted devices for this file system */
    Status = FltpEnumerateFileSystemVolumes(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to enumerate file system volumes for this file system : 0x%X", Status);
        IoDetachDevice(DeviceExtension->AttachedToDeviceObject);
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(NewDeviceObject);
    }

    return Status;
}

static
LONG_PTR
FltpDetachFromFileSystemDevice(_In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT AttachedDevice, LowestDevice;
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    LONG_PTR Count;

    PAGED_CODE();

    /* Get the attached device and increment the ref count on it */
    AttachedDevice = IoGetAttachedDeviceReference(DeviceObject);

    /* Loop through all attached devices until we reach the bottom (file system driver) */
    while (AttachedDevice->DriverObject != DriverData.DriverObject)
    {
        /* Get the attached device */
        LowestDevice = IoGetLowerDeviceObject(AttachedDevice);

        /* Remove the reference we added. If it's zero then we're already clean */
        Count = ObfDereferenceObject(AttachedDevice);
        if (Count == 0) return Count;

        /* Try the next one */
        AttachedDevice = LowestDevice;
    }


    DeviceExtension = AttachedDevice->DeviceExtension;
    if (DeviceExtension)
    {
        //
        // FIXME: Put any device extension cleanup code here
        //
    }

    /* Detach the device from the chain and delete the object */
    IoDetachDevice(DeviceObject);
    IoDeleteDevice(AttachedDevice);

    /* Remove the reference we added so the delete can complete */
    return ObfDereferenceObject(AttachedDevice);
}

DRIVER_FS_NOTIFICATION FltpFsNotification;
VOID
FltpFsNotification(_In_ PDEVICE_OBJECT DeviceObject,
                   _In_ BOOLEAN FsActive)
{
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    __debugbreak();
    PAGED_CODE();

    /* Set an empty string */
    RtlInitUnicodeString(&DeviceName, NULL);

    /* Get the name of the lowest device object on the stack */ 
    Status = FltpGetBaseDeviceObjectName(DeviceObject, &DeviceName);
    if (NT_SUCCESS(Status))
    {
        /* Check if it's attaching or detaching*/
        if (FsActive)
        {
            /* Run the attach routine */
            FltpAttachToFileSystemDevice(DeviceObject, &DeviceName);
        }
        else
        {
            /* Run the detatch routine */
            FltpDetachFromFileSystemDevice(DeviceObject);
        }

        /* Free the buffer which FltpGetBaseDeviceObjectName allocated */
        FltpFreeUnicodeString(&DeviceName);
    }
}

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
NTAPI
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\FileSystem\\Filters\\"DRIVER_NAME);
    PDEVICE_OBJECT RawDeviceObject;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT RawFileObject;
    UNICODE_STRING ObjectName;
    UNICODE_STRING SymLink;

    NTSTATUS Status;
    __debugbreak();
    RtlZeroMemory(&DriverData, sizeof(DRIVER_DATA));
    DriverData.DriverObject = DriverObject;

    /* Save the registry key for this driver */
    DriverData.ServiceKey.Length = RegistryPath->Length;
    DriverData.ServiceKey.MaximumLength = RegistryPath->MaximumLength;
    DriverData.ServiceKey.Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool,
                                                                 RegistryPath->MaximumLength,
                                                                 FM_TAG_REGISTRY_DATA);
    if (!DriverData.ServiceKey.Buffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyUnicodeString(&DriverData.ServiceKey, RegistryPath);

    /* Do some initialization */
    ExInitializeFastMutex(&DriverData.FilterAttachLock);

    /* Create the main filter manager device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("fltmgr IoCreateDevice failed.  Status = %X", Status);
        goto Cleanup;
    }

    /* Store a global reference so we can access from callbacks */
    DriverData.DeviceObject = DeviceObject;

    /* Generate the symbolic link name */
    RtlInitUnicodeString(&SymLink, L"\\??\\"DRIVER_NAME);
    Status = IoCreateSymbolicLink(&SymLink, &DeviceName);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Create the callbacks for the dispatch table, FastIo and FS callbacks */
    Status = SetupDispatchAndCallbacksTables(DriverObject);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    //
    // TODO: Create fltmgr message device
    //

    /* Register for notifications when a new file system is loaded. This also enumerates any existing file systems */
    Status = IoRegisterFsRegistrationChange(DriverObject, FltpFsNotification);
    FLT_ASSERT(Status != STATUS_DEVICE_ALREADY_ATTACHED); // Windows checks for this, but I'm not sure how that can happen??
    if (!NT_SUCCESS(Status))  goto Cleanup;

    /* IoRegisterFsRegistrationChange isn't notified about the raw  file systems, so we attach to them manually */
    RtlInitUnicodeString(&ObjectName, L"\\Device\\RawDisk");
    Status = IoGetDeviceObjectPointer(&ObjectName,
                                      FILE_READ_ATTRIBUTES,
                                      &RawFileObject,
                                      &RawDeviceObject);
    if (NT_SUCCESS(Status))
    {
        FltpFsNotification(RawDeviceObject, TRUE);
        ObfDereferenceObject(RawFileObject);
    }

    RtlInitUnicodeString(&ObjectName, L"\\Device\\RawCdRom");
    Status = IoGetDeviceObjectPointer(&ObjectName,
                                      FILE_READ_ATTRIBUTES,
                                      &RawFileObject,
                                      &RawDeviceObject);
    if (NT_SUCCESS(Status))
    {
        FltpFsNotification(RawDeviceObject, TRUE);
        ObfDereferenceObject(RawFileObject);
    }

    /* We're done, clear the initializing flag */
    ClearFlag(DeviceObject->Flags, DO_DEVICE_INITIALIZING);
    Status = STATUS_SUCCESS;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        if (DriverData.FastIoDispatch)
        {
            DriverObject->FastIoDispatch = NULL;
            ExFreePoolWithTag(DriverData.FastIoDispatch, FM_TAG_DISPATCH_TABLE);
        }

        IoDeleteSymbolicLink(&SymLink);

        if (DeviceObject)
            IoDeleteDevice(DeviceObject);

        if (DriverData.ServiceKey.Buffer)
            ExFreePoolWithTag(DriverData.ServiceKey.Buffer, FM_TAG_REGISTRY_DATA);
    }

    return Status;
}


static
NTSTATUS
SetupDispatchAndCallbacksTables(_In_ PDRIVER_OBJECT DriverObject)
{
    PFAST_IO_DISPATCH FastIoDispatch;
    FS_FILTER_CALLBACKS Callbacks;
    ULONG i;

    /* Plug all the IRPs */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = FltpDispatch;
    }

    /* Override the ones we're interested in */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FltpCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = FltpCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = FltpCreate;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FltpFsControl;

    /* The FastIo diapatch table is stored in the pool along with a tag */
    FastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_IO_DISPATCH), FM_TAG_DISPATCH_TABLE);
    if (FastIoDispatch == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Fill out the FastIo table  */
    RtlZeroMemory(FastIoDispatch, sizeof(FAST_IO_DISPATCH));
    FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible = (PFAST_IO_CHECK_IF_POSSIBLE)NULL;
    FastIoDispatch->FastIoRead = (PFAST_IO_READ)NULL;
    FastIoDispatch->FastIoWrite = (PFAST_IO_WRITE)NULL;
    FastIoDispatch->FastIoQueryBasicInfo = (PFAST_IO_QUERY_BASIC_INFO)NULL;
    FastIoDispatch->FastIoQueryStandardInfo = (PFAST_IO_QUERY_STANDARD_INFO)NULL;
    FastIoDispatch->FastIoLock = (PFAST_IO_LOCK)NULL;
    FastIoDispatch->FastIoUnlockSingle = (PFAST_IO_UNLOCK_SINGLE)NULL;
    FastIoDispatch->FastIoUnlockAll = (PFAST_IO_UNLOCK_ALL)NULL;
    FastIoDispatch->FastIoUnlockAllByKey = (PFAST_IO_UNLOCK_ALL_BY_KEY)NULL;
    FastIoDispatch->FastIoDeviceControl = (PFAST_IO_DEVICE_CONTROL)NULL;
    FastIoDispatch->FastIoDetachDevice = (PFAST_IO_DETACH_DEVICE)NULL;
    FastIoDispatch->FastIoQueryNetworkOpenInfo = (PFAST_IO_QUERY_NETWORK_OPEN_INFO)NULL;
    FastIoDispatch->MdlRead = (PFAST_IO_MDL_READ)NULL;
    FastIoDispatch->MdlReadComplete = (PFAST_IO_MDL_READ_COMPLETE)NULL;
    FastIoDispatch->PrepareMdlWrite = (PFAST_IO_PREPARE_MDL_WRITE)NULL;
    FastIoDispatch->MdlWriteComplete = (PFAST_IO_MDL_WRITE_COMPLETE)NULL;
    FastIoDispatch->FastIoReadCompressed = (PFAST_IO_READ_COMPRESSED)NULL;
    FastIoDispatch->FastIoWriteCompressed = (PFAST_IO_WRITE_COMPRESSED)NULL;
    FastIoDispatch->MdlReadCompleteCompressed = (PFAST_IO_MDL_READ_COMPLETE_COMPRESSED)NULL;
    FastIoDispatch->MdlWriteCompleteCompressed = (PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED)NULL;
    FastIoDispatch->FastIoQueryOpen = (PFAST_IO_QUERY_OPEN)NULL;

    /* Store the FastIo table address for easy reference */
    DriverData.FastIoDispatch = FastIoDispatch;

    /* Initialize the callback table */
    Callbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
    Callbacks.PreAcquireForSectionSynchronization = FltpPreFsFilterOperation;
    Callbacks.PostAcquireForSectionSynchronization = FltpPostFsFilterOperation;
    Callbacks.PreReleaseForSectionSynchronization = FltpPreFsFilterOperation;
    Callbacks.PostReleaseForSectionSynchronization = FltpPostFsFilterOperation;
    Callbacks.PreAcquireForCcFlush = FltpPreFsFilterOperation;
    Callbacks.PostAcquireForCcFlush = FltpPostFsFilterOperation;
    Callbacks.PreReleaseForCcFlush = FltpPreFsFilterOperation;
    Callbacks.PostReleaseForCcFlush = FltpPostFsFilterOperation;
    Callbacks.PreAcquireForModifiedPageWriter = FltpPreFsFilterOperation;
    Callbacks.PostAcquireForModifiedPageWriter = FltpPostFsFilterOperation;
    Callbacks.PreReleaseForModifiedPageWriter = FltpPreFsFilterOperation;
    Callbacks.PostReleaseForModifiedPageWriter = FltpPostFsFilterOperation;

    /* Register our callbacks */
    return FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &Callbacks);
}
