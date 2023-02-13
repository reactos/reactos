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

/* FIXME: including pstypes.h without ntifs fails */
#include <ntifs.h>
#include <ndk/pstypes.h>

/* Uncomment for logging of connections and address files every 10 seconds */
//#define LOG_OBJECTS

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
    KIRQL OldIrql;

    SearchContext->Address  = Address;
    SearchContext->Port     = Port;
    SearchContext->Protocol = Protocol;

    TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);

    SearchContext->Next = AddressFileListHead.Flink;

    if (!IsListEmpty(&AddressFileListHead))
        ReferenceObject(CONTAINING_RECORD(SearchContext->Next, ADDRESS_FILE, ListEntry));

    TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

    return AddrSearchNext(SearchContext);
}

BOOLEAN AddrIsBroadcastMatch(
    PIP_ADDRESS UnicastAddress,
    PIP_ADDRESS BroadcastAddress ) {
    IF_LIST_ITER(IF);

    ForEachInterface(IF) {
        if ((AddrIsUnspecified(UnicastAddress) ||
             AddrIsEqual(&IF->Unicast, UnicastAddress)) &&
            (AddrIsEqual(&IF->Broadcast, BroadcastAddress)))
            return TRUE;
    } EndFor(IF);

    return FALSE;
}

BOOLEAN AddrReceiveMatch(
   PIP_ADDRESS LocalAddress,
   PIP_ADDRESS RemoteAddress)
{
   if (AddrIsEqual(LocalAddress, RemoteAddress))
   {
       /* Unicast address match */
       return TRUE;
   }

   if (AddrIsBroadcastMatch(LocalAddress, RemoteAddress))
   {
       /* Broadcast address match */
       return TRUE;
   }

   if (AddrIsUnspecified(LocalAddress))
   {
       /* Local address unspecified */
       return TRUE;
   }

   if (AddrIsUnspecified(RemoteAddress))
   {
       /* Remote address unspecified */
       return TRUE;
   }

   return FALSE;
}

