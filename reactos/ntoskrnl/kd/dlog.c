/* $Id: dlog.c,v 1.13 2004/03/11 21:50:23 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdebug.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  21/10/99: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <roscfg.h>
#include <internal/kd.h>
#include <ntos/minmax.h>
#include <rosrtl/string.h>

/* GLOBALS *******************************************************************/

#define DEBUGLOG_SIZE (32*1024)

static CHAR DebugLog[DEBUGLOG_SIZE];
static ULONG DebugLogStart;
static ULONG DebugLogEnd;
static ULONG DebugLogCount;
static KSPIN_LOCK DebugLogLock;
static ULONG DebugLogOverflow;
static HANDLE DebugLogThreadHandle;
static CLIENT_ID DebugLogThreadCid;
static HANDLE DebugLogFile;
static KEVENT DebugLogEvent;

/* FUNCTIONS *****************************************************************/

VOID
DebugLogDumpMessages(VOID)
{
  static CHAR Buffer[256];
  ULONG Offset;
  ULONG Length;

  if (!(KdDebugState & KD_DEBUG_FILELOG))
    {
      return;
    }
  KdDebugState &= ~KD_DEBUG_FILELOG;
 
  Offset = (DebugLogEnd + 1) % DEBUGLOG_SIZE;
  do
    {
      if (Offset <= DebugLogEnd)
	{
	  Length = min(255, DebugLogEnd - Offset);
	}
      else
	{
	  Length = min(255, DEBUGLOG_SIZE - Offset);
	}
      memcpy(Buffer, DebugLog + Offset, Length);
      Buffer[Length] = 0;
      DbgPrint(Buffer);
      Offset = (Offset + Length) % DEBUGLOG_SIZE;
    }
  while (Length > 0);
}

VOID INIT_FUNCTION
DebugLogInit(VOID)
{
  KeInitializeSpinLock(&DebugLogLock);
  DebugLogStart = 0;
  DebugLogEnd = 0;
  DebugLogOverflow = 0;
  DebugLogCount = 0;
  KeInitializeEvent(&DebugLogEvent, NotificationEvent, FALSE);
}

VOID STDCALL
DebugLogThreadMain(PVOID Context)
{
  KIRQL oldIrql;
  IO_STATUS_BLOCK Iosb;
  static CHAR Buffer[256];
  ULONG WLen;

  for (;;)
    {
      LARGE_INTEGER TimeOut;
      TimeOut.QuadPart = -5000000; /* Half a second. */
      KeWaitForSingleObject(&DebugLogEvent,
			    0,
			    KernelMode,
			    FALSE,
			    &TimeOut);
      KeAcquireSpinLock(&DebugLogLock, &oldIrql);
      while (DebugLogCount > 0)
	{
	  if (DebugLogStart > DebugLogEnd)
	    {
	      WLen = min(256, DEBUGLOG_SIZE - DebugLogStart);
	      memcpy(Buffer, &DebugLog[DebugLogStart], WLen);
              Buffer[WLen + 1] = '\n';
	      DebugLogStart = 
		(DebugLogStart + WLen) % DEBUGLOG_SIZE;
	      DebugLogCount = DebugLogCount - WLen;
	      KeReleaseSpinLock(&DebugLogLock, oldIrql);
	      NtWriteFile(DebugLogFile,
			  NULL,
			  NULL,
			  NULL,
			  &Iosb,
			  Buffer,
			  WLen + 1,
			  NULL,
			  NULL);
	    }
	  else
	    {
	      WLen = min(255, DebugLogEnd - DebugLogStart);
	      memcpy(Buffer, &DebugLog[DebugLogStart], WLen);
	      DebugLogStart = 
		(DebugLogStart + WLen) % DEBUGLOG_SIZE;
	      DebugLogCount = DebugLogCount - WLen;
	      KeReleaseSpinLock(&DebugLogLock, oldIrql);
	      NtWriteFile(DebugLogFile,
			  NULL,
			  NULL,
			  NULL,
			  &Iosb,
			  Buffer,
			  WLen,
			  NULL,
			  NULL);
	    }
	  KeAcquireSpinLock(&DebugLogLock, &oldIrql);
	}
      KeResetEvent(&DebugLogEvent);
      KeReleaseSpinLock(&DebugLogLock, oldIrql);
    }
}

VOID INIT_FUNCTION
DebugLogInit2(VOID)
{
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK Iosb;

  RtlInitUnicodeString(&FileName, L"\\SystemRoot\\debug.log");
  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     0,
			     NULL,
			     NULL);
			     
  Status = NtCreateFile(&DebugLogFile,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_SUPERSEDE,
			FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to create debug log file\n");
      return;
    }

  Status = PsCreateSystemThread(&DebugLogThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&DebugLogThreadCid,
				DebugLogThreadMain,
				NULL);
}

VOID 
DebugLogWrite(PCH String)
{
  KIRQL oldIrql;

   if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
      DebugLogOverflow++;
      return;
    }

   KeAcquireSpinLock(&DebugLogLock, &oldIrql);

   if (DebugLogCount == DEBUGLOG_SIZE)
    {
      DebugLogOverflow++;
      KeReleaseSpinLock(&DebugLogLock, oldIrql);
      if (oldIrql < DISPATCH_LEVEL)
 	{
	  KeSetEvent(&DebugLogEvent, IO_NO_INCREMENT, FALSE);
 	}
      	return;
    }

   while ((*String) != 0)
    {
      DebugLog[DebugLogEnd] = *String;
      String++;
      DebugLogCount++;

 	if (DebugLogCount == DEBUGLOG_SIZE)
 	{	   
 	  DebugLogOverflow++;
 	  KeReleaseSpinLock(&DebugLogLock, oldIrql);
 	  if (oldIrql < DISPATCH_LEVEL)
 	    {
	      KeSetEvent(&DebugLogEvent, IO_NO_INCREMENT, FALSE);
 	    }
 	  return;
 	}
     DebugLogEnd = (DebugLogEnd + 1) % DEBUGLOG_SIZE;
    }

    KeReleaseSpinLock(&DebugLogLock, oldIrql);

    if (oldIrql < DISPATCH_LEVEL)
    {
      KeSetEvent(&DebugLogEvent, IO_NO_INCREMENT, FALSE);
    }
}

