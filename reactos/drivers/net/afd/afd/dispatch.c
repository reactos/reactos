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

NTSTATUS AfdpDispRecv(
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
  PAFD_READ_REQUEST ReadRequest;
  NTSTATUS Status;
  KIRQL OldIrql;
  ULONG Count;

  KeAcquireSpinLock(&FCB->ReceiveQueueLock, &OldIrql);
  if (IsListEmpty(&FCB->ReceiveQueue)) {
    KeReleaseSpinLock(&FCB->ReceiveQueueLock, OldIrql);

    /* Queue a read request and return STATUS_PENDING */

    AFD_DbgPrint(MAX_TRACE, ("Queueing read request.\n"));

    /*ReadRequest = (PAFD_READ_REQUEST)ExAllocateFromNPagedLookasideList(
        &ReadRequestLookasideList);*/
    ReadRequest = (PAFD_READ_REQUEST)ExAllocatePool(
      NonPagedPool,
      sizeof(AFD_READ_REQUEST));
    if (ReadRequest) {
      ReadRequest->Irp = Irp;
      ReadRequest->RecvFromRequest = Request;
      ReadRequest->RecvFromReply = Reply;

      ExInterlockedInsertTailList(
        &FCB->ReadRequestQueue,
        &ReadRequest->ListEntry,
        &FCB->ReadRequestQueueLock);
      Status = STATUS_PENDING;
    } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  } else {
    AFD_DbgPrint(MAX_TRACE, ("Satisfying read request.\n"));

    /* Satisfy the request at once */
    Status = FillWSABuffers(
      FCB,
      Request->Buffers,
      Request->BufferCount,
      &Count);
    KeReleaseSpinLock(&FCB->ReceiveQueueLock, OldIrql);

    Reply->NumberOfBytesRecvd = Count;
    Reply->Status = NO_ERROR;

    AFD_DbgPrint(MAX_TRACE, ("Bytes received (0x%X).\n", Count));
  }

  return Status;
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
  INT Errno;

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


NTSTATUS AfdDispListen(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Starts listening for connections
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

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  Status = STATUS_INVALID_PARAMETER;
  if ((InputBufferLength >= sizeof(FILE_REQUEST_LISTEN)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_LISTEN))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_LISTEN)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_LISTEN)Irp->AssociatedIrp.SystemBuffer;

    if (FCB->State == SOCKET_STATE_BOUND) {

      /* We have a bound socket so go ahead and create a connection endpoint
         and associate it with the address file object */

      Status = TdiOpenConnectionEndpointFile(
        &FCB->TdiDeviceName,
        &FCB->TdiConnectionObjectHandle,
        &FCB->TdiConnectionObject);

      if (NT_SUCCESS(Status)) {
        Status = TdiAssociateAddressFile(
          FCB->TdiAddressObjectHandle,
          FCB->TdiConnectionObject);
      }

      if (NT_SUCCESS(Status)) {
        Reply->Status = NO_ERROR;
      } else {
        Reply->Status = WSAEINVAL;
      }
    } else if (FCB->State == SOCKET_STATE_CONNECTED) {
      Reply->Status = WSAEISCONN;
    } else {
      Reply->Status = WSAEINVAL;
    }
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}


