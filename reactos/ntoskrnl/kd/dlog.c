/* $Id: dlog.c,v 1.2 2001/03/25 18:56:12 dwelch Exp $
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
#include <internal/kd.h>
#include <ntos/minmax.h>

/* GLOBALS *******************************************************************/

#define DEBUGLOG_SIZE (32*1024)

#ifdef DBGPRINT_FILE_LOG

static CHAR DebugLog[DEBUGLOG_SIZE];
static ULONG DebugLogStart;
static ULONG DebugLogEnd;
static ULONG DebugLogCount;
static KSPIN_LOCK DebugLogLock;
static ULONG DebugLogOverflow;
static HANDLE DebugLogThreadHandle;
static CLIENT_ID DebugLogThreadCid;
static HANDLE DebugLogFile;
static KSEMAPHORE DebugLogSem;

#endif /* DBGPRINT_FILE_LOG */

/* FUNCTIONS *****************************************************************/

#ifdef DBGPRINT_FILE_LOG

VOID
DebugLogInit(VOID)
{
  KeInitializeSpinLock(&DebugLogLock);
  DebugLogStart = 0;
  DebugLogEnd = 0;
  DebugLogOverflow = 0;
  DebugLogCount = 0;
  KeInitializeSemaphore(&DebugLogSem, 0, 255);
}

NTSTATUS
DebugLogThreadMain(PVOID Context)
{
  KIRQL oldIrql;
  IO_STATUS_BLOCK Iosb;
  static CHAR Buffer[256];
  ULONG WLen;

  for (;;)
    {
      KeWaitForSingleObject(&DebugLogSem,
			    0,
			    KernelMode,
			    FALSE,
			    NULL);
      KeAcquireSpinLock(&DebugLogLock, &oldIrql);
      while (DebugLogCount > 0)
	{
	  if (DebugLogStart > DebugLogEnd)
	    {
	      WLen = min(256, DEBUGLOG_SIZE - DebugLogStart);
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
	  else
	    {
	      WLen = min(256, DebugLogEnd - DebugLogStart);
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
      KeReleaseSpinLock(&DebugLogLock, oldIrql);
    }
}

VOID
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
	   KeReleaseSemaphore(&DebugLogSem, IO_NO_INCREMENT, 1, FALSE);
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
	       KeReleaseSemaphore(&DebugLogSem, IO_NO_INCREMENT, 1, FALSE);
	     }
	   return;
	 }
      DebugLogEnd = (DebugLogEnd + 1) % DEBUGLOG_SIZE;
     }

   KeReleaseSpinLock(&DebugLogLock, oldIrql);
   if (oldIrql < DISPATCH_LEVEL)
     {
       KeReleaseSemaphore(&DebugLogSem, IO_NO_INCREMENT, 1, FALSE);
     }
 }

 #else /* not DBGPRINT_FILE_LOG */

 VOID
 DebugLogInit(VOID)
 {
 }

VOID
DebugLogInit2(VOID)
{
}

VOID
DebugLogWrite(PCH String)
{
}

#endif /* DBGPRINT_FILE_LOG */

