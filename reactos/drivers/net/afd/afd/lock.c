/* $Id: lock.c,v 1.1.2.2 2004/07/15 03:21:47 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/lock.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

PAFD_WSABUF LockBuffers( PAFD_WSABUF Buf, UINT Count, BOOLEAN Write ) {
    UINT i;
    /* Copy the buffer array so we don't lose it */
    UINT Size = sizeof(AFD_WSABUF) * Count;
    PAFD_WSABUF NewBuf = ExAllocatePool( PagedPool, Size * 2 );

    if( NewBuf ) {
	PAFD_MAPBUF MapBuf = (PAFD_MAPBUF)(NewBuf + Count);
	RtlCopyMemory( NewBuf, Buf, Size );
	
	for( i = 0; i < Count; i++ ) {
	    AFD_DbgPrint(MID_TRACE,("Locking buffer %d (%x:%d)\n",
				    i, NewBuf[i].buf, NewBuf[i].len));

	    MapBuf[i].Mdl = IoAllocateMdl( NewBuf[i].buf, 
					   NewBuf[i].len,
					   FALSE,
					   FALSE,
					   NULL );
	    if( MapBuf[i].Mdl ) {
		MmProbeAndLockPages( MapBuf[i].Mdl, KernelMode, 
				     Write ? IoModifyAccess : IoReadAccess );
	    }
	}
    }

    return NewBuf;
}

VOID UnlockBuffers( PAFD_WSABUF Buf, UINT Count ) {
    PAFD_MAPBUF Map = (PAFD_MAPBUF)(Buf + Count);
    UINT i;

    for( i = 0; i < Count; i++ ) {
	if( Map[i].Mdl ) {
	    MmUnlockPages( Map[i].Mdl );
	    IoFreeMdl( Map[i].Mdl );
	}
    }

    ExFreePool( Buf );
}

/* Returns transitioned state or SOCKET_STATE_INVALID_TRANSITION */
UINT SocketAcquireStateLock( PAFD_FCB FCB ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID CurrentThread = KeGetCurrentThread();

    AFD_DbgPrint(MAX_TRACE,("Called on %x, attempting to lock\n", FCB));

    /* Wait for the previous user to unlock the FCB state.  There might be
     * multiple waiters waiting to change the state.  We need to check each
     * time we get the event whether somebody still has the state locked */

    if( !FCB ) return FALSE;

    if( CurrentThread == FCB->CurrentThread ) {
	FCB->LockCount++;
	AFD_DbgPrint(MID_TRACE,
		     ("Same thread, lock count %d\n", FCB->LockCount));
	return TRUE;
    } else {
	AFD_DbgPrint(MID_TRACE,
		     ("Thread %x opposes lock thread %x\n",
		      CurrentThread, FCB->CurrentThread));
    }

    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
	ExAcquireFastMutex( &FCB->Mutex );
	while( FCB->Locked ) {
	    AFD_DbgPrint
		(MID_TRACE,("FCB %x is locked, waiting for notification\n",
			    FCB));
	    ExReleaseFastMutex( &FCB->Mutex );
	    Status = KeWaitForSingleObject( &FCB->StateLockedEvent,
					    UserRequest,
					    KernelMode,
					    FALSE,
					    NULL );
	    ExAcquireFastMutex( &FCB->Mutex );
	    if( Status == STATUS_SUCCESS ) break;
	}
	FCB->Locked = TRUE;
	FCB->CurrentThread = CurrentThread;
	FCB->LockCount++;
	ExReleaseFastMutex( &FCB->Mutex );
    } else {
	KeAcquireSpinLock( &FCB->SpinLock, &FCB->OldIrql );
	FCB->Locked = TRUE;
	FCB->CurrentThread = CurrentThread;
	FCB->LockCount++;
    }
    AFD_DbgPrint(MAX_TRACE,("Got lock (%d).\n", FCB->LockCount));

    return TRUE;
}

VOID SocketStateUnlock( PAFD_FCB FCB ) {
    ASSERT(FCB->LockCount > 0);
    FCB->LockCount--;

    if( !FCB->LockCount ) {
	FCB->CurrentThread = NULL;
	if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
	    ExAcquireFastMutex( &FCB->Mutex );
	    FCB->Locked = FALSE;
	    ExReleaseFastMutex( &FCB->Mutex );
	} else {
	    FCB->Locked = FALSE;
	    KeReleaseSpinLock( &FCB->SpinLock, FCB->OldIrql );
	}

	AFD_DbgPrint(MAX_TRACE,("Unlocked.\n"));
	KePulseEvent( &FCB->StateLockedEvent, IO_NETWORK_INCREMENT, FALSE );
    } else {
	AFD_DbgPrint(MID_TRACE,("Lock count %d\n", FCB->LockCount));
    }
}

NTSTATUS DDKAPI UnlockAndMaybeComplete
( PAFD_FCB FCB, NTSTATUS Status, PIRP Irp,
  UINT Information, 
  PIO_COMPLETION_ROUTINE Completion ) {
    SocketStateUnlock( FCB );
    if( Status == STATUS_PENDING ) {
	IoMarkIrpPending( Irp );
    } else {
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	if( Completion ) 
	    Completion( FCB->DeviceExt->DeviceObject, Irp, FCB );
	else IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    }
    return Status;
}


NTSTATUS LostSocket( PIRP Irp ) {
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    AFD_DbgPrint(MIN_TRACE,("Called.\n"));
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;	
}

NTSTATUS LeaveIrpUntilLater( PAFD_FCB FCB, PIRP Irp, UINT Function ) {
    InsertTailList( &FCB->PendingIrpList[Function], 
		    &Irp->Tail.Overlay.ListEntry );
    return UnlockAndMaybeComplete( FCB, STATUS_PENDING, Irp, 0,
				   NULL );
}

