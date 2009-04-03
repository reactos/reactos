/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/titypes.h
 * PURPOSE:     TCP/IP protocol driver types
 */
#ifndef __TITYPES_H
#define __TITYPES_H


#ifdef DBG

#define DEFINE_TAG ULONG Tag;
#define INIT_TAG(_Object, _Tag) \
  ((_Object)->Tag = (_Tag))

#define DEBUG_REFCHECK(Object) {        \
   if ((Object)->RefCount <= 0) {       \
      TI_DbgPrint(MIN_TRACE, ("Object at (0x%X) has invalid reference count (%d).\n", \
        (Object), (Object)->RefCount)); \
    }                                   \
}

/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)      \
{                                    \
  CHAR c1, c2, c3, c4;               \
                                     \
  c1 = ((Object)->Tag >> 24) & 0xFF; \
  c2 = ((Object)->Tag >> 16) & 0xFF; \
  c3 = ((Object)->Tag >> 8) & 0xFF;  \
  c4 = ((Object)->Tag & 0xFF);       \
                                     \
  DEBUG_REFCHECK(Object);            \
  TI_DbgPrint(DEBUG_REFCOUNT, ("Referencing object of type (%c%c%c%c) at (0x%X). RefCount (%d).\n", \
    c4, c3, c2, c1, (Object), (Object)->RefCount)); \
                                                    \
  InterlockedIncrement(&((Object)->RefCount));      \
}

  /*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)    \
{                                    \
  CHAR c1, c2, c3, c4;               \
                                     \
  c1 = ((Object)->Tag >> 24) & 0xFF; \
  c2 = ((Object)->Tag >> 16) & 0xFF; \
  c3 = ((Object)->Tag >> 8) & 0xFF;  \
  c4 = ((Object)->Tag & 0xFF);       \
                                     \
  DEBUG_REFCHECK(Object);            \
  TI_DbgPrint(DEBUG_REFCOUNT, ("Dereferencing object of type (%c%c%c%c) at (0x%X). RefCount (%d).\n", \
    c4, c3, c2, c1, (Object), (Object)->RefCount));     \
                                                        \
  if (InterlockedDecrement(&((Object)->RefCount)) == 0) \
    (((Object)->Free)(Object));                         \
}

#else /* DBG */

#define DEFINE_TAG
#define INIT_TAG(Object, Tag)

/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)                  \
{                                                \
    InterlockedIncrement(&((Object)->RefCount)); \
}

/*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)                         \
{                                                         \
    if (InterlockedDecrement(&((Object)->RefCount)) == 0) \
        (((Object)->Free)(Object));                       \
}

#endif /* DBG */


#include <ip.h>

struct _ADDRESS_FILE;

/***************************************************
* Connection-less communication support structures *
***************************************************/

typedef NTSTATUS (*DATAGRAM_SEND_ROUTINE)(
    struct _ADDRESS_FILE *AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR Buffer,
    ULONG DataSize,
    PULONG DataUsed);

/* Datagram completion handler prototype */
typedef VOID (*DATAGRAM_COMPLETION_ROUTINE)(
    PVOID Context,
    NDIS_STATUS Status,
    ULONG Count);

typedef DATAGRAM_COMPLETION_ROUTINE PDATAGRAM_COMPLETION_ROUTINE;

typedef struct _DATAGRAM_RECEIVE_REQUEST {
    struct _ADDRESS_FILE *AddressFile;     /* AddressFile on behalf of */
    LIST_ENTRY ListEntry;                  /* Entry on list */
    IP_ADDRESS RemoteAddress;              /* Remote address we receive from (NULL means any) */
    USHORT RemotePort;                     /* Remote port we receive from (0 means any) */
    PTDI_CONNECTION_INFORMATION ReturnInfo;/* Return information */
    PCHAR Buffer;                          /* Pointer to receive buffer */
    ULONG BufferSize;                      /* Size of Buffer */
    DATAGRAM_COMPLETION_ROUTINE Complete;  /* Completion routine */
    PVOID Context;                         /* Pointer to context information */
    DATAGRAM_COMPLETION_ROUTINE UserComplete;   /* Completion routine */
    PVOID UserContext;                     /* Pointer to context information */
    PIRP Irp;                              /* IRP on behalf of */
} DATAGRAM_RECEIVE_REQUEST, *PDATAGRAM_RECEIVE_REQUEST;

/* Datagram build routine prototype */
typedef NTSTATUS (*DATAGRAM_BUILD_ROUTINE)(
    PVOID Context,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET *IPPacket);

typedef struct _DATAGRAM_SEND_REQUEST {
    LIST_ENTRY ListEntry;
    PNDIS_PACKET PacketToSend;
    DATAGRAM_COMPLETION_ROUTINE Complete; /* Completion routine */
    PVOID Context;                        /* Pointer to context information */
    IP_PACKET Packet;
    UINT BufferSize;
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    ULONG Flags;                          /* Protocol specific flags */
} DATAGRAM_SEND_REQUEST, *PDATAGRAM_SEND_REQUEST;

