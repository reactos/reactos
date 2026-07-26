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

C_ASSERT((SYSTEM_CACHE_ALIGNMENT_SIZE % FXP_BUFFER_ALIGNMENT) == 0);

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
        TRACE("'%S' request failed, default to %lu\n", EntryName, DefaultValue);

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
                         L"*TransmitBuffers",
                         &GenericUlong,
                         E100_TRANSMIT_BLOCKS_DEFAULT,
                         E100_TRANSMIT_BLOCKS_MIN,
                         E100_TRANSMIT_BLOCKS_MAX);
    Adapter->TcbCount = GenericUlong;

    EeConfigQueryInteger(ConfigurationHandle,
                         L"*ReceiveBuffers",
                         &GenericUlong,
                         E100_RECEIVE_BUFFERS_DEFAULT,
                         E100_RECEIVE_BUFFERS_MIN,
                         E100_RECEIVE_BUFFERS_MAX);
    Adapter->RfdToAllocate = GenericUlong;

    EeConfigQueryInteger(ConfigurationHandle,
                         L"*SpeedDuplex",
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
                             L"*FlowControl",
                             &GenericUlong,
                             4,
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
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_TX;
                break;
            case 2:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_RX;
                break;
            case 3:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX;
                break;
            case 4:
                Adapter->DefaultMedia |= E100_MEDIA_PAUSE_AUTO |
                                         E100_MEDIA_PAUSE_TX |
                                         E100_MEDIA_PAUSE_RX;
                break;

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }
    }

    /* Intel microcode parameters */
    if (!(Adapter->Flags & E100_FLAG_NO_UCODE))
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"UCodeIntDelay",
                             &GenericUlong,
                             1536,
                             0,
                             65535);
        Adapter->MicrocodeInterruptDelay = GenericUlong;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"UCodeBundleMax",
                             &GenericUlong,
                             6,
                             1,
                             65535);
        Adapter->MicrocodeMaxFramesPerIntr = GenericUlong;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"UCodeBundleSmall",
                             &GenericUlong,
                             1,
                             0,
                             1);
        Adapter->MicrocodeMinSizeMask = GenericUlong ? 0xFFFF : 0xFF80;
    }

    /* WOL */
    if ((Adapter->Flags & E100_FLAG_HAS_WOL) || (Adapter->RevisionID == FXP_REV_82559S_A))
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"WolLinkSpeed",
                             &GenericUlong,
                             1,
                             0,
                             1);
        if (GenericUlong)
            Adapter->Flags |= E100_FLAG_REDUCE_WOL_LINK_SPEED;
    }

    /* RX/TX checksum offload */
    if ((Adapter->Flags & E100_FLAG_82559_RXCSUM) || (Adapter->Flags & E100_FLAG_EXT_RFA))
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"ChecksumOffload",
                             &GenericUlong,
                             1,
                             0,
                             1);
        if (GenericUlong)
            Adapter->Flags |= E100_FLAG_CSUM_OFFLOAD;
    }

    /* VLAN tagging and LSO */
    if (Adapter->Flags & E100_FLAG_EXT_RFA)
    {
        EeConfigQueryInteger(ConfigurationHandle,
                             L"*PriorityVLANTag",
                             &GenericUlong,
                             3,
                             0,
                             3);
        switch (GenericUlong)
        {
            case 0:
                break;
            case 1:
                Adapter->Flags |= E100_FLAG_PACKET_PRIORITY;
                break;
            case 2:
                Adapter->Flags |= E100_FLAG_VLAN_TAGGING;
                break;
            case 3:
                Adapter->Flags |= E100_FLAG_PACKET_PRIORITY | E100_FLAG_VLAN_TAGGING;
                break;

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }

        EeConfigQueryInteger(ConfigurationHandle,
                             L"VlanID",
                             &GenericUlong,
                             0,
                             0,
                             E100_MAXIMUM_VLAN_ID);
        Adapter->VlanId = GenericUlong;

        EeConfigQueryInteger(ConfigurationHandle,
                             L"*LsoV1IPv4",
                             &GenericUlong,
                             1,
                             0,
                             1);
        if (GenericUlong)
            Adapter->Flags |= E100_FLAG_LARGE_SEND_OFFLOAD;
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
        RtlCopyMemory(Adapter->CurrentMacAddress,
                      NetworkAddress,
                      ETH_LENGTH_OF_ADDRESS);
    }
    else
    {
        RtlCopyMemory(Adapter->CurrentMacAddress,
                      Adapter->PermanentMacAddress,
                      ETH_LENGTH_OF_ADDRESS);
    }

    NdisCloseConfiguration(ConfigurationHandle);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
