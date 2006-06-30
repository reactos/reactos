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

/* GLOBALS *******************************************************************/

extern GENERIC_MAPPING IopFileMapping;

NTSTATUS
STDCALL
SeSetWorldSecurityDescriptor(SECURITY_INFORMATION SecurityInformation,
                             PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PULONG BufferLength);

/* INTERNAL FUNCTIONS ********************************************************/

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
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    DPRINT("IopParseDevice:\n"
           "DeviceObject : %p\n"
           "RelatedFileObject : %p\n"
           "CompleteName : %wZ, RemainingName : %wZ\n",
           ParseObject,
           Context,
           CompleteName,
           RemainingName);

    if (!*RemainingName->Buffer)
    {
        DeviceObject = ParseObject;

        Status = ObCreateObject(AccessMode,
                                IoFileObjectType,
                                NULL,
                                AccessMode,
                                NULL,
                                sizeof(FILE_OBJECT),
                                0,
                                0,
                                (PVOID*)&FileObject);
        /* Set File Object Data */
        ASSERT(DeviceObject);
        FileObject->DeviceObject = IoGetAttachedDevice(DeviceObject);
        DPRINT("DO. DRV Name: %p %wZ\n", DeviceObject, &DeviceObject->DriverObject->DriverName);

        /* HACK */
        FileObject->Flags |= FO_DIRECT_DEVICE_OPEN;
        *Object = FileObject;
        return STATUS_SUCCESS;
    }

    Status = ObCreateObject(AccessMode,
                        IoFileObjectType,
                        NULL,
                        AccessMode,
                        NULL,
                        sizeof(FILE_OBJECT),
                        0,
                        0,
                        (PVOID*)&FileObject);

    if (!Context)
    {
        /* Parent is a device object */
        DeviceObject = IoGetAttachedDevice((PDEVICE_OBJECT)ParseObject);

        /* Check if it has a VPB */
        if (DeviceObject->Vpb)
        {
            /* Check if it's not already mounted */
            if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
            {
                Status = IoMountVolume(DeviceObject, FALSE);
                if (!NT_SUCCESS(Status))
                {
                    /* Couldn't mount, fail the lookup */
                    ObDereferenceObject(FileObject);
                    *Object = NULL;
                    return STATUS_UNSUCCESSFUL;
                }
            }

            DeviceObject = DeviceObject->Vpb->DeviceObject;
        }
    }
    else
    {
        FileObject->RelatedFileObject = OpenPacket->RelatedFileObject;
        DeviceObject = OpenPacket->RelatedFileObject->DeviceObject;
    }

    RtlCreateUnicodeString(&FileObject->FileName, RemainingName->Buffer);
    FileObject->DeviceObject = DeviceObject;
    *Object = FileObject;
    return STATUS_SUCCESS;
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
    OPEN_PACKET OpenPacket;

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(ParseObject);
    OpenPacket.RelatedFileObject = ParseObject;

    /* Call the main routine */
    return IopParseDevice(DeviceObject,
                          ObjectType,
                          AccessState,
                          AccessMode,
                          Attributes,
                          CompleteName,
                          RemainingName,
                          &OpenPacket,
                          SecurityQos,
                          Object);
}

VOID
STDCALL
IopDeleteFile(PVOID ObjectBody)
{
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    KEVENT Event;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("IopDeleteFile()\n");

    if (FileObject->DeviceObject)
    {
        /* Check if this is a direct open or not */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
        }
        else
        {
            DeviceObject = IoGetRelatedDeviceObject(FileObject);
        }

        /* Clear and set up Events */
        KeClearEvent(&FileObject->Event);
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        /* Allocate an IRP */
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

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

        /* Call the FS Driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for completion */
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
        IoFreeIrp(Irp);

    }

    /* Clear the file name */
    if (FileObject->FileName.Buffer)
    {
       ExFreePool(FileObject->FileName.Buffer);
       FileObject->FileName.Buffer = NULL;
    }

    /* Free the completion context */
    if (FileObject->CompletionContext)
    {
       ObDereferenceObject(FileObject->CompletionContext->Port);
       ExFreePool(FileObject->CompletionContext);
    }
}

