/* $Id: errlog.c,v 1.14 2003/11/20 11:08:52 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/errlog.c
 * PURPOSE:         Error logging
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/port.h>

//#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct _ERROR_LOG_ENTRY
{
  LIST_ENTRY Entry;
  LARGE_INTEGER TimeStamp;
  ULONG PacketSize;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

typedef struct _LOG_WORKER_DPC
{
  KDPC Dpc;
  KTIMER Timer;
} LOG_WORKER_DPC, *PLOG_WORKER_DPC;


static VOID STDCALL
IopLogWorker (PVOID Parameter);


/* GLOBALS *******************************************************************/

static KSPIN_LOCK IopAllocationLock;
static ULONG IopTotalLogSize;

static KSPIN_LOCK IopLogListLock;
static LIST_ENTRY IopLogListHead;

static BOOLEAN IopLogWorkerRunning = FALSE;
static BOOLEAN IopLogPortConnected = FALSE;
static HANDLE IopLogPort;


/* FUNCTIONS *****************************************************************/

NTSTATUS
IopInitErrorLog (VOID)
{
  IopTotalLogSize = 0;
  KeInitializeSpinLock (&IopAllocationLock);

  KeInitializeSpinLock (&IopLogListLock);
  InitializeListHead (&IopLogListHead);

  return STATUS_SUCCESS;
}


static VOID STDCALL
IopLogDpcRoutine (PKDPC Dpc,
		  PVOID DeferredContext,
		  PVOID SystemArgument1,
		  PVOID SystemArgument2)
{
  PWORK_QUEUE_ITEM LogWorkItem;

  DPRINT1 ("\nIopLogDpcRoutine() called\n");

  /* Release the WorkerDpc struct */
  ExFreePool (DeferredContext);

  /* Allocate, initialize and reissue a work item */
  LogWorkItem = ExAllocatePool (NonPagedPool,
				sizeof(WORK_QUEUE_ITEM));
  if (LogWorkItem == NULL)
    {
      IopLogWorkerRunning = FALSE;
      return;
    }

  ExInitializeWorkItem (LogWorkItem,
			IopLogWorker,
			LogWorkItem);

  ExQueueWorkItem (LogWorkItem,
		   DelayedWorkQueue);
}


static VOID
IopRestartLogWorker (VOID)
{
  PLOG_WORKER_DPC WorkerDpc;
  LARGE_INTEGER Timeout;

  DPRINT1 ("IopRestartWorker() called\n");

  WorkerDpc = ExAllocatePool (NonPagedPool,
			      sizeof(LOG_WORKER_DPC));
  if (WorkerDpc == NULL)
    {
      IopLogWorkerRunning = FALSE;
      return;
    }

  /* Initialize DPC and Timer */
  KeInitializeDpc (&WorkerDpc->Dpc,
		   IopLogDpcRoutine,
		   WorkerDpc);
  KeInitializeTimer (&WorkerDpc->Timer);

  /* Restart after 30 seconds */
  Timeout.QuadPart = -300000000LL;
  KeSetTimer (&WorkerDpc->Timer,
	      Timeout,
	      &WorkerDpc->Dpc);
}


static BOOLEAN
IopConnectLogPort (VOID)
{
  UNICODE_STRING PortName;
  NTSTATUS Status;
  ULONG MaxMessageSize;

  DPRINT1 ("IopConnectLogPort() called\n");

  RtlInitUnicodeString (&PortName,
			L"\\ErrorLogPort");

  Status = NtConnectPort (&IopLogPort,
			  &PortName,
			  NULL,
			  NULL,
			  NULL,
			  &MaxMessageSize, //NULL,
			  NULL,
			  NULL);
  DPRINT1 ("NtConnectPort() (Status %lx)\n", Status);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtConnectPort() failed (Status %lx)\n", Status);
      return FALSE;
    }

  DPRINT1 ("Maximum message size %lu\n", MaxMessageSize);

  DPRINT1 ("IopConnectLogPort() done\n");

  return TRUE;
}


