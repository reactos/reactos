/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Novell Eagle 2000 driver
 * FILE:        include/ne2000.h
 * PURPOSE:     NE2000 driver definitions
 */
#ifndef __NE2000_H
#define __NE2000_H

#ifdef _MSC_VER

#ifndef ULONG_PTR
#define ULONG ULONG_PTR
#endif

#include <ndis.h>
#else
#include <net/ndis.h>
#endif
#include <8390.h>
#include <debug.h>

/* Define NOCARD to test NDIS without a card */
//#define NOCARD

/* NE2000 sepcific constants */
#define NIC_DATA            0x10    /* Data register */
#define NIC_RESET           0x1F    /* Reset register */


/* Global constants */

#define DRIVER_NDIS_MAJOR_VERSION 3
#define DRIVER_NDIS_MINOR_VERSION 0

#define DRIVER_DEFAULT_IO_BASE_ADDRESS      0x300
#define DRIVER_DEFAULT_INTERRUPT_NUMBER     10

#define DRIVER_MAX_MULTICAST_LIST_SIZE  8

#define DRIVER_VENDOR_DESCRIPTION       "Novell Eagle 2000 Adapter."
#define DRIVER_VENDOR_DRIVER_VERSION    0x0100  /* 1.0 */

#define DRIVER_FRAME_SIZE           1514    /* Size of an ethernet frame */
#define DRIVER_HEADER_SIZE          14      /* Size of an ethernet header */
#define DRIVER_LENGTH_OF_ADDRESS    6       /* Size of an ethernet address */

/* Maximum lookahead buffer size */
#define DRIVER_MAXIMUM_LOOKAHEAD (252 - DRIVER_HEADER_SIZE)

/* Size of a block in a buffer ring */
#define DRIVER_BLOCK_SIZE   256


/* Default number of transmit buffers */
#define DRIVER_DEFAULT_TX_BUFFER_COUNT 12
#define BUFFERS_PER_TX_BUF 1

/* Interrupt Mask Register value */
#define DRIVER_INTERRUPT_MASK   IMR_ALLE - IMR_RDCE



/* Global structures */

typedef struct _MINIPORT_RESERVED
{
    PNDIS_PACKET Next;
} MINIPORT_RESERVED, *PMINIPORT_RESERVED;

#define RESERVED(Packet) ((PMINIPORT_RESERVED)((Packet)->u.s1.MiniportReserved))

typedef UCHAR DRIVER_HARDWARE_ADDRESS[DRIVER_LENGTH_OF_ADDRESS];

