/* $Id$
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
		   NTSTATUS Status ) {
    int i;
    PIRP Irp = Poll->Irp;
    AFD_DbgPrint(MID_TRACE,("Called (Status %x)\n", Status));
    KeCancelTimer( &Poll->Timer );
    Poll->Irp->IoStatus.Status = Status;
    Poll->Irp->IoStatus.Information =
        FIELD_OFFSET(AFD_POLL_INFO, Handles) + sizeof(AFD_HANDLE) * PollReq->HandleCount;
    CopyBackStatus( PollReq->Handles,
		    PollReq->HandleCount );
    for( i = 0; i < PollReq->HandleCount; i++ ) {
        AFD_DbgPrint
            (MAX_TRACE,
             ("Handle(%x): Got %x,%x\n",
              PollReq->Handles[i].Handle,
              PollReq->Handles[i].Events,
              PollReq->Handles[i].Status));
    }
    UnlockHandles( AFD_HANDLES(PollReq), PollReq->HandleCount );
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
    SignalSocket( Poll, PollReq, STATUS_TIMEOUT );
    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Timeout\n"));
}

VOID KillSelectsForFCB( PAFD_DEVICE_EXTENSION DeviceExt, 
                        PFILE_OBJECT FileObject ) {
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PAFD_ACTIVE_POLL Poll;
    PIRP Irp;
    PAFD_POLL_INFO PollReq;
    int i;

    AFD_DbgPrint(MID_TRACE,("Killing selects that refer to %x\n", FileObject));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    ListEntry = DeviceExt->Polls.Flink;
    while ( ListEntry != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD(ListEntry, AFD_ACTIVE_POLL, ListEntry);
	ListEntry = ListEntry->Flink;
        Irp = Poll->Irp;
        PollReq = Irp->AssociatedIrp.SystemBuffer; 
        
        for( i = 0; i < PollReq->HandleCount; i++ ) {
            AFD_DbgPrint(MAX_TRACE,("Req: %x, This %x\n",
                                    PollReq->Handles[i].Handle, FileObject));
            if( (PVOID)PollReq->Handles[i].Handle == FileObject ) {
                ZeroEvents( PollReq->Handles, PollReq->HandleCount );
                SignalSocket( Poll, PollReq, STATUS_SUCCESS );
            }
	}
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Done\n"));
}

VOID KillExclusiveSelects( PAFD_DEVICE_EXTENSION DeviceExt ) {
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PAFD_ACTIVE_POLL Poll;
    PIRP Irp;
    PAFD_POLL_INFO PollReq;

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    ListEntry = DeviceExt->Polls.Flink;
    while ( ListEntry != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD(ListEntry, AFD_ACTIVE_POLL, ListEntry);
	ListEntry = ListEntry->Flink;
	if( Poll->Exclusive ) {
	    Irp = Poll->Irp;
	    PollReq = Irp->AssociatedIrp.SystemBuffer;
	    ZeroEvents( PollReq->Handles, PollReq->HandleCount );
	    SignalSocket( Poll, PollReq, STATUS_CANCELLED );
	}
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
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
    ULONG Exclusive = PollReq->Exclusive;

    AFD_DbgPrint(MID_TRACE,("Called (HandleCount %d Timeout %d)\n", 
			    PollReq->HandleCount,
			    (INT)(PollReq->Timeout.QuadPart)));

    SET_AFD_HANDLES(PollReq,
		    LockHandles( PollReq->Handles, PollReq->HandleCount ));

    if( Exclusive ) KillExclusiveSelects( DeviceExt );
	

    if( !AFD_HANDLES(PollReq) ) {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	return Irp->IoStatus.Status;
    }

    ZeroEvents( PollReq->Handles,
		PollReq->HandleCount );

    Poll = ExAllocatePool( NonPagedPool, AllocSize );
    
    if( Poll ) {
	Poll->Irp = Irp;
	Poll->DeviceExt = DeviceExt;
	Poll->Exclusive = Exclusive;

	KeInitializeTimerEx( &Poll->Timer, NotificationTimer );
	KeSetTimer( &Poll->Timer, PollReq->Timeout, &Poll->TimeoutDpc );

	KeInitializeDpc( (PRKDPC)&Poll->TimeoutDpc, 
			 (PKDEFERRED_ROUTINE)SelectTimeout,
			 Poll );
	
	KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );
	InsertTailList( &DeviceExt->Polls, &Poll->ListEntry );

	for( i = 0; i < PollReq->HandleCount; i++ ) {
	    if( !AFD_HANDLES(PollReq)[i].Handle ) continue;
	    
	    FileObject = (PFILE_OBJECT)AFD_HANDLES(PollReq)[i].Handle;
	    FCB = FileObject->FsContext;
	    
	    if( (FCB->PollState & AFD_EVENT_CLOSE) ||
		(PollReq->Handles[i].Status & AFD_EVENT_CLOSE) ) {
		AFD_HANDLES(PollReq)[i].Handle = 0;
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
	    SignalSocket( Poll, PollReq, Status );
	} else {
	    Status = STATUS_PENDING;
	    IoMarkIrpPending( Irp );
	}

	KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
    } else Status = STATUS_NO_MEMORY;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

NTSTATUS STDCALL
AfdEventSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
		PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    NTSTATUS Status = STATUS_NO_MEMORY;
    PAFD_EVENT_SELECT_INFO EventSelectInfo = 
	(PAFD_EVENT_SELECT_INFO)LockRequest( Irp, IrpSp );
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,("Called (Event %x Triggers %x)\n", 
			    EventSelectInfo->EventObject,
			    EventSelectInfo->Events));
    
    if( !SocketAcquireStateLock( FCB ) ) {
	UnlockRequest( Irp, IrpSp );
	return LostSocket( Irp, FALSE );
    }

    FCB->EventSelectTriggers = FCB->EventsFired = 0;
    if( FCB->EventSelect ) ObDereferenceObject( FCB->EventSelect );
    FCB->EventSelect = NULL;

    if( EventSelectInfo->EventObject && EventSelectInfo->Events ) {
	Status = ObReferenceObjectByHandle( (PVOID)EventSelectInfo->
					    EventObject,
					    FILE_ALL_ACCESS,
					    NULL,
					    KernelMode,
					    (PVOID *)&FCB->EventSelect,
					    NULL );
	
	if( !NT_SUCCESS(Status) )
	    FCB->EventSelect = NULL;
	else
	    FCB->EventSelectTriggers = EventSelectInfo->Events;
    } else /* Work done, cancelling select */
	Status = STATUS_SUCCESS;
    
    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return UnlockAndMaybeComplete( FCB, STATUS_SUCCESS, Irp,
				   0, NULL, TRUE );
}