VOID
LogActiveObjects(VOID)
{
#ifdef LOG_OBJECTS
    PLIST_ENTRY CurrentEntry;
    KIRQL OldIrql;
    PADDRESS_FILE AddrFile;
    PCONNECTION_ENDPOINT Conn;

    DbgPrint("----------- TCP/IP Active Object Dump -------------\n");

    TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);

    CurrentEntry = AddressFileListHead.Flink;
    while (CurrentEntry != &AddressFileListHead)
    {
        AddrFile = CONTAINING_RECORD(CurrentEntry, ADDRESS_FILE, ListEntry);

        DbgPrint("Address File (%s, %d, %d) @ 0x%p | Ref count: %d | Sharers: %d\n",
                 A2S(&AddrFile->Address), WN2H(AddrFile->Port), AddrFile->Protocol,
                 AddrFile, AddrFile->RefCount, AddrFile->Sharers);
        DbgPrint("\tListener: ");
        if (AddrFile->Listener == NULL)
            DbgPrint("<None>\n");
        else
            DbgPrint("0x%p\n", AddrFile->Listener);
        DbgPrint("\tAssociated endpoints: ");
        if (AddrFile->Connection == NULL)
            DbgPrint("<None>\n");
        else
        {
            Conn = AddrFile->Connection;
            while (Conn)
            {
                DbgPrint("0x%p ", Conn);
                Conn = Conn->Next;
            }
            DbgPrint("\n");
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

    TcpipAcquireSpinLock(&ConnectionEndpointListLock, &OldIrql);

    CurrentEntry = ConnectionEndpointListHead.Flink;
    while (CurrentEntry != &ConnectionEndpointListHead)
    {
        Conn = CONTAINING_RECORD(CurrentEntry, CONNECTION_ENDPOINT, ListEntry);

        DbgPrint("Connection @ 0x%p | Ref count: %d\n", Conn, Conn->RefCount);
        DbgPrint("\tPCB: ");
        if (Conn->SocketContext == NULL)
            DbgPrint("<None>\n");
        else
        {
            DbgPrint("0x%p\n", Conn->SocketContext);
            LibTCPDumpPcb(Conn->SocketContext);
        }
        DbgPrint("\tPacket queue status: %s\n", IsListEmpty(&Conn->PacketQueue) ? "Empty" : "Not Empty");
        DbgPrint("\tRequest lists: Connect: %s | Recv: %s | Send: %s | Shutdown: %s | Listen: %s\n",
                 IsListEmpty(&Conn->ConnectRequest) ? "Empty" : "Not Empty",
                 IsListEmpty(&Conn->ReceiveRequest) ? "Empty" : "Not Empty",
                 IsListEmpty(&Conn->SendRequest) ? "Empty" : "Not Empty",
                 IsListEmpty(&Conn->ShutdownRequest) ? "Empty" : "Not Empty",
                 IsListEmpty(&Conn->ListenRequest) ? "Empty" : "Not Empty");
        DbgPrint("\tSend shutdown: %s\n", Conn->SendShutdown ? "Yes" : "No");
        DbgPrint("\tReceive shutdown: %s\n", Conn->ReceiveShutdown ? "Yes" : "No");
        if (Conn->ReceiveShutdown) DbgPrint("\tReceive shutdown status: 0x%x\n", Conn->ReceiveShutdownStatus);
        DbgPrint("\tClosing: %s\n", Conn->Closing ? "Yes" : "No");

        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipReleaseSpinLock(&ConnectionEndpointListLock, OldIrql);

    DbgPrint("---------------------------------------------------\n");
#endif
}

PADDRESS_FILE AddrFindShared(
    PIP_ADDRESS BindAddress,
    USHORT Port,
    USHORT Protocol)
{
    PLIST_ENTRY CurrentEntry;
    KIRQL OldIrql;
    PADDRESS_FILE Current = NULL;

    TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);

    CurrentEntry = AddressFileListHead.Flink;
    while (CurrentEntry != &AddressFileListHead) {
        Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_FILE, ListEntry);

        /* See if this address matches the search criteria */
        if ((Current->Port == Port) &&
            (Current->Protocol == Protocol))
        {
            /* Increase the sharer count */
            ASSERT(Current->Sharers != 0);
            InterlockedIncrement(&Current->Sharers);
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
        Current = NULL;
    }

    TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

    return Current;
}

/*
 * FUNCTION: Searches through address file entries to find next match
 * ARGUMENTS:
 *     SearchContext = Pointer to search context
 * RETURNS:
 *     Pointer to referenced address file, NULL if none was found
 */
PADDRESS_FILE AddrSearchNext(
    PAF_SEARCH SearchContext)
{
    PLIST_ENTRY CurrentEntry;
    PIP_ADDRESS IPAddress;
    KIRQL OldIrql;
    PADDRESS_FILE Current = NULL;
    BOOLEAN Found = FALSE;
    PADDRESS_FILE StartingAddrFile;

    TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);

    if (SearchContext->Next == &AddressFileListHead)
    {
        TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);
        return NULL;
    }

    /* Save this pointer so we can dereference it later */
    StartingAddrFile = CONTAINING_RECORD(SearchContext->Next, ADDRESS_FILE, ListEntry);

    CurrentEntry = SearchContext->Next;

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
            (AddrReceiveMatch(IPAddress, SearchContext->Address))) {
            /* We've found a match */
            Found = TRUE;
            break;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    if (Found)
    {
        SearchContext->Next = CurrentEntry->Flink;

        if (SearchContext->Next != &AddressFileListHead)
        {
            /* Reference the next address file to prevent the link from disappearing behind our back */
            ReferenceObject(CONTAINING_RECORD(SearchContext->Next, ADDRESS_FILE, ListEntry));
        }

        /* Reference the returned address file before dereferencing the starting
         * address file because it may be that Current == StartingAddrFile */
        ReferenceObject(Current);
    }
    else
        Current = NULL;

    DereferenceObject(StartingAddrFile);

    TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

    return Current;
}

VOID AddrFileFree(
    PVOID Object)
