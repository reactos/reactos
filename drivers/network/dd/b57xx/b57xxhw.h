/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   Copyright 2021-2022 Scott Maday <coldasdryice1@gmail.com>
 */
#pragma once

#define IEEE_802_ADDR_LENGTH 6

#define MAXIMUM_MULTICAST_ADDRESSES 16

#define MAX_ATTEMPTS        1000
#define MAX_ATTEMPTS_PHY    5000

#define NUM_RX_BUFFER_DESCRIPTORS   512
#define NUM_RX_RETURN_DESCRIPTORS   1024
#define NUM_TX_BUFFER_DESCRIPTORS   512

#define B57XX_ENDIAN_BYTESWAP           0
#define B57XX_ENDIAN_WORDSWAP           1
#define B57XX_ENDIAN_BYTESWAP_DATA      1
#define B57XX_ENDIAN_WORDSWAP_DATA      1
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define B57XX_ENDIAN_BYTESWAP_NFDATA    0
#define B57XX_ENDIAN_WORDSWAP_NFDATA    1
#else
#define B57XX_ENDIAN_BYTESWAP_NFDATA    1
#define B57XX_ENDIAN_WORDSWAP_NFDATA    1
#endif

#define B57XX_ENDIAN_SWAP(ByteSwap, ByteSwapReg, WordSwap, WordSwapReg)                            \
    ((ByteSwap ? ByteSwapReg : 0) | (WordSwap ? WordSwapReg : 0))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define B57XX_PAIRED_WORD(Word1, Word2)                                                            \
    struct                                                                                         \
    {                                                                                              \
        USHORT Word2;                                                                              \
        USHORT Word1;                                                                              \
    }
#else
#define B57XX_PAIRED_WORD(Word1, Word2)                                                            \
    struct                                                                                         \
    {                                                                                              \
        USHORT Word1;                                                                              \
        USHORT Word2;                                                                              \
    }
#endif

#define BIT(x)  (1 << x)

typedef UCHAR MAC_ADDRESS[IEEE_802_ADDR_LENGTH], *PMAC_ADDRESS;

#include <pshpack1.h>

