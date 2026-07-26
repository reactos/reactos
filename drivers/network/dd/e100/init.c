/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Miniport initialization helper routines
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

#define EE_MEM_BLOCK_SIZE_CONTROL(Adapter) \
    (sizeof(*(Adapter)->ControlBlock) + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

#define EE_MEM_BLOCK_SIZE_TCB(Adapter) \
    ((Adapter)->TcbCount * sizeof(FXP_CB_TRANSMIT) + E100_TCB_ALIGNMENT - 1)

#define EE_MEM_BLOCK_SIZE_RFD(Adapter) \
    ((Adapter)->RfdSize + E100_RECEIVE_BLOCK_SIZE + \
     E100_RECEIVE_BUFFER_SHIFT + \
     SYSTEM_CACHE_ALIGNMENT_SIZE - 1) \

#define EE_MEM_BLOCK_SIZE_TX_BUFFER \
    (E100_TRANSMIT_BLOCK_SIZE + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

E100_PAGED_DATA static const struct
{
    USHORT DeviceID;
    USHORT Flags;
} E100ControllerList[] =
{
    { PCI_DEV_8255x,    0 },
    { PCI_DEV_82559ER,  0 },
    { PCI_DEV_82559_CB, 0 },
    { PCI_DEV_82559,    0 },
    { PCI_DEV_82551QM,  0 },

    { PCI_DEV_ICH2,     E100_FLAG_IS_ICH },

    { PCI_DEV_ICH3_1,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_2,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_3,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_4,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_5,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_6,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_7,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH3_8,   E100_FLAG_IS_ICH },

    { PCI_DEV_ICH4_1,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH4_2,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH4_3,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH4_4,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH4_5,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH4_6,   E100_FLAG_IS_ICH },

    { PCI_DEV_ICH5_1,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_2,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_3,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_4,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_5,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_6,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_7,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH5_8,   E100_FLAG_IS_ICH },

    { PCI_DEV_C_ICH_1,  E100_FLAG_IS_ICH },
    { PCI_DEV_C_ICH_2,  E100_FLAG_IS_ICH },

    { PCI_DEV_ICH6_1,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_2,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_3,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_4,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_5,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_6,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_7,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_8,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH6_9,   E100_FLAG_IS_ICH },

    { PCI_DEV_ICH7_1,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH7_2,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH7_3,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH7_4,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH7_5,   E100_FLAG_IS_ICH },
    { PCI_DEV_ICH7_6,   E100_FLAG_IS_ICH },
    { PCI_DEV_82552,    E100_FLAG_IS_ICH },
};

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
EeConfigQueryInteger(
    _In_ NDIS_HANDLE ConfigurationHandle,
    _In_ PCWSTR EntryName,
    _Out_ PULONG EntryContext,
    _In_ ULONG DefaultValue,
    _In_ ULONG Minimum,
    _In_ ULONG Maximum)
{
    NDIS_STATUS Status;
    UNICODE_STRING Keyword;
    PNDIS_CONFIGURATION_PARAMETER ConfigurationParameter;

    PAGED_CODE();

    NdisInitUnicodeString(&Keyword, EntryName);
    NdisReadConfiguration(&Status,
                          &ConfigurationParameter,
                          ConfigurationHandle,
                          &Keyword,
                          NdisParameterInteger);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE("'%S' request failed, default to %u\n", EntryName, DefaultValue);

        *EntryContext = DefaultValue;
        return;
    }

    if (ConfigurationParameter->ParameterData.IntegerData >= Minimum &&
        ConfigurationParameter->ParameterData.IntegerData <= Maximum)
    {
        *EntryContext = ConfigurationParameter->ParameterData.IntegerData;
    }
    else
    {
        WARN("'%S' value out of range\n", EntryName);

        *EntryContext = DefaultValue;
    }

    INFO("Set '%S' to %lu\n", EntryName, *EntryContext);
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeReadConfiguration(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    NDIS_HANDLE ConfigurationHandle;
    PUCHAR NetworkAddress;
    UINT Length;
    ULONG GenericUlong;

    PAGED_CODE();

    NdisOpenConfiguration(&Status,
                          &ConfigurationHandle,
                          Adapter->WrapperConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    EeConfigQueryInteger(ConfigurationHandle,
                         L"SpeedDuplex",
                         &GenericUlong,
                         0,
                         0,
                         4);
    switch (GenericUlong)
    {
        case 0:
            Adapter->DefaultMedia = E100_MEDIA_AUTO;
            break;
        case 1:
            Adapter->DefaultMedia = 0;
            break;
        case 2:
            Adapter->DefaultMedia = E100_MEDIA_FD;
            break;
        case 3:
            Adapter->DefaultMedia = E100_MEDIA_100T;
            break;
        case 4:
            Adapter->DefaultMedia = E100_MEDIA_100T | E100_MEDIA_FD;
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }

    /* Flow control */
    if (Adapter->Flags & E100_FLAG_HAS_FLOW_CONTROL)
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"FlowControl",
                             &GenericUlong,
                             1,
                             0,
                             4);
        if (!(Adapter->DefaultMedia & E100_MEDIA_AUTO))
        {
            if (GenericUlong == 1)
            {
                WARN("Cannot enable auto-negotiation of pause frames for forced link\n");
                GenericUlong = 0;
            }
            else if (!(Adapter->DefaultMedia & E100_MEDIA_FD) && (GenericUlong != 0))
            {
                WARN("Cannot enable pause frames for half-duplex forced link\n");
                GenericUlong = 0;
            }
        }
        switch (GenericUlong)
        {
            case 0:
                break;
            case 1:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_AUTO |
                                         E100_MEDIA_PAUSE_TX |
                                         E100_MEDIA_PAUSE_RX;
                break;
            case 2:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_RX;
                break;
            case 3:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_TX;
                break;
            case 4:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX;
                break;

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }
    }

    EeConfigQueryInteger(ConfigurationHandle,
                         L"TcbNum",
                         &GenericUlong,
                         E100_TRANSMIT_BLOCKS_DEFAULT,
                         E100_TRANSMIT_BLOCKS_MIN,
                         E100_TRANSMIT_BLOCKS_MAX);
    Adapter->TcbCount = GenericUlong;

    EeConfigQueryInteger(ConfigurationHandle,
                         L"RfdNum",
                         &GenericUlong,
                         E100_RECEIVE_BUFFERS_DEFAULT,
                         E100_RECEIVE_BUFFERS_MIN,
                         E100_RECEIVE_BUFFERS_MAX);
    Adapter->RfdToAllocate = GenericUlong;

    /* Intel microcode parameters */
    if (!(Adapter->Flags & E100_FLAG_NO_UCODE))
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"IntDelay",
                             &GenericUlong,
                             1536,
                             0,
                             65535);
        Adapter->MicrocodeInterruptDelay = GenericUlong;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"BundleMax",
                             &GenericUlong,
                             6,
                             1,
                             65535);
        Adapter->MicrocodeMaxFramesPerIntr = GenericUlong;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"BundleSmall",
                             &GenericUlong,
                             1,
                             0,
                             1);
        Adapter->MicrocodeMinSizeMask = GenericUlong ? 0xFFFF : 0xFF80;
    }

    /* VLAN tagging */
    if (Adapter->Flags & E100_FLAG_EXT_RFA)
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"Priority",
                             &GenericUlong,
                             1,
                             0,
                             1);
        if (GenericUlong)
            Adapter->Flags |= E100_FLAG_PACKET_PRIORITY;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"VlanTag",
                             &GenericUlong,
                             1,
                             0,
                             1);
        if (GenericUlong)
            Adapter->Flags |= E100_FLAG_VLAN_TAGGING;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"VlanID",
                             &GenericUlong,
                             0,
                             0,
                             E100_MAXIMUM_VLAN_ID);
        Adapter->VlanId = GenericUlong;
    }

    NdisReadNetworkAddress(&Status,
                           (PVOID*)&NetworkAddress,
                           &Length,
                           ConfigurationHandle);
    if ((Status == NDIS_STATUS_SUCCESS) && (Length == ETH_LENGTH_OF_ADDRESS))
    {
        if (ETH_IS_MULTICAST(NetworkAddress) ||
            ETH_IS_EMPTY(NetworkAddress) ||
            ETH_IS_BROADCAST(NetworkAddress) ||
            !ETH_IS_LOCALLY_ADMINISTERED(NetworkAddress))
        {
            ERR("Invalid software MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                NetworkAddress[0],
                NetworkAddress[1],
                NetworkAddress[2],
                NetworkAddress[3],
                NetworkAddress[4],
                NetworkAddress[5]);
            Status = NDIS_STATUS_INVALID_ADDRESS;
        }
    }
    else
    {
        Status = NDIS_STATUS_INVALID_ADDRESS;
    }
    if (Status == NDIS_STATUS_SUCCESS)
    {
        INFO("Using software MAC address\n");
        NdisMoveMemory(Adapter->CurrentMacAddress,
                       NetworkAddress,
                       ETH_LENGTH_OF_ADDRESS);
    }
    else
    {
        NdisMoveMemory(Adapter->CurrentMacAddress,
                       Adapter->PermanentMacAddress,
                       ETH_LENGTH_OF_ADDRESS);
    }

    NdisCloseConfiguration(ConfigurationHandle);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