NTSTATUS
STDCALL
IopSecurityFile(PVOID ObjectBody,
                SECURITY_OPERATION_CODE OperationCode,
                SECURITY_INFORMATION SecurityInformation,
                PSECURITY_DESCRIPTOR SecurityDescriptor,
                PULONG BufferLength,
                PSECURITY_DESCRIPTOR *OldSecurityDescriptor,    
                POOL_TYPE PoolType,
                PGENERIC_MAPPING GenericMapping)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    ULONG MajorFunction;
    PIRP Irp;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopSecurityFile() called\n");

    FileObject = (PFILE_OBJECT)ObjectBody;

    if (OperationCode == QuerySecurityDescriptor)
    {
        MajorFunction = IRP_MJ_QUERY_SECURITY;
        DPRINT("Query security descriptor\n");
    }
    else if (OperationCode == DeleteSecurityDescriptor)
    {
        DPRINT("Delete\n");
        return STATUS_SUCCESS;
    }
    else if (OperationCode == AssignSecurityDescriptor)
    {
        /* If this is a direct open, we can assign it */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            /* Get the Device Object */
            DPRINT("here\n");
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);

            /* Assign the Security Descriptor */
            DeviceObject->SecurityDescriptor = SecurityDescriptor;
        }
        return STATUS_SUCCESS;
    }
    else
    {
        MajorFunction = IRP_MJ_SET_SECURITY;
        DPRINT("Set security descriptor\n");

        /* If this is a direct open, we can set it */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            DPRINT1("Set SD unimplemented for Devices\n");
            return STATUS_SUCCESS;
        }
    }

    /* Get the Device Object */
    DPRINT1("FileObject: %p\n", FileObject);
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = ExGetPreviousMode();
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set Stack Parameters */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = MajorFunction;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    if (OperationCode == QuerySecurityDescriptor)
    {
        StackPtr->Parameters.QuerySecurity.SecurityInformation = SecurityInformation;
        StackPtr->Parameters.QuerySecurity.Length = *BufferLength;
        Irp->UserBuffer = SecurityDescriptor;
    }
    else
    {
        StackPtr->Parameters.SetSecurity.SecurityInformation = SecurityInformation;
        StackPtr->Parameters.SetSecurity.SecurityDescriptor = SecurityDescriptor;
    }

    ObReferenceObject(FileObject);

    /* Call the Driver */
    Status = IoCallDriver(FileObject->DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* This Driver doesn't implement Security, so try to give it a default */
    if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
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
        /* Return length */
        *BufferLength = IoStatusBlock.Information;
    }

    /* Return Status */
    return Status;
}

