/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/cancel.c
 * PURPOSE:         Cancel routine
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK CancelSpinLock;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtCancelIoFile (IN	HANDLE			FileHandle,
		OUT	PIO_STATUS_BLOCK	IoStatusBlock)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
BOOLEAN STDCALL 
IoCancelIrp(PIRP Irp)
{
   KIRQL oldlvl;
   
   DPRINT("IoCancelIrp(Irp %x)\n",Irp);
   
   IoAcquireCancelSpinLock(&oldlvl);
   Irp->Cancel = TRUE;
   if (Irp->CancelRoutine == NULL)
   {
      IoReleaseCancelSpinLock(oldlvl);
      return(FALSE);
   }
   Irp->CancelIrql = oldlvl;
   Irp->CancelRoutine(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
   return(TRUE);
}

VOID INIT_FUNCTION
IoInitCancelHandling(VOID)
{
   KeInitializeSpinLock(&CancelSpinLock);
}

/*
 * @implemented
 */
VOID STDCALL 
IoAcquireCancelSpinLock(PKIRQL Irql)
{
   KeAcquireSpinLock(&CancelSpinLock,Irql);
}

/*
 * @implemented
 */
VOID STDCALL 
IoReleaseCancelSpinLock(KIRQL Irql)
{
   KeReleaseSpinLock(&CancelSpinLock,Irql);
}

/* EOF */
