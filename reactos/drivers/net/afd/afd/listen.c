/* $Id: listen.c,v 1.2 2004/07/18 22:49:17 arty Exp $
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/listen.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include "afd.h"
#include "tdi_proto.h"
#include "tdiconn.h"
#include "debug.h"

NTSTATUS DDKAPI ListenComplete
( PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context ) {
    PAFD_FCB FCB = (PAFD_FCB)Context;
    AFD_DbgPrint(MID_TRACE,("Completing listen request.\n"));
    AFD_DbgPrint(MID_TRACE,("IoStatus was %x\n", FCB->ListenIrp.Iosb.Status));
    AFD_DbgPrint(MID_TRACE,("Doing nothing as yet.\n"));
    return STATUS_SUCCESS;
}

NTSTATUS AfdListenSocket(PDEVICE_OBJECT DeviceObject, PIRP Irp,
			 PIO_STACK_LOCATION IrpSp) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PAFD_LISTEN_DATA ListenReq;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );
    if( !(ListenReq = LockRequest( Irp, IrpSp )) ) 
	return UnlockAndMaybeComplete( FCB, STATUS_NO_MEMORY, Irp, 
				       0, NULL, FALSE );
    
    if( FCB->State != SOCKET_STATE_BOUND ) {
	Status = STATUS_UNSUCCESSFUL;
	AFD_DbgPrint(MID_TRACE,("Could not listen an unbound socket\n"));
	return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
    }
    
    FCB->DelayedAccept = ListenReq->UseDelayedAcceptance;

    Status = WarmSocketForConnection( FCB );

    FCB->State = SOCKET_STATE_LISTENING;

    Status = TdiListen( &FCB->ListenIrp.InFlightRequest,
			FCB->Connection.Object, 
			&FCB->ListenIrp.ConnectionInfo,
			&FCB->ListenIrp.Iosb,
			ListenComplete,
			FCB );

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));
    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL, TRUE );
}
