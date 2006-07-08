/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Functions that deal with managing the FILE_OBJECT itself.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
IopParseDevice(IN PVOID ParseObject,
               IN PVOID ObjectType,
               IN OUT PACCESS_STATE AccessState,
               IN KPROCESSOR_MODE AccessMode,
               IN ULONG Attributes,
               IN OUT PUNICODE_STRING CompleteName,
               IN OUT PUNICODE_STRING RemainingName,
               IN OUT PVOID Context OPTIONAL,
               IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
               OUT PVOID *Object)
{
    POPEN_PACKET OpenPacket = (POPEN_PACKET)Context;
    PDEVICE_OBJECT OriginalDeviceObject = (PDEVICE_OBJECT)ParseObject;
    PDEVICE_OBJECT DeviceObject, OwnerDevice;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PVPB Vpb = NULL;
    PIRP Irp;
    PEXTENDED_IO_STACK_LOCATION StackLoc;
    IO_SECURITY_CONTEXT SecurityContext;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN DirectOpen = FALSE, OpenCancelled, UseDummyFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KIRQL OldIrql;
    PDUMMY_FILE_OBJECT DummyFileObject;
    PFILE_BASIC_INFORMATION FileBasicInfo;
    ULONG ReturnLength;

    /* Assume failure */
    *Object = NULL;

    /* Validate the open packet */
    if (!IopValidateOpenPacket(OpenPacket)) return STATUS_OBJECT_TYPE_MISMATCH;

    /* Check if we have a related file object */
    if (OpenPacket->RelatedFileObject)
    {
        /* Use the related file object's device object */
        OriginalDeviceObject = OpenPacket->RelatedFileObject->DeviceObject;
    }

    /* Reference the DO FIXME: Don't allow failure */
    //Status = IopReferenceDeviceObject(OriginalDeviceObject);
    OriginalDeviceObject->ReferenceCount++;

    /* Map the generic mask and set the new mapping in the access state */
    RtlMapGenericMask(&AccessState->RemainingDesiredAccess,
                      &IoFileObjectType->TypeInfo.GenericMapping);
    RtlMapGenericMask(&AccessState->OriginalDesiredAccess,
                      &IoFileObjectType->TypeInfo.GenericMapping);
    SeSetAccessStateGenericMapping(AccessState,
                                   &IoFileObjectType->TypeInfo.GenericMapping);

    /* Check if we can simply use a dummy file */
    UseDummyFile = ((OpenPacket->QueryOnly) || (OpenPacket->DeleteOnly));

    /* Check if this is a direct open */
    if (!(RemainingName->Length) &&
        !(OpenPacket->RelatedFileObject) &&
        !(UseDummyFile))
    {
        /* Remember this for later */
        DirectOpen = TRUE;
    }

    /* Check if we have a related FO that wasn't a direct open */
    if ((OpenPacket->RelatedFileObject) &&
        !(OpenPacket->RelatedFileObject->Flags & FO_DIRECT_DEVICE_OPEN))
    {
        /* The device object is the one we were given */
        DeviceObject = OriginalDeviceObject;

        /* Check if the related FO had a VPB */
        if (OpenPacket->RelatedFileObject->Vpb)
        {
            /* Yes, remember it */
            Vpb = OpenPacket->RelatedFileObject->Vpb;

            /* Reference it */
            InterlockedIncrement(&Vpb->ReferenceCount);
        }
    }
    else
    {
        /* The device object is the one we were given */
        DeviceObject = OriginalDeviceObject;

        /* Check if it has a VPB */
        if ((DeviceObject->Vpb) && !(DirectOpen))
        {
            /* Check if the VPB is mounted, and mount it */
            Vpb = IopCheckVpbMounted(OpenPacket,
                                     DeviceObject,
                                     RemainingName,
                                     &Status);
            if (!Vpb) return Status;

            /* Get the VPB's device object */
            DeviceObject = Vpb->DeviceObject;
        }

        /* Check if there's an attached device */
        if (DeviceObject->AttachedDevice)
        {
            /* Get the attached device */
            DeviceObject = IoGetAttachedDevice(DeviceObject);
        }
    }

    /* Check if we really need to create an object */
    if (!UseDummyFile)
    {
        /* Create the actual file object */
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   Attributes,
                                   NULL,
                                   NULL);
        Status = ObCreateObject(KernelMode,
                                IoFileObjectType,
                                &ObjectAttributes,
                                AccessMode,
                                NULL,
                                sizeof(FILE_OBJECT),
                                0,
                                0,
                                (PVOID*)&FileObject);
        RtlZeroMemory(FileObject, sizeof(FILE_OBJECT));

        /* Check if this is Synch I/O */
        if (OpenPacket->CreateOptions &
            (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT))
        {
            /* Set the synch flag */
            FileObject->Flags |= FO_SYNCHRONOUS_IO;

            /* Check if it's also alertable */
            if (OpenPacket->CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
            {
                /* It is, set the alertable flag */
                FileObject->Flags |= FO_ALERTABLE_IO;
            }
        }

        /* Check if the caller requested no intermediate buffering */
        if (OpenPacket->CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)
        {
            /* Set the correct flag for the FSD to read */
            FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;
        }

        /* Check if the caller requested write through support */
        if (OpenPacket->CreateOptions & FILE_WRITE_THROUGH)
        {
            /* Set the correct flag for the FSD to read */
            FileObject->Flags |= FO_WRITE_THROUGH;
        }

        /* Check if the caller says the file will be only read sequentially */
        if (OpenPacket->CreateOptions & FILE_SEQUENTIAL_ONLY)
        {
            /* Set the correct flag for the FSD to read */
            FileObject->Flags |= FO_SEQUENTIAL_ONLY;
        }

        /* Check if the caller believes the file will be only read randomly */
        if (OpenPacket->CreateOptions & FILE_RANDOM_ACCESS)
        {
            /* Set the correct flag for the FSD to read */
            FileObject->Flags |= FO_RANDOM_ACCESS;
        }
    }
    else
    {
        /* Use the dummy object instead */
        DummyFileObject = OpenPacket->DummyFileObject;
        RtlZeroMemory(DummyFileObject, sizeof(DUMMY_FILE_OBJECT));

        /* Set it up */
        FileObject = (PFILE_OBJECT)&DummyFileObject->ObjectHeader.Body;
        DummyFileObject->ObjectHeader.Type = IoFileObjectType;
        DummyFileObject->ObjectHeader.PointerCount = 1;
    }

    /* Setup the file header */
    FileObject->Type = IO_TYPE_FILE;
    FileObject->Size = sizeof(FILE_OBJECT);
    FileObject->RelatedFileObject = OpenPacket->RelatedFileObject;
    FileObject->DeviceObject = DeviceObject;

    /* Check if this is a direct device open */
    if (DirectOpen) FileObject->Flags |= FO_DIRECT_DEVICE_OPEN;

    /* Check if the caller wants case sensitivity */
    if (!(Attributes & OBJ_CASE_INSENSITIVE))
    {
        /* Tell the driver about it */
        FileObject->Flags |= FO_OPENED_CASE_SENSITIVE;
    }

    /* Setup the security context */
    SecurityContext.SecurityQos = SecurityQos;
    SecurityContext.AccessState = AccessState;
    SecurityContext.DesiredAccess = AccessState->RemainingDesiredAccess;
    SecurityContext.FullCreateOptions = OpenPacket->CreateOptions;

    /* Check if this is synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Initialize the event */
        KeInitializeEvent(&FileObject->Lock, SynchronizationEvent, TRUE);
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp)
    {
        /* Dereference the device and VPB, then fail */
        IopDereferenceDeviceObject(DeviceObject, FALSE);
        if (Vpb) IopDereferenceVpb(Vpb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Now set the IRP data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = AccessMode;
    Irp->Flags = IRP_CREATE_OPERATION |
                 IRP_SYNCHRONOUS_API |
                 IRP_DEFER_IO_COMPLETION;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->UserEvent = &FileObject->Event;
    Irp->UserIosb = &IoStatusBlock;
    Irp->MdlAddress = NULL;
    Irp->PendingReturned = FALSE;
    Irp->UserEvent = NULL;
    Irp->Cancel = FALSE;
    Irp->CancelRoutine = NULL;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    /* Get the I/O Stack location */
    StackLoc = (PEXTENDED_IO_STACK_LOCATION)IoGetNextIrpStackLocation(Irp);
    StackLoc->Control = 0;
    StackLoc->FileObject = FileObject;

    /* Check what kind of file this is */
    switch (OpenPacket->CreateFileType)
    {
        /* Normal file */
        case CreateFileTypeNone:

            /* Set the major function and EA Length */
            StackLoc->MajorFunction = IRP_MJ_CREATE;
            StackLoc->Parameters.Create.EaLength = OpenPacket->EaLength;

            /* Set the flags */
            StackLoc->Flags = OpenPacket->Options;
            StackLoc->Flags |= !(Attributes & OBJ_CASE_INSENSITIVE) ?
                                SL_CASE_SENSITIVE: 0;
            break;

        /* Named pipe */
        case CreateFileTypeNamedPipe:

            /* Set the named pipe MJ and set the parameters */
            StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
            StackLoc->Parameters.CreatePipe.Parameters = 
                OpenPacket->MailslotOrPipeParameters;
            break;

        /* Mailslot */
        case CreateFileTypeMailslot:

            /* Set the mailslot MJ and set the parameters */
            StackLoc->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
            StackLoc->Parameters.CreateMailslot.Parameters =
                OpenPacket->MailslotOrPipeParameters;
            break;
    }

    /* Set the common data */
    Irp->Overlay.AllocationSize = OpenPacket->AllocationSize;
    Irp->AssociatedIrp.SystemBuffer =OpenPacket->EaBuffer;
    StackLoc->Parameters.Create.Options = (OpenPacket->Disposition << 24) |
                                          (OpenPacket->CreateOptions &
                                           0xFFFFFF);
    StackLoc->Parameters.Create.FileAttributes = OpenPacket->FileAttributes;
    StackLoc->Parameters.Create.ShareAccess = OpenPacket->ShareAccess;
    StackLoc->Parameters.Create.SecurityContext = &SecurityContext;

    /* Check if the file object has a name */
    if (RemainingName->Length)
    {
        /* Setup the unicode string */
        FileObject->FileName.MaximumLength = RemainingName->Length +
                                             sizeof(WCHAR);
        FileObject->FileName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                            FileObject->
                                                            FileName.
                                                            MaximumLength,
                                                            TAG_IO_NAME);
        if (!FileObject->FileName.Buffer)
        {
            /* Failed to allocate the name, free the IRP */
            IoFreeIrp(Irp);

            /* Dereference the device object and VPB */
            IopDereferenceDeviceObject(DeviceObject, FALSE);
            if (Vpb) IopDereferenceVpb(Vpb);

            /* Clear the FO and dereference it */
            FileObject->DeviceObject = NULL;
            if (!UseDummyFile) ObDereferenceObject(FileObject);

            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Copy the name */
    RtlCopyUnicodeString(&FileObject->FileName, RemainingName);

    /* Reference the file object */
    ObReferenceObject(FileObject);

    /* Initialize the File Object event and set the FO */
    KeInitializeEvent(&FileObject->Event, NotificationEvent, FALSE);
    OpenPacket->FileObject = FileObject;

    /* Queue the IRP and call the driver */
    //IopQueueIrpToThread(Irp);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for the driver to complete the create */
        KeWaitForSingleObject(&FileObject->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        /* Get the new status */
        Status = IoStatusBlock.Status;
    }
    else
    {
        /* We'll have to complete it ourselves */
        ASSERT(!Irp->PendingReturned);

        /* Completion happens at APC_LEVEL */
        KeRaiseIrql(APC_LEVEL, &OldIrql);

        /* Get the new I/O Status block ourselves */
        IoStatusBlock = Irp->IoStatus;
        Status = IoStatusBlock.Status;

        /* Manually signal the even, we can't have any waiters */
        FileObject->Event.Header.SignalState = 1;

        /* Now that we've signaled the events, de-associate the IRP */
        RemoveEntryList(&Irp->ThreadListEntry);
        InitializeListHead(&Irp->ThreadListEntry);

        /* Check if the IRP had an input buffer */
        if ((Irp->Flags & IRP_BUFFERED_IO) &&
            (Irp->Flags & IRP_DEALLOCATE_BUFFER))
        {
            /* Free it. A driver might've tacked one on */
            ExFreePool(Irp->AssociatedIrp.SystemBuffer);
        }

        /* Free the IRP and bring the IRQL back down */
        IoFreeIrp(Irp);
        KeLowerIrql(OldIrql);
    }

    /* Copy the I/O Status */
    OpenPacket->Information = IoStatusBlock.Information;

    /* The driver failed to create the file */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we have a name */
        if (FileObject->FileName.Length)
        {
            /* Free it */
            ExFreePool(FileObject->FileName.Buffer);
            FileObject->FileName.Length = 0;
        }

        /* Clear its device object */
        FileObject->DeviceObject = NULL;

        /* Save this now because the FO might go away */
        OpenCancelled = FileObject->Flags & FO_FILE_OPEN_CANCELLED;

        /* Clear the file object in the open packet */
        OpenPacket->FileObject = NULL;

        /* Dereference the file object */
        if (!UseDummyFile) ObDereferenceObject(FileObject);

        /* Unless the driver canelled the open, dereference the VPB */
        if (!(OpenCancelled) && (Vpb)) IopDereferenceVpb(Vpb);

        /* Set the status and return */
        OpenPacket->FinalStatus = Status;
        return Status;
    }
    else if (Status == STATUS_REPARSE)
    {
        /* FIXME: We don't handle this at all! */
        KEBUGCHECK(0);
    }

    /* Get the owner of the File Object */
    OwnerDevice = IoGetRelatedDeviceObject(FileObject);

    /*
     * It's possible that the device to whom we sent the IRP to
     * isn't actually the device that ended opening the file object
     * internally.
     */
    if (OwnerDevice != DeviceObject)
    {
        /* We have to de-reference the VPB we had associated */
        if (Vpb) IopDereferenceVpb(Vpb);

        /* And re-associate with the actual one */
        Vpb = FileObject->Vpb;
        if (Vpb) InterlockedIncrement(&Vpb->ReferenceCount);
    }

    /* Make sure we are not using a dummy */
    if (!UseDummyFile)
    {
        /* Check if this was a volume open */
        if ((!(FileObject->RelatedFileObject) ||
              (FileObject->RelatedFileObject->Flags & FO_VOLUME_OPEN)) &&
            !(FileObject->FileName.Length))
        {
            /* All signs point to it, but make sure it was actually an FSD */
            if ((OwnerDevice->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) ||
                (OwnerDevice->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) ||
                (OwnerDevice->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM) ||
                (OwnerDevice->DeviceType == FILE_DEVICE_FILE_SYSTEM))
            {
                /* The owner device is an FSD, so this is a volume open for real */
                FileObject->Flags |= FO_VOLUME_OPEN;
            }
        }

        /* Reference the object and set the parse check */
        ObReferenceObject(FileObject);
        *Object = FileObject;
        OpenPacket->FinalStatus = IoStatusBlock.Status;
        OpenPacket->ParseCheck = TRUE;
        return OpenPacket->FinalStatus;
    }
    else
    {
        /* Check if this was a query */
        if (OpenPacket->QueryOnly)
        {
            /* Check if the caller wants basic info only */
            if (!OpenPacket->FullAttributes)
            {
                /* Allocate the buffer */
                FileBasicInfo = ExAllocatePoolWithTag(NonPagedPool,
                                                      sizeof(*FileBasicInfo),
                                                      TAG_IO);
                if (FileBasicInfo)
                {
                    /* Do the query */
                    Status = IoQueryFileInformation(FileObject,
                                                    FileBasicInformation,
                                                    sizeof(*FileBasicInfo),
                                                    FileBasicInfo,
                                                    &ReturnLength);
                    if (NT_SUCCESS(Status))
                    {
                        /* Copy the data */
                        RtlCopyMemory(OpenPacket->BasicInformation,
                                      FileBasicInfo,
                                      ReturnLength);
                    }

                    /* Free our buffer */
                    ExFreePool(FileBasicInfo);
                }
                else
                {
                    /* Fail */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                /* This is a full query */
                Status = IoQueryFileInformation(
                    FileObject,
                    FileNetworkOpenInformation,
                    sizeof(FILE_NETWORK_OPEN_INFORMATION),
                    OpenPacket->NetworkInformation,
                    &ReturnLength);
                if (!NT_SUCCESS(Status)) ASSERT(Status != STATUS_NOT_IMPLEMENTED);
            }
        }

        /* Delete the file object */
        IopDeleteFile(FileObject);

        /* Clear out the file */
        OpenPacket->FileObject = NULL;

        /* Set and return status */
        OpenPacket->FinalStatus = Status;
        OpenPacket->ParseCheck = TRUE;
        return Status;
    }
}

NTSTATUS
NTAPI
IopParseFile(IN PVOID ParseObject,
             IN PVOID ObjectType,
             IN OUT PACCESS_STATE AccessState,
             IN KPROCESSOR_MODE AccessMode,
             IN ULONG Attributes,
             IN OUT PUNICODE_STRING CompleteName,
             IN OUT PUNICODE_STRING RemainingName,
             IN OUT PVOID Context OPTIONAL,
             IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
             OUT PVOID *Object)
{
    PVOID DeviceObject;
    POPEN_PACKET OpenPacket = (POPEN_PACKET)Context;

    /* Validate the open packet */
    if (!IopValidateOpenPacket(OpenPacket)) return STATUS_OBJECT_TYPE_MISMATCH;

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(ParseObject);
    OpenPacket->RelatedFileObject = ParseObject;

    /* Call the main routine */
    return IopParseDevice(DeviceObject,
                          ObjectType,
                          AccessState,
                          AccessMode,
                          Attributes,
                          CompleteName,
                          RemainingName,
                          OpenPacket,
                          SecurityQos,
                          Object);
}

VOID
NTAPI
IopDeleteFile(IN PVOID ObjectBody)
{
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    KEVENT Event;
    PDEVICE_OBJECT DeviceObject;

    /* Check if the file has a device object */
    if (FileObject->DeviceObject)
    {
        /* Check if this is a direct open or not */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            /* Get the attached device */
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
        }
        else
        {
            /* Use the file object's device object */
            DeviceObject = IoGetRelatedDeviceObject(FileObject);
        }

        /* Check if this file was opened for Synch I/O */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            /* Lock it */
            IopLockFileObject(FileObject);
        }

        /* Clear and set up Events */
        KeClearEvent(&FileObject->Event);
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        /* Allocate an IRP */
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
        if (!Irp) return;

        /* Set it up */
        Irp->UserEvent = &Event;
        Irp->UserIosb = &Irp->IoStatus;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

        /* Set up Stack Pointer Data */
        StackPtr = IoGetNextIrpStackLocation(Irp);
        StackPtr->MajorFunction = IRP_MJ_CLOSE;
        StackPtr->DeviceObject = DeviceObject;
        StackPtr->FileObject = FileObject;

        /* Queue the IRP */
        //IopQueueIrpToThread(Irp);

        /* Call the FS Driver */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }

        /* Free the IRP */
        IoFreeIrp(Irp);

        /* Clear the file name */
        if (FileObject->FileName.Buffer)
        {
           ExFreePool(FileObject->FileName.Buffer);
           FileObject->FileName.Buffer = NULL;
        }

        /* Check if the FO had a completion port */
        if (FileObject->CompletionContext)
        {
            /* Free it */
            ObDereferenceObject(FileObject->CompletionContext->Port);
            ExFreePool(FileObject->CompletionContext);
        }
    }
}