NTSTATUS AfdDispSendTo(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: Sends data to an address
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
  PFILE_REQUEST_SENDTO Request;
  PFILE_REPLY_SENDTO Reply;
  PAFDFCB FCB;
  PVOID SystemVirtualAddress;
  PVOID DataBufferAddress;
  ULONG BufferSize;
  ULONG BytesCopied;
  PMDL Mdl;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_SENDTO)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_SENDTO))) {

    AFD_DbgPrint(MAX_TRACE, ("FileObject at (0x%X).\n", IrpSp->FileObject));
    AFD_DbgPrint(MAX_TRACE, ("FCB at (0x%X).\n", IrpSp->FileObject->FsContext));
    AFD_DbgPrint(MAX_TRACE, ("CCB at (0x%X).\n", IrpSp->FileObject->FsContext2));

    FCB = IrpSp->FileObject->FsContext;
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
      if (!SystemVirtualAddress) {
          return STATUS_INSUFFICIENT_RESOURCES;
      }

      /* FIXME: Should we handle special cases here? */
      if ((FCB->SocketType == SOCK_RAW) && (FCB->AddressFamily == AF_INET)) {
        DataBufferAddress = SystemVirtualAddress + sizeof(IPv4_HEADER);

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
        return Status;
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
      return STATUS_INSUFFICIENT_RESOURCES;
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

    Status = TdiSendDatagram(FCB->TdiAddressObject,
        &Request->To,
        Mdl,
        BufferSize);

    /* FIXME: Assumes synchronous operation */
#if 0
    MmUnlockPages(Mdl);
#endif

    IoFreeMdl(Mdl);

    if (BufferSize != 0) {
        ExFreePool(SystemVirtualAddress);
    }

    Reply->NumberOfBytesSent = BufferSize;
    Reply->Status = NO_ERROR;
  } else
    Status = STATUS_INVALID_PARAMETER;

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
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
  DWORD NumberOfBytesRecvd;
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

    Status = AfdpDispRecv(
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
      AFD_DbgPrint(MAX_TRACE, ("File object is at (0x%X).\n", FileObject));

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
        /* FIXME: How can we check for writability? */
        Count++;
        break;
      case soExcept:
        /* FIXME: What is this? */
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

    AFD_DbgPrint(MAX_TRACE, ("R (0x%X)  W (0x%X).\n",
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
  PAFDFCB FCB;
  ULONG i;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_EVENTSELECT)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_EVENTSELECT))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_EVENTSELECT)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_EVENTSELECT)Irp->AssociatedIrp.SystemBuffer;

    FCB->NetworkEvents.lNetworkEvents = Request->lNetworkEvents;
    for (i = 0; i < FD_MAX_EVENTS; i++) {
      if ((Request->lNetworkEvents & (1 << i)) > 0) {
        FCB->EventObjects[i] = Request->hEventObject;
      } else {
        /* The effect of any previous call to this function is cancelled */
        FCB->EventObjects[i] = (WSAEVENT)0;
      }
    }

    Reply->Status = NO_ERROR;
    Status = STATUS_SUCCESS;
  } else
    Status = STATUS_INVALID_PARAMETER;

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
#if 0
  NTSTATUS Status;
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_RECV Request;
  PFILE_REPLY_RECV Reply;
  DWORD NumberOfBytesRecvd;
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

    Status = AfdpDispRecv(
      Irp,
      FCB,
      Request->Buffers,
      Request->BufferCount,
      &NumberOfBytesRecvd);
    Reply->NumberOfBytesRecvd = NumberOfBytesRecvd;
    Reply->Status = NO_ERROR;
  } else {
    Status = STATUS_INVALID_PARAMETER;
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
#else
  return STATUS_SUCCESS;
#endif
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
  UINT InputBufferLength;
  UINT OutputBufferLength;
  PFILE_REQUEST_SEND Request;
  PFILE_REPLY_SEND Reply;
  PAFDFCB FCB;

  InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

  /* Validate parameters */
  if ((InputBufferLength >= sizeof(FILE_REQUEST_SEND)) &&
    (OutputBufferLength >= sizeof(FILE_REPLY_SEND))) {
    FCB = IrpSp->FileObject->FsContext;

    Request = (PFILE_REQUEST_SEND)Irp->AssociatedIrp.SystemBuffer;
    Reply   = (PFILE_REPLY_SEND)Irp->AssociatedIrp.SystemBuffer;

    Reply->NumberOfBytesSent = 0;
    Reply->Status = NO_ERROR;
  } else
    Status = STATUS_INVALID_PARAMETER;

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
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

    if (FCB->State == SOCKET_STATE_BOUND) {
      Reply->Status = WSAEADDRINUSE;
    } else if (FCB->State == SOCKET_STATE_CONNECTED) {
      Reply->Status = WSAEISCONN;
    } else {
      /* We have an unbound socket so go ahead and create an address
         file object and a connection endpoint and associate the two */

      AFD_DbgPrint(MIN_TRACE, ("\n"));

      /* FIXME: Get from client */
      LocalAddress.sin_family = AF_INET;
      LocalAddress.sin_port = 1700;
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
        Status = TdiOpenConnectionEndpointFile(
          &FCB->TdiDeviceName,
          &FCB->TdiConnectionObjectHandle,
          &FCB->TdiConnectionObject);
      }

      AFD_DbgPrint(MIN_TRACE, ("\n"));

      if (NT_SUCCESS(Status)) {
        Status = TdiAssociateAddressFile(
          FCB->TdiAddressObjectHandle,
          FCB->TdiConnectionObject);
      }

      AFD_DbgPrint(MIN_TRACE, ("\n"));

      if (NT_SUCCESS(Status)) {
        /* Now attempt to connect to the remote peer */
        Status = TdiConnect(
          FCB->TdiConnectionObject,
          Request->name);
      }

      AFD_DbgPrint(MIN_TRACE, ("\n"));

      if (NT_SUCCESS(Status)) {
        FCB->State = SOCKET_STATE_CONNECTED;
        Reply->Status = NO_ERROR;
      } else {
        Reply->Status = WSAEINVAL;
      }
    }
  }

  AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

  return Status;
}

/* EOF */
