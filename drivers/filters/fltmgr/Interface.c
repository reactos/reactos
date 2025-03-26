/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/interface.c
* PURPOSE:         Implements the driver interface
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

//#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
    (((_FastIoDispatchPtr) != NULL) && \
     (((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
            (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
     ((_FastIoDispatchPtr)->_FieldName != NULL))

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj) != NULL) && \
    ((_devObj)->DriverObject == Dispatcher::DriverObject) && \
      ((_devObj)->DeviceExtension != NULL))

extern PDEVICE_OBJECT CommsDeviceObject;
extern LIST_ENTRY FilterList;
extern ERESOURCE FilterListLock;

DRIVER_DATA DriverData;

typedef struct _DETACH_DEVICE_WORK_ITEM
{
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT SourceDevice;
    PDEVICE_OBJECT TargetDevice;

} DETACH_DEVICE_WORK_ITEM, *PDETACH_DEVICE_WORK_ITEM;

/* LOCAL FUNCTIONS ****************************************/

static
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

CODE_SEG("PAGE")
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

CODE_SEG("PAGE")
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

CODE_SEG("PAGE")
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
         * Try to get the storage stack (disk) device object associated with
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

CODE_SEG("PAGE")
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

    DPRINT("Found device %wZ, checking if we need to attach...\n", &FileSystemDeviceName);

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
        DPRINT1("Failed to create a DO for attaching to a FS : 0x%X\n", Status);
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
        DPRINT("Attached to %wZ\n", &FileSystemDeviceName);
    }
    else
    {
        DPRINT1("Failed to attach to the driver stack : 0x%X\n", Status);
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
        DPRINT1("Failed to enumerate file system volumes for this file system : 0x%X\n", Status);
        IoDetachDevice(DeviceExtension->AttachedToDeviceObject);
    }

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(NewDeviceObject);
    }

    return Status;
}

