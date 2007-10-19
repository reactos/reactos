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

#include "precomp.h"
#include <pseh/pseh.h>

NTSTATUS DispPrepareIrpForCancel(
    PTRANSPORT_CONTEXT Context,
    PIRP Irp,
    PDRIVER_CANCEL CancelRoutine)
/*
 * FUNCTION: Prepare an IRP for cancellation
 * ARGUMENTS:
 *     Context       = Pointer to context information
 *     Irp           = Pointer to an I/O request packet
 *     CancelRoutine = Routine to be called when I/O request is cancelled
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IoAcquireCancelSpinLock(&OldIrql);

    if (!Irp->Cancel) {
        IoMarkIrpPending(Irp);
        (void)IoSetCancelRoutine(Irp, CancelRoutine);
        IoReleaseCancelSpinLock(OldIrql);

        TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP at 0x%X can now be cancelled).\n", Irp));

        return STATUS_SUCCESS;
    }

    /* IRP has already been cancelled */

    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP was already cancelled).\n"));

    return IRPFinish(Irp, STATUS_CANCELLED);
}


VOID DispCancelComplete(
    PVOID Context)
/*
 * FUNCTION: Completes a cancel request
 * ARGUMENTS:
 *     Context = Pointer to context information (FILE_OBJECT)
 */
{
    /*KIRQL OldIrql;*/
    PFILE_OBJECT FileObject;
    PTRANSPORT_CONTEXT TranContext;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    FileObject  = (PFILE_OBJECT)Context;
    TranContext = (PTRANSPORT_CONTEXT)FileObject->FsContext;

    /* Set the cleanup event */
    KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);

    /* We are expected to release the cancel spin lock */
    /*IoReleaseCancelSpinLock(OldIrql);*/

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));
}


VOID DispDataRequestComplete(
    PVOID Context,
    NTSTATUS Status,
    ULONG Count)
/*
 * FUNCTION: Completes a send/receive IRP
 * ARGUMENTS:
 *     Context = Pointer to context information (IRP)
 *     Status  = Status of the request
 *     Count   = Number of bytes sent or received
 */
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called for irp %x (%x, %d).\n",
			    Context, Status, Count));

    Irp         = Context;
    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    IoAcquireCancelSpinLock(&OldIrql);

    (void)IoSetCancelRoutine(Irp, NULL);

    if (Irp->Cancel || TranContext->CancelIrps) {
        /* The IRP has been cancelled */

        TI_DbgPrint(DEBUG_IRP, ("IRP is cancelled.\n"));

        Status = STATUS_CANCELLED;
        Count  = 0;
    }

    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Count;

    TI_DbgPrint(MID_TRACE, ("Irp->IoStatus.Status = %x\n",
			    Irp->IoStatus.Status));
    TI_DbgPrint(MID_TRACE, ("Irp->IoStatus.Information = %d\n",
			    Irp->IoStatus.Information));
    TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

    IRPFinish(Irp, Irp->IoStatus.Status);

    TI_DbgPrint(DEBUG_IRP, ("Done Completing IRP\n"));
}

typedef struct _DISCONNECT_TYPE {
    UINT Type;
    PVOID Context;
    PIRP Irp;
    PFILE_OBJECT FileObject;
} DISCONNECT_TYPE, *PDISCONNECT_TYPE;

VOID DispDoDisconnect( PVOID Data ) {
    PDISCONNECT_TYPE DisType = (PDISCONNECT_TYPE)Data;

    TI_DbgPrint(DEBUG_IRP, ("PostCancel: DoDisconnect\n"));
    TCPDisconnect
	( DisType->Context,
	  DisType->Type,
	  NULL,
	  NULL,
	  DispDataRequestComplete,
	  DisType->Irp );
    TI_DbgPrint(DEBUG_IRP, ("PostCancel: DoDisconnect done\n"));

    DispDataRequestComplete(DisType->Irp, STATUS_CANCELLED, 0);

    DispCancelComplete(DisType->FileObject);
}

VOID NTAPI DispCancelRequest(
    PDEVICE_OBJECT Device,
    PIRP Irp)
