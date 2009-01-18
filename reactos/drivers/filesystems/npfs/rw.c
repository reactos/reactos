/*
* COPYRIGHT:  See COPYING in the top level directory
* PROJECT:    ReactOS kernel
* FILE:       drivers/fs/np/rw.c
* PURPOSE:    Named pipe filesystem
* PROGRAMMER: David Welch <welch@cwcom.net>
*             Michael Martin
*/

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID HexDump(PUCHAR Buffer, ULONG Length)
{
	CHAR Line[65];
	UCHAR ch;
	const char Hex[] = "0123456789ABCDEF";
	int i, j;

	DbgPrint("---------------\n");

	for (i = 0; i < Length; i+= 16)
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

static DRIVER_CANCEL NpfsReadWriteCancelRoutine;
static VOID NTAPI
NpfsReadWriteCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
						   IN PIRP Irp)
{
	PNPFS_CONTEXT Context;
	PNPFS_DEVICE_EXTENSION DeviceExt;
	PIO_STACK_LOCATION IoStack;
	PNPFS_CCB Ccb;
	BOOLEAN Complete = FALSE;

	DPRINT("NpfsReadWriteCancelRoutine(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

	IoReleaseCancelSpinLock(Irp->CancelIrql);

	Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;
	DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IoStack = IoGetCurrentIrpStackLocation(Irp);
	Ccb = IoStack->FileObject->FsContext2;

	KeLockMutex(&DeviceExt->PipeListLock);
	ExAcquireFastMutex(&Ccb->DataListLock);
	switch(IoStack->MajorFunction)
	{
	case IRP_MJ_READ:
		if (Ccb->ReadRequestListHead.Flink != &Context->ListEntry)
		{
			/* we are not the first in the list, remove an complete us */
			RemoveEntryList(&Context->ListEntry);
			Complete = TRUE;
		}
		else
		{
			KeSetEvent(&Ccb->ReadEvent, IO_NO_INCREMENT, FALSE);
		}
		break;
	default:
		ASSERT(FALSE);
	}
	ExReleaseFastMutex(&Ccb->DataListLock);
	KeUnlockMutex(&DeviceExt->PipeListLock);
	if (Complete)
	{
		Irp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
}

static VOID NTAPI
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
	PNPFS_CCB Ccb;

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
					ASSERT(FALSE);
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
			ASSERT(FALSE);
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
				Ccb = IoStack->FileObject->FsContext2;
				ExAcquireFastMutex(&Ccb->DataListLock);
				RemoveEntryList(&Context->ListEntry);
				switch (IoStack->MajorFunction)
				{
				case IRP_MJ_READ:
					if (!IsListEmpty(&Ccb->ReadRequestListHead))
					{
						/* put the next request on the wait list */
						NextContext = CONTAINING_RECORD(Ccb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
						ThreadContext->WaitObjectArray[ThreadContext->Count] = NextContext->WaitEvent;
						NextIrp = CONTAINING_RECORD(NextContext, IRP, Tail.Overlay.DriverContext);
						ThreadContext->WaitIrpArray[ThreadContext->Count] = NextIrp;
						ThreadContext->Count++;
						ThreadContext->DeviceExt->EmptyWaiterCount--;
					}
					break;
				default:
					ASSERT(FALSE);
				}
				ExReleaseFastMutex(&Ccb->DataListLock);
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
		(void)IoSetCancelRoutine(Irp, NpfsReadWriteCancelRoutine);
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

NTSTATUS NTAPI
NpfsRead(IN PDEVICE_OBJECT DeviceObject,
		 IN PIRP Irp)
{
	PFILE_OBJECT FileObject;
	NTSTATUS Status;
	NTSTATUS OriginalStatus = STATUS_SUCCESS;
	PNPFS_CCB Ccb;
	PNPFS_CONTEXT Context;
	KEVENT Event;
	ULONG Length;
	ULONG Information = 0;
	ULONG CopyLength = 0;
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
	DPRINT("FileObject %p\n", FileObject);
	DPRINT("Pipe name %wZ\n", &FileObject->FileName);
	Ccb = FileObject->FsContext2;
	Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;

	if ((Ccb->OtherSide) && (Ccb->OtherSide->PipeState == FILE_PIPE_DISCONNECTED_STATE) && (Ccb->PipeState == FILE_PIPE_DISCONNECTED_STATE))
	{
		DPRINT("Both Client and Server are disconnected!\n");
		Status = STATUS_PIPE_DISCONNECTED;
		Irp->IoStatus.Information = 0;
		goto done;

	}

	if ((Ccb->OtherSide == NULL) && (Ccb->ReadDataAvailable == 0))
	{
		/* Its ok if the other side has been Disconnect, but if we have data still in the buffer
                   , need to still be able to read it. Currently this is a HAXXXX */
		DPRINT("Pipe no longer connected and no data exist in buffer. Ok to close pipe\n");
		if (Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
			Status = STATUS_PIPE_LISTENING;
		else if (Ccb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
			Status = STATUS_PIPE_DISCONNECTED;
		else
			Status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
		goto done;
	}

	if (Ccb->Data == NULL)
	{
		DPRINT1("Pipe is NOT readable!\n");
		Status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
		goto done;
	}

	ExAcquireFastMutex(&Ccb->DataListLock);

	if (IoIsOperationSynchronous(Irp))
	{
		InsertTailList(&Ccb->ReadRequestListHead, &Context->ListEntry);
		if (Ccb->ReadRequestListHead.Flink != &Context->ListEntry)
		{
			KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
			Context->WaitEvent = &Event;
			ExReleaseFastMutex(&Ccb->DataListLock);
			Status = KeWaitForSingleObject(&Event,
				Executive,
				Irp->RequestorMode,
				FALSE,
				NULL);
			if ((Status == STATUS_USER_APC) || (Status == STATUS_KERNEL_APC))
			{
				Status = STATUS_CANCELLED;
				goto done;
			}
			if (!NT_SUCCESS(Status))
			{
				ASSERT(FALSE);
			}
			ExAcquireFastMutex(&Ccb->DataListLock);
		}
		Irp->IoStatus.Information = 0;
	}
	else
	{
		KIRQL oldIrql;
		if (IsListEmpty(&Ccb->ReadRequestListHead) ||
			Ccb->ReadRequestListHead.Flink != &Context->ListEntry)
		{
			/* this is a new request */
			Irp->IoStatus.Information = 0;
			Context->WaitEvent = &Ccb->ReadEvent;
			InsertTailList(&Ccb->ReadRequestListHead, &Context->ListEntry);
			if (Ccb->ReadRequestListHead.Flink != &Context->ListEntry)
			{
				/* there was already a request on the list */
				IoAcquireCancelSpinLock(&oldIrql);
				if (Irp->Cancel)
				{
					IoReleaseCancelSpinLock(oldIrql);
					RemoveEntryList(&Context->ListEntry);
					ExReleaseFastMutex(&Ccb->DataListLock);
					Status = STATUS_CANCELLED;
					goto done;
				}
				(void)IoSetCancelRoutine(Irp, NpfsReadWriteCancelRoutine);
				IoReleaseCancelSpinLock(oldIrql);
				ExReleaseFastMutex(&Ccb->DataListLock);
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
		Buffer = (PVOID)((ULONG_PTR)Buffer + Information);
		Length -= Information;
		Status = STATUS_SUCCESS;

		while (1)
		{
			if (Ccb->ReadDataAvailable == 0)
			{
				if (Ccb->PipeState == FILE_PIPE_CONNECTED_STATE)
				{
					ASSERT(Ccb->OtherSide != NULL);
					KeSetEvent(&Ccb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
				}
				if (Information > 0 &&
					(Ccb->Fcb->ReadMode != FILE_PIPE_BYTE_STREAM_MODE ||
					Ccb->PipeState != FILE_PIPE_CONNECTED_STATE))
				{
					break;
				}
				if ((Ccb->PipeState != FILE_PIPE_CONNECTED_STATE) && (Ccb->ReadDataAvailable == 0))
				{
					DPRINT("PipeState: %x\n", Ccb->PipeState);
					Status = STATUS_PIPE_BROKEN;
					break;
				}
				ExReleaseFastMutex(&Ccb->DataListLock);

				if (IoIsOperationSynchronous(Irp))
				{
					/* Wait for ReadEvent to become signaled */

					DPRINT("Waiting for readable data (%wZ)\n", &Ccb->Fcb->PipeName);
					Status = KeWaitForSingleObject(&Ccb->ReadEvent,
						UserRequest,
						Irp->RequestorMode,
						FALSE,
						NULL);
					DPRINT("Finished waiting (%wZ)! Status: %x\n", &Ccb->Fcb->PipeName, Status);

					if ((Status == STATUS_USER_APC) || (Status == STATUS_KERNEL_APC))
					{
						Status = STATUS_CANCELLED;
						break;
					}
					if (!NT_SUCCESS(Status))
					{
						ASSERT(FALSE);
					}
					ExAcquireFastMutex(&Ccb->DataListLock);
				}
				else
				{
					Context = (PNPFS_CONTEXT)&Irp->Tail.Overlay.DriverContext;

					Context->WaitEvent = &Ccb->ReadEvent;

					Status = NpfsAddWaitingReadWriteRequest(DeviceObject, Irp);

					if (NT_SUCCESS(Status))
					{
						Status = STATUS_PENDING;
						goto done;
					}
					ExAcquireFastMutex(&Ccb->DataListLock);
					break;
				}
			}
			ASSERT(IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL);

			/* If the pipe type and read mode are both byte stream */
			if ((Ccb->Fcb->PipeType == FILE_PIPE_BYTE_STREAM_TYPE) && (Ccb->Fcb->ReadMode == FILE_PIPE_BYTE_STREAM_MODE))
			{
				DPRINT("Byte stream mode: Ccb->Data %x\n", Ccb->Data);
				/* Byte stream mode */
				while (Length > 0 && Ccb->ReadDataAvailable > 0)
				{
					CopyLength = min(Ccb->ReadDataAvailable, Length);
					if ((ULONG_PTR)Ccb->ReadPtr + CopyLength <= (ULONG_PTR)Ccb->Data + Ccb->MaxDataLength)
					{
						memcpy(Buffer, Ccb->ReadPtr, CopyLength);
						Ccb->ReadPtr = (PVOID)((ULONG_PTR)Ccb->ReadPtr + CopyLength);
						if (Ccb->ReadPtr == (PVOID)((ULONG_PTR)Ccb->Data + Ccb->MaxDataLength))
						{
							Ccb->ReadPtr = Ccb->Data;
						}
					}
					else
					{
						TempLength = (ULONG)((ULONG_PTR)Ccb->Data + Ccb->MaxDataLength - (ULONG_PTR)Ccb->ReadPtr);
						memcpy(Buffer, Ccb->ReadPtr, TempLength);
						memcpy((PVOID)((ULONG_PTR)Buffer + TempLength), Ccb->Data, CopyLength - TempLength);
						Ccb->ReadPtr = (PVOID)((ULONG_PTR)Ccb->Data + CopyLength - TempLength);
					}

					Buffer = (PVOID)((ULONG_PTR)Buffer + CopyLength);
					Length -= CopyLength;
					Information += CopyLength;

					Ccb->ReadDataAvailable -= CopyLength;
					Ccb->WriteQuotaAvailable += CopyLength;
				}

				if ((Length == 0) || (Ccb->ReadDataAvailable == 0))
				{
					if (Ccb->PipeState == FILE_PIPE_CONNECTED_STATE)
					{
						KeSetEvent(&Ccb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
					}
					KeResetEvent(&Ccb->ReadEvent);
					break;
				}
			}
			else if (Ccb->Fcb->PipeType == FILE_PIPE_MESSAGE_TYPE)
			{
				DPRINT("Message mode: Ccb>Data %x\n", Ccb->Data);

				/* Check if buffer is full and the read pointer is not at the start of the buffer */
				if ((Ccb->WriteQuotaAvailable == 0) && (Ccb->ReadPtr > Ccb->Data))
				{
					Ccb->WriteQuotaAvailable += (ULONG_PTR)Ccb->ReadPtr - (ULONG_PTR)Ccb->Data;
					memcpy(Ccb->Data, Ccb->ReadPtr, (ULONG_PTR)Ccb->WritePtr - (ULONG_PTR)Ccb->ReadPtr);
					Ccb->WritePtr = (PVOID)((ULONG_PTR)Ccb->WritePtr - ((ULONG_PTR)Ccb->ReadPtr - (ULONG_PTR)Ccb->Data));
					Ccb->ReadPtr = Ccb->Data;
				}

				/* For Message mode, the Message length is stored in the buffer preceeding the Message. */
				if (Ccb->ReadDataAvailable)
				{
					ULONG NextMessageLength = 0;

					/*First get the size of the message */
					memcpy(&NextMessageLength, Ccb->ReadPtr, sizeof(NextMessageLength));

					if ((NextMessageLength == 0) || (NextMessageLength > Ccb->ReadDataAvailable))
					{
						DPRINT1("Possible memory corruption.\n");
						HexDump(Ccb->Data, (ULONG_PTR)Ccb->WritePtr - (ULONG_PTR)Ccb->Data);
						ASSERT(FALSE);
					}

					/* Use the smaller value */
					CopyLength = min(NextMessageLength, Length);
					ASSERT(CopyLength > 0);
					ASSERT(CopyLength <= Ccb->ReadDataAvailable);
					/* retrieve the message from the buffer */
					memcpy(Buffer, (PVOID)((ULONG_PTR)Ccb->ReadPtr + sizeof(NextMessageLength)), CopyLength);

					if (Ccb->ReadDataAvailable > CopyLength)
					{
						if (CopyLength < NextMessageLength)
						/* Client only requested part of the message */
						{
							/* Calculate the remaining message new size */
							ULONG NewMessageSize = NextMessageLength-CopyLength;

							/* Update ReadPtr to point to new Message size location */
							Ccb->ReadPtr = (PVOID)((ULONG_PTR)Ccb->ReadPtr + CopyLength);

							/* Write a new Message size to buffer for the part of the message still there */
							memcpy(Ccb->ReadPtr, &NewMessageSize, sizeof(NewMessageSize));
						}
						else
						/* Client wanted the entire message */
						{
							/* Update ReadPtr to point to next message size */
							Ccb->ReadPtr = (PVOID)((ULONG_PTR)Ccb->ReadPtr + CopyLength + sizeof(CopyLength));
						}
					}
					else
					{
						/* This was the last Message, so just zero start of buffer for safety sake */
						memset(Ccb->Data, 0, NextMessageLength + sizeof(NextMessageLength));

						/* Reset to MaxDataLength as partial message retrievals dont
						   give the length back to Quota */
						Ccb->WriteQuotaAvailable = Ccb->MaxDataLength;

						/* reset read and write pointer to beginning of buffer */
						Ccb->WritePtr = Ccb->Data;
						Ccb->ReadPtr = Ccb->Data;
					}
#ifndef NDEBUG
					DPRINT("Length %d Buffer %x\n",CopyLength,Buffer);
					HexDump((PUCHAR)Buffer, CopyLength);
#endif

					Information += CopyLength;

					Ccb->ReadDataAvailable -= CopyLength;

					if ((ULONG)Ccb->WriteQuotaAvailable > (ULONG)Ccb->MaxDataLength) ASSERT(FALSE);
				}

				if (Information > 0)
				{
					if ((Ccb->Fcb->ReadMode == FILE_PIPE_BYTE_STREAM_MODE) && (Ccb->ReadDataAvailable) && (Length > CopyLength))
					{
						Buffer = (PVOID)((ULONG_PTR)Buffer + CopyLength);
						Length -= CopyLength;
					}
					else 
					{
						KeResetEvent(&Ccb->ReadEvent);

						if ((Ccb->PipeState == FILE_PIPE_CONNECTED_STATE) && (Ccb->WriteQuotaAvailable > 0))
						{
							KeSetEvent(&Ccb->OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
						}
						break;
					}
				}
			}
			else
			{
				DPRINT1("Unhandled Pipe Mode!\n");
				ASSERT(FALSE);
			}
		}
		Irp->IoStatus.Information = Information;
		Irp->IoStatus.Status = Status;

		ASSERT(IoGetCurrentIrpStackLocation(Irp)->FileObject != NULL);

		if (Status == STATUS_CANCELLED)
			goto done;

		if (IoIsOperationSynchronous(Irp))
		{
			RemoveEntryList(&Context->ListEntry);
			if (!IsListEmpty(&Ccb->ReadRequestListHead))
			{
				Context = CONTAINING_RECORD(Ccb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
				KeSetEvent(Context->WaitEvent, IO_NO_INCREMENT, FALSE);
			}
			ExReleaseFastMutex(&Ccb->DataListLock);
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
				ExReleaseFastMutex(&Ccb->DataListLock);
				DPRINT("NpfsRead done (Status %lx)\n", OriginalStatus);
				return OriginalStatus;
			}
			RemoveEntryList(&Context->ListEntry);
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			if (IsListEmpty(&Ccb->ReadRequestListHead))
			{
				ExReleaseFastMutex(&Ccb->DataListLock);
				DPRINT("NpfsRead done (Status %lx)\n", OriginalStatus);
				return OriginalStatus;
			}
			Context = CONTAINING_RECORD(Ccb->ReadRequestListHead.Flink, NPFS_CONTEXT, ListEntry);
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

NTSTATUS NTAPI
NpfsWrite(PDEVICE_OBJECT DeviceObject,
		  PIRP Irp)
{
	PIO_STACK_LOCATION IoStack;
	PFILE_OBJECT FileObject;
	PNPFS_FCB Fcb = NULL;
	PNPFS_CCB Ccb = NULL;
	PNPFS_CCB ReaderCcb;
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

	Ccb = FileObject->FsContext2;
	ReaderCcb = Ccb->OtherSide;
	Fcb = Ccb->Fcb;

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

	if ((ReaderCcb == NULL) || (Ccb->PipeState != FILE_PIPE_CONNECTED_STATE))
	{
		DPRINT("Pipe is NOT connected!\n");
		if (Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
			Status = STATUS_PIPE_LISTENING;
		else if (Ccb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
			Status = STATUS_PIPE_DISCONNECTED;
		else
			Status = STATUS_UNSUCCESSFUL;
		Length = 0;
		goto done;
	}

	if (ReaderCcb->Data == NULL)
	{
		DPRINT("Pipe is NOT writable!\n");
		Status = STATUS_UNSUCCESSFUL;
		Length = 0;
		goto done;
	}

	Status = STATUS_SUCCESS;
	Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);

	ExAcquireFastMutex(&ReaderCcb->DataListLock);

#ifndef NDEBUG
	DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
	HexDump(Buffer, Length);
#endif

	while(1)
	{
		if ((ReaderCcb->WriteQuotaAvailable == 0))
		{
			KeSetEvent(&ReaderCcb->ReadEvent, IO_NO_INCREMENT, FALSE);
			if (Ccb->PipeState != FILE_PIPE_CONNECTED_STATE)
			{
				Status = STATUS_PIPE_BROKEN;
				ExReleaseFastMutex(&ReaderCcb->DataListLock);
				goto done;
			}
			ExReleaseFastMutex(&ReaderCcb->DataListLock);

			DPRINT("Write Waiting for buffer space (%S)\n", Fcb->PipeName.Buffer);
			Status = KeWaitForSingleObject(&Ccb->WriteEvent,
				UserRequest,
				Irp->RequestorMode,
				FALSE,
				NULL);
			DPRINT("Write Finished waiting (%S)! Status: %x\n", Fcb->PipeName.Buffer, Status);
			
			if ((Status == STATUS_USER_APC) || (Status == STATUS_KERNEL_APC))
			{
				Status = STATUS_CANCELLED;
				break;
			}
			if (!NT_SUCCESS(Status))
			{
				ASSERT(FALSE);
			}
			/*
			* It's possible that the event was signaled because the
			* other side of pipe was closed.
			*/
			if (Ccb->PipeState != FILE_PIPE_CONNECTED_STATE)
			{
				DPRINT("PipeState: %x\n", Ccb->PipeState);
				Status = STATUS_PIPE_BROKEN;
				goto done;
			}
			ExAcquireFastMutex(&ReaderCcb->DataListLock);
		}

		if ((Ccb->Fcb->PipeType == FILE_PIPE_BYTE_STREAM_TYPE) && (Ccb->Fcb->WriteMode == FILE_PIPE_BYTE_STREAM_MODE))
		{
			DPRINT("Byte stream mode: Ccb->Data %x, Ccb->WritePtr %x\n", ReaderCcb->Data, ReaderCcb->WritePtr);

			while (Length > 0 && ReaderCcb->WriteQuotaAvailable > 0)
			{
				CopyLength = min(Length, ReaderCcb->WriteQuotaAvailable);

				if ((ULONG_PTR)ReaderCcb->WritePtr + CopyLength <= (ULONG_PTR)ReaderCcb->Data + ReaderCcb->MaxDataLength)
				{
					memcpy(ReaderCcb->WritePtr, Buffer, CopyLength);
					ReaderCcb->WritePtr = (PVOID)((ULONG_PTR)ReaderCcb->WritePtr + CopyLength);
					if ((ULONG_PTR)ReaderCcb->WritePtr == (ULONG_PTR)ReaderCcb->Data + ReaderCcb->MaxDataLength)
					{
						ReaderCcb->WritePtr = ReaderCcb->Data;
					}
				}
				else
				{

					TempLength = (ULONG)((ULONG_PTR)ReaderCcb->Data + ReaderCcb->MaxDataLength -
							(ULONG_PTR)ReaderCcb->WritePtr);

					memcpy(ReaderCcb->WritePtr, Buffer, TempLength);
					memcpy(ReaderCcb->Data, Buffer + TempLength, CopyLength - TempLength);
					ReaderCcb->WritePtr = (PVOID)((ULONG_PTR)ReaderCcb->Data + CopyLength - TempLength);
				}

				Buffer += CopyLength;
				Length -= CopyLength;
				Information += CopyLength;

				ReaderCcb->ReadDataAvailable += CopyLength;
				ReaderCcb->WriteQuotaAvailable -= CopyLength;
			}

			if (Length == 0)
			{
				KeSetEvent(&ReaderCcb->ReadEvent, IO_NO_INCREMENT, FALSE);
				KeResetEvent(&Ccb->WriteEvent);
				break;
			}
		}
		else if ((Ccb->Fcb->PipeType == FILE_PIPE_MESSAGE_TYPE) && (Ccb->Fcb->WriteMode == FILE_PIPE_MESSAGE_MODE))
		{
			/* For Message Type Pipe, the Pipes memory will be used to store the size of each message */
			DPRINT("Message mode: Ccb->Data %x, Ccb->WritePtr %x\n",ReaderCcb->Data, ReaderCcb->WritePtr);
			if (Length > 0)
			{
				/* Verify the WritePtr is still inside the buffer */
				if (((ULONG_PTR)ReaderCcb->WritePtr > ((ULONG_PTR)ReaderCcb->Data + (ULONG_PTR)ReaderCcb->MaxDataLength)) ||
				((ULONG_PTR)ReaderCcb->WritePtr < (ULONG_PTR)ReaderCcb->Data))
				{
					DPRINT1("NPFS is writing out of its buffer. Report to developer!\n");
					DPRINT1("ReaderCcb->WritePtr %x, ReaderCcb->Data %x, ReaderCcb->MaxDataLength %lu\n",
						ReaderCcb->WritePtr, ReaderCcb->Data, ReaderCcb->MaxDataLength);
					ASSERT(FALSE);
				}

				CopyLength = min(Length, ReaderCcb->WriteQuotaAvailable - sizeof(ULONG));
				if (CopyLength > ReaderCcb->WriteQuotaAvailable)
				{
					DPRINT1("Writing %lu byte to pipe would overflow as only %lu bytes are available\n",
						CopyLength, ReaderCcb->WriteQuotaAvailable);
					ASSERT(FALSE);
				}

				/* First Copy the Length of the message into the pipes buffer */
				memcpy(ReaderCcb->WritePtr, &CopyLength, sizeof(CopyLength));

				/* Now the user buffer itself */
				memcpy((PVOID)((ULONG_PTR)ReaderCcb->WritePtr + sizeof(CopyLength)), Buffer, CopyLength);

				/* Update the write pointer */
				ReaderCcb->WritePtr = (PVOID)((ULONG_PTR)ReaderCcb->WritePtr + sizeof(CopyLength) + CopyLength);

				Information += CopyLength;

				ReaderCcb->ReadDataAvailable += CopyLength;

				ReaderCcb->WriteQuotaAvailable -= (CopyLength + sizeof(ULONG));

				if ((ULONG_PTR)ReaderCcb->WriteQuotaAvailable > (ULONG)ReaderCcb->MaxDataLength)
				{
					DPRINT1("QuotaAvailable is greater than buffer size!\n");
					ASSERT(FALSE);
				}
			}

			if (Information > 0)
			{
				KeSetEvent(&ReaderCcb->ReadEvent, IO_NO_INCREMENT, FALSE);
				KeResetEvent(&Ccb->WriteEvent);
				break;
			}
		}
		else
		{
			DPRINT1("Unhandled Pipe Type Mode and Read Write Mode!\n");
			ASSERT(FALSE);
		}
	}

	ExReleaseFastMutex(&ReaderCcb->DataListLock);

done:
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DPRINT("NpfsWrite done (Status %lx)\n", Status);

	return Status;
}

/* EOF */
