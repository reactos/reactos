/*
 * ReactOS Cancel-Safe Queue library
 * Copyright (c) 2004, Vizzini (vizzini@plasmic.com)
 * Licensed under the GNU GPL for the ReactOS project
 *
 * This file implements the ReactOS CSQ library.  For background and overview
 * information on these routines, read csq.h.   For the authoritative reference
 * to using these routines, see the current DDK (IoCsqXXX and CsqXxx callbacks).
 *
 * There are a couple of subtle races that this library is designed to avoid.
 * Please read the code (particularly IoCsqInsertIrpEx and IoCsqRemoveIrp) for
 * some details.  
 *
 * In general, we try here to avoid the race between these queue/dequeue 
 * interfaces and our own cancel routine.  This library supplies a cancel
 * routine that is used in all IRPs that are queued to it.  The major race
 * conditions surround the proper handling of in-between cases, such as in-progress
 * queue and de-queue operations.
 *
 * When you're thinking about these operations, keep in mind that three or four
 * processors can have queue and dequeue operations in progress simultaneously,
 * and a user thread may cancel any IRP at any time.  Also, these operations don't
 * all happen at DISPATCH_LEVEL all of the time, so thread switching on a single
 * processor can create races too.
 */
/* $Id: csq.c,v 1.4 2004/07/04 08:30:28 hbirr Exp $ */

#define __NTDRIVER__
#include <ddk/ntddk.h>
#include <ddk/csq.h>


VOID NTAPI IopCsqCancelRoutine(PDEVICE_OBJECT DeviceObject,
                               PIRP Irp)
/*
 * FUNCTION: Cancel routine that is installed on any IRP that this library manages
 * ARGUMENTS:
 *     [Called back by the system]
 * NOTES:
 *     - We assume that Irp->Tail.Overlay.DriverContext[3] has either a IO_CSQ
 *       or an IO_CSQ_IRP_CONTEXT in it, but we have to figure out which it is
 *     - By the time this routine executes, the I/O Manager has already cleared
 *       the cancel routine pointer in the IRP, so it will only be canceled once
 *     - Because of this, we're guaranteed that Irp is valid the whole time
 *     - Don't forget to release the cancel spinlock ASAP --> #1 hot lock in the
 *       system
 *     - May be called at high IRQL
 */
{
	PIO_CSQ Csq;
	KIRQL Irql;

	/* First things first: */
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	/* We could either get a context or just a csq */
	Csq = (PIO_CSQ)Irp->Tail.Overlay.DriverContext[3];

	if(Csq->Type == IO_TYPE_CSQ_IRP_CONTEXT)
		{
			PIO_CSQ_IRP_CONTEXT Context = (PIO_CSQ_IRP_CONTEXT)Csq;
			Csq = Context->Csq;
			
			/* clean up context while we're here */
			Context->Irp = NULL;
		}

	/* Now that we have our CSQ, complete the IRP */
	Csq->CsqAcquireLock(Csq, &Irql);
		{
			Csq->CsqRemoveIrp(Csq, Irp);
			Csq->CsqCompleteCanceledIrp(Csq, Irp);
		}
	Csq->CsqReleaseLock(Csq, Irql);
}


NTSTATUS NTAPI IoCsqInitialize(PIO_CSQ Csq,
                               PIO_CSQ_INSERT_IRP CsqInsertIrp,
                               PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
                               PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
                               PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
                               PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
                               PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp)
