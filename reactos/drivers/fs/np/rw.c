/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ntifs.h>
#include <rosrtl/minmax.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#ifndef NDEBUG
VOID HexDump(PUCHAR Buffer, ULONG Length)
{
  CHAR Line[65];
  UCHAR ch;
  const char Hex[] = "0123456789ABCDEF";
  int i, j;

  DbgPrint("---------------\n");

  for (i = 0; i < ROUND_UP(Length, 16); i+= 16)
    {
      memset(Line, ' ', 64);
      Line[64] = 0;

      for (j = 0; j < 16 && j + i < Length; j++)
        {
          ch = Buffer[i + j];
          Line[3*j + 0] = Hex[ch >> 4];
	  Line[3*j + 1] = Hex[ch & 0x0f];
	  Line[48 + j] = isprint(ch) ? ch : '.';
        }
      DbgPrint("%s\n", Line);
    }
  DbgPrint("---------------\n");
}
#endif

static VOID STDCALL
NpfsReadWriteCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
   PNPFS_CONTEXT Context;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PIO_STACK_LOCATION IoStack;
   PNPFS_FCB Fcb;
   BOOLEAN Complete = FALSE;

   DPRINT("NpfsReadWriteCancelRoutine(DeviceObject %x, Irp %x)\n", DeviceObject, Irp);

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   Fcb = IoStack->FileObject->FsContext;

   KeLockMutex(&DeviceExt->PipeListLock);
   ExAcquireFastMutex(&Fcb->DataListLock);
   switch(IoStack->MajorFunction)
   {
      case IRP_MJ_READ:
         if (Fcb->ReadRequestListHead.Flink != &Context->ListEntry)
	 {
	    /* we are not the first in the list, remove an complete us */
	    RemoveEntryList(&Context->ListEntry);
	    Complete = TRUE;
	 }
	 else
	 {
	    KeSetEvent(&Fcb->ReadEvent, IO_NO_INCREMENT, FALSE);
	 }
	 break;
      default:
         KEBUGCHECK(0);
   }
   ExReleaseFastMutex(&Fcb->DataListLock);
   KeUnlockMutex(&DeviceExt->PipeListLock);
   if (Complete)
   {
      Irp->IoStatus.Status = STATUS_CANCELLED;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }
}

