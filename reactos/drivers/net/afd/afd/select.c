/* $Id: select.c,v 1.6 2004/11/17 05:17:22 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/select.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

VOID CopyBackStatus( PAFD_HANDLE HandleArray,
		     UINT HandleCount ) {
    UINT i;
    
    for( i = 0; i < HandleCount; i++ ) {
	HandleArray[i].Events = HandleArray[i].Status;
	HandleArray[i].Status = 0;
    }
}

VOID ZeroEvents( PAFD_HANDLE HandleArray,
		 UINT HandleCount ) {
    UINT i;
    
    for( i = 0; i < HandleCount; i++ )
	HandleArray[i].Status = 0;
}

VOID RemoveSelect( PAFD_ACTIVE_POLL Poll ) {
    AFD_DbgPrint(MID_TRACE,("Called\n"));

    RemoveEntryList( &Poll->ListEntry );
    KeCancelTimer( &Poll->Timer );

    ExFreePool( Poll );

    AFD_DbgPrint(MID_TRACE,("Done\n"));
}

VOID SignalSocket( PAFD_ACTIVE_POLL Poll, PAFD_POLL_INFO PollReq, 
		   NTSTATUS Status, UINT Collected ) {
    PIRP Irp = Poll->Irp;
    AFD_DbgPrint(MID_TRACE,("Called (Status %x Events %d)\n", 
			    Status, Collected));
    Poll->Irp->IoStatus.Status = Status;
    Poll->Irp->IoStatus.Information = Collected;
    CopyBackStatus( PollReq->Handles,
		    PollReq->HandleCount );
    UnlockHandles( PollReq->InternalUse, PollReq->HandleCount );
    AFD_DbgPrint(MID_TRACE,("Completing\n"));
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    RemoveEntryList( &Poll->ListEntry );
    RemoveSelect( Poll );
    AFD_DbgPrint(MID_TRACE,("Done\n"));
}

VOID SelectTimeout( PKDPC Dpc,
		    PVOID DeferredContext,
		    PVOID SystemArgument1,
		    PVOID SystemArgument2 ) {
    PAFD_ACTIVE_POLL Poll = DeferredContext;
    PAFD_POLL_INFO PollReq;
    PIRP Irp;
    KIRQL OldIrql;
    PAFD_DEVICE_EXTENSION DeviceExt;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    Irp = Poll->Irp;
    DeviceExt = Poll->DeviceExt;
    PollReq = Irp->AssociatedIrp.SystemBuffer;

    ZeroEvents( PollReq->Handles, PollReq->HandleCount );

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );
    SignalSocket( Poll, PollReq, STATUS_TIMEOUT, 0 );
    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Timeout\n"));
}

NTSTATUS STDCALL
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	   PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_NO_MEMORY;
    PAFD_FCB FCB;
    PFILE_OBJECT FileObject;
    PAFD_POLL_INFO PollReq = Irp->AssociatedIrp.SystemBuffer;
    PAFD_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
    PAFD_ACTIVE_POLL Poll = NULL;
    UINT CopySize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    UINT AllocSize = 
	CopySize + sizeof(AFD_ACTIVE_POLL) - sizeof(AFD_POLL_INFO);
    KIRQL OldIrql;
    UINT i, Signalled = 0;

    AFD_DbgPrint(MID_TRACE,("Called (HandleCount %d Timeout %d)\n", 
			    PollReq->HandleCount,
			    (INT)(PollReq->Timeout.QuadPart)));

    PollReq->InternalUse = 
	LockHandles( PollReq->Handles, PollReq->HandleCount );

    if( !PollReq->InternalUse ) {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = -1;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	return Irp->IoStatus.Status;
    }

    ZeroEvents( PollReq->Handles,
		PollReq->HandleCount );

    Poll = ExAllocatePool( NonPagedPool, AllocSize );
    
    if( Poll ) {
	Poll->Irp = Irp;
	Poll->DeviceExt = DeviceExt;

	KeInitializeTimerEx( &Poll->Timer, NotificationTimer );
	KeSetTimer( &Poll->Timer, PollReq->Timeout, &Poll->TimeoutDpc );

	KeInitializeDpc( (PRKDPC)&Poll->TimeoutDpc, 
			 (PKDEFERRED_ROUTINE)SelectTimeout,
			 Poll );
	
	KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );
	InsertTailList( &DeviceExt->Polls, &Poll->ListEntry );

	for( i = 0; i < PollReq->HandleCount; i++ ) {
	    if( !PollReq->InternalUse[i].Handle ) continue;
	    
	    FileObject = (PFILE_OBJECT)PollReq->InternalUse[i].Handle;
	    FCB = FileObject->FsContext;
	    
	    if( (FCB->PollState & AFD_EVENT_CLOSE) ||
		(PollReq->Handles[i].Status & AFD_EVENT_CLOSE) ) {
		PollReq->InternalUse[i].Handle = 0;
		PollReq->Handles[i].Events = 0;
		PollReq->Handles[i].Status = AFD_EVENT_CLOSE;
		Signalled++;
	    } else {
		PollReq->Handles[i].Status = 
		    PollReq->Handles[i].Events & FCB->PollState;
		if( PollReq->Handles[i].Status ) {
		    AFD_DbgPrint(MID_TRACE,("Signalling %x with %x\n", 
					    FCB, FCB->PollState));
		    Signalled++;
		}
	    }
	}

	if( Signalled ) {
	    Status = STATUS_SUCCESS;
	    Irp->IoStatus.Status = Status;
	    Irp->IoStatus.Information = Signalled;
	    SignalSocket( Poll, PollReq, Status, Signalled );
	} else {
	    Status = STATUS_PENDING;
	    IoMarkIrpPending( Irp );
	}

	KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
    } else Status = STATUS_NO_MEMORY;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

/* * * NOTE ALWAYS CALLED AT DISPATCH_LEVEL * * */
BOOLEAN UpdatePollWithFCB( PAFD_ACTIVE_POLL Poll, PFILE_OBJECT FileObject ) {
    UINT i;
    PAFD_FCB FCB;
    UINT Signalled = 0;
    PAFD_POLL_INFO PollReq = Poll->Irp->AssociatedIrp.SystemBuffer;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    for( i = 0; i < PollReq->HandleCount; i++ ) {
	if( !PollReq->InternalUse[i].Handle ) continue;

	FileObject = (PFILE_OBJECT)PollReq->InternalUse[i].Handle;
	FCB = FileObject->FsContext;
	
	if( (FCB->PollState & AFD_EVENT_CLOSE) ||
	    (PollReq->Handles[i].Status & AFD_EVENT_CLOSE) ) {
	    PollReq->InternalUse[i].Handle = 0;
	    PollReq->Handles[i].Events = 0;
	    PollReq->Handles[i].Status = AFD_EVENT_CLOSE;
	    Signalled++;
	} else {
	    PollReq->Handles[i].Status = 
		PollReq->Handles[i].Events & FCB->PollState;
	    if( PollReq->Handles[i].Status ) {
		AFD_DbgPrint(MID_TRACE,("Signalling %x with %x\n", 
					FCB, FCB->PollState));
		Signalled++;
	    }
	}
    }

    return Signalled ? 1 : 0;
}

VOID PollReeval( PAFD_DEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject ) {
    PAFD_ACTIVE_POLL Poll = NULL;
    PLIST_ENTRY ThePollEnt = NULL;
    KIRQL OldIrql;
    PAFD_POLL_INFO PollReq;

    AFD_DbgPrint(MID_TRACE,("Called: DeviceExt %x FileObject %x\n", 
			    DeviceExt, FileObject));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    ThePollEnt = DeviceExt->Polls.Flink;

    while( ThePollEnt != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD( ThePollEnt, AFD_ACTIVE_POLL, ListEntry );
	PollReq = Poll->Irp->AssociatedIrp.SystemBuffer;
	AFD_DbgPrint(MID_TRACE,("Checking poll %x\n", Poll));

	if( UpdatePollWithFCB( Poll, FileObject ) ) {
	    ThePollEnt = ThePollEnt->Flink;
	    AFD_DbgPrint(MID_TRACE,("Signalling socket\n"));
	    SignalSocket( Poll, PollReq, STATUS_SUCCESS, 1 );
	} else 
	    ThePollEnt = ThePollEnt->Flink;
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));
}