/*
 * FUNCTION: Frees an address file object
 * ARGUMENTS:
 *     Object = Pointer to address file object to free
 */
{
  PADDRESS_FILE AddrFile = Object;
  KIRQL OldIrql;
  PDATAGRAM_RECEIVE_REQUEST ReceiveRequest;
  // PDATAGRAM_SEND_REQUEST SendRequest; See WTF below
  PLIST_ENTRY CurrentEntry;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  /* We should not be associated with a connection here */
  ASSERT(!AddrFile->Connection);

  /* Remove address file from the global list */
  TcpipAcquireSpinLock(&AddressFileListLock, &OldIrql);
  RemoveEntryList(&AddrFile->ListEntry);
  TcpipReleaseSpinLock(&AddressFileListLock, OldIrql);

  /* FIXME: Kill TCP connections on this address file object */

  /* Return pending requests with error */

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting receive requests on AddrFile at (0x%X).\n", AddrFile));

  /* Go through pending receive request list and cancel them all */
  while (!IsListEmpty(&AddrFile->ReceiveQueue))
  {
    CurrentEntry = RemoveHeadList(&AddrFile->ReceiveQueue);
    ReceiveRequest = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
    (*ReceiveRequest->Complete)(ReceiveRequest->Context, STATUS_CANCELLED, 0);
    ExFreePoolWithTag(ReceiveRequest, DATAGRAM_RECV_TAG);
  }

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting send requests on address file at (0x%X).\n", AddrFile));

#if 0 /* Biggest WTF. All of this was taken care of above as DATAGRAM_RECEIVE_REQUEST. */
  /* Go through pending send request list and cancel them all */
  while (!IsListEmpty(&AddrFile->ReceiveQueue))
  {
    CurrentEntry = RemoveHeadList(&AddrFile->ReceiveQueue);
    SendRequest = CONTAINING_RECORD(CurrentEntry, DATAGRAM_SEND_REQUEST, ListEntry);
    (*SendRequest->Complete)(SendRequest->Context, STATUS_CANCELLED, 0);
    ExFreePoolWithTag(SendRequest, DATAGRAM_SEND_TAG);
  }
#endif

  /* Protocol specific handling */
  switch (AddrFile->Protocol) {
  case IPPROTO_TCP:
    if (AddrFile->Port)
    {
        TCPFreePort(AddrFile->Port);
    }
    break;

  case IPPROTO_UDP:
    UDPFreePort( AddrFile->Port );
    break;
  }

  RemoveEntityByContext(AddrFile);

  ExDeleteResourceLite(&AddrFile->Resource);

  ExFreePoolWithTag(Object, ADDR_FILE_TAG);
}


VOID ControlChannelFree(
    PVOID Object)
/*
 * FUNCTION: Frees an address file object
 * ARGUMENTS:
 *     Object = Pointer to address file object to free
 */
{
    ExFreePoolWithTag(Object, CONTROL_CHANNEL_TAG);
}


