/* $Id: select.c,v 1.5 2004/11/15 18:24:57 arty Exp $
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
    
    for( i = 0; i < HandleCount; i++ ) {
	HandleArray[i].Events = 0;
	HandleArray[i].Status = 0;
    }
}

NTSTATUS STDCALL
ScanForImmediateTrigger( PAFD_HANDLE HandleArray,
			 UINT HandleCount,
			 PUINT HandlesSet ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PAFD_FCB FCB;
    UINT i;
    BOOLEAN ShouldReturnNow = FALSE;

    for( i = 0; i < HandleCount && NT_SUCCESS(Status); i++ ) {
        HandleArray[i].Status = 0;
	Status = 
	    ObReferenceObjectByHandle
	    ( (PVOID)HandleArray[i].Handle,
	      FILE_ALL_ACCESS,
	      NULL,
	      KernelMode,
	      (PVOID*)&FileObject,
	      NULL );

	if( NT_SUCCESS(Status) ) {
	    FCB = FileObject->FsContext;
	    /* Check select bits */
	    
	    AFD_DbgPrint(MID_TRACE,("Locking socket state\n"));

	    if( !SocketAcquireStateLock( FCB ) ) {
		AFD_DbgPrint(MID_TRACE,("Failed to get a socket state\n"));
		Status = STATUS_UNSUCCESSFUL;
	    } else {
		AFD_DbgPrint(MID_TRACE,("Got a socket state\n"));
		Status = STATUS_SUCCESS;
		HandleArray[i].Status = 
		    FCB->PollState & HandleArray[i].Events;
		if( HandleArray[i].Status ) ShouldReturnNow = TRUE;
		ObDereferenceObject( (PVOID)HandleArray[i].Handle );
		AFD_DbgPrint(MID_TRACE,("Unlocking\n"));
		SocketStateUnlock( FCB );
		AFD_DbgPrint(MID_TRACE,("Unlocked\n"));
	    }
	}
    }

    if( !NT_SUCCESS(Status) || ShouldReturnNow ) return Status; 
    else return STATUS_PENDING;
}

VOID SelectTimeout( PKDPC Dpc,
		    PVOID DeferredContext,
		    PVOID SystemArgument1,
		    PVOID SystemArgument2 ) {
    PAFD_ACTIVE_POLL Poll = DeferredContext;
    PAFD_POLL_INFO PollReq;
    PAFD_DEVICE_EXTENSION DeviceExt;
    PIRP Irp;
    KIRQL OldIrql;

    Irp = Poll->Irp;
    DeviceExt = Poll->DeviceExt;

    PollReq = Irp->AssociatedIrp.SystemBuffer;

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );
    RemoveEntryList( &Poll->ListEntry );
    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    ExFreePool( Poll );

    ZeroEvents( PollReq->Handles, PollReq->HandleCount );

    Irp->IoStatus.Status = STATUS_TIMEOUT;
    Irp->IoStatus.Information = -1;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
}

NTSTATUS STDCALL
AfdSelect( PDEVICE_OBJECT DeviceObject, PIRP Irp, 
	   PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_NO_MEMORY;
    PAFD_POLL_INFO PollReq = Irp->AssociatedIrp.SystemBuffer;
    PAFD_DEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
    PAFD_ACTIVE_POLL Poll = NULL;
    UINT CopySize = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    UINT AllocSize = 
	CopySize + sizeof(AFD_ACTIVE_POLL) - sizeof(AFD_POLL_INFO);
    KIRQL OldIrql;
    UINT HandlesSignalled; 

    AFD_DbgPrint(MID_TRACE,("Called (HandleCount %d Timeout %d)\n", 
			    PollReq->HandleCount,
			    (INT)(PollReq->Timeout.QuadPart * -1)));

    Status = ScanForImmediateTrigger( PollReq->Handles,
				      PollReq->HandleCount,
				      &HandlesSignalled );
    
    if( Status == STATUS_PENDING ) {
	Poll = ExAllocatePool( NonPagedPool, AllocSize );
	
	if( Poll ) {
	    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );
	    
	    KeInitializeDpc( (PRKDPC)&Poll->TimeoutDpc, 
			     (PKDEFERRED_ROUTINE)SelectTimeout,
			     Poll );
	    PollReq->Timeout.QuadPart *= -1;
	    /* Negative values are relative */
	    KeInitializeTimerEx( &Poll->Timer, NotificationTimer );
	    KeSetTimer( &Poll->Timer, PollReq->Timeout, &Poll->TimeoutDpc );
	    
	    Poll->Irp = Irp;
	    Poll->DeviceExt = DeviceExt;

	    InsertTailList( &DeviceExt->Polls, &Poll->ListEntry );
	    Status = STATUS_PENDING;

	    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );
	} else Status = STATUS_NO_MEMORY;
    } else if( Status == STATUS_SUCCESS ) {
	CopyBackStatus( PollReq->Handles,
			PollReq->HandleCount );
    } else {
	ZeroEvents( PollReq->Handles,
		    PollReq->HandleCount );
    }

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    if( Status == STATUS_PENDING )
	IoMarkIrpPending( Irp );
    else {
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = HandlesSignalled;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    }

    return Status;
}

