/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/dispatch.h
 * PURPOSE:     TDI dispatch routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 * TODO:        Validate device object in all dispatch routines
 */
#include <roscfg.h>
#include <tcpip.h>
#include <dispatch.h>
#include <routines.h>
#include <datagram.h>
#include <info.h>

NTSTATUS IRPFinish( PIRP Irp, NTSTATUS Status ) {
    IoSetCancelRoutine( Irp, NULL );

    if( Status == STATUS_PENDING )
	IoMarkIrpPending( Irp );
    else {
	Irp->IoStatus.Status = Status;
	IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    }

    return Status;
}