/*
 * FUNCTION: Open an address file object
 * ARGUMENTS:
 *     Request  = Pointer to TDI request structure for this request
 *     Address  = Pointer to address to be opened
 *     Protocol = Protocol on which to open the address
 *     Shared   = Specifies if the address is opened for shared access
 *     Options  = Pointer to option buffer
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileOpenAddress(
  PTDI_REQUEST Request,
  PTA_IP_ADDRESS Address,
  USHORT Protocol,
  BOOLEAN Shared,
  PVOID Options)
{
  PADDRESS_FILE AddrFile;
  UINT AllocatedPort;

  TI_DbgPrint(MID_TRACE, ("Called (Proto %d).\n", Protocol));

  /* If it's shared and has a port specified, look for a match */
  if ((Shared != FALSE) && (Address->Address[0].Address[0].sin_port != 0))
  {
    AddrFile = AddrFindShared(NULL, Address->Address[0].Address[0].sin_port, Protocol);
    if (AddrFile != NULL)
    {
        Request->Handle.AddressHandle = AddrFile;
        return STATUS_SUCCESS;
    }
  }

  AddrFile = ExAllocatePoolWithTag(NonPagedPool, sizeof(ADDRESS_FILE),
                                   ADDR_FILE_TAG);
  if (!AddrFile) {
    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(AddrFile, sizeof(ADDRESS_FILE));

  AddrFile->RefCount = 1;
  AddrFile->Free = AddrFileFree;
  AddrFile->Sharers = 1;

  /* Set our default options */
  AddrFile->TTL = 128;
  AddrFile->DF = 0;
  AddrFile->BCast = 1;
  AddrFile->HeaderIncl = 1;
  AddrFile->ProcessId = PsGetCurrentProcessId();

  _SEH2_TRY {
      PTEB Teb;

      Teb = PsGetCurrentThreadTeb();
      if (Teb != NULL)
         AddrFile->SubProcessTag = Teb->SubProcessTag;
  } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
      AddrFile->SubProcessTag = 0;
  } _SEH2_END;

  KeQuerySystemTime(&AddrFile->CreationTime);

  /* Make sure address is a local unicast address or 0 */
  /* FIXME: IPv4 only */
  AddrFile->Family = Address->Address[0].AddressType;
  AddrFile->Address.Address.IPv4Address = Address->Address[0].Address[0].in_addr;
  AddrFile->Address.Type = IP_ADDRESS_V4;

  if (!AddrIsUnspecified(&AddrFile->Address) &&
      !AddrLocateInterface(&AddrFile->Address)) {
	  TI_DbgPrint(MIN_TRACE, ("Non-local address given (0x%X).\n", A2S(&AddrFile->Address)));
	  ExFreePoolWithTag(AddrFile, ADDR_FILE_TAG);
	  return STATUS_INVALID_ADDRESS;
  }

  TI_DbgPrint(MID_TRACE, ("Opening address %s for communication (P=%d U=%d).\n",
    A2S(&AddrFile->Address), Protocol, IPPROTO_UDP));

  /* Protocol specific handling */
  switch (Protocol) {
  case IPPROTO_TCP:
      if (Address->Address[0].Address[0].sin_port)
      {
          /* The client specified an explicit port so we force a bind to this */
          AllocatedPort = TCPAllocatePort(Address->Address[0].Address[0].sin_port);

          /* Check for bind success */
          if (AllocatedPort == (UINT)-1)
          {
              ExFreePoolWithTag(AddrFile, ADDR_FILE_TAG);
              return STATUS_ADDRESS_ALREADY_EXISTS;
          }
          AddrFile->Port = AllocatedPort;

          /* Sanity check */
          ASSERT(Address->Address[0].Address[0].sin_port == AddrFile->Port);
      }
      else if (!AddrIsUnspecified(&AddrFile->Address))
      {
          /* The client is trying to bind to a local address so allocate a port now too */
          AllocatedPort = TCPAllocatePort(0);

          /* Check for bind success */
          if (AllocatedPort == (UINT)-1)
          {
              ExFreePoolWithTag(AddrFile, ADDR_FILE_TAG);
              return STATUS_ADDRESS_ALREADY_EXISTS;
          }
          AddrFile->Port = AllocatedPort;
      }
      else
      {
          /* The client wants an unspecified port with an unspecified address so we wait to see what the TCP library gives us */
          AddrFile->Port = 0;
      }

      AddEntity(CO_TL_ENTITY, AddrFile, CO_TL_TCP);

      AddrFile->Send = NULL; /* TCPSendData */
      break;

  case IPPROTO_UDP:
      TI_DbgPrint(MID_TRACE,("Allocating udp port\n"));
      AllocatedPort = UDPAllocatePort(Address->Address[0].Address[0].sin_port);

      if ((Address->Address[0].Address[0].sin_port &&
           AllocatedPort != Address->Address[0].Address[0].sin_port) ||
           AllocatedPort == (UINT)-1)
      {
          ExFreePoolWithTag(AddrFile, ADDR_FILE_TAG);
          return STATUS_ADDRESS_ALREADY_EXISTS;
      }
      AddrFile->Port = AllocatedPort;

      TI_DbgPrint(MID_TRACE,("Setting port %d (wanted %d)\n",
                             AddrFile->Port,
                             Address->Address[0].Address[0].sin_port));

      AddEntity(CL_TL_ENTITY, AddrFile, CL_TL_UDP);

      AddrFile->Send = UDPSendDatagram;
      break;

  case IPPROTO_ICMP:
    AddrFile->Port = 0;
    AddrFile->Send = ICMPSendDatagram;

    /* FIXME: Verify this */
    AddEntity(ER_ENTITY, AddrFile, ER_ICMP);
    break;

  default:
    /* Use raw IP for all other protocols */
    AddrFile->Port = 0;
    AddrFile->Send = RawIPSendDatagram;

    /* FIXME: Verify this */
    AddEntity(CL_TL_ENTITY, AddrFile, 0);
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
  ExInitializeResourceLite(&AddrFile->Resource);

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
  PADDRESS_FILE AddrFile = Request->Handle.AddressHandle;
  PCONNECTION_ENDPOINT Listener;

  if (!Request->Handle.AddressHandle) return STATUS_INVALID_PARAMETER;

  LockObject(AddrFile);

  if (InterlockedDecrement(&AddrFile->Sharers) != 0)
  {
      /* Still other guys have open handles to this, so keep it around */
      UnlockObject(AddrFile);
      return STATUS_SUCCESS;
  }

  /* We have to close this listener because we started it */
  Listener = AddrFile->Listener;
  UnlockObject(AddrFile);
  if( Listener )
  {
      TCPClose( Listener );
  }

  DereferenceObject(AddrFile);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
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

  if( !NT_SUCCESS(Status) ) {
      DereferenceObject( Connection );
      return Status;
  }

  /* Return connection endpoint file object */
  Request->Handle.ConnectionContext = Connection;

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
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

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = Request->Handle.ConnectionContext;

  if (!Connection) return STATUS_INVALID_PARAMETER;

  TCPClose( Connection );

  Request->Handle.ConnectionContext = NULL;

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
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

  ControlChannel = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ControlChannel),
                                         CONTROL_CHANNEL_TAG);

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

  ControlChannel->RefCount = 1;
  ControlChannel->Free = ControlChannelFree;

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
  if (!Request->Handle.ControlChannel) return STATUS_INVALID_PARAMETER;

  DereferenceObject((PCONTROL_CHANNEL)Request->Handle.ControlChannel);

  Request->Handle.ControlChannel = NULL;

  return STATUS_SUCCESS;
}

/* EOF */
