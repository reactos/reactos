/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/fileobjs.c
 * PURPOSE:     Routines for handling file objects
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <datagram.h>
#include <address.h>
#include <pool.h>
#include <rawip.h>
#include <tcp.h>
#include <udp.h>
#include <ip.h>
#include <fileobjs.h>

/* List of all address file objects managed by this driver */
LIST_ENTRY AddressFileListHead;
KSPIN_LOCK AddressFileListLock;

/* List of all connection endpoint file objects managed by this driver */
LIST_ENTRY ConnectionEndpointListHead;
KSPIN_LOCK ConnectionEndpointListLock;


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


VOID DeleteAddress(
  PADDRESS_FILE AddrFile)
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
  KeAcquireSpinLock(&AddressFileListLock, &OldIrql);
  RemoveEntryList(&AddrFile->ListEntry);
  KeReleaseSpinLock(&AddressFileListLock, OldIrql);

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* FIXME: Kill TCP connections on this address file object */

  /* Return pending requests with error */

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting receive requests on AddrFile at (0x%X).\n", AddrFile));

  /* Go through pending receive request list and cancel them all */
  CurrentEntry = AddrFile->ReceiveQueue.Flink;
  while (CurrentEntry != &AddrFile->ReceiveQueue) {
    NextEntry = CurrentEntry->Flink;
    ReceiveRequest = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
    /* Abort the request and free its resources */
    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
    (*ReceiveRequest->Complete)(ReceiveRequest->Context, STATUS_ADDRESS_CLOSED, 0);
    ExFreePool(ReceiveRequest);
    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);
    CurrentEntry = NextEntry;
  }

  TI_DbgPrint(DEBUG_ADDRFILE, ("Aborting send requests on address file at (0x%X).\n", AddrFile));

  /* Go through pending send request list and cancel them all */
  CurrentEntry = AddrFile->TransmitQueue.Flink;
  while (CurrentEntry != &AddrFile->TransmitQueue) {
    NextEntry = CurrentEntry->Flink;
    SendRequest = CONTAINING_RECORD(CurrentEntry, DATAGRAM_SEND_REQUEST, ListEntry);
    /* Abort the request and free its resources */
    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
    (*SendRequest->Complete)(SendRequest->Context, STATUS_ADDRESS_CLOSED, 0);
    ExFreePool(SendRequest);
    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);
    CurrentEntry = NextEntry;
  }

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  /* Dereference address entry */
  DereferenceObject(AddrFile->ADE);

  /* Dereference address cache */
  if (AddrFile->AddrCache)
    DereferenceObject(AddrFile->AddrCache);

#ifdef DBG
  /* Remove reference provided at creation time */
  AddrFile->RefCount--;

  if (AddrFile->RefCount != 0)
    TI_DbgPrint(DEBUG_REFCOUNT, ("AddrFile->RefCount is (%d) (should be 0).\n", AddrFile->RefCount));
#endif
  
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
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  /* Remove connection endpoint from the global list */
  KeAcquireSpinLock(&ConnectionEndpointListLock, &OldIrql);
  RemoveEntryList(&Connection->ListEntry);
  KeReleaseSpinLock(&ConnectionEndpointListLock, OldIrql);

  KeAcquireSpinLock(&Connection->Lock, &OldIrql);

  /* Dereference and remove the address file if it exists */
  if (Connection->AddressFile) {
    RemoveEntryList(&Connection->AddrFileEntry);
    DereferenceObject(Connection->AddressFile);
  }

  KeReleaseSpinLock(&Connection->Lock, OldIrql);

#ifdef DBG
  /* Remove reference provided at creation time */
  Connection->RefCount--;

  if (Connection->RefCount != 0)
    TI_DbgPrint(DEBUG_REFCOUNT, ("Connection->RefCount is (%d) (should be 0).\n",
      Connection->RefCount));
#endif

  ExFreePool(Connection);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID RequestWorker(
  PVOID Context)
