/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/dispatch.c
 * PURPOSE:     File object dispatch functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <afd.h>

NTSTATUS AfdpDispRecvFrom(
    PIRP Irp,
    PAFDFCB FCB,
    PFILE_REQUEST_RECVFROM Request,
    PFILE_REPLY_RECVFROM Reply)
/*
 * FUNCTION: Receives data
 * ARGUMENTS:
 *     Irp     = Pointer to I/O request packet
 *     FCB     = Pointer to file control block
 *     Request = Address of request buffer
 *     Reply   = Address of reply buffer (same as request buffer)
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AFD_READ_REQUEST ReadRequest;
    
    AFD_DbgPrint(MAX_TRACE, ("Queueing read request.\n"));
    
    ReadRequest.Recv.Request = Request;
    ReadRequest.Recv.Reply = Reply;
    
    return AfdpMakeWork( AFD_OP_RECV_REQUEST, Irp, IrpSp, 
			 (PCHAR)&ReadRequest, sizeof(ReadRequest) );
}

NTSTATUS AfdDispBind(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Binds to an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_BIND Request;
  PFILE_REPLY_BIND Reply;
  PAFDFCB FCB;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
  
  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_BIND)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_BIND))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_BIND)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_BIND)Irp->AssociatedIrp.SystemBuffer;

    Status = TdiOpenAddressFile(
      &FCB->TdiDeviceName,
      &Request->Name,
      &FCB->TdiAddressObjectHandle,
      &FCB->TdiAddressObject);

    if (NT_SUCCESS(Status)) {
      AfdRegisterEventHandlers(FCB);
      FCB->State = SOCKET_STATE_BOUND;
      Reply->Status = NO_ERROR;
    } else {
      //FIXME: WSAEADDRNOTAVAIL
      Reply->Status = WSAEINVAL;
    }
  } else
      Status = STATUS_INVALID_PARAMETER;

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

NTSTATUS
STDCALL
AfdDispCompleteListen(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context)
{
    PAFDFCB FCB = (PAFDFCB) Context;

    AFD_DbgPrint(MAX_TRACE, ("1b - Status (0x%X).\n", 
			     FCB->ListenRequest.Iosb.Status));
    
    if (NT_SUCCESS(FCB->ListenRequest.Iosb.Status))
    {
	AFD_DbgPrint(MAX_TRACE, ("2 - Completed ListenRequest at (0x%X).\n",
				 FCB->ListenRequest));

	FCB->ListenRequest.Result = NO_ERROR;
	FCB->ListenRequest.Valid = TRUE;
    } else {
	FCB->ListenRequest.Result = WSAENOTSOCK; /* XXX Fix later */
	FCB->ListenRequest.Valid = TRUE;
    }
    RegisterFCBForWork(FCB);

    return STATUS_SUCCESS;
}


