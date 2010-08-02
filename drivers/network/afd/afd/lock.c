/* $Id$
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
#include "pseh/pseh2.h"

/* Lock a method_neither request so it'll be available from DISPATCH_LEVEL */
PVOID LockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp ) {
    BOOLEAN LockFailed = FALSE;

    ASSERT(IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
    ASSERT(IrpSp->Parameters.DeviceIoControl.InputBufferLength);
    ASSERT(!Irp->MdlAddress);

    Irp->MdlAddress =
	IoAllocateMdl( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
		       IrpSp->Parameters.DeviceIoControl.InputBufferLength,
		       FALSE,
		       FALSE,
		       NULL );
    if( Irp->MdlAddress ) {
	_SEH2_TRY {
	    MmProbeAndLockPages( Irp->MdlAddress, Irp->RequestorMode, IoModifyAccess );
	} _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
	    LockFailed = TRUE;
	} _SEH2_END;

	if( LockFailed ) {
	    IoFreeMdl( Irp->MdlAddress );
	    Irp->MdlAddress = NULL;
	    return NULL;
	}

	IrpSp->Parameters.DeviceIoControl.Type3InputBuffer =
	    MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

	if( !IrpSp->Parameters.DeviceIoControl.Type3InputBuffer ) {
            MmUnlockPages( Irp->MdlAddress );
	    IoFreeMdl( Irp->MdlAddress );
	    Irp->MdlAddress = NULL;
	    return NULL;
	}

	return IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    } else return NULL;
}

VOID UnlockRequest( PIRP Irp, PIO_STACK_LOCATION IrpSp )
{
    MmUnlockPages( Irp->MdlAddress );
    IoFreeMdl( Irp->MdlAddress );
    Irp->MdlAddress = NULL;
}

/* Note: We add an extra buffer if LockAddress is true.  This allows us to
 * treat the address buffer as an ordinary client buffer.  It's only used
 * for datagrams. */

PAFD_WSABUF LockBuffers( PAFD_WSABUF Buf, UINT Count,
			 PVOID AddressBuf, PINT AddressLen,
			 BOOLEAN Write, BOOLEAN LockAddress ) {
    UINT i;
    /* Copy the buffer array so we don't lose it */
    UINT Lock = LockAddress ? 2 : 0;
    UINT Size = sizeof(AFD_WSABUF) * (Count + Lock);
    PAFD_WSABUF NewBuf = ExAllocatePool( PagedPool, Size * 2 );
    BOOLEAN LockFailed = FALSE;
    PAFD_MAPBUF MapBuf;

    AFD_DbgPrint(MID_TRACE,("Called(%08x)\n", NewBuf));

    if( NewBuf ) {
        RtlZeroMemory(NewBuf, Size * 2);

	MapBuf = (PAFD_MAPBUF)(NewBuf + Count + Lock);

        _SEH2_TRY {
            RtlCopyMemory( NewBuf, Buf, sizeof(AFD_WSABUF) * Count );
            if( LockAddress ) {
                if (AddressBuf && AddressLen) {
                    NewBuf[Count].buf = AddressBuf;
                    NewBuf[Count].len = *AddressLen;
                    NewBuf[Count + 1].buf = (PVOID)AddressLen;
                    NewBuf[Count + 1].len = sizeof(*AddressLen);
                }
                Count += 2;
            }
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            AFD_DbgPrint(MIN_TRACE,("Access violation copying buffer info "
                                    "from userland (%x %x)\n",
                                    Buf, AddressLen));
            ExFreePool( NewBuf );
            _SEH2_YIELD(return NULL);
        } _SEH2_END;

	for( i = 0; i < Count; i++ ) {
	    AFD_DbgPrint(MID_TRACE,("Locking buffer %d (%x:%d)\n",
				    i, NewBuf[i].buf, NewBuf[i].len));

	    if( NewBuf[i].buf && NewBuf[i].len ) {
		MapBuf[i].Mdl = IoAllocateMdl( NewBuf[i].buf,
					       NewBuf[i].len,
					       FALSE,
					       FALSE,
					       NULL );
	    } else {
		MapBuf[i].Mdl = NULL;
		continue;
	    }

	    AFD_DbgPrint(MID_TRACE,("NewMdl @ %x\n", MapBuf[i].Mdl));

	    if( MapBuf[i].Mdl ) {
		AFD_DbgPrint(MID_TRACE,("Probe and lock pages\n"));
		_SEH2_TRY {
		    MmProbeAndLockPages( MapBuf[i].Mdl, KernelMode,
				         Write ? IoModifyAccess : IoReadAccess );
		} _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		    LockFailed = TRUE;
		} _SEH2_END;
		AFD_DbgPrint(MID_TRACE,("MmProbeAndLock finished\n"));

		if( LockFailed ) {
		    IoFreeMdl( MapBuf[i].Mdl );
		    MapBuf[i].Mdl = NULL;
		    ExFreePool( NewBuf );
		    return NULL;
		}
	    } else {
		ExFreePool( NewBuf );
		return NULL;
	    }
	}
    }

    AFD_DbgPrint(MID_TRACE,("Leaving %x\n", NewBuf));

    return NewBuf;
}