/* Ethernet frame header */
typedef struct _ETH_HEADER
{
    MAC_ADDRESS Destination;
    MAC_ADDRESS Source;
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;

/* B47XX NICs are big endian and need addresses in big endian format */
typedef struct _B57XX_PHYSICAL_ADDRESS
{
    LONG HighPart;
    ULONG LowPart;
} B57XX_PHYSICAL_ADDRESS, *PB57XX_PHYSICAL_ADDRESS;
C_ASSERT(sizeof(B57XX_PHYSICAL_ADDRESS) == 8);

typedef struct _B57XX_RING_CONTROL_BLOCK
{
    B57XX_PHYSICAL_ADDRESS HostRingAddress;
    B57XX_PAIRED_WORD
    (
        MaxLength,
        Flags
    );
    ULONG NICRingAddress;
} B57XX_RING_CONTROL_BLOCK, *PB57XX_RING_CONTROL_BLOCK;

typedef struct _B57XX_SEND_BUFFER_DESCRIPTOR
{
    B57XX_PHYSICAL_ADDRESS HostAddress;
    B57XX_PAIRED_WORD
    (
        Length,
        Flags
    );
    B57XX_PAIRED_WORD
    (
        Reserved,
        VLANTag
    );
} B57XX_SEND_BUFFER_DESCRIPTOR, *PB57XX_SEND_BUFFER_DESCRIPTOR;
C_ASSERT(sizeof(B57XX_SEND_BUFFER_DESCRIPTOR) == 16);

typedef struct _B57XX_RECEIVE_BUFFER_DESCRIPTOR
{
    B57XX_PHYSICAL_ADDRESS HostAddress;
    B57XX_PAIRED_WORD
    (
        Index,
        Length
    );
    B57XX_PAIRED_WORD
    (
        Type,
        Flags
    );
    B57XX_PAIRED_WORD
    (
        IPChecksum,
        TransportChecksum
    );
    B57XX_PAIRED_WORD
    (
        ErrorFlags,
        VLANTag
    );
    ULONG Reserved;
    ULONG Opaque;
} B57XX_RECEIVE_BUFFER_DESCRIPTOR, *PB57XX_RECEIVE_BUFFER_DESCRIPTOR;
C_ASSERT(sizeof(B57XX_RECEIVE_BUFFER_DESCRIPTOR) == 32);

typedef B57XX_PAIRED_WORD
(
    SendConsumerIndex, // Send ring
    ReceiveProducerIndex // Return ring
) B57XX_RING_INDEX_PAIR, *PB57XX_RING_INDEX_PAIR;

typedef struct _B57XX_STATUS_BLOCK
{
    ULONG Status;
    ULONG StatusTag;
    B57XX_PAIRED_WORD
    (
        ReceiveStandardConsumerIndex,
        ReceiveJumboConsumerIndex // Unused
    );
    B57XX_PAIRED_WORD
    (
        Reserved,
        ReceiveMiniConsumerIndex // Unused
    );
    B57XX_RING_INDEX_PAIR RingIndexPairs[1];
} B57XX_STATUS_BLOCK, *PB57XX_STATUS_BLOCK;
C_ASSERT(sizeof(B57XX_STATUS_BLOCK) == 20);

typedef ULONG B57XX_STATISTICS_BLOCK[512], *PB57XX_STATISTICS_BLOCK;
C_ASSERT(sizeof(B57XX_STATISTICS_BLOCK) == 2048);

#include <poppack.h>

/* Custom Blocks for general information and important hardware states */

typedef struct _B57XX_RECEIVE_PRODUCER_BLOCK
{
    B57XX_RING_CONTROL_BLOCK RingControlBlock;
    PB57XX_RECEIVE_BUFFER_DESCRIPTOR pRing; // Virtual host address
    ULONG Count; // Number of elements in the ring
    ULONG Index; // Host producer index
    PUCHAR HostBuffer; // Virtual host address
    ULONG FrameBufferLength; // Aligned size of the buffer for each received frame
} B57XX_RECEIVE_PRODUCER_BLOCK, *PB57XX_RECEIVE_PRODUCER_BLOCK;

typedef struct _B57XX_RECEIVE_CONSUMER_BLOCK
{
    B57XX_RING_CONTROL_BLOCK RingControlBlock;
    PB57XX_RECEIVE_BUFFER_DESCRIPTOR pRing; // Virtual host address
    ULONG Count; // Number of elements in the ring
    ULONG Index; // Host consumer index (tail)
} B57XX_RECEIVE_CONSUMER_BLOCK, *PB57XX_RECEIVE_CONSUMER_BLOCK;

typedef struct _B57XX_SEND_BLOCK
{
    B57XX_RING_CONTROL_BLOCK RingControlBlock;
    PB57XX_SEND_BUFFER_DESCRIPTOR pRing; // Virtual host address
    ULONG Count; // Number of elements in the ring
    BOOLEAN RingFull;
    PPNDIS_PACKET pPacketList;
    ULONG ProducerIndex; // Host transmit send producer index (head)
    ULONG ConsumerIndex; // Host completed transmission consumer index (tail)
} B57XX_SEND_BLOCK, *PB57XX_SEND_BLOCK;

/* Devices */

typedef enum _B57XX_DEVICE_ID
{
    // Programmer's Guide: BCM57XX
    B5700,
    B5701,
    B5702,
    B5703C,
    B5703S,
    B5704C,
    B5704S,
    B5705,
    B5705M,
    B5788,
    B5721,
    B5751,
    B5751M,
    B5752,
    B5752M,
    B5714C,
    B5714S,
    B5715C,
    B5715S,
    // Programmer's Guide: BCM5756M
    B5722,
    B5755,
    B5755M,
    B5754,
    B5754M,
    B5756M,
    B5757,
    B5786,
    B5787,
    B5787M,
    // Programmer's Guide: BCM5764M
    B5784M,
    B5764M,
} B57XX_DEVICE_ID, *PB57XX_DEVICE_ID;

/* Ring Control Block Flags */
#define B57XX_RCB_FLAG_USE_EXT_RECV_BD  BIT(0)
#define B57XX_RCB_FLAG_RING_DISABLED    BIT(1)

/* Send Buffer Descriptor Flags */
#define B57XX_SBD_TCP_UDP_CKSUM     BIT(0)
#define B57XX_SBD_IP_CKSUM          BIT(1)
#define B57XX_SBD_PACKET_END        BIT(2)
#define B57XX_SBD_IP_FRAG           BIT(3)
#define B57XX_SBD_IP_FRAG_END       BIT(4)
#define B57XX_SBD_VLAN_TAG          BIT(6)
#define B57XX_SBD_COAL_NOW          BIT(7)
#define B57XX_SBD_CPU_PRE_DMA       BIT(8)
#define B57XX_SBD_CPU_POST_DMA      BIT(9)
#define B57XX_SBD_INSERT_SRC_ADDR   BIT(12)
#define B57XX_SBD_DONT_GEN_CRC      BIT(15)

/* Receive Buffer Descriptor Flags */
#define B57XX_RBD_PACKET_END            BIT(2)
#define B57XX_RBD_BD_FLAG_JUMBO_RING    BIT(5)
#define B57XX_RBD_VLAN_TAG              BIT(6)
#define B57XX_RBD_FRAME_HAS_ERROR       BIT(10)
#define B57XX_RBD_MINI_RING             BIT(11)
#define B57XX_RBD_IP_CHECKSUM           BIT(12)
#define B57XX_RBD_TCP_UDP_CHECKSUM      BIT(13)
#define B57XX_RBD_TCP_UDP_IS_TCP        BIT(14)

/* Receive Buffer Descriptor Error Flags */
#define B57XX_RBD_ERR_BAD_CRC           BIT(0)
#define B57XX_RBD_ERR_COLL_DETECT       BIT(1)
#define B57XX_RBD_ERR_LINK_LOST         BIT(2)
#define B57XX_RBD_ERR_PHY_DECODE_ERR    BIT(3)
#define B57XX_RBD_ERR_ODD_NIBBLE_RX_MII BIT(4)
#define B57XX_RBD_ERR_MAC_ABORT         BIT(5)
#define B57XX_RBD_ERR_LEN_LESS_64       BIT(6)
#define B57XX_RBD_ERR_TRUNC_NO_RES      BIT(7)
#define B57XX_RBD_ERR_GIANT_PKT_RCVD    BIT(8)

/* Status Block Status Word Flags */
#define B57XX_SB_UPDATED    BIT(0)
#define B57XX_SB_LINKSTATE  BIT(1)
#define B57XX_SB_ERROR      BIT(2)

/* NIC Addresses */
#define B57XX_ADDR_SEND_RCBS            0x0100  /* To 0x1FF */
#define B57XX_ADDR_RECEIVE_RETURN_RCBS  0x0200  /* To 0x2FF */
#define B57XX_ADDR_STAT_BLCK            0x0300
#define B57XX_ADDR_STATUS_BLCK          0x0B00
#define B57XX_ADDR_FW_MAILBOX           0x0B50
#define B57XX_ADDR_FW_DRV_STATE_MAILBOX 0x0C04
#define B57XX_ADDR_FW_DRV_WOL_MAILBOX   0x0D30
#define B57XX_ADDR_SEND_RINGS           0x4000
#define B57XX_ADDR_RECEIVE_STD_RINGS    0x6000
#define B57XX_ADDR_RECEIVE_JUMBO_RINGS  0x7000
#define B57XX_ADDR_RECEIVE_MINI_RINGS   0xE000

/* Memory Constants */
#define B57XX_CONST_T3_MAGIC_NUMBER   0x4B657654
#define B57XX_CONST_DRV_STATE_START   0x00000001
#define B57XX_CONST_DRV_STATE_UNLOAD  0x00000002
#define B57XX_CONST_DRV_STATE_WOL     0x00000003
#define B57XX_CONST_DRV_STATE_SUSPEND 0x00000004
#define B57XX_CONST_WOL_MAGIC_NUMBER  0x474C0000

/* Registers */
#define B57XX_REG_VENDORID              0x0000  /* UShort */
#define B57XX_REG_DEVICEID              0x0002  /* UShort */
#define B57XX_REG_COMMAND               0x0004  /* UShort */
#define B57XX_REG_CACHE_LINE_SIZE       0x000C  /* UChar */
#define B57XX_REG_SUBSY_VENDORID        0x002C  /* UShort */
#define B57XX_REG_CAPABILITY_PTR        0x0034  /* UChar */
#define B57XX_REG_PCIX_COMMAND          0x0042  /* UShort */
#define B57XX_REG_POWER_MANAGEMENT      0x004C  /* UShort */
#define B57XX_REG_HW_FIX                0x0066  /* UShort */
#define B57XX_REG_MISC_HOST_CTRL        0x0068  /* ULong */
#define B57XX_REG_DMA_CTRL              0x006C  /* ULong */
#define B57XX_REG_PCI_STATE             0x0070  /* ULong */
#define B57XX_REG_PCI_CLOCK             0x0074  /* ULong */
#define B57XX_REG_REG_BASE              0x0078  /* ULong */
#define B57XX_REG_MEM_BASE              0x007C  /* ULong */
#define B57XX_REG_REG_DATA              0x0080  /* ULong */
#define B57XX_REG_MEM_DATA              0x0084  /* ULong */
#define B57XX_REG_DEVICE_CTRL           0x00D8  /* UShort */
#define B57XX_REG_ADV_ERROR_CAPABILITY  0x0100  /* ULong */
#define B57XX_REG_UNCORRET_ERROR_STATUS 0x0104  /* ULong */
#define B57XX_REG_UNCORRET_ERROR_MASK   0x0108  /* ULong */
#define B57XX_REG_UNCORRET_ERROR_SEVERE 0x010C  /* ULong */
#define B57XX_REG_CORRET_ERROR_STATUS   0x0110  /* ULong */
#define B57XX_REG_CORRET_ERROR_MASK     0x0114  /* ULong */
#define B57XX_REG_ADV_ERROR_CTRL        0x0118  /* ULong */
#define B57XX_REG_ETH_MAC_MODE          0x0400  /* ULong */
#define B57XX_REG_ETH_MAC_STATUS        0x0404  /* ULong */
#define B57XX_REG_ETH_MAC_EVENT         0x0408  /* ULong */
#define B57XX_REG_ETH_LED               0x040C  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR1_HI      0x0410  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR1_LO      0x0414  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR2_HI      0x0418  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR2_LO      0x041C  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR3_HI      0x0420  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR3_LO      0x0424  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR4_HI      0x0428  /* ULong */
#define B57XX_REG_ETH_MAC_ADDR4_LO      0x042C  /* ULong */
#define B57XX_REG_ETH_TX_RND_BACKOFF    0x0438  /* ULong */
#define B57XX_REG_ETH_RX_MTU_SIZE       0x043C  /* ULong */
#define B57XX_REG_ETH_MI_COMMUNICATION  0x044C  /* ULong */
#define B57XX_REG_ETH_MI_STATUS         0x0450  /* ULong */
#define B57XX_REG_ETH_MI_MODE           0x0454  /* ULong */
#define B57XX_REG_ETH_AUTOPOLL_STATUS   0x0458  /* ULong */
#define B57XX_REG_ETH_TX_MODE           0x045C  /* ULong */
#define B57XX_REG_ETH_TX_STATUS         0x0460  /* ULong */
#define B57XX_REG_ETH_TX_LENGTH         0x0464  /* ULong */
#define B57XX_REG_ETH_RX_MODE           0x0468  /* ULong */
#define B57XX_REG_ETH_RX_STATUS         0x046C  /* ULong */
#define B57XX_REG_ETH_MAC_HASH          0x0470  /* ULong, 4 of them to 0x047C */
#define B57XX_REG_ETH_RX_RULES_CTRL     0x0480  /* ULong, 8 of them to 0x4B8 */
#define B57XX_REG_ETH_RX_RULES_VALUE    0x0484  /* ULong, 8 of them to 0x4BC */
#define B57XX_REG_ETH_RX_RULES_CONFIG   0x0500  /* ULong */
#define B57XX_REG_ETH_RX_LOW_WM_MAX     0x0504  /* ULong */
#define B57XX_REG_TX_DATA_MODE          0x0C00  /* ULong */
#define B57XX_REG_TX_DATA_STAT_CTRL     0x0C08  /* ULong */
#define B57XX_REG_TX_DATA_STAT_MASK     0x0C0C  /* ULong */
#define B57XX_REG_TX_COMPL_MODE         0x1000  /* ULong */
#define B57XX_REG_TX_RING_SELECTOR_MODE 0x1400  /* ULong */
#define B57XX_REG_TX_INITIATOR_MODE     0x1800  /* ULong */
#define B57XX_REG_TX_BD_COMPL_MODE      0x1C00  /* ULong */
#define B57XX_REG_RX_LIST_PLACE_MODE    0x2000  /* ULong */
#define B57XX_REG_RX_LIST_PLACE_STATUS  0x2004  /* ULong */
#define B57XX_REG_RX_LIST_PLACE_CONFIG  0x2010  /* ULong */
#define B57XX_REG_RX_LIST_STAT_CTRL     0x2014  /* ULong */
#define B57XX_REG_RX_LIST_STAT_MASK     0x2018  /* ULong */
#define B57XX_REG_RX_DATA_BD_MODE       0x2400  /* ULong */
#define B57XX_REG_RX_DATA_BD_STATUS     0x2404  /* ULong */
#define B57XX_REG_JUMB_RXP_RING_HOST_HI 0x2440  /* ULong */
#define B57XX_REG_JUMB_RXP_RING_HOST_LO 0x2444  /* ULong */
#define B57XX_REG_JUMB_RXP_RING_ATTR    0x2448  /* ULong */
#define B57XX_REG_JUMB_RXP_RING_NIC     0x244C  /* ULong */
#define B57XX_REG_STD_RXP_RING_HOST     0x2450  /* ULongLong */
#define B57XX_REG_STD_RXP_RING_HOST_HI  0x2450  /* ULong */
#define B57XX_REG_STD_RXP_RING_HOST_LO  0x2454  /* ULong */
#define B57XX_REG_STD_RXP_RING_ATTR     0x2458  /* ULong */
#define B57XX_REG_STD_RXP_RING_NIC      0x245C  /* ULong */
#define B57XX_REG_MINI_RXP_RING_HOST_HI 0x2460  /* ULong */
#define B57XX_REG_MINI_RXP_RING_HOST_LO 0x2464  /* ULong */
#define B57XX_REG_MINI_RXP_RING_ATTR    0x2468  /* ULong */
#define B57XX_REG_MINI_RXP_RING_NIC     0x246C  /* ULong */
#define B57XX_REG_RX_DATA_COMPL_MODE    0x2800  /* ULong */
#define B57XX_REG_RX_BD_MODE            0x2C00  /* ULong */
#define B57XX_REG_MINI_RXP_RING_REPL    0x2C14  /* ULong */
#define B57XX_REG_STD_RX_RING_REPL      0x2C18  /* ULong */
#define B57XX_REG_JUMBO_RX_RING_REPL    0x2C1C  /* ULong */
#define B57XX_REG_RX_BD_COMPL_MODE      0x3000  /* ULong */
#define B57XX_REG_RX_BD_COMPL_STATUS    0x3004  /* ULong */
#define B57XX_REG_JUMBO_RXP_INDEX       0x3008  /* ULong */
#define B57XX_REG_STD_RXP_INDEX         0x300C  /* ULong */
#define B57XX_REG_MINI_RXP_INDEX        0x3010  /* ULong */
#define B57XX_REG_MBUF_CLUSTER_FREE     0x3800  /* ULong */
#define B57XX_REG_COAL_HOST_MODE        0x3C00  /* ULong */
#define B57XX_REG_COAL_HOST_STATUS      0x3C04  /* ULong */
#define B57XX_REG_COAL_RX_TICKS         0x3C08  /* ULong */
#define B57XX_REG_COAL_TX_TICKS         0x3C0C  /* ULong */
#define B57XX_REG_COAL_RX_MAX_BD        0x3C10  /* ULong */
#define B57XX_REG_COAL_TX_MAX_BD        0x3C14  /* ULong */
#define B57XX_REG_COAL_RX_INT_TICKS     0x3C18  /* ULong */
#define B57XX_REG_COAL_TX_INT_TICKS     0x3C1C  /* ULong */
#define B57XX_REG_COAL_RX_MAX_INT_BD    0x3C20  /* ULong */
#define B57XX_REG_COAL_TX_MAX_INT_BD    0x3C24  /* ULong */
#define B57XX_REG_STAT_TICKS            0x3C28  /* ULong */
#define B57XX_REG_STAT_HOST_ADDR        0x3C30  /* ULongLong */
#define B57XX_REG_STATUS_BLCK_HOST_ADDR 0x3C38  /* ULongLong */
#define B57XX_REG_STAT_BASE_ADDR        0x3C40  /* ULong */
#define B57XX_REG_STATUS_BLCK_BASE_ADDR 0x3C44  /* ULong */
#define B57XX_REG_FLOW_ATTENTION        0x3C48  /* ULong */
#define B57XX_REG_RX_LIST_SELECT_MODE   0x3400  /* ULong */
#define B57XX_REG_RX_LIST_SELECT_STATUS 0x3404  /* ULong */
#define B57XX_REG_MEM_ARB_MODE          0x4000  /* ULong */
#define B57XX_REG_MEM_ARB_STATUS        0x4004  /* ULong */
#define B57XX_REG_MEM_ARB_TRAP_LO       0x4008  /* ULong */
#define B57XX_REG_MEM_ARB_TRAP_HI       0x400C  /* ULong */
#define B57XX_REG_BUF_MAN_MODE          0x4400  /* ULong */
#define B57XX_REG_BUF_MAN_STATUS        0x4404  /* ULong */
#define B57XX_REG_MBUF_POOL_BASE        0x4408  /* ULong */
#define B57XX_REG_MBUF_POOL_LENGTH      0x440C  /* ULong */
#define B57XX_REG_MBUF_POOL_DMA_LO_WM   0x4410  /* ULong */
#define B57XX_REG_MBUF_POOL_RX_LO_WM    0x4414  /* ULong */
#define B57XX_REG_MBUF_POOL_HI_WM       0x4418  /* ULong */
#define B57XX_REG_DMA_DESC_POOL_BASE    0x442C  /* ULong */
#define B57XX_REG_DMA_DESC_POOL_LENGTH  0x4430  /* ULong */
#define B57XX_REG_DMA_DESC_POOL_LO_WM   0x4434  /* ULong */
#define B57XX_REG_DMA_DESC_POOL_HI_WM   0x4438  /* ULong */
#define B57XX_REG_DMA_WRITE_MODE        0x4C00  /* ULong */
#define B57XX_REG_DMA_READ_MODE         0x4800  /* ULong */
#define B57XX_REG_DMA_READ_STATUS       0x4804  /* ULong */
#define B57XX_REG_DMA_WRITE_STATUS      0x4C04  /* ULong */
#define B57XX_REG_RX_RISC_MODE          0x5000  /* ULong */
#define B57XX_REG_RX_RISC_STATE         0x5004  /* ULong */
#define B57XX_REG_TX_RISC_MODE          0x5400  /* ULong */
#define B57XX_REG_TX_RISC_STATE         0x5404  /* ULong */
#define B57XX_REG_FTQ_RESET             0x5C00  /* ULong */
#define B57XX_REG_MSI_MODE              0x6000  /* ULong */
#define B57XX_REG_MSI_STATUS            0x6004  /* ULong */
#define B57XX_REG_MSI_FIFO              0x6008  /* ULong */
#define B57XX_REG_DMA_COMPLETION        0x6400  /* ULong */
#define B57XX_REG_MODE_CTRL             0x6800  /* ULong */
#define B57XX_REG_MISC_CONFIG           0x6804  /* ULong */
#define B57XX_REG_MISC_LOCAL_CTRL       0x6808  /* ULong */
#define B57XX_REG_TIMER                 0x680C  /* ULong */
#define B57XX_REG_RX_RISC_EVENT         0x6810  /* ULong */
#define B57XX_REG_TX_RISC_EVENT         0x6820  /* ULong */
#define B57XX_REG_RX_CPU_EVENT          0x684C  /* ULong */
#define B57XX_REG_FASTBOOT_COUNTER      0x6894  /* ULong */
#define B57XX_REG_ASF_CTRL              0x6C00  /* ULong */
#define B57XX_REG_ASF_HEARTBEAT         0x6C10  /* ULong */
#define B57XX_REG_NVM_COMMAND           0x7000  /* ULong */
#define B57XX_REG_NVM_WRITE             0x7008  /* ULong */
#define B57XX_REG_NVM_ADDRESS           0x700C  /* ULong */
#define B57XX_REG_NVM_READ              0x7010  /* ULong */
#define B57XX_REG_NVM_CONFIG1           0x7014  /* ULong */
#define B57XX_REG_NVM_CONFIG2           0x7018  /* ULong */
#define B57XX_REG_NVM_CONFIG3           0x701C  /* ULong */
#define B57XX_REG_SOFT_ARB              0x7020  /* ULong */
#define B57XX_REG_NVM_ACCESS            0x7024  /* ULong */
#define B57XX_REG_NVM_WRITE1            0x7028  /* ULong */
#define B57XX_REG_NVM_ARB_WATCH         0x702C  /* ULong */
#define B57XX_REG_TLP_CTRL              0x7C00  /* ULong */
#define B57XX_REG_DATA_LINK_CTRL        0x7D00  /* ULong */
#define B57XX_REG_DATA_LINK_STATUS      0x7D04  /* ULong */
#define B57XX_REG_DATA_LINK_ATTN        0x7D08  /* ULong */
#define B57XX_REG_DATA_LINK_ATTN_MASK   0x7D0C  /* ULong */
#define B57XX_REG_TLP_ERROR_COUNTER     0x7D40  /* ULong */
#define B57XX_REG_MEM_WINDOW            0x8000  /* To 0xffff */

/* High priority mailbox registers */
#define B57XX_MBOX_INT_MAILBOX0         0x200
#define B57XX_MBOX_STD_RXP_RING_INDEX   0x268
#define B57XX_MBOX_JUMBO_RXP_RING_INDEX 0x270
#define B57XX_MBOX_MINI_RXP_RING_INDEX  0x278
#define B57XX_MBOX_RXC_RET_RING_INDEX1  0x280
#define B57XX_MBOX_TXP_HOST_RING_INDEX1 0x300
#define B57XX_MBOX_TXP_HOST_RING_INDEX2 0x308
#define B57XX_MBOX_TXP_HOST_RING_INDEX3 0x310
#define B57XX_MBOX_TXP_HOST_RING_INDEX4 0x318
#define B57XX_MBOX_TXP_HOST_RING_INDEX4 0x318
#define B57XX_MBOX_TXP_NIC_RING_INDEX1  0x380

/* B57XX_COMMAND */
#define B57XX_COMMAND_MEMSPACE      BIT(1)  /* Memory Space, R/W */
#define B57XX_COMMAND_BUSMASTER     BIT(2)  /* Bus Master, R/W */
#define B57XX_COMMAND_PARITY        BIT(6)  /* Parity Error Enable, R/W */
#define B57XX_COMMAND_SYSERROR      BIT(8)  /* System Error Enable, R/W */
#define B57XX_COMMAND_INTDISABLE    BIT(10) /* Interrupt Disable, R/W for supported devices */

/* B57XX_PCIX_COMMAND */
#define B57XX_PCIX_COMMAND_RELAXEDORDER BIT(1)  /* Enable Relaxed Ordering, R/W */

/* B57XX_POWER_MANAGEMENT */
#define B57XX_POWER_MANAGEMENT_STATE_D0     (0b00 << 0) /* Power State D0, R/W */
#define B57XX_POWER_MANAGEMENT_STATE_D1     (0b01 << 0) /* Power State D1, R/W */
#define B57XX_POWER_MANAGEMENT_STATE_D2     (0b10 << 0) /* Power State D2, R/W */
#define B57XX_POWER_MANAGEMENT_STATE_D3     (0b11 << 0) /* Power State D3, R/W */
#define B57XX_POWER_MANAGEMENT_PMEENABLE    BIT(8)      /* PME Enable, R/O */
#define B57XX_POWER_MANAGEMENT_PMESTATUS    BIT(15)     /* PME Status, R/W2C */

/* B57XX_HW_FIX */
#define B57XX_HW_FIX_HWFIX1     BIT(10) /* HW Fix 1, R/W */
#define B57XX_HW_FIX_HWFIX2     BIT(11) /* HW Fix 2, R/W */
#define B57XX_HW_FIX_HWFIX3     BIT(12) /* HW Fix 3, R/W */
#define B57XX_HW_FIX_HWFIX4     BIT(13) /* HW Fix 4, R/W */

/* B57XX_MISC_HOST_CTRL */
#define B57XX_MISC_HOST_CTRL_CINTA          BIT(0)  /* Clear Interrupt INTA, W/O */
#define B57XX_MISC_HOST_CTRL_MASKINTOUT     BIT(1)  /* Mask PCI Interrupt Output, R/W */
#define B57XX_MISC_HOST_CTRL_ENDIANBYTESWAP BIT(2)  /* Enable Endian Byte Swap, R/W */
#define B57XX_MISC_HOST_CTRL_ENDIANWORDSWAP BIT(3)  /* Enable Endian Word Swap, R/W */
#define B57XX_MISC_HOST_CTRL_PCISTATE       BIT(4)  /* PCI State Register capability, R/W */
#define B57XX_MISC_HOST_CTRL_CLOCKCTRL      BIT(5)  /* Clock Control Register capability, R/W */
#define B57XX_MISC_HOST_CTRL_INDIRECTACCESS BIT(7)  /* Enable Indirect Access, R/W */
#define B57XX_MISC_HOST_CTRL_MASKINTMODE    BIT(8)  /* Mask Interrupt Mode, R/W */
#define B57XX_MISC_HOST_CTRL_TAGGEDSTATUS   BIT(9)  /* Tagged Status, R/W for supported devices */

/* B57XX_REG_DMA_CTRL */
#define B57XX_DMA_CTRL_DPWB_64  (0b00 << 29)    /* Df PCI Wr Cmd bnd: 64 bytes, R/W for s. dev. */
#define B57XX_DMA_CTRL_DPWB_128 (0b01 << 29)    /* Df PCI Wr Cmd bnd: 128 bytes, R/W for s. dev. */
#define B57XX_DMA_CTRL_DPWB_NOC (0b10 << 29)    /* Df PCI Wr Cmd bnd: No bound, R/W for s. dev. */

/* B57XX_PCI_STATE */
#define B57XX_PCI_STATE_FORCERESET  BIT(0)  /* Force PCI Reset, R/O */
#define B57XX_PCI_STATE_INTSTATE    BIT(1)  /* Interrupt State, R/O */
#define B57XX_PCI_STATE_BUSMODE     BIT(2)  /* PCI bus Mode, R/O */
#define B57XX_PCI_STATE_HIGHSPEED   BIT(3)  /* 33/66 MHz PCI bus, 66/133 MHz PCI-X bus, R/O */
#define B57XX_PCI_STATE_32BITBUS    BIT(4)  /* 32-bit PCI bus, R/O */

/* B57XX_PCI_CLOCK */
#define B57XX_PCI_CLOCK_ALTCLOCKFINAL   BIT(20) /* Select Final Alt Clock, R/W */
#define B57XX_PCI_CLOCK_ALTCLOCK        BIT(12) /* Select Alt Clock, R/W */

/* B57XX_ETH_MAC_MODE */
#define B57XX_ETH_MAC_MODE_RESET        BIT(0)      /* Global Reset, R/W */
#define B57XX_ETH_MAC_MODE_HALFDUPLEX   BIT(1)      /* Half-duplex, R/W */
#define B57XX_MASK_ETH_MAC_MODE_PORT    (0b11 << 2) /* Port mode */
#define B57XX_ETH_MAC_MODE_PORT_NONE    (0b00 << 2) /* Port mode: None, R/W */
#define B57XX_ETH_MAC_MODE_PORT_MII     (0b01 << 2) /* Port mode: MII, R/W */
#define B57XX_ETH_MAC_MODE_PORT_GMI     (0b10 << 2) /* Port mode: GMI, R/W */
#define B57XX_ETH_MAC_MODE_PORT_TBI     (0b11 << 2) /* Port mode: Ten bit interface, R/W */
#define B57XX_ETH_MAC_MODE_LOOPBACK     BIT(4)      /* Loopback Mode, R/W */
#define B57XX_ETH_MAC_MODE_TAGGED       BIT(7)      /* Tagged MAC Control, R/W */
#define B57XX_ETH_MAC_MODE_TXBURST      BIT(8)      /* Enable TX Bursting, R/W */
#define B57XX_ETH_MAC_MODE_MAXDEFER     BIT(9)      /* Max Deferral checking statistic, R/W */
#define B57XX_ETH_MAC_MODE_LINKPOLARITY BIT(10)     /* Link polarity, R/W for supported devices */
#define B57XX_ETH_MAC_MODE_RXSTATENABLE BIT(11)     /* Enable RX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_RXSTATCLEAR  BIT(12)     /* Clear RX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_RXSTATFLUSH  BIT(13)     /* Flush RX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_TXSTATENABLE BIT(14)     /* Enable TX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_TXSTATCLEAR  BIT(15)     /* Clear TX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_TXSTATFLUSH  BIT(16)     /* Flush TX Statistics, R/W */
#define B57XX_ETH_MAC_MODE_SENDCONFIGS  BIT(17)     /* Send Configs, R/W */
#define B57XX_ETH_MAC_MODE_MAGICDETECT  BIT(18)     /* Magic Packet Detect Enable, R/W */
#define B57XX_ETH_MAC_MODE_ACPIPOWERON  BIT(19)     /* ACPI Power-on Enable, R/W */
#define B57XX_ETH_MAC_MODE_MIP          BIT(20)     /* Enable MIP, R/W for supported devices */
#define B57XX_ETH_MAC_MODE_TXDMA        BIT(21)     /* Enable Transmit DMA engine, R/W */
#define B57XX_ETH_MAC_MODE_RXDMA        BIT(22)     /* Enable Receive DMA engine, R/W */
#define B57XX_ETH_MAC_MODE_RXFHDDMA     BIT(23)     /* Enable RX Frame Header DMA engine, R/W */

/* B57XX_ETH_MAC_STATUS */
#define B57XX_ETH_MAC_STATUS_CONFIGCHANGED  BIT(3)  /* Config changed, W2C for supported devices */
#define B57XX_ETH_MAC_STATUS_SYNCCHANGED    BIT(4)  /* Sync changed, W2C for supported devices */
#define B57XX_ETH_MAC_STATUS_PORTDECODEERR  BIT(10) /* Port Decode Error, W2C for supported dev. */
#define B57XX_ETH_MAC_STATUS_MICOMPLETE     BIT(22) /* MI Completion, W2C */
#define B57XX_ETH_MAC_STATUS_RXSTATOR       BIT(26) /* RX Statistics Overrun, W2C */
#define B57XX_ETH_MAC_STATUS_TXSTATOR       BIT(27) /* TX Statistics Overrun, W2C */
#define B57XX_ETH_MAC_STATUS_INTEREST       BIT(28) /* Interesting Packet Attn, W2C for s. dev. */

/* B57XX_ETH_MAC_EVENT */
#define B57XX_ETH_MAC_EVENT_PORTERROR   BIT(10) /* Port Decode Error, R/W for supported devices */
#define B57XX_ETH_MAC_EVENT_LINKSTATE   BIT(12) /* Link State Changed, R/W */
#define B57XX_ETH_MAC_EVENT_MICOMPLETE  BIT(22) /* MI Completion, R/W */
#define B57XX_ETH_MAC_EVENT_MIINT       BIT(23) /* MI Interrupt, R/W */
#define B57XX_ETH_MAC_EVENT_APERROR     BIT(24) /* AP Error, R/W */
#define B57XX_ETH_MAC_EVENT_ODIERROR    BIT(25) /* ODI Error, R/W */
#define B57XX_ETH_MAC_EVENT_RXSTATOR    BIT(26) /* RX Statistics Overrun, R/W */
#define B57XX_ETH_MAC_EVENT_TXSTATOR    BIT(27) /* TX Statistics Overrun, R/W */
#define B57XX_ETH_MAC_EVENT_INTEREST    BIT(28) /* Interesting Packet PME Attn, R/W for s. dev. */

/* B57XX_ETH_MI_COMMUNICATION */
#define B57XX_ETH_MI_COMMUNICATION_READFAIL BIT(28) /* Read failed, R/W */
#define B57XX_ETH_MI_COMMUNICATION_START    BIT(29) /* Start/Busy, R/W */

/* B57XX_ETH_MI_STATUS */
#define B57XX_ETH_MI_STATUS_LINK    BIT(0)  /* Link status, R/W */
#define B57XX_ETH_MI_STATUS_10MBPS  BIT(1)  /* Mode10 Mbps, R/W */

/* B57XX_ETH_MI_MODE */
#define B57XX_ETH_MI_MODE_SHORTPREAMBLE BIT(1)  /* Use Short Preamble, R/W */
#define B57XX_ETH_MI_MODE_PORTPOLLING   BIT(4)  /* Port Polling, R/W */

/* B57XX_ETH_TX_MODE */
#define B57XX_ETH_TX_MODE_RESET         BIT(0)  /* Reset, R/W */
#define B57XX_ETH_TX_MODE_ENABLE        BIT(1)  /* Enable, R/W */
#define B57XX_ETH_TX_MODE_FLOWCTRL      BIT(4)  /* Enable Flow Contro, R/W */
#define B57XX_ETH_TX_MODE_BIGBACKOFF    BIT(5)  /* Enable Big Backoff, R/W */
#define B57XX_ETH_TX_MODE_LONGPAUSE     BIT(6)  /* Enable Long Pause, R/W */
#define B57XX_ETH_TX_MODE_LOCKUPFIX     BIT(8)  /* Lockup fix, R/W */

/* B57XX_ETH_RX_MODE */
#define B57XX_ETH_RX_MODE_RESET         BIT(0)  /* Reset, R/W */
#define B57XX_ETH_RX_MODE_ENABLE        BIT(1)  /* Enable, R/W */
#define B57XX_ETH_RX_MODE_FLOWCTRL      BIT(2)  /* Enable Flow Control, R/W */
#define B57XX_ETH_RX_MODE_KEEPPAUSE     BIT(4)  /* Keep Pause, R/W */
#define B57XX_ETH_RX_MODE_OVERSIZED     BIT(5)  /* Accept Oversized, R/W for supported devices */
#define B57XX_ETH_RX_MODE_RUNTS         BIT(6)  /* Accept Runts, R/W */
#define B57XX_ETH_RX_MODE_LENGTHCHECK   BIT(7)  /* Length Check, R/W */
#define B57XX_ETH_RX_MODE_PROMISCUOUS   BIT(8)  /* Promiscuous Mode, R/W */
#define B57XX_ETH_RX_MODE_NOCRCCHECK    BIT(9)  /* No CRC Check, R/W */
#define B57XX_ETH_RX_MODE_KEEPVLAN      BIT(10) /* Keep VLAN Tag Diag Mode, R/W */
#define B57XX_ETH_RX_MODE_FILTBROAD     BIT(11) /* Filter broadcast, R/W for supported devices */
#define B57XX_ETH_RX_MODE_EXTHASH       BIT(12) /* Extended Hash Enable, R/W for supported dev. */

/* B57XX_ETH_RX_RULES_CTRL */
#define B57XX_MASK_ETH_RX_RULES_CTRL_OFFSET     (0xFF << 0)     /* Offset, R/W */
#define B57XX_MASK_ETH_RX_RULES_CTRL_CLASS      (0x1F << 8)     /* Class, R/W */
#define B57XX_ETH_RX_RULES_CTRL_HEADER_FRAME    (0b000 << 13)   /* Start of Frame header, R/W */
#define B57XX_ETH_RX_RULES_CTRL_HEADER_IP       (0b001 << 13)   /* Start of IP Header, R/W */
#define B57XX_ETH_RX_RULES_CTRL_HEADER_TCP      (0b010 << 13)   /* Start of TCP Header, R/W */
#define B57XX_ETH_RX_RULES_CTRL_HEADER_UDP      (0b011 << 13)   /* Start of UDP Header, R/W */
#define B57XX_ETH_RX_RULES_CTRL_HEADER_DATA     (0b100 << 13)   /* Start of Data, R/W */
#define B57XX_ETH_RX_RULES_CTRL_CMP_EQUAL       (0b00 << 16)    /* Equal compare op., R/W */
#define B57XX_ETH_RX_RULES_CTRL_CMP_NOTEQUAL    (0b01 << 16)    /* Not equal compare op., R/W */
#define B57XX_ETH_RX_RULES_CTRL_CMP_GREATER     (0b10 << 16)    /* Greater than compare op, R/W */
#define B57XX_ETH_RX_RULES_CTRL_CMP_LESS        (0b11 << 16)    /* Less than compare op., R/W */
#define B57XX_ETH_RX_RULES_CTRL_MAP             BIT(24)         /* Map, R/W */
#define B57XX_ETH_RX_RULES_CTRL_DISCARD         BIT(25)         /* Discard, R/W */
#define B57XX_ETH_RX_RULES_CTRL_MASK            BIT(26)         /* Mask, R/W */
#define B57XX_ETH_RX_RULES_CTRL_PROCESSOR3      BIT(27)         /* Processor 3, R/W for s. dev. */
#define B57XX_ETH_RX_RULES_CTRL_PROCESSOR2      BIT(28)         /* Processor 2, R/W for s. dev. */
#define B57XX_ETH_RX_RULES_CTRL_PROCESSOR1      BIT(29)         /* Processor 1, R/W */
#define B57XX_ETH_RX_RULES_CTRL_NEXT            BIT(30)         /* And With Next, R/W */
#define B57XX_ETH_RX_RULES_CTRL_ENABLE          BIT(31)         /* Enable R/W */

/* B57XX_ETH_RX_RULES_VALUE */
#define B57XX_MASK_ETH_RX_RULES_VALUE_VALUE (0xffff << 0)   /* Value, R/W */
#define B57XX_MASK_ETH_RX_RULES_VALUE_MASK  (0xffff << 16)  /* Mask, R/W */

/* B57XX_ETH_RX_RULES_CONFIG */
#define B57XX_MASK_ETH_RX_RULES_CONFIG_NORULECLASS  (0xf << 3)  /* No Rules Matches Default Class */

/* B57XX_TX_DATA_MODE */
#define B57XX_TX_DATA_MODE_RESET    BIT(0)  /* Reset, R/W */
#define B57XX_TX_DATA_MODE_ENABLE   BIT(1)  /* Enable, R/W */

/* B57XX_TX_COMPL_MODE */
#define B57XX_TX_COMPL_MODE_RESET       BIT(0)  /* Reset, R/W */
#define B57XX_TX_COMPL_MODE_ENABLE      BIT(1)  /* Enable, R/W */
#define B57XX_TX_COMPL_MODE_LBDBRFIX    BIT(2)  /* Long BD Burst Read Fix, R/W for supported dev. */

/* B57XX_RX_LIST_PLACE_MODE */
#define B57XX_RX_LIST_PLACE_MODE_RESET      BIT(0)  /* Reset, R/W */
#define B57XX_RX_LIST_PLACE_MODE_ENABLE     BIT(1)  /* Enable, R/W */
#define B57XX_RX_LIST_PLACE_MODE_CLASSZERO  BIT(2)  /* Class Zero Attention Enable, R/W */
#define B57XX_RX_LIST_PLACE_MODE_MAPOOR     BIT(3)  /* Mapping Out of Range Attention Enable, R/W */
#define B57XX_RX_LIST_PLACE_MODE_STATSOVER  BIT(4)  /* Stats Overflow Attention Enable, R/W */

/* B57XX_RX_DATA_BD_MODE */
#define B57XX_RX_DATA_BD_MODE_RESET     BIT(0)  /* Reset, R/W */
#define B57XX_RX_DATA_BD_MODE_ENABLE    BIT(1)  /* Enable, R/W */
#define B57XX_RX_DATA_BD_MODE_JUMBORXBD BIT(2)  /* Jumbo RX needed & ring dis., R/W for s. dev. */
#define B57XX_RX_DATA_BD_MODE_OVERSIZED BIT(3)  /* Frame size too large to fit into 1 RX BD, R/W */
#define B57XX_RX_DATA_BD_MODE_ILLEGALSZ BIT(4)  /* Illegal return ring size, R/W */
#define B57XX_RX_DATA_BD_MODE_RDITIMER  BIT(7)  /* RDI Timer Event Enable, R/W for supported dev. */

/* B57XX_BUF_MAN_MODE */
#define B57XX_BUF_MAN_MODE_RESET    BIT(0)  /* Reset, R/W */
#define B57XX_BUF_MAN_MODE_ENABLE   BIT(1)  /* Enable, R/W */
#define B57XX_BUF_MAN_MODE_ATTN     BIT(2)  /* Attention Enable, R/W */
#define B57XX_BUF_MAN_MODE_BMTEST   BIT(3)  /* BM Test Mode, R/W */
#define B57XX_BUF_MAN_MODE_LOATTN   BIT(4)  /* MBUF Low Attn Enable, R/W */

/* B57XX_DMA_MODE */
#define B57XX_DMA_MODE_RESET        BIT(0)  /* Reset, R/W */
#define B57XX_DMA_MODE_ENABLE       BIT(1)  /* Enable, R/W */
#define B57XX_DMA_MODE_TARGETABORT  BIT(2)  /* PCI Target Abort Attention Enable, R/W */
#define B57XX_DMA_MODE_MASTERABORT  BIT(3)  /* PCI Master Abort Attention Enable, R/W */
#define B57XX_DMA_MODE_PARITYERROR  BIT(4)  /* PCI Parity Error Attention Enable, R/W */
#define B57XX_DMA_MODE_HOSTADDROF   BIT(5)  /* PCI Host Address Overflow Error Attention, R/W */
#define B57XX_DMA_MODE_FIFOOR       BIT(6)  /* PCI FIFO Overrun Attention Enable, R/W */
#define B57XX_DMA_MODE_FIFOUR       BIT(7)  /* PCI FIFO Underrun Attention Enable, R/W */
#define B57XX_DMA_MODE_FIFOOVER     BIT(8)  /* PCI FIFO Overread Attention Enable, R/W */
#define B57XX_DMA_MODE_OVERLENGTH   BIT(9)  /* Local Memory Longer Than DMA Length, R/W */
#define B57XX_DMA_MODE_SPLITTIMEOUT BIT(10) /* PCI-X Split Transaction Timeout Expired Attn, R/W */
#define B57XX_DMA_MODE_STATUSTAGFIX BIT(29) /* Status Tag Fix, R/W for supported devices */

/* B57XX_REG_RISC_MODE */
#define B57XX_REG_RISC_MODE_RESET       BIT(0)  /* Reset RISC, R/W */
#define B57XX_REG_RISC_MODE_SINGLESTEP  BIT(1)  /* Single-Step RISC, R/W */
#define B57XX_REG_RISC_MODE_DATAHALT    BIT(2)  /* Enable Page 0 Data Halt, R/W */
#define B57XX_REG_RISC_MODE_INSTRHALT   BIT(3)  /* Enable Page 0 Instruction Halt, R/W */
#define B57XX_REG_RISC_MODE_WRITEPBUF   BIT(4)  /* Enable Write Post Buffers, R/W */
#define B57XX_REG_RISC_MODE_DATACACHE   BIT(5)  /* Enable Data Cache, R/W */
#define B57XX_REG_RISC_MODE_ROMFAIL     BIT(6)  /* ROM Fail, R/W */
#define B57XX_REG_RISC_MODE_WATCHDOG    BIT(7)  /* Enable Watchdog, R/W */
#define B57XX_REG_RISC_MODE_INSTRCPREF  BIT(8)  /* Enable Instruction Cache prefetch, R/W */
#define B57XX_REG_RISC_MODE_INSTRCFLUSH BIT(9)  /* Flush Instruction Cache, R/W */
#define B57XX_REG_RISC_MODE_HALT        BIT(10) /* Halt RISC, R/W */
#define B57XX_REG_RISC_MODE_INVLDDATA   BIT(11) /* Enable Invalid Data access halt, R/W */
#define B57XX_REG_RISC_MODE_INVLDINSTR  BIT(12) /* Enable Invalid Instruction Fetch halt, R/W */
#define B57XX_REG_RISC_MODE_MEMTRAPHALT BIT(13) /* Enable memory address trap halt, R/W */
#define B57XX_REG_RISC_MODE_REGTRAPHALT BIT(14) /* Enable register address trap halt, R/W */

/* B57XX_DMA_COMPLETION */
#define B57XX_DMA_COMPLETION_RESET  BIT(0)  /* Reset, R/W */
#define B57XX_DMA_COMPLETION_ENABLE BIT(1)  /* Enable, R/W */

/* B57XX_MODE_CTRL */
#define B57XX_MODE_CTRL_UPDATEONCOAL  BIT(0)    /* Update on Coalescing Only, for supported dev. */
#define B57XX_MODE_CTRL_BYTESWAPNF    BIT(1)    /* Byte Swap Non-frame Data, R/W */
#define B57XX_MODE_CTRL_WORDSWAPNF    BIT(2)    /* Word Swap Non-frame Data, R/W */
#define B57XX_MODE_CTRL_BYTESWAPDATA  BIT(4)    /* Byte Swap Data, R/W */
#define B57XX_MODE_CTRL_WORDSWAPDATA  BIT(5)    /* Word Swap Data, R/W */
#define B57XX_MODE_CTRL_NOFRAMECRACK  BIT(9)    /* No Frame Cracking, R/W */
#define B57XX_MODE_CTRL_BADFRAMES     BIT(11)   /* Allow Bad Frames, R/W */
#define B57XX_MODE_CTRL_NOINTONTX     BIT(13)   /* Don’t Interrupt on Sends, R/W */
#define B57XX_MODE_CTRL_NOINTONRX     BIT(14)   /* Don’t Interrupt on Receives, R/W */
#define B57XX_MODE_CTRL_FORCE32BITPCI BIT(15)   /* Force 32-bit PCI, R/W */
#define B57XX_MODE_CTRL_HOSTSTACKUP   BIT(16)   /* Host Stack Up, R/W */
#define B57XX_MODE_CTRL_HOSTSENDBDS   BIT(17)   /* Host Send BDs, R/W */
#define B57XX_MODE_CTRL_NOTXPSHCS     BIT(20)   /* Send No Pseudo-header Checksum, R/W */
#define B57XX_MODE_CTRL_NVMWRITE      BIT(21)   /* NVRAM Write Enable, R/W for supported devices */
#define B57XX_MODE_CTRL_NORXPSHCS     BIT(23)   /* Receive No Pseudoheader Checksum, R/W */
#define B57XX_MODE_CTRL_INTTXRISCATTN BIT(24)   /* Int. on TX RISC, R/W for supported devices */
#define B57XX_MODE_CTRL_INTRXRISCATTN BIT(25)   /* Int. on RX RISC, R/W for supported devices */
#define B57XX_MODE_CTRL_INTMACATTN    BIT(26)   /* Interrupt on MAC Attention, R/W */
#define B57XX_MODE_CTRL_INTDMAATTN    BIT(27)   /* Interrupt on DMA Attention, R/W */
#define B57XX_MODE_CTRL_INTFLOWATTN   BIT(28)   /* Interrupt on Flow Attention, R/W */
#define B57XX_MODE_CTRL_4XTXRINGS     BIT(29)   /* 4x NIC TX Rings, R/W for supported devices */
#define B57XX_MODE_CTRL_MULTICASTRISC BIT(30)   /* Route MC Frames to RISC, R/W for s. devices */

/* B57XX_MISC_CONFIG */
#define B57XX_MISC_CONFIG_CLCKRESET BIT(0)  /* CORE Clock Blocks Reset, R/W */
#define B57XX_MISC_CONFIG_POVERRIDE BIT(26) /* Power_Down_Override, R/W for supported devices */
#define B57XX_MISC_CONFIG_DGRPCIEB  BIT(29) /* Disa_GRC_Reset_PCI-E, R/W for supported devices*/

/* B57XX_MISC_LOCAL_CTRL */
#define B57XX_MISC_LOCAL_CTRL_INTSTATE  BIT(0)  /* Interrupt State, R/O */
#define B57XX_MISC_LOCAL_CTRL_CLEARINT  BIT(1)  /* Clear Interrupt, W/O */
#define B57XX_MISC_LOCAL_CTRL_SETINT    BIT(2)  /* Set Interrupt, W/O */
#define B57XX_MISC_LOCAL_CTRL_INTATTN   BIT(3)  /* Interrupt on Attention, R/W */
#define B57XX_MISC_LOCAL_CTRL_GPIOI3    BIT(5)  /* GPIO 3 Input Enable, R/W for s. devices */
#define B57XX_MISC_LOCAL_CTRL_GPIOO3    BIT(6)  /* GPIO 3 Output Enable, R/W for s. devices */
#define B57XX_MISC_LOCAL_CTRL_EXTMEM    BIT(17) /* Enable External Memory, R/W for s. devices */
#define B57XX_MISC_LOCAL_CTRL_SEEPROM   BIT(24) /* Auto SEEPROM Access, R/W */

/* B57XX_RX_LIST_SELECT_MODE */
#define B57XX_RX_LIST_SELECT_MODE_RESET     BIT(0)  /* Reset, R/W */
#define B57XX_RX_LIST_SELECT_MODE_ENABLE    BIT(1)  /* Enable, R/W */
#define B57XX_RX_LIST_SELECT_MODE_ATTN      BIT(2)  /* Attention Enable, R/W */

/* B57XX_SOFT_ARB */
#define B57XX_SOFT_ARB_REQSET0  BIT(0)  /* REQ_SET0, W/O */
#define B57XX_SOFT_ARB_REQSET1  BIT(1)  /* REQ_SET1, W/O */
#define B57XX_SOFT_ARB_REQSET2  BIT(2)  /* REQ_SET2, W/O */
#define B57XX_SOFT_ARB_REQSET3  BIT(3)  /* REQ_SET0, W/O */
#define B57XX_SOFT_ARB_REQCLR0  BIT(4)  /* REQ_CLR0, W/O */
#define B57XX_SOFT_ARB_REQCLR1  BIT(5)  /* REQ_CLR1, W/O */
#define B57XX_SOFT_ARB_REQCLR2  BIT(6)  /* REQ_CLR2, W/O */
#define B57XX_SOFT_ARB_REQCLR3  BIT(7)  /* REQ_CLR3, W/O */
#define B57XX_SOFT_ARB_ARBWON0  BIT(8)  /* ARB_WON0, R/O */
#define B57XX_SOFT_ARB_ARBWON1  BIT(9)  /* ARB_WON1, R/O */
#define B57XX_SOFT_ARB_ARBWON2  BIT(10) /* ARB_WON2, R/O */
#define B57XX_SOFT_ARB_ARBWON3  BIT(11) /* ARB_WON3, R/O */
#define B57XX_SOFT_ARB_ARBWON3  BIT(11) /* ARB_WON3, R/O */
#define B57XX_SOFT_ARB_REQ0     BIT(12) /* REQ0, R/O */
#define B57XX_SOFT_ARB_REQ1     BIT(13) /* REQ1, R/O */
#define B57XX_SOFT_ARB_REQ2     BIT(14) /* REQ2, R/O */
#define B57XX_SOFT_ARB_REQ3     BIT(15) /* REQ3, R/O */

/* B57XX_STAT_CTRL */
#define B57XX_STAT_CTRL_ENABLE      BIT(0)  /* Statistics Enable, R/W */
#define B57XX_STAT_CTRL_FASTSTAT    BIT(1)  /* Faster Statistics Update, R/W for s. devices */
#define B57XX_STAT_CTRL_CLEAR       BIT(2)  /* Statistics Clear, R/W */
#define B57XX_STAT_CTRL_FORCEFLUSH  BIT(3)  /* Force Statistics Flush, R/W for supported devices */
#define B57XX_STAT_CTRL_FORCEZERO   BIT(4)  /* Force Statistics Zero, R/W for supported devices */

/* B57XX_RX_DATA_COMPL_MODE */
#define B57XX_RX_DATA_COMPL_MODE_RESET  BIT(0)  /* Reset, R/W */
#define B57XX_RX_DATA_COMPL_MODE_ENABLE BIT(1)  /* Enable, R/W */
#define B57XX_RX_DATA_COMPL_MODE_ATTN   BIT(2)  /* Attention Enable, R/W */

/* B57XX_BD_MODE */
#define B57XX_BD_MODE_RESET     BIT(0)  /* Reset, R/W */
#define B57XX_BD_MODE_ENABLE    BIT(1)  /* Enable, R/W */
#define B57XX_BD_MODE_ATTN      BIT(2)  /* Attention Enable, R/W */

/* B57XX_MBUF_CLUSTER_FREE */
#define B57XX_MBUF_CLUSTER_FREE_RESET   BIT(0)  /* Reset, R/W */
#define B57XX_MBUF_CLUSTER_FREE_ENABLE  BIT(1)  /* Enable, R/W */
#define B57XX_MBUF_CLUSTER_FREE_ATTN    BIT(2)  /* Attention Enable, R/W */

/* B57XX_COAL_HOST_MODE */
#define B57XX_COAL_HOST_MODE_RESET          BIT(0)      /* Reset, R/W */
#define B57XX_COAL_HOST_MODE_ENABLE         BIT(1)      /* Enable, R/W */
#define B57XX_COAL_HOST_MODE_ATTN           BIT(2)      /* Attention Enable, R/W */
#define B57XX_COAL_HOST_MODE_COALNOW        BIT(3)      /* Coalesce Now, R/W */
#define B57XX_COAL_HOST_MODE_STATUS_SZFULL  (0b00 << 7) /* Full status block, R/W for s. dev. */
#define B57XX_COAL_HOST_MODE_STATUS_SZ64    (0b01 << 7) /* 64 byte status block, R/W for s. dev. */
#define B57XX_COAL_HOST_MODE_STATUS_SZ32    (0b10 << 7) /* 32 byte status block, R/W for s. dev. */
#define B57XX_COAL_HOST_MODE_STATUS_SZUNDEF (0b11 << 7) /* Undefined size, R/W for supported dev. */
#define B57XX_COAL_HOST_MODE_CLEARTICKRX    BIT(9)      /* Clear Ticks Mode on RX, R/W */
#define B57XX_COAL_HOST_MODE_CLEARTICKTX    BIT(10)     /* Clear Ticks Mode on TX, R/W */
#define B57XX_COAL_HOST_MODE_NOINTDMADFORCE BIT(11)     /* No Interrupt on DMAD Force, R/W */
#define B57XX_COAL_HOST_MODE_NOINTFORCEUPD  BIT(12)     /* No Interrupt on Force Update, R/W */

/* B57XX_MEM_ARB_MODE */
#define B57XX_MEM_ARB_MODE_RESET    BIT(0)  /* Reset, R/W */
#define B57XX_MEM_ARB_MODE_ENABLE   BIT(1)  /* Enable, R/W */

/* B57XX_MSI_MODE */
#define B57XX_MSI_MODE_RESET    BIT(0)  /* Reset, R/W */
#define B57XX_MSI_MODE_ENABLE   BIT(1)  /* Enable, R/W */

/* B57XX_TLP_CTRL */
#define B57XX_TLP_CTRL_FIFOPROTECT  BIT(25) /* Data FIFO Protect, R/W */
#define B57XX_TLP_CTRL_INTMODEFIX   BIT(29) /* Interrupt Mode Fix, R/W for supported devices */

/* PHY MII Registers */
#define B57XX_MII_REG_CTRL              0x00
#define B57XX_MII_REG_STATUS            0x01
#define B57XX_MII_REG_PHY_ID1           0x02
#define B57XX_MII_REG_PHY_ID2           0x03
#define B57XX_MII_REG_AUTONEG_ADVERT    0x04
#define B57XX_MII_REG_AUTONEG_PARTNER   0x05
#define B57XX_MII_REG_AUX_CTRL          0x18
#define B57XX_MII_REG_AUX_STATUS        0x19
#define B57XX_MII_REG_INT_STATUS        0x1A
#define B57XX_MII_REG_INT_MASK          0x1B

/* PHY Interrupts */
#define B57XX_PHY_INT_CRC           BIT(0)  /* CRC Error */
#define B57XX_PHY_INT_LINKSTATUS    BIT(1)  /* Link Status Change */
#define B57XX_PHY_INT_LINKSPEED     BIT(2)  /* Link Speed Change */
#define B57XX_PHY_INT_DUPLEXMODE    BIT(3)  /* Duplex Mode Change */
#define B57XX_PHY_INT_LOCALCHANGE   BIT(4)  /* Local Receiver Status Change */
#define B57XX_PHY_INT_REMOTECHANGE  BIT(5)  /* Remote Receiver Status Change */
#define B57XX_PHY_INT_SCRAMBLERSYNC BIT(6)  /* Scrambler Synchronization Error */
#define B57XX_PHY_INT_UNSUPHCD      BIT(7)  /* Negotiated Unsupported HCD */
#define B57XX_PHY_INT_NOHDC         BIT(8)  /* No HCD */
#define B57XX_PHY_INT_HDCNOLINK     BIT(9)  /* HCD No Link */
#define B57XX_PHY_INT_AUTONEGPAGERX BIT(10) /* Auto-negotiation Page Received */
#define B57XX_PHY_INT_EXCEEDEDLOCNT BIT(11) /* Exceeded Low Counter Threshold */
#define B57XX_PHY_INT_EXCEEDEDHICNT BIT(12) /* Exceeded High Counter Threshold */
#define B57XX_PHY_INT_MDIXSTATUS    BIT(13) /* MDIX Status Change */
#define B57XX_PHY_INT_PAIRSWAP      BIT(14) /* Illegal Pair Swap */
#define B57XX_PHY_INT_SIGDETECT     BIT(15) /* Signal Detect/Energy Detect Change */

/* PHY Auto-Negotiation Partner/Advertisement Ability */
#define B57XX_PHY_AUTONEG_10TH          BIT(5)  /* 10BASE-T Half-Duplex Capability */
#define B57XX_PHY_AUTONEG_10TF          BIT(6)  /* 10BASE-T Full-Duplex Capability */
#define B57XX_PHY_AUTONEG_100TH         BIT(7)  /* 100BASE-TX Half-Duplex Capability */
#define B57XX_PHY_AUTONEG_100TF         BIT(8)  /* 100BASE-TX Full-Duplex Capability */
#define B57XX_PHY_AUTONEG_100T4         BIT(9)  /* 100BASE-T4 Capability */
#define B57XX_PHY_AUTONEG_PAUSE         BIT(10) /* Pause Capable */
#define B57XX_PHY_AUTONEG_ASYMPAUSE     BIT(11) /* Asymmetric Pause */
#define B57XX_PHY_AUTONEG_REMOTEFAULT   BIT(13) /* Remote Fault */
#define B57XX_PHY_AUTONEG_ACKNOWLEDGE   BIT(14) /* Acknowledge */
#define B57XX_PHY_AUTONEG_NEXTPAGE      BIT(15) /* Next Page */

/* PHY MII Status */
#define B57XX_PHY_MII_STATUS_EXTCAP             BIT(0)  /* Extended Capability */
#define B57XX_PHY_MII_STATUS_JABBER             BIT(1)  /* Jabber Detect */
#define B57XX_PHY_MII_STATUS_LINKSTATUS         BIT(2)  /* Link Status */
#define B57XX_PHY_MII_STATUS_AUTONEGABILITY     BIT(3)  /* Auto-negotiation Ability */
#define B57XX_PHY_MII_STATUS_REMOTEFAULT        BIT(4)  /* Remote Fault */
#define B57XX_PHY_MII_STATUS_AUTONEGCOMPLETE    BIT(5)  /* Auto-negotiation Complete */
#define B57XX_PHY_MII_STATUS_MGMTFRAMEPRESUP    BIT(6)  /* Management Frames Preamble Suppression */
#define B57XX_PHY_MII_STATUS_EXTSTATUS          BIT(8)  /* Extended Status */
#define B57XX_PHY_MII_STATUS_CAP100TH           BIT(9)  /* 100BASE-T2 Half-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP100TF           BIT(10) /* 100BASE-T2 Full-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP10TH            BIT(11) /* 10BASE-T Half-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP10TF            BIT(12) /* 10BASE-T Full-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP100XH           BIT(13) /* 100BASE-X Half-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP100XF           BIT(14) /* 100BASE-X Full-Duplex Capable */
#define B57XX_PHY_MII_STATUS_CAP100T4           BIT(15) /* 100BASE-T4 Capable */

/* PHY Aux Status */
#define B57XX_PHY_AUX_STATUS_PAUSETXDIR     BIT(0)          /* Pause Resolution–TX Direction */
#define B57XX_PHY_AUX_STATUS_PAUSERXDIR     BIT(1)          /* Pause Resolution–RX Direction */
#define B57XX_PHY_AUX_STATUS_LINKSTATUS     BIT(2)          /* Link Status */
#define B57XX_PHY_AUX_STATUS_PARTNERNP      BIT(3)          /* Link Partner NextPage Ability */
#define B57XX_PHY_AUX_STATUS_PARTERAUTONEG  BIT(4)          /* Link Partner Autoneg Ability */
#define B57XX_PHY_AUX_STATUS_ANPAGERX       BIT(5)          /* Auto-negotiation Page Received */
#define B57XX_PHY_AUX_STATUS_REMOTEFAULT    BIT(6)          /* Remote Fault */
#define B57XX_PHY_AUX_STATUS_PARALLELFAULT  BIT(7)          /* Parallel Detection Fault */
#define B57XX_MASK_PHY_AUX_STATUS_MODE      (0b111 << 8)    /* Status mode */
#define B57XX_PHY_AUX_STATUS_MODE_NONE      (0b000 << 8)    /* No highest common denominator */
#define B57XX_PHY_AUX_STATUS_MODE_10TH      (0b001 << 8)    /* 10BASE-T half-duplex */
#define B57XX_PHY_AUX_STATUS_MODE_10TF      (0b010 << 8)    /* 10BASE-T full-duplex */
#define B57XX_PHY_AUX_STATUS_MODE_100TXH    (0b011 << 8)    /* 100BASE-TX half-duplex */
#define B57XX_PHY_AUX_STATUS_MODE_100T4     (0b100 << 8)    /* 100BASE-T4 */
#define B57XX_PHY_AUX_STATUS_MODE_100TXF    (0b101 << 8)    /* 100BASE-TX full-duplex */
#define B57XX_PHY_AUX_STATUS_MODE_1000TH    (0b110 << 8)    /* 1000BASE-T half-duplex */
#define B57XX_PHY_AUX_STATUS_MODE_1000TF    (0b111 << 8)    /* 1000BASE-T full-duplex */
#define B57XX_PHY_AUX_STATUS_AUTONEGNPW     BIT(11)         /* Auto-negotiation NextPage Wait */
#define B57XX_PHY_AUX_STATUS_AUTONEG        BIT(12)         /* Auto-negotiation Ability Detect */
#define B57XX_PHY_AUX_STATUS_AUTONEGACK     BIT(13)         /* Auto-negotiation Ack Detect */
#define B57XX_PHY_AUX_STATUS_AUTONEGCOMPACK BIT(14)         /* Auto-negotiation Complete Ack */
#define B57XX_PHY_AUX_STATUS_AUTONEGCOMP    BIT(15)         /* Auto-negotiation Complete */

/* EOF */