/*
 * FUNCTION: Cancels an IRP
 * ARGUMENTS:
 *     Device = Pointer to device object
 *     Irp    = Pointer to an I/O request packet
 */
{
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    PFILE_OBJECT FileObject;
    UCHAR MinorFunction;
    DISCONNECT_TYPE DisType;
    PVOID WorkItem;
    /*NTSTATUS Status = STATUS_SUCCESS;*/

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp         = IoGetCurrentIrpStackLocation(Irp);
    FileObject    = IrpSp->FileObject;
    TranContext   = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    MinorFunction = IrpSp->MinorFunction;

    TI_DbgPrint(DEBUG_IRP, ("IRP at (0x%X)  MinorFunction (0x%X)  IrpSp (0x%X).\n", Irp, MinorFunction, IrpSp));

    Irp->IoStatus.Status = STATUS_PENDING;

#ifdef DBG
    if (!Irp->Cancel)
        TI_DbgPrint(MIN_TRACE, ("Irp->Cancel is FALSE, should be TRUE.\n"));
#endif

    /* Try canceling the request */
    switch(MinorFunction) {
    case TDI_SEND:
    case TDI_RECEIVE:
	DisType.Type = TDI_DISCONNECT_RELEASE |
	    ((MinorFunction == TDI_RECEIVE) ? TDI_DISCONNECT_ABORT : 0);
	DisType.Context = TranContext->Handle.ConnectionContext;
	DisType.Irp = Irp;
	DisType.FileObject = FileObject;

	TCPRemoveIRP( TranContext->Handle.ConnectionContext, Irp );

	if( !ChewCreate( &WorkItem, sizeof(DISCONNECT_TYPE),
			 DispDoDisconnect, &DisType ) )
	    ASSERT(0);
        break;

    case TDI_SEND_DATAGRAM:
	Irp->IoStatus.Status = STATUS_CANCELLED;
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_SEND_DATAGRAM, but no address file.\n"));
            break;
        }

        /*DGCancelSendRequest(TranContext->Handle.AddressHandle, Irp);*/
        break;

    case TDI_RECEIVE_DATAGRAM:
	Irp->IoStatus.Status = STATUS_CANCELLED;
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_RECEIVE_DATAGRAM, but no address file.\n"));
            break;
        }

        /*DGCancelReceiveRequest(TranContext->Handle.AddressHandle, Irp);*/
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Unknown IRP. MinorFunction (0x%X).\n", MinorFunction));
        break;
    }

    if( Irp->IoStatus.Status == STATUS_PENDING )
	IoMarkIrpPending(Irp);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID NTAPI DispCancelListenRequest(
    PDEVICE_OBJECT Device,
    PIRP Irp)