NTSTATUS
STDCALL
IopQueryNameFile(PVOID ObjectBody,
                 IN BOOLEAN HasName,
                 POBJECT_NAME_INFORMATION ObjectNameInfo,
                 ULONG Length,
                 PULONG ReturnLength,
                 IN KPROCESSOR_MODE PreviousMode)
{
    POBJECT_NAME_INFORMATION LocalInfo;
    PFILE_OBJECT FileObject;
    ULONG LocalReturnLength;
    NTSTATUS Status;

    DPRINT1("IopQueryNameFile() called\n");

    FileObject = (PFILE_OBJECT)ObjectBody;

    /* Allocate Buffer */
    LocalInfo = ExAllocatePool(PagedPool,
                               sizeof(OBJECT_NAME_INFORMATION) +
                               MAX_PATH * sizeof(WCHAR));
    if (LocalInfo == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Query the name */
    Status = ObQueryNameString(FileObject->DeviceObject,
                               LocalInfo,
                               MAX_PATH * sizeof(WCHAR),
                               &LocalReturnLength);
    if (!NT_SUCCESS (Status))
    {
        ExFreePool (LocalInfo);
        return Status;
    }
    DPRINT ("Device path: %wZ\n", &LocalInfo->Name);
    
    /* Write Device Path */
    Status = RtlAppendUnicodeStringToString(&ObjectNameInfo->Name,
                                            &(LocalInfo)->Name);

    /* Query the File name */
    Status = IoQueryFileInformation(FileObject,
                                    FileNameInformation,
                                    LocalReturnLength,
                                    LocalInfo,
                                    NULL);
    if (Status != STATUS_SUCCESS)
    {
        ExFreePool(LocalInfo);
        return Status;
    }

    /* Write the Name */
    Status = RtlAppendUnicodeToString(&ObjectNameInfo->Name,
                                      ((PFILE_NAME_INFORMATION)LocalInfo)->FileName);
    DPRINT ("Total path: %wZ\n", &ObjectNameInfo->Name);

    /* Free buffer and return */
    ExFreePool(LocalInfo);
    return Status;
}

VOID
STDCALL
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

    DPRINT("IopCloseFile()\n");

    if (HandleCount > 1 || FileObject->DeviceObject == NULL) return;

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Clear and set up Events */
    KeClearEvent(&FileObject->Event);
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set it up */
    Irp->UserEvent = &Event;
    Irp->UserIosb = &Irp->IoStatus;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

    /* Set up Stack Pointer Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_CLEANUP;
    StackPtr->FileObject = FileObject;

    /* Call the FS Driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Wait for completion */
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }
    IoFreeIrp(Irp);
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
 * NAME       EXPORTED
 *  IoCreateFile@56
 *
 * DESCRIPTION
 *  Either causes a new file or directory to be created, or it
 *  opens an existing file, device, directory or volume, giving
 *  the caller a handle for the file object. This handle can be
 *  used by subsequent calls to manipulate data within the file
 *  or the file object's state of attributes.
 *
 * ARGUMENTS
 * FileHandle (OUT)
 *  Points to a variable which receives the file handle
 *  on return;
 *
 * DesiredAccess
 *  Desired access to the file;
 *
 * ObjectAttributes
 *  Structure describing the file;
 *
 * IoStatusBlock (OUT)
 *  Receives information about the operation on return;
 *
 * AllocationSize [OPTIONAL]
 *  Initial size of the file in bytes;
 *
 * FileAttributes
 *  Attributes to create the file with;
 *
 * ShareAccess
 *  Type of shared access the caller would like to the
 *  file;
 *
 * CreateDisposition
 *  Specifies what to do, depending on whether the
 *  file already exists;
 *
 * CreateOptions
 *  Options for creating a new file;
 *
 * EaBuffer [OPTIONAL]
 *  Undocumented;
 *
 * EaLength
 *  Undocumented;
 *
 * CreateFileType
 *  Type of file (normal, named pipe, mailslot) to create;
 *
 * ExtraCreateParameters [OPTIONAL]
 *  Additional creation data for named pipe and mailsots;
 *
 * Options
 *  Undocumented.
 *
 * RETURN VALUE
 *  Status
 *
 * NOTE
 *  Prototype taken from Bo Branten's ntifs.h v15.
 *  Description taken from old NtCreateFile's which is
 *  now a wrapper of this call.
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS
STDCALL
IoCreateFile(OUT PHANDLE  FileHandle,
             IN ACCESS_MASK  DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PLARGE_INTEGER  AllocationSize  OPTIONAL,
             IN ULONG   FileAttributes,
             IN ULONG   ShareAccess,
             IN ULONG   CreateDisposition,
             IN ULONG   CreateOptions,
             IN PVOID   EaBuffer  OPTIONAL,
             IN ULONG   EaLength,
             IN CREATE_FILE_TYPE CreateFileType,
             IN PVOID   ExtraCreateParameters OPTIONAL,
             IN ULONG   Options)
{
   PFILE_OBJECT  FileObject = NULL;
   //PDEVICE_OBJECT DeviceObject;
   PIRP   Irp;
   PEXTENDED_IO_STACK_LOCATION StackLoc;
   IO_SECURITY_CONTEXT  SecurityContext;
   KPROCESSOR_MODE      AccessMode;
   HANDLE               LocalHandle;
   LARGE_INTEGER        SafeAllocationSize;
   PVOID                SystemEaBuffer = NULL;
   NTSTATUS  Status = STATUS_SUCCESS;
   AUX_DATA AuxData;
   ACCESS_STATE AccessState;

   DPRINT("IoCreateFile(FileHandle 0x%p, DesiredAccess %x, "
          "ObjectAttributes 0x%p ObjectAttributes->ObjectName->Buffer %S)\n",
          FileHandle,DesiredAccess,ObjectAttributes,
          ObjectAttributes->ObjectName->Buffer);

   ASSERT_IRQL(PASSIVE_LEVEL);

   if (IoStatusBlock == NULL || FileHandle == NULL)
     return STATUS_ACCESS_VIOLATION;

   LocalHandle = 0;

   if(Options & IO_NO_PARAMETER_CHECKING)
     AccessMode = KernelMode;
   else
     AccessMode = ExGetPreviousMode();

   if(AccessMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWriteHandle(FileHandle);
       ProbeForWrite(IoStatusBlock,
                     sizeof(IO_STATUS_BLOCK),
                     sizeof(ULONG));
       if(AllocationSize != NULL)
       {
         SafeAllocationSize = ProbeForReadLargeInteger(AllocationSize);
       }
       else
         SafeAllocationSize.QuadPart = 0;

       if(EaBuffer != NULL && EaLength > 0)
       {
         ProbeForRead(EaBuffer,
                      EaLength,
                      sizeof(ULONG));

         /* marshal EaBuffer */
         SystemEaBuffer = ExAllocatePool(NonPagedPool,
                                         EaLength);
         if(SystemEaBuffer == NULL)
         {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           _SEH_LEAVE;
         }

         RtlCopyMemory(SystemEaBuffer,
                       EaBuffer,
                       EaLength);
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }
   else
   {
     if(AllocationSize != NULL)
       SafeAllocationSize = *AllocationSize;
     else
       SafeAllocationSize.QuadPart = 0;

     if(EaBuffer != NULL && EaLength > 0)
     {
       SystemEaBuffer = EaBuffer;
     }
   }

   if(Options & IO_CHECK_CREATE_PARAMETERS)
   {
     DPRINT1("FIXME: IO_CHECK_CREATE_PARAMETERS not yet supported!\n");
   }

   RtlMapGenericMask(&DesiredAccess, &IoFileObjectType->TypeInfo.GenericMapping);

   /* First try to open an existing named object */
   Status = ObOpenObjectByName(ObjectAttributes,
                               NULL,
                               AccessMode,
                               NULL,
                               DesiredAccess,
                               NULL,
                               &LocalHandle);

    ObReferenceObjectByHandle(LocalHandle,
                              DesiredAccess,
                              NULL,
                              KernelMode,
                              (PVOID*)&FileObject,
                              NULL);
    if (!NT_SUCCESS(Status)) return Status;

   //
   // stop stuff that should be in IopParseDevice
   //

   if (CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
     {
       FileObject->Flags |= (FO_ALERTABLE_IO | FO_SYNCHRONOUS_IO);
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)
     {
       FileObject->Flags |= FO_SYNCHRONOUS_IO;
     }

   if (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)
     FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;

    /* 
     * FIXME: We should get the access state from Ob once this function becomes
     * a parse routine once the Ob is refactored.
     */
   SeCreateAccessState(&AccessState, &AuxData, FILE_ALL_ACCESS, NULL);

   SecurityContext.SecurityQos = NULL; /* ?? */
   SecurityContext.AccessState = &AccessState;
   SecurityContext.DesiredAccess = DesiredAccess;
   SecurityContext.FullCreateOptions = 0; /* ?? */

   KeInitializeEvent(&FileObject->Lock, SynchronizationEvent, TRUE);
   KeInitializeEvent(&FileObject->Event, NotificationEvent, FALSE);

   DPRINT("FileObject 0x%p\n", FileObject);
   DPRINT("FileObject->DeviceObject 0x%p\n", FileObject->DeviceObject);
   /*
    * Create a new IRP to hand to
    * the FS driver: this may fail
    * due to resource shortage.
    */
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, FALSE);
   if (Irp == NULL)
     {
       ZwClose(LocalHandle);
       return STATUS_UNSUCCESSFUL;
     }

   //trigger FileObject/Event dereferencing
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = AccessMode;
   Irp->UserIosb = IoStatusBlock;
   Irp->AssociatedIrp.SystemBuffer = SystemEaBuffer;
   Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = &FileObject->Event;
   Irp->Overlay.AllocationSize = SafeAllocationSize;

   /*
    * Get the stack location for the new
    * IRP and prepare it.
    */
   StackLoc = (PEXTENDED_IO_STACK_LOCATION)IoGetNextIrpStackLocation(Irp);
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = (UCHAR)Options;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = FileObject->DeviceObject;
   StackLoc->FileObject = FileObject;

   switch (CreateFileType)
     {
       default:
       case CreateFileTypeNone:
         StackLoc->MajorFunction = IRP_MJ_CREATE;
         StackLoc->Parameters.Create.SecurityContext = &SecurityContext;
         StackLoc->Parameters.Create.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
         StackLoc->Parameters.Create.Options |= (CreateDisposition << 24);
         StackLoc->Parameters.Create.FileAttributes = (USHORT)FileAttributes;
         StackLoc->Parameters.Create.ShareAccess = (USHORT)ShareAccess;
         StackLoc->Parameters.Create.EaLength = SystemEaBuffer != NULL ? EaLength : 0;
         break;

      case CreateFileTypeNamedPipe:
        StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
        StackLoc->Parameters.CreatePipe.SecurityContext = &SecurityContext;
        StackLoc->Parameters.CreatePipe.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
        StackLoc->Parameters.CreatePipe.Options |= (CreateDisposition << 24);
        StackLoc->Parameters.CreatePipe.ShareAccess = (USHORT)ShareAccess;
        StackLoc->Parameters.CreatePipe.Parameters = ExtraCreateParameters;
        break;

      case CreateFileTypeMailslot:
        StackLoc->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
        StackLoc->Parameters.CreateMailslot.SecurityContext = &SecurityContext;
        StackLoc->Parameters.CreateMailslot.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
        StackLoc->Parameters.CreateMailslot.Options |= (CreateDisposition << 24);
        StackLoc->Parameters.CreateMailslot.ShareAccess = (USHORT)ShareAccess;
        StackLoc->Parameters.CreateMailslot.Parameters = ExtraCreateParameters;
        break;
     }

   /*
    * Now call the driver and
    * possibly wait if it can
    * not complete the request
    * immediately.
    */
   Status = IofCallDriver(FileObject->DeviceObject, Irp );
   DPRINT("Status :%x\n", Status);
   
   if (Status == STATUS_PENDING)
     {
       KeWaitForSingleObject(&FileObject->Event,
                             Executive,
                             AccessMode,
                             FALSE,
                             NULL);
       Status = IoStatusBlock->Status;
     }
   if (!NT_SUCCESS(Status))
     {
       DPRINT("Failing create request with status %x\n", Status);
       FileObject->DeviceObject = NULL;
       FileObject->Vpb = NULL;

       ZwClose(LocalHandle);
     }
   else
     {
       _SEH_TRY
         {
           *FileHandle = LocalHandle;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
     }

   /* cleanup EABuffer if captured */
   if(AccessMode != KernelMode && SystemEaBuffer != NULL)
   {
     ExFreePool(SystemEaBuffer);
   }

   ASSERT_IRQL(PASSIVE_LEVEL);

   DPRINT("Finished IoCreateFile() (*FileHandle) 0x%p\n", (*FileHandle));

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
 * NAME       EXPORTED
 *  IoCreateStreamFileObject@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * FileObject
 *  ?
 *
 * DeviceObject
 *  ?
 *
 * RETURN VALUE
 *
 * NOTE
 *
 * REVISIONS
 *
 * @implemented
 */
PFILE_OBJECT 
STDCALL
IoCreateStreamFileObject(PFILE_OBJECT FileObject,
                         PDEVICE_OBJECT DeviceObject)
{
    PFILE_OBJECT CreatedFileObject;
    NTSTATUS Status;
    
    /* FIXME: This function should call ObInsertObject. The "Lite" version 
       doesnt. This function is also called from IoCreateFile for some 
       reason. These hacks need to be removed.
    */

    DPRINT("IoCreateStreamFileObject(FileObject 0x%p, DeviceObject 0x%p)\n",
            FileObject, DeviceObject);
    PAGED_CODE();

    /* Create the File Object */
    Status = ObCreateObject(KernelMode,
                            IoFileObjectType,
                            NULL,
                            KernelMode,
                            NULL,
                            sizeof(FILE_OBJECT),
                            0,
                            0,
                            (PVOID*)&CreatedFileObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create FileObject\n");
        return (NULL);
    }

    /* Choose Device Object */
    if (FileObject) DeviceObject = FileObject->DeviceObject;
    DPRINT("DeviceObject 0x%p\n", DeviceObject);
    
    /* HACK */
    DeviceObject = IoGetAttachedDevice(DeviceObject);
    
    /* Set File Object Data */
    CreatedFileObject->DeviceObject = DeviceObject; 
    CreatedFileObject->Vpb = DeviceObject->Vpb;
    CreatedFileObject->Type = IO_TYPE_FILE;
    CreatedFileObject->Flags |= FO_STREAM_FILE;

    /* Initialize Lock and Event */
    KeInitializeEvent(&CreatedFileObject->Event, NotificationEvent, FALSE);
    KeInitializeEvent(&CreatedFileObject->Lock, SynchronizationEvent, TRUE);

    /* Return file */
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
IopQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                       IN FILE_INFORMATION_CLASS FileInformationClass,
                       OUT PVOID FileInformation)
{
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    NTSTATUS Status;
    KPROCESSOR_MODE AccessMode;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    ULONG BufferSize;
    union
    {
        FILE_BASIC_INFORMATION BasicInformation;
        FILE_NETWORK_OPEN_INFORMATION NetworkOpenInformation;
    }LocalFileInformation;

    if (FileInformationClass == FileBasicInformation)
    {
        BufferSize = sizeof(FILE_BASIC_INFORMATION);
    }
    else if (FileInformationClass == FileNetworkOpenInformation)
    {
        BufferSize = sizeof(FILE_NETWORK_OPEN_INFORMATION);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    AccessMode = ExGetPreviousMode();

    if (AccessMode != KernelMode)
    {
        Status = STATUS_SUCCESS;
        _SEH_TRY
        {
            ProbeForWrite(FileInformation,
                          BufferSize,
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (NT_SUCCESS(Status))
        {
            Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                                AccessMode,
                                                FALSE,
                                                &ObjectCreateInfo,
                                                &ObjectName);
        }
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        InitializeObjectAttributes(&LocalObjectAttributes,
                                   &ObjectName,
                                   ObjectCreateInfo.Attributes,
                                   ObjectCreateInfo.RootDirectory,
                                   ObjectCreateInfo.SecurityDescriptor);
    }

    /* Open the file */
    Status = ZwOpenFile(&FileHandle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        AccessMode == KernelMode ? ObjectAttributes : &LocalObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (AccessMode != KernelMode)
    {
        ObpReleaseCapturedAttributes(&ObjectCreateInfo);
        if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
    }
    if (!NT_SUCCESS (Status))
    {
        DPRINT ("ZwOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Get file attributes */
    Status = ZwQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    AccessMode == KernelMode ? FileInformation : &LocalFileInformation,
                                    BufferSize,
                                    FileInformationClass);
    if (!NT_SUCCESS (Status))
    {
        DPRINT ("ZwQueryInformationFile() failed (Status %lx)\n", Status);
    }
    ZwClose(FileHandle);

    if (NT_SUCCESS(Status) && AccessMode != KernelMode)
    {
        _SEH_TRY
        {
            memcpy(FileInformation, &LocalFileInformation, BufferSize);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    return Status;
}

NTSTATUS
STDCALL
NtQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                      OUT PFILE_BASIC_INFORMATION FileInformation)
{
    return IopQueryAttributesFile(ObjectAttributes,
                                  FileBasicInformation,
                                  FileInformation);
}

NTSTATUS
STDCALL
NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                          OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
  return IopQueryAttributesFile(ObjectAttributes,
                                FileNetworkOpenInformation,
                                FileInformation);
}


/* EOF */
