/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for routines used by NDIS miniport drivers
 */

#ifndef __MINIPORT_H
#define __MINIPORT_H

#include <ndissys.h>

#define ETH_LENGTH_OF_ADDRESS 6

typedef struct _HARDWARE_ADDRESS {
    union {
        UCHAR Medium802_3[ETH_LENGTH_OF_ADDRESS];
    } Type;
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

/* Information about a miniport */
typedef struct _MINIPORT_DRIVER {
    LIST_ENTRY                      ListEntry;          /* Entry on global list */
    KSPIN_LOCK                      Lock;               /* Protecting spin lock */
    ULONG                           RefCount;           /* Reference count */
    NDIS_MINIPORT_CHARACTERISTICS   Chars;              /* Miniport characteristics */
    WORK_QUEUE_ITEM                 WorkItem;           /* Work item */
    PDRIVER_OBJECT                  DriverObject;       /* Driver object of miniport */
    LIST_ENTRY                      AdapterListHead;    /* Adapters created by miniport */
} MINIPORT_DRIVER, *PMINIPORT_DRIVER;

#define GET_MINIPORT_DRIVER(Handle)((PMINIPORT_DRIVER)Handle)

/* Information about a logical adapter */
typedef struct _LOGICAL_ADAPTER {
    NDIS_MINIPORT_BLOCK NdisMiniportBlock;                                /* NDIS defined fields */

    KDPC                        MiniportDpc;            /* DPC routine for adapter */
    BOOLEAN                     MiniportBusy;           /* A MiniportXxx routine is executing */
    NDIS_HANDLE                 MiniportAdapterBinding; /* Binding handle for current caller */
    ULONG                       WorkQueueLevel;         /* Number of used work item buffers */
    NDIS_MINIPORT_WORK_ITEM     WorkQueue[NDIS_MINIPORT_WORK_QUEUE_SIZE];
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueHead;          /* Head of work queue */
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueTail;          /* Tail of work queue */

    LIST_ENTRY                  ListEntry;              /* Entry on global list */
    LIST_ENTRY                  MiniportListEntry;      /* Entry on miniport driver list */
    LIST_ENTRY                  ProtocolListHead;       /* List of bound protocols */
    ULONG                       RefCount;               /* Reference count */
    PMINIPORT_DRIVER            Miniport;               /* Miniport owning this adapter */
    UNICODE_STRING              DeviceName;             /* Device name of this adapter */
    ULONG                       Attributes;             /* Attributes of adapter */
    /* TRUE if the miniport has called NdisSetAttributes(Ex) for this adapter */
    BOOLEAN                     AttributesSet;
    PVOID                       QueryBuffer;            /* Buffer to use for queries */
    ULONG                       QueryBufferLength;      /* Length of QueryBuffer */
    ULONG                       MediumHeaderSize;       /* Size of medium header */
    HARDWARE_ADDRESS            Address;                /* Hardware address of adapter */
    ULONG                       AddressLength;          /* Length of hardware address */
    PUCHAR                      LookaheadBuffer;        /* Pointer to lookahead buffer */
    ULONG                       LookaheadLength;        /* Length of lookahead buffer */
    ULONG                       CurLookaheadLength;     /* Current (selected) length of lookahead buffer */
    ULONG                       MaxLookaheadLength;     /* Maximum length of lookahead buffer */

    PNDIS_PACKET                PacketQueueHead;        /* Head of packet queue */
    PNDIS_PACKET                PacketQueueTail;        /* Head of packet queue */

    PNDIS_PACKET                LoopPacket;             /* Current packet beeing looped */
} LOGICAL_ADAPTER, *PLOGICAL_ADAPTER;

#define GET_LOGICAL_ADAPTER(Handle)((PLOGICAL_ADAPTER)Handle)

extern LIST_ENTRY MiniportListHead;
extern KSPIN_LOCK MiniportListLock;
extern LIST_ENTRY AdapterListHead;
extern KSPIN_LOCK AdapterListLock;


#ifdef DBG
VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet);
#endif /* DBG */

VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize);

BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet);

PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName);

NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PULONG              BytesWritten);

NDIS_STATUS
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE WorkItemType,
    PVOID               WorkItemContext,
    NDIS_HANDLE         Initiator);

NDIS_STATUS
FASTCALL
MiniDequeueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE *WorkItemType,
    PVOID               *WorkItemContext,
    NDIS_HANDLE         *Initiator);

NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest);

#endif /* __MINIPORT_H */

/* EOF */
