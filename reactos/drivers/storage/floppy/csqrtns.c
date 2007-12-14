/*
 *  ReactOS Floppy Driver
 *  Copyright (C) 2004, Vizzini (vizzini@plasmic.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * PROJECT:         ReactOS Floppy Driver
 * FILE:            csqrtns.c
 * PURPOSE:         Cancel-safe queue routines
 * PROGRAMMER:      Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *                  15-Feb-2004 vizzini - Created
 * NOTES:
 *     - These functions provide the callbacks for the CSQ routines.
 *       They will be called automatically by the IoCsqXxx() routines.
 *       This driver uses the CSQ in the standard way.  In addition to
 *       queuing and de-queuing IRPs, the InsertIrp routine releases
 *       a semaphore every time an IRP is queued, allowing a queue management
 *       thread to properly drain the queue.
 *     - Note that the semaphore can get ahead of the number of IRPs in the
 *       queue if any are canceled; the queue management thread that de-queues
 *       IRPs is coded with that in mind.
 *     - For more information, see the csqtest driver in the ReactOS tree,
 *       or the cancel sample in recent (3790+) Microsoft DDKs.
 *     - Many of these routines are called at DISPATCH_LEVEL, due to the fact
 *       that my lock choice is a spin lock.
 */

#include <ntddk.h>
#define NDEBUG
#include <debug.h>

#include "floppy.h"
#include "csqrtns.h"

/* Global CSQ struct that the CSQ functions initialize and use */
IO_CSQ Csq;

/* List and lock for the actual IRP queue */
LIST_ENTRY IrpQueue;
KSPIN_LOCK IrpQueueLock;
KSEMAPHORE QueueSemaphore;

/*
 * CSQ Callbacks
 */


VOID NTAPI CsqRemoveIrp(PIO_CSQ UnusedCsq,
                        PIRP Irp)
/*
 * FUNCTION: Remove an IRP from the queue
 * ARGUMENTS:
 *     UnusedCsq: Pointer to CSQ context structure
 *     Irp: Pointer to the IRP to remove from the queue
 * NOTES:
 *     - Called under the protection of the queue lock
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  DPRINT("CSQ: Removing IRP 0x%p\n", Irp);
  RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}


PIRP NTAPI CsqPeekNextIrp(PIO_CSQ UnusedCsq,
                          PIRP Irp,
                          PVOID PeekContext)
/*
 * FUNCTION: Find the next matching IRP in the queue
 * ARGUMENTS:
 *     UnusedCsq: Pointer to CSQ context structure
 *     Irp: Pointer to a starting IRP in the queue (i.e. start search here)
 *     PeekContext: Unused
 * RETURNS:
 *     Pointer to an IRP that is next in line to be removed, if one can be found
 * NOTES:
 *     - This does *not* remove the IRP from the queue; it merely returns a pointer.
 *     - Called under the protection of the queue lock
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  UNREFERENCED_PARAMETER(PeekContext);
  DPRINT(("CSQ: Peeking for next IRP\n"));

  if(Irp)
    return CONTAINING_RECORD(&Irp->Tail.Overlay.ListEntry.Flink, IRP, Tail.Overlay.ListEntry);

  if(IsListEmpty(&IrpQueue))
    return NULL;

  return CONTAINING_RECORD(IrpQueue.Flink, IRP, Tail.Overlay.ListEntry);
}


VOID NTAPI CsqAcquireLock(PIO_CSQ UnusedCsq,
                          PKIRQL Irql)
/*
 * FUNCTION: Acquire the queue lock
 * ARGUMENTS:
 *     UnusedCsq: Pointer to CSQ context structure
 *     Irql: Pointer to a variable to store the old irql into
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  DPRINT(("CSQ: Acquiring spin lock\n"));
  KeAcquireSpinLock(&IrpQueueLock, Irql);
}


VOID NTAPI CsqReleaseLock(PIO_CSQ UnusedCsq,
                          KIRQL Irql)
/*
 * FUNCTION: Release the queue lock
 * ARGUMENTS:
 *     UnusedCsq: Pointer to CSQ context structure
 *     Irql: IRQL to lower to on release
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  DPRINT(("CSQ: Releasing spin lock\n"));
  KeReleaseSpinLock(&IrpQueueLock, Irql);
}


VOID NTAPI CsqCompleteCanceledIrp(PIO_CSQ UnusedCsq,
                                  PIRP Irp)
/*
 * FUNCTION: Complete a canceled IRP
 * ARGUMENTS:
 *    UnusedCsq: Pointer to CSQ context structure
 *    Irp: IRP to complete
 * NOTES:
 *    - Perhaps we should complete with something besides NO_INCREMENT
 *    - MS misspelled CANCELLED... sigh...
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  DPRINT("CSQ: Canceling irp 0x%p\n", Irp);
  Irp->IoStatus.Status = STATUS_CANCELLED;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


VOID NTAPI CsqInsertIrp(PIO_CSQ UnusedCsq,
                        PIRP Irp)
/*
 * FUNCTION: Queue an IRP
 * ARGUMENTS:
 *     UnusedCsq: Unused
 *     Irp: IRP to add to the queue
 * NOTES:
 *     - Called under the protection of the queue lock
 *     - Releases the semaphore for each queued packet, which is how
 *       the queue management thread knows that there might be
 *       an IRP in the queue
 *     - Note that the semaphore will get released more times than
 *       the queue management thread will have IRPs to process, given
 *       that at least one IRP is canceled at some point
 */
{
  UNREFERENCED_PARAMETER(UnusedCsq);
  DPRINT("CSQ: Inserting IRP 0x%p\n", Irp);
  InsertTailList(&IrpQueue, &Irp->Tail.Overlay.ListEntry);
  KeReleaseSemaphore(&QueueSemaphore, 0, 1, FALSE);
}

