/*
 * Cancel-Safe Queue Library
 * Copyright (c) 2004, Vizzini (vizzini@plasmic.com)
 * Licensed under the GNU GPL for the ReactOS project
 *
 * This header defines the interface to the ReactOS Cancel-Safe Queue library.
 * This interface is based on and is similar to the Microsoft Cancel-Safe
 * Queue interface.
 *
 * BACKGROUND
 *
 * IRP queuing is a royal pain in the butt, due to the fact that there are
 * tons of built-in race conditions.  IRP handling is difficult in general,
 * but the cancel logic has been particularly complicated due to some subtle
 * races, coupled with the fact that the system interfaces have changed over
 * time.
 *
 * Walter Oney (2nd. Ed. of Programming the Windows Driver Model) states a
 * common opinion among driver developers when he says that it is foolish
 * to try to roll your own cancel logic.  There are only a very few people
 * who have gotten it right in the past.  He suggests, instead, that you
 * either use his own well-tested code, or use the code in the Microsoft
 * Cancel-Safe Queue Library.
 *
 * We cannot do either, of course, due to copyright issues.  I have therefore
 * created this clone of the Microsoft library in order to concentrate all
 * of the IRP-queuing bugs in one place.  I'm quite sure there are problems
 * here, so if you are a driver writer, I'd be glad to hear your feedback.
 *
 * Apart from that, please try to use these routines, rather than building
 * your own.  If you think you have found a bug, please bring it up with me
 * or on-list, as this is complicated and non-obvious stuff.  Don't just
 * change this and hope for the best!
 *
 * USAGE
 *
 * This library follows exactly the same interface as the Microsoft Cancel-Safe
 * Queue routines (IoCsqXxx()).  As such, the authoritative reference is the
 * current DDK.  There is also a DDK sample called "cancel" that has an
 * example of how to use this code.  I have also provided a sample driver
 * that makes use of this queue. Finally, please do read the header and the
 * source if you're curious about the inner workings of these routines.
 */

#ifndef _REACTOS_CSQ_H
#define _REACTOS_CSQ_H

/*
 * Prevent including the CSQ definitions twice. They're present in NTDDK
 * now too, except the *_EX versions.
 */
#ifndef IO_TYPE_CSQ_IRP_CONTEXT

struct _IO_CSQ;


/*
 * CSQ Callbacks
 *
 * The cancel-safe queue is implemented as a set of IoCsqXxx() OS routines
 * copuled with a set of driver callbacks to handle the basic operations of
 * the queue.  You need to supply one of each of these functions in your own
 * driver.  These routines are also documented in the DDK under CsqXxx().
 * That is the authoritative documentation.
 */

/*
 * Function to insert an IRP in the queue.  No need to worry about locking;
 * just tack it onto your list or something.
 *
 * Sample implementation: 
 *
	VOID NTAPI CsqInsertIrp(PIO_CSQ Csq, PIRP Irp)
	{
		KdPrint(("Inserting IRP 0x%x into CSQ\n", Irp));
		InsertTailList(&IrpQueue, &Irp->Tail.Overlay.ListEntry);
	}
 *
 */
typedef VOID (NTAPI *PIO_CSQ_INSERT_IRP) (struct _IO_CSQ *Csq,
                                          PIRP Irp);


/*
 * Function to remove an IRP from the queue.
 *
 * Sample:
 *
	VOID NTAPI CsqRemoveIrp(PIO_CSQ Csq, PIRP Irp)
	{
		KdPrint(("Removing IRP 0x%x from CSQ\n", Irp));
		RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
	}
 *
 */
typedef VOID (NTAPI *PIO_CSQ_REMOVE_IRP) (struct _IO_CSQ *Csq,
                                          PIRP Irp);

/*
 * Function to look for an IRP in the queue
 *
 * Sample:
 *
	PIRP NTAPI CsqPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext)
	{
		KdPrint(("Peeking for next IRP\n"));

		if(Irp)
			return CONTAINING_RECORD(&Irp->Tail.Overlay.ListEntry.Flink, IRP, Tail.Overlay.ListEntry);

		if(IsListEmpty(&IrpQueue))
			return NULL;

		return CONTAINING_RECORD(IrpQueue.Flink, IRP, Tail.Overlay.ListEntry);
	}
 *
 */
typedef PIRP (NTAPI *PIO_CSQ_PEEK_NEXT_IRP) (struct _IO_CSQ *Csq,
                                             PIRP Irp,
                                             PVOID PeekContext);

/*
 * Lock the queue.  This can be a spinlock, a mutex, or whatever
 * else floats your boat.
 *
 * Sample:
 *
	VOID NTAPI CsqAcquireLock(PIO_CSQ Csq, PKIRQL Irql)
	{
		KdPrint(("Acquiring spin lock\n"));
		KeAcquireSpinLock(&IrpQueueLock, Irql);
	}
 *
 */
typedef VOID (NTAPI *PIO_CSQ_ACQUIRE_LOCK) (struct _IO_CSQ *Csq,
                                            PKIRQL Irql);

/*
 * Unlock the queue:
 *
	VOID NTAPI CsqReleaseLock(PIO_CSQ Csq, KIRQL Irql)
	{
		KdPrint(("Releasing spin lock\n"));
		KeReleaseSpinLock(&IrpQueueLock, Irql);
	}
 *
 */
typedef VOID (NTAPI *PIO_CSQ_RELEASE_LOCK) (struct _IO_CSQ *Csq,
                                            KIRQL Irql);

