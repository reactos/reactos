#include <ntddk.h>
#include <afd.h>

extern VOID AfdpWork( PDEVICE_OBJECT DriverObject, PVOID Dummy );

NTSTATUS InitWorker() {
    return STATUS_SUCCESS;
}

VOID ShutdownWorker() {
}

VOID RegisterFCBForWork( PAFDFCB FCB ) {
    if( !FCB->WorkItem ) {
	FCB->WorkItem = IoAllocateWorkItem( FCB->DeviceObject );
	if( FCB->WorkItem ) 
	    IoQueueWorkItem( FCB->WorkItem,
			     AfdpWork,
			     DelayedWorkQueue,
			     FCB );
    }
}

NTSTATUS AfdpMakeWork( UINT Opcode, 
		       PIRP Irp, 
		       PIO_STACK_LOCATION IrpSp,
		       PCHAR Payload,
		       UINT PayloadSize ) {
    PAFDFCB FCB;
    PAFD_WORK_REQUEST Request = 
	ExAllocatePool( NonPagedPool, sizeof(AFD_WORK_REQUEST) + PayloadSize );
    
    if( !Request )
	return STATUS_NO_MEMORY;
    
    FCB = IrpSp->FileObject->FsContext;
    
    Request->Opcode      = AFD_OP_ACCEPT_REQUEST;
    Request->Irp         = Irp;
    Request->IrpSp       = IrpSp;
    if( PayloadSize ) RtlCopyMemory(Request->Payload, Payload, PayloadSize);
    Request->PayloadSize = PayloadSize;

    ExInterlockedInsertTailList( &FCB->WorkQueue, 
				 &Request->ListEntry,
				 &FCB->WorkQueueLock );

    RegisterFCBForWork( FCB );

    return STATUS_PENDING;    
}    

typedef VOID (*WORK_CANCEL_FUN)( PAFD_WORK_REQUEST );
typedef BOOL (*WORK_PERFORM_FUN)( PAFDFCB, PAFD_WORK_REQUEST );

VOID WorkCancelAccept( PAFD_WORK_REQUEST WorkRequest ) {
    PFILE_REPLY_CONNECT Reply =
	(PFILE_REPLY_CONNECT)WorkRequest->Irp->AssociatedIrp.SystemBuffer;
    Reply->Status = WSAECONNABORTED;
    WorkRequest->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
}

VOID WorkCancelSend( PAFD_WORK_REQUEST WorkRequest ) {
    PFILE_REPLY_SENDTO Reply =
	(PFILE_REPLY_SENDTO)WorkRequest->Irp->AssociatedIrp.SystemBuffer;
    Reply->Status = WSAECONNABORTED;
    WorkRequest->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
}

VOID WorkCancelRecv( PAFD_WORK_REQUEST WorkRequest ) {
    PAFD_READ_REQUEST ReadRequest = (PAFD_READ_REQUEST)WorkRequest->Payload;
    ReadRequest->Recv.Reply->Status = WSAENOTSOCK;
    WorkRequest->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
}

VOID WorkCancelConnect( PAFD_WORK_REQUEST WorkRequest ) {
    WorkRequest->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
}

WORK_CANCEL_FUN WorkCancel[] = { WorkCancelAccept,
				 WorkCancelSend,
				 WorkCancelRecv,
				 WorkCancelConnect };

VOID CancelWork( PAFDFCB FCB ) {
    PLIST_ENTRY ListEntry;
    PAFD_WORK_REQUEST Request;
    KIRQL OldIrql;

    /* Clear the work queue */
    KeAcquireSpinLock( &FCB->WorkQueueLock, &OldIrql );
    while( !IsListEmpty( &FCB->WorkQueue ) ) {
	ListEntry = RemoveHeadList( &FCB->WorkQueue );
	Request = CONTAINING_RECORD( ListEntry, AFD_WORK_REQUEST,
				     ListEntry );
	WorkCancel[Request->Opcode]( Request );
	IoCompleteRequest( Request->Irp, IO_NETWORK_INCREMENT );
    }
    KeReleaseSpinLock( &FCB->WorkQueueLock, OldIrql );
}

