/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/fileobjs.c
 * PURPOSE:     Routines for handling file objects
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


/* List of all address file objects managed by this driver */
LIST_ENTRY AddressFileListHead;
KSPIN_LOCK AddressFileListLock;

/* List of all connection endpoint file objects managed by this driver */
LIST_ENTRY ConnectionEndpointListHead;
KSPIN_LOCK ConnectionEndpointListLock;

/*
 * FUNCTION: Searches through address file entries to find the first match
 * ARGUMENTS:
 *     Address       = IP address
 *     Port          = Port number
 *     Protocol      = Protocol number
 *     SearchContext = Pointer to search context
 * RETURNS:
 *     Pointer to address file, NULL if none was found
 */
PADDRESS_FILE AddrSearchFirst(
    PIP_ADDRESS Address,
    USHORT Port,
    USHORT Protocol,
    PAF_SEARCH SearchContext)
{
    SearchContext->Address  = Address;
    SearchContext->Port     = Port;
    SearchContext->Next     = AddressFileListHead.Flink;
    SearchContext->Protocol = Protocol;

    return AddrSearchNext(SearchContext);
}

BOOLEAN AddrIsBroadcast(
    PIP_ADDRESS PossibleMatch,
    PIP_ADDRESS TargetAddress ) {
    IF_LIST_ITER(IF);

    ForEachInterface(IF) {
        if( AddrIsEqual( &IF->Unicast, PossibleMatch ) &&
            AddrIsEqual( &IF->Broadcast, TargetAddress ) )
            return TRUE;
    } EndFor(IF);

    return FALSE;
}

/*
 * FUNCTION: Searches through address file entries to find next match
 * ARGUMENTS:
 *     SearchContext = Pointer to search context
 * RETURNS:
 *     Pointer to address file, NULL if none was found
 */
PADDRESS_FILE AddrSearchNext(
    PAF_SEARCH SearchContext)
{
    PLIST_ENTRY CurrentEntry;
    PIP_ADDRESS IPAddress;
    KIRQL OldIrql;
    PADDRESS_FILE Current = NULL;
    BOOLEAN Found = FALSE;

    if (IsListEmpty(SearchContext->Next))
        return NULL;

    CurrentEntry = SearchContext->Next;

    TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);

    while (CurrentEntry != &AddressFileListHead) {
        Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_FILE, ListEntry);

        IPAddress = &Current->Address;

        TI_DbgPrint(DEBUG_ADDRFILE, ("Comparing: ((%d, %d, %s), (%d, %d, %s)).\n",
            WN2H(Current->Port),
            Current->Protocol,
            A2S(IPAddress),
            WN2H(SearchContext->Port),
            SearchContext->Protocol,
            A2S(SearchContext->Address)));

        /* See if this address matches the search criteria */
        if ((Current->Port    == SearchContext->Port) &&
            (Current->Protocol == SearchContext->Protocol) &&
            (AddrIsEqual(IPAddress, SearchContext->Address) ||
             AddrIsBroadcast(IPAddress, SearchContext->Address) ||
             AddrIsUnspecified(IPAddress))) {
            /* We've found a match */
            Found = TRUE;
            break;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

    if (Found) {
        SearchContext->Next = CurrentEntry->Flink;
        return Current;
    } else
        return NULL;
}

VOID AddrFileFree(
    PVOID Object)
/*
 * FUNCTION: Frees an address file object
 * ARGUMENTS:
 *     Object = Pointer to address file object to free
 */
{
    ExFreePool(Object);
}


VOID ControlChannelFree(
    PVOID Object)
/*
 * FUNCTION: Frees an address file object
 * ARGUMENTS:
 *     Object = Pointer to address file object to free
 */
{
    ExFreePool(Object);
}


VOID DeleteAddress(PADDRESS_FILE AddrFile)
/*
 * FUNCTION: Deletes an address file object
 * ARGUMENTS:
 *     AddrFile = Pointer to address file object to delete
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PDATAGRAM_SEND_REQUEST SendRequest;
  PDATAGRAM_RECEIVE_REQUEST ReceiveRequest;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  /* Remove address file from the global list */
  TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);
  RemoveEntryList(&AddrFile->ListEntry);
  TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

  TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* FIXME: Kill TCP connections on this address file object */

  /* Return pending requests with error */

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting receive requests on AddrFile at (0x%X).\n", AddrFile));

  /* Go through pending receive request list and cancel them all */
  CurrentEntry = AddrFile->ReceiveQueue.Flink;
  while (CurrentEntry != &AddrFile->ReceiveQueue) {
    NextEntry = CurrentEntry->Flink;
    ReceiveRequest = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
    /* Abort the request and free its resources */
    TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);
    (*ReceiveRequest->Complete)(ReceiveRequest->Context, STATUS_ADDRESS_CLOSED, 0);
    TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);
    CurrentEntry = NextEntry;
  }

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting send requests on address file at (0x%X).\n", AddrFile));

  /* Go through pending send request list and cancel them all */
  CurrentEntry = AddrFile->TransmitQueue.Flink;
  while (CurrentEntry != &AddrFile->TransmitQueue) {
    NextEntry = CurrentEntry->Flink;
    SendRequest = CONTAINING_RECORD(CurrentEntry,
				    DATAGRAM_SEND_REQUEST, ListEntry);
    /* Abort the request and free its resources */
    TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);
    (*SendRequest->Complete)(SendRequest->Context, STATUS_ADDRESS_CLOSED, 0);
    ExFreePool(SendRequest);
    TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);
    CurrentEntry = NextEntry;
  }

  TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

  (*AddrFile->Free)(AddrFile);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