NTSTATUS AfdDispListen(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Starts listening for connections
 *
 * The real purpose of the TDI_LISTEN call is put one backlog object on the
 * the socket to hold one connection when it becomes ready.  This rather
 * counterintuitive method essentially requires that we keep one TDI_LISTEN
 * live at all times after the first.  I haven't been able to figure out 
 * whether subsequent calls to listen adjust the backlog or simply succeed.
 *
 * We succeed each listen request after this by checking the FCB state.
 * Listen requests really don't make a socket more listeny.
 *
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_LISTEN Request;
    PFILE_REPLY_LISTEN Reply;
    PAFDFCB FCB;
    SOCKADDR EmptyAddr = { 0 };
    
    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    /* Validate parameters */
    Status = STATUS_INVALID_PARAMETER;
    if ((InputBufferLength >= sizeof(FILE_REQUEST_LISTEN)) &&
	(OutputBufferLength >= sizeof(FILE_REPLY_LISTEN))) {
	FCB = IrpSp->FileObject->FsContext;
	
	Request = (PFILE_REQUEST_LISTEN)Irp->AssociatedIrp.SystemBuffer;
	Reply   = (PFILE_REPLY_LISTEN)Irp->AssociatedIrp.SystemBuffer;
	
	if (FCB->State == SOCKET_STATE_CREATED)
	{
	    EmptyAddr.sa_family = FCB->AddressFamily;
	    Status = TdiOpenAddressFile(
		&FCB->TdiDeviceName,
		&EmptyAddr,
		&FCB->TdiAddressObjectHandle,
		&FCB->TdiAddressObject);

	    if(NT_SUCCESS(Status)) {
		AfdRegisterEventHandlers(FCB);
		FCB->State = SOCKET_STATE_BOUND;
	    }
	}

	if (FCB->State == SOCKET_STATE_BOUND)
	{
	    /* We have a bound socket so go ahead and create a connection 
	       endpoint and associate it with the address file object */
	    
	    Status = TdiOpenConnectionEndpointFile(
		&FCB->TdiDeviceName,
		&FCB->TdiConnectionObjectHandle,
		&FCB->TdiConnectionObject);
	    
	    if (NT_SUCCESS(Status))
	    {
		Status = TdiAssociateAddressFile(
		    FCB->TdiAddressObjectHandle,
		    FCB->TdiConnectionObject);
		
		if (NT_SUCCESS(Status))
		{
		    Status = TdiListen(&FCB->ListenRequest, 
				       AfdDispCompleteListen, 
				       FCB);

		    if ((Status == STATUS_PENDING) || NT_SUCCESS(Status))
		    {
			Status = STATUS_SUCCESS;
		    }
		    else
		    {
			/* FIXME: Cleanup ListenRequest */
			/* FIXME: Cleanup from TdiOpenConnectionEndpointFile */
		    }
		}
		else
		{
		    /* FIXME: Cleanup from TdiOpenConnectionEndpointFile */
		}
	    }
	    else
	    {
		/* FIXME: Cleanup from TdiOpenConnectionEndpointFile */
	    }
	}
	
	if (NT_SUCCESS(Status)) {
	    Reply->Status = NO_ERROR;
	} else {
	    Reply->Status = WSAEINVAL;
	}
    }
    else if (FCB->State == SOCKET_STATE_LISTENING)
    {
	Reply->Status = NO_ERROR;
	Status = STATUS_SUCCESS;
    }
    else if (FCB->State == SOCKET_STATE_CONNECTED)
    {
	Reply->Status = WSAEISCONN;
    }
    else
    {
	Reply->Status = WSAEINVAL;
    }
    
    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
    
    return Status;
}

/* 
 * Accept requests are a prime candidate for handling by the work thread.
 */
NTSTATUS AfdDispAccept(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp) {
    return AfdpMakeWork( AFD_OP_ACCEPT_REQUEST, Irp, IrpSp, NULL, 0 );
}

NTSTATUS AfdpDispSendTo(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp,
    BOOL WithAddr) {
/*
 * FUNCTION: Sends data to an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
    return AfdpMakeWork( AFD_OP_SEND_REQUEST, Irp, IrpSp, 
			 (PCHAR)&WithAddr, sizeof(WithAddr) );
}

NTSTATUS AfdDispSendTo( PIRP Irp, PIO_STACK_LOCATION IrpSp ) {
    return AfdpDispSendTo( Irp, IrpSp, TRUE );
}

NTSTATUS AfdDispRecvFrom(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Receives data from an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_RECVFROM Request;
  PFILE_REPLY_RECVFROM Reply;
  PAFDFCB FCB;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_RECVFROM)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_RECVFROM))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_RECVFROM)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_RECVFROM)Irp->AssociatedIrp.SystemBuffer;

    /* Since we're using bufferred I/O */
    Request->Buffers = (LPWSABUF)(Request + 1);

    Status = AfdpDispRecvFrom(
	Irp,
	FCB,
	Request,
	Reply);
  } else {
      Status = STATUS_INVALID_PARAMETER;
  }
  
  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

typedef enum {
  soRead,
  soWrite,
  soExcept
} SelectOperation;

