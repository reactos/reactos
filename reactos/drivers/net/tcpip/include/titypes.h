/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/titypes.h
 * PURPOSE:     TCP/IP protocol driver types
 */
#ifndef __TITYPES_H
#define __TITYPES_H

#include <ip.h>


#ifdef DBG

#define DEBUG_REFCHECK(Object) {            \
    if ((Object)->RefCount <= 0) {          \
        TI_DbgPrint(MIN_TRACE, ("Object at (0x%X) has invalid reference count (%d).\n", \
            (Object), (Object)->RefCount)); \
        }                                   \
}

#else

#define DEBUG_REFCHECK(Object)

#endif


/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)                  \
{                                                \
    DEBUG_REFCHECK(Object);                      \
    TI_DbgPrint(DEBUG_REFCOUNT, ("Referencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));          \
                                                 \
    InterlockedIncrement(&((Object)->RefCount)); \
}

/*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)                         \
{                                                         \
    DEBUG_REFCHECK(Object);                               \
    TI_DbgPrint(DEBUG_REFCOUNT, ("Dereferencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));                   \
                                                          \
    if (InterlockedDecrement(&((Object)->RefCount)) == 0) \
        PoolFreeBuffer(Object);                           \
}


typedef NTSTATUS (*DATAGRAM_SEND_ROUTINE)(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize);

/* Datagram completion handler prototype */
typedef VOID (*DATAGRAM_COMPLETION_ROUTINE)(
    PVOID Context,
    NDIS_STATUS Status,
    ULONG Count);

typedef struct _DATAGRAM_RECEIVE_REQUEST {
    LIST_ENTRY ListEntry;                   /* Entry on list */
    PIP_ADDRESS RemoteAddress;              /* Remote address we receive from (NULL means any) */
    USHORT RemotePort;                      /* Remote port we receive from (0 means any) */
    PTDI_CONNECTION_INFORMATION ReturnInfo; /* Return information */
    PNDIS_BUFFER Buffer;                    /* Pointer to receive buffer */
    ULONG BufferSize;                       /* Size of Buffer */
    DATAGRAM_COMPLETION_ROUTINE Complete;   /* Completion routine */
    PVOID Context;                          /* Pointer to context information */
} DATAGRAM_RECEIVE_REQUEST, *PDATAGRAM_RECEIVE_REQUEST;

/* Datagram build routine prototype */
typedef NTSTATUS (*DATAGRAM_BUILD_ROUTINE)(
    PVOID Context,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET *IPPacket);

typedef struct _DATAGRAM_SEND_REQUEST {
    LIST_ENTRY ListEntry;                 /* Entry on list */
    PIP_ADDRESS RemoteAddress;            /* Pointer to remote IP address */
    USHORT RemotePort;                    /* Remote port number */
    PNDIS_BUFFER Buffer;                  /* Pointer to NDIS buffer to send */
    DWORD BufferSize;                     /* Size of Buffer */
    DATAGRAM_COMPLETION_ROUTINE Complete; /* Completion routine */
    PVOID Context;                        /* Pointer to context information */
    DATAGRAM_BUILD_ROUTINE Build;         /* Datagram build routine */
} DATAGRAM_SEND_REQUEST, *PDATAGRAM_SEND_REQUEST;


/* Transport (TCP/UDP) endpoint context structure. The FileObject->FsContext
   field holds a pointer to this structure */
typedef struct _TRANSPORT_CONTEXT {
    union {
        HANDLE AddressHandle;
        CONNECTION_CONTEXT ConnectionContext;
        HANDLE ControlChannel;
    } Handle;
    ULONG RefCount;
    BOOL CancelIrps;
    KEVENT CleanupEvent;
} TRANSPORT_CONTEXT, *PTRANSPORT_CONTEXT;


typedef struct _ADDRESS_FILE {
    LIST_ENTRY ListEntry;                 /* Entry on list */
    KSPIN_LOCK Lock;                      /* Spin lock to manipulate this structure */
    ULONG RefCount;                       /* Number of references to this object */
    USHORT Flags;                         /* Flags for address file (see below) */
    PADDRESS_ENTRY ADE;                   /* Associated address entry */
    USHORT Protocol;                      /* Protocol number */
    USHORT Port;                          /* Network port (network byte order) */
    WORK_QUEUE_ITEM WorkItem;             /* Work queue item handle */
    DATAGRAM_COMPLETION_ROUTINE Complete; /* Completion routine for delete request */
    PVOID Context;                        /* Delete request context */
    DATAGRAM_SEND_ROUTINE Send;           /* Routine to send a datagram */
    LIST_ENTRY ReceiveQueue;              /* List of outstanding receive requests */
    LIST_ENTRY TransmitQueue;             /* List of outstanding transmit requests */
    PIP_ADDRESS AddrCache;                /* One entry address cache (destination
                                             address of last packet transmitted) */

    /* The following members are used to control event notification */

    /* Connection indication handler */
    PTDI_IND_CONNECT ConnectionHandler;
    PVOID ConnectionHandlerContext;
    BOOL RegisteredConnectionHandler;
    /* Disconnect indication handler */
    PTDI_IND_DISCONNECT DisconnectHandler;
    PVOID DisconnectHandlerContext;
    BOOL RegisteredDisconnectHandler;
    /* Receive indication handler */
    PTDI_IND_RECEIVE ReceiveHandler;
    PVOID ReceiveHandlerContext;
    BOOL RegisteredReceiveHandler;
    /* Expedited receive indication handler */
    PTDI_IND_RECEIVE_EXPEDITED ExpeditedReceiveHandler;
    PVOID ExpeditedReceiveHandlerContext;
    BOOL RegisteredExpeditedReceiveHandler;
    /* Receive datagram indication handler */
    PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
    PVOID ReceiveDatagramHandlerContext;
    BOOL RegisteredReceiveDatagramHandler;
    /* Error indication handler */
    PTDI_IND_ERROR ErrorHandler;
    PVOID ErrorHandlerContext;
    PVOID ErrorHandlerOwner;
    BOOL RegisteredErrorHandler;
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

/* Transport connection context structure. The FileObject->FsContext2
   field holds a pointer to this structure */
typedef struct _CONNECTION_ENDPOINT {
    LIST_ENTRY ListEntry;   /* Entry on list */
    KSPIN_LOCK Lock;        /* Spin lock to protect this structure */
    ULONG RefCount;         /* Number of references to this object */
} CONNECTION_ENDPOINT, *PCONNECTION_ENDPOINT;

/* Transport control channel context structure. The FileObject->FsContext2
   field holds a pointer to this structure */
typedef struct _CONTROL_CHANNEL {
    LIST_ENTRY ListEntry;       /* Entry on list */
    KSPIN_LOCK Lock;            /* Spin lock to protect this structure */
    ULONG RefCount;             /* Number of references to this object */
} CONTROL_CHANNEL, *PCONTROL_CHANNEL;

typedef struct _TI_QUERY_CONTEXT {
    PIRP Irp;
    PMDL InputMdl;
    PMDL OutputMdl;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;
} TI_QUERY_CONTEXT, *PTI_QUERY_CONTEXT;

#endif /* __TITYPES_H */

/* EOF */
