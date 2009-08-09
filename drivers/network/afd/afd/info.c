/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/info.c
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

NTSTATUS NTAPI
AfdGetInfo( PDEVICE_OBJECT DeviceObject, PIRP Irp,
	    PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PAFD_INFO InfoReq = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;

    AFD_DbgPrint(MID_TRACE,("Called %x %x\n", InfoReq,
			    InfoReq ? InfoReq->InformationClass : 0));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    _SEH2_TRY {
	switch( InfoReq->InformationClass ) {
	case AFD_INFO_RECEIVE_WINDOW_SIZE:
	    InfoReq->Information.Ulong = FCB->Recv.Size;
	    break;

	case AFD_INFO_SEND_WINDOW_SIZE:
	    InfoReq->Information.Ulong = FCB->Send.Size;
	    AFD_DbgPrint(MID_TRACE,("Send window size %d\n", FCB->Send.Size));
	    break;

	case AFD_INFO_GROUP_ID_TYPE:
	    InfoReq->Information.Ulong = 0; /* What is group id */
	    break;

	case AFD_INFO_BLOCKING_MODE:
	    InfoReq->Information.Ulong = 0;
	    break;

    case AFD_INFO_RECEIVE_CONTENT_SIZE:
        /* Only touch InfoReq if a socket has been set up.
           Behaviour was verified under WinXP SP2. */
        if(FCB->AddressFile.Object)
            InfoReq->Information.Ulong = FCB->Recv.Content - FCB->Recv.BytesUsed;

        break;

	default:
	    AFD_DbgPrint(MID_TRACE,("Unknown info id %x\n",
				    InfoReq->InformationClass));
	    Status = STATUS_INVALID_PARAMETER;
	    break;
	}
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
	AFD_DbgPrint(MID_TRACE,("Exception executing GetInfo\n"));
	Status = STATUS_INVALID_PARAMETER;
    } _SEH2_END;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}

NTSTATUS NTAPI
AfdGetSockOrPeerName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp, BOOLEAN Local ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PMDL Mdl = NULL, SysMdl = NULL;
    PTDI_CONNECTION_INFORMATION ConnInfo = NULL;
    PTRANSPORT_ADDRESS TransAddr = NULL;
    HANDLE ProcHandle = NULL;
    BOOLEAN UnlockSysMdl = FALSE;
    PVOID UserSpace = NULL;
    ULONG Length, InOutLength;

    AFD_DbgPrint(MID_TRACE,("Called on %x\n", FCB));

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    Mdl = IoAllocateMdl
	( Irp->UserBuffer,
	  IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
	  FALSE,
	  FALSE,
	  NULL );

    if( Mdl != NULL ) {
	_SEH2_TRY {
	    MmProbeAndLockPages( Mdl, Irp->RequestorMode, IoModifyAccess );
	} _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
	    AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	    Status = _SEH2_GetExceptionCode();
	} _SEH2_END;

	if( NT_SUCCESS(Status) ) {
            if( Local ) {
                if( FCB->AddressFile.Object == NULL ) {
	            return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER, Irp, 0,
	                                           NULL );
                }

                Status = TdiQueryInformation
                    ( FCB->Connection.Object ? 
					  FCB->Connection.Object : FCB->AddressFile.Object,
                      TDI_QUERY_ADDRESS_INFO,
                      Mdl );
            } else {
                // what follows is fucked up shit.
                // i'm not sure how to avoid it
                // sorry
                // -- arty

                if( FCB->Connection.Object == NULL || (FCB->State != SOCKET_STATE_BOUND && FCB->State != SOCKET_STATE_CONNECTED) ) {
	            return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER, Irp, 0,
	                                           NULL );
                }

                if( NT_SUCCESS
                    ( Status = TdiBuildNullConnectionInfo
                      ( &ConnInfo,
                        FCB->RemoteAddress->Address[0].AddressType ) ) ) {

		    Length = TaLengthOfTransportAddress
			(ConnInfo->RemoteAddress);

		    if (NT_SUCCESS(Status))
			Status = ObOpenObjectByPointer
			    (PsGetCurrentProcess(),
			     0,
			     NULL,
			     PROCESS_ALL_ACCESS,
			     PsProcessType,
			     KernelMode,
			     &ProcHandle);

		    if (NT_SUCCESS(Status))
		    {
			InOutLength =
			    PAGE_ROUND_UP(sizeof(TDI_CONNECTION_INFO));

			Status = NtAllocateVirtualMemory
			    (ProcHandle,
			     (PVOID*)&UserSpace,
			     PAGE_SHIFT,
			     &InOutLength,
			     MEM_COMMIT,
			     PAGE_READWRITE);
		    }

		    if (NT_SUCCESS(Status))
		    {
			ExFreePool(ConnInfo);
			ConnInfo = (PTDI_CONNECTION_INFORMATION)UserSpace;

			SysMdl = IoAllocateMdl
			    ( UserSpace, Length, FALSE, FALSE, NULL );
		    }
		    else
		    {
			ExFreePool(ConnInfo);
			ConnInfo = NULL;
		    }
		}

                if( SysMdl ) {
                    _SEH2_TRY {
                        MmProbeAndLockPages( SysMdl, Irp->RequestorMode, IoModifyAccess );
			UnlockSysMdl = TRUE;
                    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
	                AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	                Status = _SEH2_GetExceptionCode();
                    } _SEH2_END;
                } else Status = STATUS_NO_MEMORY;

                if( NT_SUCCESS(Status) ) {
                    Status = TdiQueryInformation
                        ( FCB->Connection.Object,
                          TDI_QUERY_CONNECTION_INFO,
                          SysMdl );
                }

                if( NT_SUCCESS(Status) ) {
                    TransAddr =
                        (PTRANSPORT_ADDRESS)MmGetSystemAddressForMdlSafe( Mdl, NormalPagePriority );

                    if( TransAddr )
                        RtlCopyMemory( TransAddr, ConnInfo->RemoteAddress,
                                       TaLengthOfTransportAddress
                                       ( ConnInfo->RemoteAddress ) );
                    else Status = STATUS_INSUFFICIENT_RESOURCES;
		}

		if (UnlockSysMdl)
		    MmUnlockPages( SysMdl );

                if( SysMdl ) IoFreeMdl( SysMdl );
                if( ConnInfo )
		    NtFreeVirtualMemory
			( ProcHandle,
			  (PVOID)ConnInfo,
			  &InOutLength,
			  MEM_RELEASE );
		if( ProcHandle ) NtClose(ProcHandle);
                if( TransAddr ) MmUnmapLockedPages( TransAddr, Mdl );
                MmUnlockPages( Mdl );
                IoFreeMdl( Mdl );
            }
	}
    } else {
    	Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}