DWORD AfdpDispSelectEx(
    LPFD_SET FDSet,
    SelectOperation Operation)
{
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PAFDFCB Current;
    KIRQL OldIrql;
    DWORD Count;
    ULONG i;
    
    AFD_DbgPrint(MAX_TRACE, ("FDSet (0x%X)  Operation (0x%X).\n",
			     FDSet, Operation));
    
    AFD_DbgPrint(MAX_TRACE, ("FDSet->fd_count (0x%X).\n", FDSet->fd_count));
    
    Count = 0;
    for (i = 0; i < FDSet->fd_count; i++) {
	AFD_DbgPrint(MAX_TRACE, ("Handle (0x%X).\n", FDSet->fd_array[i]));
	
	Status = ObReferenceObjectByHandle(
	    (HANDLE)FDSet->fd_array[i],
	    0,
	    IoFileObjectType,
	    KernelMode,
	    (PVOID*)&FileObject,
	    NULL);

	if (NT_SUCCESS(Status)) {
	    AFD_DbgPrint(MAX_TRACE, 
			 ("File object is at (0x%X).\n", FileObject));
	    
	    Current = FileObject->FsContext;
	    
	    switch (Operation) {
	    case soRead:
		KeAcquireSpinLock(&Current->ReceiveQueueLock, &OldIrql);
		if (!IsListEmpty(&Current->ReceiveQueue)) {
		    AFD_DbgPrint(MAX_TRACE, ("Socket is readable.\n"));
		    Count++;
		}
		KeReleaseSpinLock(&Current->ReceiveQueueLock, OldIrql);
		break;
	    case soWrite:
		Count++;
		break;
		
	    case soExcept:
		/* Signals sockets that an exceptional condition has occurred.
		 * AFAIK RST or FIN from the other end, or any condition that 
		 * makes RST emanate here, as well as a transition from
		 * ESTABLISHED to any other state based on inactivity.
		 */
		Count++;
		break;
	    }
	    
	    ObDereferenceObject(FileObject);
	}
    }
    
    return Count;
}

NTSTATUS AfdDispSelect(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Checks if sockets have data in the receive buffers
 *           and/or if client can send data
 *           Will complete the IRP when at least one socket is ready.
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_SELECT Request;
  PFILE_REPLY_SELECT Reply;
  DWORD SocketCount;
  PAFDFCB FCB;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_SELECT)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_SELECT))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_SELECT)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_SELECT)Irp->AssociatedIrp.SystemBuffer;

    AFD_DbgPrint(MAX_TRACE, ("R (0x%X)  W (0x%X) .\n",
      Request->ReadFDSet, Request->WriteFDSet));

    SocketCount = 0;

    if (Request->WriteFDSet) {
	AFD_DbgPrint(MAX_TRACE, ("Write.\n"));
	SocketCount += AfdpDispSelectEx(Request->WriteFDSet, soWrite);
    }
    if (Request->ReadFDSet) {
	AFD_DbgPrint(MAX_TRACE, ("Read.\n"));
	SocketCount += AfdpDispSelectEx(Request->ReadFDSet, soRead);
    }
    if (Request->ExceptFDSet) {
	AFD_DbgPrint(MAX_TRACE, ("Exception.\n"));
	SocketCount += AfdpDispSelectEx(Request->ExceptFDSet, soExcept);
    }

    AFD_DbgPrint(MAX_TRACE, ("Sockets selected (0x%X).\n", SocketCount));

    Reply->Status = NO_ERROR;
    Reply->SocketCount = SocketCount;
    Status = STATUS_SUCCESS;
  } else
    Status = STATUS_INVALID_PARAMETER;

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

