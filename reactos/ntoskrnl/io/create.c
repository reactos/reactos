/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/create.c
 * PURPOSE:         Handling file create/open apis
 * PROGRAMMER:      David Welch (welch@cwcom.net) 
 * UPDATE HISTORY:
 *                  24/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

NTSTATUS STDCALL NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwDeleteFile(ObjectAttributes));
}

NTSTATUS STDCALL ZwDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS NtCreateFile(PHANDLE FileHandle,
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
   return(ZwCreateFile(FileHandle,
		       DesiredAccess,
		       ObjectAttributes,
		       IoStatusBlock,
		       AllocateSize,
		       FileAttributes,
		       ShareAccess,
		       CreateDisposition,
		       CreateOptions,
		       EaBuffer,
		       EaLength));
}

NTSTATUS IopCreateFile(PVOID ObjectBody,
		       PVOID Parent,
		       PWSTR RemainingPath,
		       POBJECT_ATTRIBUTES ObjectAttributes)
{
   PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Parent;
   PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
   NTSTATUS Status;
   
   DPRINT("IopCreateFile(ObjectBody %x, Parent %x, RemainingPath %w)\n",
	  ObjectBody,Parent,RemainingPath);
   
   Status = ObReferenceObjectByPointer(DeviceObject,
				       STANDARD_RIGHTS_REQUIRED,
				       IoDeviceType,
				       UserMode);
   if (Status != STATUS_SUCCESS)
     {
	CHECKPOINT;
	return(Status);
     }
   
   DeviceObject = IoGetAttachedDevice(DeviceObject);
   
   DPRINT("DeviceObject %x\n",DeviceObject);
   
   if (RemainingPath == NULL)
     {
	FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
	FileObject->FileName.Buffer = ExAllocatePool(NonPagedPool,
				   (ObjectAttributes->ObjectName->Length+1)*2);
	FileObject->FileName.Length = ObjectAttributes->ObjectName->Length;
	FileObject->FileName.MaximumLength = 
          ObjectAttributes->ObjectName->MaximumLength;
	RtlCopyUnicodeString(&(FileObject->FileName),
			     ObjectAttributes->ObjectName);
     }
   else
     {
	if (DeviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM &&
	    DeviceObject->DeviceType != FILE_DEVICE_DISK)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
	  {
	     Status = IoTryToMountStorageDevice(DeviceObject);
	     if (Status!=STATUS_SUCCESS)
	       {
		  return(Status);
	       }
	     DeviceObject = IoGetAttachedDevice(DeviceObject);
	  }
	RtlInitUnicodeString(&(FileObject->FileName),wstrdup(RemainingPath));
     }
   DPRINT("FileObject->FileName.Buffer %w\n",FileObject->FileName.Buffer);
   FileObject->DeviceObject=DeviceObject;
   FileObject->Vpb=DeviceObject->Vpb;
   
   return(STATUS_SUCCESS);
}

NTSTATUS ZwCreateFile(PHANDLE FileHandle,
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
/*
 * FUNCTION: Either causes a new file or directory to be created, or it opens
 * an existing file, device, directory or volume, giving the caller a handle
 * for the file object. This handle can be used by subsequent calls to
 * manipulate data within the file or the file object's state of attributes.
 * ARGUMENTS:
 *        FileHandle (OUT) = Points to a variable which receives the file
 *                           handle on return
 *        DesiredAccess = Desired access to the file
 *        ObjectAttributes = Structure describing the file
 *        IoStatusBlock (OUT) = Receives information about the operation on
 *                              return
 *        AllocationSize = Initial size of the file in bytes
 *        FileAttributes = Attributes to create the file with
 *        ShareAccess = Type of shared access the caller would like to the file
 *        CreateDisposition = Specifies what to do, depending on whether the
 *                            file already exists
 *        CreateOptions = Options for creating a new file
 *        EaBuffer = Undocumented
 *        EaLength = Undocumented
 * RETURNS: Status
 */
{
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PIRP Irp;
   KEVENT Event;
   PIO_STACK_LOCATION StackLoc;
   
   DPRINT("ZwCreateFile(FileHandle %x, DesiredAccess %x, "
	    "ObjectAttributes %x ObjectAttributes->ObjectName->Buffer %w)\n",
	    FileHandle,DesiredAccess,ObjectAttributes,
	    ObjectAttributes->ObjectName->Buffer);   
   
   assert_irql(PASSIVE_LEVEL);
   
   *FileHandle=0;

   FileObject = ObCreateObject(FileHandle,
			       DesiredAccess,
			       ObjectAttributes,
			       IoFileType);
   if (FileObject == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
     {
	FileObject->Flags = FileObject->Flags | FO_ALERTABLE_IO;
	FileObject->Flags = FileObject->Flags | FO_SYNCHRONOUS_IO;
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)
     {
	FileObject->Flags = FileObject->Flags | FO_SYNCHRONOUS_IO;
     }
      
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, FALSE);
   if (Irp==NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   StackLoc = IoGetNextIrpStackLocation(Irp);
   StackLoc->MajorFunction = IRP_MJ_CREATE;
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = 0;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = FileObject->DeviceObject;
   StackLoc->FileObject = FileObject;
   StackLoc->Parameters.Create.Options = CreateOptions&FILE_VALID_OPTION_FLAGS;
   StackLoc->Parameters.Create.Options |= CreateDisposition<<24;
   
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
	Status = IoStatusBlock->Status;
     }
   
   if (!NT_SUCCESS(Status))
     {
	ZwClose(*FileHandle);
	(*FileHandle) = 0;
     }
   
   DPRINT("(*FileHandle) %x\n",(*FileHandle));
   DPRINT("Finished ZwCreateFile()\n");
   return(Status);
}

NTSTATUS NtOpenFile(PHANDLE FileHandle,
		    ACCESS_MASK DesiredAccess,
		    POBJECT_ATTRIBUTES ObjectAttributes,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    ULONG ShareAccess,
		    ULONG OpenOptions)
{
   return(ZwOpenFile(FileHandle,
		     DesiredAccess,
		     ObjectAttributes,
		     IoStatusBlock,
		     ShareAccess,
		     OpenOptions));
}

NTSTATUS ZwOpenFile(PHANDLE FileHandle,
		    ACCESS_MASK DesiredAccess,
		    POBJECT_ATTRIBUTES ObjectAttributes,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    ULONG ShareAccess,
		    ULONG OpenOptions)
/*
 * FUNCTION: Opens a file (simpler than ZwCreateFile)
 * ARGUMENTS:
 *       FileHandle (OUT) = Variable that receives the file handle on return
 *       DesiredAccess = Access desired by the caller to the file
 *       ObjectAttributes = Structue describing the file to be opened
 *       IoStatusBlock (OUT) = Receives details about the result of the
 *                             operation
 *       ShareAccess = Type of shared access the caller requires
 *       OpenOptions = Options for the file open
 * RETURNS: Status
 * NOTE: Undocumented
 */
{
   return(ZwCreateFile(FileHandle,
		       DesiredAccess,
		       ObjectAttributes,
		       IoStatusBlock,
		       NULL,
		       0,
		       ShareAccess,
		       FILE_OPEN,
		       OpenOptions,
		       NULL,
		       0));
}