static VOID STDCALL
IopLogWorker (PVOID Parameter)
{
  PERROR_LOG_ENTRY LogEntry;
  PLPC_MAX_MESSAGE Request;
  PIO_ERROR_LOG_MESSAGE Message;
  PIO_ERROR_LOG_PACKET Packet;
  KIRQL Irql;
  NTSTATUS Status;

  DPRINT1 ("IopLogWorker() called\n");

  /* Release the work item */
  ExFreePool (Parameter);


  /* Connect to the error log port */
  if (IopLogPortConnected == FALSE)
    {
      if (IopConnectLogPort () == FALSE)
	{
CHECKPOINT1;
	  IopRestartLogWorker ();
	  return;
	}

CHECKPOINT1;
      IopLogPortConnected = TRUE;
    }

  while (TRUE)
    {
      /* Remove last entry from the list */
      KeAcquireSpinLock (&IopLogListLock,
			 &Irql);

      if (!IsListEmpty (&IopLogListHead))
	{
	  LogEntry = CONTAINING_RECORD (IopLogListHead.Blink,
					ERROR_LOG_ENTRY,
					Entry);
	  RemoveEntryList (&LogEntry->Entry);
	}
      else
	{
	  LogEntry = NULL;
	}

      KeReleaseSpinLock (&IopLogListLock,
			 Irql);

      if (LogEntry == NULL)
	{
	  DPRINT1 ("No message in log list\n");
	  break;
	}

      /* Get pointer to the log packet */
      Packet = (PIO_ERROR_LOG_PACKET)((ULONG_PTR)LogEntry + sizeof(ERROR_LOG_ENTRY));

      /* Allocate request buffer */
      Request = ExAllocatePool (NonPagedPool,
				sizeof(LPC_MAX_MESSAGE));
      if (Request == NULL)
	{
	  DPRINT1("Failed to allocate request buffer!\n");

	  /* FIXME: Requeue log message and restart the worker */

	  return;
	}

      /* Initialize the log message */
      Message = (PIO_ERROR_LOG_MESSAGE)Request->Data;
      Message->Type = 0xC; //IO_TYPE_ERROR_MESSAGE;
      Message->Size =
	sizeof(IO_ERROR_LOG_MESSAGE) - sizeof(IO_ERROR_LOG_PACKET) + LogEntry->PacketSize;
      Message->DriverNameLength = 0; /* FIXME */
      Message->TimeStamp.QuadPart = LogEntry->TimeStamp.QuadPart;
      Message->DriverNameOffset = 0;  /* FIXME */

      RtlCopyMemory (&Message->EntryData,
		     Packet,
		     LogEntry->PacketSize);

  DPRINT1 ("SequenceNumber %lx\n", Packet->SequenceNumber);

      Request->Header.DataSize = Message->Size;
      Request->Header.MessageSize =
	Request->Header.DataSize + sizeof(LPC_MESSAGE);

      /* Send the error message to the log port */
      Status = NtRequestPort (IopLogPort,
			      &Request->Header);

      /* Release request buffer */
      ExFreePool (Request);

      if (!NT_SUCCESS(Status))
	{
	  DPRINT1 ("NtRequestPort() failed (Status %lx)\n", Status);

	  /* FIXME: Requeue log message and restart the worker */

	  return;
	}

      /* Release error log entry */
      KeAcquireSpinLock (&IopAllocationLock,
			 &Irql);

      IopTotalLogSize -= LogEntry->PacketSize;
      ExFreePool (LogEntry);

      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
    }

  IopLogWorkerRunning = FALSE;

  DPRINT1 ("IopLogWorker() done\n");
}


/*
 * @implemented
 */
PVOID STDCALL
IoAllocateErrorLogEntry (IN PVOID IoObject,
			 IN UCHAR EntrySize)
{
  PERROR_LOG_ENTRY LogEntry;
  ULONG LogEntrySize;
  KIRQL Irql;

  DPRINT1 ("IoAllocateErrorLogEntry() called\n");

  if (IoObject == NULL)
    return NULL;

  KeAcquireSpinLock (&IopAllocationLock,
		     &Irql);

  if (IopTotalLogSize > PAGE_SIZE)
    {
      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
      return NULL;
    }

  LogEntrySize = sizeof(ERROR_LOG_ENTRY) + EntrySize;
  LogEntry = ExAllocatePool (NonPagedPool,
			     LogEntrySize);
  if (LogEntry == NULL)
    {
      KeReleaseSpinLock (&IopAllocationLock,
			 Irql);
      return NULL;
    }

  IopTotalLogSize += EntrySize;

  LogEntry->PacketSize = LogEntrySize;

  KeReleaseSpinLock (&IopAllocationLock,
		     Irql);

  return (PVOID)((ULONG_PTR)LogEntry + sizeof(ERROR_LOG_ENTRY));
}


/*
 * @implemented
 */
VOID STDCALL
IoWriteErrorLogEntry (IN PVOID ElEntry)
{
  PWORK_QUEUE_ITEM LogWorkItem;
  PERROR_LOG_ENTRY LogEntry;
  KIRQL Irql;

  DPRINT1 ("IoWriteErrorLogEntry() called\n");

  LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));

  /* Get time stamp */
  KeQuerySystemTime (&LogEntry->TimeStamp);

  KeAcquireSpinLock (&IopLogListLock,
		     &Irql);

  InsertHeadList (&IopLogListHead,
		  &LogEntry->Entry);

  if (IopLogWorkerRunning == FALSE)
    {
      LogWorkItem = ExAllocatePool (NonPagedPool,
				    sizeof(WORK_QUEUE_ITEM));
      if (LogWorkItem != NULL)
	{
	  ExInitializeWorkItem (LogWorkItem,
				IopLogWorker,
				LogWorkItem);

	  ExQueueWorkItem (LogWorkItem,
			   DelayedWorkQueue);

	  IopLogWorkerRunning = TRUE;
	}
    }

  KeReleaseSpinLock (&IopLogListLock,
		     Irql);

  DPRINT1 ("IoWriteErrorLogEntry() done\n");
}

/* EOF */