static VOID STDCALL
NpfsWaiterThread(PVOID InitContext)
{
   PNPFS_THREAD_CONTEXT ThreadContext = (PNPFS_THREAD_CONTEXT) InitContext;
   ULONG CurrentCount;
   ULONG Count = 0;
   PIRP Irp = NULL;
   PIRP NextIrp;
   NTSTATUS Status;
   BOOLEAN Terminate = FALSE;
   BOOLEAN Cancel = FALSE;
   PIO_STACK_LOCATION IoStack = NULL;
   PNPFS_CONTEXT Context;
   PNPFS_CONTEXT NextContext;
   PNPFS_FCB Fcb;

   KeLockMutex(&ThreadContext->DeviceExt->PipeListLock);

   while (1)
     {
       CurrentCount = ThreadContext->Count;
       KeUnlockMutex(&ThreadContext->DeviceExt->PipeListLock);
       if (Irp)
         {
           if (Cancel)
             {
	       Irp->IoStatus.Status = STATUS_CANCELLED;
               Irp->IoStatus.Information = 0;
               IoCompleteRequest(Irp, IO_NO_INCREMENT);
	     }
	   else
	     {
               switch (IoStack->MajorFunction)
	         {
	           case IRP_MJ_READ:
                     NpfsRead(IoStack->DeviceObject, Irp);
		     break;
		   default:
		     KEBUGCHECK(0);
		 }
	     }
         }
       if (Terminate)
         {
	   break;
	 }
       Status = KeWaitForMultipleObjects(CurrentCount,
	                                 ThreadContext->WaitObjectArray,
					 WaitAny,
					 Executive,
					 KernelMode,
					 FALSE,
					 NULL,
					 ThreadContext->WaitBlockArray);
       if (!NT_SUCCESS(Status))
         {
           KEBUGCHECK(0);
         }
       KeLockMutex(&ThreadContext->DeviceExt->PipeListLock);
       Count = Status - STATUS_SUCCESS;
       ASSERT (Count < CurrentCount);
       if (Count > 0)
       {
	  Irp = ThreadContext->WaitIrpArray[Count];
	  ThreadContext->Count--;
	  ThreadContext->DeviceExt->EmptyWaiterCount++;
	  ThreadContext->WaitObjectArray[Count] = ThreadContext->WaitObjectArray[ThreadContext->Count];
	  ThreadContext->WaitIrpArray[Count] = ThreadContext->WaitIrpArray[ThreadContext->Count];

          Cancel = (NULL == IoSetCancelRoutine(Irp, NULL));
	  Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;
	  IoStack = IoGetCurrentIrpStackLocation(Irp);

	  if (Cancel)
	  {
	     Fcb = IoStack->FileObject->FsContext;
 	     ExAcquireFastMutex(&Fcb->DataListLock);
	     RemoveEntryList(&Context->ListEntry);
	     switch (IoStack->MajorFunction)
	     {
	        case IRP_MJ_READ:
                   if (!IsListEmpty(&Fcb->ReadRequestListHead))
		   {
		      /* put the next request on the wait list */
                      NextContext = CONTAINING_RECORD(Fcb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
		      ThreadContext->WaitObjectArray[ThreadContext->Count] = NextContext->WaitEvent;
		      NextIrp = CONTAINING_RECORD(NextContext, IRP, Tail.Overlay.DriverContext);
		      ThreadContext->WaitIrpArray[ThreadContext->Count] = NextIrp;
		      ThreadContext->Count++;
                      ThreadContext->DeviceExt->EmptyWaiterCount--;
		   }
		   break;
		default:
		   KEBUGCHECK(0);
	     }
	     ExReleaseFastMutex(&Fcb->DataListLock);
	  }
       }
       else
       {
	  /* someone has add a new wait request */
          Irp = NULL;
       }
       if (ThreadContext->Count == 1 && ThreadContext->DeviceExt->EmptyWaiterCount >= MAXIMUM_WAIT_OBJECTS)
        {
          /* it exist an other thread with empty wait slots, we can remove our thread from the list */
          RemoveEntryList(&ThreadContext->ListEntry);
          ThreadContext->DeviceExt->EmptyWaiterCount -= MAXIMUM_WAIT_OBJECTS - 1;
	  Terminate = TRUE;
        }
     }
   KeUnlockMutex(&ThreadContext->DeviceExt->PipeListLock);
   ExFreePool(ThreadContext);
}

static NTSTATUS
NpfsAddWaitingReadWriteRequest(IN PDEVICE_OBJECT DeviceObject,
			       IN PIRP Irp)
{
   PLIST_ENTRY ListEntry;
   PNPFS_THREAD_CONTEXT ThreadContext = NULL;
   NTSTATUS Status;
   HANDLE hThread;
   KIRQL oldIrql;

   PNPFS_CONTEXT Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;
   PNPFS_DEVICE_EXTENSION DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

   DPRINT("NpfsAddWaitingReadWriteRequest(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

   KeLockMutex(&DeviceExt->PipeListLock);

   ListEntry = DeviceExt->ThreadListHead.Flink;
   while (ListEntry != &DeviceExt->ThreadListHead)
     {
       ThreadContext = CONTAINING_RECORD(ListEntry, NPFS_THREAD_CONTEXT, ListEntry);
       if (ThreadContext->Count < MAXIMUM_WAIT_OBJECTS)
         {
           break;
         }
       ListEntry = ListEntry->Flink;
     }
   if (ListEntry == &DeviceExt->ThreadListHead)
     {
       ThreadContext = ExAllocatePool(NonPagedPool, sizeof(NPFS_THREAD_CONTEXT));
       if (ThreadContext == NULL)
         {
           KeUnlockMutex(&DeviceExt->PipeListLock);
           return STATUS_NO_MEMORY;
         }
       ThreadContext->DeviceExt = DeviceExt;
       KeInitializeEvent(&ThreadContext->Event, SynchronizationEvent, FALSE);
       ThreadContext->Count = 1;
       ThreadContext->WaitObjectArray[0] = &ThreadContext->Event;


       DPRINT("Creating a new system thread for waiting read/write requests\n");

       Status = PsCreateSystemThread(&hThread,
		                     THREAD_ALL_ACCESS,
				     NULL,
				     NULL,
				     NULL,
				     NpfsWaiterThread,
				     (PVOID)ThreadContext);
       if (!NT_SUCCESS(Status))
         {
           ExFreePool(ThreadContext);
           KeUnlockMutex(&DeviceExt->PipeListLock);
           return Status;
	 }
       InsertHeadList(&DeviceExt->ThreadListHead, &ThreadContext->ListEntry);
       DeviceExt->EmptyWaiterCount += MAXIMUM_WAIT_OBJECTS - 1;
     }
   IoMarkIrpPending(Irp);

   IoAcquireCancelSpinLock(&oldIrql);
   if (Irp->Cancel)
     {
       IoReleaseCancelSpinLock(oldIrql);
       Status = STATUS_CANCELLED;
     }
   else
     {
       IoSetCancelRoutine(Irp, NpfsReadWriteCancelRoutine);
       IoReleaseCancelSpinLock(oldIrql);
       ThreadContext->WaitObjectArray[ThreadContext->Count] = Context->WaitEvent;
       ThreadContext->WaitIrpArray[ThreadContext->Count] = Irp;
       ThreadContext->Count++;
       DeviceExt->EmptyWaiterCount--;
       KeSetEvent(&ThreadContext->Event, IO_NO_INCREMENT, FALSE);
       Status = STATUS_SUCCESS;
     }
   KeUnlockMutex(&DeviceExt->PipeListLock);
   return Status;
}

NTSTATUS STDCALL
NpfsRead(IN PDEVICE_OBJECT DeviceObject,
	 IN PIRP Irp)
{
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  NTSTATUS OriginalStatus = STATUS_SUCCESS;
  PNPFS_FCB Fcb;
  PNPFS_CONTEXT Context;
  KEVENT Event;
  ULONG Length;
  ULONG Information;
  ULONG CopyLength;
  ULONG TempLength;
  BOOLEAN IsOriginalRequest = TRUE;
  PVOID Buffer;

  DPRINT("NpfsRead(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

  if (Irp->MdlAddress == NULL)
  {
     DPRINT("Irp->MdlAddress == NULL\n");
     Status = STATUS_UNSUCCESSFUL;
     Irp->IoStatus.Information = 0;
     goto done;
  }

  FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
  Fcb = FileObject->FsContext;
  Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;

  if (Fcb->Data == NULL)
  {
     DPRINT1("Pipe is NOT readable!\n");
     Status = STATUS_UNSUCCESSFUL;
     Irp->IoStatus.Information = 0;
     goto done;
  }

  ExAcquireFastMutex(&Fcb->DataListLock);

  if (IoIsOperationSynchronous(Irp))
  {
     InsertTailList(&Fcb->ReadRequestListHead, &Context->ListEntry);
     if (Fcb->ReadRequestListHead.Flink != &Context->ListEntry)
     {
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
	Context->WaitEvent = &Event;
        ExReleaseFastMutex(&Fcb->DataListLock);
        Status = KeWaitForSingleObject(&Event,
	                               Executive,
				       KernelMode,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	{
	   KEBUGCHECK(0);
	}
        ExAcquireFastMutex(&Fcb->DataListLock);
     }
     Irp->IoStatus.Information = 0;
  }
  else
  {
     KIRQL oldIrql;
     if (IsListEmpty(&Fcb->ReadRequestListHead) ||
	 Fcb->ReadRequestListHead.Flink != &Context->ListEntry)
     {
        /* this is a new request */
        Irp->IoStatus.Information = 0;
	Context->WaitEvent = &Fcb->ReadEvent;
        InsertTailList(&Fcb->ReadRequestListHead, &Context->ListEntry);
	if (Fcb->ReadRequestListHead.Flink != &Context->ListEntry)
	{
	   /* there was already a request on the list */
           IoAcquireCancelSpinLock(&oldIrql);
           if (Irp->Cancel)
           {
	      IoReleaseCancelSpinLock(oldIrql);
	      RemoveEntryList(&Context->ListEntry);
	      ExReleaseFastMutex(&Fcb->DataListLock);
	      Status = STATUS_CANCELLED;
	      goto done;
           }
           IoSetCancelRoutine(Irp, NpfsReadWriteCancelRoutine);
           IoReleaseCancelSpinLock(oldIrql);
	   ExReleaseFastMutex(&Fcb->DataListLock);
           IoMarkIrpPending(Irp);
	   Status = STATUS_PENDING;
	   goto done;
	}
     }
  }

  while (1)
  {
     Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
     Information = Irp->IoStatus.Information;
     Length = IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length;
     ASSERT (Information <= Length);
     Buffer += Information;
     Length -= Information;
     Status = STATUS_SUCCESS;

     while (1)
     {
        if (Fcb->ReadDataAvailable == 0)
        {
	   if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
	   {
	      KeSetEvent(&Fcb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
	   }
	   if (Information > 0 &&
	       (Fcb->Pipe->ReadMode != FILE_PIPE_BYTE_STREAM_MODE ||
	        Fcb->PipeState != FILE_PIPE_CONNECTED_STATE))
	   {
	      break;
	   }
      	   if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	   {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      break;
           }
	   ExReleaseFastMutex(&Fcb->DataListLock);
	   if (IoIsOperationSynchronous(Irp))
	   {
	      /* Wait for ReadEvent to become signaled */

	      DPRINT("Waiting for readable data (%wZ)\n", &Fcb->Pipe->PipeName);
	      Status = KeWaitForSingleObject(&Fcb->ReadEvent,
				             UserRequest,
				             KernelMode,
				             FALSE,
				             NULL);
	      DPRINT("Finished waiting (%wZ)! Status: %x\n", &Fcb->Pipe->PipeName, Status);
	      ExAcquireFastMutex(&Fcb->DataListLock);
	   }
	   else
	   {
              PNPFS_CONTEXT Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;

              Context->WaitEvent = &Fcb->ReadEvent;
	      Status = NpfsAddWaitingReadWriteRequest(DeviceObject, Irp);

	      if (NT_SUCCESS(Status))
	      {
	         Status = STATUS_PENDING;
	      }
	      ExAcquireFastMutex(&Fcb->DataListLock);
	      break;
	   }
        }
        if (Fcb->Pipe->ReadMode == FILE_PIPE_BYTE_STREAM_MODE)
        {
	   DPRINT("Byte stream mode\n");
	   /* Byte stream mode */
	   while (Length > 0 && Fcb->ReadDataAvailable > 0)
	   {
	      CopyLength = RtlRosMin(Fcb->ReadDataAvailable, Length);
	      if (Fcb->ReadPtr + CopyLength <= Fcb->Data + Fcb->MaxDataLength)
	      {
	         memcpy(Buffer, Fcb->ReadPtr, CopyLength);
	         Fcb->ReadPtr += CopyLength;
	         if (Fcb->ReadPtr == Fcb->Data + Fcb->MaxDataLength)
	         {
		    Fcb->ReadPtr = Fcb->Data;
	         }
	      }
              else
	      {
	         TempLength = Fcb->Data + Fcb->MaxDataLength - Fcb->ReadPtr;
	         memcpy(Buffer, Fcb->ReadPtr, TempLength);
	         memcpy(Buffer + TempLength, Fcb->Data, CopyLength - TempLength);
	         Fcb->ReadPtr = Fcb->Data + CopyLength - TempLength;
	      }

	      Buffer += CopyLength;
	      Length -= CopyLength;
	      Information += CopyLength;

	      Fcb->ReadDataAvailable -= CopyLength;
	      Fcb->WriteQuotaAvailable += CopyLength;
	   }

	   if (Length == 0)
	   {
	      if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
	      {
	         KeSetEvent(&Fcb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
	      }
	      KeResetEvent(&Fcb->ReadEvent);
              break;
	   }
        }
        else
        {
	   DPRINT("Message mode\n");

	   /* Message mode */
	   if (Fcb->ReadDataAvailable)
	   {
	      /* Truncate the message if the receive buffer is too small */
	      CopyLength = RtlRosMin(Fcb->ReadDataAvailable, Length);
	      memcpy(Buffer, Fcb->Data, CopyLength);

#ifndef NDEBUG
	      DPRINT("Length %d Buffer %x\n",CopyLength,Buffer);
	      HexDump((PUCHAR)Buffer, CopyLength);
#endif

	      Information = CopyLength;

	      if (Fcb->ReadDataAvailable > Length)
	      {
	         memmove(Fcb->Data, Fcb->Data + Length,
	                 Fcb->ReadDataAvailable - Length);
	         Fcb->ReadDataAvailable -= Length;
	         Status = STATUS_MORE_ENTRIES;
	      }
	      else
	      {
                 KeResetEvent(&Fcb->ReadEvent);
                 if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
                 {
                    KeSetEvent(&Fcb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
                 }
	         Fcb->ReadDataAvailable = 0;
	         Fcb->WriteQuotaAvailable = Fcb->MaxDataLength;
	      }
	   }

	   if (Information > 0)
	   {
              break;
	   }
        }
     }
     Irp->IoStatus.Information = Information;
     Irp->IoStatus.Status = Status;

     if (IoIsOperationSynchronous(Irp))
     {
        RemoveEntryList(&Context->ListEntry);
        if (!IsListEmpty(&Fcb->ReadRequestListHead))
	{
	   Context = CONTAINING_RECORD(Fcb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
           KeSetEvent(Context->WaitEvent, IO_NO_INCREMENT, FALSE);
	}
        ExReleaseFastMutex(&Fcb->DataListLock);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DPRINT("NpfsRead done (Status %lx)\n", Status);
        return Status;
     }
     else
     {
        if (IsOriginalRequest)
	{
	   IsOriginalRequest = FALSE;
	   OriginalStatus = Status;
	}
        if (Status == STATUS_PENDING)
	{
           ExReleaseFastMutex(&Fcb->DataListLock);
	   DPRINT("NpfsRead done (Status %lx)\n", OriginalStatus);
           return OriginalStatus;
	}
	RemoveEntryList(&Context->ListEntry);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
        if (IsListEmpty(&Fcb->ReadRequestListHead))
	{
           ExReleaseFastMutex(&Fcb->DataListLock);
	   DPRINT("NpfsRead done (Status %lx)\n", OriginalStatus);
           return OriginalStatus;
	}
        Context = CONTAINING_RECORD(Fcb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
	Irp = CONTAINING_RECORD(Context, IRP, Tail.Overlay.DriverContext);
     }
  }

done:
  Irp->IoStatus.Status = Status;

  if (Status != STATUS_PENDING)
    {
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
  DPRINT("NpfsRead done (Status %lx)\n", Status);

  return Status;
}

NTSTATUS STDCALL
NpfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  PNPFS_FCB Fcb = NULL;
  PNPFS_FCB ReaderFcb;
  PNPFS_PIPE Pipe = NULL;
  PUCHAR Buffer;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG Length;
  ULONG Offset;
  ULONG Information;
  ULONG CopyLength;
  ULONG TempLength;

  DPRINT("NpfsWrite()\n");

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("Pipe name %wZ\n", &FileObject->FileName);

  Fcb = FileObject->FsContext;
  ReaderFcb = Fcb->OtherSide;
  Pipe = Fcb->Pipe;

  Length = IoStack->Parameters.Write.Length;
  Offset = IoStack->Parameters.Write.ByteOffset.u.LowPart;
  Information = 0;

  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  if (ReaderFcb == NULL)
    {
      DPRINT("Pipe is NOT connected!\n");
      if (Fcb->PipeState == FILE_PIPE_LISTENING_STATE)
        Status = STATUS_PIPE_LISTENING;
      else if (Fcb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
        Status = STATUS_PIPE_DISCONNECTED;
      else
        Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  if (ReaderFcb->Data == NULL)
    {
      DPRINT("Pipe is NOT writable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Length = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);

  ExAcquireFastMutex(&ReaderFcb->DataListLock);
#ifndef NDEBUG
  DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
  HexDump(Buffer, Length);
#endif

  while(1)
    {
      if (ReaderFcb->WriteQuotaAvailable == 0)
	{
	  KeSetEvent(&ReaderFcb->ReadEvent, IO_NO_INCREMENT, FALSE);
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      Status = STATUS_PIPE_BROKEN;
	      ExReleaseFastMutex(&ReaderFcb->DataListLock);
	      goto done;
	    }
	  ExReleaseFastMutex(&ReaderFcb->DataListLock);

	  DPRINT("Waiting for buffer space (%S)\n", Pipe->PipeName.Buffer);
	  Status = KeWaitForSingleObject(&Fcb->WriteEvent,
	                                 UserRequest,
				         KernelMode,
				         FALSE,
				         NULL);
	  DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);

	  ExAcquireFastMutex(&ReaderFcb->DataListLock);
	  /*
	   * It's possible that the event was signaled because the
	   * other side of pipe was closed.
	   */
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      ExReleaseFastMutex(&ReaderFcb->DataListLock);
	      goto done;
	    }
	}

      if (Pipe->WriteMode == FILE_PIPE_BYTE_STREAM_MODE)
	{
	  DPRINT("Byte stream mode\n");
	  while (Length > 0 && ReaderFcb->WriteQuotaAvailable > 0)
	    {
	      CopyLength = RtlRosMin(Length, ReaderFcb->WriteQuotaAvailable);
	      if (ReaderFcb->WritePtr + CopyLength <= ReaderFcb->Data + ReaderFcb->MaxDataLength)
		{
		  memcpy(ReaderFcb->WritePtr, Buffer, CopyLength);
		  ReaderFcb->WritePtr += CopyLength;
		  if (ReaderFcb->WritePtr == ReaderFcb->Data + ReaderFcb->MaxDataLength)
		    {
		      ReaderFcb->WritePtr = ReaderFcb->Data;
		    }
		}
	      else
		{
		  TempLength = ReaderFcb->Data + ReaderFcb->MaxDataLength - ReaderFcb->WritePtr;
		  memcpy(ReaderFcb->WritePtr, Buffer, TempLength);
		  memcpy(ReaderFcb->Data, Buffer + TempLength, CopyLength - TempLength);
		  ReaderFcb->WritePtr = ReaderFcb->Data + CopyLength - TempLength;
		}

	      Buffer += CopyLength;
	      Length -= CopyLength;
	      Information += CopyLength;

	      ReaderFcb->ReadDataAvailable += CopyLength;
	      ReaderFcb->WriteQuotaAvailable -= CopyLength;
	    }

	  if (Length == 0)
	    {
	      KeSetEvent(&ReaderFcb->ReadEvent, IO_NO_INCREMENT, FALSE);
	      KeResetEvent(&Fcb->WriteEvent);
	      break;
	    }
	}
      else
	{
	  DPRINT("Message mode\n");
	  if (Length > 0)
	    {
	      CopyLength = RtlRosMin(Length, ReaderFcb->WriteQuotaAvailable);
	      memcpy(ReaderFcb->Data, Buffer, CopyLength);

	      Information = CopyLength;
	      ReaderFcb->ReadDataAvailable = CopyLength;
	      ReaderFcb->WriteQuotaAvailable = 0;
	    }

   	  if (Information > 0)
   	    {
   	      KeSetEvent(&ReaderFcb->ReadEvent, IO_NO_INCREMENT, FALSE);
   	      KeResetEvent(&Fcb->WriteEvent);
   	      break;
   	    }
	}
    }

  ExReleaseFastMutex(&ReaderFcb->DataListLock);

done:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Information;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("NpfsWrite done (Status %lx)\n", Status);

  return Status;
}

/* EOF */
