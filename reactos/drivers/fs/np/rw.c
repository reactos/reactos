/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
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

static NTSTATUS
NpfsReadFromPipe(PNPFS_CONTEXT Context);

static VOID STDCALL 
NpfsWaitingCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
   PNPFS_CONTEXT Context;
   PNPFS_DEVICE_EXTENSION DeviceExt;

   DPRINT1("NpfsWaitingCancelRoutine() called\n");

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   Context = Irp->Tail.Overlay.DriverContext[0];
   DeviceExt = Context->DeviceObject->DeviceExtension;

   KeLockMutex(&DeviceExt->PipeListLock);
   KeSetEvent(&Context->Fcb->Event, IO_NO_INCREMENT, FALSE);
   KeUnlockMutex(&DeviceExt->PipeListLock);
}

static VOID STDCALL
NpfsWaiterThread(PVOID Context)
{
   PNPFS_THREAD_CONTEXT ThreadContext = (PNPFS_THREAD_CONTEXT) Context;
   ULONG CurrentCount, Count = 0;
   PNPFS_CONTEXT WaitContext = NULL;
   NTSTATUS Status;
   BOOLEAN Terminate = FALSE;
   BOOLEAN Cancel = FALSE;
   KIRQL oldIrql;

   KeLockMutex(&ThreadContext->DeviceExt->PipeListLock);

   while (1)
     {
       CurrentCount = ThreadContext->Count;
       KeResetEvent(&ThreadContext->Event);
       KeUnlockMutex(&ThreadContext->DeviceExt->PipeListLock);
       if (WaitContext)
         {
           if (Cancel)
             {
	       WaitContext->Irp->IoStatus.Status = STATUS_CANCELLED;
               WaitContext->Irp->IoStatus.Information = 0;
               IoCompleteRequest(WaitContext->Irp, IO_NO_INCREMENT);
	       ExFreePool(WaitContext);
	     }
	   else
	     {
	       switch (WaitContext->MajorFunction)
	         {
	           case IRP_MJ_READ:
                     NpfsReadFromPipe(WaitContext);
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
       KeLockMutex(&ThreadContext->DeviceExt->PipeListLock);
       if (!NT_SUCCESS(Status))
         {
           KEBUGCHECK(0);
         }
       Count = Status - STATUS_SUCCESS;
       ASSERT (Count <= CurrentCount);
       if (Count > 0)
         {
	   WaitContext = ThreadContext->WaitContextArray[Count];
	   ThreadContext->Count--;
	   ThreadContext->DeviceExt->EmptyWaiterCount++;
	   ThreadContext->WaitObjectArray[Count] = ThreadContext->WaitObjectArray[ThreadContext->Count];
	   ThreadContext->WaitContextArray[Count] = ThreadContext->WaitContextArray[ThreadContext->Count];
           IoAcquireCancelSpinLock(&oldIrql);
	   Cancel = NULL == IoSetCancelRoutine(WaitContext->Irp, NULL);
           IoReleaseCancelSpinLock(oldIrql);
        }
      else
        {
	  /* someone has add a new wait request */
          WaitContext = NULL;
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
NpfsAddWaitingReader(PNPFS_DEVICE_EXTENSION DeviceExt, PNPFS_CONTEXT Context, PNPFS_FCB Fcb)
{
   PLIST_ENTRY ListEntry;
   PNPFS_THREAD_CONTEXT ThreadContext;
   NTSTATUS Status;
   HANDLE hThread;
   KIRQL oldIrql;

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
       KeInitializeEvent(&ThreadContext->Event, NotificationEvent, FALSE);
       ThreadContext->Count = 1;
       ThreadContext->WaitObjectArray[0] = &ThreadContext->Event;

   
       DPRINT("Creating a new system thread for waiting read requests\n");
      
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
   IoMarkIrpPending(Context->Irp);
   Context->Irp->Tail.Overlay.DriverContext[0] = Context;

   IoAcquireCancelSpinLock(&oldIrql);
   if (Context->Irp->Cancel)
     {
       IoReleaseCancelSpinLock(oldIrql);
       Status = STATUS_CANCELLED;
     }
   else
     {
       IoSetCancelRoutine(Context->Irp, NpfsWaitingCancelRoutine);
       IoReleaseCancelSpinLock(oldIrql);
       ThreadContext->WaitObjectArray[ThreadContext->Count] = &Fcb->Event;
       ThreadContext->WaitContextArray[ThreadContext->Count] = Context;
       ThreadContext->Count++;
       DeviceExt->EmptyWaiterCount--;
       KeSetEvent(&ThreadContext->Event, IO_NO_INCREMENT, FALSE);
       Status = STATUS_SUCCESS;
     }
   KeUnlockMutex(&DeviceExt->PipeListLock);
   return Status;
}

static NTSTATUS
NpfsReadFromPipe(PNPFS_CONTEXT Context)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  ULONG Information;
  PNPFS_FCB Fcb;
  PNPFS_FCB WriterFcb;
  PNPFS_PIPE Pipe;
  ULONG Length;
  PVOID Buffer;
  ULONG CopyLength;
  ULONG TempLength;

  DPRINT("NpfsReadFromPipe(Context %p)\n", Context);

  IoStack = IoGetCurrentIrpStackLocation(Context->Irp);
  FileObject = IoStack->FileObject;
  Fcb = FileObject->FsContext;
  Pipe = Fcb->Pipe;
  WriterFcb = Fcb->OtherSide;

  if (Fcb->Data == NULL)
    {
      DPRINT("Pipe is NOT readable!\n");
      Status = STATUS_UNSUCCESSFUL;
      Information = 0;
      goto done;
    }

  Status = STATUS_SUCCESS;
  Length = IoStack->Parameters.Read.Length;
  Information = 0;

  Buffer = MmGetSystemAddressForMdl(Context->Irp->MdlAddress);
  ExAcquireFastMutex(&Fcb->DataListLock);
  while (1)
    {
      if (Fcb->ReadDataAvailable == 0)
	{
	  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
	    {
	      KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
	    }
	  ExReleaseFastMutex(&Fcb->DataListLock);
	  if (Information > 0)
	    {
	      Status = STATUS_SUCCESS;
	      goto done;
	    }

	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE &&
	      !(Fcb->PipeState == FILE_PIPE_LISTENING_STATE && Fcb->PipeEnd == FILE_PIPE_SERVER_END))
	    {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }

	  if (IoIsOperationSynchronous(Context->Irp))
	    {
	      /* Wait for ReadEvent to become signaled */
	      DPRINT("Waiting for readable data (%S)\n", Pipe->PipeName.Buffer);
	      Status = KeWaitForSingleObject(&Fcb->Event,
				             UserRequest,
				             KernelMode,
				             FALSE,
				             NULL);
	      DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);
	    }
	  else
	    {
	      PNPFS_CONTEXT NewContext;

	      NewContext = ExAllocatePool(NonPagedPool, sizeof(NPFS_CONTEXT));
	      if (NewContext == NULL)
	        {
		   Status = STATUS_NO_MEMORY;
		   goto done;
		}
	      memcpy(NewContext, Context, sizeof(NPFS_CONTEXT));
	      NewContext->AllocatedFromPool = TRUE;
	      NewContext->Fcb = Fcb;
	      NewContext->MajorFunction = IRP_MJ_READ;

	      Status = NpfsAddWaitingReader(Context->DeviceObject->DeviceExtension, NewContext, Fcb);
	         
	      if (NT_SUCCESS(Status))
	        {
		  Status = STATUS_PENDING;
		}
	      else
	        {
		  ExFreePool(NewContext);
		}
	      goto done;
	    }

	  ExAcquireFastMutex(&Fcb->DataListLock);
	}

      if (Pipe->ReadMode == FILE_PIPE_BYTE_STREAM_MODE)
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
	          KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
	        }
	      KeResetEvent(&Fcb->Event);
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
                  KeResetEvent(&Fcb->Event);
                  if (Fcb->PipeState == FILE_PIPE_CONNECTED_STATE)
                    {
                      KeSetEvent(&WriterFcb->Event, IO_NO_INCREMENT, FALSE);
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

  ExReleaseFastMutex(&Fcb->DataListLock);

done:
  Context->Irp->IoStatus.Status = Status;
  Context->Irp->IoStatus.Information = Information;

  if (Status != STATUS_PENDING)
    {
      IoCompleteRequest(Context->Irp, IO_NO_INCREMENT);
    }

  if (Context->AllocatedFromPool)
    {
      ExFreePool(Context);
    }
  DPRINT("NpfsRead done (Status %lx)\n", Status);

  return Status;
}

NTSTATUS STDCALL
NpfsRead(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
  NPFS_CONTEXT Context;

  Context.AllocatedFromPool = FALSE;
  Context.DeviceObject = DeviceObject;
  Context.Irp = Irp;
  
  if (Irp->MdlAddress == NULL)
    {
      DPRINT("Irp->MdlAddress == NULL\n");
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_UNSUCCESSFUL;
    }

  return NpfsReadFromPipe(&Context);
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
	  KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
	  ExReleaseFastMutex(&ReaderFcb->DataListLock);
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }

	  DPRINT("Waiting for buffer space (%S)\n", Pipe->PipeName.Buffer);
	  Status = KeWaitForSingleObject(&Fcb->Event,
				         UserRequest,
				         KernelMode,
				         FALSE,
				         NULL);
	  DPRINT("Finished waiting (%S)! Status: %x\n", Pipe->PipeName.Buffer, Status);

	  /*
	   * It's possible that the event was signaled because the
	   * other side of pipe was closed.
	   */
	  if (Fcb->PipeState != FILE_PIPE_CONNECTED_STATE)
	    {
	      DPRINT("PipeState: %x\n", Fcb->PipeState);
	      Status = STATUS_PIPE_BROKEN;
	      goto done;
	    }
	  ExAcquireFastMutex(&ReaderFcb->DataListLock);
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
	      KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
	      KeResetEvent(&Fcb->Event);
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
   	      KeSetEvent(&ReaderFcb->Event, IO_NO_INCREMENT, FALSE);
   	      KeResetEvent(&Fcb->Event);
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