NTSTATUS AfdDispEventSelect(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Associate an event object with one or more network events
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_EVENTSELECT Request;
  PFILE_REPLY_EVENTSELECT Reply;
  PKEVENT Event;
  PAFDFCB FCB;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_EVENTSELECT)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_EVENTSELECT))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_EVENTSELECT)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_EVENTSELECT)Irp->AssociatedIrp.SystemBuffer;

	/* FIXME: Need to ObDereferenceObject(FCB->EventObject) this somewhere */
	Status = ObReferenceObjectByHandle(
      Request->hEventObject,
      0,
      ExEventObjectType,
      UserMode,
      (PVOID*)&Event,
      NULL);
    if (NT_SUCCESS(Status)) {
      AFD_DbgPrint(MID_TRACE, ("AfdDispEventSelect() Event set (Event 0x%X).\n", Event));
      FCB->NetworkEvents.lNetworkEvents = Request->lNetworkEvents;
	  FCB->EventObject = Event;
      Reply->Status = NO_ERROR;
      Status = STATUS_SUCCESS;
    }
    else
    {
	AFD_DbgPrint(MID_TRACE, ("Bad event handle (0x%X).\n", Status));
	
	Reply->Status = WSAEINVAL;
	Status = STATUS_SUCCESS;
    }
  }
  else
  {
    Status = STATUS_INVALID_PARAMETER;
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

NTSTATUS AfdDispEnumNetworkEvents(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Reports network events
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_ENUMNETWORKEVENTS Request;
  PFILE_REPLY_ENUMNETWORKEVENTS Reply;
  HANDLE EventObject;
  PAFDFCB FCB;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_ENUMNETWORKEVENTS)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_ENUMNETWORKEVENTS))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_ENUMNETWORKEVENTS)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_ENUMNETWORKEVENTS)Irp->AssociatedIrp.SystemBuffer;

    EventObject = (HANDLE)Request->hEventObject;

    RtlCopyMemory(
      &Reply->NetworkEvents,
      &FCB->NetworkEvents,
      sizeof(WSANETWORKEVENTS));

    RtlZeroMemory(
      &FCB->NetworkEvents,
      sizeof(WSANETWORKEVENTS));

    if (EventObject != NULL) {
      ZwClearEvent(EventObject);
    }

    Reply->Status = NO_ERROR;
    Status = STATUS_SUCCESS;
  } else
    Status = STATUS_INVALID_PARAMETER;

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}


NTSTATUS AfdDispRecv(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Receives data from an address
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_RECV Request;
  PFILE_REPLY_RECV Reply;
  PAFDFCB FCB;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_RECV)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_RECV))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_RECV)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_RECV)Irp->AssociatedIrp.SystemBuffer;

    /* Since we're using bufferred I/O */
    Request->Buffers = (LPWSABUF)(Request + 1);

    Status = AfdpDispRecvFrom(
	Irp,
	FCB,
	(PFILE_REQUEST_RECVFROM)Request,
	(PFILE_REPLY_RECVFROM)Reply);
  } else {
      Status = STATUS_INVALID_PARAMETER;
  }
  
  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}


NTSTATUS AfdDispSend(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Sends data
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;

  Status = AfdpDispSendTo(Irp, IrpSp, FALSE);

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}


NTSTATUS
STDCALL
AfdDispCompleteConnect(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PAFDFCB FCB = IrpSp->FileObject->FsContext;
    RegisterFCBForWork( FCB );
    return STATUS_PENDING;
}

NTSTATUS AfdDispCompleteAccept(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context)
{
    PAFD_ACCEPT_REQUEST Request = (PAFD_ACCEPT_REQUEST)Context;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PAFDFCB FCB = IrpSp->FileObject->FsContext;
    Request->Valid = TRUE;
    Request->DeviceObject = DeviceObject;
    RegisterFCBForWork( FCB );
    return STATUS_PENDING;
}

