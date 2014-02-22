/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/transmit.h
 * PURPOSE:     Internet Protocol transmit prototypes
 */

#pragma once

typedef VOID (*PIP_TRANSMIT_COMPLETE)( PVOID Context,
				       PNDIS_PACKET Packet,
				       NDIS_STATUS Status );

/* IP fragment context information */
typedef struct IPFRAGMENT_CONTEXT {
    struct IPFRAGMENT_CONTEXT *Next;    /* Pointer to next in list */
    PNDIS_PACKET Datagram;              /* Pointer to original NDIS packet */
    PVOID DatagramData;                 /* Pointer to datagram data */
    UINT HeaderSize;                    /* IP datagram header size */
    PNDIS_PACKET NdisPacket;            /* Pointer to NDIS packet */
    PNDIS_BUFFER NdisBuffer;            /* Pointer to NDIS buffer */
    PVOID Header;                       /* Pointer to IP header in fragment buffer */
    PVOID Data;                         /* Pointer to fragment data */
    UINT Position;                      /* Current fragment offset */
    UINT BytesLeft;                     /* Number of bytes left to send */
    UINT PathMTU;                       /* Path Maximum Transmission Unit */
    PNEIGHBOR_CACHE_ENTRY NCE;          /* Pointer to NCE to use */
    KEVENT Event;                       /* Signalled when the transmission is complete */
    NDIS_STATUS Status;                 /* Status of the transmission */
} IPFRAGMENT_CONTEXT, *PIPFRAGMENT_CONTEXT;


NTSTATUS IPSendDatagram(PIP_PACKET IPPacket, PNEIGHBOR_CACHE_ENTRY NCE);

/* EOF */