/*
 * FUNCTION: Deletes a connection endpoint file object
 * ARGUMENTS:
 *     Connection = Pointer to connection endpoint to delete
 */
VOID DeleteConnectionEndpoint(
  PCONNECTION_ENDPOINT Connection)
{
  KIRQL OldIrql;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  /* Remove connection endpoint from the global list */
  TcpipAcquireSpinLock(&ConnectionEndpointListLock, &OldIrql);
  RemoveEntryList(&Connection->ListEntry);
  TcpipReleaseSpinLock(&ConnectionEndpointListLock, OldIrql);

  ExFreePool(Connection);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

/*
 * FUNCTION: Open an address file object
 * ARGUMENTS:
 *     Request  = Pointer to TDI request structure for this request
 *     Address  = Pointer to address to be opened
 *     Protocol = Protocol on which to open the address
 *     Options  = Pointer to option buffer
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileOpenAddress(
  PTDI_REQUEST Request,
  PTA_IP_ADDRESS Address,
  USHORT Protocol,
  PVOID Options)
{
  IPv4_RAW_ADDRESS IPv4Address;
  BOOLEAN Matched;
  PADDRESS_FILE AddrFile;

  TI_DbgPrint(MID_TRACE, ("Called (Proto %d).\n", Protocol));

  AddrFile = ExAllocatePool(NonPagedPool, sizeof(ADDRESS_FILE));
  if (!AddrFile) {
    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  TI_DbgPrint(DEBUG_ADDRFILE, ("Address file object allocated at (0x%X).\n", AddrFile));

  RtlZeroMemory(AddrFile, sizeof(ADDRESS_FILE));

  AddrFile->Free = AddrFileFree;

  /* Make sure address is a local unicast address or 0 */

  /* Locate address entry. If specified address is 0, a random address is chosen */

  /* FIXME: IPv4 only */
  AddrFile->Family = Address->Address[0].AddressType;
  IPv4Address = Address->Address[0].Address[0].in_addr;
  if (IPv4Address == 0)
      Matched = IPGetDefaultAddress(&AddrFile->Address);
  else
      Matched = AddrLocateADEv4(IPv4Address, &AddrFile->Address);

  if (!Matched) {
    ExFreePool(AddrFile);
    TI_DbgPrint(MIN_TRACE, ("Non-local address given (0x%X).\n", DN2H(IPv4Address)));
    return STATUS_INVALID_PARAMETER;
  }

  TI_DbgPrint(MID_TRACE, ("Opening address %s for communication (P=%d U=%d).\n",
    A2S(&AddrFile->Address), Protocol, IPPROTO_UDP));

  /* Protocol specific handling */
  switch (Protocol) {
  case IPPROTO_TCP:
      AddrFile->Port =
          TCPAllocatePort(Address->Address[0].Address[0].sin_port);
      AddrFile->Send = NULL; /* TCPSendData */
      break;

  case IPPROTO_UDP:
      TI_DbgPrint(MID_TRACE,("Allocating udp port\n"));
      AddrFile->Port =
	  UDPAllocatePort(Address->Address[0].Address[0].sin_port);
      TI_DbgPrint(MID_TRACE,("Setting port %d (wanted %d)\n",
                             AddrFile->Port,
                             Address->Address[0].Address[0].sin_port));
      AddrFile->Send = UDPSendDatagram;
      break;

  default:
    /* Use raw IP for all other protocols */
    AddrFile->Port = 0;
    AddrFile->Send = RawIPSendDatagram;
    break;
  }

  TI_DbgPrint(MID_TRACE, ("IP protocol number for address file object is %d.\n",
    Protocol));

  TI_DbgPrint(MID_TRACE, ("Port number for address file object is %d.\n",
    WN2H(AddrFile->Port)));

  /* Set protocol */
  AddrFile->Protocol = Protocol;

  /* Initialize receive and transmit queues */
  InitializeListHead(&AddrFile->ReceiveQueue);
  InitializeListHead(&AddrFile->TransmitQueue);

  /* Initialize spin lock that protects the address file object */
  KeInitializeSpinLock(&AddrFile->Lock);

  /* Set valid flag so the address can be used */
  AF_SET_VALID(AddrFile);

  /* Return address file object */
  Request->Handle.AddressHandle = AddrFile;

  /* Add address file to global list */
  ExInterlockedInsertTailList(
    &AddressFileListHead,
    &AddrFile->ListEntry,
    &AddressFileListLock);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Closes an address file object
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileCloseAddress(
  PTDI_REQUEST Request)
{
  KIRQL OldIrql;
  PADDRESS_FILE AddrFile;
  NTSTATUS Status = STATUS_SUCCESS;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  AddrFile = Request->Handle.AddressHandle;

  TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Set address file object exclusive to us */
  AF_SET_BUSY(AddrFile);
  AF_CLR_VALID(AddrFile);

  TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

  /* Protocol specific handling */
  switch (AddrFile->Protocol) {
  case IPPROTO_TCP:
    TCPFreePort( AddrFile->Port );
    if( AddrFile->Listener )
	TCPClose( AddrFile->Listener );
    break;

  case IPPROTO_UDP:
    UDPFreePort( AddrFile->Port );
    break;
  }

  DeleteAddress(AddrFile);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return Status;
}


/*
 * FUNCTION: Opens a connection file object
 * ARGUMENTS:
 *     Request       = Pointer to TDI request structure for this request
 *     ClientContext = Pointer to client context information
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileOpenConnection(
  PTDI_REQUEST Request,
  PVOID ClientContext)
{
  NTSTATUS Status;
  PCONNECTION_ENDPOINT Connection;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = TCPAllocateConnectionEndpoint( ClientContext );

  if( !Connection ) return STATUS_NO_MEMORY;

  Status = TCPSocket( Connection, AF_INET, SOCK_STREAM, IPPROTO_TCP );

  /* Return connection endpoint file object */
  Request->Handle.ConnectionContext = Connection;

  /* Add connection endpoint to global list */
  ExInterlockedInsertTailList(
    &ConnectionEndpointListHead,
    &Connection->ListEntry,
    &ConnectionEndpointListLock);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Find a connection by examining the context field.  This
 * is needed in some situations where a FIN reply is needed after a
 * socket is formally broken.
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
PCONNECTION_ENDPOINT FileFindConnectionByContext( PVOID Context ) {
    PLIST_ENTRY Entry;
    KIRQL OldIrql;
    PCONNECTION_ENDPOINT Connection = NULL;

    TcpipAcquireSpinLock( &ConnectionEndpointListLock, &OldIrql );

    for( Entry = ConnectionEndpointListHead.Flink;
	 Entry != &ConnectionEndpointListHead;
	 Entry = Entry->Flink ) {
	Connection =
	    CONTAINING_RECORD( Entry, CONNECTION_ENDPOINT, ListEntry );
	if( Connection->SocketContext == Context ) break;
	else Connection = NULL;
    }

    TcpipReleaseSpinLock( &ConnectionEndpointListLock, OldIrql );

    return Connection;
}

/*
 * FUNCTION: Closes an connection file object
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileCloseConnection(
  PTDI_REQUEST Request)
{
  PCONNECTION_ENDPOINT Connection;
  NTSTATUS Status = STATUS_SUCCESS;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = Request->Handle.ConnectionContext;

  TcpipRecursiveMutexEnter( &TCPLock, TRUE );
  TCPClose(Connection);
  DeleteConnectionEndpoint(Connection);
  TcpipRecursiveMutexLeave( &TCPLock );

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return Status;
}


/*
 * FUNCTION: Opens a control channel file object
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileOpenControlChannel(
    PTDI_REQUEST Request)
{
  PCONTROL_CHANNEL ControlChannel;
  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  ControlChannel = ExAllocatePool(NonPagedPool, sizeof(*ControlChannel));

  if (!ControlChannel) {
    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(ControlChannel, sizeof(CONTROL_CHANNEL));

  /* Make sure address is a local unicast address or 0 */

  /* Locate address entry. If specified address is 0, a random address is chosen */

  /* Initialize receive and transmit queues */
  InitializeListHead(&ControlChannel->ListEntry);

  /* Initialize spin lock that protects the address file object */
  KeInitializeSpinLock(&ControlChannel->Lock);

  /* Return address file object */
  Request->Handle.ControlChannel = ControlChannel;

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
}

/*
 * FUNCTION: Closes a control channel file object
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileCloseControlChannel(
  PTDI_REQUEST Request)
{
  PCONTROL_CHANNEL ControlChannel = Request->Handle.ControlChannel;
  NTSTATUS Status = STATUS_SUCCESS;

  ExFreePool(ControlChannel);
  Request->Handle.ControlChannel = NULL;

  return Status;
}

/* EOF */
