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
			     CriticalWorkQueue,
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

    AFD_DbgPrint(MAX_TRACE,
		 ("Added work item: Request: %x, Opcode %d, Irp %x, PayloadSize %d\n",
		  Request, Opcode, Irp, PayloadSize));

    Request->Opcode      = Opcode;
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
    PFILE_REPLY_RECVFROM Reply =
	(PFILE_REPLY_RECVFROM)WorkRequest->Irp->AssociatedIrp.SystemBuffer;
    Reply->Status = WSAENOTSOCK;
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
    KIRQL RROldIrql;
    PFILE_REQUEST_RECVFROM Request;
    PFILE_REPLY_RECVFROM Reply;
    NTSTATUS Status = STATUS_PENDING;
    ULONG Count = 0, i;
    BOOL Complete = FALSE;
    LPWSABUF Buffers;

    Request = 
	(PFILE_REQUEST_RECVFROM)WorkRequest->Irp->AssociatedIrp.SystemBuffer;
    Reply = 
	(PFILE_REPLY_RECVFROM)WorkRequest->Irp->AssociatedIrp.SystemBuffer;
    Buffers = (LPWSABUF)(Request + 1);

    AFD_DbgPrint(MAX_TRACE, ("Locking pages\n"));
    /* Lock the regions */
    for( i = 0; i < Request->BufferCount; i++ ) {
	MmProbeAndLockPages( (PMDL)Buffers[i].buf, KernelMode, IoWriteAccess );
    }
    AFD_DbgPrint(MAX_TRACE, ("Unlocking pages\n"));

    KeAcquireSpinLock(&FCB->ReceiveQueueLock, &RROldIrql);
    
    while ( !IsListEmpty(&FCB->ReceiveQueue) ) {
	AFD_DbgPrint(MAX_TRACE, ("Satisfying read request.\n"));
	
	Status = FillWSABuffers
	    (FCB,
	     Buffers,
	     Request->BufferCount,
	     &Count);
	
	AFD_DbgPrint(MAX_TRACE,
		     ("FillMdls: %x %d\n", Status, Count));
	
	if( Status == STATUS_SUCCESS && Count > 0 ) {
	    Reply->NumberOfBytesRecvd = Count;
	    Reply->Status = NO_ERROR;
	    WorkRequest->Irp->IoStatus.Status = Status;
	    Complete = TRUE;
	} else {
	    Reply->NumberOfBytesRecvd = Count;
	    Reply->Status = Status;
	    WorkRequest->Irp->IoStatus.Information = sizeof(*Reply);
	    WorkRequest->Irp->IoStatus.Status = Status;
	}
    }
    
    KeReleaseSpinLock(&FCB->ReceiveQueueLock, RROldIrql);

    AFD_DbgPrint(MAX_TRACE, ("Unlocking pages\n"));

    for( i = 0; i < Request->BufferCount; i++ ) {
	MmUnlockPages( (PMDL)Buffers[i].buf );
	IoFreeMdl( (PMDL)Buffers[i].buf );
    }

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
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    PFILE_REQUEST_SENDTO Request;
    PFILE_REPLY_SENDTO Reply;
    PVOID SystemVirtualAddress;
    PVOID DataBufferAddress;
    LPWSABUF Buffers;
    ULONG BufferSize;
    ULONG BytesCopied;
    ULONG i;
    PMDL Mdl;
    
    Irp = WorkRequest->Irp;
    IrpSp = WorkRequest->IrpSp;
    WithAddr = *((BOOL *)WorkRequest->Payload);
    
    InputBufferLength  = 
	IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = 
	IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
    /* Validate parameters */
    AFD_DbgPrint(MAX_TRACE, ("FileObject at (0x%X).\n", IrpSp->FileObject));
    AFD_DbgPrint(MAX_TRACE, ("FCB at (0x%X).\n", IrpSp->FileObject->FsContext));
    AFD_DbgPrint(MAX_TRACE, ("CCB at (0x%X).\n", IrpSp->FileObject->FsContext2));

    Request = (PFILE_REQUEST_SENDTO)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_SENDTO)Irp->AssociatedIrp.SystemBuffer;
    
    AFD_DbgPrint(MAX_TRACE, ("Got Request->Buffers @ %x\n", 
			     Request->Buffers));

    /* Since we're using bufferred I/O */
    Buffers = (LPWSABUF)(Request + 1);
    BufferSize = WSABufferSize(Buffers, Request->BufferCount);
    
    /* FIXME: Should we handle special cases here? */
    if ((FCB->SocketType == SOCK_RAW) && (FCB->AddressFamily == AF_INET)) {
	BufferSize += sizeof(IPv4_HEADER);
    }
        
    if (BufferSize != 0) {
	AFD_DbgPrint(MAX_TRACE, ("Allocating %d bytes for send buffer.\n", BufferSize));
	SystemVirtualAddress = ExAllocatePool(NonPagedPool, BufferSize);
	if (!SystemVirtualAddress) {
	    for( i = 0; i < Request->BufferCount; i++ ) {
		MmUnlockPages( (PMDL)Buffers[i].buf );
		IoFreeMdl( (PMDL)Buffers[i].buf );
	    }
	    return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	/* FIXME: Should we handle special cases here? */
	if ((FCB->SocketType == SOCK_RAW) && (FCB->AddressFamily == AF_INET) && 
	    WithAddr) {
	    DataBufferAddress = ((PCHAR)SystemVirtualAddress) + sizeof(IPv4_HEADER);
	    
	    /* FIXME: Should TCP/IP driver assign source address for raw 
	     * sockets? */
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

    for( i = 0; i < Request->BufferCount; i++ ) {
	MmUnlockPages( (PMDL)Buffers[i].buf );
	IoFreeMdl( (PMDL)Buffers[i].buf );
    }
    
    /* Note: By this point, we should have unlocked the mdls to the user pages
     * we've been keeping 
     * Any branches above that return early should loop over the buffers array
     * and unlock the mdls. */

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

    MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
    /* MmBuildMdlForNonPagedPool(Mdl); */
    
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
	
	AFD_DbgPrint(MID_TRACE,("Binding socket on first send.\n"));
	
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
    WorkRequest->Irp->IoStatus.Information = sizeof(*Reply);
    Irp->IoStatus.Status = Status;

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
    KIRQL OldIrql;
    PAFD_WORK_REQUEST WorkRequest;
    PLIST_ENTRY ListEntry = FCB->WorkQueue.Flink;

    while( ListEntry != &FCB->WorkQueue ) {
	WorkRequest = CONTAINING_RECORD(ListEntry,
					AFD_WORK_REQUEST, 
					ListEntry);

	AFD_DbgPrint(MID_TRACE, ("With request %x...\n", WorkRequest));
	
	AFD_DbgPrint(MAX_TRACE,
		     ("Got work item: Request: %x, Opcode %d, Irp %x, PayloadSize %d\n",
		      WorkRequest,
		      WorkRequest->Opcode, 
		      WorkRequest->Irp, 
		      WorkRequest->PayloadSize));
	
	if( WorkPerform[WorkRequest->Opcode]( FCB, WorkRequest ) ) {
	    AFD_DbgPrint(MID_TRACE, ("Request should be removed.\n"));
	    IoCompleteRequest( WorkRequest->Irp, IO_NETWORK_INCREMENT );
	    AFD_DbgPrint(MID_TRACE, ("Locking the work queue.\n"));
	    KeAcquireSpinLock( &FCB->WorkQueueLock, &OldIrql );
	    AFD_DbgPrint(MID_TRACE, ("Removing.\n"));
	    RemoveEntryList( ListEntry );
	    AFD_DbgPrint(MID_TRACE, ("Unlocking.\n"));
	    KeReleaseSpinLock( &FCB->WorkQueueLock, OldIrql );
	    AFD_DbgPrint(MID_TRACE, ("Freeing.\n"));
	    ExFreePool( WorkRequest );
	    ListEntry = &FCB->WorkQueue;
	} else {
	    AFD_DbgPrint(MID_TRACE, ("Request should be left.\n"));
	    KeAcquireSpinLock( &FCB->WorkQueueLock, &OldIrql );
	    ListEntry = ListEntry->Flink;
	    KeReleaseSpinLock( &FCB->WorkQueueLock, OldIrql );
	}
    }
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
    FCB->WorkItem = 0;
    
    AFD_DbgPrint(MID_TRACE, ("... Done\n"));
}