/*
 * FUNCTION: Cancels a listen IRP
 * ARGUMENTS:
 *     Device = Pointer to device object
 *     Irp    = Pointer to an I/O request packet
 */
{
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    PFILE_OBJECT FileObject;
    PCONNECTION_ENDPOINT Connection;
    /*NTSTATUS Status = STATUS_SUCCESS;*/

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp         = IoGetCurrentIrpStackLocation(Irp);
    FileObject    = IrpSp->FileObject;
    TranContext   = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    ASSERT( TDI_LISTEN == IrpSp->MinorFunction);

    TI_DbgPrint(DEBUG_IRP, ("IRP at (0x%X).\n", Irp));

#ifdef DBG
    if (!Irp->Cancel)
        TI_DbgPrint(MIN_TRACE, ("Irp->Cancel is FALSE, should be TRUE.\n"));
#endif

    /* Try canceling the request */
    Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
    TCPAbortListenForSocket(
	    Connection->AddressFile->Listener,
	    Connection );

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    DispDataRequestComplete(Irp, STATUS_CANCELLED, 0);

    DispCancelComplete(FileObject);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS DispTdiAccept(
  PIRP Irp)
/*
 * FUNCTION: TDI_ACCEPT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiAssociateAddress(
    PIRP Irp)
/*
 * FUNCTION: TDI_ASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_ASSOCIATE Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  PCONNECTION_ENDPOINT Connection;
  PFILE_OBJECT FileObject;
  PADDRESS_FILE AddrFile = NULL;
  NTSTATUS Status;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  if (Connection->AddressFile) {
    TI_DbgPrint(MID_TRACE, ("An address file is already asscociated.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;

  Status = ObReferenceObjectByHandle(
    Parameters->AddressHandle,
    0,
    IoFileObjectType,
    KernelMode,
    (PVOID*)&FileObject,
    NULL);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MID_TRACE, ("Bad address file object handle (0x%X): %x.\n",
      Parameters->AddressHandle, Status));
    return STATUS_INVALID_PARAMETER;
  }

  if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
    ObDereferenceObject(FileObject);
    TI_DbgPrint(MID_TRACE, ("Bad address file object. Magic (0x%X).\n",
      FileObject->FsContext2));
    return STATUS_INVALID_PARAMETER;
  }

  /* Get associated address file object. Quit if none exists */

  TranContext = FileObject->FsContext;
  if (!TranContext) {
    ObDereferenceObject(FileObject);
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
  if (!AddrFile) {
      ObDereferenceObject(FileObject);
      TI_DbgPrint(MID_TRACE, ("No address file object.\n"));
      return STATUS_INVALID_PARAMETER;
  }

  Connection->AddressFile = AddrFile;

  /* Add connection endpoint to the address file */
  AddrFile->Connection = Connection;

  /* FIXME: Maybe do this in DispTdiDisassociateAddress() instead? */
  ObDereferenceObject(FileObject);

  return Status;
}


NTSTATUS DispTdiConnect(
  PIRP Irp)
/*
 * FUNCTION: TDI_CONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTDI_REQUEST_KERNEL Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Parameters = (PTDI_REQUEST_KERNEL)&IrpSp->Parameters;

  Status = TCPConnect(
      TranContext->Handle.ConnectionContext,
      Parameters->RequestConnectionInformation,
      Parameters->ReturnConnectionInformation,
      DispDataRequestComplete,
      Irp );

  TI_DbgPrint(MAX_TRACE, ("TCP Connect returned %08x\n", Status));

  return Status;
}


NTSTATUS DispTdiDisassociateAddress(
  PIRP Irp)
/*
 * FUNCTION: TDI_DISASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  if (!Connection->AddressFile) {
    TI_DbgPrint(MID_TRACE, ("No address file is asscociated.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  return STATUS_SUCCESS;
}


NTSTATUS DispTdiDisconnect(
  PIRP Irp)
/*
 * FUNCTION: TDI_DISCONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  PTDI_REQUEST_KERNEL_DISCONNECT DisReq;
  PCONNECTION_ENDPOINT Connection;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  DisReq = (PTDI_REQUEST_KERNEL_DISCONNECT)&IrpSp->Parameters;

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Status = TCPDisconnect(
      TranContext->Handle.ConnectionContext,
      DisReq->RequestFlags,
      DisReq->RequestConnectionInformation,
      DisReq->ReturnConnectionInformation,
      DispDataRequestComplete,
      Irp );

  TI_DbgPrint(MAX_TRACE, ("TCP Connect returned %08x\n", Status));

  return Status;
}


NTSTATUS DispTdiListen(
  PIRP Irp)
/*
 * FUNCTION: TDI_LISTEN handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTDI_REQUEST_KERNEL Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status = STATUS_SUCCESS;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (Connection == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  Parameters = (PTDI_REQUEST_KERNEL)&IrpSp->Parameters;

  TI_DbgPrint(MIN_TRACE, ("Connection->AddressFile: %x\n",
			  Connection->AddressFile ));
  if( Connection->AddressFile ) {
      TI_DbgPrint(MIN_TRACE, ("Connection->AddressFile->Listener: %x\n",
			      Connection->AddressFile->Listener));
  }

  /* Listening will require us to create a listening socket and store it in
   * the address file.  It will be signalled, and attempt to complete an irp
   * when a new connection arrives. */
  /* The important thing to note here is that the irp we'll complete belongs
   * to the socket to be accepted onto, not the listener */
  if( !Connection->AddressFile->Listener ) {
      Connection->AddressFile->Listener =
	  TCPAllocateConnectionEndpoint( NULL );

      if( !Connection->AddressFile->Listener )
	  Status = STATUS_NO_MEMORY;

      if( NT_SUCCESS(Status) ) {
	  Connection->AddressFile->Listener->AddressFile =
	      Connection->AddressFile;

	  Status = TCPSocket( Connection->AddressFile->Listener,
			      Connection->AddressFile->Family,
			      SOCK_STREAM,
			      Connection->AddressFile->Protocol );
      }

      if( NT_SUCCESS(Status) )
	  Status = TCPListen( Connection->AddressFile->Listener, 1024 );
	  /* BACKLOG */
  }
  if( NT_SUCCESS(Status) ) {
      Status = DispPrepareIrpForCancel
          (TranContext->Handle.ConnectionContext,
           Irp,
           (PDRIVER_CANCEL)DispCancelListenRequest);
  }

  if( NT_SUCCESS(Status) ) {
      Status = TCPAccept
	  ( (PTDI_REQUEST)Parameters,
	    Connection->AddressFile->Listener,
	    Connection,
	    DispDataRequestComplete,
	    Irp );
  }

  TI_DbgPrint(MID_TRACE,("Leaving %x\n", Status));

  return Status;
}