NTSTATUS
NTAPI
IopSecurityFile(IN PVOID ObjectBody,
                IN SECURITY_OPERATION_CODE OperationCode,
                IN SECURITY_INFORMATION SecurityInformation,
                IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                IN OUT PULONG BufferLength,
                IN OUT PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
                IN POOL_TYPE PoolType,
                IN OUT PGENERIC_MAPPING GenericMapping)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if this is a device or file */
    if (((PFILE_OBJECT)ObjectBody)->Type == IO_TYPE_DEVICE)
    {
        /* It's a device */
        DeviceObject = (PDEVICE_OBJECT)ObjectBody;
        FileObject = NULL;
    }
    else
    {
        /* It's a file */
        FileObject = (PFILE_OBJECT)ObjectBody;

        /* Check if this is a direct open */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            /* Get the Device Object */
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
        }
        else
        {
            /* Otherwise, use the direct device*/
            DeviceObject = FileObject->DeviceObject;
        }
    }

    /* Check if the request was for a device object */
    if (!(FileObject) || (FileObject->Flags & FO_DIRECT_DEVICE_OPEN))
    {
        /* Check what kind of request this was */
        if (OperationCode == QuerySecurityDescriptor)
        {
            DPRINT1("FIXME: Device Query security descriptor UNHANDLED\n");
            return STATUS_SUCCESS;
        }
        else if (OperationCode == DeleteSecurityDescriptor)
        {
            /* Simply return success */
            return STATUS_SUCCESS;
        }
        else if (OperationCode == AssignSecurityDescriptor)
        {
            /* Make absolutely sure this is a device object */
            if (!(FileObject) || !(FileObject->Flags & FO_STREAM_FILE))
            {
                /* Assign the Security Descriptor */
                DeviceObject->SecurityDescriptor = SecurityDescriptor;
            }

            /* Return success */
            return STATUS_SUCCESS;
        }
        else
        {
            DPRINT1("FIXME: Set SD unimplemented for Devices\n");
            return STATUS_SUCCESS;
        }
    }
    else if (OperationCode == DeleteSecurityDescriptor)
    {
        /* Same as for devices, do nothing */
        return STATUS_SUCCESS;
    }

    /* At this point, we know we're a file. Reference it */
    ObReferenceObject(FileObject);

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock the file object */
        IopLockFileObject(FileObject);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Clear the File Object event */
    KeClearEvent(&FileObject->Event);

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = ExGetPreviousMode();
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    /* Set Stack Parameters */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;

    /* Check if this is a query or set */
    if (OperationCode == QuerySecurityDescriptor)
    {
        /* Set the major function and parameters */
        StackPtr->MajorFunction = IRP_MJ_QUERY_SECURITY;
        StackPtr->Parameters.QuerySecurity.SecurityInformation =
            SecurityInformation;
        StackPtr->Parameters.QuerySecurity.Length = *BufferLength;
        Irp->UserBuffer = SecurityDescriptor;
    }
    else
    {
        /* Set the major function and parameters for a set */
        StackPtr->MajorFunction = IRP_MJ_SET_SECURITY;
        StackPtr->Parameters.SetSecurity.SecurityInformation =
            SecurityInformation;
        StackPtr->Parameters.SetSecurity.SecurityDescriptor =
            SecurityDescriptor;
    }

    /* Queue the IRP */
    //IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Call the Driver */
    Status = IoCallDriver(FileObject->DeviceObject, Irp);

    /* Check if this was async I/O */
    if (LocalEvent)
    {
        /* Check if the IRP is pending completion */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the local event */
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
        /* Check if the IRP is pending completion */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the file obejct */
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = FileObject->FinalStatus;
        }

        /* Release the lock */
        IopUnlockFileObject(FileObject);
    }

    /* This Driver doesn't implement Security, so try to give it a default */
    if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        /* Was this a query? */
        if (OperationCode == QuerySecurityDescriptor)
        {
            /* Set a World Security Descriptor */
            Status = SeSetWorldSecurityDescriptor(SecurityInformation,
                                                  SecurityDescriptor,
                                                  BufferLength);
        }
        else
        {
            /* It wasn't a query, so just fake success */
            Status = STATUS_SUCCESS;
        }
    }
    else if (OperationCode == QuerySecurityDescriptor)
    {
        /* Callers usually expect the normalized form */
        if (Status == STATUS_BUFFER_OVERFLOW) Status = STATUS_BUFFER_TOO_SMALL;

        /* Return length */
        *BufferLength = IoStatusBlock.Information;
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
IopQueryNameFile(IN PVOID ObjectBody,
                 IN BOOLEAN HasName,
                 OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
                 IN ULONG Length,
                 OUT PULONG ReturnLength,
                 IN KPROCESSOR_MODE PreviousMode)
{
    POBJECT_NAME_INFORMATION LocalInfo;
    PFILE_NAME_INFORMATION LocalFileInfo;
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    ULONG LocalReturnLength, FileLength;
    NTSTATUS Status;
    PWCHAR p;

    /* Validate length */
    if (Length < sizeof(OBJECT_NAME_INFORMATION))
    {
        /* Wrong length, fail */
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Allocate Buffer */
    LocalInfo = ExAllocatePoolWithTag(PagedPool, Length, TAG_IO);
    if (!LocalInfo) return STATUS_INSUFFICIENT_RESOURCES;

    /* Query the name */
    Status = ObQueryNameString(FileObject->DeviceObject,
                               LocalInfo,
                               Length,
                               &LocalReturnLength);
    if (!NT_SUCCESS (Status))
    {
        /* Free the buffer and fail */
        ExFreePool(LocalInfo);
        return Status;
    }

    /* Copy the information */
    RtlCopyMemory(ObjectNameInfo,
                  LocalInfo,
                  LocalReturnLength > Length ?
                  Length : LocalReturnLength);

    /* Set buffer pointer */
    p = (PWCHAR)(ObjectNameInfo + 1);
    ObjectNameInfo->Name.Buffer = p;

    /* Advance in buffer */
    p += (LocalInfo->Name.Length / sizeof(WCHAR));

    /* Now get the file name buffer and check the length needed */
    LocalFileInfo = (PFILE_NAME_INFORMATION)LocalInfo;
    FileLength = Length -
                 LocalReturnLength +
                 FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);

    /* Query the File name */
    Status = IoQueryFileInformation(FileObject,
                                    FileNameInformation,
                                    Length,
                                    LocalFileInfo,
                                    &LocalReturnLength);
    if (NT_ERROR(Status))
    {
        /* Fail on errors only, allow warnings */
        ExFreePool(LocalInfo);
        return Status;
    }

    /* Now calculate the new lenghts left */
    FileLength = LocalReturnLength -
                 FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
    LocalReturnLength = (ULONG_PTR)p -
                        (ULONG_PTR)ObjectNameInfo +
                        LocalFileInfo->FileNameLength;

    /* Write the Name and null-terminate it */
    RtlMoveMemory(p, LocalFileInfo->FileName, FileLength);
    p += (FileLength / sizeof(WCHAR));
    *p = UNICODE_NULL;
    LocalReturnLength += sizeof(UNICODE_NULL);

    /* Return the length needed */
    *ReturnLength = LocalReturnLength;

    /* Setup the length and maximum length */
    FileLength = (ULONG_PTR)p - (ULONG_PTR)ObjectNameInfo;
    ObjectNameInfo->Name.Length = Length - sizeof(OBJECT_NAME_INFORMATION);
    ObjectNameInfo->Name.MaximumLength = ObjectNameInfo->Name.Length +
                                         sizeof(UNICODE_NULL);

    /* Free buffer and return */
    ExFreePool(LocalInfo);
    return Status;
}

VOID
NTAPI
IopCloseFile(IN PEPROCESS Process OPTIONAL,
             IN PVOID ObjectBody,
             IN ACCESS_MASK GrantedAccess,
             IN ULONG HandleCount,
             IN ULONG SystemHandleCount)
{
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    KEVENT Event;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    /* Check if the file is locked and has more then one handle opened */
    if ((FileObject->LockOperation) && (SystemHandleCount != 1))
    {
        DPRINT1("We need to unlock this file!\n");
        KEBUGCHECK(0);
    }

    /* Make sure this is the last handle */
    if (SystemHandleCount != 1) return;

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        /* Get the attached device */
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        /* Get the FO's device */
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Set the handle created flag */
    FileObject->Flags |= FO_HANDLE_CREATED;

    /* Check if this is a sync FO and lock it */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO) IopLockFileObject(FileObject);

    /* Clear and set up Events */
    KeClearEvent(&FileObject->Event);
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return;

    /* Set it up */
    Irp->UserEvent = &Event;
    Irp->UserIosb = &Irp->IoStatus;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

    /* Set up Stack Pointer Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_CLEANUP;
    StackPtr->FileObject = FileObject;

    /* Queue the IRP */
    //IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Call the FS Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, UserRequest, KernelMode, FALSE, NULL);
    }

    /* Free the IRP */
    IoFreeIrp(Irp);

    /* Release the lock if we were holding it */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO) IopUnlockFileObject(FileObject);
}

