/* $Id: create.c,v 1.50 2001/11/02 22:22:33 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/create.c
 * PURPOSE:         Handling file create/open apis
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  24/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/id.h>
#include <internal/pool.h>

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
 */
NTSTATUS STDCALL
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
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
 * 
 */
NTSTATUS STDCALL
IopCreateFile(PVOID			ObjectBody,
	      PVOID			Parent,
	      PWSTR			RemainingPath,
	      POBJECT_ATTRIBUTES	ObjectAttributes)
{
   PDEVICE_OBJECT	DeviceObject = (PDEVICE_OBJECT) Parent;
   PFILE_OBJECT	FileObject = (PFILE_OBJECT) ObjectBody;
   NTSTATUS	Status;
   
   DPRINT("IopCreateFile(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody,
	  Parent,
	  RemainingPath);
   
   if (NULL == DeviceObject)
     {
	/* This is probably an attempt to create a meta fileobject (eg. for FAT)
	   for the cache manager, so return STATUS_SUCCESS */
	DPRINT("DeviceObject was NULL\n");
	return (STATUS_SUCCESS);
     }
   if (IoDeviceObjectType != BODY_TO_HEADER(Parent)->ObjectType)
     {
	CPRINT("Parent is a %S which is not a device type\n",
	       BODY_TO_HEADER(Parent)->ObjectType->TypeName.Buffer);
	return (STATUS_UNSUCCESSFUL);
     }
   Status = ObReferenceObjectByPointer (DeviceObject,
					STANDARD_RIGHTS_REQUIRED,
					IoDeviceObjectType,
					UserMode);
   if (STATUS_SUCCESS != Status)
     {
	CHECKPOINT1;
	return (Status);
     }
   
   DeviceObject = IoGetAttachedDevice (DeviceObject);
   
   DPRINT("DeviceObject %x\n", DeviceObject);
   
   if (NULL == RemainingPath)
     {
	FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
	FileObject->FileName.Buffer = 
	  ExAllocatePoolWithTag(NonPagedPool,
				(ObjectAttributes->ObjectName->Length+1)*sizeof(WCHAR),
				TAG_FILE_NAME);
	FileObject->FileName.Length = ObjectAttributes->ObjectName->Length;
	FileObject->FileName.MaximumLength = 
	  ObjectAttributes->ObjectName->MaximumLength;
	RtlCopyUnicodeString(&(FileObject->FileName),
			     ObjectAttributes->ObjectName);
     }
   else
     {
	if ((DeviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM)
	    && (DeviceObject->DeviceType != FILE_DEVICE_DISK)
	    && (DeviceObject->DeviceType != FILE_DEVICE_NETWORK)
	    && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
	    && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
	  {
	     CPRINT ("Device was wrong type\n");
	     return (STATUS_UNSUCCESSFUL);
	  }

	if (DeviceObject->DeviceType != FILE_DEVICE_NETWORK
	    && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
	    && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
	  {
	     if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
	       {
		  DPRINT("Trying to mount storage device\n");
		  Status = IoTryToMountStorageDevice (DeviceObject);
		  DPRINT("Status %x\n", Status);
		  if (!NT_SUCCESS(Status))
		    {
		       CPRINT("Failed to mount storage device (statux %x)\n",
			      Status);
		       return (Status);
		    }
		  DeviceObject = IoGetAttachedDevice(DeviceObject);
	       }
	  }
	RtlCreateUnicodeString(&(FileObject->FileName),
			       RemainingPath);
     }
   DPRINT("FileObject->FileName.Buffer %S\n",
	  FileObject->FileName.Buffer);
   FileObject->DeviceObject = DeviceObject;
   DPRINT("FileObject %x DeviceObject %x\n",
	  FileObject,
	  DeviceObject);
   FileObject->Vpb = DeviceObject->Vpb;
   FileObject->Type = InternalFileType;
   
   return (STATUS_SUCCESS);
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
 */
PFILE_OBJECT STDCALL
IoCreateStreamFileObject(PFILE_OBJECT FileObject,
			 PDEVICE_OBJECT DeviceObject)
{
  HANDLE		FileHandle;
  PFILE_OBJECT	CreatedFileObject;
  NTSTATUS Status;

  DPRINT("IoCreateStreamFileObject(FileObject %x, DeviceObject %x)\n",
	   FileObject, DeviceObject);
   
  assert_irql (PASSIVE_LEVEL);

  Status = ObCreateObject (&FileHandle,
			   STANDARD_RIGHTS_REQUIRED,
			   NULL,
			   IoFileObjectType,
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

  CreatedFileObject->DeviceObject = DeviceObject;
  CreatedFileObject->Vpb = DeviceObject->Vpb;
  CreatedFileObject->Type = InternalFileType;
  CreatedFileObject->Flags |= FO_DIRECT_DEVICE_OPEN;
  
  ZwClose (FileHandle);
  
  return (CreatedFileObject);
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
 */
NTSTATUS STDCALL
IoCreateFile(
	OUT	PHANDLE			FileHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
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
   NTSTATUS		Status;
   PIRP			Irp;
   KEVENT			Event;
   PIO_STACK_LOCATION	StackLoc;
   IO_STATUS_BLOCK      IoSB;
   
   DPRINT("IoCreateFile(FileHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %S)\n",
	  FileHandle,DesiredAccess,ObjectAttributes,
	  ObjectAttributes->ObjectName->Buffer);
   
   assert_irql(PASSIVE_LEVEL);
   
   *FileHandle = 0;

   Status = ObCreateObject(FileHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   IoFileObjectType,
			   (PVOID*)&FileObject);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObCreateObject() failed!\n");
	return (Status);
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
     {
	FileObject->Flags |= (FO_ALERTABLE_IO | FO_SYNCHRONOUS_IO);
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)
     {
	FileObject->Flags |= FO_SYNCHRONOUS_IO;
     }
   KeInitializeEvent(&FileObject->Lock, NotificationEvent, TRUE);
   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   
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
	ZwClose(*FileHandle);
	return (STATUS_UNSUCCESSFUL);
     }
     
   Irp->UserIosb = &IoSB;   //return iostatus
   Irp->AssociatedIrp.SystemBuffer = EaBuffer;
   Irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR)ExtraCreateParameters;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = &Event;
   
   /*
    * Get the stack location for the new
    * IRP and prepare it.
    */
   StackLoc = IoGetNextIrpStackLocation(Irp);
   switch (CreateFileType)
     {
	default:
	case CreateFileTypeNone:
	  StackLoc->MajorFunction = IRP_MJ_CREATE;
	  break;
	
	case CreateFileTypeNamedPipe:
	  StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
	  break;

	case CreateFileTypeMailslot:
	  StackLoc->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
	  break;
     }
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = 0;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = FileObject->DeviceObject;
   StackLoc->FileObject = FileObject;
   StackLoc->Parameters.Create.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
   StackLoc->Parameters.Create.Options |= (CreateDisposition << 24);
   
   /*
    * Now call the driver and 
    * possibly wait if it can
    * not complete the request
    * immediately.
    */
   Status = IofCallDriver(FileObject->DeviceObject, Irp );
   
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoSB.Status;
     }
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failing create request with status %x\n", Status);
	ZwClose(*FileHandle);
     }
   if (IoStatusBlock)
     {
       *IoStatusBlock = IoSB;
     }
   assert_irql(PASSIVE_LEVEL);

   DPRINT("Finished IoCreateFile() (*FileHandle) %x\n", (*FileHandle));

   return (Status);
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