/* Information about an adapter */
typedef struct _NIC_ADAPTER
{
    /* Entry on global adapter list */
    LIST_ENTRY ListEntry;
    /* Adapter handle */
    NDIS_HANDLE MiniportAdapterHandle;
    /* NDIS interrupt object */
    NDIS_MINIPORT_INTERRUPT Interrupt;

    /* I/O base address and interrupt number of adapter */
    ULONG IoBaseAddress;
    ULONG InterruptNumber;

    /* Mapped address of the I/O base port */
    PUCHAR IOBase;

    /* TRUE if the NIC can transfer in word mode */
    BOOLEAN WordMode;

    /* Base address and size of the onboard memory window */
    PUCHAR RamBase;
    UINT RamSize;

    /* Station Address PROM (SAPROM) */
    UCHAR SAPROM[16];

    /* Onboard ethernet address from the manufacturer */
    DRIVER_HARDWARE_ADDRESS PermanentAddress;

    /* Ethernet address currently in use */
    DRIVER_HARDWARE_ADDRESS StationAddress;

    /* Maximum number of multicast addresses this adapter supports */
    ULONG MaxMulticastListSize;

    /* List of multicast addresses in use */
    DRIVER_HARDWARE_ADDRESS Addresses[DRIVER_MAX_MULTICAST_LIST_SIZE];

    /* Current multicast address mask */
    UCHAR MulticastAddressMask[8];

    /* Masked interrupts (IMR value) */
    ULONG InterruptMask;

    /* Interrupts that have occurred */
    UCHAR InterruptStatus;

    /* Current packet filter */
    ULONG PacketFilter;

    /* Lookahead buffer */
    UINT LookaheadSize;
    UCHAR Lookahead[DRIVER_MAXIMUM_LOOKAHEAD + DRIVER_HEADER_SIZE];

    /* Receive buffer ring */
    UINT PageStart;
    UINT PageStop;
    UINT CurrentPage;
    UINT NextPacket;

    /* TRUE if there was a buffer overflow */
    BOOLEAN BufferOverflow;

    /* TRUE if an error occurred during reception of a packet */
    BOOLEAN ReceiveError;

    /* TRUE if an error occurred during transmission of a packet */
    BOOLEAN TransmitError;

    /* TRUE if a transmit interrupt is pending */
    BOOLEAN TransmitPending;

    /* Received packet header */
    PACKET_HEADER PacketHeader;

    /* Offset in onboard RAM of received packet */
    ULONG PacketOffset;

    /* TRUE if receive indications are done and should be completed */
    BOOLEAN DoneIndicating;

    /* Transmit buffers */
    UINT TXStart;   /* Start block of transmit buffer ring */
    UINT TXCount;   /* Number of blocks in transmit buffer ring */
    UINT TXFree;    /* Number of free transmit buffers */
    UINT TXNext;    /* Next buffer to use */
    /* Length of packet. 0 means buffer is unused */
    INT TXSize[DRIVER_DEFAULT_TX_BUFFER_COUNT];
    INT TXCurrent;  /* Current buffer beeing transmitted. -1 means none */

    /* Head of transmit queue */
    PNDIS_PACKET TXQueueHead;
    /* Tail of transmit queue */
    PNDIS_PACKET TXQueueTail;

    /* Statistics */
    ULONG FrameAlignmentErrors;
    ULONG CrcErrors;
    ULONG MissedPackets;

    /* Flags used for driver cleanup */
    BOOLEAN IOPortRangeRegistered;
    BOOLEAN InterruptRegistered;
} NIC_ADAPTER, *PNIC_ADAPTER;

/* Global driver information */
typedef struct _DRIVER_INFORMATION
{
    NDIS_HANDLE NdisWrapperHandle;  /* Returned from NdisInitializeWrapper */
    NDIS_HANDLE NdisMacHandle;      /* Returned from NdisRegisterMac */
    LIST_ENTRY  AdapterListHead;    /* Adapters this driver control */
} DRIVER_INFORMATION, *PDRIVER_INFORMATION;



/* Global variable */

extern DRIVER_INFORMATION DriverInfo;
extern NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;



/* Prototypes */

BOOLEAN NICCheck(
    PNIC_ADAPTER Adapter);

NDIS_STATUS NICInitialize(
    PNIC_ADAPTER Adapter);

NDIS_STATUS NICSetup(
    PNIC_ADAPTER Adapter);

NDIS_STATUS NICStart(
    PNIC_ADAPTER Adapter);

NDIS_STATUS NICStop(
    PNIC_ADAPTER Adapter);

NDIS_STATUS NICReset(
    PNIC_ADAPTER Adapter);

VOID NICUpdateCounters(
    PNIC_ADAPTER Adapter);

VOID NICReadDataAlign(
    PNIC_ADAPTER Adapter,
    PUSHORT Target,
    ULONG Source,
    USHORT Length);

VOID NICWriteDataAlign(
    PNIC_ADAPTER Adapter,
    ULONG Target,
    PUSHORT Source,
    USHORT Length);

VOID NICReadData(
    PNIC_ADAPTER Adapter,
    PUCHAR Target,
    ULONG Source,
    USHORT Length);

VOID NICWriteData(
    PNIC_ADAPTER Adapter,
    ULONG Target,
    PUCHAR Source,
    USHORT Length);

VOID NICTransmit(
    PNIC_ADAPTER Adapter);

#endif /* __NE2000_H */

/* EOF */
