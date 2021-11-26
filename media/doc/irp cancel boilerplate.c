

/*
Boiler plate for irp cancelation, for irp queues you manage yourself
-Gunnar
*/



CancelRoutine(
   DEV_OBJ Dev,
   Irp
   )
{
   //don't need this since we have our own sync. protecting irp cancellation
   IoReleaseCancelSpinLock(Irp->CancelIrql);

   theLock = Irp->Tail.Overlay.DriverContext[3];

   Lock(theLock);
   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
   Unlock(theLock);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

}


QUEUE_BOLIERPLATE
{
   Lock(theLock);

   Irp->Tail.Overlay.DriverContext[3] = &theLock;

   IoSetCancelRoutine(Irp, CancelRoutine);
   if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
   {
      /*
      Irp has already been cancelled (before we got to queue it),
      and we got to remove the cancel routine before the canceler could,
      so we cancel/complete the irp ourself.
      */

      Unlock(theLock);

      Irp->IoStatus.Status = STATUS_CANCELLED;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return FALSE;
   }

   //else were ok


   Irp->IoStatus.Status = STATUS_PENDING;
   IoMarkIrpPending(Irp);

   InsertTailList(Queue);

   Unlock(theLock);

}


DEQUEUE_BOILERPLATE
{
   Lock(theLock);

   Irp = RemoveHeadList(Queue);

   if (!IoSetCancelRoutine(Irp, NULL))
   {
      /*
      Cancel routine WILL be called after we release the spinlock. It will try to remove
      the irp from the list and cancel/complete this irp. Since we allready removed it,
      make its ListEntry point to itself.
      */

      InitializeListHead(&Irp->Tail.Overlay.ListEntry);

      Unlock(theLock);

      return;
   }


   /*
   Cancel routine will NOT be called, canceled or not.
   The Irp might have been canceled (Irp->Cancel flag set) but we don't care,
   since we are to complete this Irp now anyways.
   */

   Unlock(theLock);

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

}
