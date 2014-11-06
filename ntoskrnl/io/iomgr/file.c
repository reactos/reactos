/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         Functions that deal with managing the FILE_OBJECT itself.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Eric Kohl
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
IopCheckBackupRestorePrivilege(IN PACCESS_STATE AccessState,
                               IN OUT PULONG CreateOptions,
                               IN KPROCESSOR_MODE PreviousMode,
                               IN ULONG Disposition)
{
    ACCESS_MASK DesiredAccess, ReadAccess, WriteAccess;
    PRIVILEGE_SET Privileges;
    BOOLEAN AccessGranted, HaveBackupPriv = FALSE, CheckRestore = FALSE;
    PAGED_CODE();

    /* Don't do anything if privileges were checked already */
    if (AccessState->Flags & SE_BACKUP_PRIVILEGES_CHECKED) return;

    /* Check if the file was actually opened for backup purposes */
    if (*CreateOptions & FILE_OPEN_FOR_BACKUP_INTENT)
    {
        /* Set the check flag since were doing it now */
        AccessState->Flags |= SE_BACKUP_PRIVILEGES_CHECKED;

        /* Set the access masks required */
        ReadAccess = READ_CONTROL |
                     ACCESS_SYSTEM_SECURITY |
                     FILE_GENERIC_READ |
                     FILE_TRAVERSE;
        WriteAccess = WRITE_DAC |
                      WRITE_OWNER |
                      ACCESS_SYSTEM_SECURITY |
                      FILE_GENERIC_WRITE |
                      FILE_ADD_FILE |
                      FILE_ADD_SUBDIRECTORY |
                      DELETE;
        DesiredAccess = AccessState->RemainingDesiredAccess;

        /* Check if desired access was the maximum */
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Then add all the access masks required */
            DesiredAccess |= (ReadAccess | WriteAccess);
        }

        /* Check if the file already exists */
        if (Disposition & FILE_OPEN)
        {
            /* Check if desired access has the read mask */
            if (ReadAccess & DesiredAccess)
            {
                /* Setup the privilege check lookup */
                Privileges.PrivilegeCount = 1;
                Privileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
                Privileges.Privilege[0].Luid = SeBackupPrivilege;
                Privileges.Privilege[0].Attributes = 0;
                AccessGranted = SePrivilegeCheck(&Privileges,
                                                 &AccessState->
                                                 SubjectSecurityContext,
                                                 PreviousMode);
                if (AccessGranted)
                {
                    /* Remember that backup was allowed */
                    HaveBackupPriv = TRUE;

                    /* Append the privileges and update the access state */
                    SeAppendPrivileges(AccessState, &Privileges);
                    AccessState->PreviouslyGrantedAccess |= (DesiredAccess & ReadAccess);
                    AccessState->RemainingDesiredAccess &= ~ReadAccess;
                    DesiredAccess &= ~ReadAccess;

                    /* Set backup privilege for the token */
                    AccessState->Flags |= TOKEN_HAS_BACKUP_PRIVILEGE;
                }
            }
        }
        else
        {
            /* Caller is creating the file, check restore privileges later */
            CheckRestore = TRUE;
        }

        /* Check if caller wants write access or if it's creating a file */
        if ((WriteAccess & DesiredAccess) || (CheckRestore))
        {
            /* Setup the privilege lookup and do it */
            Privileges.PrivilegeCount = 1;
            Privileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
            Privileges.Privilege[0].Luid = SeRestorePrivilege;
            Privileges.Privilege[0].Attributes = 0;
            AccessGranted = SePrivilegeCheck(&Privileges,
                                             &AccessState->SubjectSecurityContext,
                                             PreviousMode);
            if (AccessGranted)
            {
                /* Remember that privilege was given */
                HaveBackupPriv = TRUE;

                /* Append the privileges and update the access state */
                SeAppendPrivileges(AccessState, &Privileges);
                AccessState->PreviouslyGrantedAccess |= (DesiredAccess & WriteAccess);
                AccessState->RemainingDesiredAccess &= ~WriteAccess;

                /* Set restore privilege for the token */
                AccessState->Flags |= TOKEN_HAS_RESTORE_PRIVILEGE;
            }
        }

        /* If we don't have the privilege, remove the option */
        if (!HaveBackupPriv) *CreateOptions &= ~FILE_OPEN_FOR_BACKUP_INTENT;
    }
}

NTSTATUS
NTAPI
IopCheckDeviceAndDriver(IN POPEN_PACKET OpenPacket,
                        IN PDEVICE_OBJECT DeviceObject)
{
    /* Make sure the object is valid */
    if ((IoGetDevObjExtension(DeviceObject)->ExtensionFlags &
        (DOE_UNLOAD_PENDING |
         DOE_DELETE_PENDING |
         DOE_REMOVE_PENDING |
         DOE_REMOVE_PROCESSED)) ||
        (DeviceObject->Flags & DO_DEVICE_INITIALIZING))
    {
        /* It's unloading or initializing, so fail */
        DPRINT1("You are seeing this because the following ROS driver: %wZ\n"
                " sucks. Please fix it's AddDevice Routine\n",
                &DeviceObject->DriverObject->DriverName);
        return STATUS_NO_SUCH_DEVICE;
    }
    else if ((DeviceObject->Flags & DO_EXCLUSIVE) &&
             (DeviceObject->ReferenceCount) &&
             !(OpenPacket->RelatedFileObject) &&
             !(OpenPacket->Options & IO_ATTACH_DEVICE))
    {
        return STATUS_ACCESS_DENIED;
    }

    else
    {
        /* Increase reference count */
        InterlockedIncrement(&DeviceObject->ReferenceCount);
        return STATUS_SUCCESS;
    }
}