/* Transport address file context structure. The FileObject->FsContext2
   field holds a pointer to this structure */
typedef struct _ADDRESS_FILE {
    DEFINE_TAG
    LIST_ENTRY ListEntry;                 /* Entry on list */
    KSPIN_LOCK Lock;                      /* Spin lock to manipulate this structure */
    OBJECT_FREE_ROUTINE Free;             /* Routine to use to free resources for the object */
    USHORT Flags;                         /* Flags for address file (see below) */
    IP_ADDRESS Address;                   /* Address of this address file */
    USHORT Family;                        /* Address family */
    USHORT Protocol;                      /* Protocol number */
    USHORT Port;                          /* Network port (network byte order) */
    WORK_QUEUE_ITEM WorkItem;             /* Work queue item handle */
    DATAGRAM_COMPLETION_ROUTINE Complete; /* Completion routine for delete request */
    PVOID Context;                        /* Delete request context */
    DATAGRAM_SEND_ROUTINE Send;           /* Routine to send a datagram */
    LIST_ENTRY ReceiveQueue;              /* List of outstanding receive requests */
    LIST_ENTRY TransmitQueue;             /* List of outstanding transmit requests */
    struct _CONNECTION_ENDPOINT *Connection;
    /* Associated connection or NULL if no associated connection exist */
    struct _CONNECTION_ENDPOINT *Listener;
    /* Associated listener (see transport/tcp/accept.c) */
    IP_ADDRESS AddrCache;                 /* One entry address cache (destination
                                             address of last packet transmitted) */

    /* The following members are used to control event notification */

    /* Connection indication handler */
    PTDI_IND_CONNECT ConnectHandler;
    PVOID ConnectHandlerContext;
    BOOLEAN RegisteredConnectHandler;
    /* Disconnect indication handler */
    PTDI_IND_DISCONNECT DisconnectHandler;
    PVOID DisconnectHandlerContext;
    BOOLEAN RegisteredDisconnectHandler;
    /* Error indication handler */
    PTDI_IND_ERROR ErrorHandler;
    PVOID ErrorHandlerContext;
    PVOID ErrorHandlerOwner;
    BOOLEAN RegisteredErrorHandler;
    /* Receive indication handler */
    PTDI_IND_RECEIVE ReceiveHandler;
    PVOID ReceiveHandlerContext;
    BOOLEAN RegisteredReceiveHandler;
    /* Receive datagram indication handler */
    PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
    PVOID ReceiveDatagramHandlerContext;
    BOOLEAN RegisteredReceiveDatagramHandler;
    /* Expedited receive indication handler */
    PTDI_IND_RECEIVE_EXPEDITED ExpeditedReceiveHandler;
    PVOID ExpeditedReceiveHandlerContext;
    BOOLEAN RegisteredExpeditedReceiveHandler;
    /* Chained receive indication handler */
    PTDI_IND_CHAINED_RECEIVE ChainedReceiveHandler;
    PVOID ChainedReceiveHandlerContext;
    BOOLEAN RegisteredChainedReceiveHandler;
    /* Chained receive datagram indication handler */
    PTDI_IND_CHAINED_RECEIVE_DATAGRAM ChainedReceiveDatagramHandler;
    PVOID ChainedReceiveDatagramHandlerContext;
    BOOLEAN RegisteredChainedReceiveDatagramHandler;
    /* Chained expedited receive indication handler */
    PTDI_IND_CHAINED_RECEIVE_EXPEDITED ChainedReceiveExpeditedHandler;
    PVOID ChainedReceiveExpeditedHandlerContext;
    BOOLEAN RegisteredChainedReceiveExpeditedHandler;
} ADDRESS_FILE, *PADDRESS_FILE;

/* Address File Flag constants */
#define AFF_VALID    0x0001 /* Address file object is valid for use */
#define AFF_BUSY     0x0002 /* Address file object is exclusive to someone */
#define AFF_DELETE   0x0004 /* Address file object is sheduled to be deleted */
#define AFF_SEND     0x0008 /* A send request is pending */
#define AFF_RECEIVE  0x0010 /* A receive request is pending */
#define AFF_PENDING  0x001C /* A request is pending */

/* Macros for manipulating address file object flags */

#define AF_IS_VALID(ADF)  ((ADF)->Flags & AFF_VALID)
#define AF_SET_VALID(ADF) ((ADF)->Flags |= AFF_VALID)
#define AF_CLR_VALID(ADF) ((ADF)->Flags &= ~AFF_VALID)

#define AF_IS_BUSY(ADF)  ((ADF)->Flags & AFF_BUSY)
#define AF_SET_BUSY(ADF) ((ADF)->Flags |= AFF_BUSY)
#define AF_CLR_BUSY(ADF) ((ADF)->Flags &= ~AFF_BUSY)

#define AF_IS_PENDING(ADF, X)  (ADF->Flags & X)
#define AF_SET_PENDING(ADF, X) (ADF->Flags |= X)
#define AF_CLR_PENDING(ADF, X) (ADF->Flags &= ~X)


