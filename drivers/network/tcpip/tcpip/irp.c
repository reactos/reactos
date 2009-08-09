/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/dispatch.h
 * PURPOSE:     TDI dispatch routines
 * PROGRAMMERS: arty
 * REVISIONS:
 *   CSH 01/08-2000 Created
 * TODO:        Validate device object in all dispatch routines
 */

#include "precomp.h"

VOID IRPRemember( PIRP Irp, PCHAR File, UINT Line ) {
    TrackWithTag( IRP_TAG, Irp, File, Line );
}

NTSTATUS IRPFinish( PIRP Irp, NTSTATUS Status ) {
    //DbgPrint("Called: Irp %x, Status %x Event %x\n", Irp, Status, Irp->UserEvent);

    UntrackFL( __FILE__, __LINE__, Irp, IRP_TAG );

    Irp->IoStatus.Status = Status;

    if( Status == STATUS_PENDING )
	IoMarkIrpPending( Irp );
    else {
	(void)IoSetCancelRoutine( Irp, NULL );
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    }

    return Status;
}