/*
 * FUNCTION: Set up a CSQ struct to initialize the queue
 * ARGUMENTS:
 *     Csq: Caller-allocated non-paged space for our IO_CSQ to be initialized
 *     CsqInsertIrp: Insert routine
 *     CsqRemoveIrp: Remove routine
 *     CsqPeekNextIrp: Routine to paeek at the next IRP in queue
 *     CsqAcquireLock: Acquire the queue's lock
 *     CsqReleaseLock: Release the queue's lock
 *     CsqCompleteCanceledIrp: Routine to complete IRPs when they are canceled
 * RETURNS:
 *     - STATUS_SUCCESS in all cases
 * NOTES:
 *     - Csq must be non-paged, as the queue is manipulated with a held spinlock
 */
{
	Csq->Type = IO_TYPE_CSQ;
	Csq->CsqInsertIrp = CsqInsertIrp;
	Csq->CsqRemoveIrp = CsqRemoveIrp;
	Csq->CsqPeekNextIrp = CsqPeekNextIrp;
	Csq->CsqAcquireLock = CsqAcquireLock;
	Csq->CsqReleaseLock = CsqReleaseLock;
	Csq->CsqCompleteCanceledIrp = CsqCompleteCanceledIrp;
	Csq->ReservePointer = NULL;

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI IoCsqInitializeEx(PIO_CSQ Csq,
                                 PIO_CSQ_INSERT_IRP_EX CsqInsertIrpEx,
                                 PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
                                 PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
                                 PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
                                 PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
                                 PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp)
/*
 * FUNCTION: Set up a CSQ struct to initialize the queue (extended version)
 * ARGUMENTS:
 *     Csq: Caller-allocated non-paged space for our IO_CSQ to be initialized
 *     CsqInsertIrpEx: Extended insert routine
 *     CsqRemoveIrp: Remove routine
 *     CsqPeekNextIrp: Routine to paeek at the next IRP in queue
 *     CsqAcquireLock: Acquire the queue's lock
 *     CsqReleaseLock: Release the queue's lock
 *     CsqCompleteCanceledIrp: Routine to complete IRPs when they are canceled
 * RETURNS:
 *     - STATUS_SUCCESS in all cases
 * NOTES:
 *     - Csq must be non-paged, as the queue is manipulated with a held spinlock
 */
{
	Csq->Type = IO_TYPE_CSQ_EX;
	Csq->CsqInsertIrp = (PIO_CSQ_INSERT_IRP)CsqInsertIrpEx;
	Csq->CsqRemoveIrp = CsqRemoveIrp;
	Csq->CsqPeekNextIrp = CsqPeekNextIrp;
	Csq->CsqAcquireLock = CsqAcquireLock;
	Csq->CsqReleaseLock = CsqReleaseLock;
	Csq->CsqCompleteCanceledIrp = CsqCompleteCanceledIrp;
	Csq->ReservePointer = NULL;

	return STATUS_SUCCESS;
}


VOID NTAPI IoCsqInsertIrp(PIO_CSQ Csq,
                          PIRP Irp,
                          PIO_CSQ_IRP_CONTEXT Context)
/*
 * FUNCTION: Insert an IRP into the CSQ
 * ARGUMENTS:
 *     Csq: Pointer to the initialized CSQ
 *     Irp: Pointer to the IRP to queue
 *     Context: Context record to track the IRP while queued
 * NOTES:
 *     - Just passes through to IoCsqInsertIrpEx, with no InsertContext
 */
{
	IoCsqInsertIrpEx(Csq, Irp, Context, 0);
}


NTSTATUS NTAPI IoCsqInsertIrpEx(PIO_CSQ Csq,
                                PIRP Irp,
                                PIO_CSQ_IRP_CONTEXT Context,
                                PVOID InsertContext)
/*
 * FUNCTION: Insert an IRP into the CSQ, with additional tracking context
 * ARGUMENTS:
 *     Csq: Pointer to the initialized CSQ
 *     Irp: Pointer to the IRP to queue
 *     Context: Context record to track the IRP while queued
 *     InsertContext: additional data that is passed through to CsqInsertIrpEx
 * NOTES:
 *     - Passes the additional context through to the driver-supplied callback,
 *       which can be used with more sophistocated queues
 *     - Marks the IRP pending in all cases
 *     - Guaranteed to not queue a canceled IRP
 *     - This is complicated logic, and is patterend after the Microsoft library.
 *       I'm sure I have gotten the details wrong on a fine point or two, but
 *       basically this works with the MS-supplied samples.
 */
{
	NTSTATUS Retval = STATUS_SUCCESS;
	KIRQL Irql;

	Csq->CsqAcquireLock(Csq, &Irql);

	do
		{
			/* mark all irps pending -- says so in the cancel sample */
			IoMarkIrpPending(Irp);

			/* set up the context if we have one */
			if(Context)
				{
					Context->Type = IO_TYPE_CSQ_IRP_CONTEXT;
					Context->Irp = Irp;
					Context->Csq = Csq;
					Irp->Tail.Overlay.DriverContext[3] = Context;
				}
			else
				Irp->Tail.Overlay.DriverContext[3] = Csq;

			/*
			 * NOTE!  This is very sensitive to order.  If you set the cancel routine
			 * *before* you queue the IRP, our cancel routine will get called back for
			 * an IRP that isn't in its queue.
			 *
			 * There are three possibilities:
			 * 1) We get an IRP, we queue it, and it is valid the whole way
			 * 2) We get an IRP, and the IO manager cancels it before we're done here
			 * 3) We get an IRP, queue it, and the IO manager cancels it.
			 *
			 * #2 is is a booger.  
			 *
			 * When the IO manger receives a request to cancel an IRP, it sets the cancel
			 * bit in the IRP's control byte to TRUE.  Then, it looks to see if a cancel
			 * routine is set.  If it isn't, the IO manager just returns to the caller.
			 * If there *is* a routine, it gets called.
			 *
			 * If we test for cancel first and then set the cancel routine, there is a spot
			 * between test and set that the IO manager can cancel us without our knowledge,
			 * so we miss a cancel request.  That is bad.
			 *
			 * If we set a routine first and then test for cancel, we race with our completion
			 * routine:  We set the routine, the IO Manager sets cancel, we test cancel and find
			 * it is TRUE.  Meanwhile the IO manager has called our cancel routine already, so 
			 * we can't complete the IRP because it'll rip it out from under the cancel routine.
			 *
			 * The IO manager does us a favor though: it nulls out the cancel routine in the IRP
			 * before calling it.  Therefore, if we test to see if the cancel routine is NULL
			 * (after we have just set it), that means our own cancel routine is already working
			 * on the IRP, and we can just return quietly.  Otherwise, we have to de-queue the
			 * IRP and cancel it ourselves.
			 *
			 * We have to go through all of this mess because this API guarantees that we will
			 * never return having left a canceled IRP in the queue.
			 */
			
			/* Step 1: Queue the IRP */
			if(Csq->Type == IO_TYPE_CSQ)
				Csq->CsqInsertIrp(Csq, Irp);
			else
				{
					PIO_CSQ_INSERT_IRP_EX pCsqInsertIrpEx = (PIO_CSQ_INSERT_IRP_EX)Csq->CsqInsertIrp;
					Retval = pCsqInsertIrpEx(Csq, Irp, InsertContext);
					if(Retval != STATUS_SUCCESS)
						break;
				}

			/* Step 2: Set our cancel routine */
			IoSetCancelRoutine(Irp, IopCsqCancelRoutine);

			/* Step 3: Deal with an IRP that is already canceled */
			if(!Irp->Cancel)
				break;

			/* 
			 * Since we're canceled, see if our cancel routine is already running 
			 * If this is NULL, the IO Manager has already called our cancel routine 
			 */
			if(!IoSetCancelRoutine(Irp, NULL))
				break;

			/* OK, looks like we have to de-queue and complete this ourselves */
			Csq->CsqRemoveIrp(Csq, Irp);
			Csq->CsqCompleteCanceledIrp(Csq, Irp);
			
			if(Context)
				Context->Irp = NULL;
		}
	while(0);

	Csq->CsqReleaseLock(Csq, Irql);

	return Retval;
}


PIRP NTAPI IoCsqRemoveIrp(PIO_CSQ Csq,
                          PIO_CSQ_IRP_CONTEXT Context)
/*
 * FUNCTION: Remove anb IRP from the queue
 * ARGUMENTS:
 *     Csq: Queue to remove the IRP from
 *     Context: Context record containing the IRP to be dequeued
 * RETURNS:
 *     - Pointer to an IRP if we found it
 * NOTES:
 *     - Don't forget that we can be canceled any time up to the point 
 *       where we unset our cancel routine
 */
{
	KIRQL Irql;
	PIRP Irp = NULL;

	Csq->CsqAcquireLock(Csq, &Irql);

	do
		{
			/* It's possible that this IRP could have been canceled */
			Irp = Context->Irp;

			if(!Irp)
				break;

			/* Unset the cancel routine and see if it has already been canceled */
			if(!IoSetCancelRoutine(Irp, NULL))
				{
					/* 
					 * already gone, return NULL  --> NOTE  that we cannot touch this IRP *or* the context,
					 * since the context is being simultaneously twiddled by the cancel routine
					 */ 
					Irp = NULL;
					break;
				}

			/* This IRP is valid and is ours.  Dequeue it, fix it up, and return */
			Csq->CsqRemoveIrp(Csq, Irp);

			Context = (PIO_CSQ_IRP_CONTEXT)InterlockedExchangePointer(&Irp->Tail.Overlay.DriverContext[3], NULL);

			if(Context && Context->Type == IO_TYPE_CSQ_IRP_CONTEXT)
				Context->Irp = NULL;
		}
	while(0);

	Csq->CsqReleaseLock(Csq, Irql);

	return Irp;
}

PIRP NTAPI IoCsqRemoveNextIrp(PIO_CSQ Csq,
                              PVOID PeekContext)
/*
 * FUNCTION: IoCsqRemoveNextIrp - Removes the next IRP from the queue
 * ARGUMENTS:
 *     Csq: Queue to remove the IRP from
 *     PeekContext: Identifier of the IRP to be removed
 * RETURNS:
 *     Pointer to the IRP that was removed, or NULL if one
 *     could not be found
 * NOTES:
 *     - This function is sensitive to yet another race condition.
 *       The basic idea is that we have to return the first IRP that
 *       we get that matches the PeekContext >that is not already canceled<.
 *       Therefore, we have to do a trick similar to the one done in Insert
 *       above.
 */
{
	KIRQL Irql;
	PIRP Irp = NULL;
	PIO_CSQ_IRP_CONTEXT Context;

	Csq->CsqAcquireLock(Csq, &Irql);

	while((Irp = Csq->CsqPeekNextIrp(Csq, Irp, PeekContext)))
		{
			/* 
			 * If the cancel routine is gone, we're already canceled,
			 * and are spinning on the queue lock in our own cancel
			 * routine.  Move on to the next candidate.  It'll get
			 * removed by the cance routine.
			 */
			if(!IoSetCancelRoutine(Irp, NULL))
				continue;

			Csq->CsqRemoveIrp(Csq, Irp);

			/* Unset the context stuff and return */
			Context = (PIO_CSQ_IRP_CONTEXT)InterlockedExchangePointer(&Irp->Tail.Overlay.DriverContext[3], NULL);

			if(Context && Context->Type == IO_TYPE_CSQ_IRP_CONTEXT)
				Context->Irp = NULL;
			
			break;
		}

	Csq->CsqReleaseLock(Csq, Irql);

	return Irp;
}