CODE_SEG("PAGE")
static
LONG_PTR
FltpDetachFromFileSystemDevice(_In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT AttachedDevice, NextDevice;
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    LONG_PTR Count;

    PAGED_CODE();

    /* Get the top device in the chain and increment the ref count on it */
    AttachedDevice = IoGetAttachedDeviceReference(DeviceObject);

    /* Loop all attached devices looking for our file system driver */
    while (AttachedDevice->DriverObject != DriverData.DriverObject)
    {
        FLT_ASSERT(AttachedDevice != NULL);

        /* Get the next lower device object. This adds a ref on NextDevice */
        NextDevice = IoGetLowerDeviceObject(AttachedDevice);

        /* Remove the reference we added */
        Count = ObDereferenceObject(AttachedDevice);

        /* Bail if this is the last one */
        if (NextDevice == NULL) return Count;

        /* Try the next one */
        AttachedDevice = NextDevice;
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
    return ObDereferenceObject(AttachedDevice);
}


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
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Check if this is a request for us */
    if (DeviceObject == DriverData.DeviceObject)
    {
        FLT_ASSERT(DeviceObject->DriverObject == DriverData.DriverObject);
        FLT_ASSERT(DeviceExtension == NULL);

        /* Hand it off to our internal handler */
        Status = FltpDispatchHandler(DeviceObject, Irp);
        if (Status != STATUS_REPARSE)
        {
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, 0);
        }
        return Status;
    }

    /* Check if this is a request for a the messaging device */
    if (DeviceObject == CommsDeviceObject)
    {
        /* Hand off to our internal routine */
        return FltpMsgDispatch(DeviceObject, Irp);
    }

    FLT_ASSERT(DeviceExtension &&
               DeviceExtension->AttachedToDeviceObject);

    StackPtr = IoGetCurrentIrpStackLocation(Irp);
    if (StackPtr->MajorFunction == IRP_MJ_SHUTDOWN)
    {
        // handle shutdown request
    }

    DPRINT1("Received %X from %wZ\n", StackPtr->MajorFunction, &DeviceExtension->DeviceName);

    /* Just pass the IRP down the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->AttachedToDeviceObject, Irp);
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
FltpCreate(_In_ PDEVICE_OBJECT DeviceObject,
           _Inout_ PIRP Irp)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Check if this is a request for us */
    if (DeviceObject == DriverData.DeviceObject)
    {
        FLT_ASSERT(DeviceObject->DriverObject == DriverData.DriverObject);
        FLT_ASSERT(DeviceExtension == NULL);

        /* Someone wants a handle to the fltmgr, allow it */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IofCompleteRequest(Irp, 0);
        return STATUS_SUCCESS;
    }

    /* Check if this is a request for a the new comms connection */
    if (DeviceObject == CommsDeviceObject)
    {
        /* Hand off to our internal routine */
        return FltpMsgCreate(DeviceObject, Irp);
    }

    FLT_ASSERT(DeviceExtension &&
               DeviceExtension->AttachedToDeviceObject);

    DPRINT1("Received create from %wZ (%lu)\n", &DeviceExtension->DeviceName, PsGetCurrentProcessId());

    /* Just pass the IRP down the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->AttachedToDeviceObject, Irp);
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
FltpFsControl(_In_ PDEVICE_OBJECT DeviceObject,
              _Inout_ PIRP Irp)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    /* Check if this is a request for us */
    if (DeviceObject == DriverData.DeviceObject)
    {
        /* We don't handle this request */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IofCompleteRequest(Irp, 0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    DeviceExtension = DeviceObject->DeviceExtension;

    FLT_ASSERT(DeviceExtension &&
               DeviceExtension->AttachedToDeviceObject);

    /* Just pass the IRP down the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->AttachedToDeviceObject, Irp);
}

NTSTATUS
NTAPI
FltpDeviceControl(_In_ PDEVICE_OBJECT DeviceObject,
                  _Inout_ PIRP Irp)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    /* Check if the request was meant for us */
    if (DeviceObject == DriverData.DeviceObject)
    {
        Status = FltpDeviceControlHandler(DeviceObject, Irp);
        if (Status != STATUS_REPARSE)
        {
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, 0);
        }

        return Status;
    }

    DeviceExtension = DeviceObject->DeviceExtension;

    FLT_ASSERT(DeviceExtension &&
               DeviceExtension->AttachedToDeviceObject);

    /* Just pass the IRP down the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->AttachedToDeviceObject, Irp);
}



/* FASTIO ROUTINES ************************************************/

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoCheckIfPossible(_In_ PFILE_OBJECT FileObject,
                          _In_ PLARGE_INTEGER FileOffset,
                          _In_ ULONG Length,
                          _In_ BOOLEAN Wait,
                          _In_ ULONG LockKey,
                          _In_ BOOLEAN CheckForReadOperation,
                          _Out_ PIO_STATUS_BLOCK IoStatus,
                          _In_ PDEVICE_OBJECT DeviceObject)

{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoCheckIfPossible)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                     FileOffset,
                                                     Length,
                                                     Wait,
                                                     LockKey,
                                                     CheckForReadOperation,
                                                     IoStatus,
                                                     AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoRead(_In_ PFILE_OBJECT FileObject,
               _In_ PLARGE_INTEGER FileOffset,
               _In_ ULONG Length,
               _In_ BOOLEAN Wait,
               _In_ ULONG LockKey,
               _Out_ PVOID Buffer,
               _Out_ PIO_STATUS_BLOCK IoStatus,
               _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoRead)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoRead(FileObject,
                                          FileOffset,
                                          Length,
                                          Wait,
                                          LockKey,
                                          Buffer,
                                          IoStatus,
                                          AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoWrite(_In_ PFILE_OBJECT FileObject,
                _In_ PLARGE_INTEGER FileOffset,
                 _In_ ULONG Length,
                 _In_ BOOLEAN Wait,
                 _In_ ULONG LockKey,
                 _In_ PVOID Buffer,
                 _Out_ PIO_STATUS_BLOCK IoStatus,
                 _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoWrite)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoWrite(FileObject,
                                           FileOffset,
                                           Length,
                                           Wait,
                                           LockKey,
                                           Buffer,
                                           IoStatus,
                                           AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoQueryBasicInfo(_In_ PFILE_OBJECT FileObject,
                         _In_ BOOLEAN Wait,
                         _Out_ PFILE_BASIC_INFORMATION Buffer,
                         _Out_ PIO_STATUS_BLOCK IoStatus,
                         _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoQueryBasicInfo)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoQueryBasicInfo(FileObject,
                                                    Wait,
                                                    Buffer,
                                                    IoStatus,
                                                    AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoQueryStandardInfo(_In_ PFILE_OBJECT FileObject,
                            _In_ BOOLEAN Wait,
                            _Out_ PFILE_STANDARD_INFORMATION Buffer,
                            _Out_ PIO_STATUS_BLOCK IoStatus,
                            _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoQueryStandardInfo)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoQueryStandardInfo(FileObject,
                                                       Wait,
                                                       Buffer,
                                                       IoStatus,
                                                       AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoLock(_In_ PFILE_OBJECT FileObject,
               _In_ PLARGE_INTEGER FileOffset,
               _In_ PLARGE_INTEGER Length,
               _In_ PEPROCESS ProcessId,
               _In_ ULONG Key,
               _In_ BOOLEAN FailImmediately,
               _In_ BOOLEAN ExclusiveLock,
               _Out_ PIO_STATUS_BLOCK IoStatus,
               _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoLock)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoLock(FileObject,
                                          FileOffset,
                                          Length,
                                          ProcessId,
                                          Key,
                                          FailImmediately,
                                          ExclusiveLock,
                                          IoStatus,
                                          AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoUnlockSingle(_In_ PFILE_OBJECT FileObject,
                       _In_ PLARGE_INTEGER FileOffset,
                       _In_ PLARGE_INTEGER Length,
                       _In_ PEPROCESS ProcessId,
                       _In_ ULONG Key,
                       _Out_ PIO_STATUS_BLOCK IoStatus,
                       _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoUnlockSingle)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoUnlockSingle(FileObject,
                                                  FileOffset,
                                                  Length,
                                                  ProcessId,
                                                  Key,
                                                  IoStatus,
                                                  AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoUnlockAll(_In_ PFILE_OBJECT FileObject,
                    _In_ PEPROCESS ProcessId,
                    _Out_ PIO_STATUS_BLOCK IoStatus,
                    _In_ PDEVICE_OBJECT DeviceObject)

{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoUnlockAll)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoUnlockAll(FileObject,
                                               ProcessId,
                                               IoStatus,
                                               AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoUnlockAllByKey(_In_ PFILE_OBJECT FileObject,
                         _In_ PVOID ProcessId,
                         _In_ ULONG Key,
                         _Out_ PIO_STATUS_BLOCK IoStatus,
                         _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoUnlockAllByKey)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoUnlockAllByKey(FileObject,
                                                    ProcessId,
                                                    Key,
                                                    IoStatus,
                                                    AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoDeviceControl(_In_ PFILE_OBJECT FileObject,
                        _In_ BOOLEAN Wait,
                        _In_opt_ PVOID InputBuffer,
                        _In_ ULONG InputBufferLength,
                        _Out_opt_ PVOID OutputBuffer,
                        _In_ ULONG OutputBufferLength,
                        _In_ ULONG IoControlCode,
                        _Out_ PIO_STATUS_BLOCK IoStatus,
                        _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the request, send it down the slow path */
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoDeviceControl)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoDeviceControl(FileObject,
                                                   Wait,
                                                   InputBuffer,
                                                   InputBufferLength,
                                                   OutputBuffer,
                                                   OutputBufferLength,
                                                   IoControlCode,
                                                   IoStatus,
                                                   AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

VOID
NTAPI
FltpFastIoDetachDeviceWorker(_In_ PVOID Parameter)
{
    PDETACH_DEVICE_WORK_ITEM DetachDeviceWorkItem = Parameter;

    /* Run any cleanup routines */
    FltpCleanupDeviceObject(DetachDeviceWorkItem->SourceDevice);

    /* Detach from the target device */
    IoDetachDevice(DetachDeviceWorkItem->TargetDevice);

    /* Delete the source */
    IoDeleteDevice(DetachDeviceWorkItem->SourceDevice);

    /* Free the pool we allocated in FltpFastIoDetachDevice */
    ExFreePoolWithTag(DetachDeviceWorkItem, 0x1234);
}

CODE_SEG("PAGE")
VOID
NTAPI
FltpFastIoDetachDevice(_In_ PDEVICE_OBJECT SourceDevice,
                     _In_ PDEVICE_OBJECT TargetDevice)
{
    PDETACH_DEVICE_WORK_ITEM DetachDeviceWorkItem;

    PAGED_CODE();

    /*
     * Detaching and deleting devices is a lot of work and takes too long
     * to be a worthwhile FastIo candidate, so we defer this call to speed
     * it up. There's no return value so we're okay to do this.
     */

    /* Allocate the work item and it's corresponding data */
    DetachDeviceWorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(DETACH_DEVICE_WORK_ITEM),
                                                 0x1234);
    if (DetachDeviceWorkItem)
    {
        /* Initialize the work item */
        ExInitializeWorkItem(&DetachDeviceWorkItem->WorkItem,
                             FltpFastIoDetachDeviceWorker,
                             DetachDeviceWorkItem);

        /* Queue the work item and return the call */
        ExQueueWorkItem(&DetachDeviceWorkItem->WorkItem,
                        DelayedWorkQueue);
    }
    else
    {
        /* We failed to defer, just cleanup here */
        FltpCleanupDeviceObject(SourceDevice);
        IoDetachDevice(TargetDevice);
        IoDeleteDevice(SourceDevice);
    }

}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoQueryNetworkOpenInfo(_In_ PFILE_OBJECT FileObject,
                               _In_ BOOLEAN Wait,
                               _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer,
                               _Out_ PIO_STATUS_BLOCK IoStatus,
                               _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoQueryNetworkOpenInfo)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoQueryNetworkOpenInfo(FileObject,
                                                          Wait,
                                                          Buffer,
                                                          IoStatus,
                                                          AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoMdlRead(_In_ PFILE_OBJECT FileObject,
                  _In_ PLARGE_INTEGER FileOffset,
                  _In_ ULONG Length,
                  _In_ ULONG LockKey,
                  _Out_ PMDL *MdlChain,
                  _Out_ PIO_STATUS_BLOCK IoStatus,
                  _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->MdlRead)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->MdlRead(FileObject,
                                       FileOffset,
                                       Length,
                                       LockKey,
                                       MdlChain,
                                       IoStatus,
                                       AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoMdlReadComplete(_In_ PFILE_OBJECT FileObject,
                          _In_ PMDL MdlChain,
                          _In_ PDEVICE_OBJECT DeviceObject)

{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the request, send it down the slow path */
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->MdlReadComplete)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->MdlReadComplete(FileObject,
                                               MdlChain,
                                               AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoPrepareMdlWrite(_In_ PFILE_OBJECT FileObject,
                          _In_ PLARGE_INTEGER FileOffset,
                          _In_ ULONG Length,
                          _In_ ULONG LockKey,
                          _Out_ PMDL *MdlChain,
                          _Out_ PIO_STATUS_BLOCK IoStatus,
                          _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the call */
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        IoStatus->Information = 0;
        return TRUE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->PrepareMdlWrite)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->PrepareMdlWrite(FileObject,
                                               FileOffset,
                                               Length,
                                               LockKey,
                                               MdlChain,
                                               IoStatus,
                                               AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoMdlWriteComplete(_In_ PFILE_OBJECT FileObject,
                           _In_ PLARGE_INTEGER FileOffset,
                           _In_ PMDL MdlChain,
                           _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the request, send it down the slow path */
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->MdlWriteComplete)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->MdlWriteComplete(FileObject,
                                               FileOffset,
                                               MdlChain,
                                               AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoReadCompressed(_In_ PFILE_OBJECT FileObject,
                         _In_ PLARGE_INTEGER FileOffset,
                         _In_ ULONG Length,
                         _In_ ULONG LockKey,
                         _Out_ PVOID Buffer,
                         _Out_ PMDL *MdlChain,
                         _Out_ PIO_STATUS_BLOCK IoStatus,
                         _Out_ PCOMPRESSED_DATA_INFO CompressedDataInfo,
                         _In_ ULONG CompressedDataInfoLength,
                         _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the request, send it down the slow path */
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoReadCompressed)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoReadCompressed(FileObject,
                                                    FileOffset,
                                                    Length,
                                                    LockKey,
                                                    Buffer,
                                                    MdlChain,
                                                    IoStatus,
                                                    CompressedDataInfo,
                                                    CompressedDataInfoLength,
                                                    AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoWriteCompressed(_In_ PFILE_OBJECT FileObject,
                          _In_ PLARGE_INTEGER FileOffset,
                          _In_ ULONG Length,
                          _In_ ULONG LockKey,
                          _In_ PVOID Buffer,
                          _Out_ PMDL *MdlChain,
                          _Out_ PIO_STATUS_BLOCK IoStatus,
                          _In_ PCOMPRESSED_DATA_INFO CompressedDataInfo,
                          _In_ ULONG CompressedDataInfoLength,
                          _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        /* Fail the request, send it down the slow path */
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoWriteCompressed)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->FastIoWriteCompressed(FileObject,
                                                     FileOffset,
                                                     Length,
                                                     LockKey,
                                                     Buffer,
                                                     MdlChain,
                                                     IoStatus,
                                                     CompressedDataInfo,
                                                     CompressedDataInfoLength,
                                                     AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoMdlReadCompleteCompressed(_In_ PFILE_OBJECT FileObject,
                                    _In_ PMDL MdlChain,
                                    _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->MdlReadCompleteCompressed)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->MdlReadCompleteCompressed(FileObject,
                                                         MdlChain,
                                                         AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoMdlWriteCompleteCompressed(_In_ PFILE_OBJECT FileObject,
                                     _In_ PLARGE_INTEGER FileOffset,
                                     _In_ PMDL MdlChain,
                                     _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->MdlWriteCompleteCompressed)
    {
        /* Forward the call onto the device we attached to */
        return FastIoDispatch->MdlWriteCompleteCompressed(FileObject,
                                                          FileOffset,
                                                          MdlChain,
                                                          AttachedDeviceObject);
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
FltpFastIoQueryOpen(_Inout_ PIRP Irp,
                    _Out_ PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
                    _In_ PDEVICE_OBJECT DeviceObject)
{
    PFLTMGR_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    BOOLEAN Success;

    PAGED_CODE();

    /* If it doesn't have a device extension, then it's not our device object */
    if (DeviceObject->DeviceExtension == NULL)
    {
        return FALSE;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    FLT_ASSERT(DeviceExtension->AttachedToDeviceObject);

    /* Get the device that we attached to */
    AttachedDeviceObject = DeviceExtension->AttachedToDeviceObject;
    FastIoDispatch = AttachedDeviceObject->DriverObject->FastIoDispatch;

    /* Make sure our FastIo table is valid */
    if (FastIoDispatch && FastIoDispatch->FastIoQueryOpen)
    {
        PIO_STACK_LOCATION StackPtr = IoGetCurrentIrpStackLocation(Irp);

        /* Update the stack to contain the correct device for the next filter */
        StackPtr->DeviceObject = AttachedDeviceObject;

        /* Now forward the call */
        Success = FastIoDispatch->FastIoQueryOpen(Irp,
                                                  NetworkInformation,
                                                  AttachedDeviceObject);

        /* Restore the DeviceObject as we found it */
        StackPtr->DeviceObject = DeviceObject;
        return Success;
    }

    /* We failed to handle the request, send it down the slow path */
    FLT_ASSERT(FALSE);
    return FALSE;
}

CODE_SEG("PAGE")
DRIVER_FS_NOTIFICATION FltpFsNotification;

CODE_SEG("PAGE")
VOID
NTAPI
FltpFsNotification(_In_ PDEVICE_OBJECT DeviceObject,
                   _In_ BOOLEAN FsActive)
{
    UNICODE_STRING DeviceName;
    NTSTATUS Status;

    PAGED_CODE();

    /* Set an empty string */
    RtlInitUnicodeString(&DeviceName, NULL);

    /* Get the name of the lowest device object on the stack */
    Status = FltpGetBaseDeviceObjectName(DeviceObject, &DeviceName);
    if (NT_SUCCESS(Status))
    {
        /* Check if it's attaching or detaching */
        if (FsActive)
        {
            /* Run the attach routine */
            FltpAttachToFileSystemDevice(DeviceObject, &DeviceName);
        }
        else
        {
            /* Run the detach routine */
            FltpDetachFromFileSystemDevice(DeviceObject);
        }

        /* Free the buffer which FltpGetBaseDeviceObjectName allocated */
        FltpFreeUnicodeString(&DeviceName);
    }
}

static
CODE_SEG("INIT")
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
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FltpDeviceControl;

    /* The FastIo dispatch table is stored in the pool along with a tag */
    FastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_IO_DISPATCH), FM_TAG_DISPATCH_TABLE);
    if (FastIoDispatch == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Fill out the FastIo table  */
    RtlZeroMemory(FastIoDispatch, sizeof(FAST_IO_DISPATCH));
    FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch->FastIoCheckIfPossible = FltpFastIoCheckIfPossible;
    FastIoDispatch->FastIoRead = FltpFastIoRead;
    FastIoDispatch->FastIoWrite = FltpFastIoWrite;
    FastIoDispatch->FastIoQueryBasicInfo = FltpFastIoQueryBasicInfo;
    FastIoDispatch->FastIoQueryStandardInfo = FltpFastIoQueryStandardInfo;
    FastIoDispatch->FastIoLock = FltpFastIoLock;
    FastIoDispatch->FastIoUnlockSingle = FltpFastIoUnlockSingle;
    FastIoDispatch->FastIoUnlockAll = FltpFastIoUnlockAll;
    FastIoDispatch->FastIoUnlockAllByKey = FltpFastIoUnlockAllByKey;
    FastIoDispatch->FastIoDeviceControl = FltpFastIoDeviceControl;
    FastIoDispatch->FastIoDetachDevice = FltpFastIoDetachDevice;
    FastIoDispatch->FastIoQueryNetworkOpenInfo = FltpFastIoQueryNetworkOpenInfo;
    FastIoDispatch->MdlRead = FltpFastIoMdlRead;
    FastIoDispatch->MdlReadComplete = FltpFastIoMdlReadComplete;
    FastIoDispatch->PrepareMdlWrite = FltpFastIoPrepareMdlWrite;
    FastIoDispatch->MdlWriteComplete = FltpFastIoMdlWriteComplete;
    FastIoDispatch->FastIoReadCompressed = FltpFastIoReadCompressed;
    FastIoDispatch->FastIoWriteCompressed = FltpFastIoWriteCompressed;
    FastIoDispatch->MdlReadCompleteCompressed = FltpFastIoMdlReadCompleteCompressed;
    FastIoDispatch->MdlWriteCompleteCompressed = FltpFastIoMdlWriteCompleteCompressed;
    FastIoDispatch->FastIoQueryOpen = FltpFastIoQueryOpen;

    /* Store the FastIo table for internal and our access */
    DriverObject->FastIoDispatch = FastIoDispatch;
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

CODE_SEG("INIT") DRIVER_INITIALIZE DriverEntry;

CODE_SEG("INIT")
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
        DPRINT1("fltmgr IoCreateDevice failed.  Status = %X\n", Status);
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

    /* Initialize the comms objects */
    Status = FltpSetupCommunicationObjects(DriverObject);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Register for notifications when a new file system is loaded. This also enumerates any existing file systems */
    Status = IoRegisterFsRegistrationChange(DriverObject, FltpFsNotification);
    FLT_ASSERT(Status != STATUS_DEVICE_ALREADY_ATTACHED); // Windows checks for this, I'm not sure how it can happen. Needs investigation??
    if (!NT_SUCCESS(Status))  goto Cleanup;

    InitializeListHead(&FilterList);
    ExInitializeResourceLite(&FilterListLock);

    /* IoRegisterFsRegistrationChange isn't notified about the raw  file systems, so we attach to them manually */
    RtlInitUnicodeString(&ObjectName, L"\\Device\\RawDisk");
    Status = IoGetDeviceObjectPointer(&ObjectName,
                                      FILE_READ_ATTRIBUTES,
                                      &RawFileObject,
                                      &RawDeviceObject);
    if (NT_SUCCESS(Status))
    {
        FltpFsNotification(RawDeviceObject, TRUE);
        ObDereferenceObject(RawFileObject);
    }

    RtlInitUnicodeString(&ObjectName, L"\\Device\\RawCdRom");
    Status = IoGetDeviceObjectPointer(&ObjectName,
                                      FILE_READ_ATTRIBUTES,
                                      &RawFileObject,
                                      &RawDeviceObject);
    if (NT_SUCCESS(Status))
    {
        FltpFsNotification(RawDeviceObject, TRUE);
        ObDereferenceObject(RawFileObject);
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
