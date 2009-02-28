/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/neighbor.h
 * PURPOSE:     Neighbor definitions
 */
#ifndef __NEIGHBOR_H
#define __NEIGHBOR_H


#define NB_HASHMASK 0xF /* Hash mask for neighbor cache */

typedef VOID (*PNEIGHBOR_PACKET_COMPLETE)
    ( PVOID Context, PNDIS_PACKET Packet, NDIS_STATUS Status );

typedef struct _NEIGHBOR_PACKET {
    LIST_ENTRY Next;
    PNDIS_PACKET Packet;
    PNEIGHBOR_PACKET_COMPLETE Complete;
    PVOID Context;
} NEIGHBOR_PACKET, *PNEIGHBOR_PACKET;

typedef struct NEIGHBOR_CACHE_TABLE {
    struct NEIGHBOR_CACHE_ENTRY *Cache; /* Pointer to cache */
    KSPIN_LOCK Lock;                    /* Protecting lock */
} NEIGHBOR_CACHE_TABLE, *PNEIGHBOR_CACHE_TABLE;

/* Information about a neighbor */
typedef struct NEIGHBOR_CACHE_ENTRY {
    DEFINE_TAG
    struct NEIGHBOR_CACHE_ENTRY *Next;  /* Pointer to next entry */
    struct NEIGHBOR_CACHE_TABLE *Table; /* Pointer to table */
    UCHAR State;                        /* State of NCE */
    UINT EventTimer;                    /* Ticks since last event */
    UINT EventCount;                    /* Number of events */
    PIP_INTERFACE Interface;            /* Pointer to interface */
    UINT LinkAddressLength;             /* Length of link address */
    PVOID LinkAddress;                  /* Pointer to link address */
    IP_ADDRESS Address;                 /* IP address of neighbor */
    LIST_ENTRY PacketQueue;             /* Packet queue */
} NEIGHBOR_CACHE_ENTRY, *PNEIGHBOR_CACHE_ENTRY;

/* NCE states */
#define NUD_NONE       0x00
#define NUD_INCOMPLETE 0x01
#define NUD_REACHABLE  0x02
#define NUD_STALE      0x04
#define NUD_DELAY      0x08
#define NUD_PROBE      0x10
#define NUD_FAILED     0x20
#define NUD_NOARP      0x40
#define NUD_PERMANENT  0x80

#define NUD_IN_TIMER  (NUD_INCOMPLETE | NUD_DELAY | NUD_PROBE)
#define NUD_VALID     (NUD_REACHABLE | NUD_NOARP | NUD_STALE | NUD_DELAY | \
                       NUD_PROBE | NUD_PERMANENT)
#define NUD_CONNECTED (NUD_PERMANENT | NUD_NOARP | NUD_REACHABLE)


/* Maximum number of retransmissions of multicast solicits */
#define MAX_MULTICAST_SOLICIT 3 /* 3 transmissions */

/* Number of ticks between address resolution messages */
#define RETRANS_TIMER IP_TICKS_SECOND /* One second */


extern NEIGHBOR_CACHE_TABLE NeighborCache[NB_HASHMASK + 1];


VOID NBTimeout(
    VOID);

VOID NBStartup(
    VOID);

VOID NBShutdown(
    VOID);

VOID NBSendSolicit(
    PNEIGHBOR_CACHE_ENTRY NCE);

PNEIGHBOR_CACHE_ENTRY NBAddNeighbor(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Address,
    PVOID LinkAddress,
    UINT LinkAddressLength,
    UCHAR Type);

VOID NBUpdateNeighbor(
    PNEIGHBOR_CACHE_ENTRY NCE,
    PVOID LinkAddress,
    UCHAR State);

PNEIGHBOR_CACHE_ENTRY NBLocateNeighbor(
    PIP_ADDRESS Address);

PNEIGHBOR_CACHE_ENTRY NBFindOrCreateNeighbor(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Address);

BOOLEAN NBQueuePacket(
    PNEIGHBOR_CACHE_ENTRY NCE,
    PNDIS_PACKET NdisPacket,
    PNEIGHBOR_PACKET_COMPLETE PacketComplete,
    PVOID PacketContext);

VOID NBRemoveNeighbor(
    PNEIGHBOR_CACHE_ENTRY NCE);

ULONG NBCopyNeighbors(
    PIP_INTERFACE Interface,
    PIPARP_ENTRY ArpTable);

#endif /* __NEIGHBOR_H */

/* EOF */
