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

static VOID PrintEvents( ULONG Events ) {
#if DBG
    char *events_list[] = { "AFD_EVENT_RECEIVE",
                            "AFD_EVENT_OOB_RECEIVE",
                            "AFD_EVENT_SEND",
                            "AFD_EVENT_DISCONNECT",
                            "AFD_EVENT_ABORT",
                            "AFD_EVENT_CLOSE",
                            "AFD_EVENT_CONNECT",
                            "AFD_EVENT_ACCEPT",
                            "AFD_EVENT_CONNECT_FAIL",
                            "AFD_EVENT_QOS",
                            "AFD_EVENT_GROUP_QOS",
                            NULL };
    int i;

    for( i = 0; events_list[i]; i++ )
        if( Events & (1 << i) ) AFD_DbgPrint(MID_TRACE,("%s ", events_list[i] ));
#endif
}

static VOID CopyBackStatus( PAFD_HANDLE HandleArray,
		     UINT HandleCount ) {
    UINT i;

    for( i = 0; i < HandleCount; i++ ) {
	HandleArray[i].Events = HandleArray[i].Status;
	HandleArray[i].Status = 0;
    }
}

static VOID ZeroEvents( PAFD_HANDLE HandleArray,
		 UINT HandleCount ) {
    UINT i;

    for( i = 0; i < HandleCount; i++ )
	HandleArray[i].Status = 0;
}


/* you must pass either Poll OR Irp */
static VOID SignalSocket(
   PAFD_ACTIVE_POLL Poll OPTIONAL,
   PIRP _Irp OPTIONAL,
   PAFD_POLL_INFO PollReq,
	NTSTATUS Status
   )
{
    UINT i;
    PIRP Irp = _Irp ? _Irp : Poll->Irp;
    AFD_DbgPrint(MID_TRACE,("Called (Status %x)\n", Status));

    if (Poll)
    {
       KeCancelTimer( &Poll->Timer );
      RemoveEntryList( &Poll->ListEntry );
      ExFreePool( Poll );
   }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information =
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
    if( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );
    AFD_DbgPrint(MID_TRACE,("Completing\n"));
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    AFD_DbgPrint(MID_TRACE,("Done\n"));
}

static VOID SelectTimeout( PKDPC Dpc,
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
    SignalSocket( Poll, NULL, PollReq, STATUS_TIMEOUT );
    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Timeout\n"));
}

VOID KillSelectsForFCB( PAFD_DEVICE_EXTENSION DeviceExt,
                        PFILE_OBJECT FileObject,
                        BOOLEAN OnlyExclusive ) {
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;
    PAFD_ACTIVE_POLL Poll;
    PIRP Irp;
    PAFD_POLL_INFO PollReq;
    PAFD_HANDLE HandleArray;
    UINT i;

    AFD_DbgPrint(MID_TRACE,("Killing selects that refer to %x\n", FileObject));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    ListEntry = DeviceExt->Polls.Flink;
    while ( ListEntry != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD(ListEntry, AFD_ACTIVE_POLL, ListEntry);
	ListEntry = ListEntry->Flink;
        Irp = Poll->Irp;
        PollReq = Irp->AssociatedIrp.SystemBuffer;
        HandleArray = AFD_HANDLES(PollReq);

        for( i = 0; i < PollReq->HandleCount; i++ ) {
            AFD_DbgPrint(MAX_TRACE,("Req: %x, This %x\n",
                                    HandleArray[i].Handle, FileObject));
            if( (PVOID)HandleArray[i].Handle == FileObject &&
                (!OnlyExclusive || (OnlyExclusive && Poll->Exclusive)) ) {
                ZeroEvents( PollReq->Handles, PollReq->HandleCount );
                SignalSocket( Poll, NULL, PollReq, STATUS_SUCCESS );
            }
	}
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Done\n"));
}

NTSTATUS NTAPI
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	   PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_NO_MEMORY;
    PAFD_FCB FCB;
    PFILE_OBJECT FileObject;
    PAFD_POLL_INFO PollReq = Irp->AssociatedIrp.SystemBuffer;
    PAFD_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
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

    if( !AFD_HANDLES(PollReq) ) {
	Irp->IoStatus.Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
	return STATUS_NO_MEMORY;
    }

    if( Exclusive ) {
        for( i = 0; i < PollReq->HandleCount; i++ ) {
            if( !AFD_HANDLES(PollReq)[i].Handle ) continue;

            KillSelectsForFCB( DeviceExt,
                               (PFILE_OBJECT)AFD_HANDLES(PollReq)[i].Handle,
                               TRUE );
        }
    }

    ZeroEvents( PollReq->Handles,
		PollReq->HandleCount );

	KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

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
                AFD_DbgPrint(MID_TRACE, ("AFD: Select Events: "));
                PrintEvents( PollReq->Handles[i].Events );
                AFD_DbgPrint(MID_TRACE,("\n"));

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
	    SignalSocket( NULL, Irp, PollReq, Status );
	} else {

       PAFD_ACTIVE_POLL Poll = NULL;

       Poll = ExAllocatePool( NonPagedPool, AllocSize );

       if (Poll){
          Poll->Irp = Irp;
          Poll->DeviceExt = DeviceExt;
          Poll->Exclusive = Exclusive;

          KeInitializeTimerEx( &Poll->Timer, NotificationTimer );

          KeInitializeDpc( (PRKDPC)&Poll->TimeoutDpc,
             (PKDEFERRED_ROUTINE)SelectTimeout,
             Poll );

          InsertTailList( &DeviceExt->Polls, &Poll->ListEntry );

          KeSetTimer( &Poll->Timer, PollReq->Timeout, &Poll->TimeoutDpc );

          Status = STATUS_PENDING;
          IoMarkIrpPending( Irp );
       } else {
          AFD_DbgPrint(MAX_TRACE, ("FIXME: do something with the IRP!\n"));
          Status = STATUS_NO_MEMORY;
       }
	}

	KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return Status;
}

