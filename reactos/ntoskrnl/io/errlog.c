/* $Id: errlog.c,v 1.10 2003/07/10 15:47:00 royce Exp $
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


typedef struct _IO_ERROR_LOG_PACKET
{
   UCHAR MajorFunctionCode;
   UCHAR RetryCount;
   USHORT DumpDataSize;
   USHORT NumberOfStrings;
   USHORT StringOffset;
   USHORT EventCategory;
   NTSTATUS ErrorCode;
   ULONG UniqueErrorValue;
   NTSTATUS FinalStatus;
   ULONG SequenceNumber;
   ULONG IoControlCode;
   LARGE_INTEGER DeviceOffset;
   ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

typedef struct _ERROR_LOG_ENTRY
{
  ULONG EntrySize;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;


/* GLOBALS *******************************************************************/

static KSPIN_LOCK IopAllocationLock;
static ULONG IopTotalLogSize;


/* FUNCTIONS *****************************************************************/

NTSTATUS
IopInitErrorLog (VOID)
{
  IopTotalLogSize = 0;
  KeInitializeSpinLock (&IopAllocationLock);

  return STATUS_SUCCESS;
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

  LogEntry->EntrySize = LogEntrySize;

  KeReleaseSpinLock (&IopAllocationLock,
		     Irql);

  return (PVOID)((ULONG_PTR)LogEntry + sizeof(ERROR_LOG_ENTRY));
}


/*
 * @unimplemented
 */
VOID STDCALL
IoWriteErrorLogEntry (IN PVOID ElEntry)
{
  PERROR_LOG_ENTRY LogEntry;
  KIRQL Irql;

  DPRINT1 ("IoWriteErrorLogEntry() called\n");

  LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));


  /* FIXME: Write log entry to the error log port or keep it in a list */


  /* Release error log entry */
  KeAcquireSpinLock (&IopAllocationLock,
		     &Irql);

  IopTotalLogSize -= LogEntry->EntrySize;
  ExFreePool (LogEntry);

  KeReleaseSpinLock (&IopAllocationLock,
		     Irql);
}

/* EOF */