NTSTATUS
NTAPI
IopParseDevice(IN PVOID ParseObject,
               IN PVOID ObjectType,
               IN OUT PACCESS_STATE AccessState,
               IN KPROCESSOR_MODE AccessMode,
               IN ULONG Attributes,
               IN OUT PUNICODE_STRING CompleteName,
               IN OUT PUNICODE_STRING RemainingName,
               IN OUT PVOID Context,
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
    PDUMMY_FILE_OBJECT LocalFileObject;
    PFILE_BASIC_INFORMATION FileBasicInfo;
    ULONG ReturnLength;
    KPROCESSOR_MODE CheckMode;
    BOOLEAN VolumeOpen = FALSE;
    ACCESS_MASK DesiredAccess, GrantedAccess;
    BOOLEAN AccessGranted, LockHeld = FALSE;
    PPRIVILEGE_SET Privileges = NULL;
    UNICODE_STRING FileString;
    IOTRACE(IO_FILE_DEBUG, "ParseObject: %p. RemainingName: %wZ\n",
            ParseObject, RemainingName);

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

    /* Validate device status */
    Status = IopCheckDeviceAndDriver(OpenPacket, OriginalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, return status */
        OpenPacket->FinalStatus = Status;
        return Status;
    }

    /* Map the generic mask and set the new mapping in the access state */
    RtlMapGenericMask(&AccessState->RemainingDesiredAccess,
                      &IoFileObjectType->TypeInfo.GenericMapping);
    RtlMapGenericMask(&AccessState->OriginalDesiredAccess,
                      &IoFileObjectType->TypeInfo.GenericMapping);
    SeSetAccessStateGenericMapping(AccessState,
                                   &IoFileObjectType->TypeInfo.GenericMapping);
    DesiredAccess = AccessState->RemainingDesiredAccess;

    /* Check what kind of access checks to do */
    if ((AccessMode != KernelMode) ||
        (OpenPacket->Options & IO_FORCE_ACCESS_CHECK))
    {
        /* Call is from user-mode or kernel is forcing checks */
        CheckMode = UserMode;
    }
    else
    {
        /* Call is from the kernel */
        CheckMode = KernelMode;
    }

    /* Check privilege for backup or restore operation */
    IopCheckBackupRestorePrivilege(AccessState,
                                   &OpenPacket->CreateOptions,
                                   CheckMode,
                                   OpenPacket->Disposition);

    /* Check if we are re-parsing */
    if (((OpenPacket->Override) && !(RemainingName->Length)) ||
        (AccessState->Flags & SE_BACKUP_PRIVILEGES_CHECKED))
    {
        /* Get granted access from the last call */
        DesiredAccess |= AccessState->PreviouslyGrantedAccess;
    }

    /* Check if this is a volume open */
    if ((OpenPacket->RelatedFileObject) &&
        (OpenPacket->RelatedFileObject->Flags & FO_VOLUME_OPEN) &&
        !(RemainingName->Length))
    {
        /* It is */
        VolumeOpen = TRUE;
    }

    /* Now check if we need access checks */
    if (((AccessMode != KernelMode) ||
         (OpenPacket->Options & IO_FORCE_ACCESS_CHECK)) &&
        (!(OpenPacket->RelatedFileObject) || (VolumeOpen)) &&
        !(OpenPacket->Override))
    {
        /* Check if a device object is being parsed  */
        if (!RemainingName->Length)
        {
            /* Lock the subject context */
            SeLockSubjectContext(&AccessState->SubjectSecurityContext);
            LockHeld = TRUE;

            /* Do access check */
            AccessGranted = SeAccessCheck(OriginalDeviceObject->
                                          SecurityDescriptor,
                                          &AccessState->SubjectSecurityContext,
                                          LockHeld,
                                          DesiredAccess,
                                          0,
                                          &Privileges,
                                          &IoFileObjectType->
                                          TypeInfo.GenericMapping,
                                          UserMode,
                                          &GrantedAccess,
                                          &Status);
            if (Privileges)
            {
                /* Append and free the privileges */
                SeAppendPrivileges(AccessState, Privileges);
                SeFreePrivileges(Privileges);
            }

            /* Check if we got access */
            if (AccessGranted)
            {
                /* Update access state */
                AccessState->PreviouslyGrantedAccess |= GrantedAccess;
                AccessState->RemainingDesiredAccess &= ~(GrantedAccess &
                                                         MAXIMUM_ALLOWED);
                OpenPacket->Override= TRUE;
            }

            FileString.Length = 8;
            FileString.MaximumLength = 8;
            FileString.Buffer = L"File";

            /* Do Audit/Alarm for open operation */
            SeOpenObjectAuditAlarm(&FileString,
                                   OriginalDeviceObject,
                                   CompleteName,
                                   OriginalDeviceObject->SecurityDescriptor,
                                   AccessState,
                                   FALSE,
                                   AccessGranted,
                                   UserMode,
                                   &AccessState->GenerateOnClose);
        }
        else
        {
            /* Check if we need to do traverse validation */
            if (!(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE) ||
                ((OriginalDeviceObject->DeviceType == FILE_DEVICE_DISK) ||
                 (OriginalDeviceObject->DeviceType == FILE_DEVICE_CD_ROM)))
            {
                /* Check if this is a restricted token */
                if (!(AccessState->Flags & TOKEN_IS_RESTRICTED))
                {
                    /* Do the FAST traverse check */
                    AccessGranted = SeFastTraverseCheck(OriginalDeviceObject->SecurityDescriptor,
                                                        AccessState,
                                                        FILE_TRAVERSE,
                                                        UserMode);
                }
                else
                {
                    /* Fail */
                    AccessGranted = FALSE;
                }

                /* Check if we failed to get access */
                if (!AccessGranted)
                {
                    /* Lock the subject context */
                    SeLockSubjectContext(&AccessState->SubjectSecurityContext);
                    LockHeld = TRUE;

                    /* Do access check */
                    AccessGranted = SeAccessCheck(OriginalDeviceObject->
                                                  SecurityDescriptor,
                                                  &AccessState->SubjectSecurityContext,
                                                  LockHeld,
                                                  FILE_TRAVERSE,
                                                  0,
                                                  &Privileges,
                                                  &IoFileObjectType->
                                                  TypeInfo.GenericMapping,
                                                  UserMode,
                                                  &GrantedAccess,
                                                  &Status);
                    if (Privileges)
                    {
                        /* Append and free the privileges */
                        SeAppendPrivileges(AccessState, Privileges);
                        SeFreePrivileges(Privileges);
                    }
                }

                /* FIXME: Do Audit/Alarm for traverse check */
            }
            else
            {
                /* Access automatically granted */
                AccessGranted = TRUE;
            }
        }

        /* Check if we hold the lock */
        if (LockHeld)
        {
            /* Release it */
            SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
        }

        /* Check if access failed */
        if (!AccessGranted)
        {
            /* Dereference the device and fail */
            DPRINT1("Traverse access failed!\n");
            IopDereferenceDeviceObject(OriginalDeviceObject, FALSE);
            return STATUS_ACCESS_DENIED;
        }
    }

    /* Check if we can simply use a dummy file */
    UseDummyFile = ((OpenPacket->QueryOnly) || (OpenPacket->DeleteOnly));

    /* Check if this is a direct open */
    if (!(RemainingName->Length) &&
        !(OpenPacket->RelatedFileObject) &&
        ((DesiredAccess & ~(SYNCHRONIZE |
                            FILE_READ_ATTRIBUTES |
                            READ_CONTROL |
                            ACCESS_SYSTEM_SECURITY |
                            WRITE_OWNER |
                            WRITE_DAC)) == 0) &&
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
        DeviceObject = ParseObject;

        /* Check if the related FO had a VPB */
        if (OpenPacket->RelatedFileObject->Vpb)
        {
            /* Yes, remember it */
            Vpb = OpenPacket->RelatedFileObject->Vpb;

            /* Reference it */
            InterlockedIncrement((PLONG)&Vpb->ReferenceCount);
        }
    }
    else
    {
        /* The device object is the one we were given */
        DeviceObject = OriginalDeviceObject;

        /* Check if it has a VPB */
        if ((OriginalDeviceObject->Vpb) && !(DirectOpen))
        {
            /* Check if the VPB is mounted, and mount it */
            Vpb = IopCheckVpbMounted(OpenPacket,
                                     OriginalDeviceObject,
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

    /* Check if this is a secure FSD */
    if ((DeviceObject->Characteristics & FILE_DEVICE_SECURE_OPEN) &&
        ((OpenPacket->RelatedFileObject) || (RemainingName->Length)) &&
        (!VolumeOpen))
    {
        DPRINT("Fix Secure FSD support!!!\n");
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp)
    {
        /* Dereference the device and VPB, then fail */
        IopDereferenceDeviceObject(OriginalDeviceObject, FALSE);
        if (Vpb) IopDereferenceVpbAndFree(Vpb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Now set the IRP data */
    Irp->RequestorMode = AccessMode;
    Irp->Flags = IRP_CREATE_OPERATION | IRP_SYNCHRONOUS_API | IRP_DEFER_IO_COMPLETION;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->UserIosb = &IoStatusBlock;
    Irp->MdlAddress = NULL;
    Irp->PendingReturned = FALSE;
    Irp->UserEvent = NULL;
    Irp->Cancel = FALSE;
    Irp->CancelRoutine = NULL;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    /* Setup the security context */
    SecurityContext.SecurityQos = SecurityQos;
    SecurityContext.AccessState = AccessState;
    SecurityContext.DesiredAccess = AccessState->RemainingDesiredAccess;
    SecurityContext.FullCreateOptions = OpenPacket->CreateOptions;

    /* Get the I/O Stack location */
    StackLoc = (PEXTENDED_IO_STACK_LOCATION)IoGetNextIrpStackLocation(Irp);
    StackLoc->Control = 0;

    /* Check what kind of file this is */
    switch (OpenPacket->CreateFileType)
    {
        /* Normal file */
        case CreateFileTypeNone:

            /* Set the major function and EA Length */
            StackLoc->MajorFunction = IRP_MJ_CREATE;
            StackLoc->Parameters.Create.EaLength = OpenPacket->EaLength;

            /* Set the flags */
            StackLoc->Flags = (UCHAR)OpenPacket->Options;
            StackLoc->Flags |= !(Attributes & OBJ_CASE_INSENSITIVE) ? SL_CASE_SENSITIVE: 0;
            break;

        /* Named pipe */
        case CreateFileTypeNamedPipe:

            /* Set the named pipe MJ and set the parameters */
            StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
            StackLoc->Parameters.CreatePipe.Parameters = OpenPacket->ExtraCreateParameters;
            break;

        /* Mailslot */
        case CreateFileTypeMailslot:

            /* Set the mailslot MJ and set the parameters */
            StackLoc->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
            StackLoc->Parameters.CreateMailslot.Parameters = OpenPacket->ExtraCreateParameters;
            break;
    }

    /* Set the common data */
    Irp->Overlay.AllocationSize = OpenPacket->AllocationSize;
    Irp->AssociatedIrp.SystemBuffer = OpenPacket->EaBuffer;
    StackLoc->Parameters.Create.Options = (OpenPacket->Disposition << 24) |
                                          (OpenPacket->CreateOptions &
                                           0xFFFFFF);
    StackLoc->Parameters.Create.FileAttributes = OpenPacket->FileAttributes;
    StackLoc->Parameters.Create.ShareAccess = OpenPacket->ShareAccess;
    StackLoc->Parameters.Create.SecurityContext = &SecurityContext;

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
        if (!NT_SUCCESS(Status))
        {
            /* Create failed, free the IRP */
            IoFreeIrp(Irp);

            /* Dereference the device and VPB */
            IopDereferenceDeviceObject(OriginalDeviceObject, FALSE);
            if (Vpb) IopDereferenceVpbAndFree(Vpb);

            /* We failed, return status */
            OpenPacket->FinalStatus = Status;
            return Status;
        }

        /* Clear the file object */
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

        /* Check if this is synch I/O */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            /* Initialize the event */
            KeInitializeEvent(&FileObject->Lock, SynchronizationEvent, FALSE);
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
        LocalFileObject = OpenPacket->LocalFileObject;
        RtlZeroMemory(LocalFileObject, sizeof(DUMMY_FILE_OBJECT));

        /* Set it up */
        FileObject = (PFILE_OBJECT)&LocalFileObject->ObjectHeader.Body;
        LocalFileObject->ObjectHeader.Type = IoFileObjectType;
        LocalFileObject->ObjectHeader.PointerCount = 1;
    }

    /* Setup the file header */
    FileObject->Type = IO_TYPE_FILE;
    FileObject->Size = sizeof(FILE_OBJECT);
    FileObject->RelatedFileObject = OpenPacket->RelatedFileObject;
    FileObject->DeviceObject = OriginalDeviceObject;

    /* Check if this is a direct device open */
    if (DirectOpen) FileObject->Flags |= FO_DIRECT_DEVICE_OPEN;

    /* Check if the caller wants case sensitivity */
    if (!(Attributes & OBJ_CASE_INSENSITIVE))
    {
        /* Tell the driver about it */
        FileObject->Flags |= FO_OPENED_CASE_SENSITIVE;
    }

    /* Now set the file object */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    StackLoc->FileObject = FileObject;

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
            IopDereferenceDeviceObject(OriginalDeviceObject, FALSE);
            if (Vpb) IopDereferenceVpbAndFree(Vpb);

            /* Clear the FO and dereference it */
            FileObject->DeviceObject = NULL;
            if (!UseDummyFile) ObDereferenceObject(FileObject);

            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Copy the name */
    RtlCopyUnicodeString(&FileObject->FileName, RemainingName);

    /* Initialize the File Object event and set the FO */
    KeInitializeEvent(&FileObject->Event, NotificationEvent, FALSE);
    OpenPacket->FileObject = FileObject;

    /* Queue the IRP and call the driver */
    IopQueueIrpToThread(Irp);
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
        ASSERT(!Irp->MdlAddress);

        /* Completion happens at APC_LEVEL */
        KeRaiseIrql(APC_LEVEL, &OldIrql);

        /* Get the new I/O Status block ourselves */
        IoStatusBlock = Irp->IoStatus;
        Status = IoStatusBlock.Status;

        /* Manually signal the even, we can't have any waiters */
        FileObject->Event.Header.SignalState = 1;

        /* Now that we've signaled the events, de-associate the IRP */
        IopUnQueueIrpFromThread(Irp);

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
            ExFreePoolWithTag(FileObject->FileName.Buffer, TAG_IO_NAME);
            FileObject->FileName.Length = 0;
        }

        /* Clear its device object */
        FileObject->DeviceObject = NULL;

        /* Save this now because the FO might go away */
        OpenCancelled = FileObject->Flags & FO_FILE_OPEN_CANCELLED ?
                        TRUE : FALSE;

        /* Clear the file object in the open packet */
        OpenPacket->FileObject = NULL;

        /* Dereference the file object */
        if (!UseDummyFile) ObDereferenceObject(FileObject);

        /* Dereference the device object */
        IopDereferenceDeviceObject(OriginalDeviceObject, FALSE);

        /* Unless the driver cancelled the open, dereference the VPB */
        if (!(OpenCancelled) && (Vpb)) IopDereferenceVpbAndFree(Vpb);

        /* Set the status and return */
        OpenPacket->FinalStatus = Status;
        return Status;
    }
    else if (Status == STATUS_REPARSE)
    {
        /* FIXME: We don't handle this at all! */
        ASSERT(FALSE);
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
        if (Vpb) IopDereferenceVpbAndFree(Vpb);

        /* And re-associate with the actual one */
        Vpb = FileObject->Vpb;
        if (Vpb) InterlockedIncrement((PLONG)&Vpb->ReferenceCount);
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
                    ExFreePoolWithTag(FileBasicInfo, TAG_IO);
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
    BOOLEAN DereferenceDone = FALSE;
    PVPB Vpb;
    KIRQL OldIrql;
    IOTRACE(IO_FILE_DEBUG, "ObjectBody: %p\n", ObjectBody);

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

        /* Sanity check */
        ASSERT(!(FileObject->Flags & FO_SYNCHRONOUS_IO) ||
               (InterlockedExchange((PLONG)&FileObject->Busy, TRUE) == FALSE));

        /* Check if the handle wasn't created yet */
        if (!(FileObject->Flags & FO_HANDLE_CREATED))
        {
            /* Send the cleanup IRP */
            IopCloseFile(NULL, ObjectBody, 0, 1, 1);
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
        StackPtr->FileObject = FileObject;

        /* Queue the IRP */
        IopQueueIrpToThread(Irp);

        /* Get the VPB and check if this isn't a direct open */
        Vpb = FileObject->Vpb;
        if ((Vpb) && !(FileObject->Flags & FO_DIRECT_DEVICE_OPEN))
        {
            /* Dereference the VPB before the close */
            InterlockedDecrement((PLONG)&Vpb->ReferenceCount);
        }

        /* Check if the FS will never disappear by itself */
        if (FileObject->DeviceObject->Flags & DO_NEVER_LAST_DEVICE)
        {
            /* Dereference it */
            InterlockedDecrement(&FileObject->DeviceObject->ReferenceCount);
            DereferenceDone = TRUE;
        }

        /* Call the FS Driver */
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }

        /* De-queue the IRP */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        IopUnQueueIrpFromThread(Irp);
        KeLowerIrql(OldIrql);

        /* Free the IRP */
        IoFreeIrp(Irp);

        /* Clear the file name */
        if (FileObject->FileName.Buffer)
        {
           ExFreePoolWithTag(FileObject->FileName.Buffer, TAG_IO_NAME);
           FileObject->FileName.Buffer = NULL;
        }

        /* Check if the FO had a completion port */
        if (FileObject->CompletionContext)
        {
            /* Free it */
            ObDereferenceObject(FileObject->CompletionContext->Port);
            ExFreePool(FileObject->CompletionContext);
        }

        /* Check if the FO had extension */
        if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
        {
            /* Release filter context structure if any */
            FsRtlPTeardownPerFileObjectContexts(FileObject);
        }

        /* Check if dereference has been done yet */
        if (!DereferenceDone)
        {
            /* Dereference device object */
            IopDereferenceDeviceObject(FileObject->DeviceObject, FALSE);
        }
    }
}

NTSTATUS
NTAPI
IopSecurityFile(IN PVOID ObjectBody,
                IN SECURITY_OPERATION_CODE OperationCode,
                IN PSECURITY_INFORMATION SecurityInformation,
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
    IOTRACE(IO_FILE_DEBUG, "ObjectBody: %p\n", ObjectBody);

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
    if (!(FileObject) ||
        (!(FileObject->FileName.Length) && !(FileObject->RelatedFileObject)) ||
        (FileObject->Flags & FO_DIRECT_DEVICE_OPEN))
    {
        /* Check what kind of request this was */
        if (OperationCode == QuerySecurityDescriptor)
        {
            return SeQuerySecurityDescriptorInfo(SecurityInformation,
                                                 SecurityDescriptor,
                                                 BufferLength,
                                                 &DeviceObject->SecurityDescriptor);
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
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, NULL);

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
            *SecurityInformation;
        StackPtr->Parameters.QuerySecurity.Length = *BufferLength;
        Irp->UserBuffer = SecurityDescriptor;
    }
    else
    {
        /* Set the major function and parameters for a set */
        StackPtr->MajorFunction = IRP_MJ_SET_SECURITY;
        StackPtr->Parameters.SetSecurity.SecurityInformation =
            *SecurityInformation;
        StackPtr->Parameters.SetSecurity.SecurityDescriptor =
            SecurityDescriptor;
    }

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);

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
            /* Wait on the file object */
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
            Status = SeSetWorldSecurityDescriptor(*SecurityInformation,
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

        _SEH2_TRY
        {
            /* Return length */
            *BufferLength = (ULONG)IoStatusBlock.Information;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
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
    BOOLEAN LengthMismatch = FALSE;
    NTSTATUS Status;
    PWCHAR p;
    IOTRACE(IO_FILE_DEBUG, "ObjectBody: %p\n", ObjectBody);

    /* Validate length */
    if (Length < sizeof(OBJECT_NAME_INFORMATION))
    {
        /* Wrong length, fail */
        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);
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
    if (!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
    {
        /* Free the buffer and fail */
        ExFreePoolWithTag(LocalInfo, TAG_IO);
        return Status;
    }

    /* Copy the information */
    RtlCopyMemory(ObjectNameInfo,
                  LocalInfo,
                  (LocalReturnLength > Length) ?
                  Length : LocalReturnLength);

    /* Set buffer pointer */
    p = (PWCHAR)(ObjectNameInfo + 1);
    ObjectNameInfo->Name.Buffer = p;

    /* Advance in buffer */
    p += (LocalInfo->Name.Length / sizeof(WCHAR));

    /* Check if this already filled our buffer */
    if (LocalReturnLength > Length)
    {
        /* Set the length mismatch to true, so that we can return
         * the proper buffer size to the caller later
         */
        LengthMismatch = TRUE;

        /* Save the initial buffer length value */
        *ReturnLength = LocalReturnLength;
    }

    /* Now get the file name buffer and check the length needed */
    LocalFileInfo = (PFILE_NAME_INFORMATION)LocalInfo;
    FileLength = Length -
                 LocalReturnLength +
                 FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);

    /* Query the File name */
    Status = IoQueryFileInformation(FileObject,
                                    FileNameInformation,
                                    LengthMismatch ? Length : FileLength,
                                    LocalFileInfo,
                                    &LocalReturnLength);
    if (NT_ERROR(Status))
    {
        /* Fail on errors only, allow warnings */
        ExFreePoolWithTag(LocalInfo, TAG_IO);
        return Status;
    }

    /* If the provided buffer is too small, return the required size */
    if (LengthMismatch)
    {
        /* Add the required length */
        *ReturnLength += LocalFileInfo->FileNameLength;

        /* Free the allocated buffer and return failure */
        ExFreePoolWithTag(LocalInfo, TAG_IO);
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Now calculate the new lengths left */
    FileLength = LocalReturnLength -
                 FIELD_OFFSET(FILE_NAME_INFORMATION, FileName);
    LocalReturnLength = (ULONG)((ULONG_PTR)p -
                                (ULONG_PTR)ObjectNameInfo +
                                LocalFileInfo->FileNameLength);

    /* Write the Name and null-terminate it */
    RtlCopyMemory(p, LocalFileInfo->FileName, FileLength);
    p += (FileLength / sizeof(WCHAR));
    *p = UNICODE_NULL;
    LocalReturnLength += sizeof(UNICODE_NULL);

    /* Return the length needed */
    *ReturnLength = LocalReturnLength;

    /* Setup the length and maximum length */
    FileLength = (ULONG)((ULONG_PTR)p - (ULONG_PTR)ObjectNameInfo);
    ObjectNameInfo->Name.Length = (USHORT)FileLength -
                                          sizeof(OBJECT_NAME_INFORMATION);
    ObjectNameInfo->Name.MaximumLength = (USHORT)ObjectNameInfo->Name.Length +
                                                 sizeof(UNICODE_NULL);

    /* Free buffer and return */
    ExFreePoolWithTag(LocalInfo, TAG_IO);
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
    KIRQL OldIrql;
    IOTRACE(IO_FILE_DEBUG, "ObjectBody: %p\n", ObjectBody);

    /* If this isn't the last handle for the current process, quit */
    if (HandleCount != 1) return;

    /* Check if the file is locked and has more then one handle opened */
    if ((FileObject->LockOperation) && (SystemHandleCount != 1))
    {
        DPRINT1("We need to unlock this file!\n");
        ASSERT(FALSE);
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
    IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Call the FS Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, UserRequest, KernelMode, FALSE, NULL);
    }

    /* Unqueue the IRP */
    KeRaiseIrql(APC_LEVEL, &OldIrql);
    IopUnQueueIrpFromThread(Irp);
    KeLowerIrql(OldIrql);

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
    NTSTATUS Status;
    KPROCESSOR_MODE AccessMode = ExGetPreviousMode();
    DUMMY_FILE_OBJECT LocalFileObject;
    FILE_NETWORK_OPEN_INFORMATION NetworkOpenInfo;
    HANDLE Handle;
    OPEN_PACKET OpenPacket;
    BOOLEAN IsBasic;
    PAGED_CODE();
    IOTRACE(IO_FILE_DEBUG, "Class: %lx\n", FileInformationClass);

    /* Check if the caller was user mode */
    if (AccessMode != KernelMode)
    {
        /* Protect probe in SEH */
        _SEH2_TRY
        {
            /* Probe the buffer */
            ProbeForWrite(FileInformation, FileInformationSize, sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Check if this is a basic or full request */
    IsBasic = (FileInformationSize == sizeof(FILE_BASIC_INFORMATION));

    /* Setup the Open Packet */
    RtlZeroMemory(&OpenPacket, sizeof(OPEN_PACKET));
    OpenPacket.Type = IO_TYPE_OPEN_PACKET;
    OpenPacket.Size = sizeof(OPEN_PACKET);
    OpenPacket.CreateOptions = FILE_OPEN_REPARSE_POINT;
    OpenPacket.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    OpenPacket.Disposition = FILE_OPEN;
    OpenPacket.BasicInformation = IsBasic ? FileInformation : NULL;
    OpenPacket.NetworkInformation = IsBasic ? &NetworkOpenInfo :
                                    (AccessMode != KernelMode) ?
                                    &NetworkOpenInfo : FileInformation;
    OpenPacket.QueryOnly = TRUE;
    OpenPacket.FullAttributes = IsBasic ? FALSE : TRUE;
    OpenPacket.LocalFileObject = &LocalFileObject;

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
        DPRINT("IopQueryAttributesFile failed for '%wZ' with 0x%lx\n",
               ObjectAttributes->ObjectName, Status);
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
        _SEH2_TRY
        {
            /* Copy the buffer back */
            RtlCopyMemory(FileInformation,
                          &NetworkOpenInfo,
                          FileInformationSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

PVOID
NTAPI
IoGetFileObjectFilterContext(IN PFILE_OBJECT FileObject)
{
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        UNIMPLEMENTED;
        /* FIXME: return NULL for the moment ~ */
        return NULL;
    }

    return NULL;
}

NTSTATUS
NTAPI
IoChangeFileObjectFilterContext(IN PFILE_OBJECT FileObject,
                                IN PVOID FilterContext,
                                IN BOOLEAN Define)
{
    if (!(FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION))
    {
        return STATUS_INVALID_PARAMETER;
    }

    UNIMPLEMENTED;

    return STATUS_NOT_IMPLEMENTED;
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
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
NTAPI
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
             IN PLARGE_INTEGER AllocationSize OPTIONAL,
             IN ULONG FileAttributes,
             IN ULONG ShareAccess,
             IN ULONG Disposition,
             IN ULONG CreateOptions,
             IN PVOID EaBuffer OPTIONAL,
             IN ULONG EaLength,
             IN CREATE_FILE_TYPE CreateFileType,
             IN PVOID ExtraCreateParameters OPTIONAL,
             IN ULONG Options)
{
    KPROCESSOR_MODE AccessMode;
    HANDLE LocalHandle = 0;
    LARGE_INTEGER SafeAllocationSize;
    NTSTATUS Status = STATUS_SUCCESS;
    PNAMED_PIPE_CREATE_PARAMETERS NamedPipeCreateParameters;
    POPEN_PACKET OpenPacket;
    ULONG EaErrorOffset;
    PAGED_CODE();
    IOTRACE(IO_FILE_DEBUG, "FileName: %wZ\n", ObjectAttributes->ObjectName);


    /* Check if we have no parameter checking to do */
    if (Options & IO_NO_PARAMETER_CHECKING)
    {
        /* Then force kernel-mode access to avoid checks */
        AccessMode = KernelMode;
    }
    else
    {
        /* Otherwise, use the actual mode */
        AccessMode = ExGetPreviousMode();
    }

    /* Check if we need to do parameter checking */
    if ((AccessMode != KernelMode) || (Options & IO_CHECK_CREATE_PARAMETERS))
    {
        /* Validate parameters */
        if (FileAttributes & ~FILE_ATTRIBUTE_VALID_FLAGS)
        {
            DPRINT1("File Create 'FileAttributes' Parameter contains invalid flags!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (ShareAccess & ~FILE_SHARE_VALID_FLAGS)
        {
            DPRINT1("File Create 'ShareAccess' Parameter contains invalid flags!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (Disposition > FILE_MAXIMUM_DISPOSITION)
        {
            DPRINT1("File Create 'Disposition' Parameter is out of range!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (CreateOptions & ~FILE_VALID_OPTION_FLAGS)
        {
            DPRINT1("File Create 'CreateOptions' parameter contains invalid flags!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) &&
            (!(DesiredAccess & SYNCHRONIZE)))
        {
            DPRINT1("File Create 'CreateOptions' parameter FILE_SYNCHRONOUS_IO_* requested, but 'DesiredAccess' does not have SYNCHRONIZE!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & FILE_DELETE_ON_CLOSE) && (!(DesiredAccess & DELETE)))
        {
            DPRINT1("File Create 'CreateOptions' parameter FILE_DELETE_ON_CLOSE requested, but 'DesiredAccess' does not have DELETE!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & (FILE_SYNCHRONOUS_IO_NONALERT | FILE_SYNCHRONOUS_IO_ALERT)) ==
            (FILE_SYNCHRONOUS_IO_NONALERT | FILE_SYNCHRONOUS_IO_ALERT))
        {
            DPRINT1("File Create 'FileAttributes' parameter both FILE_SYNCHRONOUS_IO_NONALERT and FILE_SYNCHRONOUS_IO_ALERT specified!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & FILE_DIRECTORY_FILE) && !(CreateOptions & FILE_NON_DIRECTORY_FILE) &&
            (CreateOptions & ~(FILE_DIRECTORY_FILE |
                               FILE_SYNCHRONOUS_IO_ALERT |
                               FILE_SYNCHRONOUS_IO_NONALERT |
                               FILE_WRITE_THROUGH |
                               FILE_COMPLETE_IF_OPLOCKED |
                               FILE_OPEN_FOR_BACKUP_INTENT |
                               FILE_DELETE_ON_CLOSE |
                               FILE_OPEN_FOR_FREE_SPACE_QUERY |
                               FILE_OPEN_BY_FILE_ID |
                               FILE_NO_COMPRESSION |
                               FILE_OPEN_REPARSE_POINT)))
        {
            DPRINT1("File Create 'CreateOptions' Parameter has flags incompatible with FILE_DIRECTORY_FILE!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & FILE_DIRECTORY_FILE) && !(CreateOptions & FILE_NON_DIRECTORY_FILE) &&
            (Disposition != FILE_CREATE) && (Disposition != FILE_OPEN) && (Disposition != FILE_OPEN_IF))
        {
            DPRINT1("File Create 'CreateOptions' Parameter FILE_DIRECTORY_FILE requested, but 'Disposition' is not FILE_CREATE/FILE_OPEN/FILE_OPEN_IF!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & FILE_COMPLETE_IF_OPLOCKED) && (CreateOptions & FILE_RESERVE_OPFILTER))
        {
            DPRINT1("File Create 'CreateOptions' Parameter both FILE_COMPLETE_IF_OPLOCKED and FILE_RESERVE_OPFILTER specified!\n");
            return STATUS_INVALID_PARAMETER;
        }

        if ((CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING) && (DesiredAccess & FILE_APPEND_DATA))
        {
            DPRINT1("File Create 'CreateOptions' parameter FILE_NO_INTERMEDIATE_BUFFERING requested, but 'DesiredAccess' FILE_APPEND_DATA requires it!\n");
            return STATUS_INVALID_PARAMETER;
        }

        /* Now check if this is a named pipe */
        if (CreateFileType == CreateFileTypeNamedPipe)
        {
            /* Make sure we have extra parameters */
            if (!ExtraCreateParameters)
            {
                DPRINT1("Invalid parameter: ExtraCreateParameters == 0!\n");
                return STATUS_INVALID_PARAMETER;
            }

            /* Get the parameters and validate them */
            NamedPipeCreateParameters = ExtraCreateParameters;
            if ((NamedPipeCreateParameters->NamedPipeType > FILE_PIPE_MESSAGE_TYPE) ||
                (NamedPipeCreateParameters->ReadMode > FILE_PIPE_MESSAGE_MODE) ||
                (NamedPipeCreateParameters->CompletionMode > FILE_PIPE_COMPLETE_OPERATION) ||
                (ShareAccess & FILE_SHARE_DELETE) ||
                ((Disposition < FILE_OPEN) || (Disposition > FILE_OPEN_IF)) ||
                (CreateOptions & ~FILE_VALID_PIPE_OPTION_FLAGS))
            {
                /* Invalid named pipe create */
                DPRINT1("Invalid named pipe create\n");
                return STATUS_INVALID_PARAMETER;
            }
        }
        else if (CreateFileType == CreateFileTypeMailslot)
        {
            /* Make sure we have extra parameters */
            if (!ExtraCreateParameters)
            {
                DPRINT1("Invalid parameter: ExtraCreateParameters == 0!\n");
                return STATUS_INVALID_PARAMETER;
            }

            /* Get the parameters and validate them */
            if ((ShareAccess & FILE_SHARE_DELETE) ||
                !(ShareAccess & ~FILE_SHARE_WRITE) ||
                (Disposition != FILE_CREATE) ||
                (CreateOptions & ~FILE_VALID_MAILSLOT_OPTION_FLAGS))
            {
                /* Invalid mailslot create */
                DPRINT1("Invalid mailslot create\n");
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    /* Allocate the open packet */
    OpenPacket = ExAllocatePoolWithTag(NonPagedPool, sizeof(*OpenPacket), 'pOoI');
    if (!OpenPacket) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(OpenPacket, sizeof(*OpenPacket));

    /* Check if the call came from user mode */
    if (AccessMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the output parameters */
            ProbeForWriteHandle(FileHandle);
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the allocation size if one was passed in */
            if (AllocationSize)
            {
                SafeAllocationSize = ProbeForReadLargeInteger(AllocationSize);
            }
            else
            {
                SafeAllocationSize.QuadPart = 0;
            }

            /* Make sure it's valid */
            if (SafeAllocationSize.QuadPart < 0)
            {
                RtlRaiseStatus(STATUS_INVALID_PARAMETER);
            }

            /* Check if EA was passed in */
            if ((EaBuffer) && (EaLength))
            {
                /* Probe it */
                ProbeForRead(EaBuffer, EaLength, sizeof(ULONG));

                /* And marshall it */
                OpenPacket->EaBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                             EaLength,
                                                             TAG_EA);
                OpenPacket->EaLength = EaLength;
                RtlCopyMemory(OpenPacket->EaBuffer, EaBuffer, EaLength);

                /* Validate the buffer */
                Status = IoCheckEaBufferValidity(OpenPacket->EaBuffer,
                                                 EaLength,
                                                 &EaErrorOffset);
                if (!NT_SUCCESS(Status))
                {
                    /* Undo everything if it's invalid */
                    DPRINT1("Invalid EA buffer\n");
                    IoStatusBlock->Status = Status;
                    IoStatusBlock->Information = EaErrorOffset;
                    RtlRaiseStatus(Status);
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            if (OpenPacket->EaBuffer != NULL) ExFreePool(OpenPacket->EaBuffer);
            ExFreePool(OpenPacket);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Check if this is a device attach */
        if (CreateOptions & IO_ATTACH_DEVICE_API)
        {
            /* Set the flag properly */
            Options |= IO_ATTACH_DEVICE;
            CreateOptions &= ~IO_ATTACH_DEVICE_API;
        }

        /* Check if we have allocation size */
        if (AllocationSize)
        {
            /* Capture it */
            SafeAllocationSize = *AllocationSize;
        }
        else
        {
            /* Otherwise, no size */
            SafeAllocationSize.QuadPart = 0;
        }

        /* Check if we have an EA packet */
        if ((EaBuffer) && (EaLength))
        {
            /* Allocate the kernel copy */
            OpenPacket->EaBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                         EaLength,
                                                         TAG_EA);
            if (!OpenPacket->EaBuffer)
            {
                ExFreePool(OpenPacket);
                DPRINT1("Failed to allocate open packet EA buffer\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Copy the data */
            OpenPacket->EaLength = EaLength;
            RtlCopyMemory(OpenPacket->EaBuffer, EaBuffer, EaLength);

            /* Validate the buffer */
            Status = IoCheckEaBufferValidity(OpenPacket->EaBuffer,
                                             EaLength,
                                             &EaErrorOffset);
            if (!NT_SUCCESS(Status))
            {
                /* Undo everything if it's invalid */
                DPRINT1("Invalid EA buffer\n");
                ExFreePool(OpenPacket->EaBuffer);
                IoStatusBlock->Status = Status;
                IoStatusBlock->Information = EaErrorOffset;
                ExFreePool(OpenPacket);
                return Status;
            }
        }
    }

    /* Setup the Open Packet */
    OpenPacket->Type = IO_TYPE_OPEN_PACKET;
    OpenPacket->Size = sizeof(*OpenPacket);
    OpenPacket->AllocationSize = SafeAllocationSize;
    OpenPacket->CreateOptions = CreateOptions;
    OpenPacket->FileAttributes = (USHORT)FileAttributes;
    OpenPacket->ShareAccess = (USHORT)ShareAccess;
    OpenPacket->Options = Options;
    OpenPacket->Disposition = Disposition;
    OpenPacket->CreateFileType = CreateFileType;
    OpenPacket->ExtraCreateParameters = ExtraCreateParameters;

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
                                OpenPacket,
                                &LocalHandle);

    /* Free the EA Buffer */
    if (OpenPacket->EaBuffer) ExFreePool(OpenPacket->EaBuffer);

    /* Now check for Ob or Io failure */
    if (!(NT_SUCCESS(Status)) || (OpenPacket->ParseCheck != TRUE))
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
        if (!NT_SUCCESS(OpenPacket->FinalStatus))
        {
            /* Use this status instead of Ob's */
            Status = OpenPacket->FinalStatus;

            /* Check if it was only a warning */
            if (NT_WARNING(Status))
            {
                /* Protect write with SEH */
                _SEH2_TRY
                {
                    /* In this case, we copy the I/O Status back */
                    IoStatusBlock->Information = OpenPacket->Information;
                    IoStatusBlock->Status = OpenPacket->FinalStatus;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get exception code */
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }
        else if ((OpenPacket->FileObject) && (OpenPacket->ParseCheck != 1))
        {
            /*
             * This can happen in the very bizarre case where the parse routine
             * actually executed more then once (due to a reparse) and ended
             * up failing after already having created the File Object.
             */
            if (OpenPacket->FileObject->FileName.Length)
            {
                /* It had a name, free it */
                ExFreePoolWithTag(OpenPacket->FileObject->FileName.Buffer, TAG_IO_NAME);
            }

            /* Clear the device object to invalidate the FO, and dereference */
            OpenPacket->FileObject->DeviceObject = NULL;
            ObDereferenceObject(OpenPacket->FileObject);
        }
    }
    else
    {
        /* We reached success and have a valid file handle */
        OpenPacket->FileObject->Flags |= FO_HANDLE_CREATED;
        ASSERT(OpenPacket->FileObject->Type == IO_TYPE_FILE);

        /* Enter SEH for write back */
        _SEH2_TRY
        {
            /* Write back the handle and I/O Status */
            *FileHandle = LocalHandle;
            IoStatusBlock->Information = OpenPacket->Information;
            IoStatusBlock->Status = OpenPacket->FinalStatus;

            /* Get the Io status */
            Status = OpenPacket->FinalStatus;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception status */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Check if we were 100% successful */
    if ((OpenPacket->ParseCheck == TRUE) && (OpenPacket->FileObject))
    {
        /* Dereference the File Object */
        ObDereferenceObject(OpenPacket->FileObject);
    }

    /* Return status */
    ExFreePool(OpenPacket);
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
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
IoCreateStreamFileObjectEx(IN PFILE_OBJECT FileObject OPTIONAL,
                           IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                           OUT PHANDLE FileObjectHandle OPTIONAL)
{
    PFILE_OBJECT CreatedFileObject;
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PAGED_CODE();
    IOTRACE(IO_FILE_DEBUG, "FileObject: %p\n", FileObject);

    /* Choose Device Object */
    if (FileObject) DeviceObject = FileObject->DeviceObject;

    /* Reference the device object and initialize attributes */
    InterlockedIncrement(&DeviceObject->ReferenceCount);
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    /* Create the File Object */
    Status = ObCreateObject(KernelMode,
                            IoFileObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(FILE_OBJECT),
                            sizeof(FILE_OBJECT),
                            0,
                            (PVOID*)&CreatedFileObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        IopDereferenceDeviceObject(DeviceObject, FALSE);
        ExRaiseStatus(Status);
    }

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
    if (!NT_SUCCESS(Status)) ExRaiseStatus(Status);

    /* Set the handle created flag */
    CreatedFileObject->Flags |= FO_HANDLE_CREATED;
    ASSERT(CreatedFileObject->Type == IO_TYPE_FILE);

    /* Check if we have a VPB */
    if (DeviceObject->Vpb)
    {
        /* Reference it */
         InterlockedIncrement((PLONG)&DeviceObject->Vpb->ReferenceCount);
    }

    /* Check if the caller wants the handle */
    if (FileObjectHandle)
    {
        /* Return it */
        *FileObjectHandle = FileHandle;
        ObDereferenceObject(CreatedFileObject);
    }
    else
    {
        /* Otherwise, close it */
        ObCloseHandle(FileHandle, KernelMode);
    }

    /* Return the file object */
    return CreatedFileObject;
}

/*
 * @implemented
 */
PFILE_OBJECT
NTAPI
IoCreateStreamFileObject(IN PFILE_OBJECT FileObject,
                         IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the newer function */
    return IoCreateStreamFileObjectEx(FileObject, DeviceObject, NULL);
}

/*
 * @implemented
 */
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectLite(IN PFILE_OBJECT FileObject OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL)
{
    PFILE_OBJECT CreatedFileObject;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PAGED_CODE();
    IOTRACE(IO_FILE_DEBUG, "FileObject: %p\n", FileObject);

    /* Choose Device Object */
    if (FileObject) DeviceObject = FileObject->DeviceObject;

    /* Reference the device object and initialize attributes */
    InterlockedIncrement(&DeviceObject->ReferenceCount);
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    /* Create the File Object */
    Status = ObCreateObject(KernelMode,
                            IoFileObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(FILE_OBJECT),
                            sizeof(FILE_OBJECT),
                            0,
                            (PVOID*)&CreatedFileObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        IopDereferenceDeviceObject(DeviceObject, FALSE);
        ExRaiseStatus(Status);
    }

    /* Set File Object Data */
    RtlZeroMemory(CreatedFileObject, sizeof(FILE_OBJECT));
    CreatedFileObject->DeviceObject = DeviceObject;
    CreatedFileObject->Type = IO_TYPE_FILE;
    CreatedFileObject->Size = sizeof(FILE_OBJECT);
    CreatedFileObject->Flags = FO_STREAM_FILE;

    /* Initialize the wait event */
    KeInitializeEvent(&CreatedFileObject->Event, SynchronizationEvent, FALSE);

    /* Destroy create information */
    ObFreeObjectCreateInfoBuffer(OBJECT_TO_OBJECT_HEADER(CreatedFileObject)->
                                 ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(CreatedFileObject)->ObjectCreateInfo = NULL;

    /* Set the handle created flag */
    CreatedFileObject->Flags |= FO_HANDLE_CREATED;
    ASSERT(CreatedFileObject->Type == IO_TYPE_FILE);

    /* Check if we have a VPB */
    if (DeviceObject->Vpb)
    {
        /* Reference it */
         InterlockedIncrement((PLONG)&DeviceObject->Vpb->ReferenceCount);
    }

    /* Return the file object */
    return CreatedFileObject;
}

/*
 * @implemented
 */
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(VOID)
{
    /* Return the mapping */
    return &IopFileMapping;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoIsFileOriginRemote(IN PFILE_OBJECT FileObject)
{
    /* Return the flag status */
    return FileObject->Flags & FO_REMOTE_ORIGIN ? TRUE : FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoFastQueryNetworkAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
                             IN ACCESS_MASK DesiredAccess,
                             IN ULONG OpenOptions,
                             OUT PIO_STATUS_BLOCK IoStatus,
                             OUT PFILE_NETWORK_OPEN_INFORMATION Buffer)
{
    NTSTATUS Status;
    DUMMY_FILE_OBJECT LocalFileObject;
    HANDLE Handle;
    OPEN_PACKET OpenPacket;
    PAGED_CODE();
    IOTRACE(IO_FILE_DEBUG, "FileName: %wZ\n", ObjectAttributes->ObjectName);

    /* Setup the Open Packet */
    RtlZeroMemory(&OpenPacket, sizeof(OPEN_PACKET));
    OpenPacket.Type = IO_TYPE_OPEN_PACKET;
    OpenPacket.Size = sizeof(OPEN_PACKET);
    OpenPacket.CreateOptions = OpenOptions | FILE_OPEN_REPARSE_POINT;
    OpenPacket.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    OpenPacket.Options = IO_FORCE_ACCESS_CHECK;
    OpenPacket.Disposition = FILE_OPEN;
    OpenPacket.NetworkInformation = Buffer;
    OpenPacket.QueryOnly = TRUE;
    OpenPacket.FullAttributes = TRUE;
    OpenPacket.LocalFileObject = &LocalFileObject;

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
                                KernelMode,
                                NULL,
                                DesiredAccess,
                                &OpenPacket,
                                &Handle);
    if (OpenPacket.ParseCheck != TRUE)
    {
        /* Parse failed */
        IoStatus->Status = Status;
    }
    else
    {
        /* Use the Io status */
        IoStatus->Status = OpenPacket.FinalStatus;
        IoStatus->Information = OpenPacket.Information;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
IoUpdateShareAccess(IN PFILE_OBJECT FileObject,
                    OUT PSHARE_ACCESS ShareAccess)
{
    PAGED_CODE();

    /* Check if the file has an extension */
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        /* Check if caller specified to ignore access checks */
        //if (FileObject->FoExtFlags & IO_IGNORE_SHARE_ACCESS_CHECK)
        {
            /* Don't update share access */
            return;
        }
    }

    /* Otherwise, check if there's any access present */
    if ((FileObject->ReadAccess) ||
        (FileObject->WriteAccess) ||
        (FileObject->DeleteAccess))
    {
        /* Increase the open count */
        ShareAccess->OpenCount++;

        /* Add new share access */
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
NTSTATUS
NTAPI
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

    /* Get access masks */
    ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
    WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
    DeleteAccess = (DesiredAccess & DELETE) != 0;

    /* Set them in the file object */
    FileObject->ReadAccess = ReadAccess;
    FileObject->WriteAccess = WriteAccess;
    FileObject->DeleteAccess = DeleteAccess;

    /* Check if the file has an extension */
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        /* Check if caller specified to ignore access checks */
        //if (FileObject->FoExtFlags & IO_IGNORE_SHARE_ACCESS_CHECK)
        {
            /* Don't check share access */
            return STATUS_SUCCESS;
        }
    }

    /* Check if we have any access */
    if ((ReadAccess) || (WriteAccess) || (DeleteAccess))
    {
        /* Get shared access masks */
        SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
        SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
        SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

        /* Set them */
        FileObject->SharedRead = SharedRead;
        FileObject->SharedWrite = SharedWrite;
        FileObject->SharedDelete = SharedDelete;

        /* Check if the shared access is violated */
        if ((ReadAccess &&
             (ShareAccess->SharedRead < ShareAccess->OpenCount)) ||
            (WriteAccess &&
             (ShareAccess->SharedWrite < ShareAccess->OpenCount)) ||
            (DeleteAccess &&
             (ShareAccess->SharedDelete < ShareAccess->OpenCount)) ||
            ((ShareAccess->Readers != 0) && !SharedRead) ||
            ((ShareAccess->Writers != 0) && !SharedWrite) ||
            ((ShareAccess->Deleters != 0) && !SharedDelete))
        {
            /* Sharing violation, fail */
            return STATUS_SHARING_VIOLATION;
        }

        /* It's not, check if caller wants us to update it */
        if (Update)
        {
            /* Increase open count */
            ShareAccess->OpenCount++;

            /* Update shared access */
            ShareAccess->Readers += ReadAccess;
            ShareAccess->Writers += WriteAccess;
            ShareAccess->Deleters += DeleteAccess;
            ShareAccess->SharedRead += SharedRead;
            ShareAccess->SharedWrite += SharedWrite;
            ShareAccess->SharedDelete += SharedDelete;
        }
    }

    /* Validation successful */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoRemoveShareAccess(IN PFILE_OBJECT FileObject,
                    IN PSHARE_ACCESS ShareAccess)
{
    PAGED_CODE();

    /* Check if the file has an extension */
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        /* Check if caller specified to ignore access checks */
        //if (FileObject->FoExtFlags & IO_IGNORE_SHARE_ACCESS_CHECK)
        {
            /* Don't update share access */
            return;
        }
    }

    /* Otherwise, check if there's any access present */
    if ((FileObject->ReadAccess) ||
        (FileObject->WriteAccess) ||
        (FileObject->DeleteAccess))
    {
        /* Decrement the open count */
        ShareAccess->OpenCount--;

        /* Remove share access */
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
VOID
NTAPI
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
    BOOLEAN Update = TRUE;
    PAGED_CODE();

    ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE)) != 0;
    WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0;
    DeleteAccess = (DesiredAccess & DELETE) != 0;

    /* Check if the file has an extension */
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        /* Check if caller specified to ignore access checks */
        //if (FileObject->FoExtFlags & IO_IGNORE_SHARE_ACCESS_CHECK)
        {
            /* Don't update share access */
            Update = FALSE;
        }
    }

    /* Update basic access */
    FileObject->ReadAccess = ReadAccess;
    FileObject->WriteAccess = WriteAccess;
    FileObject->DeleteAccess = DeleteAccess;

    /* Check if we have no access as all */
    if (!(ReadAccess) && !(WriteAccess) && !(DeleteAccess))
    {
        /* Check if we need to update the structure */
        if (!Update) return;

        /* Otherwise, clear data */
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
        /* Calculate shared access */
        SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
        SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;
        SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE) != 0;

        /* Set it in the FO */
        FileObject->SharedRead = SharedRead;
        FileObject->SharedWrite = SharedWrite;
        FileObject->SharedDelete = SharedDelete;

        /* Check if we need to update the structure */
        if (!Update) return;

        /* Otherwise, set data */
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
 * @implemented
 */
VOID
NTAPI
IoCancelFileOpen(IN PDEVICE_OBJECT DeviceObject,
                 IN PFILE_OBJECT FileObject)
{
    PIRP Irp;
    KEVENT Event;
    KIRQL OldIrql;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;

    /* Check if handles were already created for the
     * open file. If so, that's over.
     */
    if (FileObject->Flags & FO_HANDLE_CREATED)
        KeBugCheckEx(INVALID_CANCEL_OF_FILE_OPEN,
                     (ULONG_PTR)FileObject,
                     (ULONG_PTR)DeviceObject, 0, 0);

    /* Reset the events */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP we'll use */
    Irp = IopAllocateIrpMustSucceed(DeviceObject->StackSize);
    /* Properly set it */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->UserEvent = &Event;
    Irp->UserIosb = &Irp->IoStatus;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_CLEANUP;
    Stack->FileObject = FileObject;

    /* Put on top of IRPs list of the thread */
    IopQueueIrpToThread(Irp);

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, UserRequest,
                              KernelMode, FALSE, NULL);
    }

    /* Remove from IRPs list */
    KeRaiseIrql(APC_LEVEL, &OldIrql);
    IopUnQueueIrpFromThread(Irp);
    KeLowerIrql(OldIrql);

    /* Free the IRP */
    IoFreeIrp(Irp);

    /* Clear the event */
    KeClearEvent(&FileObject->Event);
    /* And finally, mark the open operation as canceled */
    FileObject->Flags |= FO_FILE_OPEN_CANCELLED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoQueryFileDosDeviceName(IN PFILE_OBJECT FileObject,
                         OUT POBJECT_NAME_INFORMATION *ObjectNameInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSetFileOrigin(IN PFILE_OBJECT FileObject,
                IN BOOLEAN Remote)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN FlagSet;

    /* Get the flag status */
    FlagSet = FileObject->Flags & FO_REMOTE_ORIGIN ? TRUE : FALSE;

    /* Don't set the flag if it was set already, and don't remove it if it wasn't set */
    if (Remote && !FlagSet)
    {
        /* Set the flag */
        FileObject->Flags |= FO_REMOTE_ORIGIN;
    }
    else if (!Remote && FlagSet)
    {
        /* Remove the flag */
        FileObject->Flags &= ~FO_REMOTE_ORIGIN;
    }
    else
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER_MIX;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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
NTAPI
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
    PAGED_CODE();

    /* Check for Timeout */
    if (TimeOut)
    {
        /* check if the call came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            /* Enter SEH for Probe */
            _SEH2_TRY
            {
                /* Probe the timeout */
                Buffer.ReadTimeout = ProbeForReadLargeInteger(TimeOut);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }
        else
        {
            /* Otherwise, capture directly */
            Buffer.ReadTimeout = *TimeOut;
        }

        /* Set the correct setting */
        Buffer.TimeoutSpecified = TRUE;
    }
    else
    {
        /* Tell the FSD we don't have a timeout */
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
                        0,
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
NTAPI
NtCreateNamedPipeFile(OUT PHANDLE FileHandle,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_ATTRIBUTES ObjectAttributes,
                      OUT PIO_STATUS_BLOCK IoStatusBlock,
                      IN ULONG ShareAccess,
                      IN ULONG CreateDisposition,
                      IN ULONG CreateOptions,
                      IN ULONG NamedPipeType,
                      IN ULONG ReadMode,
                      IN ULONG CompletionMode,
                      IN ULONG MaximumInstances,
                      IN ULONG InboundQuota,
                      IN ULONG OutboundQuota,
                      IN PLARGE_INTEGER DefaultTimeout)
{
    NAMED_PIPE_CREATE_PARAMETERS Buffer;
    PAGED_CODE();

    /* Check for Timeout */
    if (DefaultTimeout)
    {
        /* check if the call came from user mode */
        if (KeGetPreviousMode() != KernelMode)
        {
            /* Enter SEH for Probe */
            _SEH2_TRY
            {
                /* Probe the timeout */
                Buffer.DefaultTimeout =
                    ProbeForReadLargeInteger(DefaultTimeout);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }
        else
        {
            /* Otherwise, capture directly */
            Buffer.DefaultTimeout = *DefaultTimeout;
        }

        /* Set the correct setting */
        Buffer.TimeoutSpecified = TRUE;
    }
    else
    {
        /* Tell the FSD we don't have a timeout */
        Buffer.TimeoutSpecified = FALSE;
    }

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
                        0,
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
NTAPI
NtFlushWriteBuffer(VOID)
{
    PAGED_CODE();

    /* Call the kernel */
    KeFlushWriteBuffer();
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenFile(OUT PHANDLE FileHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN ULONG ShareAccess,
           IN ULONG OpenOptions)
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
NTAPI
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
NTAPI
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
    NTSTATUS Status;
    PLIST_ENTRY ListHead, NextEntry;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check the previous mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status Block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Reference the file object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* IRP cancellations are synchronized at APC_LEVEL. */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Get the current thread */
    Thread = PsGetCurrentThread();

    /* Update the operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Loop the list */
    ListHead = &Thread->IrpList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the IRP and check if the File Object matches */
        Irp = CONTAINING_RECORD(NextEntry, IRP, ThreadListEntry);
        if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
        {
            /* Cancel this IRP and keep looping */
            IoCancelIrp(Irp);
            OurIrpsInList = TRUE;
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Lower the IRQL */
    KeLowerIrql(OldIrql);

    /* Check if we had found an IRP */
    if (OurIrpsInList)
    {
        /* Setup a 10ms wait */
        Interval.QuadPart = -100000;

        /* Start looping */
        while (OurIrpsInList)
        {
            /* Do the wait */
            KeDelayExecutionThread(KernelMode, FALSE, &Interval);
            OurIrpsInList = FALSE;

            /* Raise IRQL */
            KeRaiseIrql(APC_LEVEL, &OldIrql);

            /* Now loop the list again */
            NextEntry = ListHead->Flink;
            while (NextEntry != ListHead)
            {
                /* Get the IRP and check if the File Object matches */
                Irp = CONTAINING_RECORD(NextEntry, IRP, ThreadListEntry);
                if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
                {
                    /* Keep looping */
                    OurIrpsInList = TRUE;
                    break;
                }

                /* Go to the next entry */
                NextEntry = NextEntry->Flink;
            }

            /* Lower the IRQL */
            KeLowerIrql(OldIrql);
        }
    }

    /* Enter SEH for writing back the I/O Status */
    _SEH2_TRY
    {
        /* Write success */
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = 0;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Ignore exception */
    }
    _SEH2_END;

    /* Dereference the file object and return success */
    ObDereferenceObject(FileObject);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    NTSTATUS Status;
    DUMMY_FILE_OBJECT LocalFileObject;
    HANDLE Handle;
    KPROCESSOR_MODE AccessMode = KeGetPreviousMode();
    OPEN_PACKET OpenPacket;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileMame: %wZ\n", ObjectAttributes->ObjectName);

    /* Setup the Open Packet */
    RtlZeroMemory(&OpenPacket, sizeof(OPEN_PACKET));
    OpenPacket.Type = IO_TYPE_OPEN_PACKET;
    OpenPacket.Size = sizeof(OPEN_PACKET);
    OpenPacket.CreateOptions = FILE_DELETE_ON_CLOSE;
    OpenPacket.ShareAccess = FILE_SHARE_READ |
                             FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE;
    OpenPacket.Disposition = FILE_OPEN;
    OpenPacket.DeleteOnly = TRUE;
    OpenPacket.LocalFileObject = &LocalFileObject;

    /* Update the operation counts */
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
                                DELETE,
                                &OpenPacket,
                                &Handle);
    if (OpenPacket.ParseCheck != TRUE) return Status;

    /* Retrn the Io status */
    return OpenPacket.FinalStatus;
}

/* EOF */