NTSTATUS DispTdiQueryInformation(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: TDI_QUERY_INFORMATION handler
 * ARGUMENTS:
 *     DeviceObject = Pointer to device object structure
 *     Irp          = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_QUERY_INFORMATION Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  Parameters = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  switch (Parameters->QueryType)
  {
    case TDI_QUERY_ADDRESS_INFO:
      {
        PTDI_ADDRESS_INFO AddressInfo;
        PADDRESS_FILE AddrFile;
        PTA_IP_ADDRESS Address;

        AddressInfo = (PTDI_ADDRESS_INFO)MmGetSystemAddressForMdl(Irp->MdlAddress);

        switch ((ULONG)IrpSp->FileObject->FsContext2) {
          case TDI_TRANSPORT_ADDRESS_FILE:
            AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
            break;

          case TDI_CONNECTION_FILE:
            AddrFile =
              ((PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext)->
              AddressFile;
            break;

          default:
            TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
            return STATUS_INVALID_PARAMETER;
        }

        if (!AddrFile) {
          TI_DbgPrint(MID_TRACE, ("No address file object.\n"));
          return STATUS_INVALID_PARAMETER;
        }

        if (MmGetMdlByteCount(Irp->MdlAddress) <
            (FIELD_OFFSET(TDI_ADDRESS_INFO, Address.Address[0].Address) +
             sizeof(TDI_ADDRESS_IP))) {
          TI_DbgPrint(MID_TRACE, ("MDL buffer too small.\n"));
          return STATUS_BUFFER_OVERFLOW;
        }

        Address = (PTA_IP_ADDRESS)&AddressInfo->Address;
        Address->TAAddressCount = 1;
        Address->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        Address->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        Address->Address[0].Address[0].sin_port = AddrFile->Port;
        Address->Address[0].Address[0].in_addr =
          AddrFile->Address.Address.IPv4Address;
        RtlZeroMemory(
          &Address->Address[0].Address[0].sin_zero,
          sizeof(Address->Address[0].Address[0].sin_zero));

        return STATUS_SUCCESS;
      }

    case TDI_QUERY_CONNECTION_INFO:
      {
        PTDI_CONNECTION_INFORMATION AddressInfo;
        PADDRESS_FILE AddrFile;
        PCONNECTION_ENDPOINT Endpoint = NULL;

        AddressInfo = (PTDI_CONNECTION_INFORMATION)
          MmGetSystemAddressForMdl(Irp->MdlAddress);

        switch ((ULONG)IrpSp->FileObject->FsContext2) {
          case TDI_TRANSPORT_ADDRESS_FILE:
            AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
            break;

          case TDI_CONNECTION_FILE:
            Endpoint =
              (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
            break;

          default:
            TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
            return STATUS_INVALID_PARAMETER;
        }

        if (!Endpoint) {
          TI_DbgPrint(MID_TRACE, ("No connection object.\n"));
          return STATUS_INVALID_PARAMETER;
        }

        if (MmGetMdlByteCount(Irp->MdlAddress) <
            (FIELD_OFFSET(TDI_CONNECTION_INFORMATION, RemoteAddress) +
             sizeof(PVOID))) {
          TI_DbgPrint(MID_TRACE, ("MDL buffer too small (ptr).\n"));
          return STATUS_BUFFER_OVERFLOW;
        }

        return TCPGetPeerAddress( Endpoint, AddressInfo->RemoteAddress );
      }
  }

  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiReceive(
  PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST_KERNEL_RECEIVE ReceiveInfo;
  PTRANSPORT_CONTEXT TranContext;
  NTSTATUS Status;
  ULONG BytesReceived;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  ReceiveInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&(IrpSp->Parameters);

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  if (TranContext->Handle.ConnectionContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  /* Initialize a receive request */
  Status = DispPrepareIrpForCancel
      (TranContext->Handle.ConnectionContext,
       Irp,
       (PDRIVER_CANCEL)DispCancelRequest);

  TI_DbgPrint(MID_TRACE,("TCPIP<<< Got an MDL: %x\n", Irp->MdlAddress));
  if (NT_SUCCESS(Status))
    {
      /* Lock here so we're sure we've got the following 'mark pending' */
      TcpipRecursiveMutexEnter( &TCPLock, TRUE );

      Status = TCPReceiveData(
	  TranContext->Handle.ConnectionContext,
	  (PNDIS_BUFFER)Irp->MdlAddress,
	  ReceiveInfo->ReceiveLength,
	  &BytesReceived,
	  ReceiveInfo->ReceiveFlags,
	  DispDataRequestComplete,
	  Irp);
      if (Status != STATUS_PENDING)
      {
          DispDataRequestComplete(Irp, Status, BytesReceived);
      } else {
	  IoMarkIrpPending(Irp);
      }

      TcpipRecursiveMutexLeave( &TCPLock );
    }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return Status;
}


NTSTATUS DispTdiReceiveDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST_KERNEL_RECEIVEDG DgramInfo;
  PTRANSPORT_CONTEXT TranContext;
  TDI_REQUEST Request;
  NTSTATUS Status;
  ULONG BytesReceived;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp     = IoGetCurrentIrpStackLocation(Irp);
  DgramInfo = (PTDI_REQUEST_KERNEL_RECEIVEDG)&(IrpSp->Parameters);

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_ADDRESS;
    }

  /* Initialize a receive request */
  Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
  Request.RequestNotifyObject  = DispDataRequestComplete;
  Request.RequestContext       = Irp;

  Status = DispPrepareIrpForCancel(
    IrpSp->FileObject->FsContext,
    Irp,
    (PDRIVER_CANCEL)DispCancelRequest);

  if (NT_SUCCESS(Status))
    {
	PCHAR DataBuffer;
	UINT BufferSize;

	NdisQueryBuffer( (PNDIS_BUFFER)Irp->MdlAddress,
			 &DataBuffer,
			 &BufferSize );

      Status = UDPReceiveDatagram(
	  Request.Handle.AddressHandle,
	  DgramInfo->ReceiveDatagramInformation,
	  DataBuffer,
	  DgramInfo->ReceiveLength,
	  DgramInfo->ReceiveFlags,
	  DgramInfo->ReturnDatagramInformation,
	  &BytesReceived,
	  (PDATAGRAM_COMPLETION_ROUTINE)DispDataRequestComplete,
	  Irp);
      if (Status != STATUS_PENDING) {
          DispDataRequestComplete(Irp, Status, BytesReceived);
      } else
	  IoMarkIrpPending(Irp);
    }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return Status;
}


NTSTATUS DispTdiSend(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST_KERNEL_SEND SendInfo;
  PTRANSPORT_CONTEXT TranContext;
  NTSTATUS Status;
  ULONG BytesSent;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  SendInfo = (PTDI_REQUEST_KERNEL_SEND)&(IrpSp->Parameters);

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  if (TranContext->Handle.ConnectionContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  Status = DispPrepareIrpForCancel(
    IrpSp->FileObject->FsContext,
    Irp,
    (PDRIVER_CANCEL)DispCancelRequest);

  TI_DbgPrint(MID_TRACE,("TCPIP<<< Got an MDL: %x\n", Irp->MdlAddress));
  if (NT_SUCCESS(Status))
    {
	PCHAR Data;
	UINT Len;

	NdisQueryBuffer( Irp->MdlAddress, &Data, &Len );

	TI_DbgPrint(MID_TRACE,("About to TCPSendData\n"));
	Status = TCPSendData(
	    TranContext->Handle.ConnectionContext,
	    Data,
	    SendInfo->SendLength,
	    &BytesSent,
	    SendInfo->SendFlags,
	    DispDataRequestComplete,
	    Irp);
	if (Status != STATUS_PENDING)
	{
	    DispDataRequestComplete(Irp, Status, BytesSent);
	} else
	    IoMarkIrpPending( Irp );
    }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return Status;
}


NTSTATUS DispTdiSendDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    TDI_REQUEST Request;
    PTDI_REQUEST_KERNEL_SENDDG DgramInfo;
    PTRANSPORT_CONTEXT TranContext;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    DgramInfo   = (PTDI_REQUEST_KERNEL_SENDDG)&(IrpSp->Parameters);
    TranContext = IrpSp->FileObject->FsContext;

    /* Initialize a send request */
    Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
    Request.RequestNotifyObject  = DispDataRequestComplete;
    Request.RequestContext       = Irp;

    Status = DispPrepareIrpForCancel(
        IrpSp->FileObject->FsContext,
        Irp,
        (PDRIVER_CANCEL)DispCancelRequest);

    if (NT_SUCCESS(Status)) {
	PCHAR DataBuffer;
	UINT BufferSize;

	TI_DbgPrint(MID_TRACE,("About to query buffer %x\n", Irp->MdlAddress));

	NdisQueryBuffer( (PNDIS_BUFFER)Irp->MdlAddress,
			 &DataBuffer,
			 &BufferSize );

        /* FIXME: DgramInfo->SendDatagramInformation->RemoteAddress
           must be of type PTDI_ADDRESS_IP */
	TI_DbgPrint(MID_TRACE,
		    ("About to call send routine %x\n",
		     (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send)));

        if( (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send) )
            Status = (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send)(
                Request.Handle.AddressHandle,
                DgramInfo->SendDatagramInformation,
                DataBuffer,
                BufferSize,
                &Irp->IoStatus.Information);
        else
            Status = STATUS_UNSUCCESSFUL;

        if (Status != STATUS_PENDING) {
            DispDataRequestComplete(Irp, Status, Irp->IoStatus.Information);
            /* Return STATUS_PENDING because DispPrepareIrpForCancel
               marks Irp as pending */
            Status = STATUS_PENDING;
        } else
	    IoMarkIrpPending( Irp );
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));

    return Status;
}


NTSTATUS DispTdiSetEventHandler(PIRP Irp)
/*
 * FUNCTION: TDI_SET_EVENT_HANDER handler
 * ARGUMENTS:
 *     Irp = Pointer to a I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_SET_EVENT Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  PADDRESS_FILE AddrFile;
  NTSTATUS Status;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated address file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MIN_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
  if (!AddrFile) {
    TI_DbgPrint(MIN_TRACE, ("No address file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)&IrpSp->Parameters;
  Status     = STATUS_SUCCESS;

  TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Set the event handler. if an event handler is associated with
     a specific event, it's flag (RegisteredXxxHandler) is TRUE.
     If an event handler is not used it's flag is FALSE */
  switch (Parameters->EventType) {
  case TDI_EVENT_CONNECT:
    if (!Parameters->EventHandler) {
      AddrFile->ConnectHandlerContext    = NULL;
      AddrFile->RegisteredConnectHandler = FALSE;
    } else {
      AddrFile->ConnectHandler =
        (PTDI_IND_CONNECT)Parameters->EventHandler;
      AddrFile->ConnectHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredConnectHandler = TRUE;
    }
    break;

  case TDI_EVENT_DISCONNECT:
    if (!Parameters->EventHandler) {
      AddrFile->DisconnectHandlerContext    = NULL;
      AddrFile->RegisteredDisconnectHandler = FALSE;
    } else {
      AddrFile->DisconnectHandler =
        (PTDI_IND_DISCONNECT)Parameters->EventHandler;
      AddrFile->DisconnectHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredDisconnectHandler = TRUE;
    }
    break;

    case TDI_EVENT_ERROR:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ErrorHandlerContext    = NULL;
      AddrFile->RegisteredErrorHandler = FALSE;
    } else {
      AddrFile->ErrorHandler =
        (PTDI_IND_ERROR)Parameters->EventHandler;
      AddrFile->ErrorHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredErrorHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ReceiveHandlerContext    = NULL;
      AddrFile->RegisteredReceiveHandler = FALSE;
    } else {
      AddrFile->ReceiveHandler =
        (PTDI_IND_RECEIVE)Parameters->EventHandler;
      AddrFile->ReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE_DATAGRAM:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ReceiveDatagramHandlerContext    = NULL;
      AddrFile->RegisteredReceiveDatagramHandler = FALSE;
    } else {
      AddrFile->ReceiveDatagramHandler =
        (PTDI_IND_RECEIVE_DATAGRAM)Parameters->EventHandler;
      AddrFile->ReceiveDatagramHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredReceiveDatagramHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE_EXPEDITED:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ExpeditedReceiveHandlerContext    = NULL;
      AddrFile->RegisteredExpeditedReceiveHandler = FALSE;
    } else {
      AddrFile->ExpeditedReceiveHandler =
        (PTDI_IND_RECEIVE_EXPEDITED)Parameters->EventHandler;
      AddrFile->ExpeditedReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredExpeditedReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveHandler =
        (PTDI_IND_CHAINED_RECEIVE)Parameters->EventHandler;
      AddrFile->ChainedReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveDatagramHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveDatagramHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveDatagramHandler =
        (PTDI_IND_CHAINED_RECEIVE_DATAGRAM)Parameters->EventHandler;
      AddrFile->ChainedReceiveDatagramHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveDatagramHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveExpeditedHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveExpeditedHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveExpeditedHandler =
        (PTDI_IND_CHAINED_RECEIVE_EXPEDITED)Parameters->EventHandler;
      AddrFile->ChainedReceiveExpeditedHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveExpeditedHandler = TRUE;
    }
    break;

  default:
    TI_DbgPrint(MIN_TRACE, ("Unknown event type (0x%X).\n",
      Parameters->EventType));

    Status = STATUS_INVALID_PARAMETER;
  }

  TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

  return Status;
}


