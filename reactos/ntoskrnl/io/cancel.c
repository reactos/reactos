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

/**
 * @name NtCancelIoFile
 *
 * Cancel all pending I/O operations in the current thread for specified 
 * file object.
 *
 * @param FileHandle
 *        Handle to file object to cancel requests for. No specific
 *        access rights are needed.
 * @param IoStatusBlock
 *        Pointer to status block which is filled with final completition
 *        status on successful return.
 *
 * @return Status.
 *
 * @implemented
 */

NTSTATUS STDCALL
NtCancelIoFile(
   IN HANDLE FileHandle,
   OUT PIO_STATUS_BLOCK IoStatusBlock)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PETHREAD Thread;
   PLIST_ENTRY IrpEntry;
   PIRP Irp;
   KIRQL OldIrql;
   BOOLEAN OurIrpsInList = FALSE;
   LARGE_INTEGER Interval;

   if ((ULONG_PTR)IoStatusBlock >= MmUserProbeAddress &&
       KeGetPreviousMode() == UserMode)
      return STATUS_ACCESS_VIOLATION;

   Status = ObReferenceObjectByHandle(FileHandle, 0, IoFileObjectType,
                                      KeGetPreviousMode(), (PVOID*)&FileObject,
                                      NULL);
   if (!NT_SUCCESS(Status))
      return Status;

   /* IRP cancellations are synchronized at APC_LEVEL. */
   OldIrql = KfRaiseIrql(APC_LEVEL);

   /*
    * Walk the list of active IRPs and cancel the ones that belong to
    * our file object.
    */

   Thread = PsGetCurrentThread();
   for (IrpEntry = Thread->IrpList.Flink;
        IrpEntry != &Thread->IrpList;
        IrpEntry = IrpEntry->Flink)
   {
      Irp = CONTAINING_RECORD(IrpEntry, IRP, ThreadListEntry);
      if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
      {
         IoCancelIrp(Irp);
         /* Don't break here, we want to cancel all IRPs for the file object. */
         OurIrpsInList = TRUE;
      }
   }   

   KfLowerIrql(OldIrql);

   while (OurIrpsInList)
   {
      OurIrpsInList = FALSE;

      /* Wait a short while and then look if all our IRPs were completed. */
      Interval.QuadPart = -1000000; /* 100 milliseconds */
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);

      OldIrql = KfRaiseIrql(APC_LEVEL);

      /*
       * Look in the list if all IRPs for the specified file object
       * are completed (or cancelled). If someone sends a new IRP
       * for our file object while we're here we can happily loop
       * forever.
       */

      for (IrpEntry = Thread->IrpList.Flink;
           IrpEntry != &Thread->IrpList;
           IrpEntry = IrpEntry->Flink)
      {
         Irp = CONTAINING_RECORD(IrpEntry, IRP, ThreadListEntry);
         if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
         {
            OurIrpsInList = TRUE;
            break;
         }
      }

      KfLowerIrql(OldIrql);
   }

   _SEH_TRY
   {
      IoStatusBlock->Status = STATUS_SUCCESS;
      IoStatusBlock->Information = 0;
      Status = STATUS_SUCCESS;
   }
   _SEH_HANDLE
   {
      Status = STATUS_UNSUCCESSFUL;
   }
   _SEH_END;

   ObDereferenceObject(FileObject);

   return Status;
}

/**
 * @name IoCancelThreadIo
 *
 * Cancel all pending I/O request associated with specified thread.
 *
 * @param Thread
 *        Thread to cancel requests for.
 */

VOID STDCALL
IoCancelThreadIo(PETHREAD Thread)
{
   PLIST_ENTRY IrpEntry;
   PIRP Irp;
   KIRQL OldIrql;
   ULONG Retries = 3000;
   LARGE_INTEGER Interval;

   OldIrql = KfRaiseIrql(APC_LEVEL);

   /*
    * Start by cancelling all the IRPs in the current thread queue.
    */

   for (IrpEntry = Thread->IrpList.Flink;
        IrpEntry != &Thread->IrpList;
        IrpEntry = IrpEntry->Flink)
   {
      Irp = CONTAINING_RECORD(IrpEntry, IRP, ThreadListEntry);
      IoCancelIrp(Irp);
   }

   /*
    * Wait till all the IRPs are completed or cancelled.
    */

   while (!IsListEmpty(&Thread->IrpList))
   {
      KfLowerIrql(OldIrql);

      /* Wait a short while and then look if all our IRPs were completed. */
      Interval.QuadPart = -1000000; /* 100 milliseconds */
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);

      /*
       * Don't stay here forever if some broken driver doesn't complete
       * the IRP.
       */

      if (Retries-- == 0)
      {
         /* FIXME: Handle this gracefully. */
         DPRINT1("Thread with dead IRPs!");
         ASSERT(FALSE);
      }
      
      OldIrql = KfRaiseIrql(APC_LEVEL);
   }

   KfLowerIrql(OldIrql);
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
