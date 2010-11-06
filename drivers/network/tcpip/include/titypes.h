/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/titypes.h
 * PURPOSE:     TCP/IP protocol driver types
 */

#pragma once

/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)                            \
{                                                          \
    InterlockedIncrement(&((Object)->RefCount));           \
}

/*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)                           \
{                                                           \
    if (InterlockedDecrement(&((Object)->RefCount)) == 0)   \
        (((Object)->Free)(Object));                         \
}

/*
 * VOID LockObject(PVOID Object, PKIRQL OldIrql)
 */
#define LockObject(Object, Irql)                         \
{                                                        \
    ReferenceObject(Object);                             \
    KeAcquireSpinLock(&((Object)->Lock), Irql);          \
    memcpy(&(Object)->OldIrql, Irql, sizeof(KIRQL));     \
}

/*
 * VOID LockObjectAtDpcLevel(PVOID Object)
 */
#define LockObjectAtDpcLevel(Object)                     \
{                                                        \
    ReferenceObject(Object);                             \
    KeAcquireSpinLockAtDpcLevel(&((Object)->Lock));      \
    (Object)->OldIrql = DISPATCH_LEVEL;                  \
}

/*
 * VOID UnlockObject(PVOID Object, KIRQL OldIrql)
 */
#define UnlockObject(Object, OldIrql)                       \
{                                                           \
    KeReleaseSpinLock(&((Object)->Lock), OldIrql);          \
    DereferenceObject(Object);                              \
}

/*
 * VOID UnlockObjectFromDpcLevel(PVOID Object)
 */
#define UnlockObjectFromDpcLevel(Object)                    \
{                                                           \
    KeReleaseSpinLockFromDpcLevel(&((Object)->Lock));       \
    DereferenceObject(Object);                              \
}



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
    LIST_ENTRY ListEntry;                 /* Entry on list */
    LONG RefCount;                        /* Reference count */
    OBJECT_FREE_ROUTINE Free;             /* Routine to use to free resources for the object */
    KSPIN_LOCK Lock;                      /* Spin lock to manipulate this structure */
    KIRQL OldIrql;                        /* Currently not used */
    IP_ADDRESS Address;                   /* Address of this address file */
    USHORT Family;                        /* Address family */
    USHORT Protocol;                      /* Protocol number */
    USHORT Port;                          /* Network port (network byte order) */
    UCHAR TTL;                            /* Time to live stored in packets sent from this address file */
    UINT DF;                              /* Don't fragment */
    UINT BCast;                           /* Receive broadcast packets */
    UINT HeaderIncl;                      /* Include header in RawIP packets */
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
    NTSTATUS Status;
    ULONG Information;
} TDI_BUCKET, *PTDI_BUCKET;

/* Transport connection context structure A.K.A. Transmission Control Block
   (TCB) in TCP terminology. The FileObject->FsContext2 field holds a pointer
   to this structure */
typedef struct _CONNECTION_ENDPOINT {
    LIST_ENTRY ListEntry;       /* Entry on list */
    LONG RefCount;              /* Reference count */
    OBJECT_FREE_ROUTINE Free;   /* Routine to use to free resources for the object */
    KSPIN_LOCK Lock;            /* Spin lock to protect this structure */
    KIRQL OldIrql;              /* The old irql is stored here for use in HandleSignalledConnection */
    PVOID ClientContext;        /* Pointer to client context information */
    PADDRESS_FILE AddressFile;  /* Associated address file object (NULL if none) */
    PVOID SocketContext;        /* Context for lower layer */

    /* Requests */
    LIST_ENTRY ConnectRequest; /* Queued connect rqueusts */
    LIST_ENTRY ListenRequest;  /* Queued listen requests */
    LIST_ENTRY ReceiveRequest; /* Queued receive requests */
    LIST_ENTRY SendRequest;    /* Queued send requests */
    LIST_ENTRY CompletionQueue;/* Completed requests to finish */

    /* Signals */
    UINT    SignalState;       /* Active signals from oskit */
} CONNECTION_ENDPOINT, *PCONNECTION_ENDPOINT;



/*************************
* TDI support structures *
*************************/

/* Transport control channel context structure. The FileObject->FsContext2
   field holds a pointer to this structure */
typedef struct _CONTROL_CHANNEL {
    LIST_ENTRY ListEntry;       /* Entry on list */
    LONG RefCount;              /* Reference count */
    OBJECT_FREE_ROUTINE Free;   /* Routine to use to free resources for the object */
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

/* EOF */
