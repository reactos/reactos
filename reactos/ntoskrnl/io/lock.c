/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


#define TAG_LOCK			TAG('F','l','c','k')

/* FUNCTIONS *****************************************************************/

static NTSTATUS STDCALL
IopLockFileCompletionRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
  ExFreePool(Context);
  return STATUS_SUCCESS;
  // FIXME: Should I call IoFreeIrp and return STATUS_MORE_PROCESSING_REQUIRED?
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtLockFile (
	IN	HANDLE			FileHandle,
	IN	HANDLE			EventHandle OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID			ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	PLARGE_INTEGER		ByteOffset,
	IN	PLARGE_INTEGER		Length,
	IN	PULONG			Key,
	IN	BOOLEAN			FailImmediatedly,
	IN	BOOLEAN			ExclusiveLock
	)
{
  PFILE_OBJECT FileObject = NULL;
  PLARGE_INTEGER LocalLength = NULL;
  PKEVENT Event = NULL;
  PIRP Irp = NULL;
  PIO_STACK_LOCATION StackPtr;
  PDEVICE_OBJECT DeviceObject;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;

  // FIXME: instead of this, use SEH when available?
  if (!Length || !ByteOffset)
  {
    Status = STATUS_INVALID_PARAMETER;
    goto fail;
  }

  PreviousMode = ExGetPreviousMode();

  Status = ObReferenceObjectByHandle(FileHandle,
				     0,
				     IoFileObjectType,
				     PreviousMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
  {
    goto fail;
  }

  DeviceObject = IoGetRelatedDeviceObject(FileObject);

  Irp = IoAllocateIrp(DeviceObject->StackSize,
		      TRUE);
  if (Irp == NULL)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto fail;
  }

  if (EventHandle != NULL && !FailImmediatedly)
  {
    Status = ObReferenceObjectByHandle(EventHandle,
				       SYNCHRONIZE,
				       ExEventObjectType,
				       PreviousMode,
				       (PVOID*)&Event,
				       NULL);
    if (!NT_SUCCESS(Status))
    {
      goto fail;
    }
  }
  else
  {
    Event = &FileObject->Event;
    KeResetEvent(Event);
  }

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->RequestorMode = PreviousMode;
  Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

  Irp->UserEvent = Event;
  Irp->UserIosb = IoStatusBlock;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
  StackPtr->MinorFunction = IRP_MN_LOCK;
  StackPtr->FileObject = FileObject;

  if (ExclusiveLock)
    StackPtr->Flags |= SL_EXCLUSIVE_LOCK;

  if (FailImmediatedly)
    StackPtr->Flags |= SL_FAIL_IMMEDIATELY;

  LocalLength = ExAllocatePoolWithTag(NonPagedPool,
				      sizeof(LARGE_INTEGER),
				      TAG_LOCK);
  if (!LocalLength)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto fail;
  }

  *LocalLength = *Length;

  StackPtr->Parameters.LockControl.Length = LocalLength;
  StackPtr->Parameters.LockControl.ByteOffset = *ByteOffset;
  StackPtr->Parameters.LockControl.Key = Key ? *Key : 0;

  IoSetCompletionRoutine(Irp,
			 IopLockFileCompletionRoutine,
			 LocalLength,
			 TRUE,
			 TRUE,
			 TRUE);

  /* Can't touch FileObject after IoCallDriver since it might be freed */
  Status = IofCallDriver(DeviceObject, Irp);
  if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
  {
    Status = KeWaitForSingleObject(Event,
				   Executive,
				   PreviousMode,
				   FileObject->Flags & FO_ALERTABLE_IO,
				   NULL);

    if (Status != STATUS_WAIT_0)
    {
      DPRINT1("NtLockFile -> KeWaitForSingleObject failed!\n");
      /*
       * FIXME: Should do some special processing here if alertable wait
       * was interupted by user apc or a thread alert (STATUS_ALERTED, STATUS_USER_APC)
       */
      return Status; /* Set status to something else? */
    }

    Status = IoStatusBlock->Status;
  }

  return Status;

fail:;
  if (LocalLength)
    ExFreePool(LocalLength);

  if (Irp)
    IoFreeIrp(Irp);

  if (Event)
    ObDereferenceObject(Event);

  if (FileObject)
    ObDereferenceObject(FileObject);

  return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtUnlockFile (
	IN	HANDLE			FileHandle,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	PLARGE_INTEGER		ByteOffset,
	IN	PLARGE_INTEGER		Length,
	OUT	PULONG			Key OPTIONAL
	)
{
  PFILE_OBJECT FileObject = NULL;
  PLARGE_INTEGER LocalLength = NULL;
  PIRP Irp = NULL;
  PIO_STACK_LOCATION StackPtr;
  PDEVICE_OBJECT DeviceObject;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;

  // FIXME: instead of this, use SEH when available
  if (!Length || !ByteOffset)
  {
    Status = STATUS_INVALID_PARAMETER;
    goto fail;
  }

  PreviousMode = ExGetPreviousMode();

  /*
   * BUGBUG: ObReferenceObjectByHandle fails if DesiredAccess=0 and mode=UserMode
   * It should ONLY fail if we desire an access that conflict with granted access!
   */
  Status = ObReferenceObjectByHandle(FileHandle,
				     0, //FILE_READ_DATA,//BUGBUG: have to use something...but shouldn't have to!
				     IoFileObjectType,
				     PreviousMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
  {
    goto fail;
  }

  DeviceObject = IoGetRelatedDeviceObject(FileObject);

  Irp = IoAllocateIrp(DeviceObject->StackSize,
		      TRUE);
  if (Irp == NULL)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto fail;
  }

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;
  Irp->RequestorMode = PreviousMode;
  Irp->UserIosb = IoStatusBlock;
  Irp->Tail.Overlay.Thread = PsGetCurrentThread();

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
  StackPtr->MinorFunction = IRP_MN_UNLOCK_SINGLE;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->FileObject = FileObject;

  LocalLength = ExAllocatePoolWithTag(NonPagedPool,
				      sizeof(LARGE_INTEGER),
				      TAG_LOCK);
  if (!LocalLength)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto fail;
  }

  *LocalLength = *Length;

  StackPtr->Parameters.LockControl.Length = LocalLength;
  StackPtr->Parameters.LockControl.ByteOffset = *ByteOffset;
  StackPtr->Parameters.LockControl.Key = Key ? *Key : 0;

  /* Allways synchronous */
  Status = IofCallDriver(DeviceObject, Irp);

  ExFreePool(LocalLength);

  return Status;

fail:;
  if (LocalLength)
    ExFreePool(LocalLength);

  if (Irp)
    IoFreeIrp(Irp);

  if (FileObject)
    ObDereferenceObject(FileObject);

  return Status;
}