EeFreeSharedMemory(
    _In_ PE100_ADAPTER Adapter,
    _In_ BOOLEAN Cached,
    _In_ PVOID VirtualAddress,
    _In_ ULONG PhysicalAddress)
{
    PE100_SHARED_MEMORY_METADATA Metadata;
    PVOID VirtualAddressOriginal;
    NDIS_PHYSICAL_ADDRESS PhysicalAddressOriginal;

    PAGED_CODE();

    Metadata = (PE100_SHARED_MEMORY_METADATA)VirtualAddress - 1;

    ASSERT(Metadata->Signature == E100_SHARED_MEMORY_SIGNATURE);

    VirtualAddressOriginal = (PVOID)((ULONG_PTR)VirtualAddress - Metadata->VirtOffset);
    PhysicalAddressOriginal.QuadPart = PhysicalAddress - Metadata->PhysOffset;

    NdisMFreeSharedMemory(Adapter->AdapterHandle,
                          Metadata->BlockSize,
                          Cached,
                          VirtualAddressOriginal,
                          PhysicalAddressOriginal);
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateSharedMemory(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG Size,
    _In_ ULONG Alignment,
    _In_ ULONG Shift,
    _In_ BOOLEAN Cached,
    _Out_opt_ ULONG* BuffersPerPage,
    _Out_ PVOID* ResultVirtualAddress,
    _Out_ ULONG* ResultPhysicalAddress)
{
    PE100_SHARED_MEMORY_METADATA Metadata;
    PVOID VirtOriginal;
    ULONG_PTR VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysOriginal;
    ULONG PhysicalAddress, BlockSize;

    PAGED_CODE();

    ASSERT(Alignment > 0 && Alignment < 0x10000);

    BlockSize = (Alignment - 1) + Shift + Size;

    if (BuffersPerPage)
    {
        ULONG NumPerPage;

        /* Allocate one-page chunks to reduce memory usage */
        NumPerPage = (PAGE_SIZE - sizeof(*Metadata)) / BlockSize;
        NumPerPage = max(NumPerPage, 1);
        *BuffersPerPage = NumPerPage;

        BlockSize *= NumPerPage;
    }
    BlockSize += sizeof(*Metadata);

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              BlockSize,
                              Cached,
                              &VirtOriginal,
                              &PhysOriginal);
    if (!VirtOriginal)
        return NDIS_STATUS_RESOURCES;
    RtlZeroMemory(VirtOriginal, BlockSize);

    /* 32-bit DMA */
    ASSERT(PhysOriginal.HighPart == 0);

    VirtualAddress = (ULONG_PTR)VirtOriginal + sizeof(*Metadata);
    VirtualAddress = ALIGN_UP_BY(VirtualAddress, Alignment) + Shift;

    PhysicalAddress = PhysOriginal.LowPart + sizeof(*Metadata);
    PhysicalAddress = ALIGN_UP_BY(PhysicalAddress, Alignment) + Shift;

    Metadata = (PE100_SHARED_MEMORY_METADATA)VirtualAddress - 1;
    Metadata->BlockSize = BlockSize;
    Metadata->VirtOffset = VirtualAddress - (ULONG_PTR)VirtOriginal;
    Metadata->PhysOffset = PhysicalAddress - PhysOriginal.LowPart;
#if DBG
    Metadata->Signature = E100_SHARED_MEMORY_SIGNATURE;
#endif

    *ResultPhysicalAddress = PhysicalAddress;
    *ResultVirtualAddress = (PVOID)VirtualAddress;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
BOOLEAN
EeAllocateRfd(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PE100_RX_CONTEXT RxContext)
{
    NDIS_STATUS Status;

    PAGED_CODE();

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
    {
        NdisFreeBuffer(RxContext->RfdMdl);
        RxContext->RfdMdl = NULL;
    }
    if (RxContext->ReceiveBufferMdl)
    {
        NdisFreeBuffer(RxContext->ReceiveBufferMdl);
        RxContext->ReceiveBufferMdl = NULL;
    }
    if (RxContext->Packet)
    {
        NdisFreePacket(RxContext->Packet);
        RxContext->Packet = NULL;
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateReceiveBuffers(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    PVOID VirtualAddress;
    ULONG i, PhysicalAddress, RfdBufferSize;
    ULONG RfdPerPage = 0;

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
    RtlZeroMemory(Adapter->RxContext, sizeof(*Adapter->RxContext) * Adapter->RfdToAllocate);

    RfdBufferSize = Adapter->RfdSize + E100_RECEIVE_BLOCK_SIZE;

    /* Allocate RFDs */
    for (i = 0; i < Adapter->RfdToAllocate; ++i)
    {
        PE100_RX_CONTEXT RxContext = &Adapter->RxContext[i];

        if ((RfdPerPage == 0) || (i % RfdPerPage) == 0)
        {
            /* Allocate a chunk of memory */
            Status = EeAllocateSharedMemory(Adapter,
                                            RfdBufferSize,
                                            SYSTEM_CACHE_ALIGNMENT_SIZE,
                                            E100_RECEIVE_BUFFER_SHIFT,
                                            TRUE, /* Cached */
                                            &RfdPerPage,
                                            (PVOID*)&VirtualAddress,
                                            &PhysicalAddress);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                WARN("RFD allocation failed, total buffers %lu\n", Adapter->RfdCount);
                break;
            }

            RxContext->Flags |= E100_RX_FLAG_HAS_MEMORY;
        }
        else
        {
            VirtualAddress = ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
            PhysicalAddress = ALIGN_UP_BY(PhysicalAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);

            /*
             * NOTE: This makes the LinkAddress and RbdAddress fields
             * to be not aligned to 4 bytes.
             */
            VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + E100_RECEIVE_BUFFER_SHIFT);
            PhysicalAddress += E100_RECEIVE_BUFFER_SHIFT;
        }

        RxContext->Rfd = VirtualAddress;
        RxContext->RfdPhys = PhysicalAddress;

        ASSERT((RxContext->RfdPhys % FXP_BUFFER_ALIGNMENT) == 0);

        if (!EeAllocateRfd(Adapter, RxContext))
        {
            WARN("RFD allocation failed, total buffers %lu\n", Adapter->RfdCount);
            break;
        }

        /* Split the allocation */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + RfdBufferSize);
        PhysicalAddress += RfdBufferSize;

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

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->TxContext,
                                       sizeof(*Adapter->TxContext) * Adapter->TcbCount,
                                       E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;
    RtlZeroMemory(Adapter->TxContext, sizeof(*Adapter->TxContext) * Adapter->TcbCount);

    Status = EeAllocateSharedMemory(Adapter,
                                    Adapter->TcbCount * sizeof(FXP_CB_TRANSMIT),
                                    E100_TCB_ALIGNMENT,
                                    0,
                                    FALSE, /* Non-cached */
                                    NULL,
                                    (PVOID*)&Adapter->HeadTcb,
                                    &Adapter->HeadTcbPa);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateTransmitBuffers(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    PVOID VirtualAddress;
    ULONG i, PhysicalAddress;
    ULONG BuffersPerPage = 0;

    PAGED_CODE();

    for (i = 0; i < E100_TRANSMIT_BUFFERS; ++i)
    {
        PE100_COALESCE_BUFFER CoalesceBuffer = &Adapter->CoalesceBuffer[i];

        if ((BuffersPerPage == 0) || (i % BuffersPerPage) == 0)
        {
            /* Allocate a chunk of memory */
            Status = EeAllocateSharedMemory(Adapter,
                                            E100_TRANSMIT_BLOCK_SIZE,
                                            SYSTEM_CACHE_ALIGNMENT_SIZE,
                                            0,
                                            FALSE, /* Non-cached */
                                            &BuffersPerPage,
                                            (PVOID*)&VirtualAddress,
                                            &PhysicalAddress);
            if (Status != NDIS_STATUS_SUCCESS)
                return Status;

            CoalesceBuffer->Flags |= E100_BUFFER_FLAG_HAS_MEMORY;
        }
        else
        {
            VirtualAddress = ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
            PhysicalAddress = ALIGN_UP_BY(PhysicalAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
        }

        ASSERT(((ULONG_PTR)VirtualAddress % SYSTEM_CACHE_ALIGNMENT_SIZE) == 0);
        ASSERT((PhysicalAddress % SYSTEM_CACHE_ALIGNMENT_SIZE) == 0);

        CoalesceBuffer->VirtualAddress = VirtualAddress;
        CoalesceBuffer->PhysicalAddress = PhysicalAddress;

        PushEntryList(&Adapter->SendBufferList, &CoalesceBuffer->ListEntry);

        /* Split the allocation */
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + E100_TRANSMIT_BLOCK_SIZE);
        PhysicalAddress += E100_TRANSMIT_BLOCK_SIZE;
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
EeAllocateControlBlock(
    _In_ PE100_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    PAGED_CODE();

    Status = EeAllocateSharedMemory(Adapter,
                                    sizeof(*Adapter->ControlBlock),
                                    SYSTEM_CACHE_ALIGNMENT_SIZE,
                                    0,
                                    FALSE, /* Non-cached */
                                    NULL,
                                    (PVOID*)&Adapter->ControlBlock,
                                    &Adapter->ControlBlockPa);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

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

CODE_SEG("PAGE")
VOID
EeInitTxRing(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_TX_CONTEXT PrevTxContext, TxContext;
    PFXP_CB_TRANSMIT PrevTcb, Tcb;
    ULONG i, TcbPa;

    PAGED_CODE();

    RtlZeroMemory(Adapter->HeadTcb, sizeof(*Tcb) * Adapter->TcbCount);

    PrevTxContext = &Adapter->TxContext[Adapter->TcbCount - 1];
    TxContext = Adapter->TxContext;
    PrevTcb = &Adapter->HeadTcb[Adapter->TcbCount - 1];
    Tcb = Adapter->HeadTcb;
    TcbPa = Adapter->HeadTcbPa;

    for (i = 0; i < Adapter->TcbCount; ++i)
    {
        Tcb->Header.Status = 0;
        Tcb->Header.Command = htole16(FXP_CB_COMMAND_NOP | FXP_CB_COMMAND_S);

        if (Adapter->Flags & E100_FLAG_EXT_TXCB)
        {
            /* The TBD address points to the third TBD in this mode */
            Tcb->TbdArrayAddress = htole32(TcbPa + FIELD_OFFSET(FXP_CB_TRANSMIT, Tbd[2]));
        }
        else
        {
            Tcb->TbdArrayAddress = htole32(TcbPa + FIELD_OFFSET(FXP_CB_TRANSMIT, Tbd[0]));
        }

        ASSERT((TcbPa % FXP_BUFFER_ALIGNMENT) == 0);
        ASSERT((Tcb->TbdArrayAddress % FXP_BUFFER_ALIGNMENT) == 0);

        PrevTcb->Header.LinkAddress = htole32(TcbPa);

        TxContext->Tcb = Tcb;
        PrevTxContext->Next = TxContext;

        PrevTxContext = TxContext;
        PrevTcb = Tcb;

        TxContext++;
        Tcb++;
        TcbPa += sizeof(*Tcb);
    }

    Adapter->TxLast = &Adapter->TxContext[Adapter->TcbCount - 1];
    Adapter->TxFirst = Adapter->TxLast->Next;
    Adapter->TxPending = 0;

    InitializeListHead(&Adapter->SendQueueList);
}

static
CODE_SEG("PAGE")
VOID
EeCreateRfaChain(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_RX_CONTEXT RxContext = Adapter->RxContext;
    PE100_RX_CONTEXT LastRxContext = NULL;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < Adapter->RfdCount; ++i)
    {
        PFXP_RFD Rfd;

        if (LastRxContext)
        {
            le32enc(&LastRxContext->Rfd->Header.LinkAddress, RxContext->RfdPhys);
            NdisFlushBuffer(LastRxContext->RfdMdl, TRUE);
        }

        Rfd = RxContext->Rfd;
        Rfd->Header.Status = 0;
        Rfd->Header.Command = 0;
        le32enc(&Rfd->Header.LinkAddress, 0xFFFFFFFF);
        le32enc(&Rfd->RbdAddress, 0xFFFFFFFF);
        Rfd->Size = htole16(E100_RECEIVE_BLOCK_SIZE);
        Rfd->ActualSize = 0;

        InsertTailList(&Adapter->RxContextList, &RxContext->ListEntry);

        LastRxContext = RxContext;
        RxContext++;
    }

    /* Last entry */
    LastRxContext->Rfd->Header.Command = htole16(FXP_RFD_CONTROL_EL);
    NdisFlushBuffer(LastRxContext->RfdMdl, TRUE);
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
    INFO("IRQ Level %lu, Vector %lu\n",
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
    ULONG i, BytesRead, Flags;

    PAGED_CODE();

    BytesRead = NdisReadPciSlotInformation(Adapter->AdapterHandle,
                                           0,
                                           FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                                           Buffer,
                                           sizeof(Buffer));
    if (BytesRead != sizeof(Buffer))
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
        INFO("MWI enabled\n");
        Adapter->Flags |= E100_FLAG_MWI_ENABLE;
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
    RtlZeroMemory(UnalignedAdapter, AdapterSize);

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

    /* The ISR handler has to be unregistered first */
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

    if (Adapter->ControlBlock)
    {
        EeFreeSharedMemory(Adapter,
                           FALSE, /* Non-cached */
                           Adapter->ControlBlock,
                           Adapter->ControlBlockPa);
    }

    if (Adapter->TxContext)
    {
        NdisFreeMemory(Adapter->TxContext, sizeof(*Adapter->TxContext) * Adapter->TcbCount, 0);
    }

    if (Adapter->HeadTcb)
    {
        EeFreeSharedMemory(Adapter,
                           FALSE, /* Non-cached */
                           Adapter->HeadTcb,
                           Adapter->HeadTcbPa);
    }

    if (Adapter->RxContext)
    {
        for (i = 0; i < Adapter->RfdToAllocate; ++i)
        {
            PE100_RX_CONTEXT RxContext = &Adapter->RxContext[i];

            if (RxContext->Flags & E100_RX_FLAG_HAS_MEMORY)
            {
                EeFreeSharedMemory(Adapter,
                                   FALSE, /* Non-cached */
                                   RxContext->Rfd,
                                   RxContext->RfdPhys);
            }

            if (RxContext->RfdMdl)
                NdisFreeBuffer(RxContext->RfdMdl);
            if (RxContext->ReceiveBufferMdl)
                NdisFreeBuffer(RxContext->ReceiveBufferMdl);
            if (RxContext->Packet)
                NdisFreePacket(RxContext->Packet);
        }

        NdisFreeMemory(Adapter->RxContext, sizeof(*Adapter->RxContext) * Adapter->RfdToAllocate, 0);
    }

    if (Adapter->CoalesceBuffer[0].VirtualAddress)
    {
        for (i = 0; i < E100_TRANSMIT_BUFFERS; ++i)
        {
            PE100_COALESCE_BUFFER CoalesceBuffer = &Adapter->CoalesceBuffer[i];

            if (CoalesceBuffer->Flags & E100_BUFFER_FLAG_HAS_MEMORY)
            {
                EeFreeSharedMemory(Adapter,
                                   FALSE, /* Non-cached */
                                   CoalesceBuffer->VirtualAddress,
                                   CoalesceBuffer->PhysicalAddress);
            }
        }
    }

    while (!IsListEmpty(&Adapter->WakeUpFrameList))
    {
        PLIST_ENTRY Entry = RemoveHeadList(&Adapter->WakeUpFrameList);
        PE100_WAKE_UP_FRAME WakeFrame = CONTAINING_RECORD(Entry, E100_WAKE_UP_FRAME, ListEntry);

        NdisFreeMemory(WakeFrame, sizeof(*WakeFrame), 0);
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

    UNREFERENCED_PARAMETER(OpenErrorStatus);

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
    NdisInitializeWorkItem(&Adapter->ResetWorkItem, EeResetWorker, Adapter);
    NdisInitializeWorkItem(&Adapter->PowerWorkItem, EePowerWorker, Adapter);

    InitializeListHead(&Adapter->WakeUpFrameList);

    Adapter->PowerState = NdisDeviceStateD0;
    Adapter->PrevPowerState = Adapter->PowerState;

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

    EeSoftReset(Adapter, TRUE);

    if (!EeReadEeprom(Adapter))
        goto Failure;

    Adapter->MaxTbdCount = E100_TBD_PER_TCB;
    Adapter->TransmitThreshold = 512 / 8;

    if (Adapter->RevisionID == FXP_REV_82557)
    {
        /* Older chips do not support MWI */
        Adapter->Flags &= ~E100_FLAG_MWI_ENABLE;

        /* A hack to get long VLAN frames on a 82557 */
        Adapter->Flags |= E100_FLAG_SAVE_BAD_PACKETS;
    }
    else
    {
        /* Enable reception of long frames for VLAN */
        Adapter->Flags |= E100_FLAG_LONG_PKT;

        /* The 82558 and later chips have flow control */
        Adapter->Flags |= E100_FLAG_HAS_FLOW_CONTROL;

        /* Turn on the extended TxCB feature */
        Adapter->Flags |= E100_FLAG_EXT_TXCB;

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
        --Adapter->MaxTbdCount;

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

    EeCreateRfaChain(Adapter);

    Status = EeFindMiiPhy(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    if ((Adapter->Flags & E100_FLAG_HAS_WOL) || (Adapter->RevisionID == FXP_REV_82559S_A))
    {
        /* Clear wakeup events */
        CSR_WRITE_8(Adapter, FXP_CSR_PMDR, CSR_READ_8(Adapter, FXP_CSR_PMDR));
    }

    EePhySetPower(Adapter, TRUE);

    Status = EeSetupAdapter(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        goto StopHardware;

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
        goto StopHardware;
    }

    EeStartAdapter(Adapter);

    return NDIS_STATUS_SUCCESS;

StopHardware:
    EeSoftReset(Adapter, FALSE);
Failure:
    ERR("Initialization failed with status %08lx\n", Status);

    EeFreeAdapter(Adapter);
    return Status;
}
