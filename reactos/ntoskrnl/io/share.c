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
 * @implemented
 */
NTSTATUS STDCALL
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
		     IN ACCESS_MASK GrantedAccess)
{
  PAGED_CODE();

  RtlMapGenericMask(DesiredAccess,
		    &IoFileObjectType->TypeInfo.GenericMapping);

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
				       0, /* FIXME - depends on the information class */
				       IoFileObjectType,
				       KernelMode);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   DPRINT("FileObject 0x%p\n", FileObject);

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
