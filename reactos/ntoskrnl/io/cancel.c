/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/cancel.c
 * PURPOSE:         Cancel routine
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK CancelSpinLock = {0,};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCancelIoFile(IN HANDLE FileHandle,
				OUT PIO_STATUS_BLOCK IoStatusBlock)
{
   return(ZwCancelIoFile(FileHandle,IoStatusBlock));
}

NTSTATUS STDCALL ZwCancelIoFile(IN HANDLE FileHandle,
				OUT PIO_STATUS_BLOCK IoStatusBlock)
{
   UNIMPLEMENTED;
}

BOOLEAN IoCancelIrp(PIRP Irp)
{
   KIRQL oldlvl;
   
   DPRINT("IoCancelIrp(Irp %x)\n",Irp);
   
   IoAcquireCancelSpinLock(&oldlvl);
   Irp->Cancel = TRUE;
   if (Irp->CancelRoutine==NULL)
     {
	return(FALSE);
     }
   Irp->CancelRoutine(Irp->Stack[0].DeviceObject,Irp);
   IoReleaseCancelSpinLock(&oldlvl);
   return(TRUE);
}

VOID IoInitCancelHandling(VOID)
{
   KeInitializeSpinLock(&CancelSpinLock);
}

VOID IoAcquireCancelSpinLock(PKIRQL Irql)
{
   KeAcquireSpinLock(&CancelSpinLock,Irql);
}

VOID IoReleaseCancelSpinLock(KIRQL Irql)
{
   KeReleaseSpinLock(&CancelSpinLock,Irql);
}

PDRIVER_CANCEL IoSetCancelRoutine(PIRP Irp, PDRIVER_CANCEL CancelRoutine)
{
   return((PDRIVER_CANCEL)InterlockedExchange((PULONG)&Irp->CancelRoutine,
					      (ULONG)CancelRoutine));
}