NTSTATUS DispTdiSetInformation(
    PIRP Irp)
/*
 * FUNCTION: TDI_SET_INFORMATION handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


VOID DispTdiQueryInformationExComplete(
    PVOID Context,
    ULONG Status,
    UINT ByteCount)
/*
 * FUNCTION: Completes a TDI QueryInformationEx request
 * ARGUMENTS:
 *     Context   = Pointer to the IRP for the request
 *     Status    = TDI status of the request
 *     ByteCount = Number of bytes returned in output buffer
 */
{
    PTI_QUERY_CONTEXT QueryContext;
    UINT Count = 0;

    QueryContext = (PTI_QUERY_CONTEXT)Context;
    if (NT_SUCCESS(Status)) {
        Count = CopyBufferToBufferChain(
            QueryContext->InputMdl,
            FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX, Context),
            (PCHAR)&QueryContext->QueryInfo.Context,
            CONTEXT_SIZE);
    }

    MmUnlockPages(QueryContext->InputMdl);
    IoFreeMdl(QueryContext->InputMdl);
    if( QueryContext->OutputMdl ) {
	MmUnlockPages(QueryContext->OutputMdl);
	IoFreeMdl(QueryContext->OutputMdl);
    }

    QueryContext->Irp->IoStatus.Information = ByteCount;
    QueryContext->Irp->IoStatus.Status      = Status;

    ExFreePool(QueryContext);
}