BOOL AfdpTryToSatisfyAcceptRequest( PAFDFCB FCB,
				    PAFD_WORK_REQUEST WorkRequest ) {
    PIRP Irp = WorkRequest->Irp;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PFILE_OBJECT FileObject;
    PFILE_REQUEST_ACCEPT Request;
    PFILE_REPLY_ACCEPT Reply;
    PAFD_ACCEPT_REQUEST AcceptRequest = 
	(PAFD_ACCEPT_REQUEST)WorkRequest->Payload;

    Request = (PFILE_REQUEST_ACCEPT)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_ACCEPT)Irp->AssociatedIrp.SystemBuffer;

    if( AcceptRequest->Valid ) {
	Status = ObReferenceObjectByHandle( (HANDLE)Request->Socket,
					    0,
					    NULL,
					    (KPROCESSOR_MODE)
					    KeGetPreviousMode(),
					    (PVOID)&FileObject,
					    NULL );

	WorkRequest->Irp->IoStatus.Status = Status;

	if( NT_SUCCESS(Status) ) {
	    Reply->Socket = Request->Socket;
	}
    } else {
	Reply->Socket = INVALID_SOCKET;
    }

    return TRUE;
}

BOOL AfdpTryToSatisfyRecvRequest( PAFDFCB FCB, 
				  PAFD_WORK_REQUEST WorkRequest ) {
    PAFD_READ_REQUEST ReadRequest = (PAFD_READ_REQUEST)WorkRequest->Payload;
    KIRQL RROldIrql;
    NTSTATUS Status = STATUS_PENDING;
    ULONG Count = 0;
    BOOL Complete = FALSE;

    KeAcquireSpinLock(&FCB->ReceiveQueueLock, &RROldIrql);
    
    while ( !IsListEmpty(&FCB->ReceiveQueue) ) {
	AFD_DbgPrint(MAX_TRACE, ("Satisfying read request.\n"));
	
	Status = FillWSABuffers
	    (FCB,
	     ReadRequest->Recv.Request->Buffers,
	     ReadRequest->Recv.Request->BufferCount,
	     &Count);
	
	AFD_DbgPrint(MAX_TRACE,
		     ("FillWSABuffers: %x %d\n", Status, Count));
	
	if( Status == STATUS_SUCCESS && Count > 0 ) {
	    ReadRequest->Recv.Reply->NumberOfBytesRecvd = Count;
	    ReadRequest->Recv.Reply->Status = NO_ERROR;
	    WorkRequest->Irp->IoStatus.Status = Status;
	    Complete = TRUE;
	} else {
	    ReadRequest->Recv.Reply->NumberOfBytesRecvd = Count;
	    ReadRequest->Recv.Reply->Status = Status;
	    WorkRequest->Irp->IoStatus.Information = 
		sizeof(*ReadRequest->Recv.Reply);
	    WorkRequest->Irp->IoStatus.Status = Status;
	}
    }
    
    KeReleaseSpinLock(&FCB->ReceiveQueueLock, RROldIrql);

    AFD_DbgPrint(MAX_TRACE, ("Leaving.\n"));
    
    return Complete;
}

/* XXX Future: Leave send requests hanging when we don't have more buffer
 * space available and the socket is blocking.  This buffer space should
 * be measured in afd according to what I've read.  We don't fully comply
 * yet. */