/*
 * FUNCTION: Worker routine for processing address file object requests
 * ARGUMENTS:
 *     Context = Pointer to context information (ADDRESS_FILE)
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PADDRESS_FILE AddrFile = Context;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Check it the address file should be deleted */
  if (AF_IS_PENDING(AddrFile, AFF_DELETE)) {
    DATAGRAM_COMPLETION_ROUTINE RtnComplete;
    PVOID RtnContext;

    RtnComplete = AddrFile->Complete;
    RtnContext  = AddrFile->Context;

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    DeleteAddress(AddrFile);

    (*RtnComplete)(RtnContext, TDI_SUCCESS, 0);

    TI_DbgPrint(MAX_TRACE, ("Leaving (delete).\n"));

    return;
  }

  /* Check if there is a pending send request */
  if (AF_IS_PENDING(AddrFile, AFF_SEND)) {
    if (!IsListEmpty(&AddrFile->TransmitQueue)) {
      PDATAGRAM_SEND_REQUEST SendRequest;

      CurrentEntry = RemoveHeadList(&AddrFile->TransmitQueue);
      SendRequest  = CONTAINING_RECORD(CurrentEntry, DATAGRAM_SEND_REQUEST, ListEntry);

      AF_CLR_BUSY(AddrFile);

      ReferenceObject(AddrFile);

      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      /* The send routine processes the send requests in
         the transmit queue on the address file. When the
         queue is empty the pending send flag is cleared.
         The routine may return with the pending send flag
         set. This can happen if there was not enough free
         resources available to complete all send requests */
      DGSend(AddrFile, SendRequest);

      KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);
      DereferenceObject(AddrFile);
      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      TI_DbgPrint(MAX_TRACE, ("Leaving (send request).\n"));

      return;
    } else
      /* There was a pending send, but no send request.
         Print a debug message and continue */
      TI_DbgPrint(MIN_TRACE, ("Pending send, but no send request.\n"));
  }

  AF_CLR_BUSY(AddrFile);

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

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
  PTA_ADDRESS_IP Address,
  USHORT Protocol,
  PVOID Options)
{
  PADDRESS_FILE AddrFile;
  IPv4_RAW_ADDRESS IPv4Address;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

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
  IPv4Address = Address->Address[0].Address[0].in_addr;
  if (IPv4Address == 0)
    AddrFile->ADE = IPGetDefaultADE(ADE_UNICAST);
  else
    AddrFile->ADE = AddrLocateADEv4(IPv4Address);

  if (!AddrFile->ADE) {
    ExFreePool(AddrFile);
    TI_DbgPrint(MIN_TRACE, ("Non-local address given (0x%X).\n", DN2H(IPv4Address)));
    return STATUS_INVALID_PARAMETER;
  }

  TI_DbgPrint(MID_TRACE, ("Opening address %s for communication.\n",
    A2S(AddrFile->ADE->Address)));

  /* Protocol specific handling */
  switch (Protocol) {
  case IPPROTO_TCP:
    /* FIXME: If specified port is 0, a port is chosen dynamically */
    AddrFile->Port = Address->Address[0].Address[0].sin_port;
    AddrFile->Send = TCPSendDatagram;
    break;

  case IPPROTO_UDP:
    /* FIXME: If specified port is 0, a port is chosen dynamically */
    AddrFile->Port = Address->Address[0].Address[0].sin_port;
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

  /* Set protocol */
  AddrFile->Protocol = Protocol;
  
  /* Initialize receive and transmit queues */
  InitializeListHead(&AddrFile->ReceiveQueue);
  InitializeListHead(&AddrFile->TransmitQueue);

  /* Initialize associated connection list */
  InitializeListHead(&AddrFile->Connections);

  /* Initialize work queue item. We use this for pending requests */
  ExInitializeWorkItem(&AddrFile->WorkItem, RequestWorker, AddrFile);

  /* Initialize spin lock that protects the address file object */
  KeInitializeSpinLock(&AddrFile->Lock);

  /* Reference the object */
  AddrFile->RefCount = 1;

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

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  if ((!AF_IS_BUSY(AddrFile)) && (AddrFile->RefCount == 1)) {
    /* Set address file object exclusive to us */
    AF_SET_BUSY(AddrFile);
    AF_CLR_VALID(AddrFile);

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    DeleteAddress(AddrFile);
  } else {
    if (!AF_IS_PENDING(AddrFile, AFF_DELETE)) {
      AddrFile->Complete = Request->RequestNotifyObject;
      AddrFile->Context  = Request->RequestContext;

      /* Shedule address file for deletion */
      AF_SET_PENDING(AddrFile, AFF_DELETE);
      AF_CLR_VALID(AddrFile);

      if (!AF_IS_BUSY(AddrFile)) {
        /* Worker function is not running, so shedule it to run */
        AF_SET_BUSY(AddrFile);
        KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
        ExQueueWorkItem(&AddrFile->WorkItem, CriticalWorkQueue);
      } else
        KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      TI_DbgPrint(MAX_TRACE, ("Leaving (pending).\n"));

      return STATUS_PENDING;
    } else
      Status = STATUS_ADDRESS_CLOSED;

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
  }

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
  PCONNECTION_ENDPOINT Connection;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = ExAllocatePool(NonPagedPool, sizeof(CONNECTION_ENDPOINT));
  if (!Connection)
    return STATUS_INSUFFICIENT_RESOURCES;

  TI_DbgPrint(DEBUG_CPOINT, ("Connection point file object allocated at (0x%X).\n", Connection));

  RtlZeroMemory(Connection, sizeof(CONNECTION_ENDPOINT));

  /* Initialize spin lock that protects the connection endpoint file object */
  KeInitializeSpinLock(&Connection->Lock);

  /* Reference the object */
  Connection->RefCount = 1;

  /* Put connection in the closed state */
  Connection->State = ctClosed;

  /* Save client context pointer */
  Connection->ClientContext = ClientContext;

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
 * FUNCTION: Closes an connection file object
 * ARGUMENTS:
 *     Request = Pointer to TDI request structure for this request
 * RETURNS:
 *     Status of operation
 */
NTSTATUS FileCloseConnection(
  PTDI_REQUEST Request)
{
  KIRQL OldIrql;
  PCONNECTION_ENDPOINT Connection;
  NTSTATUS Status = STATUS_SUCCESS;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Connection = Request->Handle.ConnectionContext;

  KeAcquireSpinLock(&Connection->Lock, &OldIrql);
#if 0
  if ((!AF_IS_BUSY(Connection)) && (Connection->RefCount == 1)) {
    /* Set connection endpoint file object exclusive to us */
    AF_SET_BUSY(Connection);
    AF_CLR_VALID(Connection);

    KeReleaseSpinLock(&Connection->Lock, OldIrql);
#endif
    DeleteConnectionEndpoint(Connection);
#if 0
  } else {
    if (!AF_IS_PENDING(Connection, AFF_DELETE)) {
      Connection->Complete = Request->RequestNotifyObject;
      Connection->Context  = Request->RequestContext;

      /* Shedule connection endpoint for deletion */
      AF_SET_PENDING(Connection, AFF_DELETE);
      AF_CLR_VALID(Connection);

      if (!AF_IS_BUSY(Connection)) {
        /* Worker function is not running, so shedule it to run */
        AF_SET_BUSY(Connection);
        KeReleaseSpinLock(&Connection->Lock, OldIrql);
        ExQueueWorkItem(&Connection->WorkItem, CriticalWorkQueue);
      } else
        KeReleaseSpinLock(&Connection->Lock, OldIrql);

      TI_DbgPrint(MAX_TRACE, ("Leaving (pending).\n"));

      return STATUS_PENDING;
    } else
      Status = STATUS_ADDRESS_CLOSED;

    KeReleaseSpinLock(&Connection->Lock, OldIrql);
  }
#endif
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
  return STATUS_NOT_IMPLEMENTED;
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
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
