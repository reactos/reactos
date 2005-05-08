/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/transmit.h
 * PURPOSE:     Internet Protocol transmit prototypes
 */
#ifndef __TRANSMIT_H
#define __TRANSMIT_H

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
    PIP_TRANSMIT_COMPLETE Complete;     /* Completion Routine */
    PVOID Context;                      /* Completion Context */
} IPFRAGMENT_CONTEXT, *PIPFRAGMENT_CONTEXT;


NTSTATUS IPSendDatagram(PIP_PACKET IPPacket, PNEIGHBOR_CACHE_ENTRY NCE,
			PIP_TRANSMIT_COMPLETE Complete, PVOID Context);

#endif /* __TRANSMIT_H */

/* EOF */