BOOL AfdpTryToSatisfySendRequest( PAFDFCB FCB, 
				  PAFD_WORK_REQUEST WorkRequest ) {
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    BOOL WithAddr;
    NTSTATUS Status;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_SENDTO Request;
    PFILE_REPLY_SENDTO Reply;
    PVOID SystemVirtualAddress;
    PVOID DataBufferAddress;
    ULONG BufferSize;
    ULONG BytesCopied;
    PMDL Mdl;
    
    Irp = WorkRequest->Irp;
    IrpSp = WorkRequest->IrpSp;
    WithAddr = *((BOOL *)WorkRequest->Payload);
    
    InputBufferLength  = 
	IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = 
	IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    /* Validate parameters */
    if ((InputBufferLength >= sizeof(FILE_REQUEST_SENDTO)) &&
	(OutputBufferLength >= sizeof(FILE_REPLY_SENDTO))) {
	
	AFD_DbgPrint(MAX_TRACE, ("FileObject at (0x%X).\n", IrpSp->FileObject));
	AFD_DbgPrint(MAX_TRACE, ("FCB at (0x%X).\n", IrpSp->FileObject->FsContext));
	AFD_DbgPrint(MAX_TRACE, ("CCB at (0x%X).\n", IrpSp->FileObject->FsContext2));
	
	Request = (PFILE_REQUEST_SENDTO)Irp->AssociatedIrp.SystemBuffer;
	Reply   = (PFILE_REPLY_SENDTO)Irp->AssociatedIrp.SystemBuffer;
	
	/* Since we're using bufferred I/O */
	Request->Buffers = (LPWSABUF)(Request + 1);
	BufferSize = WSABufferSize(Request->Buffers, Request->BufferCount);
	
	/* FIXME: Should we handle special cases here? */
	if ((FCB->SocketType == SOCK_RAW) && (FCB->AddressFamily == AF_INET)) {
	    BufferSize += sizeof(IPv4_HEADER);
	}
	
	
	if (BufferSize != 0) {
	    AFD_DbgPrint(MAX_TRACE, ("Allocating %d bytes for send buffer.\n", BufferSize));
	    SystemVirtualAddress = ExAllocatePool(NonPagedPool, BufferSize);
	    if (!SystemVirtualAddress)
		return STATUS_INSUFFICIENT_RESOURCES;
	    
	    /* FIXME: Should we handle special cases here? */
	    if ((FCB->SocketType == SOCK_RAW) && (FCB->AddressFamily == AF_INET) && 
		WithAddr) {
		DataBufferAddress = ((PCHAR)SystemVirtualAddress) + sizeof(IPv4_HEADER);
		
		/* FIXME: Should TCP/IP driver assign source address for raw sockets? */
		((PSOCKADDR_IN)&FCB->SocketName)->sin_addr.S_un.S_addr = 0x0100007F;
		
		BuildIPv4Header(
		    (PIPv4_HEADER)SystemVirtualAddress,
		    BufferSize,
		    FCB->Protocol,
		    &FCB->SocketName,
		    &Request->To);
	    } else {
		DataBufferAddress = SystemVirtualAddress;
	    }
	    
	    Status = MergeWSABuffers(
		Request->Buffers,
		Request->BufferCount,
		DataBufferAddress,
		BufferSize,
		&BytesCopied);
	    if (!NT_SUCCESS(Status)) {
		AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
		Irp->IoStatus.Status = WSAENOBUFS;
		return TRUE;
	    }
	} else {
	    SystemVirtualAddress = NULL;
	    BytesCopied = 0;
	}
	
	Mdl = IoAllocateMdl(
	    SystemVirtualAddress,   /* Virtual address of buffer */
	    BufferSize,             /* Length of buffer */
	    FALSE,                  /* Not secondary */
	    FALSE,                  /* Don't charge quota */
	    NULL);                  /* Don't use IRP */
	if (!Mdl) {
	    ExFreePool(SystemVirtualAddress);
	    Irp->IoStatus.Status = WSAENOBUFS;
	    return TRUE;
	}
	
	MmBuildMdlForNonPagedPool(Mdl);
	
	AFD_DbgPrint(MAX_TRACE, ("System virtual address is (0x%X).\n", SystemVirtualAddress));
	AFD_DbgPrint(MAX_TRACE, ("MDL for data buffer is at (0x%X).\n", Mdl));
	
	AFD_DbgPrint(MAX_TRACE, ("AFD.SYS: NDIS data buffer is at (0x%X).\n", Mdl));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer MdlFlags is (0x%X).\n", Mdl->MdlFlags));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer Next is at (0x%X).\n", Mdl->Next));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer Size is (0x%X).\n", Mdl->Size));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer MappedSystemVa is (0x%X).\n", Mdl->MappedSystemVa));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer StartVa is (0x%X).\n", Mdl->StartVa));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer ByteCount is (0x%X).\n", Mdl->ByteCount));
	AFD_DbgPrint(MAX_TRACE, ("NDIS data buffer ByteOffset is (0x%X).\n", Mdl->ByteOffset));
	
#if 0
#ifdef _MSC_VER
	try {
#endif
	    MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
#ifdef _MSC_VER
	} except(EXCEPTION_EXECUTE_HANDLER) {
	    AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	    IoFreeMdl(Mdl);
	    if (BufferSize != 0) {
		ExFreePool(SystemVirtualAddress);
	    }
	    return STATUS_UNSUCCESSFUL;
	}
#endif
#endif
	
	if (!FCB->TdiAddressObject) {
	    struct sockaddr_in BindName;
	    
	    RtlZeroMemory(&BindName,sizeof(BindName));
	    BindName.sin_family = AF_INET;
	    
	    Status = TdiOpenAddressFile
		(&FCB->TdiDeviceName,
		 (SOCKADDR *)&BindName,
		 &FCB->TdiAddressObjectHandle,
		 &FCB->TdiAddressObject);
	    
	    if (NT_SUCCESS(Status)) {
		AfdRegisterEventHandlers(FCB);
		FCB->State = SOCKET_STATE_BOUND;
		Reply->Status = NO_ERROR;
	    } else {
		//FIXME: WSAEADDRNOTAVAIL
		Reply->Status = WSAEINVAL;
		MmUnlockPages(Mdl);
		IoFreeMdl(Mdl);
		return Status;
	    }
	}
	
	if( WithAddr ) {
	    Status = TdiSendDatagram(FCB->TdiAddressObject,
				     &Request->To,
				     Mdl,
				     BufferSize);
	} else {
	    Status = TdiSend(FCB->TdiAddressObject,
			     Mdl,
			     BufferSize);
	    
	    /* FIXME: Assumes synchronous operation */
#if 0
	    MmUnlockPages(Mdl);
#endif
	}
	
	IoFreeMdl(Mdl);
	
	if (BufferSize != 0) {
	    ExFreePool(SystemVirtualAddress);
	}
	
	AfdpKickFCB( FCB, FD_WRITE_BIT, NO_ERROR );
	Reply->NumberOfBytesSent = BufferSize;
	Reply->Status = NO_ERROR;
	Irp->IoStatus.Status = Status;
    }
    
    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return TRUE;
}