NTSTATUS STDCALL
AfdEnumEvents( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	       PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_ENUM_NETWORK_EVENTS_INFO EnumReq = 
	(PAFD_ENUM_NETWORK_EVENTS_INFO)LockRequest( Irp, IrpSp );
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,("Called (FCB %x)\n", FCB));
    
    if( !SocketAcquireStateLock( FCB ) ) {
	UnlockRequest( Irp, IrpSp );
	return LostSocket( Irp, FALSE );
    }

    EnumReq->PollEvents = FCB->PollState;
    RtlZeroMemory( EnumReq->EventStatus, sizeof(EnumReq->EventStatus) );
    
    return UnlockAndMaybeComplete( FCB, STATUS_SUCCESS, Irp,
				   0, NULL, TRUE );
}

/* * * NOTE ALWAYS CALLED AT DISPATCH_LEVEL * * */
BOOLEAN UpdatePollWithFCB( PAFD_ACTIVE_POLL Poll, PFILE_OBJECT FileObject ) {
    UINT i;
    PAFD_FCB FCB;
    UINT Signalled = 0;
    PAFD_POLL_INFO PollReq = Poll->Irp->AssociatedIrp.SystemBuffer;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    for( i = 0; i < PollReq->HandleCount; i++ ) {
	if( !AFD_HANDLES(PollReq)[i].Handle ) continue;

	FileObject = (PFILE_OBJECT)AFD_HANDLES(PollReq)[i].Handle;
	FCB = FileObject->FsContext;

	if( (FCB->PollState & AFD_EVENT_CLOSE) ||
	    (PollReq->Handles[i].Status & AFD_EVENT_CLOSE) ) {
	    AFD_HANDLES(PollReq)[i].Handle = 0;
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
    PAFD_FCB FCB;
    KIRQL OldIrql;
    PAFD_POLL_INFO PollReq;
    PKEVENT EventSelect = NULL;

    AFD_DbgPrint(MID_TRACE,("Called: DeviceExt %x FileObject %x\n", 
			    DeviceExt, FileObject));
    
    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    /* Take care of any event select signalling */
    FCB = (PAFD_FCB)FileObject->FsContext;

    /* Not sure if i can do this at DISPATCH_LEVEL ... try it at passive */
    AFD_DbgPrint(MID_TRACE,("Current State: %x, Events Fired: %x, "
			    "Select Triggers %x\n",
			    FCB->PollState, FCB->EventsFired, 
			    FCB->EventSelectTriggers));
    if( FCB->PollState & ~FCB->EventsFired & FCB->EventSelectTriggers ) {
	FCB->EventsFired |= FCB->PollState;
	EventSelect = FCB->EventSelect;
    }

    if( !FCB ) {
	KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
	return;
    }

    /* Now signal normal select irps */
    ThePollEnt = DeviceExt->Polls.Flink;

    while( ThePollEnt != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD( ThePollEnt, AFD_ACTIVE_POLL, ListEntry );
	PollReq = Poll->Irp->AssociatedIrp.SystemBuffer;
	AFD_DbgPrint(MID_TRACE,("Checking poll %x\n", Poll));

	if( UpdatePollWithFCB( Poll, FileObject ) ) {
	    ThePollEnt = ThePollEnt->Flink;
	    AFD_DbgPrint(MID_TRACE,("Signalling socket\n"));
	    SignalSocket( Poll, PollReq, STATUS_SUCCESS );
	} else 
	    ThePollEnt = ThePollEnt->Flink;
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Setting event %x\n", EventSelect));
    if( EventSelect ) KeSetEvent( EventSelect, IO_NETWORK_INCREMENT, FALSE );

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));
}