NTSTATUS
NTAPI
IopQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                       IN FILE_INFORMATION_CLASS FileInformationClass,
                       IN ULONG FileInformationSize,
                       OUT PVOID FileInformation)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE AccessMode = ExGetPreviousMode();
    DUMMY_FILE_OBJECT DummyFileObject;
    FILE_NETWORK_OPEN_INFORMATION NetworkOpenInfo;
    HANDLE Handle;
    OPEN_PACKET OpenPacket;
    BOOLEAN IsBasic;
    PAGED_CODE();

    /* Check if the caller was user mode */
    if (AccessMode != KernelMode)
    {
        /* Protect probe in SEH */
        _SEH_TRY
        {
            /* Probe the buffer */
            ProbeForWrite(FileInformation, FileInformationSize, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
        if (!NT_SUCCESS(Status))return Status;
    }

    /* Check if this is a basic or full request */
    IsBasic = (FileInformationSize == sizeof(FILE_BASIC_INFORMATION));

    /* Setup the Open Packet */
    OpenPacket.Type = IO_TYPE_OPEN_PACKET;
    OpenPacket.Size = sizeof(OPEN_PACKET);
    OpenPacket.FileObject = NULL;
    OpenPacket.FinalStatus = STATUS_SUCCESS;
    OpenPacket.Information = 0;
    OpenPacket.ParseCheck = 0;
    OpenPacket.RelatedFileObject = NULL;
    OpenPacket.AllocationSize.QuadPart = 0;
    OpenPacket.CreateOptions = FILE_OPEN_REPARSE_POINT;
    OpenPacket.FileAttributes = 0;
    OpenPacket.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    OpenPacket.EaBuffer = NULL;
    OpenPacket.EaLength = 0;
    OpenPacket.Options = 0;
    OpenPacket.Disposition = FILE_OPEN;
    OpenPacket.BasicInformation = IsBasic ? FileInformation : NULL;
    OpenPacket.NetworkInformation = IsBasic ? &NetworkOpenInfo :
                                    (AccessMode != KernelMode) ?
                                    &NetworkOpenInfo : FileInformation;
    OpenPacket.CreateFileType = 0;
    OpenPacket.MailslotOrPipeParameters = NULL;
    OpenPacket.Override = FALSE;
    OpenPacket.QueryOnly = TRUE;
    OpenPacket.DeleteOnly = FALSE;
    OpenPacket.FullAttributes = IsBasic ? FALSE : TRUE;
    OpenPacket.DummyFileObject = &DummyFileObject;
    OpenPacket.InternalFlags = 0;

    /* Update the operation count */
    IopUpdateOperationCount(IopOtherTransfer);

    /*
     * Attempt opening the file. This will call the I/O Parse Routine for
     * the File Object (IopParseDevice) which will use the dummy file obejct
     * send the IRP to its device object. Note that we have two statuses
     * to worry about: the Object Manager's status (in Status) and the I/O
     * status, which is in the Open Packet's Final Status, and determined
     * by the Parse Check member.
     */
    Status = ObOpenObjectByName(ObjectAttributes,
                                NULL,
                                AccessMode,
                                NULL,
                                FILE_READ_ATTRIBUTES,
                                &OpenPacket,
                                &Handle);
    if (OpenPacket.ParseCheck != TRUE)
    {
        /* Parse failed */
        return Status;
    }
    else
    {
        /* Use the Io status */
        Status = OpenPacket.FinalStatus;
    }

    /* Check if we were succesful and this was user mode and a full query */
    if ((NT_SUCCESS(Status)) && (AccessMode != KernelMode) && !(IsBasic))
    {
        /* Enter SEH for copy */
        _SEH_TRY
        {
            /* Copy the buffer back */
            RtlMoveMemory(FileInformation,
                          &NetworkOpenInfo,
                          FileInformationSize);
        }
        _SEH_HANDLE
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return status */
    return Status;
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuerySetFileInformation(IN FILE_INFORMATION_CLASS FileInformationClass,
                               IN ULONG Length,
                               IN BOOLEAN SetOperation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuotaBufferValidity(IN PFILE_QUOTA_INFORMATION QuotaBuffer,
                           IN ULONG QuotaLength,
                           OUT PULONG ErrorOffset)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateFile(OUT PHANDLE FileHandle,
             IN ACCESS_MASK DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PLARGE_INTEGER AllocationSize  OPTIONAL,
             IN ULONG FileAttributes,
             IN ULONG ShareAccess,
             IN ULONG CreateDisposition,
             IN ULONG CreateOptions,
             IN PVOID EaBuffer  OPTIONAL,
             IN ULONG EaLength,
             IN CREATE_FILE_TYPE CreateFileType,
             IN PVOID ExtraCreateParameters OPTIONAL,
             IN ULONG Options)
{
    KPROCESSOR_MODE AccessMode;
    HANDLE LocalHandle = 0;
    LARGE_INTEGER SafeAllocationSize;
    PVOID SystemEaBuffer = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    OPEN_PACKET OpenPacket;
    PAGED_CODE();

    if(Options & IO_NO_PARAMETER_CHECKING)
    {
        AccessMode = KernelMode;
    }
    else
    {
        AccessMode = ExGetPreviousMode();
    }

    if(AccessMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(FileHandle);
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            if (AllocationSize)
            {
                SafeAllocationSize = ProbeForReadLargeInteger(AllocationSize);
            }
            else
            {
                SafeAllocationSize.QuadPart = 0;
            }

            if ((EaBuffer) && (EaLength))
            {
                ProbeForRead(EaBuffer,
                             EaLength,
                             sizeof(ULONG));

                /* marshal EaBuffer */
                SystemEaBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                       EaLength,
                                                       TAG_EA);
                if(!SystemEaBuffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }

                RtlCopyMemory(SystemEaBuffer, EaBuffer, EaLength);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        if (AllocationSize)
        {
            SafeAllocationSize = *AllocationSize;
        }
        else
        {
            SafeAllocationSize.QuadPart = 0;
        }

        if ((EaBuffer) && (EaLength))
        {
            SystemEaBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                   EaLength,
                                                   TAG_EA);
            RtlCopyMemory(SystemEaBuffer, EaBuffer, EaLength);
        }
    }

    if(Options & IO_CHECK_CREATE_PARAMETERS)
    {
        DPRINT1("FIXME: IO_CHECK_CREATE_PARAMETERS not yet supported!\n");
    }

    /* Setup the Open Packet */
    OpenPacket.Type = IO_TYPE_OPEN_PACKET;
    OpenPacket.Size = sizeof(OPEN_PACKET);
    OpenPacket.FileObject = NULL;
    OpenPacket.FinalStatus = STATUS_SUCCESS;
    OpenPacket.Information = 0;
    OpenPacket.ParseCheck = 0;
    OpenPacket.RelatedFileObject = NULL;
    OpenPacket.OriginalAttributes = *ObjectAttributes;
    OpenPacket.AllocationSize = SafeAllocationSize;
    OpenPacket.CreateOptions = CreateOptions;
    OpenPacket.FileAttributes = FileAttributes;
    OpenPacket.ShareAccess = ShareAccess;
    OpenPacket.EaBuffer = SystemEaBuffer;
    OpenPacket.EaLength = EaLength;
    OpenPacket.Options = Options;
    OpenPacket.Disposition = CreateDisposition;
    OpenPacket.BasicInformation = NULL;
    OpenPacket.NetworkInformation = NULL;
    OpenPacket.CreateFileType = CreateFileType;
    OpenPacket.MailslotOrPipeParameters = ExtraCreateParameters;
    OpenPacket.Override = FALSE;
    OpenPacket.QueryOnly = FALSE;
    OpenPacket.DeleteOnly = FALSE;
    OpenPacket.FullAttributes = FALSE;
    OpenPacket.DummyFileObject = NULL;
    OpenPacket.InternalFlags = 0;

    /* Update the operation count */
    IopUpdateOperationCount(IopOtherTransfer);

    /*
     * Attempt opening the file. This will call the I/O Parse Routine for
     * the File Object (IopParseDevice) which will create the object and
     * send the IRP to its device object. Note that we have two statuses
     * to worry about: the Object Manager's status (in Status) and the I/O
     * status, which is in the Open Packet's Final Status, and determined
     * by the Parse Check member.
     */
    Status = ObOpenObjectByName(ObjectAttributes,
                                NULL,
                                AccessMode,
                                NULL,
                                DesiredAccess,
                                &OpenPacket,
                                &LocalHandle);

    /* Free the EA Buffer */
    if (OpenPacket.EaBuffer) ExFreePool(OpenPacket.EaBuffer);

    /* Now check for Ob or Io failure */
    if (!(NT_SUCCESS(Status)) || (OpenPacket.ParseCheck != TRUE))
    {
        /* Check if Ob thinks well went well */
        if (NT_SUCCESS(Status))
        {
            /*
             * Tell it otherwise. Because we didn't use an ObjectType,
             * it incorrectly returned us a handle to God knows what.
             */
            ZwClose(LocalHandle);
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        /* Now check the Io status */
        if (!NT_SUCCESS(OpenPacket.FinalStatus))
        {
            /* Use this status instead of Ob's */
            Status = OpenPacket.FinalStatus;

            /* Check if it was only a warning */
            if (NT_WARNING(Status))
            {
                /* Protect write with SEH */
                _SEH_TRY
                {
                    /* In this case, we copy the I/O Status back */
                    IoStatusBlock->Information = OpenPacket.Information;
                    IoStatusBlock->Status = OpenPacket.FinalStatus;
                }
                _SEH_HANDLE
                {
                    /* Get exception code */
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
        }
        else if ((OpenPacket.FileObject) && (OpenPacket.ParseCheck != 1))
        {
            /*
             * This can happen in the very bizare case where the parse routine
             * actually executed more then once (due to a reparse) and ended
             * up failing after already having created the File Object.
             */
            if (OpenPacket.FileObject->FileName.Length)
            {
                /* It had a name, free it */
                ExFreePool(OpenPacket.FileObject->FileName.Buffer);
            }

            /* Clear the device object to invalidate the FO, and dereference */
            OpenPacket.FileObject->DeviceObject = NULL;
            ObDereferenceObject(OpenPacket.FileObject);
        }
    }
    else
    {
        /* We reached success and have a valid file handle */
        OpenPacket.FileObject->Flags |= FO_HANDLE_CREATED;

        /* Enter SEH for write back */
        _SEH_TRY
        {
            /* Write back the handle and I/O Status */
            *FileHandle = LocalHandle;
            IoStatusBlock->Information = OpenPacket.Information;
            IoStatusBlock->Status = OpenPacket.FinalStatus;

            /* Get the Io status */
            Status = OpenPacket.FinalStatus;
        }
        _SEH_HANDLE
        {
            /* Get the exception status */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Check if we were 100% successful */
    if ((OpenPacket.ParseCheck == TRUE) && (OpenPacket.FileObject))
    {
        /* Dereference the File Object */
        ObDereferenceObject(OpenPacket.FileObject);
    }

    /* Return status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCreateFileSpecifyDeviceObjectHint(OUT PHANDLE FileHandle,
                                    IN ACCESS_MASK DesiredAccess,
                                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                                    OUT PIO_STATUS_BLOCK IoStatusBlock,
                                    IN PLARGE_INTEGER AllocationSize OPTIONAL,
                                    IN ULONG FileAttributes,
                                    IN ULONG ShareAccess,
                                    IN ULONG Disposition,
                                    IN ULONG CreateOptions,
                                    IN PVOID EaBuffer OPTIONAL,
                                    IN ULONG EaLength,
                                    IN CREATE_FILE_TYPE CreateFileType,
                                    IN PVOID ExtraCreateParameters OPTIONAL,
                                    IN ULONG Options,
                                    IN PVOID DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
PFILE_OBJECT
NTAPI
IoCreateStreamFileObject(IN PFILE_OBJECT FileObject,
                         IN PDEVICE_OBJECT DeviceObject)
{
    PFILE_OBJECT CreatedFileObject;
    NTSTATUS Status;
    HANDLE FileHandle;
    PAGED_CODE();

    /* Create the File Object */
    Status = ObCreateObject(KernelMode,
                            IoFileObjectType,
                            NULL,
                            KernelMode,
                            NULL,
                            sizeof(FILE_OBJECT),
                            sizeof(FILE_OBJECT),
                            0,
                            (PVOID*)&CreatedFileObject);
    if (!NT_SUCCESS(Status)) return NULL;

    /* Choose Device Object */
    if (FileObject) DeviceObject = FileObject->DeviceObject;

    /* Set File Object Data */
    RtlZeroMemory(CreatedFileObject, sizeof(FILE_OBJECT));
    CreatedFileObject->DeviceObject = DeviceObject; 
    CreatedFileObject->Type = IO_TYPE_FILE;
    CreatedFileObject->Size = sizeof(FILE_OBJECT);
    CreatedFileObject->Flags = FO_STREAM_FILE;

    /* Initialize the wait event */
    KeInitializeEvent(&CreatedFileObject->Event, SynchronizationEvent, FALSE);

    /* Insert it to create a handle for it */
    Status = ObInsertObject(CreatedFileObject,
                            NULL,
                            FILE_READ_DATA,
                            1,
                            (PVOID*)&CreatedFileObject,
                            &FileHandle);
    CreatedFileObject->Flags |= FO_HANDLE_CREATED;

    /* Close the extra handle and return file */
    NtClose(FileHandle);
    return CreatedFileObject;
}

/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectEx(IN PFILE_OBJECT FileObject OPTIONAL,
                           IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                           OUT PHANDLE FileObjectHandle OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectLite(IN PFILE_OBJECT FileObject OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
PGENERIC_MAPPING
STDCALL
IoGetFileObjectGenericMapping(VOID)
{
    return &IopFileMapping;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
IoIsFileOriginRemote(IN PFILE_OBJECT FileObject)
{
    /* Return the flag status */
    return (FileObject->Flags & FO_REMOTE_ORIGIN);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoQueryFileDosDeviceName(IN PFILE_OBJECT FileObject,
                         OUT POBJECT_NAME_INFORMATION *ObjectNameInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetFileOrigin(IN PFILE_OBJECT FileObject,
                IN BOOLEAN Remote)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}



/*
 * NAME       EXPORTED
 * NtCreateFile@44
 *
 * DESCRIPTION
 * Entry point to call IoCreateFile with
 * default parameters.
 *
 * ARGUMENTS
 *  See IoCreateFile.
 *
 * RETURN VALUE
 *  See IoCreateFile.
 *
 * REVISIONS
 *  2000-03-25 (ea)
 *   Code originally in NtCreateFile moved in IoCreateFile.
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtCreateFile(PHANDLE FileHandle,
             ACCESS_MASK DesiredAccess,
             POBJECT_ATTRIBUTES ObjectAttributes,
             PIO_STATUS_BLOCK IoStatusBlock,
             PLARGE_INTEGER AllocateSize,
             ULONG FileAttributes,
             ULONG ShareAccess,
             ULONG CreateDisposition,
             ULONG CreateOptions,
             PVOID EaBuffer,
             ULONG EaLength)
{
    /* Call the I/O Function */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        AllocateSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        EaBuffer,
                        EaLength,
                        CreateFileTypeNone,
                        NULL,
                        0);
}

NTSTATUS
STDCALL
NtCreateMailslotFile(OUT PHANDLE FileHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN POBJECT_ATTRIBUTES ObjectAttributes,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN ULONG CreateOptions,
                     IN ULONG MailslotQuota,
                     IN ULONG MaxMessageSize,
                     IN PLARGE_INTEGER TimeOut)
{
    MAILSLOT_CREATE_PARAMETERS Buffer;

    DPRINT("NtCreateMailslotFile(FileHandle 0x%p, DesiredAccess %x, "
           "ObjectAttributes 0x%p)\n",
           FileHandle,DesiredAccess,ObjectAttributes);

    PAGED_CODE();

    /* Check for Timeout */
    if (TimeOut != NULL)
    {
        if (KeGetPreviousMode() != KernelMode)
        {
            NTSTATUS Status = STATUS_SUCCESS;

            _SEH_TRY
            {
                Buffer.ReadTimeout = ProbeForReadLargeInteger(TimeOut);
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if (!NT_SUCCESS(Status)) return Status;
        }
        else
        {
            Buffer.ReadTimeout = *TimeOut;
        }

        Buffer.TimeoutSpecified = TRUE;
    }
    else
    {
        Buffer.TimeoutSpecified = FALSE;
    }

    /* Set Settings */
    Buffer.MailslotQuota = MailslotQuota;
    Buffer.MaximumMessageSize = MaxMessageSize;

    /* Call I/O */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        CreateOptions,
                        NULL,
                        0,
                        CreateFileTypeMailslot,
                        (PVOID)&Buffer,
                        0);
}

NTSTATUS
STDCALL
NtCreateNamedPipeFile(PHANDLE FileHandle,
                      ACCESS_MASK DesiredAccess,
                      POBJECT_ATTRIBUTES ObjectAttributes,
                      PIO_STATUS_BLOCK IoStatusBlock,
                      ULONG ShareAccess,
                      ULONG CreateDisposition,
                      ULONG CreateOptions,
                      ULONG NamedPipeType,
                      ULONG ReadMode,
                      ULONG CompletionMode,
                      ULONG MaximumInstances,
                      ULONG InboundQuota,
                      ULONG OutboundQuota,
                      PLARGE_INTEGER DefaultTimeout)
{
    NAMED_PIPE_CREATE_PARAMETERS Buffer;

    DPRINT("NtCreateNamedPipeFile(FileHandle 0x%p, DesiredAccess %x, "
           "ObjectAttributes 0x%p)\n",
            FileHandle,DesiredAccess,ObjectAttributes);

    PAGED_CODE();

    /* Check for Timeout */
    if (DefaultTimeout != NULL)
    {
        if (KeGetPreviousMode() != KernelMode)
        {
            NTSTATUS Status = STATUS_SUCCESS;

            _SEH_TRY
            {
                Buffer.DefaultTimeout = ProbeForReadLargeInteger(DefaultTimeout);
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if (!NT_SUCCESS(Status)) return Status;
        }
        else
        {
            Buffer.DefaultTimeout = *DefaultTimeout;
        }

        Buffer.TimeoutSpecified = TRUE;
    }
    else
        Buffer.TimeoutSpecified = FALSE;

    /* Set Settings */
    Buffer.NamedPipeType = NamedPipeType;
    Buffer.ReadMode = ReadMode;
    Buffer.CompletionMode = CompletionMode;
    Buffer.MaximumInstances = MaximumInstances;
    Buffer.InboundQuota = InboundQuota;
    Buffer.OutboundQuota = OutboundQuota;

    /* Call I/O */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        CreateFileTypeNamedPipe,
                        (PVOID)&Buffer,
                        0);
}

NTSTATUS
STDCALL
NtFlushWriteBuffer(VOID)
{
    PAGED_CODE();

    KeFlushWriteBuffer();
    return STATUS_SUCCESS;
}

/*
 * NAME       EXPORTED
 *  NtOpenFile@24
 *
 * DESCRIPTION
 *  Opens an existing file (simpler than NtCreateFile).
 *
 * ARGUMENTS
 * FileHandle (OUT)
 *  Variable that receives the file handle on return;
 *
 * DesiredAccess
 *  Access desired by the caller to the file;
 *
 * ObjectAttributes
 *  Structue describing the file to be opened;
 *
 * IoStatusBlock (OUT)
 *  Receives details about the result of the
 *  operation;
 *
 * ShareAccess
 *  Type of shared access the caller requires;
 *
 * OpenOptions
 *  Options for the file open.
 *
 * RETURN VALUE
 *  Status.
 *
 * NOTE
 *  Undocumented.
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenFile(PHANDLE FileHandle,
           ACCESS_MASK DesiredAccess,
           POBJECT_ATTRIBUTES ObjectAttributes,
           PIO_STATUS_BLOCK IoStatusBlock,
           ULONG ShareAccess,
           ULONG OpenOptions)
{
    /* Call the I/O Function */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        0,
                        ShareAccess,
                        FILE_OPEN,
                        OpenOptions,
                        NULL,
                        0,
                        CreateFileTypeNone,
                        NULL,
                        0);
}

NTSTATUS
STDCALL
NtQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                      OUT PFILE_BASIC_INFORMATION FileInformation)
{
    /* Call the internal helper API */
    return IopQueryAttributesFile(ObjectAttributes,
                                  FileBasicInformation,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileInformation);
}

NTSTATUS
STDCALL
NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                          OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
    /* Call the internal helper API */
    return IopQueryAttributesFile(ObjectAttributes,
                                  FileNetworkOpenInformation,
                                  sizeof(FILE_NETWORK_OPEN_INFORMATION),
                                  FileInformation);
}

/**
 * @name NtCancelIoFile
 *
 * Cancel all pending I/O operations in the current thread for specified
 * file object.
 *
 * @param FileHandle
 *        Handle to file object to cancel requests for. No specific
 *        access rights are needed.
 * @param IoStatusBlock
 *        Pointer to status block which is filled with final completition
 *        status on successful return.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS
NTAPI
NtCancelIoFile(IN HANDLE FileHandle,
               OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PFILE_OBJECT FileObject;
    PETHREAD Thread;
    PIRP Irp;
    KIRQL OldIrql;
    BOOLEAN OurIrpsInList = FALSE;
    LARGE_INTEGER Interval;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* IRP cancellations are synchronized at APC_LEVEL. */
    OldIrql = KfRaiseIrql(APC_LEVEL);

    /*
    * Walk the list of active IRPs and cancel the ones that belong to
    * our file object.
    */

    Thread = PsGetCurrentThread();

    LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)
    {
        if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
        {
            IoCancelIrp(Irp);
            /* Don't break here, we want to cancel all IRPs for the file object. */
            OurIrpsInList = TRUE;
        }
    }

    KfLowerIrql(OldIrql);

    while (OurIrpsInList)
    {
        OurIrpsInList = FALSE;

        /* Wait a short while and then look if all our IRPs were completed. */
        Interval.QuadPart = -1000000; /* 100 milliseconds */
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        OldIrql = KfRaiseIrql(APC_LEVEL);

        /*
        * Look in the list if all IRPs for the specified file object
        * are completed (or cancelled). If someone sends a new IRP
        * for our file object while we're here we can happily loop
        * forever.
        */

        LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)
        {
            if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
            {
                OurIrpsInList = TRUE;
                break;
            }
        }

        KfLowerIrql(OldIrql);
    }

    _SEH_TRY
    {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = 0;
        Status = STATUS_SUCCESS;
    }
    _SEH_HANDLE
    {
    
    }
    _SEH_END;

    ObDereferenceObject(FileObject);
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
IoFastQueryNetworkAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
			     IN ACCESS_MASK DesiredAccess,
			     IN ULONG OpenOptions,
			     OUT PIO_STATUS_BLOCK IoStatus,
			     OUT PFILE_NETWORK_OPEN_INFORMATION Buffer)
{
  UNIMPLEMENTED;
  return(FALSE);
}

/*
 * @implemented
 */
VOID STDCALL
IoUpdateShareAccess(PFILE_OBJECT FileObject,
		    PSHARE_ACCESS ShareAccess)
{
   PAGED_CODE();

   if (FileObject->ReadAccess ||
       FileObject->WriteAccess ||
       FileObject->DeleteAccess)
     {
       ShareAccess->OpenCount++;

       ShareAccess->Readers += FileObject->ReadAccess;
       ShareAccess->Writers += FileObject->WriteAccess;
       ShareAccess->Deleters += FileObject->DeleteAccess;
       ShareAccess->SharedRead += FileObject->SharedRead;
       ShareAccess->SharedWrite += FileObject->SharedWrite;
       ShareAccess->SharedDelete += FileObject->SharedDelete;
     }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
IoCheckShareAccess(IN ACCESS_MASK DesiredAccess,
		   IN ULONG DesiredShareAccess,
		   IN PFILE_OBJECT FileObject,
		   IN PSHARE_ACCESS ShareAccess,
		   IN BOOLEAN Update)
{
  BOOLEAN ReadAccess;
  BOOLEAN WriteAccess;
  BOOLEAN DeleteAccess;
  BOOLEAN SharedRead;
  BOOLEAN SharedWrite;
  BOOLEAN SharedDelete;

  PAGED_CODE();

  ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
  WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
  DeleteAccess = (DesiredAccess & DELETE) != 0;

  FileObject->ReadAccess = ReadAccess;
  FileObject->WriteAccess = WriteAccess;
  FileObject->DeleteAccess = DeleteAccess;

  if (ReadAccess || WriteAccess || DeleteAccess)
    {
      SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
      SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
      SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

      FileObject->SharedRead = SharedRead;
      FileObject->SharedWrite = SharedWrite;
      FileObject->SharedDelete = SharedDelete;

      if ((ReadAccess && (ShareAccess->SharedRead < ShareAccess->OpenCount)) ||
          (WriteAccess && (ShareAccess->SharedWrite < ShareAccess->OpenCount)) ||
          (DeleteAccess && (ShareAccess->SharedDelete < ShareAccess->OpenCount)) ||
          ((ShareAccess->Readers != 0) && !SharedRead) ||
          ((ShareAccess->Writers != 0) && !SharedWrite) ||
          ((ShareAccess->Deleters != 0) && !SharedDelete))
        {
          return(STATUS_SHARING_VIOLATION);
        }

      if (Update)
        {
          ShareAccess->OpenCount++;

          ShareAccess->Readers += ReadAccess;
          ShareAccess->Writers += WriteAccess;
          ShareAccess->Deleters += DeleteAccess;
          ShareAccess->SharedRead += SharedRead;
          ShareAccess->SharedWrite += SharedWrite;
          ShareAccess->SharedDelete += SharedDelete;
        }
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
IoRemoveShareAccess(IN PFILE_OBJECT FileObject,
		    IN PSHARE_ACCESS ShareAccess)
{
  PAGED_CODE();

  if (FileObject->ReadAccess ||
      FileObject->WriteAccess ||
      FileObject->DeleteAccess)
    {
      ShareAccess->OpenCount--;

      ShareAccess->Readers -= FileObject->ReadAccess;
      ShareAccess->Writers -= FileObject->WriteAccess;
      ShareAccess->Deleters -= FileObject->DeleteAccess;
      ShareAccess->SharedRead -= FileObject->SharedRead;
      ShareAccess->SharedWrite -= FileObject->SharedWrite;
      ShareAccess->SharedDelete -= FileObject->SharedDelete;
    }
}


/*
 * @implemented
 */
VOID STDCALL
IoSetShareAccess(IN ACCESS_MASK DesiredAccess,
		 IN ULONG DesiredShareAccess,
		 IN PFILE_OBJECT FileObject,
		 OUT PSHARE_ACCESS ShareAccess)
{
  BOOLEAN ReadAccess;
  BOOLEAN WriteAccess;
  BOOLEAN DeleteAccess;
  BOOLEAN SharedRead;
  BOOLEAN SharedWrite;
  BOOLEAN SharedDelete;

  PAGED_CODE();

  ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
  WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
  DeleteAccess = (DesiredAccess & DELETE) != 0;

  FileObject->ReadAccess = ReadAccess;
  FileObject->WriteAccess = WriteAccess;
  FileObject->DeleteAccess = DeleteAccess;

  if (!ReadAccess && !WriteAccess && !DeleteAccess)
    {
      ShareAccess->OpenCount = 0;
      ShareAccess->Readers = 0;
      ShareAccess->Writers = 0;
      ShareAccess->Deleters = 0;

      ShareAccess->SharedRead = 0;
      ShareAccess->SharedWrite = 0;
      ShareAccess->SharedDelete = 0;
    }
  else
    {
      SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
      SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
      SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

      FileObject->SharedRead = SharedRead;
      FileObject->SharedWrite = SharedWrite;
      FileObject->SharedDelete = SharedDelete;

      ShareAccess->OpenCount = 1;
      ShareAccess->Readers = ReadAccess;
      ShareAccess->Writers = WriteAccess;
      ShareAccess->Deleters = DeleteAccess;

      ShareAccess->SharedRead = SharedRead;
      ShareAccess->SharedWrite = SharedWrite;
      ShareAccess->SharedDelete = SharedDelete;
    }
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoCancelFileOpen(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PFILE_OBJECT    FileObject
    )
{
	UNIMPLEMENTED;
}
/* EOF */
