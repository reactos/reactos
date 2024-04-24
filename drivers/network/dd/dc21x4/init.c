/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport initialization helper routines
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

/*
 * The driver must align the buffers on a 4 byte boundary to meet the hardware requirement.
 * We stick with cache alignment to get better performance.
 */
C_ASSERT((SYSTEM_CACHE_ALIGNMENT_SIZE % DC_RECEIVE_BUFFER_ALIGNMENT) == 0);
C_ASSERT((SYSTEM_CACHE_ALIGNMENT_SIZE % DC_DESCRIPTOR_ALIGNMENT) == 0);
C_ASSERT((SYSTEM_CACHE_ALIGNMENT_SIZE % DC_SETUP_FRAME_ALIGNMENT) == 0);

/* Errata: The end of receive buffer must not fall on a cache boundary */
#define DC_RECEIVE_BUFFER_SIZE     (DC_RECEIVE_BLOCK_SIZE - 4)
C_ASSERT((DC_RECEIVE_BUFFER_SIZE % 32) != 0);

#define DC_MEM_BLOCK_SIZE_RCB \
    (DC_RECEIVE_BLOCK_SIZE + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

#define DC_MEM_BLOCK_SIZE_RBD \
    (sizeof(DC_RBD) * DC_RECEIVE_BUFFERS_DEFAULT + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

#define DC_MEM_BLOCK_SIZE_TBD_AUX \
    (sizeof(DC_TBD) * DC_TRANSMIT_DESCRIPTORS + SYSTEM_CACHE_ALIGNMENT_SIZE - 1 + \
     DC_SETUP_FRAME_SIZE + SYSTEM_CACHE_ALIGNMENT_SIZE - 1 + \
     DC_LOOPBACK_FRAME_SIZE * DC_LOOPBACK_FRAMES + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

#define DC_MEM_BLOCK_SIZE_TX_BUFFER \
    (DC_TRANSMIT_BLOCK_SIZE + SYSTEM_CACHE_ALIGNMENT_SIZE - 1)

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
DcConfigQueryInteger(
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
        TRACE("'%S' request failed, default value %u\n", EntryName, DefaultValue);

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

    TRACE("Set '%S' to %u\n", EntryName, *EntryContext);
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcReadConfiguration(
    _In_ PDC21X4_ADAPTER Adapter)
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

    DcConfigQueryInteger(ConfigurationHandle,
                         L"SpeedDuplex",
                         &GenericUlong,
                         MEDIA_AUTO,
                         MEDIA_10T,
                         MEDIA_HMR);
    Adapter->DefaultMedia = GenericUlong;

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
        }
        else
        {
            INFO("Using software MAC address\n");

            /* Override the MAC address */
            NdisMoveMemory(Adapter->CurrentMacAddress, NetworkAddress, ETH_LENGTH_OF_ADDRESS);
        }
    }

    NdisCloseConfiguration(ConfigurationHandle);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
DcFreeRcb(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ __drv_freesMem(Mem) PDC_RCB Rcb)
{
    PAGED_CODE();

    if (Rcb->VirtualAddressOriginal)
    {
        NdisMFreeSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_RCB,
                              TRUE, /* Cached */
                              Rcb->VirtualAddressOriginal,
                              Rcb->PhysicalAddressOriginal);
    }

    if (Rcb->NdisBuffer)
        NdisFreeBuffer(Rcb->NdisBuffer);
    if (Rcb->Packet)
        NdisFreePacket(Rcb->Packet);

    NdisFreeMemory(Rcb, sizeof(*Rcb), 0);
}

static
CODE_SEG("PAGE")
PDC_RCB
DcAllocateRcb(
    _In_ PDC21X4_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    PDC_RCB Rcb;
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&Rcb, sizeof(*Rcb), DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return NULL;
    NdisZeroMemory(Rcb, sizeof(*Rcb));

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_RCB,
                              TRUE, /* Cached */
                              &VirtualAddress,
                              &PhysicalAddress);
    if (!VirtualAddress)
        goto Failure;

    /* 32-bit DMA */
    ASSERT(PhysicalAddress.HighPart == 0);

    Rcb->VirtualAddress = ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
    Rcb->PhysicalAddress = ALIGN_UP_BY(PhysicalAddress.LowPart, SYSTEM_CACHE_ALIGNMENT_SIZE);

    ASSERT((Rcb->PhysicalAddress % DC_RECEIVE_BUFFER_ALIGNMENT) == 0);

    Rcb->VirtualAddressOriginal = VirtualAddress;
    Rcb->PhysicalAddressOriginal.QuadPart = PhysicalAddress.QuadPart;

    NdisAllocatePacket(&Status, &Rcb->Packet, Adapter->PacketPool);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    *DC_RCB_FROM_PACKET(Rcb->Packet) = Rcb;

    NdisAllocateBuffer(&Status,
                       &Rcb->NdisBuffer,
                       Adapter->BufferPool,
                       Rcb->VirtualAddress,
                       DC_RECEIVE_BLOCK_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
        goto Failure;

    NDIS_SET_PACKET_HEADER_SIZE(Rcb->Packet, DC_ETHERNET_HEADER_SIZE);
    NdisChainBufferAtFront(Rcb->Packet, Rcb->NdisBuffer);

    PushEntryList(&Adapter->AllocRcbList, &Rcb->AllocListEntry);

    return Rcb;

Failure:
    DcFreeRcb(Adapter, Rcb);

    return NULL;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateReceiveBuffers(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i;
    NDIS_STATUS Status;
    PDC_RCB Rcb;

    PAGED_CODE();

    NdisAllocatePacketPool(&Status,
                           &Adapter->PacketPool,
                           DC_RECEIVE_BUFFERS_DEFAULT + DC_RECEIVE_BUFFERS_EXTRA,
                           PROTOCOL_RESERVED_SIZE_IN_PACKET);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisAllocateBufferPool(&Status,
                           &Adapter->BufferPool,
                           DC_RECEIVE_BUFFERS_DEFAULT + DC_RECEIVE_BUFFERS_EXTRA);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    /* Allocate RCBs */
    for (i = 0; i < DC_RECEIVE_BUFFERS_DEFAULT; ++i)
    {
        Rcb = DcAllocateRcb(Adapter);
        if (!Rcb)
        {
            WARN("RCB allocation failed, total buffers %u\n", Adapter->RcbCount);
            break;
        }

        PushEntryList(&Adapter->UsedRcbList, &Rcb->ListEntry);

        ++Adapter->RcbCount;
    }

    if (Adapter->RcbCount < DC_RECEIVE_BUFFERS_MIN)
        return NDIS_STATUS_RESOURCES;

    Adapter->RcbFree = Adapter->RcbCount;

    /* Fix up the ring size */
    Adapter->TailRbd = Adapter->HeadRbd + Adapter->RcbCount - 1;

    /* Allocate extra RCBs for receive indication */
    for (i = 0; i < DC_RECEIVE_BUFFERS_EXTRA; ++i)
    {
        Rcb = DcAllocateRcb(Adapter);
        if (!Rcb)
        {
            WARN("Extra RCB allocation failed\n");
            break;
        }

        PushEntryList(&Adapter->FreeRcbList, &Rcb->ListEntry);
    }

    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->RcbArray,
                                       sizeof(PVOID) * Adapter->RcbCount,
                                       DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateReceiveDescriptors(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_RBD Rbd;

    PAGED_CODE();

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_RBD,
                              FALSE, /* Non-cached */
                              &Adapter->RbdOriginal,
                              &Adapter->RbdPhysOriginal);
    if (!Adapter->RbdOriginal)
        return NDIS_STATUS_RESOURCES;

    /* 32-bit DMA */
    ASSERT(Adapter->RbdPhysOriginal.HighPart == 0);

    Adapter->RbdPhys = ALIGN_UP_BY(Adapter->RbdPhysOriginal.LowPart, SYSTEM_CACHE_ALIGNMENT_SIZE);

    ASSERT((Adapter->RbdPhys % DC_DESCRIPTOR_ALIGNMENT) == 0);

    Rbd = ALIGN_UP_POINTER_BY(Adapter->RbdOriginal, SYSTEM_CACHE_ALIGNMENT_SIZE);

    Adapter->HeadRbd = Rbd;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateTransmitBlocks(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_TCB Tcb;
    NDIS_STATUS Status;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&Tcb,
                                       DC_TRANSMIT_BLOCKS * sizeof(*Tcb),
                                       DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisZeroMemory(Tcb, DC_TRANSMIT_BLOCKS * sizeof(*Tcb));

    Adapter->HeadTcb = Tcb;
    Adapter->TailTcb = Tcb + (DC_TRANSMIT_BLOCKS - 1);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateTransmitDescriptorsAndBuffers(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG_PTR BufferVa, BufferPa;
    NDIS_STATUS Status;
    ULONG i;

    PAGED_CODE();

    NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_TBD_AUX,
                              FALSE, /* Non-cached */
                              &Adapter->TbdOriginal,
                              &Adapter->TbdPhysOriginal);
    if (!Adapter->TbdOriginal)
        return NDIS_STATUS_RESOURCES;

    /* 32-bit DMA */
    ASSERT(Adapter->TbdPhysOriginal.HighPart == 0);

    BufferVa = (ULONG_PTR)Adapter->TbdOriginal;
    BufferPa = Adapter->TbdPhysOriginal.LowPart;

    BufferVa = ALIGN_UP_BY(BufferVa, SYSTEM_CACHE_ALIGNMENT_SIZE);
    BufferPa = ALIGN_UP_BY(BufferPa, SYSTEM_CACHE_ALIGNMENT_SIZE);

    ASSERT((BufferPa % DC_DESCRIPTOR_ALIGNMENT) == 0);

    Adapter->TbdPhys = (ULONG)BufferPa;
    Adapter->HeadTbd = (PDC_TBD)BufferVa;
    Adapter->TailTbd = (PDC_TBD)BufferVa + DC_TRANSMIT_DESCRIPTORS - 1;

    BufferVa += sizeof(DC_TBD) * DC_TRANSMIT_DESCRIPTORS;
    BufferPa += sizeof(DC_TBD) * DC_TRANSMIT_DESCRIPTORS;

    BufferVa = ALIGN_UP_BY(BufferVa, SYSTEM_CACHE_ALIGNMENT_SIZE);
    BufferPa = ALIGN_UP_BY(BufferPa, SYSTEM_CACHE_ALIGNMENT_SIZE);

    ASSERT((BufferPa % DC_SETUP_FRAME_ALIGNMENT) == 0);

    Adapter->SetupFrame = (PVOID)BufferVa;
    Adapter->SetupFramePhys = BufferPa;

    BufferVa += DC_SETUP_FRAME_SIZE;
    BufferPa += DC_SETUP_FRAME_SIZE;

    BufferVa = ALIGN_UP_BY(BufferVa, SYSTEM_CACHE_ALIGNMENT_SIZE);
    BufferPa = ALIGN_UP_BY(BufferPa, SYSTEM_CACHE_ALIGNMENT_SIZE);

    for (i = 0; i < DC_LOOPBACK_FRAMES; ++i)
    {
        Adapter->LoopbackFrame[i] = (PVOID)BufferVa;
        Adapter->LoopbackFramePhys[i] = BufferPa;

        BufferVa += DC_LOOPBACK_FRAME_SIZE;
        BufferPa += DC_LOOPBACK_FRAME_SIZE;
    }

    if (Adapter->Features & DC_HAS_POWER_MANAGEMENT)
    {
        Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->SetupFrameSaved,
                                           DC_SETUP_FRAME_SIZE,
                                           DC21X4_TAG);
        if (Status != NDIS_STATUS_SUCCESS)
            return Status;
    }

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateTransmitBuffers(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_COALESCE_BUFFER CoalesceBuffer;
    NDIS_STATUS Status;
    ULONG i;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&CoalesceBuffer,
                                       DC_TRANSMIT_BUFFERS * sizeof(*CoalesceBuffer),
                                       DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisZeroMemory(CoalesceBuffer, DC_TRANSMIT_BUFFERS * sizeof(*CoalesceBuffer));

    Adapter->CoalesceBuffer = CoalesceBuffer;

    for (i = 0; i < DC_TRANSMIT_BUFFERS; ++i)
    {
        PVOID VirtualAddress;
        NDIS_PHYSICAL_ADDRESS PhysicalAddress;

        NdisMAllocateSharedMemory(Adapter->AdapterHandle,
                                  DC_MEM_BLOCK_SIZE_TX_BUFFER,
                                  FALSE, /* Non-cached */
                                  &VirtualAddress,
                                  &PhysicalAddress);
        if (!VirtualAddress)
            continue;

        ASSERT(PhysicalAddress.HighPart == 0);

        CoalesceBuffer->VirtualAddress =
            ALIGN_UP_POINTER_BY(VirtualAddress, SYSTEM_CACHE_ALIGNMENT_SIZE);
        CoalesceBuffer->PhysicalAddress =
            ALIGN_UP_BY(PhysicalAddress.LowPart, SYSTEM_CACHE_ALIGNMENT_SIZE);

        Adapter->SendBufferData[i].PhysicalAddress.QuadPart = PhysicalAddress.QuadPart;
        Adapter->SendBufferData[i].VirtualAddress = VirtualAddress;

        PushEntryList(&Adapter->SendBufferList, &CoalesceBuffer->ListEntry);

        ++CoalesceBuffer;
    }

    if (!Adapter->SendBufferList.Next)
        return NDIS_STATUS_RESOURCES;

    return NDIS_STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
DcInitTxRing(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_TCB Tcb;
    PDC_TBD Tbd;

    PAGED_CODE();

    InitializeListHead(&Adapter->SendQueueList);

    Tcb = Adapter->HeadTcb;

    Adapter->CurrentTcb = Tcb;
    Adapter->LastTcb = Tcb;

    Adapter->TcbSlots = DC_TRANSMIT_BLOCKS - DC_TCB_RESERVE;
    Adapter->TbdSlots = DC_TRANSMIT_DESCRIPTORS - DC_TBD_RESERVE;
    Adapter->LoopbackFrameSlots = DC_LOOPBACK_FRAMES;
    Adapter->TcbCompleted = 0;

    Tbd = Adapter->HeadTbd;
    Adapter->CurrentTbd = Tbd;

    NdisZeroMemory(Tbd, sizeof(*Tbd) * DC_TRANSMIT_DESCRIPTORS);

    Adapter->TailTbd->Control |= DC_TBD_CONTROL_END_OF_RING;
}

static
CODE_SEG("PAGE")
VOID
DcCreateRxRing(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_RCB* RcbSlot;
    PDC_RCB Rcb;
    PDC_RBD Rbd;
    PSINGLE_LIST_ENTRY Entry;

    PAGED_CODE();

    Rbd = Adapter->HeadRbd;
    Adapter->CurrentRbd = Rbd;

    RcbSlot = DC_GET_RCB_SLOT(Adapter, Rbd);
    Rcb = (PDC_RCB)Adapter->UsedRcbList.Next;

    for (Entry = Adapter->UsedRcbList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        Rcb = (PDC_RCB)Entry;

        C_ASSERT((DC_RECEIVE_BUFFER_SIZE % DC_RECEIVE_BUFFER_SIZE_MULTIPLE) == 0);

        Rbd->Address1 = Rcb->PhysicalAddress;
        Rbd->Address2 = 0;
        Rbd->Control = DC_RECEIVE_BUFFER_SIZE;
        Rbd->Status = DC_RBD_STATUS_OWNED;

        *RcbSlot = Rcb;

        ++RcbSlot;
        ++Rbd;
        Rcb = (PDC_RCB)Rcb->ListEntry.Next;
    }
    Rbd = Rbd - 1;
    Rbd->Control |= DC_RBD_CONTROL_CHAINED;
    Rbd->Address2 = Adapter->RbdPhys;

    ASSERT(Rbd == Adapter->TailRbd);
}

CODE_SEG("PAGE")
VOID
DcInitRxRing(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_RBD Rbd;
    ULONG i;

    PAGED_CODE();

    Rbd = Adapter->HeadRbd;
    Adapter->CurrentRbd = Rbd;

    for (i = 0; i < Adapter->RcbCount; ++i)
    {
        Rbd->Control = DC_RECEIVE_BUFFER_SIZE;
        Rbd->Status = DC_RBD_STATUS_OWNED;

        ++Rbd;
    }
    Rbd = Rbd - 1;
    Rbd->Control |= DC_RBD_CONTROL_CHAINED;

    ASSERT(Rbd == Adapter->TailRbd);
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcAllocateMemory(
    _In_ PDC21X4_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    PAGED_CODE();

    Status = NdisMInitializeScatterGatherDma(Adapter->AdapterHandle,
                                             FALSE, /* 32-bit DMA */
                                             DC_TRANSMIT_BLOCK_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = DcAllocateReceiveDescriptors(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = DcAllocateTransmitBlocks(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = DcAllocateTransmitBuffers(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = DcAllocateTransmitDescriptorsAndBuffers(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    Status = DcAllocateReceiveBuffers(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
        return Status;

    NdisAllocateSpinLock(&Adapter->SendLock);
    NdisAllocateSpinLock(&Adapter->ReceiveLock);
    NdisAllocateSpinLock(&Adapter->ModeLock);

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
DcInitTestPacket(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < DC_LOOPBACK_FRAMES; ++i)
    {
        PETH_HEADER PacketBuffer = Adapter->LoopbackFrame[i];

        NdisZeroMemory(PacketBuffer, DC_LOOPBACK_FRAME_SIZE);

        /* Destination MAC address */
        NdisMoveMemory(PacketBuffer->Destination,
                       Adapter->CurrentMacAddress,
                       ETH_LENGTH_OF_ADDRESS);

        /* Source MAC address */
        NdisMoveMemory(PacketBuffer->Source,
                       Adapter->CurrentMacAddress,
                       ETH_LENGTH_OF_ADDRESS);

        ++PacketBuffer;
    }
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcInitializeAdapterResources(
    _In_ PDC21X4_ADAPTER Adapter)
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
                                       DC21X4_TAG);
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
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                if (!IoDescriptor && (Descriptor->u.Port.Length == DC_IO_LENGTH))
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
        Adapter->Flags |= DC_IRQ_SHARED;

    Adapter->IoBaseAddress = IoDescriptor->u.Port.Start;

    if ((IoDescriptor->Type == CmResourceTypePort) &&
        (IoDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        Status = NdisMRegisterIoPortRange((PVOID*)&Adapter->IoBase,
                                          Adapter->AdapterHandle,
                                          Adapter->IoBaseAddress.LowPart,
                                          DC_IO_LENGTH);
    }
    else
    {
        Status = NdisMMapIoSpace((PVOID*)&Adapter->IoBase,
                                 Adapter->AdapterHandle,
                                 Adapter->IoBaseAddress,
                                 DC_IO_LENGTH);

        Adapter->Flags |= DC_IO_MAPPED;
    }
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
DcInitializeAdapterLocation(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDEVICE_OBJECT Pdo;
    NTSTATUS Status;
    ULONG PropertyValue, Length;

    PAGED_CODE();

    NdisMGetDeviceProperty(Adapter->AdapterHandle,
                           &Pdo,
                           NULL,
                           NULL,
                           NULL,
                           NULL);

    Status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyAddress,
                                 sizeof(PropertyValue),
                                 &PropertyValue,
                                 &Length);
    if (!NT_SUCCESS(Status))
        return NDIS_STATUS_FAILURE;

    /* We need this for PCI devices only ((DeviceNumber << 16) | Function) */
    Adapter->DeviceNumber = (PropertyValue >> 16) & 0x000000FF;

    Status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyBusNumber,
                                 sizeof(PropertyValue),
                                 &Adapter->BusNumber,
                                 &Length);
    if (!NT_SUCCESS(Status))
        return NDIS_STATUS_FAILURE;

    return NDIS_STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
ULONG
DcGetBusModeParameters(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    ULONG DefaultMode, NewMode;

    PAGED_CODE();

    /* TODO: Other architectures than x86 */
    DefaultMode = DC_BUS_MODE_CACHE_ALIGNMENT_16 | DC_BUS_MODE_BURST_LENGTH_NO_LIMIT;

    if (!(Adapter->Features & DC_ENABLE_PCI_COMMANDS))
        return DefaultMode;

    INFO("PCI Cache Line Size %u\n", PciData->CacheLineSize * 4);
    INFO("PCI Command %04lx\n", PciData->Command);

    /* Use the cache line size if it was set up by firmware */
    switch (PciData->CacheLineSize)
    {
        case 8:
            NewMode = DC_BUS_MODE_CACHE_ALIGNMENT_8 | DC_BUS_MODE_BURST_LENGTH_8;
            break;
        case 16:
            NewMode = DC_BUS_MODE_CACHE_ALIGNMENT_16 | DC_BUS_MODE_BURST_LENGTH_16;
            break;
        case 32:
            NewMode = DC_BUS_MODE_CACHE_ALIGNMENT_32 | DC_BUS_MODE_BURST_LENGTH_32;
            break;

        default:
            return DefaultMode;
    }

    /* Enable one of those commands */
    if (PciData->Command & PCI_ENABLE_WRITE_AND_INVALIDATE)
    {
        NewMode |= DC_BUS_MODE_WRITE_INVALIDATE;
    }
    else
    {
        NewMode |= DC_BUS_MODE_READ_LINE;
    }

    return NewMode;
}

static
CODE_SEG("PAGE")
NDIS_STATUS
DcRecognizeHardware(
    _In_ PDC21X4_ADAPTER Adapter)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_CONFIG, CacheLineSize)];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)Buffer; // Partial PCI header
    PNDIS_TIMER_FUNCTION MediaMonitorRoutine;
    ULONG Bytes;

    PAGED_CODE();

    Bytes = NdisReadPciSlotInformation(Adapter->AdapterHandle,
                                       0,
                                       FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                                       Buffer,
                                       sizeof(Buffer));
    if (Bytes != sizeof(Buffer))
        return NDIS_STATUS_FAILURE;

    Adapter->DeviceId = PciConfig->DeviceID;
    Adapter->RevisionId = PciConfig->RevisionID;

    switch ((PciConfig->DeviceID << 16) | PciConfig->VendorID)
    {
        case DC_DEV_DECCHIP_21040:
        {
            Adapter->ChipType = DC21040;
            Adapter->InterruptMask = DC_GENERIC_IRQ_MASK | DC_IRQ_LINK_FAIL;
            Adapter->LinkStateChangeMask = DC_IRQ_LINK_FAIL;
            Adapter->HandleLinkStateChange = MediaLinkStateChange21040;
            MediaMonitorRoutine = MediaMonitor21040Dpc;
            break;
        }

        case DC_DEV_DECCHIP_21041:
        {
            Adapter->ChipType = DC21041;
            Adapter->Features |= DC_HAS_POWER_SAVING | DC_HAS_TIMER;
            Adapter->InterruptMask = DC_GENERIC_IRQ_MASK | DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;
            Adapter->LinkStateChangeMask = DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;
            Adapter->HandleLinkStateChange = MediaLinkStateChange21041;
            MediaMonitorRoutine = MediaMonitor21041Dpc;
            break;
        }

        case DC_DEV_DECCHIP_21140:
        {
            Adapter->ChipType = DC21140;
            Adapter->Features |= DC_HAS_TIMER;

            if ((PciConfig->RevisionID & 0xF0) < 0x20)
            {
                /* 21140 */
                Adapter->Features |= DC_PERFECT_FILTERING_ONLY;
            }
            else
            {
                /* 21140A */
                Adapter->Features |= DC_NEED_RX_OVERFLOW_WORKAROUND | DC_ENABLE_PCI_COMMANDS |
                                     DC_HAS_POWER_SAVING;
            }

            Adapter->OpMode |= DC_OPMODE_PORT_ALWAYS;
            Adapter->InterruptMask = DC_GENERIC_IRQ_MASK;
            MediaMonitorRoutine = MediaMonitor21140Dpc;
            break;
        }

        case DC_DEV_INTEL_21143:
        {
            Adapter->Features |= DC_NEED_RX_OVERFLOW_WORKAROUND | DC_SIA_GPIO |
                                 DC_HAS_POWER_SAVING | DC_MII_AUTOSENSE | DC_HAS_TIMER;

            Adapter->InterruptMask = DC_GENERIC_IRQ_MASK | DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;
            Adapter->LinkStateChangeMask = DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;

            Adapter->ChipType = DC21143;

            if ((PciConfig->RevisionID & 0xF0) < 0x20)
            {
                /* 21142 */
            }
            else
            {
                /* 21143 */
                Adapter->Features |= DC_ENABLE_PCI_COMMANDS;
                Adapter->OpMode |= DC_OPMODE_PORT_ALWAYS;
                Adapter->InterruptMask |= DC_IRQ_LINK_CHANGED;
                Adapter->LinkStateChangeMask |= DC_IRQ_LINK_CHANGED;
            }

            /* 21143 -PD and -TD */
            if ((PciConfig->RevisionID & 0xF0) > 0x30)
                Adapter->Features |= DC_HAS_POWER_MANAGEMENT;

            Adapter->HandleLinkStateChange = MediaLinkStateChange21143;
            MediaMonitorRoutine = MediaMonitor21143Dpc;
            break;
        }

        case DC_DEV_INTEL_21145:
        {
            Adapter->ChipType = DC21145;

            Adapter->Features |= DC_NEED_RX_OVERFLOW_WORKAROUND | DC_SIA_GPIO |
                                 DC_HAS_POWER_MANAGEMENT | DC_HAS_POWER_SAVING |
                                 DC_SIA_ANALOG_CONTROL | DC_ENABLE_PCI_COMMANDS |
                                 DC_MII_AUTOSENSE | DC_HAS_TIMER;

            Adapter->OpMode |= DC_OPMODE_PORT_ALWAYS;
            Adapter->InterruptMask = DC_GENERIC_IRQ_MASK | DC_IRQ_LINK_CHANGED |
                                     DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;
            Adapter->LinkStateChangeMask = DC_IRQ_LINK_CHANGED |
                                           DC_IRQ_LINK_PASS | DC_IRQ_LINK_FAIL;
            Adapter->HandleLinkStateChange = MediaLinkStateChange21143;
            MediaMonitorRoutine = MediaMonitor21143Dpc;

            Adapter->AnalogControl = DC_HPNA_ANALOG_CTRL;

            /* Workaround for internal RX errata */
            Adapter->HpnaRegister[HPNA_NOISE_FLOOR] = 0x10;
            Adapter->HpnaInitBitmap = (1 << HPNA_NOISE_FLOOR);
            break;
        }

        default:
            return NDIS_STATUS_NOT_RECOGNIZED;
    }

    Adapter->BusMode = DcGetBusModeParameters(Adapter, PciConfig);

    INFO("Bus Mode %08lx\n", Adapter->BusMode);

    /* Errata: hash filtering is broken on some chips */
    if (Adapter->Features & DC_PERFECT_FILTERING_ONLY)
        Adapter->MulticastMaxEntries = DC_SETUP_FRAME_ADDRESSES;
    else
        Adapter->MulticastMaxEntries = DC_MULTICAST_LIST_SIZE;

    NdisMInitializeTimer(&Adapter->MediaMonitorTimer,
                         Adapter->AdapterHandle,
                         MediaMonitorRoutine,
                         Adapter);

    return NDIS_STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
DcFreeAdapter(
    _In_ __drv_freesMem(Mem) PDC21X4_ADAPTER Adapter)
{
    ULONG i;

    PAGED_CODE();

    DcFreeEeprom(Adapter);

    if (Adapter->Interrupt.InterruptObject)
    {
        NdisMDeregisterInterrupt(&Adapter->Interrupt);
    }

    if (Adapter->IoBase)
    {
        if (Adapter->Flags & DC_IO_MAPPED)
        {
            NdisMUnmapIoSpace(Adapter->AdapterHandle,
                              Adapter->IoBase,
                              DC_IO_LENGTH);
        }
        else
        {
            NdisMDeregisterIoPortRange(Adapter->AdapterHandle,
                                       Adapter->IoBaseAddress.LowPart,
                                       DC_IO_LENGTH,
                                       Adapter->IoBase);
        }
    }

    if (Adapter->HeadTcb)
    {
        NdisFreeMemory(Adapter->HeadTcb, sizeof(DC_TCB) * DC_TRANSMIT_BLOCKS, 0);
    }
    if (Adapter->RcbArray)
    {
        NdisFreeMemory(Adapter->RcbArray, sizeof(PVOID) * Adapter->RcbCount, 0);
    }
    if (Adapter->SetupFrameSaved)
    {
        NdisFreeMemory(Adapter->SetupFrameSaved, DC_SETUP_FRAME_SIZE, 0);
    }

    while (Adapter->AllocRcbList.Next)
    {
        PSINGLE_LIST_ENTRY Entry = PopEntryList(&Adapter->AllocRcbList);
        PDC_RCB Rcb = CONTAINING_RECORD(Entry, DC_RCB, AllocListEntry);

        DcFreeRcb(Adapter, Rcb);
    }

    if (Adapter->RbdOriginal)
    {
        NdisMFreeSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_RBD,
                              FALSE, /* Non-cached */
                              Adapter->RbdOriginal,
                              Adapter->RbdPhysOriginal);
    }
    if (Adapter->TbdOriginal)
    {
        NdisMFreeSharedMemory(Adapter->AdapterHandle,
                              DC_MEM_BLOCK_SIZE_TBD_AUX,
                              FALSE, /* Non-cached */
                              Adapter->TbdOriginal,
                              Adapter->TbdPhysOriginal);
    }
    if (Adapter->CoalesceBuffer)
    {
        for (i = 0; i < DC_TRANSMIT_BUFFERS; ++i)
        {
            PDC_TX_BUFFER_DATA SendBufferData = &Adapter->SendBufferData[i];

            if (!SendBufferData->VirtualAddress)
                continue;

            NdisMFreeSharedMemory(Adapter->AdapterHandle,
                                  DC_MEM_BLOCK_SIZE_TX_BUFFER,
                                  FALSE, /* Non-cached */
                                  SendBufferData->VirtualAddress,
                                  SendBufferData->PhysicalAddress);
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
    if (Adapter->ModeLock.SpinLock)
        NdisFreeSpinLock(&Adapter->ModeLock);

    NdisFreeMemory(Adapter->AdapterOriginal, sizeof(*Adapter), 0);
}

CODE_SEG("PAGE")
NDIS_STATUS
NTAPI
DcInitialize(
    _Out_ PNDIS_STATUS OpenErrorStatus,
    _Out_ PUINT SelectedMediumIndex,
    _In_ PNDIS_MEDIUM MediumArray,
    _In_ UINT MediumArraySize,
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE WrapperConfigurationContext)
{
    PDC21X4_ADAPTER Adapter;
    PVOID UnalignedAdapter;
    NDIS_STATUS Status;
    ULONG Alignment, AdapterSize, OpMode;
    BOOLEAN Success;
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

    Alignment = NdisGetSharedDataAlignment();
    AdapterSize = sizeof(*Adapter) + Alignment;

    Status = NdisAllocateMemoryWithTag((PVOID*)&UnalignedAdapter, AdapterSize, DC21X4_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Failed to allocate adapter context\n");
        return NDIS_STATUS_RESOURCES;
    }
    NdisZeroMemory(UnalignedAdapter, AdapterSize);

    Adapter = ALIGN_UP_POINTER_BY(UnalignedAdapter, Alignment);
    Adapter->AdapterOriginal = UnalignedAdapter;
    Adapter->AdapterSize = AdapterSize;
    Adapter->AdapterHandle = MiniportAdapterHandle;
    Adapter->WrapperConfigurationHandle = WrapperConfigurationContext;

    NdisInitializeWorkItem(&Adapter->ResetWorkItem, DcResetWorker, Adapter);
    NdisInitializeWorkItem(&Adapter->PowerWorkItem, DcPowerWorker, Adapter);
    NdisInitializeWorkItem(&Adapter->TxRecoveryWorkItem, DcTransmitTimeoutRecoveryWorker, Adapter);

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         Adapter,
                         2, /* CheckForHangTimeInSeconds */
                         NDIS_ATTRIBUTE_BUS_MASTER |
                         NDIS_ATTRIBUTE_DESERIALIZE |
                         NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS,
                         NdisInterfacePci);

    Status = DcRecognizeHardware(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }

    Status = DcInitializeAdapterResources(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Failure;
    }

    Status = DcInitializeAdapterLocation(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Failure;
    }

    /* Bring the chip out of sleep mode */
    DcPowerSave(Adapter, FALSE);

    OpMode = DC_READ(Adapter, DcCsr6_OpMode);
    OpMode &= ~(DC_OPMODE_RX_ENABLE | DC_OPMODE_TX_ENABLE);
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    MediaInitMediaList(Adapter);

    Status = DcReadEeprom(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Failure;
    }

    NdisMoveMemory(Adapter->CurrentMacAddress, Adapter->PermanentMacAddress, ETH_LENGTH_OF_ADDRESS);

    Status = DcReadConfiguration(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Failure;
    }

    Status = DcAllocateMemory(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Failure;
    }

    DcInitTestPacket(Adapter);

    DcCreateRxRing(Adapter);

    /* Execute the reset sequence */
    if (Adapter->ResetStreamLength)
    {
        for (i = 0; i < Adapter->ResetStreamLength; ++i)
        {
            DcWriteGpio(Adapter, Adapter->ResetStream[i]);
            NdisMSleep(100);
        }

        /* Give the PHY some time to reset */
        NdisMSleep(5000);
    }

    /* Perform a software reset */
    DC_WRITE(Adapter, DcCsr0_BusMode, DC_BUS_MODE_SOFT_RESET);
    NdisMSleep(100);
    DC_WRITE(Adapter, DcCsr0_BusMode, Adapter->BusMode);

    /* Try to detect a MII PHY */
    if (Adapter->Features & DC_HAS_MII)
    {
        MediaSelectMiiPort(Adapter, TRUE);

        Success = DcFindMiiPhy(Adapter);
        if (Success)
        {
            /* Disable all link interrupts when the MII PHY media is found */
            Adapter->InterruptMask &= ~(DC_IRQ_LINK_CHANGED | DC_IRQ_LINK_FAIL | DC_IRQ_LINK_PASS);
            Adapter->LinkStateChangeMask = 0;
        }
        else
        {
            /* Incorrect EEPROM table or PHY is not connected, switch to a serial transceiver */
            WARN("No PHY devices found\n");
            Adapter->Features &= ~DC_HAS_MII;
        }
    }

    MediaInitDefaultMedia(Adapter, Adapter->DefaultMedia);

    /* Set the MAC address */
    DcSetupFrameInitialize(Adapter);

    /* Clear statistics */
    DC_READ(Adapter, DcCsr8_RxCounters);

    Adapter->Flags |= DC_FIRST_SETUP;

    Status = DcSetupAdapter(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Failed to initialize the NIC\n");
        goto Disable;
    }

    Adapter->Flags &= ~DC_FIRST_SETUP;

    Status = NdisMRegisterInterrupt(&Adapter->Interrupt,
                                    Adapter->AdapterHandle,
                                    Adapter->InterruptVector,
                                    Adapter->InterruptLevel,
                                    TRUE, /* Request ISR calls */
                                    !!(Adapter->Flags & DC_IRQ_SHARED),
                                    (Adapter->InterruptFlags & CM_RESOURCE_INTERRUPT_LATCHED) ?
                                    NdisInterruptLatched : NdisInterruptLevelSensitive);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Unable to register interrupt\n");
        goto Disable;
    }

    DcStartAdapter(Adapter);

    return NDIS_STATUS_SUCCESS;

Disable:
    DcDisableHw(Adapter);
Failure:
    ERR("Initialization failed with status %08lx\n", Status);

    DcFreeAdapter(Adapter);

    return Status;
}