NTSTATUS NTAPI
AfdEventSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp,
		PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    NTSTATUS Status = STATUS_NO_MEMORY;
    PAFD_EVENT_SELECT_INFO EventSelectInfo =
	(PAFD_EVENT_SELECT_INFO)LockRequest( Irp, IrpSp );
    PAFD_FCB FCB = FileObject->FsContext;

    if( !SocketAcquireStateLock( FCB ) ) {
	return LostSocket( Irp );
    }

    if ( !EventSelectInfo ) {
         return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
				   0 );
    }
    AFD_DbgPrint(MID_TRACE,("Called (Event %x Triggers %x)\n",
			    EventSelectInfo->EventObject,
			    EventSelectInfo->Events));

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

    return UnlockAndMaybeComplete( FCB, Status, Irp,
				   0 );
}

NTSTATUS NTAPI
AfdEnumEvents( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	       PIO_STACK_LOCATION IrpSp ) {
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_ENUM_NETWORK_EVENTS_INFO EnumReq =
	(PAFD_ENUM_NETWORK_EVENTS_INFO)LockRequest( Irp, IrpSp );
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,("Called (FCB %x)\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) {
	return LostSocket( Irp );
    }

    if ( !EnumReq ) {
         return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp,
				   0 );
    }

    EnumReq->PollEvents = FCB->PollState;
    RtlZeroMemory( EnumReq->EventStatus, sizeof(EnumReq->EventStatus) );

    return UnlockAndMaybeComplete( FCB, STATUS_SUCCESS, Irp,
				   0 );
}

/* * * NOTE ALWAYS CALLED AT DISPATCH_LEVEL * * */
static BOOLEAN UpdatePollWithFCB( PAFD_ACTIVE_POLL Poll, PFILE_OBJECT FileObject ) {
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

    if( !FCB ) {
	KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
	return;
    }

    /* Not sure if i can do this at DISPATCH_LEVEL ... try it at passive */
    AFD_DbgPrint(MID_TRACE,("Current State: %x, Events Fired: %x, "
			    "Select Triggers %x\n",
			    FCB->PollState, FCB->EventsFired,
			    FCB->EventSelectTriggers));
    if( FCB->PollState & ~FCB->EventsFired & FCB->EventSelectTriggers ) {
	FCB->EventsFired |= FCB->PollState;
	EventSelect = FCB->EventSelect;
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
	    SignalSocket( Poll, NULL, PollReq, STATUS_SUCCESS );
	} else
	    ThePollEnt = ThePollEnt->Flink;
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Setting event %x\n", EventSelect));
    if( EventSelect ) KeSetEvent( EventSelect, IO_NETWORK_INCREMENT, FALSE );

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));
}