EeGetTransmitBufferAllocSize(
    _Out_ PULONG BuffersPerPage,
    _Out_ PULONG BlockSize)
{
    ULONG NumPerPage;

    PAGED_CODE();

    NumPerPage = PAGE_SIZE / EE_MEM_BLOCK_SIZE_TX_BUFFER;
    NumPerPage = max(NumPerPage, 1);

    *BlockSize = NumPerPage * EE_MEM_BLOCK_SIZE_TX_BUFFER;
    *BuffersPerPage = NumPerPage;
}

static
CODE_SEG("PAGE")
VOID
EeGetRfdAllocSize(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PULONG BuffersPerPage,
    _Out_ PULONG BlockSize)
{
    ULONG NumPerPage;

    PAGED_CODE();

    NumPerPage = PAGE_SIZE / EE_MEM_BLOCK_SIZE_RFD(Adapter);
    NumPerPage = max(NumPerPage, 1);

    *BlockSize = NumPerPage * EE_MEM_BLOCK_SIZE_RFD(Adapter);
    *BuffersPerPage = NumPerPage;
}

static
CODE_SEG("PAGE")
BOOLEAN
EeAllocateRfd(
    _In_ PE100_ADAPTER Adapter,
    _In_ PE100_RX_CONTEXT RxContext,
    _In_ PVOID VirtualAddress,
    _In_ NDIS_PHYSICAL_ADDRESS PhysicalAddress)
{
    NDIS_STATUS Status;

    PAGED_CODE();

    VirtualAddress = ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
    PhysicalAddress.QuadPart = ALIGN_UP_BY(PhysicalAddress.QuadPart, SYSTEM_CACHE_ALIGNMENT_SIZE);

    /* NOTE: This makes the LinkAddress and RbdAddress fields to be not aligned to 4 bytes */
    ASSERT(((ULONG_PTR)VirtualAddress % E100_RECEIVE_BUFFER_DATA_ALIGN) == 0);
    ASSERT((PhysicalAddress.QuadPart % E100_RECEIVE_BUFFER_DATA_ALIGN) == 0);
    VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + E100_RECEIVE_BUFFER_SHIFT);
    PhysicalAddress.QuadPart += E100_RECEIVE_BUFFER_SHIFT;

    RxContext->Rfd = VirtualAddress;
    RxContext->RfdPhys = PhysicalAddress.LowPart;

    NdisAllocatePacket(&Status, &RxContext->Packet, Adapter->PacketPool);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    *E100_RX_CONTEXT_FROM_PACKET(RxContext->Packet) = RxContext;

    /* Allocate MDL for RFD */
    NdisAllocateBuffer(&Status,
                       &RxContext->RfdMdl,
                       Adapter->BufferPool,
                       RxContext->Rfd,
                       Adapter->RfdSize);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    /* Allocate MDL for receive buffer */
    NdisAllocateBuffer(&Status,
                       &RxContext->ReceiveBufferMdl,
                       Adapter->BufferPool,
                       (PVOID)((ULONG_PTR)RxContext->Rfd + Adapter->RfdSize),
                       E100_RECEIVE_BLOCK_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    NDIS_SET_PACKET_HEADER_SIZE(RxContext->Packet, E100_ETHERNET_HEADER_SIZE);
    NdisChainBufferAtFront(RxContext->Packet, RxContext->ReceiveBufferMdl);

    return TRUE;

Failure:
    if (RxContext->RfdMdl)
        NdisFreeBuffer(RxContext->RfdMdl);
    if (RxContext->ReceiveBufferMdl)
        NdisFreeBuffer(RxContext->ReceiveBufferMdl);
    if (RxContext->Packet)
        NdisFreePacket(RxContext->Packet);

    return FALSE;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateReceiveBuffers(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    ULONG i;
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG RfdPerPage, BlockSize;

    PAGED_CODE();

    InitializeListHead(&Adapter->RxContextList);

    NdisAllocatePacketPool(&Status,
                           &Adapter->PacketPool,
                           Adapter->RfdToAllocate,
                           PROTOCOL_RESERVED_SIZE_IN_PACKET);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisAllocateBufferPool(&Status,
                           &Adapter->BufferPool,
                           Adapter->RfdToAllocate);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->RxContext,
                                       sizeof(*Adapter->RxContext) * Adapter->RfdToAllocate,
                                       E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;
    NdisZeroMemory(Adapter->RxContext, sizeof(*Adapter->RxContext) * Adapter->RfdToAllocate);

    /* Allocate one-page chunks to reduce memory usage */
    EeGetRfdAllocSize(Adapter, &RfdPerPage, &BlockSize);

    /* Allocate RFDs */
    for (i = 0; i < Adapter->RfdToAllocate; ++i)
    {
        PE100_RX_CONTEXT RxContext = &Adapter->RxContext[i];

        if ((i % RfdPerPage) == 0)
        {
            /* Allocate a chunk of memory */
            NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                                      BlockSize,
                                      TRUE, /* Cached */
                                      &VirtualAddress,
                                      &PhysicalAddress);
            if (!VirtualAddress)
            {
                WARN("RFD allocation failed, total buffers %lu\n", Adapter->RfdCount);
                break;
            }
            NdisZeroMemory(VirtualAddress, BlockSize);

            /* 32-bit DMA */
            ASSERT(PhysicalAddress.HighPart == 0);

            RxContext->VirtualAddressOriginal = VirtualAddress;
            RxContext->PhysicalAddressOriginal = PhysicalAddress.LowPart;
        }

        if (!EeAllocateRfd(Adapter, RxContext, VirtualAddress, PhysicalAddress))
        {
            WARN("RFD allocation failed, total buffers %lu\n", Adapter->RfdCount);
            break;
        }

        /* Split the allocation */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + EE_MEM_BLOCK_SIZE_RFD(Adapter));
        PhysicalAddress.QuadPart += EE_MEM_BLOCK_SIZE_RFD(Adapter);

        ++Adapter->RfdCount;
    }

    if (Adapter->RfdCount < E100_RECEIVE_BUFFERS_MIN)
        return NDIS_STATUS_RESOURCES;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateTransmitBlocks(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              EE_MEM_BLOCK_SIZE_TCB(Adapter),
                              FALSE, /* Non-cached */
                              &Adapter->TcbOriginalVa,
                              &PhysicalAddress);
    if (!Adapter->TcbOriginalVa)
        return NDIS_STATUS_RESOURCES;
    NdisZeroMemory(Adapter->TcbOriginalVa, EE_MEM_BLOCK_SIZE_TCB(Adapter));

    /* 32-bit DMA */
    ASSERT(PhysicalAddress.HighPart == 0);

    Adapter->TcbOriginalPhys = PhysicalAddress.LowPart;

    Adapter->HeadTcbPa = ALIGN_UP_BY(Adapter->TcbOriginalPhys, E100_TCB_ALIGNMENT);
    Adapter->HeadTcb = ALIGN_UP_POINTER_BY(Adapter->TcbOriginalVa, E100_TCB_ALIGNMENT);

    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->TxContext,
                                       sizeof(*Adapter->TxContext) * Adapter->TcbCount,
                                       E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;
    NdisZeroMemory(Adapter->TxContext, sizeof(*Adapter->TxContext) * Adapter->TcbCount);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateTransmitBuffers(
    _In_ PE100_ADAPTER Adapter)
{
    ULONG i;
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG BuffersPerPage, BlockSize;

    PAGED_CODE();

    /* Allocate one-page chunks to reduce memory usage */
    EeGetTransmitBufferAllocSize(&BuffersPerPage, &BlockSize);

    for (i = 0; i < E100_TRANSMIT_BUFFERS; ++i)
    {
        PE100_COALESCE_BUFFER CoalesceBuffer = &Adapter->CoalesceBuffer[i];

        if ((i % BuffersPerPage) == 0)
        {
            /* Allocate a chunk of memory */
            NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                                      BlockSize,
                                      TRUE, /* Cached */
                                      &VirtualAddress,
                                      &PhysicalAddress);
            if (!VirtualAddress)
                return NDIS_STATUS_RESOURCES;
            NdisZeroMemory(VirtualAddress, BlockSize);

            /* 32-bit DMA */
            ASSERT(PhysicalAddress.HighPart == 0);

            CoalesceBuffer->VirtualAddressOriginal = VirtualAddress;
            CoalesceBuffer->PhysicalAddressOriginal = PhysicalAddress.LowPart;
        }

        CoalesceBuffer->VirtualAddress =
            ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
        CoalesceBuffer->PhysicalAddress =
            ALIGN_UP_BY(PhysicalAddress.LowPart, SYSTEM_CACHE_ALIGNMENT_SIZE);

        PushEntryList(&Adapter->SendBufferList, &CoalesceBuffer->ListEntry);

        /* Split the allocation */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + EE_MEM_BLOCK_SIZE_TX_BUFFER);
        PhysicalAddress.QuadPart += EE_MEM_BLOCK_SIZE_TX_BUFFER;
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateControlBlock(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              EE_MEM_BLOCK_SIZE_CONTROL(Adapter),
                              FALSE, /* Non-cached */
                              &Adapter->ControlBlockOriginal,
                              &PhysicalAddress);
    if (!Adapter->ControlBlockOriginal)
        return NDIS_STATUS_RESOURCES;
    NdisZeroMemory(Adapter->ControlBlockOriginal, EE_MEM_BLOCK_SIZE_CONTROL(Adapter));

    /* 32-bit DMA */
    ASSERT(PhysicalAddress.HighPart == 0);

    Adapter->ControlBlockOriginalPhys = PhysicalAddress.LowPart;

    Adapter->ControlBlockPa = ALIGN_UP_BY(PhysicalAddress.LowPart, SYSTEM_CACHE_ALIGNMENT_SIZE);
    Adapter->ControlBlock = ALIGN_UP_POINTER_BY(Adapter->ControlBlockOriginal,
                                                SYSTEM_CACHE_ALIGNMENT_SIZE);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateMemory(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    PAGED_CODE();

    Status = NdisMInitializeScatterGatherDma(Adapter->AdapterHandle,
                                             FALSE, /* 32-bit DMA */
                                             E100_MAXIMUM_FRAME_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = EeAllocateControlBlock(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = EeAllocateTransmitBlocks(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = EeAllocateTransmitBuffers(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = EeAllocateReceiveBuffers(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisAllocateSpinLock(&Adapter->SendLock);
    NdisAllocateSpinLock(&Adapter->ReceiveLock);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
EeCreateTxRing(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_TX_CONTEXT PrevTxContext, TxContext;
    PFXP_CB_TRANSMIT PrevTcb, Tcb;
    ULONG i, TcbPa;

    PAGED_CODE();

    PrevTxContext = &Adapter->TxContext[Adapter->TcbCount - 1];
    TxContext = Adapter->TxContext;
    PrevTcb = &Adapter->HeadTcb[Adapter->TcbCount - 1];
    Tcb = Adapter->HeadTcb;
    TcbPa = Adapter->HeadTcbPa;

    for (i = 0; i < Adapter->TcbCount; ++i)
    {
        Tcb->Header.Status = 0;
        Tcb->Header.Command = htole16(FXP_CB_COMMAND_NOP | FXP_CB_COMMAND_S);
        PrevTcb->Header.LinkAddress = htole32(TcbPa);
        if (Adapter->Flags & E100_FLAG_EXT_TXCB)
        {
            /* The TBD address points to the third TBD in this mode */
            Tcb->TbdArrayAddress = htole32(TcbPa + FIELD_OFFSET(FXP_CB_TRANSMIT, Tbd[2]));
        }
        else
        {
            Tcb->TbdArrayAddress = htole32(TcbPa + FIELD_OFFSET(FXP_CB_TRANSMIT, Tbd[0]));
        }

        TxContext->Tcb = Tcb;
        TxContext->TcbPhys = TcbPa;
        PrevTxContext->Next = TxContext;

        PrevTxContext = TxContext;
        PrevTcb = Tcb;

        TxContext++;
        Tcb++;
        TcbPa += sizeof(*Tcb);
    }
}

static
CODE_SEG("PAGE")
VOID
EeCreateRxRing(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_RX_CONTEXT RxContext = Adapter->RxContext;
    PFXP_RFD Rfd = NULL;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < Adapter->RfdCount; ++i)
    {
        /* Attach a new buffer to the receive chain */
        if (Rfd)
        {
            le32enc(&Rfd->Header.LinkAddress, RxContext->RfdPhys);
        }

        Rfd = RxContext->Rfd;
        Rfd->Header.Status = 0;
        Rfd->Header.Command = 0;
        le32enc(&Rfd->Header.LinkAddress, 0xFFFFFFFF);
        le32enc(&Rfd->RbdAddress, 0xFFFFFFFF);
        Rfd->Size = htole16(E100_RECEIVE_BLOCK_SIZE);
        Rfd->ActualSize = 0;

        InsertTailList(&Adapter->RxContextList, &RxContext->ListEntry);

        RxContext++;
    }
    Rfd->Header.Command = htole16(FXP_RFD_CONTROL_EL);
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeInitializeAdapterResources(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    PNDIS_RESOURCE_LIST AssignedResources = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR IoDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    UINT i, ResourceListSize = 0;

    PAGED_CODE();

    NdisMQueryAdapterResources(&Status,
                               Adapter->WrapperConfigurationHandle,
                               AssignedResources,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_RESOURCES)
        return NDIS_STATUS_FAILURE;

    Status = NdisAllocateMemoryWithTag((PVOID*)&AssignedResources,
                                       ResourceListSize,
                                       E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisMQueryAdapterResources(&Status,
                               Adapter->WrapperConfigurationHandle,
                               AssignedResources,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Cleanup;

    for (i = 0; i < AssignedResources->Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &AssignedResources->PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypeMemory:
            {
                if (!IoDescriptor && (Descriptor->u.Memory.Length == FXP_PCI_MMIO_BAR_LENGTH))
                    IoDescriptor = Descriptor;
                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

    if (!IoDescriptor || !InterruptDescriptor)
    {
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    Adapter->InterruptVector = InterruptDescriptor->u.Interrupt.Vector;
    Adapter->InterruptLevel = InterruptDescriptor->u.Interrupt.Level;
    Adapter->InterruptFlags = InterruptDescriptor->Flags;
    if (InterruptDescriptor->ShareDisposition == CmResourceShareShared)
        Adapter->Flags |= E100_FLAG_IRQ_SHARED;

    Status = NdisMMapIoSpace((PVOID*)&Adapter->IoBase,
                             Adapter->AdapterHandle,
                             IoDescriptor->u.Memory.Start,
                             FXP_PCI_MMIO_BAR_LENGTH);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Cleanup;

    INFO("IO Base %p\n", Adapter->IoBase);
    INFO("IRQ Level %u, Vector %u\n",
         Adapter->InterruptLevel,
         Adapter->InterruptVector);
    INFO("IRQ ShareDisposition %u, InterruptFlags %lx\n",
         InterruptDescriptor->ShareDisposition,
         InterruptDescriptor->Flags);

Cleanup:
    NdisFreeMemory(AssignedResources, ResourceListSize, 0);

    return Status;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeRecognizeHardware(
    _In_ PE100_ADAPTER Adapter)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_CONFIG, CacheLineSize)];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)Buffer; // Partial PCI header
    ULONG i, Bytes, Flags;

    PAGED_CODE();

    Bytes = NdisReadPciSlotInformation(Adapter->AdapterHandle,
                                       0,
                                       FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                                       Buffer,
                                       sizeof(Buffer));
    if (Bytes != sizeof(Buffer))
        return NDIS_STATUS_FAILURE;

    if (PciConfig->VendorID != PCI_VEN_INTEL)
        return NDIS_STATUS_FAILURE;

    for (i = 0; i < RTL_NUMBER_OF(E100ControllerList); ++i)
    {
        Flags = E100ControllerList[i].Flags;

        if (PciConfig->DeviceID == E100ControllerList[i].DeviceID)
            break;
    }
    if (i == RTL_NUMBER_OF(E100ControllerList))
        return NDIS_STATUS_NOT_RECOGNIZED;

    INFO("Starting controller %04X:%04X.%02X\n",
         PciConfig->VendorID,
         PciConfig->DeviceID,
         PciConfig->RevisionID);

    Adapter->DeviceID = PciConfig->DeviceID;
    Adapter->RevisionID = PciConfig->RevisionID;
    Adapter->Flags = Flags;

    if (PciConfig->Command & PCI_ENABLE_WRITE_AND_INVALIDATE)
    {
        INFO("WMI enabled\n");
        Adapter->Flags |= E100_FLAG_WMI_ENANLE;
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
PE100_ADAPTER
EeAllocateAdapter(
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE WrapperConfigurationContext)
{
    PE100_ADAPTER Adapter;
    PVOID UnalignedAdapter;
    ULONG Alignment, AdapterSize;
    NDIS_STATUS Status;

    PAGED_CODE();

    Alignment = NdisGetSharedDataAlignment();
    AdapterSize = sizeof(*Adapter) + Alignment - 1;

    Status = NdisAllocateMemoryWithTag((PVOID*)&UnalignedAdapter, AdapterSize, E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return NULL;
    NdisZeroMemory(UnalignedAdapter, AdapterSize);

    Adapter = ALIGN_UP_POINTER_BY(UnalignedAdapter, Alignment);
    Adapter->AdapterOriginal = UnalignedAdapter;
    Adapter->AdapterSize = AdapterSize;
    Adapter->AdapterHandle = MiniportAdapterHandle;
    Adapter->WrapperConfigurationHandle = WrapperConfigurationContext;
    return Adapter;
}

CODE_SEG("PAGE")
VOID
EeFreeAdapter(
    _In_ __drv_freesMem(Mem) PE100_ADAPTER Adapter)
{
    ULONG i;

    PAGED_CODE();

    if (Adapter->Interrupt.InterruptObject)
    {
        NdisMDeregisterInterrupt(&Adapter->Interrupt);
    }

    if (Adapter->IoBase)
    {
        NdisMUnmapIoSpace(Adapter->AdapterHandle,
                          Adapter->IoBase,
                          FXP_PCI_MMIO_BAR_LENGTH);
    }

    if (Adapter->ControlBlockOriginal)
    {
        NDIS_PHYSICAL_ADDRESS PhysicalAddress;

        PhysicalAddress.QuadPart = Adapter->ControlBlockOriginalPhys;
        NdisMFreeSharedMemory(Adapter->AdapterHandle,
                              EE_MEM_BLOCK_SIZE_CONTROL(Adapter),
                              FALSE, /* Non-cached */
                              Adapter->ControlBlockOriginal,
                              PhysicalAddress);
    }

    if (Adapter->TxContext)
    {
        NdisFreeMemory(Adapter->TxContext, sizeof(*Adapter->TxContext) * Adapter->TcbCount, 0);
    }

    if (Adapter->TcbOriginalVa)
    {
        NDIS_PHYSICAL_ADDRESS PhysicalAddress;

        PhysicalAddress.QuadPart = Adapter->TcbOriginalPhys;
        NdisMFreeSharedMemory(Adapter->AdapterHandle,
                              EE_MEM_BLOCK_SIZE_TCB(Adapter),
                              FALSE, /* Non-cached */
                              Adapter->TcbOriginalVa,
                              PhysicalAddress);
    }

    if (Adapter->RxContext)
    {
        ULONG RfdPerPage, BlockSize;

        EeGetRfdAllocSize(Adapter, &RfdPerPage, &BlockSize);

        for (i = 0; i < Adapter->RfdToAllocate; ++i)
        {
            PE100_RX_CONTEXT RxContext = &Adapter->RxContext[i];

            if (RxContext->VirtualAddressOriginal)
            {
                NDIS_PHYSICAL_ADDRESS PhysicalAddress;

                PhysicalAddress.QuadPart = RxContext->PhysicalAddressOriginal;
                NdisMFreeSharedMemory(Adapter->AdapterHandle,
                                      BlockSize,
                                      TRUE, /* Cached */
                                      RxContext->VirtualAddressOriginal,
                                      PhysicalAddress);

                if (RxContext->RfdMdl)
                    NdisFreeBuffer(RxContext->RfdMdl);
                if (RxContext->ReceiveBufferMdl)
                    NdisFreeBuffer(RxContext->ReceiveBufferMdl);
                if (RxContext->Packet)
                    NdisFreePacket(RxContext->Packet);
            }
        }

        NdisFreeMemory(Adapter->RxContext, sizeof(*Adapter->RxContext) * Adapter->RfdToAllocate, 0);
    }

    if (Adapter->CoalesceBuffer[0].VirtualAddress)
    {
        ULONG BuffersPerPage, BlockSize;

        EeGetTransmitBufferAllocSize(&BuffersPerPage, &BlockSize);

        for (i = 0; i < E100_TRANSMIT_BUFFERS; ++i)
        {
            PE100_COALESCE_BUFFER CoalesceBuffer = &Adapter->CoalesceBuffer[i];

            if (CoalesceBuffer->VirtualAddressOriginal)
            {
                NDIS_PHYSICAL_ADDRESS PhysicalAddress;

                PhysicalAddress.QuadPart = CoalesceBuffer->PhysicalAddressOriginal;
                NdisMFreeSharedMemory(Adapter->AdapterHandle,
                                      BlockSize,
                                      FALSE, /* Non-cached */
                                      CoalesceBuffer->VirtualAddressOriginal,
                                      PhysicalAddress);
            }
        }
    }

    if (Adapter->PacketPool)
        NdisFreePacketPool(Adapter->PacketPool);
    if (Adapter->BufferPool)
        NdisFreeBufferPool(Adapter->BufferPool);

    if (Adapter->SendLock.SpinLock)
        NdisFreeSpinLock(&Adapter->SendLock);
    if (Adapter->ReceiveLock.SpinLock)
        NdisFreeSpinLock(&Adapter->ReceiveLock);

    NdisFreeMemory(Adapter->AdapterOriginal, sizeof(*Adapter), 0);
}

CODE_SEG("PAGE")
NDIS_STATUS
NTAPI
MiniportInitialize(
    _Out_ PNDIS_STATUS OpenErrorStatus,
    _Out_ PUINT SelectedMediumIndex,
    _In_ PNDIS_MEDIUM MediumArray,
    _In_ UINT MediumArraySize,
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE WrapperConfigurationContext)
{
    PE100_ADAPTER Adapter;
    NDIS_STATUS Status;
    UINT i;

    INFO("Called\n");

    PAGED_CODE();

    for (i = 0; i < MediumArraySize; ++i)
    {
        if (MediumArray[i] == NdisMedium802_3)
        {
            *SelectedMediumIndex = i;
            break;
        }
    }
    if (i == MediumArraySize)
    {
        ERR("No supported media\n");
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    Adapter = EeAllocateAdapter(MiniportAdapterHandle, WrapperConfigurationContext);
    if (!Adapter)
    {
        ERR("Failed to allocate adapter context\n");
        return NDIS_STATUS_RESOURCES;
    }

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         Adapter,
                         2, /* CheckForHangTimeInSeconds */
                         NDIS_ATTRIBUTE_BUS_MASTER |
                         NDIS_ATTRIBUTE_DESERIALIZE |
                         NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS,
                         NdisInterfacePci);

    Status = EeRecognizeHardware(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    Status = EeInitializeAdapterResources(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    EeSoftReset(Adapter);

    if (!EeReadEeprom(Adapter))
        goto Failure;

    Adapter->TbdCount = E100_TBD_PER_TCB;
    Adapter->TransmitThreshold = 64;

    if (Adapter->RevisionID == FXP_REV_82557)
    {
        /* Older chips do not support MWI */
        Adapter->Flags &= ~E100_FLAG_WMI_ENANLE;

        /* A hack to get long VLAN frames on a 82557 */
        Adapter->Flags |= E100_FLAG_SAVE_BAD_PACKETS;
    }
    else
    {
        /* The 82558 and later chips have flow control */
        Adapter->Flags |= E100_FLAG_HAS_FLOW_CONTROL;

        /* Turn on the extended TxCB feature */
        Adapter->Flags |= E100_FLAG_EXT_TXCB;

        /* Enable reception of long frames for VLAN */
        Adapter->Flags |= E100_FLAG_LONG_PKT;

        /* For 82559 or later chips, RX checksum offload is supported */
        if (Adapter->RevisionID >= FXP_REV_82559_A0)
        {
            /* 82559ER does not support RX checksum offloading */
            if (Adapter->DeviceID != PCI_DEV_82559ER)
                Adapter->Flags |= E100_FLAG_82559_RXCSUM;
        }
    }

    /* Enable use of extended RFDs and TCBs for 82550 and later chips */
    if ((Adapter->RevisionID == FXP_REV_82550) ||
        (Adapter->RevisionID == FXP_REV_82550_C) ||
        (Adapter->RevisionID == FXP_REV_82551_E) ||
        (Adapter->RevisionID == FXP_REV_82551_F) ||
        (Adapter->RevisionID == FXP_REV_82551_10))
    {
        /* We need extended TxCB support too */
        ASSERT(Adapter->Flags & E100_FLAG_EXT_TXCB);

        /* -1 for IPCB */
        --Adapter->TbdCount;

        Adapter->RfdSize = sizeof(FXP_RFD);
        Adapter->TransmitCommand = htole16(FXP_CB_COMMAND_IPCBXMIT |
                                           FXP_CB_COMMAND_SF |
                                           FXP_CB_COMMAND_S |
                                           FXP_CB_COMMAND_I);

        /* Use extended RFA instead of 82559 checksum mode */
        Adapter->Flags |= E100_FLAG_EXT_RFA;
        Adapter->Flags &= ~E100_FLAG_82559_RXCSUM;
    }
    else
    {
        Adapter->RfdSize = RTL_SIZEOF_THROUGH_FIELD(FXP_RFD, Size);
        Adapter->TransmitCommand = htole16(FXP_CB_COMMAND_XMIT |
                                           FXP_CB_COMMAND_SF |
                                           FXP_CB_COMMAND_S |
                                           FXP_CB_COMMAND_I);
    }

    Status = EeReadConfiguration(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    Status = EeAllocateMemory(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    EeCreateTxRing(Adapter);
    EeCreateRxRing(Adapter);

    Status = EeFindMiiPhy(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    // TODO: Improve
    if (Adapter->Flags & E100_FLAG_HAS_WOL)
    {
        CSR_WRITE_8(Adapter, FXP_CSR_PMDR, CSR_READ_8(Adapter, FXP_CSR_PMDR));

    }

    Status = EeSetupAdapter(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Disable;

    Status = NdisMRegisterInterrupt(&Adapter->Interrupt,
                                    Adapter->AdapterHandle,
                                    Adapter->InterruptVector,
                                    Adapter->InterruptLevel,
                                    TRUE, /* Request ISR calls */
                                    !!(Adapter->Flags & E100_FLAG_IRQ_SHARED),
                                    (Adapter->InterruptFlags & CM_RESOURCE_INTERRUPT_LATCHED) ?
                                    NdisInterruptLatched : NdisInterruptLevelSensitive);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Unable to register interrupt\n");
        goto Disable;
    }

    EeStartAdapter(Adapter);

    return NDIS_STATUS_SUCCESS;

Disable:
    EeSoftReset(Adapter);
Failure:
    ERR("Initialization failed with status %08lx\n", Status);

    EeFreeAdapter(Adapter);
    return Status;
}