VOID SignalSocket( PAFD_ACTIVE_POLL Poll, PAFD_POLL_INFO PollReq, UINT i ) {
    /* One of the files was destroyed.  We return now with error. */
    Poll->Irp->IoStatus.Status = STATUS_SUCCESS; /* XXX REVISIT */
    Poll->Irp->IoStatus.Information = 1;
    CopyBackStatus( PollReq->Handles,
		    PollReq->HandleCount );
    IoCompleteRequest( Poll->Irp, IO_NETWORK_INCREMENT );
}

BOOLEAN UpdatePollWithFCB( PAFD_ACTIVE_POLL Poll, PFILE_OBJECT FileObject ) {
    UINT i;
    NTSTATUS Status;
    PFILE_OBJECT TargetFile;
    PAFD_FCB FCB;
    PAFD_POLL_INFO PollReq = Poll->Irp->AssociatedIrp.SystemBuffer;

    for( i = 0; i < PollReq->HandleCount; i++ ) {
	Status = 
	    ObReferenceObjectByHandle
	    ( (PVOID)PollReq->Handles[i].Handle,
	      FILE_ALL_ACCESS,
	      NULL,
	      KernelMode,
	      (PVOID*)&TargetFile,
	      NULL );
	
	if( !NT_SUCCESS(Status) ) { 
	    PollReq->Handles[i].Status = AFD_EVENT_CLOSE;
	    SignalSocket( Poll, PollReq, i );
	    return TRUE;
	} else {
	    FCB = FileObject->FsContext;

	    AFD_DbgPrint(MID_TRACE,("Locking socket state\n"));

	    if( !SocketAcquireStateLock( FCB ) ) {
		PollReq->Handles[i].Status = AFD_EVENT_CLOSE;
		SignalSocket( Poll, PollReq, i );
	    } else {
		PollReq->Handles[i].Status = 
		    PollReq->Handles[i].Events & FCB->PollState;
		if( PollReq->Handles[i].Status )
		    SignalSocket( Poll, PollReq, i );
		SocketStateUnlock( FCB );
	    }
	    return TRUE;
	}
    }

    return FALSE;
}

VOID PollReeval( PAFD_DEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject ) {
    PAFD_ACTIVE_POLL Poll = NULL;
    PLIST_ENTRY ThePollEnt = NULL;
    KIRQL OldIrql;

    AFD_DbgPrint(MID_TRACE,("Called: DeviceExt %x FileObject %x\n", 
			    DeviceExt, FileObject));

    KeAcquireSpinLock( &DeviceExt->Lock, &OldIrql );

    ThePollEnt = DeviceExt->Polls.Flink;

    while( ThePollEnt != &DeviceExt->Polls ) {
	Poll = CONTAINING_RECORD( ThePollEnt, AFD_ACTIVE_POLL, ListEntry );
	if( UpdatePollWithFCB( Poll, FileObject ) ) {
	    ThePollEnt = ThePollEnt->Flink;
	    RemoveEntryList( &Poll->ListEntry );
	} else 
	    ThePollEnt = ThePollEnt->Flink;
    }

    KeReleaseSpinLock( &DeviceExt->Lock, OldIrql );

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));
}
