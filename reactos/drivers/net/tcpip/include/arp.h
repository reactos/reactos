/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/arp.h
 * PURPOSE:     Address Resolution Protocol definitions
 */
#ifndef __ARP_H
#define __ARP_H

typedef struct ARP_HEADER {
    USHORT HWType;       /* Hardware Type */
    USHORT ProtoType;    /* Protocol Type */
    UCHAR  HWAddrLen;    /* Hardware Address Length */
    UCHAR  ProtoAddrLen; /* Protocol Address Length */
    USHORT Opcode;       /* Opcode */
    /* Sender's Hardware Address */
    /* Sender's Protocol Address */
    /* Target's Hardware Address */
    /* Target's Protocol Address */
} ARP_HEADER, *PARP_HEADER;

/* We swap constants so we can compare values at runtime without swapping them */
#define ARP_OPCODE_REQUEST WH2N(0x0001) /* ARP request */
#define ARP_OPCODE_REPLY   WH2N(0x0002) /* ARP reply */


BOOLEAN ARPTransmit(
    PIP_ADDRESS Address,
    PNET_TABLE_ENTRY NTE);

VOID ARPReceive(
    PVOID Context,
    PIP_PACKET Packet);

#endif /* __ARP_H */

/* EOF */
