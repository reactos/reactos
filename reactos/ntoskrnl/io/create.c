/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/create.c
 * PURPOSE:         Handling file create/open apis
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_FILE_NAME     TAG('F', 'N', 'A', 'M')

/* FUNCTIONS *************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	NtDeleteFile@4
 * 	
 * DESCRIPTION
 * 	
 * ARGUMENTS
 *	ObjectAttributes
 *		?
 *
 * RETURN VALUE
 *
 * REVISIONS
 * 
 * @unimplemented
 */
NTSTATUS STDCALL
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							INTERNAL
 * 	IopCreateFile
 * 	
 * DESCRIPTION
 * 	
 * ARGUMENTS
 *		
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
IopCreateFile(PVOID			ObjectBody,
	      PVOID			Parent,
	      PWSTR			RemainingPath,
	      POBJECT_ATTRIBUTES	ObjectAttributes)
{
  PDEVICE_OBJECT DeviceObject;
  PFILE_OBJECT FileObject = (PFILE_OBJECT) ObjectBody;
  POBJECT_TYPE ParentObjectType;
  NTSTATUS Status;

  DPRINT("IopCreateFile(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody,
	 Parent,
	 RemainingPath);

  if (NULL == Parent)
    {
      /* This is probably an attempt to create a meta fileobject (eg. for FAT)
         for the cache manager, so return STATUS_SUCCESS */
      DPRINT("Parent object was NULL\n");
      return(STATUS_SUCCESS);
    }

  ParentObjectType = BODY_TO_HEADER(Parent)->ObjectType;

  if (ParentObjectType != IoDeviceObjectType &&
      ParentObjectType != IoFileObjectType)
    {
      DPRINT("Parent [%wZ] is a %S which is neither a file type nor a device type ; remaining path = %S\n",
        &BODY_TO_HEADER(Parent)->Name,
        BODY_TO_HEADER(Parent)->ObjectType->TypeName.Buffer,
        RemainingPath);
      return(STATUS_UNSUCCESSFUL);
    }

  Status = ObReferenceObjectByPointer(Parent,
				      STANDARD_RIGHTS_REQUIRED,
				      ParentObjectType,
				      UserMode);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Failed to reference parent object %x\n", Parent);
      return(Status);
    }

  if (ParentObjectType == IoDeviceObjectType)
    {
      /* Parent is a devce object */
      DeviceObject = IoGetAttachedDevice((PDEVICE_OBJECT)Parent);
      DPRINT("DeviceObject %x\n", DeviceObject);

      if (RemainingPath == NULL)
	{
	  FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
	  FileObject->FileName.Buffer = 0;
	  FileObject->FileName.Length = FileObject->FileName.MaximumLength = 0;
	}
      else
	{
	  if ((DeviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM)
	      && (DeviceObject->DeviceType != FILE_DEVICE_DISK)
	      && (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM)
	      && (DeviceObject->DeviceType != FILE_DEVICE_TAPE)
	      && (DeviceObject->DeviceType != FILE_DEVICE_NETWORK)
	      && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
	      && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
	    {
	      CPRINT("Device was wrong type\n");
	      return(STATUS_UNSUCCESSFUL);
	    }

	  if (DeviceObject->DeviceType != FILE_DEVICE_NETWORK
	      && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
	      && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
	    {
	      if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
		{
		  DPRINT("Mount the logical volume\n");
		  Status = IoMountVolume(DeviceObject, FALSE);
		  DPRINT("Status %x\n", Status);
		  if (!NT_SUCCESS(Status))
		    {
		      CPRINT("Failed to mount logical volume (Status %x)\n",
			     Status);
		      return(Status);
		    }
		}
	      DeviceObject = DeviceObject->Vpb->DeviceObject;
	      DPRINT("FsDeviceObject %lx\n", DeviceObject);
	    }
	  RtlpCreateUnicodeString(&(FileObject->FileName),
             RemainingPath, NonPagedPool);
	}
    }
  else
    {
      /* Parent is a file object */
      if (RemainingPath == NULL)
	{
	  CPRINT("Device is unnamed\n");
	  return STATUS_UNSUCCESSFUL;
	}

      DeviceObject = ((PFILE_OBJECT)Parent)->DeviceObject;
      DPRINT("DeviceObject %x\n", DeviceObject);

      FileObject->RelatedFileObject = (PFILE_OBJECT)Parent;

      RtlpCreateUnicodeString(&(FileObject->FileName),
              RemainingPath, NonPagedPool);
    }

  DPRINT("FileObject->FileName %wZ\n",
	 &FileObject->FileName);
  FileObject->DeviceObject = DeviceObject;
  DPRINT("FileObject %x DeviceObject %x\n",
	 FileObject,
	 DeviceObject);
  FileObject->Vpb = DeviceObject->Vpb;
  FileObject->Type = InternalFileType;

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoCreateStreamFileObject@8
 * 	
 * DESCRIPTION
 * 	
 * ARGUMENTS
 *	FileObject
 *		?
 *		
 *	DeviceObject
 *		?
 *		
 * RETURN VALUE
 *
 * NOTE
 * 	
 * REVISIONS
 * 
 * @implemented
 */
PFILE_OBJECT STDCALL
IoCreateStreamFileObject(PFILE_OBJECT FileObject,
			 PDEVICE_OBJECT DeviceObject)
{
  PFILE_OBJECT	CreatedFileObject;
  NTSTATUS Status;

  DPRINT("IoCreateStreamFileObject(FileObject %x, DeviceObject %x)\n",
	 FileObject, DeviceObject);

  ASSERT_IRQL(PASSIVE_LEVEL);

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
      DPRINT("Could not create FileObject\n");
      return (NULL);
    }

  if (FileObject != NULL)
    {
      DeviceObject = FileObject->DeviceObject;
    }
  DeviceObject = IoGetAttachedDevice(DeviceObject);

  DPRINT("DeviceObject %x\n", DeviceObject);

  CreatedFileObject->DeviceObject = DeviceObject->Vpb->DeviceObject;
  CreatedFileObject->Vpb = DeviceObject->Vpb;
  CreatedFileObject->Type = InternalFileType;
  CreatedFileObject->Flags |= FO_DIRECT_DEVICE_OPEN;

  // shouldn't we initialize the lock event, and several other things here too?
  KeInitializeEvent(&CreatedFileObject->Event, NotificationEvent, FALSE);
  KeInitializeEvent(&CreatedFileObject->Lock, SynchronizationEvent, TRUE);

  return CreatedFileObject;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoCreateFile@56
 * 	
 * DESCRIPTION
 * 	Either causes a new file or directory to be created, or it
 * 	opens an existing file, device, directory or volume, giving
 * 	the caller a handle for the file object. This handle can be
 * 	used by subsequent calls to manipulate data within the file
 * 	or the file object's state of attributes.
 * 	
 * ARGUMENTS
 *	FileHandle (OUT)
 *		Points to a variable which receives the file handle
 *		on return;
 *		
 *	DesiredAccess
 *		Desired access to the file;
 *		
 *	ObjectAttributes
 *		Structure describing the file;
 *		
 *	IoStatusBlock (OUT)
 *		Receives information about the operation on return;
 *		
 *	AllocationSize [OPTIONAL]
 *		Initial size of the file in bytes;
 *		
 *	FileAttributes
 *		Attributes to create the file with;
 *		
 *	ShareAccess
 *		Type of shared access the caller would like to the
 *		file;
 *		
 *	CreateDisposition
 *		Specifies what to do, depending on whether the
 *		file already exists;
 *		
 *	CreateOptions
 *		Options for creating a new file;
 *		
 *	EaBuffer [OPTIONAL]
 *		Undocumented;
 *		
 *	EaLength
 *		Undocumented;
 *		
 *	CreateFileType
 *		Type of file (normal, named pipe, mailslot) to create;
 *		
 *	ExtraCreateParameters [OPTIONAL]
 *		Additional creation data for named pipe and mailsots;
 *		
 *	Options
 *		Undocumented.
 *		
 * RETURN VALUE
 * 	Status
 *
 * NOTE
 * 	Prototype taken from Bo Branten's ntifs.h v15.
 * 	Description taken from old NtCreateFile's which is
 * 	now a wrapper of this call.
 * 	
 * REVISIONS
 * 
 * @implemented
 */
NTSTATUS STDCALL
IoCreateFile(OUT PHANDLE		FileHandle,
	     IN	ACCESS_MASK		DesiredAccess,
	     IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	     OUT PIO_STATUS_BLOCK	IoStatusBlock,
	     IN	PLARGE_INTEGER		AllocationSize		OPTIONAL,
	     IN	ULONG			FileAttributes,
	     IN	ULONG			ShareAccess,
	     IN	ULONG			CreateDisposition,
	     IN	ULONG			CreateOptions,
	     IN	PVOID			EaBuffer		OPTIONAL,
	     IN	ULONG			EaLength,
	     IN	CREATE_FILE_TYPE	CreateFileType,
	     IN	PVOID			ExtraCreateParameters	OPTIONAL,
	     IN	ULONG			Options)
{
   PFILE_OBJECT		FileObject;
   PIRP			Irp;
   PIO_STACK_LOCATION	StackLoc;
   IO_SECURITY_CONTEXT  SecurityContext;
   KPROCESSOR_MODE      AccessMode;
   HANDLE               LocalFileHandle;
   IO_STATUS_BLOCK      LocalIoStatusBlock;
   LARGE_INTEGER        SafeAllocationSize;
   PVOID                SystemEaBuffer = NULL;
   NTSTATUS		Status = STATUS_SUCCESS;
   
   DPRINT("IoCreateFile(FileHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	  FileHandle,DesiredAccess,ObjectAttributes,
	  ObjectAttributes->ObjectName->Buffer);
   
   ASSERT_IRQL(PASSIVE_LEVEL);

   if (IoStatusBlock == NULL || FileHandle == NULL)
     return STATUS_ACCESS_VIOLATION;

   LocalFileHandle = 0;

   if(Options & IO_NO_PARAMETER_CHECKING)
     AccessMode = KernelMode;
   else
     AccessMode = ExGetPreviousMode();
   
   if(AccessMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(FileHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
       ProbeForWrite(IoStatusBlock,
                     sizeof(IO_STATUS_BLOCK),
                     sizeof(ULONG));
       if(AllocationSize != NULL)
       {
         ProbeForRead(AllocationSize,
                      sizeof(LARGE_INTEGER),
                      sizeof(ULONG));
         SafeAllocationSize = *AllocationSize;
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

   Status = ObCreateObject(AccessMode,
			   IoFileObjectType,
			   ObjectAttributes,
			   AccessMode,
			   NULL,
			   sizeof(FILE_OBJECT),
			   0,
			   0,
			   (PVOID*)&FileObject);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObCreateObject() failed! (Status %lx)\n", Status);
	return Status;
     }

   RtlMapGenericMask(&DesiredAccess,
                     BODY_TO_HEADER(FileObject)->ObjectType->Mapping);

   Status = ObInsertObject ((PVOID)FileObject,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    &LocalFileHandle);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObInsertObject() failed! (Status %lx)\n", Status);
	ObDereferenceObject (FileObject);
	return Status;
     }

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

   SecurityContext.SecurityQos = NULL; /* ?? */
   SecurityContext.AccessState = NULL; /* ?? */
   SecurityContext.DesiredAccess = DesiredAccess;
   SecurityContext.FullCreateOptions = 0; /* ?? */
   
   KeInitializeEvent(&FileObject->Lock, SynchronizationEvent, TRUE);
   KeInitializeEvent(&FileObject->Event, NotificationEvent, FALSE);
   
   DPRINT("FileObject %x\n", FileObject);
   DPRINT("FileObject->DeviceObject %x\n", FileObject->DeviceObject);
   /*
    * Create a new IRP to hand to
    * the FS driver: this may fail
    * due to resource shortage.
    */
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, FALSE);
   if (Irp == NULL)
     {
	ZwClose(LocalFileHandle);
	return STATUS_UNSUCCESSFUL;
     }

   //trigger FileObject/Event dereferencing
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = AccessMode;
   Irp->UserIosb = &LocalIoStatusBlock;
   Irp->AssociatedIrp.SystemBuffer = SystemEaBuffer;
   Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = &FileObject->Event;
   Irp->Overlay.AllocationSize = SafeAllocationSize;
   
   /*
    * Get the stack location for the new
    * IRP and prepare it.
    */
   StackLoc = IoGetNextIrpStackLocation(Irp);
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
   
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&FileObject->Event,
			      Executive,
			      AccessMode,
			      FALSE,
			      NULL);
	Status = LocalIoStatusBlock.Status;
     }
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failing create request with status %x\n", Status);
        FileObject->DeviceObject = NULL;
        FileObject->Vpb = NULL;

	ZwClose(LocalFileHandle);
     }
   else
     {
	 _SEH_TRY
	   {
	      *FileHandle = LocalFileHandle;
	      *IoStatusBlock = LocalIoStatusBlock;
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

   DPRINT("Finished IoCreateFile() (*FileHandle) %x\n", (*FileHandle));

   return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtCreateFile@44
 * 
 * DESCRIPTION
 *	Entry point to call IoCreateFile with
 *	default parameters.
 *
 * ARGUMENTS
 * 	See IoCreateFile.
 * 
 * RETURN VALUE
 * 	See IoCreateFile.
 *
 * REVISIONS
 * 	2000-03-25 (ea)
 * 		Code originally in NtCreateFile moved in IoCreateFile.
 *
 * @implemented
 */
NTSTATUS STDCALL
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


/**********************************************************************
 * NAME							EXPORTED
 * 	NtOpenFile@24
 * 	
 * DESCRIPTION
 * 	Opens an existing file (simpler than NtCreateFile).
 *
 * ARGUMENTS
 *	FileHandle (OUT)
 *		Variable that receives the file handle on return;
 *		
 *	DesiredAccess
 *		Access desired by the caller to the file;
 *		
 *	ObjectAttributes
 *		Structue describing the file to be opened;
 *		
 *	IoStatusBlock (OUT)
 *		Receives details about the result of the
 *		operation;
 *		
 *	ShareAccess
 *		Type of shared access the caller requires;
 *		
 *	OpenOptions
 *		Options for the file open.
 *
 * RETURN VALUE
 * 	Status.
 * 	
 * NOTE
 * 	Undocumented.
 *
 * @implemented
 */
NTSTATUS STDCALL
NtOpenFile(PHANDLE FileHandle,
	   ACCESS_MASK DesiredAccess,
	   POBJECT_ATTRIBUTES ObjectAttributes,
	   PIO_STATUS_BLOCK IoStatusBlock,
	   ULONG ShareAccess,
	   ULONG OpenOptions)
{
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

/* EOF */
