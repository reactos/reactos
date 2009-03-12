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
#include "pseh/pseh.h"

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

    _SEH_TRY {
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
    } _SEH_HANDLE {
	AFD_DbgPrint(MID_TRACE,("Exception executing GetInfo\n"));
	Status = STATUS_INVALID_PARAMETER;
    } _SEH_END;

    AFD_DbgPrint(MID_TRACE,("Returning %x\n", Status));

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}

NTSTATUS NTAPI
AfdGetSockName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PMDL Mdl = NULL;

    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if( FCB->AddressFile.Object == NULL && FCB->Connection.Object == NULL ) {
	 return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER, Irp, 0,
	                                NULL );
    }

    Mdl = IoAllocateMdl
	( Irp->UserBuffer,
	  IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
	  FALSE,
	  FALSE,
	  NULL );

    if( Mdl != NULL ) {
	_SEH_TRY {
	    MmProbeAndLockPages( Mdl, Irp->RequestorMode, IoModifyAccess );
	} _SEH_HANDLE {
	    AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	    Status = _SEH_GetExceptionCode();
	} _SEH_END;

	if( NT_SUCCESS(Status) ) {
                Status = TdiQueryInformation
                    ( FCB->Connection.Object ? FCB->Connection.Object : FCB->AddressFile.Object,
                      TDI_QUERY_ADDRESS_INFO,
                      Mdl );
        }
    } else
        Status = STATUS_INSUFFICIENT_RESOURCES;

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}

NTSTATUS NTAPI
AfdGetPeerName( PDEVICE_OBJECT DeviceObject, PIRP Irp,
                      PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PAFD_FCB FCB = FileObject->FsContext;
    PMDL Mdl = NULL;
    PTDI_CONNECTION_INFORMATION ConnInfo = NULL;


    if( !SocketAcquireStateLock( FCB ) ) return LostSocket( Irp );

    if (FCB->RemoteAddress == NULL) {
        return UnlockAndMaybeComplete( FCB, STATUS_INVALID_PARAMETER, Irp, 0, NULL );
    }

    if(NT_SUCCESS(Status = TdiBuildNullConnectionInfo
                      (&ConnInfo,
                       FCB->RemoteAddress->Address[0].AddressType)))
    {
        Mdl = IoAllocateMdl(ConnInfo, 
                            sizeof(TDI_CONNECTION_INFORMATION) + 
                                   TaLengthOfTransportAddress(ConnInfo->RemoteAddress),
                            FALSE,
                            FALSE,
                            NULL);

        if (Mdl)
        {
            _SEH_TRY {
               MmProbeAndLockPages(Mdl, Irp->RequestorMode, IoModifyAccess);
            } _SEH_HANDLE {
	       AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	       Status = _SEH_GetExceptionCode();
	    } _SEH_END;

            if (NT_SUCCESS(Status))
            {
                Status = TdiQueryInformation(FCB->Connection.Object,
                          TDI_QUERY_CONNECTION_INFO,
                          Mdl);

                if (NT_SUCCESS(Status))
                {
                    RtlCopyMemory(Irp->UserBuffer, ConnInfo->RemoteAddress, TaLengthOfTransportAddress
                                                                                  (ConnInfo->RemoteAddress));
                }
            }
         }

         ExFreePool(ConnInfo);
    }

    return UnlockAndMaybeComplete( FCB, Status, Irp, 0, NULL );
}