/*
 * Finally, this is called by the queue library when it wants to complete
 * a canceled IRP.
 * 
 * Sample:
 *
	VOID NTAPI CsqCompleteCancelledIrp(PIO_CSQ Csq, PIRP Irp)
	{
		KdPrint(("cancelling irp 0x%x\n", Irp));
		Irp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
 *
 */
typedef VOID (NTAPI *PIO_CSQ_COMPLETE_CANCELED_IRP) (struct _IO_CSQ *Csq,
                                                     PIRP Irp);


/*
 * STRUCTURES
 *
 * NOTE:  Please do not use these directly.  You will make incompatible code
 * if you do.  Always only use the documented IoCsqXxx() interfaces and you
 * will amass much Good Karma.
 */
#define IO_TYPE_CSQ_IRP_CONTEXT 1
#define IO_TYPE_CSQ 2

/*
 * IO_CSQ - Queue control structure
 */
typedef struct _IO_CSQ {
	ULONG                          Type;
	PIO_CSQ_INSERT_IRP             CsqInsertIrp;
	PIO_CSQ_REMOVE_IRP             CsqRemoveIrp;
	PIO_CSQ_PEEK_NEXT_IRP          CsqPeekNextIrp;
	PIO_CSQ_ACQUIRE_LOCK           CsqAcquireLock;
	PIO_CSQ_RELEASE_LOCK           CsqReleaseLock;
	PIO_CSQ_COMPLETE_CANCELED_IRP  CsqCompleteCanceledIrp;
	PVOID                          ReservePointer;	/* must be NULL */
} IO_CSQ, *PIO_CSQ;

/*
 * IO_CSQ_IRP_CONTEXT - Context used to track an IRP in the CSQ
 */
typedef struct _IO_CSQ_IRP_CONTEXT {
	ULONG   Type;
	PIRP    Irp;
	PIO_CSQ Csq;
} IO_CSQ_IRP_CONTEXT, *PIO_CSQ_IRP_CONTEXT;

#endif /* IO_TYPE_CSQ_IRP_CONTEXT */

/* See IO_TYPE_CSQ_* above */
#define IO_TYPE_CSQ_EX 3

/*
 * Function to insert an IRP into the queue with extended context information.
 * This is useful if you need to be able to de-queue particular IRPs more
 * easily in some cases.
 *
 * Same deal as above; sample implementation:
 *
	NTSTATUS NTAPI CsqInsertIrpEx(PIO_CSQ Csq, PIRP Irp, PVOID InsertContext)
	{
		CsqInsertIrp(Csq, Irp);
		return STATUS_PENDING;
	}
 *
 */
typedef NTSTATUS (NTAPI *PIO_CSQ_INSERT_IRP_EX) (struct _IO_CSQ *Csq,
                                                 PIRP Irp,
                                                 PVOID InsertContext);

/*
 * CANCEL-SAFE QUEUE DDIs
 *
 * These device driver interfaces are called to make use of the queue.  Again,
 * authoritative documentation for these functions is in the DDK.  The csqtest
 * driver also makes use of some of them.
 */


/*
 * Call this in DriverEntry or similar in order to set up the Csq structure.
 * As long as the Csq struct and the functions you pass in are resident,
 * there are no IRQL restrictions.
 */
NTKERNELAPI
NTSTATUS NTAPI IoCsqInitialize(PIO_CSQ Csq,
                               PIO_CSQ_INSERT_IRP CsqInsertIrp,
                               PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
                               PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
                               PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
                               PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
                               PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp);

/*
 * Same as above, except you provide a CsqInsertIrpEx routine instead of
 * CsqInsertIrp.  This eventually allows you to supply extra tracking
 * information for use with the queue.
 */
NTKERNELAPI
NTSTATUS NTAPI IoCsqInitializeEx(PIO_CSQ Csq,
                                 PIO_CSQ_INSERT_IRP_EX CsqInsertIrpEx,
                                 PIO_CSQ_REMOVE_IRP CsqRemoveIrp,
                                 PIO_CSQ_PEEK_NEXT_IRP CsqPeekNextIrp,
                                 PIO_CSQ_ACQUIRE_LOCK CsqAcquireLock,
                                 PIO_CSQ_RELEASE_LOCK CsqReleaseLock,
                                 PIO_CSQ_COMPLETE_CANCELED_IRP CsqCompleteCanceledIrp);

/*
 * Insert an IRP into the queue
 */
NTKERNELAPI
VOID NTAPI IoCsqInsertIrp(PIO_CSQ Csq,
                          PIRP Irp,
                          PIO_CSQ_IRP_CONTEXT Context);

/*
 * Insert an IRP into the queue, with special context maintained that
 * makes it easy to find IRPs in the queue
 */
NTKERNELAPI
NTSTATUS NTAPI IoCsqInsertIrpEx(PIO_CSQ Csq,
                                PIRP Irp,
                                PIO_CSQ_IRP_CONTEXT Context,
                                PVOID InsertContext);

/*
 * Remove a particular IRP from the queue
 */
NTKERNELAPI
PIRP NTAPI IoCsqRemoveIrp(PIO_CSQ Csq,
                          PIO_CSQ_IRP_CONTEXT Context);

/*
 * Remove the next IRP from the queue 
 */
NTKERNELAPI
PIRP NTAPI IoCsqRemoveNextIrp(PIO_CSQ Csq,
                              PVOID PeekContext);

#endif /* _REACTOS_CSQ_H */
