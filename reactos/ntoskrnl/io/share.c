/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/share.c
 * PURPOSE:         No purpose listed.
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
IoUpdateShareAccess(PFILE_OBJECT FileObject,
		    PSHARE_ACCESS ShareAccess)
{
   if ((FileObject->ReadAccess == FALSE) &&
       (FileObject->WriteAccess == FALSE) &&
       (FileObject->DeleteAccess == FALSE))
     {
	return;
     }

   ShareAccess->OpenCount++;

   if (FileObject->ReadAccess == TRUE)
     {
	ShareAccess->Readers++;
     }

   if (FileObject->WriteAccess == TRUE)
     {
	ShareAccess->Writers++;
     }

   if (FileObject->DeleteAccess == TRUE)
     {
	ShareAccess->Deleters++;
     }

   if (FileObject->SharedRead == TRUE)
     {
	ShareAccess->SharedRead++;
     }

   if (FileObject->SharedWrite == TRUE)
     {
	ShareAccess->SharedWrite++;
     }

   if (FileObject->SharedDelete == TRUE)
     {
	ShareAccess->SharedDelete++;
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

  ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE));
  WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA));
  DeleteAccess = (DesiredAccess & DELETE);

  FileObject->ReadAccess = ReadAccess;
  FileObject->WriteAccess = WriteAccess;
  FileObject->DeleteAccess = DeleteAccess;

  if (!ReadAccess && !WriteAccess && !DeleteAccess)
    {
      return(STATUS_SUCCESS);
    }

  SharedRead = (DesiredShareAccess & FILE_SHARE_READ);
  SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE);
  SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE);

  FileObject->SharedRead = SharedRead;
  FileObject->SharedWrite = SharedWrite;
  FileObject->SharedDelete = SharedDelete;

  if (ReadAccess)
    {
      if (ShareAccess->SharedRead < ShareAccess->OpenCount)
	return(STATUS_SHARING_VIOLATION);
    }

  if (WriteAccess)
    {
      if (ShareAccess->SharedWrite < ShareAccess->OpenCount)
	return(STATUS_SHARING_VIOLATION);
    }

  if (DeleteAccess)
    {
      if (ShareAccess->SharedDelete < ShareAccess->OpenCount)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Readers != 0)
    {
      if (SharedRead == FALSE)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Writers != 0)
    {
      if (SharedWrite == FALSE)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Deleters != 0)
    {
      if (SharedDelete == FALSE)
	return(STATUS_SHARING_VIOLATION);
    }

  if (Update == TRUE)
    {
      ShareAccess->OpenCount++;

      if (ReadAccess == TRUE)
	ShareAccess->Readers++;

      if (WriteAccess == TRUE)
	ShareAccess->Writers++;

      if (DeleteAccess == TRUE)
	ShareAccess->Deleters++;

      if (SharedRead == TRUE)
	ShareAccess->SharedRead++;

      if (SharedWrite == TRUE)
	ShareAccess->SharedWrite++;

      if (SharedDelete == TRUE)
	ShareAccess->SharedDelete++;
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
  if ((FileObject->ReadAccess == FALSE) &&
      (FileObject->WriteAccess == FALSE) &&
      (FileObject->DeleteAccess == FALSE))
    {
      return;
    }

  ShareAccess->OpenCount--;

  if (FileObject->ReadAccess == TRUE)
    {
      ShareAccess->Readers--;
    }

  if (FileObject->WriteAccess == TRUE)
    {
      ShareAccess->Writers--;
    }

  if (FileObject->DeleteAccess == TRUE)
    {
      ShareAccess->Deleters--;
    }

  if (FileObject->SharedRead == TRUE)
    {
      ShareAccess->SharedRead--;
    }

  if (FileObject->SharedWrite == TRUE)
    {
      ShareAccess->SharedWrite--;
    }

  if (FileObject->SharedDelete == TRUE)
    {
      ShareAccess->SharedDelete--;
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

  ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE));
  WriteAccess = (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA));
  DeleteAccess = (DesiredAccess & DELETE);

  FileObject->ReadAccess = ReadAccess;
  FileObject->WriteAccess = WriteAccess;
  FileObject->DeleteAccess = DeleteAccess;

  if (!ReadAccess && !WriteAccess && !DeleteAccess)
    {
      FileObject->SharedRead = FALSE;
      FileObject->SharedWrite = FALSE;
      FileObject->SharedDelete = FALSE;

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
      SharedRead = (DesiredShareAccess & FILE_SHARE_READ);
      SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE);
      SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE);

      FileObject->SharedRead = SharedRead;
      FileObject->SharedWrite = SharedWrite;
      FileObject->SharedDelete = SharedDelete;

      ShareAccess->OpenCount = 1;
      ShareAccess->Readers = (ReadAccess) ? 1 : 0;
      ShareAccess->Writers = (WriteAccess) ? 1 : 0;
      ShareAccess->Deleters = (DeleteAccess) ? 1 : 0;

      ShareAccess->SharedRead = (SharedRead) ? 1 : 0;
      ShareAccess->SharedWrite = (SharedWrite) ? 1 : 0;
      ShareAccess->SharedDelete = (SharedDelete) ? 1 : 0;
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
		     IN ACCESS_MASK GrantedAccess)
{
  PAGED_CODE();
  
  RtlMapGenericMask(DesiredAccess,
		    IoFileObjectType->Mapping);

  if ((~(*DesiredAccess) & GrantedAccess) != 0)
    return STATUS_ACCESS_DENIED;
  else
    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
			IN ULONG EaLength,
			OUT PULONG ErrorOffset)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoCheckFunctionAccess(IN ACCESS_MASK GrantedAccess,
		      IN UCHAR MajorFunction,
		      IN UCHAR MinorFunction,
		      IN ULONG IoControlCode,
		      IN PVOID ExtraData OPTIONAL,
		      IN PVOID ExtraData2 OPTIONAL)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
IoSetInformation(IN PFILE_OBJECT FileObject,
		 IN FILE_INFORMATION_CLASS FileInformationClass,
		 IN ULONG Length,
		 OUT PVOID FileInformation)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;

   ASSERT(FileInformation != NULL);

   if (FileInformationClass == FileCompletionInformation)
   {
      return STATUS_NOT_IMPLEMENTED;
   }



   Status = ObReferenceObjectByPointer(FileObject,
				       FILE_WRITE_ATTRIBUTES,
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   DPRINT("FileObject %x\n", FileObject);
   
   DeviceObject = FileObject->DeviceObject;

   Irp = IoAllocateIrp(DeviceObject->StackSize,
		       TRUE);
   if (Irp == NULL)
   {
      ObDereferenceObject(FileObject);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = KernelMode;
   Irp->AssociatedIrp.SystemBuffer = FileInformation;
   Irp->UserIosb = &IoStatusBlock;
   Irp->UserEvent = &FileObject->Event;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   KeResetEvent( &FileObject->Event );

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
   StackPtr->MinorFunction = 0;
   StackPtr->Flags = 0;
   StackPtr->Control = 0;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
   StackPtr->Parameters.SetFile.Length = Length;

   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   if (Status==STATUS_PENDING)
   {
      KeWaitForSingleObject(&FileObject->Event,
			    Executive,
			    KernelMode,
			    FileObject->Flags & FO_ALERTABLE_IO,
			    NULL);
      Status = IoStatusBlock.Status;
   }
   
   return Status;
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

/* EOF */