BOOL AfdpTryToSatisfyConnectRequest( PAFDFCB FCB,
				     PAFD_WORK_REQUEST Request ) {
    PAFD_CONNECT_REQUEST ConnectRequest = 
    (PAFD_CONNECT_REQUEST)Request->Payload;

    if( FCB->State == SOCKET_STATE_CONNECTED ) {
	AFD_DbgPrint(MAX_TRACE, 
		     ("1a - Completed ConnectRequest at (0x%X).\n", 
		      ConnectRequest));
	AFD_DbgPrint(MAX_TRACE, 
		     ("1b - Status (0x%X).\n", ConnectRequest->Iosb.Status));
	
	if (NT_SUCCESS(ConnectRequest->Iosb.Status)) {
	    AFD_DbgPrint(MAX_TRACE, 
			 ("2 - Completed ConnectRequest at (0x%X).\n", 
			  ConnectRequest));
	    AfdpKickFCB( FCB, FD_CONNECT_BIT, NO_ERROR );
	} else {
	    AfdpKickFCB( FCB, FD_CONNECT_BIT, WSAECONNREFUSED ); 
	}
	
	ExFreePool(ConnectRequest->RequestConnectionInfo);
	ConnectRequest->RequestConnectionInfo = NULL;
	return TRUE;
    }
    return FALSE;
}

WORK_PERFORM_FUN WorkPerform[] = { AfdpTryToSatisfyAcceptRequest,
				   AfdpTryToSatisfySendRequest,
				   AfdpTryToSatisfyRecvRequest,
				   AfdpTryToSatisfyConnectRequest };

VOID AfdpWorkOnFcb( PAFDFCB FCB ) {
    PAFD_WORK_REQUEST WorkRequest;
    PLIST_ENTRY ListEntry = 0, NextListEntry = 0;
    KIRQL OldIrql;

    KeAcquireSpinLock( &FCB->WorkQueueLock, &OldIrql );
    ListEntry = FCB->WorkQueue.Flink;
    
    while( ListEntry != &FCB->WorkQueue ) {
	AFD_DbgPrint(MID_TRACE, ("Got item %x...\n", ListEntry));
	WorkRequest = CONTAINING_RECORD(&FCB->WorkQueue,
					AFD_WORK_REQUEST, 
					ListEntry);
	AFD_DbgPrint(MID_TRACE, ("With request %x...\n", WorkRequest));
	NextListEntry = ListEntry->Flink;
	KeReleaseSpinLock( &FCB->WorkQueueLock, OldIrql );
    
	if( WorkPerform[WorkRequest->Opcode]( FCB, WorkRequest ) ) {
	    IoCompleteRequest( WorkRequest->Irp, IO_NETWORK_INCREMENT );
	    RemoveEntryList( ListEntry );
	    ListEntry = NextListEntry;
	}
	KeAcquireSpinLock( &FCB->WorkQueueLock, &OldIrql );
    }
    KeReleaseSpinLock( &FCB->WorkQueueLock, OldIrql );
}

VOID AfdpWork( PDEVICE_OBJECT DriverObject, PVOID Data ) {
    PAFDFCB FCB = Data;
    NTSTATUS Status;
    
    AFD_DbgPrint(MID_TRACE, ("Working...\n"));
    
    if( FCB->ListenRequest.Valid ) { 
	Status = TdiListen(&FCB->ListenRequest, 
			   AfdDispCompleteListen, 
			   FCB);
	
	AfdpKickFCB( FCB, FD_ACCEPT_BIT, 
		     FCB->ListenRequest.Result );
	
	FCB->ListenRequest.Valid = FALSE;
    }
    
    AfdpWorkOnFcb( FCB );

    
    AFD_DbgPrint(MID_TRACE, ("... Done\n"));
}