VOID UnlockBuffers( PAFD_WSABUF Buf, UINT Count, BOOL Address ) {
    UINT Lock = Address ? 2 : 0;
    PAFD_MAPBUF Map = (PAFD_MAPBUF)(Buf + Count + Lock);
    UINT i;

    if( !Buf ) return;

    for( i = 0; i < Count + Lock; i++ ) {
	if( Map[i].Mdl ) {
	    MmUnlockPages( Map[i].Mdl );
	    IoFreeMdl( Map[i].Mdl );
	    Map[i].Mdl = NULL;
	}
    }

    ExFreePool( Buf );
    Buf = NULL;
}

/* Produce a kernel-land handle array with handles replaced by object
 * pointers.  This will allow the system to do proper alerting */
PAFD_HANDLE LockHandles( PAFD_HANDLE HandleArray, UINT HandleCount ) {
    UINT i;
    NTSTATUS Status = STATUS_SUCCESS;

    PAFD_HANDLE FileObjects = ExAllocatePool
	( NonPagedPool, HandleCount * sizeof(AFD_HANDLE) );

    for( i = 0; FileObjects && i < HandleCount; i++ ) {
	FileObjects[i].Status = 0;
	FileObjects[i].Events = HandleArray[i].Events;
        FileObjects[i].Handle = 0;
	if( !HandleArray[i].Handle ) continue;
	if( NT_SUCCESS(Status) ) {
		Status = ObReferenceObjectByHandle
	    	( (PVOID)HandleArray[i].Handle,
	     	 FILE_ALL_ACCESS,
	     	 NULL,
	      	 KernelMode,
	      	 (PVOID*)&FileObjects[i].Handle,
	      	 NULL );
	}

        if( !NT_SUCCESS(Status) )
            FileObjects[i].Handle = 0;
    }

    if( !NT_SUCCESS(Status) ) {
	UnlockHandles( FileObjects, HandleCount );
	return NULL;
    }

    return FileObjects;
}

VOID UnlockHandles( PAFD_HANDLE HandleArray, UINT HandleCount ) {
    UINT i;

    for( i = 0; i < HandleCount; i++ ) {
	if( HandleArray[i].Handle )
	    ObDereferenceObject( (PVOID)HandleArray[i].Handle );
    }

    ExFreePool( HandleArray );
    HandleArray = NULL;
}

BOOLEAN SocketAcquireStateLock( PAFD_FCB FCB ) {
    if( !FCB ) return FALSE;

    return !KeWaitForMutexObject(&FCB->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 NULL);
}

VOID SocketStateUnlock( PAFD_FCB FCB ) {
    KeReleaseMutex(&FCB->Mutex, FALSE);
}

NTSTATUS NTAPI UnlockAndMaybeComplete
( PAFD_FCB FCB, NTSTATUS Status, PIRP Irp,
  UINT Information ) {
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    if ( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );
    (void)IoSetCancelRoutine(Irp, NULL);
    SocketStateUnlock( FCB );
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );
    return Status;
}


NTSTATUS LostSocket( PIRP Irp ) {
    NTSTATUS Status = STATUS_FILE_CLOSED;
    AFD_DbgPrint(MIN_TRACE,("Called.\n"));
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    if ( Irp->MdlAddress ) UnlockRequest( Irp, IoGetCurrentIrpStackLocation( Irp ) );
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}

NTSTATUS LeaveIrpUntilLater( PAFD_FCB FCB, PIRP Irp, UINT Function ) {
    InsertTailList( &FCB->PendingIrpList[Function],
		    &Irp->Tail.Overlay.ListEntry );
    IoMarkIrpPending(Irp);
    (void)IoSetCancelRoutine(Irp, AfdCancelHandler);
    SocketStateUnlock( FCB );
    return STATUS_PENDING;
}