NTSTATUS DispTdiQueryInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI QueryInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTCP_REQUEST_QUERY_INFORMATION_EX InputBuffer;
    PTRANSPORT_CONTEXT TranContext;
    PTI_QUERY_CONTEXT QueryContext;
    PVOID OutputBuffer;
    TDI_REQUEST Request;
    UINT Size;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    BOOLEAN InputMdlLocked  = FALSE;
    BOOLEAN OutputMdlLocked = FALSE;
    PMDL InputMdl           = NULL;
    PMDL OutputMdl          = NULL;
    NTSTATUS Status         = STATUS_SUCCESS;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength == sizeof(TCP_REQUEST_QUERY_INFORMATION_EX)) &&
        (OutputBufferLength != 0)) {

        InputBuffer = (PTCP_REQUEST_QUERY_INFORMATION_EX)
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        OutputBuffer = Irp->UserBuffer;

        QueryContext = ExAllocatePool(NonPagedPool, sizeof(TI_QUERY_CONTEXT));
        if (QueryContext) {
	    _SEH_TRY {
                InputMdl = IoAllocateMdl(InputBuffer,
                    sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                    FALSE, TRUE, NULL);

                OutputMdl = IoAllocateMdl(OutputBuffer,
                    OutputBufferLength, FALSE, TRUE, NULL);

                if (InputMdl && OutputMdl) {

                    MmProbeAndLockPages(InputMdl, Irp->RequestorMode,
                        IoModifyAccess);

                    InputMdlLocked = TRUE;

                    MmProbeAndLockPages(OutputMdl, Irp->RequestorMode,
                        IoWriteAccess);

                    OutputMdlLocked = TRUE;

                    RtlCopyMemory(&QueryContext->QueryInfo,
                        InputBuffer, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
                } else
                    Status = STATUS_INSUFFICIENT_RESOURCES;
            } _SEH_HANDLE {
                Status = _SEH_GetExceptionCode();
            } _SEH_END;

            if (NT_SUCCESS(Status)) {
                Size = MmGetMdlByteCount(OutputMdl);

                QueryContext->Irp       = Irp;
                QueryContext->InputMdl  = InputMdl;
                QueryContext->OutputMdl = OutputMdl;

                Request.RequestNotifyObject = DispTdiQueryInformationExComplete;
                Request.RequestContext      = QueryContext;
                Status = InfoTdiQueryInformationEx(&Request,
                    &QueryContext->QueryInfo.ID, OutputMdl,
                    &Size, &QueryContext->QueryInfo.Context);
                DispTdiQueryInformationExComplete(QueryContext, Status, Size);

                TI_DbgPrint(MAX_TRACE, ("Leaving. Status = (0x%X)\n", Status));

                return Status;
            }

            /* An error occurred if we get here */

            if (InputMdl) {
                if (InputMdlLocked)
                    MmUnlockPages(InputMdl);
                IoFreeMdl(InputMdl);
            }

            if (OutputMdl) {
                if (OutputMdlLocked)
                    MmUnlockPages(OutputMdl);
                IoFreeMdl(OutputMdl);
            }

            ExFreePool(QueryContext);
        } else
            Status = STATUS_INSUFFICIENT_RESOURCES;
    } else if( InputBufferLength ==
	       sizeof(TCP_REQUEST_QUERY_INFORMATION_EX) ) {
	/* Handle the case where the user is probing the buffer for length */
	TI_DbgPrint(MAX_TRACE, ("InputBufferLength %d OutputBufferLength %d\n",
				InputBufferLength, OutputBufferLength));
        InputBuffer = (PTCP_REQUEST_QUERY_INFORMATION_EX)
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

	Size = 0;

        QueryContext = ExAllocatePool(NonPagedPool, sizeof(TI_QUERY_CONTEXT));
        if (!QueryContext) return STATUS_INSUFFICIENT_RESOURCES;

	_SEH_TRY {
	    InputMdl = IoAllocateMdl(InputBuffer,
				     sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
				     FALSE, TRUE, NULL);

	    MmProbeAndLockPages(InputMdl, Irp->RequestorMode,
				IoModifyAccess);

	    InputMdlLocked = TRUE;
	    Status = STATUS_SUCCESS;
	} _SEH_HANDLE {
	    TI_DbgPrint(MAX_TRACE, ("Failed to acquire client buffer\n"));
	    Status = _SEH_GetExceptionCode();
	} _SEH_END;

	if( !NT_SUCCESS(Status) || !InputMdl ) {
	    if( InputMdl ) IoFreeMdl( InputMdl );
	    ExFreePool(QueryContext);
	    return Status;
	}

	RtlCopyMemory(&QueryContext->QueryInfo,
		      InputBuffer, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

	QueryContext->Irp       = Irp;
	QueryContext->InputMdl  = InputMdl;
	QueryContext->OutputMdl = NULL;

	Request.RequestNotifyObject = DispTdiQueryInformationExComplete;
	Request.RequestContext      = QueryContext;
	Status = InfoTdiQueryInformationEx(&Request,
					   &QueryContext->QueryInfo.ID,
					   NULL,
					   &Size,
					   &QueryContext->QueryInfo.Context);
	DispTdiQueryInformationExComplete(QueryContext, Status, Size);
	TI_DbgPrint(MAX_TRACE, ("Leaving. Status = (0x%X)\n", Status));
    } else Status = STATUS_INVALID_PARAMETER;

    TI_DbgPrint(MIN_TRACE, ("Leaving. Status = (0x%X)\n", Status));

    return Status;
}


NTSTATUS DispTdiSetInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI SetInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTRANSPORT_CONTEXT TranContext;
    PTCP_REQUEST_SET_INFORMATION_EX Info;
    TDI_REQUEST Request;
    TDI_STATUS Status;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;
    Info        = (PTCP_REQUEST_SET_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        return IRPFinish(Irp, STATUS_INVALID_PARAMETER);
    }

    Status = DispPrepareIrpForCancel(TranContext, Irp, NULL);
    if (NT_SUCCESS(Status)) {
        Request.RequestNotifyObject = DispDataRequestComplete;
        Request.RequestContext      = Irp;

        Status = InfoTdiSetInformationEx(&Request, &Info->ID,
            &Info->Buffer, Info->BufferSize);

        if (Status != STATUS_PENDING) {
            IoAcquireCancelSpinLock(&OldIrql);
            (void)IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(OldIrql);
        }
    }

    return Status;
}