/* Structure used to search through Address Files */
typedef struct _AF_SEARCH {
    PLIST_ENTRY Next;       /* Next address file to check */
    PIP_ADDRESS Address;    /* Pointer to address to be found */
    USHORT Port;            /* Network port */
    USHORT Protocol;        /* Protocol number */
} AF_SEARCH, *PAF_SEARCH;

/*******************************************************
* Connection-oriented communication support structures *
*******************************************************/

typedef struct _TCP_RECEIVE_REQUEST {
  LIST_ENTRY ListEntry;                 /* Entry on list */
  PNDIS_BUFFER Buffer;                  /* Pointer to receive buffer */
  ULONG BufferSize;                     /* Size of Buffer */
  DATAGRAM_COMPLETION_ROUTINE Complete; /* Completion routine */
  PVOID Context;                        /* Pointer to context information */
} TCP_RECEIVE_REQUEST, *PTCP_RECEIVE_REQUEST;

/* Connection states */
typedef enum {
  ctListen = 0,   /* Waiting for incoming connection requests */
  ctSynSent,      /* Waiting for matching connection request */
  ctSynReceived,  /* Waiting for connection request acknowledgment */
  ctEstablished,  /* Connection is open for data transfer */
  ctFinWait1,     /* Waiting for termination request or ack. for same */
  ctFinWait2,     /* Waiting for termination request from remote TCP */
  ctCloseWait,    /* Waiting for termination request from local user */
  ctClosing,      /* Waiting for termination ack. from remote TCP */
  ctLastAck,      /* Waiting for termination request ack. from remote TCP */
  ctTimeWait,     /* Waiting for enough time to pass to be sure the remote TCP
                     received the ack. of its connection termination request */
  ctClosed        /* Represents a closed connection */
} CONNECTION_STATE, *PCONNECTION_STATE;


/* Structure for an TCP segment */
typedef struct _TCP_SEGMENT {
  LIST_ENTRY ListEntry;
  PIP_PACKET IPPacket;        /* Pointer to IP packet */
  PVOID SegmentData;          /* Pointer to segment data */
  ULONG SequenceNumber;       /* Sequence number of first byte in segment */
  ULONG Length;               /* Number of bytes in segment */
  ULONG BytesDelivered;       /* Number of bytes already delivered to the client */
} TCP_SEGMENT, *PTCP_SEGMENT;

typedef struct _TDI_BUCKET {
    LIST_ENTRY Entry;
    struct _CONNECTION_ENDPOINT *AssociatedEndpoint;
    TDI_REQUEST Request;
} TDI_BUCKET, *PTDI_BUCKET;

/* Transport connection context structure A.K.A. Transmission Control Block
   (TCB) in TCP terminology. The FileObject->FsContext2 field holds a pointer
   to this structure */
typedef struct _CONNECTION_ENDPOINT {
    LIST_ENTRY ListEntry;       /* Entry on list */
    KSPIN_LOCK Lock;            /* Spin lock to protect this structure */
    PVOID ClientContext;        /* Pointer to client context information */
    PADDRESS_FILE AddressFile;  /* Associated address file object (NULL if none) */
    PVOID SocketContext;        /* Context for lower layer */

    UINT State;                 /* Socket state W.R.T. oskit */

    /* Requests */
    LIST_ENTRY ConnectRequest; /* Queued connect rqueusts */
    LIST_ENTRY ListenRequest;  /* Queued listen requests */
    LIST_ENTRY ReceiveRequest; /* Queued receive requests */
    LIST_ENTRY SendRequest;    /* Queued send requests */

    /* Signals */
    LIST_ENTRY SignalList;     /* Entry in the list of sockets waiting for
				* notification service to the client */
    UINT    SignalState;       /* Active signals from oskit */
    BOOLEAN Signalled;         /* Are we a member of the signal list */
} CONNECTION_ENDPOINT, *PCONNECTION_ENDPOINT;



/*************************
* TDI support structures *
*************************/

/* Transport control channel context structure. The FileObject->FsContext2
   field holds a pointer to this structure */
typedef struct _CONTROL_CHANNEL {
    LIST_ENTRY ListEntry;       /* Entry on list */
    KSPIN_LOCK Lock;            /* Spin lock to protect this structure */
} CONTROL_CHANNEL, *PCONTROL_CHANNEL;

/* Transport (TCP/UDP) endpoint context structure. The FileObject->FsContext
   field holds a pointer to this structure */
typedef struct _TRANSPORT_CONTEXT {
    union {
        HANDLE AddressHandle;
        CONNECTION_CONTEXT ConnectionContext;
        HANDLE ControlChannel;
    } Handle;
    BOOLEAN CancelIrps;
    KEVENT CleanupEvent;
} TRANSPORT_CONTEXT, *PTRANSPORT_CONTEXT;

typedef struct _TI_QUERY_CONTEXT {
    PIRP Irp;
    PMDL InputMdl;
    PMDL OutputMdl;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;
} TI_QUERY_CONTEXT, *PTI_QUERY_CONTEXT;

#endif /* __TITYPES_H */

/* EOF */
