/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/protocol.h
 * PURPOSE:     Definitions for routines used by NDIS protocol drivers
 */

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

typedef struct _PROTOCOL_BINDING {
    LIST_ENTRY                    ListEntry;        /* Entry on global list */
    KSPIN_LOCK                    Lock;             /* Protecting spin lock */
    NDIS_PROTOCOL_CHARACTERISTICS Chars;            /* Characteristics */
    WORK_QUEUE_ITEM               WorkItem;         /* Work item */
    LIST_ENTRY                    AdapterListHead;  /* List of adapter bindings */
} PROTOCOL_BINDING, *PPROTOCOL_BINDING;

#define GET_PROTOCOL_BINDING(Handle)((PPROTOCOL_BINDING)Handle)


typedef struct _ADAPTER_BINDING {
    NDIS_OPEN_BLOCK NdisOpenBlock;                            /* NDIS defined fields */

    LIST_ENTRY        ListEntry;                /* Entry on global list */
    LIST_ENTRY        ProtocolListEntry;        /* Entry on protocol binding adapter list */
    LIST_ENTRY        AdapterListEntry;         /* Entry on logical adapter list */
    KSPIN_LOCK        Lock;                     /* Protecting spin lock */
    PPROTOCOL_BINDING ProtocolBinding;          /* Protocol that opened adapter */
    PLOGICAL_ADAPTER  Adapter;                  /* Adapter opened by protocol */
} ADAPTER_BINDING, *PADAPTER_BINDING;

typedef struct _NDIS_REQUEST_MAC_BLOCK {
    PVOID Unknown1;
    PNDIS_OPEN_BLOCK Binding;
    PVOID Unknown3;
    PVOID Unknown4;
} NDIS_REQUEST_MAC_BLOCK, *PNDIS_REQUEST_MAC_BLOCK;

#define GET_ADAPTER_BINDING(Handle)((PADAPTER_BINDING)Handle)


extern LIST_ENTRY ProtocolListHead;
extern KSPIN_LOCK ProtocolListLock;


NDIS_STATUS
ProIndicatePacket(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet);

VOID NTAPI
ProSendPackets(
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets);

#endif /* __PROTOCOL_H */

/* EOF */