/* TODO: Support multiple addresses per interface.
 * For now just set the nte context to the interface index.
 *
 * Later on, create an NTE context and NTE instance
 */

NTSTATUS DispTdiSetIPAddress( PIRP Irp, PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_DEVICE_DOES_NOT_EXIST;
    PIP_SET_ADDRESS IpAddrChange =
        (PIP_SET_ADDRESS)Irp->AssociatedIrp.SystemBuffer;
    IF_LIST_ITER(IF);

    TI_DbgPrint(MID_TRACE,("Setting IP Address for adapter %d\n",
			   IpAddrChange->NteIndex));

    ForEachInterface(IF) {
	TI_DbgPrint(MID_TRACE,("Looking at adapter %d\n", IF->Index));

        if( IF->Unicast.Address.IPv4Address == IpAddrChange->Address ) {
            Status = STATUS_DUPLICATE_OBJECTID;
            break;
        }
        if( IF->Index == IpAddrChange->NteIndex ) {
            IPRemoveInterfaceRoute( IF );

            IF->Unicast.Type = IP_ADDRESS_V4;
            IF->Unicast.Address.IPv4Address = IpAddrChange->Address;
            IF->Netmask.Type = IP_ADDRESS_V4;
            IF->Netmask.Address.IPv4Address = IpAddrChange->Netmask;
	    IF->Broadcast.Address.IPv4Address =
		IF->Unicast.Address.IPv4Address |
		~IF->Netmask.Address.IPv4Address;

            TI_DbgPrint(MID_TRACE,("New Unicast Address: %x\n",
                                   IF->Unicast.Address.IPv4Address));
            TI_DbgPrint(MID_TRACE,("New Netmask        : %x\n",
                                   IF->Netmask.Address.IPv4Address));

            IPAddInterfaceRoute( IF );

            IpAddrChange->Address = IF->Index;
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = IF->Index;
            break;
        }
    } EndFor(IF);

    Irp->IoStatus.Status = Status;
    return Status;
}

NTSTATUS DispTdiDeleteIPAddress( PIRP Irp, PIO_STACK_LOCATION IrpSp ) {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PUSHORT NteIndex = Irp->AssociatedIrp.SystemBuffer;
    IF_LIST_ITER(IF);

    ForEachInterface(IF) {
        if( IF->Index == *NteIndex ) {
            IPRemoveInterfaceRoute( IF );
            IF->Unicast.Type = IP_ADDRESS_V4;
            IF->Unicast.Address.IPv4Address = 0;
            IF->Netmask.Type = IP_ADDRESS_V4;
            IF->Netmask.Address.IPv4Address = 0;
            Status = STATUS_SUCCESS;
        }
    } EndFor(IF);

    Irp->IoStatus.Status = Status;
    return Status;
}

/* EOF */