NTSTATUS AfdDispConnect(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Connect to a remote peer
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_CONNECT Request;
    PFILE_REPLY_CONNECT Reply;
    /* AFD_CONNECT_REQUEST ConnectRequest = { 0 }; */
    PAFDFCB FCB;
    SOCKADDR_IN LocalAddress;
    
    AFD_DbgPrint(MIN_TRACE, ("\n"));
    
    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    /* Validate parameters */
    Status = STATUS_INVALID_PARAMETER;
    if ((InputBufferLength >= sizeof(FILE_REQUEST_CONNECT)) &&
	(OutputBufferLength >= sizeof(FILE_REPLY_CONNECT))) {
	FCB = IrpSp->FileObject->FsContext;
	
	Request = (PFILE_REQUEST_CONNECT)Irp->AssociatedIrp.SystemBuffer;
	Reply   = (PFILE_REPLY_CONNECT)Irp->AssociatedIrp.SystemBuffer;
	
	AFD_DbgPrint(MIN_TRACE, ("\n"));
	
	if (FCB->State == SOCKET_STATE_CONNECTED) {
	    Reply->Status = WSAEISCONN;
	} else {
	    /* We have an unbound socket so go ahead and create an address
	       file object and a connection endpoint and associate the two */
	    
	    AFD_DbgPrint(MIN_TRACE, ("\n"));
	    
	    /* FIXME: Get from client */
	    LocalAddress.sin_family = AF_INET;
	    LocalAddress.sin_port = 
		AfdpGetAvailablePort( &FCB->TdiDeviceName );
	    LocalAddress.sin_addr.S_un.S_addr = 0x0; /* Dynamically allocate */
	    
	    Status = TdiOpenAddressFile(
		&FCB->TdiDeviceName,
		(LPSOCKADDR)&LocalAddress,
		&FCB->TdiAddressObjectHandle,
		&FCB->TdiAddressObject);
	    
	    if (NT_SUCCESS(Status)) {
		AfdRegisterEventHandlers(FCB);
		FCB->State = SOCKET_STATE_BOUND;
	    }
	    
	    AFD_DbgPrint(MIN_TRACE, ("\n"));
	    
	    if (NT_SUCCESS(Status)) {
		Status = TdiOpenConnectionEndpointFile
		    (&FCB->TdiDeviceName,
		     &FCB->TdiConnectionObjectHandle,
		     &FCB->TdiConnectionObject);
	    }
	    
	    AFD_DbgPrint(MIN_TRACE, ("\n"));
	    
	    if (NT_SUCCESS(Status)) {
		Status = TdiAssociateAddressFile
		    (FCB->TdiAddressObjectHandle,
		     FCB->TdiConnectionObject);
	    }
	    
	    AFD_DbgPrint(MIN_TRACE, ("\n"));
	    
	    if (NT_SUCCESS(Status) && FCB->State != SOCKET_STATE_CONNECTED) {
		Status = TdiConnect
		    (FCB->TdiConnectionObject,
		     Request->name);
		
		AFD_DbgPrint(MIN_TRACE, ("FIXME: Status (0x%X).\n", Status));
	    } else {
		/* FIXME: Cleanup from TdiOpenConnectionEndpointFile */
		Status = STATUS_NO_MEMORY;
	    }
	}
    } 
    
    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
    
    return Status;
}


NTSTATUS AfdDispGetName(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Get socket name
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_GETNAME Request;
  PFILE_REPLY_GETNAME Reply;
  PAFDFCB FCB;
  PFILE_OBJECT FileObject;
  PMDL Mdl;
  PTDI_ADDRESS_INFO AddressInfoBuffer;
  ULONG AddressInfoSize;

  AFD_DbgPrint(MIN_TRACE, ("\n"));

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  Status = STATUS_INVALID_PARAMETER;
  if ((InputBufferLength >= sizeof(FILE_REQUEST_GETNAME)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_GETNAME))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_GETNAME)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_GETNAME)Irp->AssociatedIrp.SystemBuffer;

    AFD_DbgPrint(MIN_TRACE, ("\n"));

    if (Request->Peer) {
      if (FCB->State != SOCKET_STATE_CONNECTED) {
        Reply->Status = WSAENOTCONN;
        return STATUS_UNSUCCESSFUL;
      }
      FileObject = FCB->TdiConnectionObject;
    } else {
      FileObject = FCB->TdiAddressObject;
    }

    AddressInfoSize = sizeof(TDI_ADDRESS_INFO) + sizeof(TDI_ADDRESS_IP);
    AddressInfoBuffer = ExAllocatePool(NonPagedPool, AddressInfoSize);

    Mdl = IoAllocateMdl(
      AddressInfoBuffer,        /* Virtual address of buffer */
      AddressInfoSize,          /* Length of buffer */
      FALSE,                    /* Not secondary */
      FALSE,                    /* Don't charge quota */
      NULL);                    /* Don't use IRP */

    if (!Mdl) {
      ExFreePool(AddressInfoBuffer);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    Status = TdiQueryInformation(
      FileObject,
      TDI_QUERY_ADDRESS_INFO,
      Mdl);

    if (NT_SUCCESS(Status)) {
      Reply->Name.sa_family = AddressInfoBuffer->Address.Address[0].AddressType;

      RtlCopyMemory(
        Reply->Name.sa_data,
        AddressInfoBuffer->Address.Address[0].Address,
        sizeof(Reply->Name.sa_data));

      Reply->NameSize = sizeof(Reply->Name.sa_family) + 
                        AddressInfoBuffer->Address.Address[0].AddressLength;
    }

    IoFreeMdl(Mdl);
    ExFreePool(AddressInfoBuffer);
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

/* EOF */
