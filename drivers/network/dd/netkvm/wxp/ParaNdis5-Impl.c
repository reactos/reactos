/*
 * This file contains NDIS5.X Implementation of adapter driver procedures.
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "ParaNdis5.h"


#ifdef WPP_EVENT_TRACING
#include "ParaNdis5-Impl.tmh"
#endif


/**********************************************************
Per-packet information holder
***********************************************************/
#define SEND_ENTRY_FLAG_READY       0x0001
#define SEND_ENTRY_TSO_USED         0x0002
#define SEND_ENTRY_NO_INDIRECT      0x0004
#define SEND_ENTRY_TCP_CS           0x0008
#define SEND_ENTRY_UDP_CS           0x0010
#define SEND_ENTRY_IP_CS            0x0020



typedef struct _tagSendEntry
{
    LIST_ENTRY      list;
    PNDIS_PACKET    packet;
    ULONG           flags;
    ULONG           ipTransferUnit;
    union
    {
        ULONG PriorityDataLong;
        UCHAR PriorityData[4];
    };
} tSendEntry;

/**********************************************************
This defines field in NDIS_PACKET structure to use as holder
of our reference pointer for indicated packets
***********************************************************/
#define IDXTOUSE    0
#define REF_MINIPORT(Packet) ((PVOID *)(Packet->MiniportReservedEx + IDXTOUSE * sizeof(PVOID)))


/**********************************************************
Memory allocation procedure
Parameters:
    context(not used)
    ULONG ulRequiredSize    size of block to allocate
Return value:
    PVOID                   pointer to block or NULL if failed
***********************************************************/
PVOID ParaNdis_AllocateMemory(PARANDIS_ADAPTER *pContext, ULONG ulRequiredSize)
{
    PVOID p;
    UNREFERENCED_PARAMETER(pContext);
    if (NDIS_STATUS_SUCCESS != NdisAllocateMemoryWithTag(&p, ulRequiredSize, PARANDIS_MEMORY_TAG))
        p = NULL;
    if (!p)
    {
        DPrintf(0, ("[%s] failed (%d bytes)", __FUNCTION__, ulRequiredSize));
    }
    return p;
}

/**********************************************************
Implementation of "open adapter configuration" operation
Parameters:
    context
Return value:
    NDIS_HANDLE     Handle to open configuration or NULL, if failed
***********************************************************/
NDIS_HANDLE ParaNdis_OpenNICConfiguration(PARANDIS_ADAPTER *pContext)
{
    NDIS_STATUS status;
    NDIS_HANDLE cfg;
    DEBUG_ENTRY(2);
    NdisOpenConfiguration(&status, &cfg, pContext->WrapperConfigurationHandle);
    if (status != NDIS_STATUS_SUCCESS)
        cfg = NULL;
    DEBUG_EXIT_STATUS(0, status);
    return cfg;
}

void ParaNdis_RestoreDeviceConfigurationAfterReset(
    PARANDIS_ADAPTER *pContext)
{

}


/**********************************************************
Indicates connect/disconnect events
Parameters:
    context
    BOOLEAN bConnected  1/0 connect/disconnect
***********************************************************/
VOID ParaNdis_IndicateConnect(PARANDIS_ADAPTER *pContext, BOOLEAN bConnected, BOOLEAN bForce)
{
    // indicate disconnect always
    if (bConnected != pContext->bConnected || bForce)
    {
        pContext->bConnected = bConnected;
        DPrintf(0, ("Indicating %sconnect", bConnected ? "" : "dis"));
        ParaNdis_DebugHistory(pContext, hopConnectIndication, NULL, bConnected, 0, 0);
        NdisMIndicateStatus(
            pContext->MiniportHandle,
            bConnected ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
            0,
            0);
        NdisMIndicateStatusComplete(pContext->MiniportHandle);
    }
}

VOID ParaNdis_SetPowerState(PARANDIS_ADAPTER *pContext, NDIS_DEVICE_POWER_STATE newState)
{
    //NDIS_DEVICE_POWER_STATE prev = pContext->powerState;
    pContext->powerState = newState;
}


/**********************************************************
Callback of timer for connect indication, if used
Parameters:
    context (on FunctionContext)
    all the rest are irrelevant
***********************************************************/
static VOID NTAPI OnConnectTimer(
    IN PVOID  SystemSpecific1,
    IN PVOID  FunctionContext,
    IN PVOID  SystemSpecific2,
    IN PVOID  SystemSpecific3
    )
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)FunctionContext;
    ParaNdis_ReportLinkStatus(pContext, FALSE);
}

/**********************************************************
NDIS5 implementation of shared memory allocation
Parameters:
    context
    tCompletePhysicalAddress *pAddresses
            the structure accumulates all our knowledge
            about the allocation (size, addresses, cacheability etc)
Return value:
    TRUE if the allocation was successful
***********************************************************/
BOOLEAN ParaNdis_InitialAllocatePhysicalMemory(
    PARANDIS_ADAPTER *pContext,
    tCompletePhysicalAddress *pAddresses)
{
    NdisMAllocateSharedMemory(
        pContext->MiniportHandle,
        pAddresses->size,
        (BOOLEAN)pAddresses->IsCached,
        &pAddresses->Virtual,
        &pAddresses->Physical);
    return pAddresses->Virtual != NULL;
}

/**********************************************************
Callback of timer for pending events cleanup after regular DPC processing
Parameters:
    context (on FunctionContext)
    all the rest are irrelevant
***********************************************************/
static VOID NTAPI OnDPCPostProcessTimer(
    IN PVOID  SystemSpecific1,
    IN PVOID  FunctionContext,
    IN PVOID  SystemSpecific2,
    IN PVOID  SystemSpecific3
    )
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)FunctionContext;
    ULONG requiresProcessing;
    requiresProcessing = ParaNdis_DPCWorkBody(pContext, PARANDIS_UNLIMITED_PACKETS_TO_INDICATE);
    if (requiresProcessing)
    {
        // we need to request additional DPC
        InterlockedOr(&pContext->InterruptStatus, requiresProcessing);
        NdisSetTimer(&pContext->DPCPostProcessTimer, 10);
    }
}

/**********************************************************
NDIS5 implementation of shared memory freeing
Parameters:
    context
    tCompletePhysicalAddress *pAddresses
            the structure accumulates all our knowledge
            about the allocation (size, addresses, cacheability etc)
            filled by ParaNdis_InitialAllocatePhysicalMemory
***********************************************************/
VOID ParaNdis_FreePhysicalMemory(
    PARANDIS_ADAPTER *pContext,
    tCompletePhysicalAddress *pAddresses)
{

    NdisMFreeSharedMemory(
        pContext->MiniportHandle,
        pAddresses->size,
        (BOOLEAN)pAddresses->IsCached,
        pAddresses->Virtual,
        pAddresses->Physical);
}

static void DebugParseOffloadBits()
{
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO info;
    tChecksumCheckResult res;
    ULONG val = 1;
    int level = 1;
    while (val)
    {
        info.Value = val;
        if (info.Receive.NdisPacketIpChecksumFailed) DPrintf(level, ("W.%X=IPCS failed", val));
        if (info.Receive.NdisPacketIpChecksumSucceeded) DPrintf(level, ("W.%X=IPCS OK", val));
        if (info.Receive.NdisPacketTcpChecksumFailed) DPrintf(level, ("W.%X=TCPCS failed", val));
        if (info.Receive.NdisPacketTcpChecksumSucceeded) DPrintf(level, ("W.%X=TCPCS OK", val));
        if (info.Receive.NdisPacketUdpChecksumFailed) DPrintf(level, ("W.%X=UDPCS failed", val));
        if (info.Receive.NdisPacketUdpChecksumSucceeded) DPrintf(level, ("W.%X=UDPCS OK", val));
        val = val << 1;
    }
    val = 1;
    while (val)
    {
        res.value = val;
        if (res.flags.IpFailed) DPrintf(level, ("C.%X=IPCS failed", val));
        if (res.flags.IpOK) DPrintf(level, ("C.%X=IPCS OK", val));
        if (res.flags.TcpFailed) DPrintf(level, ("C.%X=TCPCS failed", val));
        if (res.flags.TcpOK) DPrintf(level, ("C.%X=TCPCS OK", val));
        if (res.flags.UdpFailed) DPrintf(level, ("C.%X=UDPCS failed", val));
        if (res.flags.UdpOK) DPrintf(level, ("C.%X=UDPCS OK", val));
        val = val << 1;
    }
}

/**********************************************************
Procedure for NDIS5 specific initialization:
    register interrupt handler
    allocate pool of packets to indicate
    allocate pool of buffers to indicate
    initialize halt event
Parameters:
    context
Return value:
    SUCCESS or failure code
***********************************************************/
NDIS_STATUS NTAPI ParaNdis_FinishSpecificInitialization(
    PARANDIS_ADAPTER *pContext)
{
    NDIS_STATUS     status;
    UINT            nPackets = pContext->NetMaxReceiveBuffers * 2;
    DEBUG_ENTRY(2);
    NdisInitializeEvent(&pContext->HaltEvent);
    InitializeListHead(&pContext->SendQueue);
    InitializeListHead(&pContext->TxWaitingList);
    NdisInitializeTimer(&pContext->ConnectTimer, OnConnectTimer, pContext);
    NdisInitializeTimer(&pContext->DPCPostProcessTimer, OnDPCPostProcessTimer, pContext);

    status = NdisMRegisterInterrupt(
        &pContext->Interrupt,
        pContext->MiniportHandle,
        pContext->AdapterResources.Vector,
        pContext->AdapterResources.Level,
        TRUE,
        TRUE,
        NdisInterruptLevelSensitive);

    if (status == NDIS_STATUS_SUCCESS)
    {
        NdisAllocatePacketPool(
            &status,
            &pContext->PacketPool,
            nPackets,
            PROTOCOL_RESERVED_SIZE_IN_PACKET );
    }
    if (status == NDIS_STATUS_SUCCESS)
    {
        NdisAllocateBufferPool(
            &status,
            &pContext->BuffersPool,
            nPackets);
    }

#if !DO_MAP_REGISTERS
    if (status == NDIS_STATUS_SUCCESS)
    {
        status = NdisMInitializeScatterGatherDma(
            pContext->MiniportHandle,
            TRUE,
            0x10000);
        pContext->bDmaInitialized = status == NDIS_STATUS_SUCCESS;
    }
#else
    if (status == NDIS_STATUS_SUCCESS)
    {
        status = NdisMAllocateMapRegisters(
            pContext->MiniportHandle,
            0,
            NDIS_DMA_32BITS,
            64,
            PAGE_SIZE);
        pContext->bDmaInitialized = status == NDIS_STATUS_SUCCESS;
    }
#endif
    if (status == NDIS_STATUS_SUCCESS)
    {
        DebugParseOffloadBits();
    }
    DEBUG_EXIT_STATUS(status ? 0 : 2, status);
    return status;
}

/**********************************************************
Procedure of NDIS5-specific cleanup:
    deregister interrupt
    free buffer and packet pool
Parameters:
    context
***********************************************************/
VOID ParaNdis_FinalizeCleanup(PARANDIS_ADAPTER *pContext)
{
    if (pContext->Interrupt.InterruptObject)
    {
        NdisMDeregisterInterrupt(&pContext->Interrupt);
    }
    if (pContext->BuffersPool)
    {
        NdisFreeBufferPool(pContext->BuffersPool);
    }
    if (pContext->PacketPool)
    {
        NdisFreePacketPool(pContext->PacketPool);
    }
#if DO_MAP_REGISTERS
    if (pContext->bDmaInitialized)
    {
        NdisMFreeMapRegisters(pContext->MiniportHandle);
    }
#endif
}


static FORCEINLINE ULONG MaxNdisBufferDataSize(PARANDIS_ADAPTER *pContext, pIONetDescriptor pBufferDesc)
{
    ULONG size  = pBufferDesc->DataInfo.size;
    if (pContext->bUseMergedBuffers) size -= pContext->nVirtioHeaderSize;
    return size;
}


/**********************************************************
NDIS5-specific procedure for binding RX buffer to
NDIS_PACKET and NDIS_BUFFER
Parameters:
    context
    pIONetDescriptor pBuffersDesc   VirtIO buffer descriptor

Return value:
    TRUE, if bound successfully
    FALSE, if no buffer or packet can be allocated
***********************************************************/
BOOLEAN ParaNdis_BindBufferToPacket(
    PARANDIS_ADAPTER *pContext,
    pIONetDescriptor pBufferDesc)
{
    NDIS_STATUS     status;
    PNDIS_BUFFER    pBuffer = NULL;
    PNDIS_PACKET    Packet = NULL;
    NdisAllocatePacket(&status, &Packet, pContext->PacketPool);
    if (status == NDIS_STATUS_SUCCESS)
    {
        NdisReinitializePacket(Packet);
        NdisAllocateBuffer(
            &status,
            &pBuffer,
            pContext->BuffersPool,
            RtlOffsetToPointer(pBufferDesc->DataInfo.Virtual, pContext->bUseMergedBuffers ? pContext->nVirtioHeaderSize : 0),
            MaxNdisBufferDataSize(pContext, pBufferDesc));
    }
    if (status == NDIS_STATUS_SUCCESS)
    {
        PNDIS_PACKET_OOB_DATA pOOB = NDIS_OOB_DATA_FROM_PACKET(Packet);
        NdisZeroMemory(pOOB, sizeof(NDIS_PACKET_OOB_DATA));
        NDIS_SET_PACKET_HEADER_SIZE(Packet, ETH_HEADER_SIZE);
        NdisChainBufferAtFront(Packet, pBuffer);
        pBufferDesc->pHolder = Packet;
    }
    else
    {
        if (pBuffer) NdisFreeBuffer(pBuffer);
        if (Packet)  NdisFreePacket(Packet);
    }
    return status == NDIS_STATUS_SUCCESS;
}


/**********************************************************
NDIS5-specific procedure for unbinding
previously bound RX buffer from it's NDIS_PACKET and NDIS_BUFFER
Parameters:
    context
    pIONetDescriptor pBuffersDesc   VirtIO buffer descriptor
***********************************************************/
void ParaNdis_UnbindBufferFromPacket(
    PARANDIS_ADAPTER *pContext,
    pIONetDescriptor pBufferDesc)
{
    if (pBufferDesc->pHolder)
    {
        PNDIS_BUFFER    pBuffer = NULL;
        PNDIS_PACKET    Packet = pBufferDesc->pHolder;
        pBufferDesc->pHolder = NULL;
        NdisUnchainBufferAtFront(Packet, &pBuffer);
        if (pBuffer)
        {
            NdisAdjustBufferLength(pBuffer, MaxNdisBufferDataSize(pContext, pBufferDesc));
            NdisFreeBuffer(pBuffer);
        }
        NdisFreePacket(Packet);
    }
}

/**********************************************************
NDIS5-specific procedure to indicate received packets

Parameters:
    context
    pIONetDescriptor pBuffersDescriptor - VirtIO buffer descriptor of data buffer
    PVOID dataBuffer  - data buffer to pass to network stack
    PULONG pLength - size of received packet.
    BOOLEAN bPrepareOnly - only return NBL for further indication in batch
Return value:
    TRUE  is packet indicated
    FALSE if not (in this case, the descriptor should be freed now)
If priority header is in the packet. it will be removed and *pLength decreased
***********************************************************/
tPacketIndicationType ParaNdis_IndicateReceivedPacket(
    PARANDIS_ADAPTER *pContext,
    PVOID dataBuffer,
    PULONG pLength,
    BOOLEAN bPrepareOnly,
    pIONetDescriptor pBuffersDesc)
{
    PNDIS_BUFFER    pBuffer = NULL;
    PNDIS_BUFFER    pNoBuffer = NULL;
    PNDIS_PACKET    Packet = pBuffersDesc->pHolder;
    ULONG length = *pLength;
    if (Packet) NdisUnchainBufferAtFront(Packet, &pBuffer);
    if (Packet) NdisUnchainBufferAtFront(Packet, &pNoBuffer);
    if (pBuffer)
    {
        UINT uTotalLength;
        NDIS_PACKET_8021Q_INFO qInfo;
        qInfo.Value = NULL;
        if ((pContext->ulPriorityVlanSetting && length > (ETH_PRIORITY_HEADER_OFFSET + ETH_PRIORITY_HEADER_SIZE)) ||
            length > pContext->MaxPacketSize.nMaxFullSizeOS)
        {
            PUCHAR pPriority = (PUCHAR)dataBuffer + ETH_PRIORITY_HEADER_OFFSET;
            if (ETH_HAS_PRIO_HEADER(dataBuffer))
            {
                if (IsPrioritySupported(pContext))
                    qInfo.TagHeader.UserPriority = (pPriority[2] & 0xE0) >> 5;
                if (IsVlanSupported(pContext))
                {
                    qInfo.TagHeader.VlanId = (((USHORT)(pPriority[2] & 0x0F)) << 8) | pPriority[3];
                    if (pContext->VlanId && pContext->VlanId != qInfo.TagHeader.VlanId)
                    {
                        DPrintf(0, ("[%s] Failing unexpected VlanID %d", __FUNCTION__, qInfo.TagHeader.VlanId));
                        pContext->extraStatistics.framesFilteredOut++;
                        pBuffer = NULL;
                    }
                }
                RtlMoveMemory(
                    pPriority,
                    pPriority + ETH_PRIORITY_HEADER_SIZE,
                    length - ETH_PRIORITY_HEADER_OFFSET - ETH_PRIORITY_HEADER_SIZE);
                length -= ETH_PRIORITY_HEADER_SIZE;
                if (length > pContext->MaxPacketSize.nMaxFullSizeOS)
                {
                    DPrintf(0, ("[%s] Can not indicate up packet of %d", __FUNCTION__, length));
                    pBuffer = NULL;
                }
                DPrintf(1, ("[%s] Found priority data %p", __FUNCTION__, qInfo.Value));
                pContext->extraStatistics.framesRxPriority++;
            }
        }

        if (pBuffer)
        {
            PVOID headerBuffer = pContext->bUseMergedBuffers ? pBuffersDesc->DataInfo.Virtual:pBuffersDesc->HeaderInfo.Virtual;
            virtio_net_hdr_basic *pHeader = (virtio_net_hdr_basic *)headerBuffer;
            tChecksumCheckResult csRes;
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) = qInfo.Value;
            NDIS_SET_PACKET_STATUS(Packet, STATUS_SUCCESS);
            ParaNdis_PadPacketReceived(dataBuffer, &length);
            NdisAdjustBufferLength(pBuffer, length);
            NdisChainBufferAtFront(Packet, pBuffer);
            NdisQueryPacket(Packet, NULL, NULL, NULL, &uTotalLength);
            *REF_MINIPORT(Packet) = pBuffersDesc;
            csRes = ParaNdis_CheckRxChecksum(pContext, pHeader->flags, dataBuffer, length);
            if (csRes.value)
            {
                NDIS_TCP_IP_CHECKSUM_PACKET_INFO qCSInfo;
                qCSInfo.Value = 0;
                qCSInfo.Receive.NdisPacketIpChecksumFailed = csRes.flags.IpFailed;
                qCSInfo.Receive.NdisPacketIpChecksumSucceeded = csRes.flags.IpOK;
                qCSInfo.Receive.NdisPacketTcpChecksumFailed = csRes.flags.TcpFailed;
                qCSInfo.Receive.NdisPacketTcpChecksumSucceeded = csRes.flags.TcpOK;
                qCSInfo.Receive.NdisPacketUdpChecksumFailed = csRes.flags.UdpFailed;
                qCSInfo.Receive.NdisPacketUdpChecksumSucceeded = csRes.flags.UdpOK;
                NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpIpChecksumPacketInfo) = (PVOID) (ULONG_PTR) qCSInfo.Value;
                DPrintf(1, ("Reporting CS %X->%X", csRes.value, qCSInfo.Value));
            }

            DPrintf(4, ("[%s] buffer %p(%d b.)", __FUNCTION__, pBuffersDesc, length));
            if (!bPrepareOnly)
            {
                NdisMIndicateReceivePacket(
                    pContext->MiniportHandle,
                    &Packet,
                    1);
            }
        }
        *pLength = length;
    }
    if (!pBuffer)
    {
        DPrintf(0, ("[%s] Error: %p(%d b.) with packet %p", __FUNCTION__,
            pBuffersDesc, length, Packet));
        Packet = NULL;
    }
    if (pNoBuffer)
    {
        DPrintf(0, ("[%s] Error: %p(%d b.) with packet %p, buf %p,%p", __FUNCTION__,
            pBuffersDesc, length, Packet, pBuffer, pNoBuffer));
    }
    return Packet;
}

VOID ParaNdis_IndicateReceivedBatch(
    PARANDIS_ADAPTER *pContext,
    tPacketIndicationType *pBatch,
    ULONG nofPackets)
{
    NdisMIndicateReceivePacket(
        pContext->MiniportHandle,
        pBatch,
        nofPackets);
}

static FORCEINLINE void GET_NUMBER_OF_SG_ELEMENTS(PNDIS_PACKET Packet, UINT *pNum)
{
    PSCATTER_GATHER_LIST pSGList;
    pSGList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);
    if (pSGList)
    {
        *pNum = pSGList->NumberOfElements;
    }
}

/**********************************************************
Complete TX packets to NDIS with status, indicated inside packet
Parameters:
    context
    PNDIS_PACKET Packet     packet to complete
***********************************************************/
static void CompletePacket(PARANDIS_ADAPTER *pContext, PNDIS_PACKET Packet)
{
    LONG lRestToReturn;
    NDIS_STATUS status = NDIS_GET_PACKET_STATUS(Packet);
    lRestToReturn = NdisInterlockedDecrement(&pContext->NetTxPacketsToReturn);
    ParaNdis_DebugHistory(pContext, hopSendComplete, Packet, 0, lRestToReturn, status);
    NdisMSendComplete(pContext->MiniportHandle, Packet, status);
}

/**********************************************************
Copy data from specified packet to VirtIO buffer, minimum 60 bytes
Parameters:
    PNDIS_PACKET Packet     packet to copy data from
    PVOID dest              destination to copy
    ULONG maxSize           maximal size of destination
Return value:
    size = number of bytes copied
    if 0, the packet is not transmitted and should be dropped
    ( should never happen)
    request
***********************************************************/
tCopyPacketResult ParaNdis_PacketCopier(
    PNDIS_PACKET Packet, PVOID dest, ULONG maxSize, PVOID refValue, BOOLEAN bPreview)
{
    PNDIS_BUFFER pBuffer;
    ULONG PriorityDataLong = ((tSendEntry *)refValue)->PriorityDataLong;
    tCopyPacketResult result;
    /* the copier called also for getting Ethernet header
       for statistics, when the transfer uses SG table */
    UINT uLength = 0;
    ULONG nCopied  = 0;
    ULONG ulToCopy = 0;
    if (bPreview) PriorityDataLong = 0;
    NdisQueryPacket(Packet,
                    NULL,
                    NULL,
                    &pBuffer,
                    (PUINT)&ulToCopy);

    if (ulToCopy > maxSize) ulToCopy = bPreview ? maxSize : 0;
    while (pBuffer && ulToCopy)
    {
        PVOID VirtualAddress = NULL;
        NdisQueryBufferSafe(pBuffer,
                            &VirtualAddress,
                            &uLength,
                            NormalPagePriority);
        if (!VirtualAddress)
        {
            /* the packet copy failed */
            nCopied = 0;
            break;
        }
        if(uLength)
        {
            // Copy the data.
            if (uLength > ulToCopy) uLength = ulToCopy;
            ulToCopy -= uLength;
            if ((PriorityDataLong & 0xFFFF) &&
                nCopied < ETH_PRIORITY_HEADER_OFFSET &&
                (nCopied + uLength) >= ETH_PRIORITY_HEADER_OFFSET)
            {
                ULONG ulCopyNow = ETH_PRIORITY_HEADER_OFFSET - nCopied;
                NdisMoveMemory(dest, VirtualAddress, ulCopyNow);
                dest = (PUCHAR)dest + ulCopyNow;
                VirtualAddress = (PUCHAR)VirtualAddress + ulCopyNow;
                NdisMoveMemory(dest, &PriorityDataLong, 4);
                nCopied += 4;
                dest = (PCHAR)dest + 4;
                ulCopyNow = uLength - ulCopyNow;
                if (ulCopyNow) NdisMoveMemory(dest, VirtualAddress, ulCopyNow);
                dest = (PCHAR)dest + ulCopyNow;
                nCopied += uLength;
            }
            else
            {
                NdisMoveMemory(dest, VirtualAddress, uLength);
                nCopied += uLength;
                dest = (PUCHAR)dest + uLength;
            }
        }
        NdisGetNextBuffer(pBuffer, &pBuffer);
    }

    DEBUG_EXIT_STATUS(4, nCopied);
    result.size = nCopied;
    return result;
}


/**********************************************************
    Callback on finished Tx descriptor
***********************************************************/
VOID ParaNdis_OnTransmitBufferReleased(PARANDIS_ADAPTER *pContext, IONetDescriptor *pDesc)
{
    tSendEntry *pEntry = (tSendEntry *)pDesc->ReferenceValue;
    if (pEntry)
    {
        DPrintf(2, ("[%s] Entry %p (packet %p, %d buffers) ready!", __FUNCTION__, pEntry, pEntry->packet, pDesc->nofUsedBuffers));
        pEntry->flags |= SEND_ENTRY_FLAG_READY;
        pDesc->ReferenceValue = NULL;
        ParaNdis_DebugHistory(pContext, hopBufferSent, pEntry->packet, 0, pContext->nofFreeHardwareBuffers, pContext->nofFreeTxDescriptors);
    }
    else
    {
        ParaNdis_DebugHistory(pContext, hopBufferSent, NULL, 0, pContext->nofFreeHardwareBuffers, pContext->nofFreeTxDescriptors);
        DPrintf(0, ("[%s] ERROR: Send Entry not set!", __FUNCTION__));
    }
}


static FORCEINLINE ULONG CalculateTotalOffloadSize(
    ULONG packetSize,
    ULONG mss,
    ULONG ipheaderOffset,
    ULONG maxPossiblePacketSize,
    tTcpIpPacketParsingResult packetReview)
{
    ULONG ul = 0;
    ULONG tcpipHeaders = packetReview.XxpIpHeaderSize;
    ULONG allHeaders = tcpipHeaders + ipheaderOffset;
    if (tcpipHeaders && (mss + allHeaders) <= maxPossiblePacketSize)
    {
        ULONG nFragments = (packetSize - allHeaders)/mss;
        ULONG last = (packetSize - allHeaders)%mss;
        ul = nFragments * (mss + allHeaders) + last + (last ? allHeaders : 0);
    }
    DPrintf(1, ("[%s]%s %d/%d, headers %d)",
        __FUNCTION__, !ul ? "ERROR:" : "", ul, mss, allHeaders));
    return ul;
}

/**********************************************************
Maps the HW buffers of the packet into entries of VirtIO queue
Parameters:
    miniport context
    PNDIS_PACKET Packet     packet to copy data from
    PVOID        ReferenceValue - tSendEntry * of the packet
    VirtIOBufferDescriptor buffers = array of buffers to map packet buffers
    (it contains number of SG entries >= number of hw elements in the packet)
    pIONetDescriptor pDesc - holder of VirtIO header and reserved data buffer
       for possible replacement of one or more HW buffers

Returns @pMapperResult: (zeroed before call)
    .usBuffersMapped - number of buffers mapped (one of them may be our own)
    .ulDataSize - number of bytes to report as transmitted (802.1P tag is not counted)
    .usBufferSpaceUsed - number of bytes used in data space of pIONetDescriptor pDesc
***********************************************************/
VOID ParaNdis_PacketMapper(
    PARANDIS_ADAPTER *pContext,
    PNDIS_PACKET packet,
    PVOID ReferenceValue,
    struct VirtIOBufferDescriptor *buffers,
    pIONetDescriptor pDesc,
    tMapperResult *pMapperResult)
{
    tSendEntry *pSendEntry = (tSendEntry *)ReferenceValue;
    ULONG PriorityDataLong = pSendEntry->PriorityDataLong;
    PSCATTER_GATHER_LIST pSGList = NDIS_PER_PACKET_INFO_FROM_PACKET(packet, ScatterGatherListPacketInfo);
    SCATTER_GATHER_ELEMENT *pSGElements = pSGList->Elements;


    if (pSGList && pSGList->NumberOfElements)
    {
        UINT i, lengthGet = 0, lengthPut = 0, nCompleteBuffersToSkip = 0, nBytesSkipInFirstBuffer = 0;
        if (pSendEntry->flags & (SEND_ENTRY_TSO_USED | SEND_ENTRY_TCP_CS | SEND_ENTRY_UDP_CS | SEND_ENTRY_IP_CS))
            lengthGet = pContext->Offload.ipHeaderOffset + MAX_IPV4_HEADER_SIZE + sizeof(TCPHeader);
        if (PriorityDataLong && !lengthGet)
            lengthGet = ETH_HEADER_SIZE;
        if (lengthGet)
        {
            ULONG len = 0;
            for (i = 0; i < pSGList->NumberOfElements; ++i)
            {
                len += pSGElements[i].Length;
                if (len > lengthGet)
                {
                    nBytesSkipInFirstBuffer = pSGList->Elements[i].Length - (len - lengthGet);
                    break;
                }
                DPrintf(2, ("[%s] skipping buffer %d of %d", __FUNCTION__, nCompleteBuffersToSkip, pSGElements[i].Length));
                nCompleteBuffersToSkip++;
            }
            // just for case of UDP packet shorter than TCP header
            if (lengthGet > len) lengthGet = len;
            lengthPut = lengthGet + (PriorityDataLong ? ETH_PRIORITY_HEADER_SIZE : 0);
        }

        if (lengthPut > pDesc->DataInfo.size)
        {
            DPrintf(0, ("[%s] ERROR: can not substitute %d bytes, sending as is", __FUNCTION__, lengthPut));
            nCompleteBuffersToSkip = 0;
            nBytesSkipInFirstBuffer = 0;
            lengthGet = lengthPut = 0;
        }

        if (lengthPut)
        {
            // we replace 1 or more HW buffers with one buffer preallocated for data
            buffers->physAddr = pDesc->DataInfo.Physical;
            buffers->length   = lengthPut;
            pMapperResult->usBufferSpaceUsed = (USHORT)lengthPut;
            pMapperResult->ulDataSize += lengthGet;
            pMapperResult->usBuffersMapped = (USHORT)(pSGList->NumberOfElements - nCompleteBuffersToSkip + 1);
            pSGElements += nCompleteBuffersToSkip;
            buffers++;
            DPrintf(1, ("[%s](%d bufs) skip %d buffers + %d bytes",
                __FUNCTION__, pSGList->NumberOfElements, nCompleteBuffersToSkip, nBytesSkipInFirstBuffer));
        }
        else
        {
            pMapperResult->usBuffersMapped = (USHORT)pSGList->NumberOfElements;
        }

        for (i = nCompleteBuffersToSkip; i < pSGList->NumberOfElements; ++i)
        {
            if (nBytesSkipInFirstBuffer)
            {
                buffers->physAddr.QuadPart = pSGElements->Address.QuadPart + nBytesSkipInFirstBuffer;
                buffers->length   = pSGElements->Length - nBytesSkipInFirstBuffer;
                DPrintf(2, ("[%s] using HW buffer %d of %d-%d", __FUNCTION__, i, pSGElements->Length, nBytesSkipInFirstBuffer));
                nBytesSkipInFirstBuffer = 0;
            }
            else
            {
                buffers->physAddr = pSGElements->Address;
                buffers->length   = pSGElements->Length;
            }
            pMapperResult->ulDataSize += buffers->length;
            pSGElements++;
            buffers++;
        }

        if (lengthPut)
        {
            PVOID pBuffer = pDesc->DataInfo.Virtual;
            PVOID pIpHeader = RtlOffsetToPointer(pBuffer, pContext->Offload.ipHeaderOffset);
            ParaNdis_PacketCopier(packet, pBuffer, lengthGet, ReferenceValue, TRUE);

            if (pSendEntry->flags & SEND_ENTRY_TSO_USED)
            {
                tTcpIpPacketParsingResult packetReview;
                ULONG dummyTransferSize = 0;
                USHORT saveBuffers = pMapperResult->usBuffersMapped;
                ULONG flags = pcrIpChecksum | pcrTcpChecksum | pcrFixIPChecksum | pcrFixPHChecksum;
                pMapperResult->usBuffersMapped = 0;
                packetReview = ParaNdis_CheckSumVerify(
                    pIpHeader,
                    lengthGet - pContext->Offload.ipHeaderOffset,
                    flags,
                    __FUNCTION__);
                /* uncomment to verify */
                /*
                packetReview = ParaNdis_CheckSumVerify(
                    pIpHeader,
                    lengthGet - pContext->Offload.ipHeaderOffset,
                    pcrIpChecksum | pcrTcpChecksum,
                    __FUNCTION__);
                */
                if (packetReview.ipCheckSum == ppresCSOK || packetReview.fixedIpCS)
                {
                    dummyTransferSize = CalculateTotalOffloadSize(
                        pMapperResult->ulDataSize,
                        pSendEntry->ipTransferUnit,
                        pContext->Offload.ipHeaderOffset,
                        pContext->MaxPacketSize.nMaxFullSizeOS,
                        packetReview);
                }
                else
                {
                    DPrintf(0, ("[%s] ERROR locating IP header in %d bytes(IP header of %d)", __FUNCTION__,
                        lengthGet, packetReview.ipHeaderSize));
                }
                NDIS_PER_PACKET_INFO_FROM_PACKET(packet, TcpLargeSendPacketInfo) = (PVOID)(ULONG_PTR)dummyTransferSize;
                if (dummyTransferSize)
                {
                    virtio_net_hdr_basic *pheader = pDesc->HeaderInfo.Virtual;
                    unsigned short addPriorityLen = PriorityDataLong ? ETH_PRIORITY_HEADER_SIZE : 0;
                    pheader->flags = VIRTIO_NET_HDR_F_NEEDS_CSUM;
                    pheader->gso_type = VIRTIO_NET_HDR_GSO_TCPV4;
                    pheader->hdr_len  = (USHORT)(packetReview.XxpIpHeaderSize + pContext->Offload.ipHeaderOffset) + addPriorityLen;
                    pheader->gso_size = (USHORT)pSendEntry->ipTransferUnit;
                    pheader->csum_start = (USHORT)pContext->Offload.ipHeaderOffset + (USHORT)packetReview.ipHeaderSize + addPriorityLen;
                    pheader->csum_offset = TCP_CHECKSUM_OFFSET;
                    pMapperResult->usBuffersMapped = saveBuffers;
                }
            }
            else if (pSendEntry->flags & SEND_ENTRY_IP_CS)
            {
                ParaNdis_CheckSumVerify(
                    pIpHeader,
                    lengthGet - pContext->Offload.ipHeaderOffset,
                    pcrIpChecksum | pcrFixIPChecksum,
                    __FUNCTION__);
            }

            if (PriorityDataLong && pMapperResult->usBuffersMapped)
            {
                RtlMoveMemory(
                    RtlOffsetToPointer(pBuffer, ETH_PRIORITY_HEADER_OFFSET + ETH_PRIORITY_HEADER_SIZE),
                    RtlOffsetToPointer(pBuffer, ETH_PRIORITY_HEADER_OFFSET),
                    lengthGet - ETH_PRIORITY_HEADER_OFFSET
                    );
                NdisMoveMemory(
                    RtlOffsetToPointer(pBuffer, ETH_PRIORITY_HEADER_OFFSET),
                    &PriorityDataLong,
                    sizeof(ETH_PRIORITY_HEADER_SIZE));
                DPrintf(1, ("[%s] Populated priority value %lX", __FUNCTION__, PriorityDataLong));
            }
        }
    }

}

static void InitializeTransferParameters(tTxOperationParameters *pParams, tSendEntry *pEntry)
{
    ULONG flags = (pEntry->flags & SEND_ENTRY_TSO_USED) ? pcrLSO : 0;
    if (pEntry->flags & SEND_ENTRY_NO_INDIRECT) flags |= pcrNoIndirect;
    NdisQueryPacket(pEntry->packet, &pParams->nofSGFragments, NULL, NULL, (PUINT)&pParams->ulDataSize);
    pParams->ReferenceValue = pEntry;
    pParams->packet = pEntry->packet;
    pParams->offloadMss = (pEntry->flags & SEND_ENTRY_TSO_USED) ? pEntry->ipTransferUnit : 0;
    // on NDIS5 it is unknown
    pParams->tcpHeaderOffset = 0;
    // fills only if SGList present in the packet
    GET_NUMBER_OF_SG_ELEMENTS(pEntry->packet, &pParams->nofSGFragments);
    if (NDIS_GET_PACKET_PROTOCOL_TYPE(pEntry->packet) == NDIS_PROTOCOL_ID_TCP_IP)
    {
        flags |= pcrIsIP;
        if (pEntry->flags & SEND_ENTRY_TCP_CS)
        {
            flags |= pcrTcpChecksum;
        }
        if (pEntry->flags & SEND_ENTRY_UDP_CS)
        {
            flags |= pcrUdpChecksum;
        }
        if (pEntry->flags & SEND_ENTRY_IP_CS)
        {
            flags |= pcrIpChecksum;
        }
    }
    if (pEntry->PriorityDataLong) flags |= pcrPriorityTag;
    pParams->flags = flags;
}

BOOLEAN ParaNdis_ProcessTx(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN IsDpc,
    BOOLEAN IsInterrupt)
{
    LIST_ENTRY DoneList;
    BOOLEAN bDoKick = FALSE;
    UINT nBuffersSent = 0, nBytesSent = 0;
    BOOLEAN bDataAvailable = FALSE;
    tSendEntry *pEntry;
    ONPAUSECOMPLETEPROC CallbackToCall = NULL;
    InitializeListHead(&DoneList);
    UNREFERENCED_PARAMETER(IsDpc);
    NdisAcquireSpinLock(&pContext->SendLock);

    ParaNdis_DebugHistory(pContext, hopTxProcess, NULL, 1, pContext->nofFreeHardwareBuffers, pContext->nofFreeTxDescriptors);
    do
    {
        if(IsTimeToReleaseTx(pContext))
        {
            // release some buffers
            ParaNdis_VirtIONetReleaseTransmitBuffers(pContext);
        }
        pEntry = NULL;
        if (!IsListEmpty(&pContext->SendQueue))
        {
            tCopyPacketResult result;
            tTxOperationParameters Params;
            pEntry = (tSendEntry *)RemoveHeadList(&pContext->SendQueue);
            InitializeTransferParameters(&Params, pEntry);
            bDataAvailable = TRUE;
            result = ParaNdis_DoSubmitPacket(pContext, &Params);
            if (result.error == cpeNoBuffer)
            {
                // can not send now, try next time
                InsertHeadList(&pContext->SendQueue, &pEntry->list);
                pEntry = NULL;
            }
            else if (result.error == cpeNoIndirect)
            {
                InsertHeadList(&pContext->SendQueue, &pEntry->list);
                pEntry->flags |= SEND_ENTRY_NO_INDIRECT;
            }
            else
            {
                InsertTailList(&pContext->TxWaitingList, &pEntry->list);
                ParaNdis_DebugHistory(pContext, hopSubmittedPacket, pEntry->packet, 0, result.error, Params.flags);
                if (!result.size)
                {
                    NDIS_STATUS status = NDIS_STATUS_FAILURE;
                    DPrintf(0, ("[%s] ERROR %d copying packet!", __FUNCTION__, result.error));
                    if (result.error == cpeTooLarge)
                    {
                        status = NDIS_STATUS_BUFFER_OVERFLOW;
                        pContext->Statistics.ifOutErrors++;
                    }
                    NDIS_SET_PACKET_STATUS(pEntry->packet, status);
                    pEntry->flags |= SEND_ENTRY_FLAG_READY;
                    // do not worry, go to the next one

                }
                else
                {
                    nBuffersSent++;
                    nBytesSent += result.size;
                    DPrintf(2, ("[%s] Scheduled packet %p, entry %p(%d bytes)!", __FUNCTION__,
                        pEntry->packet, pEntry, result.size));
                }
            }
        }
    } while (pEntry);

    if (nBuffersSent)
    {
        if(IsInterrupt)
        {
            bDoKick = TRUE;
        }
        else
        {
#ifdef PARANDIS_TEST_TX_KICK_ALWAYS
            virtqueue_kick_always(pContext->NetSendQueue);
#else
            virtqueue_kick(pContext->NetSendQueue);
#endif
        }
        DPrintf(2, ("[%s] sent down %d p.(%d b.)", __FUNCTION__, nBuffersSent, nBytesSent));
    }
    else if (bDataAvailable)
    {
        DPrintf(2, ("[%s] nothing sent", __FUNCTION__));
    }

    /* now check the waiting list of packets */
    while (!IsListEmpty(&pContext->TxWaitingList))
    {
        pEntry = (tSendEntry *)RemoveHeadList(&pContext->TxWaitingList);
        if (pEntry->flags & SEND_ENTRY_FLAG_READY)
        {
            InsertTailList(&DoneList, &pEntry->list);
        }
        else
        {
            InsertHeadList(&pContext->TxWaitingList, &pEntry->list);
            break;
        }
    }

    if (IsListEmpty(&pContext->TxWaitingList) && pContext->SendState == srsPausing && pContext->SendPauseCompletionProc)
    {
        CallbackToCall = pContext->SendPauseCompletionProc;
        pContext->SendPauseCompletionProc = NULL;
        pContext->SendState = srsDisabled;
        ParaNdis_DebugHistory(pContext, hopInternalSendPause, NULL, 0, 0, 0);
    }
    NdisReleaseSpinLock(&pContext->SendLock);

    while (!IsListEmpty(&DoneList))
    {
        pEntry = (tSendEntry *)RemoveHeadList(&DoneList);
        CompletePacket(pContext, pEntry->packet);
        NdisFreeMemory(pEntry, 0, 0);
    }
    if (CallbackToCall) CallbackToCall(pContext);

    return bDoKick;
}

/**********************************************************
NDIS releases packets previously indicated by miniport
Free the packet's buffer and the packet back to their pools
Returns VirtIO buffer back to queue of free blocks
Parameters:
    context
    IN PNDIS_PACKET Packet      returned packet
***********************************************************/
VOID NTAPI ParaNdis5_ReturnPacket(IN NDIS_HANDLE  MiniportAdapterContext,IN PNDIS_PACKET Packet)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    pIONetDescriptor pBufferDescriptor;
    pBufferDescriptor = (pIONetDescriptor) *REF_MINIPORT(Packet);
    DPrintf(4, ("[%s] buffer %p", __FUNCTION__, pBufferDescriptor));

    NdisAcquireSpinLock(&pContext->ReceiveLock);
    pContext->ReuseBufferProc(pContext, pBufferDescriptor);
    NdisReleaseSpinLock(&pContext->ReceiveLock);
}

static __inline tSendEntry * PrepareSendEntry(PARANDIS_ADAPTER *pContext, PNDIS_PACKET Packet, ULONG len)
{
    ULONG mss = (ULONG)(ULONG_PTR)NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpLargeSendPacketInfo);
    UINT  protocol = NDIS_GET_PACKET_PROTOCOL_TYPE(Packet);
    LPCSTR errorFmt = NULL;
    LPCSTR offloadName = "NO offload";
    tSendEntry *pse = (tSendEntry *)ParaNdis_AllocateMemory(pContext, sizeof(tSendEntry));
    if (pse)
    {
        NDIS_PACKET_8021Q_INFO qInfo;
        pse->packet = Packet;
        pse->flags  = 0;
        pse->PriorityDataLong = 0;
        pse->ipTransferUnit = len;
        //pse->fullTCPCheckSum = 0;
        qInfo.Value = pContext->ulPriorityVlanSetting ?
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) : NULL;
        if (!qInfo.TagHeader.VlanId) qInfo.TagHeader.VlanId = pContext->VlanId;
        if (qInfo.TagHeader.CanonicalFormatId || !IsValidVlanId(pContext, qInfo.TagHeader.VlanId))
        {
            DPrintf(0, ("[%s] Discarding priority tag %p", __FUNCTION__, qInfo.Value));
            errorFmt = "invalid priority tag";
        }
        else if (qInfo.Value)
        {
            // ignore priority, if configured
            if (!IsPrioritySupported(pContext))
                qInfo.TagHeader.UserPriority = 0;
            // ignore VlanId, if specified
            if (!IsVlanSupported(pContext))
                qInfo.TagHeader.VlanId = 0;
            SetPriorityData(pse->PriorityData, qInfo.TagHeader.UserPriority, qInfo.TagHeader.VlanId);
            DPrintf(1, ("[%s] Populated priority tag %p", __FUNCTION__, qInfo.Value));
        }

        if (!errorFmt && !mss && len > pContext->MaxPacketSize.nMaxFullSizeOS)
        {
            DPrintf(0, ("[%s] Request for offload with NO MSS, lso %d, ipheader %d",
                __FUNCTION__, pContext->Offload.flags.fTxLso, pContext->Offload.ipHeaderOffset));
            if (pContext->Offload.flags.fTxLso && pContext->Offload.ipHeaderOffset)
            {
                mss = pContext->MaxPacketSize.nMaxFullSizeOS;
            }
            else
                errorFmt = "illegal LSO request";
        }

        if (errorFmt)
        {
            // already failed
        }
        else if (mss > pContext->MaxPacketSize.nMaxFullSizeOS)
            errorFmt = "mss is too big";
        else if (len > 0xFFFF)
            errorFmt = "packet is bigger than we able to send";
        else if (mss && pContext->Offload.flags.fTxLso)
        {
            offloadName = "LSO";
            pse->ipTransferUnit = mss;
            pse->flags |= SEND_ENTRY_TSO_USED;
            // todo: move to common space
            // to transmit 'len' with 'mss' we usually need 2 additional buffers
            if ((len / mss + 3) > pContext->maxFreeHardwareBuffers)
                errorFmt = "packet too big to fragment";
            else if (len < pContext->Offload.ipHeaderOffset)
                errorFmt = "ip offset is bigger than packet";
            else if (protocol != NDIS_PROTOCOL_ID_TCP_IP)
                errorFmt = "attempt to offload non-IP packet";
            else if (mss < pContext->Offload.ipHeaderOffset)
                errorFmt = "mss is too small";
        }
        else
        {
            // unexpected CS requests we do not fail - WHQL expects us to send them as is
            NDIS_TCP_IP_CHECKSUM_PACKET_INFO csInfo;
            csInfo.Value = (ULONG)(ULONG_PTR)NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpIpChecksumPacketInfo);
            if (csInfo.Transmit.NdisPacketChecksumV4)
            {
                if (csInfo.Transmit.NdisPacketTcpChecksum)
                {
                    offloadName = "TCP CS";
                    if (pContext->Offload.flags.fTxTCPChecksum)
                        pse->flags |= SEND_ENTRY_TCP_CS;
                    else
                        errorFmt = "TCP CS requested but not enabled";
                }
                if (csInfo.Transmit.NdisPacketUdpChecksum)
                {
                    offloadName = "UDP CS";
                    if (pContext->Offload.flags.fTxUDPChecksum)
                        pse->flags |= SEND_ENTRY_UDP_CS;
                    else
                        errorFmt = "UDP CS requested but not enabled";
                }
                if (csInfo.Transmit.NdisPacketIpChecksum)
                {
                    if (pContext->Offload.flags.fTxIPChecksum)
                        pse->flags |= SEND_ENTRY_IP_CS;
                    else
                        errorFmt = "IP CS requested but not enabled";
                }
                if (errorFmt)
                {
                    DPrintf(0, ("[%s] ERROR: %s (len %d)", __FUNCTION__, errorFmt, len));
                    errorFmt = NULL;
                }
            }
        }
    }

    if (errorFmt)
    {
        DPrintf(0, ("[%s] ERROR: %s (len %d, mss %d)", __FUNCTION__, errorFmt, len, mss));
        if (pse) NdisFreeMemory(pse, 0, 0);
        pse = NULL;
    }
    else
    {
        NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, TcpLargeSendPacketInfo) = (PVOID)(ULONG_PTR)0;
        DPrintf(1, ("[%s] Sending packet of %d with %s", __FUNCTION__, len, offloadName));
        if (pContext->bDoIPCheckTx)
        {
            tTcpIpPacketParsingResult res;
            VOID *pcopy = ParaNdis_AllocateMemory(pContext, len);
            ParaNdis_PacketCopier(pse->packet, pcopy, len, pse, TRUE);
            res = ParaNdis_CheckSumVerify(
                RtlOffsetToPointer(pcopy, pContext->Offload.ipHeaderOffset),
                len,
                pcrAnyChecksum/* | pcrFixAnyChecksum*/,
                __FUNCTION__);
            /*
            if (res.xxpStatus == ppresXxpKnown)
            {
                TCPHeader *ptcp = (TCPHeader *)
                    RtlOffsetToPointer(pcopy, pContext->Offload.ipHeaderOffset + res.ipHeaderSize);
                pse->fullTCPCheckSum = ptcp->tcp_xsum;
            }
            */
            NdisFreeMemory(pcopy, 0, 0);
        }
    }
    return pse;
}

/**********************************************************
NDIS sends us packets
    Queues packets internally and calls the procedure to process the queue

Parameters:
    context
    IN PPNDIS_PACKET PacketArray        Array of packets to send
    IN UINT NumberOfPackets             number of packets

***********************************************************/
VOID NTAPI ParaNdis5_SendPackets(IN NDIS_HANDLE MiniportAdapterContext,
                               IN PPNDIS_PACKET PacketArray,
                               IN UINT NumberOfPackets)
{
    UINT i;
    LIST_ENTRY FailedList, DoneList;
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    InitializeListHead(&FailedList);
    InitializeListHead(&DoneList);
    DPrintf(3, ("[%s] %d packets", __FUNCTION__, NumberOfPackets));
    ParaNdis_DebugHistory(pContext, hopSend, NULL, 1, NumberOfPackets, 0);

    NdisAcquireSpinLock(&pContext->SendLock);

    for (i = 0; i < NumberOfPackets; ++i)
    {
        UINT uPacketLength = 0;
        NdisQueryPacketLength(PacketArray[i], &uPacketLength);
        NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_SUCCESS);
        NdisInterlockedIncrement(&pContext->NetTxPacketsToReturn);
        if (!pContext->bSurprizeRemoved && pContext->bConnected && pContext->SendState == srsEnabled && uPacketLength)
        {
            tSendEntry *pse = PrepareSendEntry(pContext, PacketArray[i], uPacketLength);
            if (!pse)
            {
                NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_FAILURE);
                CompletePacket(pContext, PacketArray[i]);
            }
            else
            {
                UINT nFragments = 0;
                GET_NUMBER_OF_SG_ELEMENTS(PacketArray[i], &nFragments);
                ParaNdis_DebugHistory(pContext, hopSendPacketMapped, PacketArray[i], 0, nFragments, 0);
                InsertTailList(&pContext->SendQueue, &pse->list);
            }
        }
        else
        {
            NDIS_STATUS status = NDIS_STATUS_FAILURE;
            if (pContext->bSurprizeRemoved) status = NDIS_STATUS_NOT_ACCEPTED;
            NDIS_SET_PACKET_STATUS(PacketArray[i], status);
            CompletePacket(pContext, PacketArray[i]);
            DPrintf(1, ("[%s] packet of %d rejected", __FUNCTION__, uPacketLength));
        }
    }

    NdisReleaseSpinLock(&pContext->SendLock);

    ParaNdis_ProcessTx(pContext, FALSE, FALSE);
}

/**********************************************************
NDIS procedure, not easy to test
NDIS asks us to cancel packets with specified CancelID

Parameters:
    context
    PVOID CancelId              ID to cancel

***********************************************************/
VOID NTAPI ParaNdis5_CancelSendPackets(IN NDIS_HANDLE MiniportAdapterContext,IN PVOID CancelId)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    LIST_ENTRY DoneList, KeepList;
    UINT n = 0;
    tSendEntry *pEntry;
    DEBUG_ENTRY(0);
    InitializeListHead(&DoneList);
    InitializeListHead(&KeepList);
    NdisAcquireSpinLock(&pContext->SendLock);
    while ( !IsListEmpty(&pContext->SendQueue))
    {
        PNDIS_PACKET  Packet;
        pEntry = (tSendEntry *)RemoveHeadList(&pContext->SendQueue);
        Packet = pEntry->packet;
        if (NDIS_GET_PACKET_CANCEL_ID(Packet) == CancelId)
        {
            InsertTailList(&DoneList, &pEntry->list);
            ++n;
        }
        else InsertTailList(&KeepList, &pEntry->list);
    }
    while ( !IsListEmpty(&KeepList))
    {
        pEntry = (tSendEntry *)RemoveHeadList(&KeepList);
        InsertTailList(&pContext->SendQueue, &pEntry->list);
    }
    NdisReleaseSpinLock(&pContext->SendLock);
    while (!IsListEmpty(&DoneList))
    {
        pEntry = (tSendEntry *)RemoveHeadList(&DoneList);
        NDIS_SET_PACKET_STATUS(pEntry->packet, NDIS_STATUS_REQUEST_ABORTED);
        CompletePacket(pContext, pEntry->packet);
        NdisFreeMemory(pEntry, 0, 0);
    }
    DEBUG_EXIT_STATUS(0, n);
}

/**********************************************************
Request to pause or resume data transmit
if stopped, all the packets in internal queue are returned
Parameters:
    context
    BOOLEAN bStop       1/0 - top or resume
***********************************************************/
NDIS_STATUS ParaNdis5_StopSend(PARANDIS_ADAPTER *pContext, BOOLEAN bStop, ONPAUSECOMPLETEPROC Callback)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    if (bStop)
    {
        LIST_ENTRY DoneList;
        tSendEntry *pEntry;
        DEBUG_ENTRY(0);
        ParaNdis_DebugHistory(pContext, hopInternalSendPause, NULL, 1, 0, 0);
        InitializeListHead(&DoneList);
        NdisAcquireSpinLock(&pContext->SendLock);
        if (IsListEmpty(&pContext->TxWaitingList))
        {
            pContext->SendState = srsDisabled;
            while (!IsListEmpty(&pContext->SendQueue))
            {
                pEntry = (tSendEntry *)RemoveHeadList(&pContext->SendQueue);
                InsertTailList(&DoneList, &pEntry->list);
            }
            ParaNdis_DebugHistory(pContext, hopInternalSendPause, NULL, 0, 0, 0);
        }
        else
        {
            pContext->SendState = srsPausing;
            pContext->SendPauseCompletionProc = Callback;
            status = NDIS_STATUS_PENDING;
            while (!IsListEmpty(&pContext->SendQueue))
            {
                pEntry = (tSendEntry *)RemoveHeadList(&pContext->SendQueue);
                pEntry->flags |= SEND_ENTRY_FLAG_READY;
                InsertTailList(&pContext->TxWaitingList, &pEntry->list);
            }
        }

        NdisReleaseSpinLock(&pContext->SendLock);
        while (!IsListEmpty(&DoneList))
        {
            pEntry = (tSendEntry *)RemoveHeadList(&DoneList);
            NDIS_SET_PACKET_STATUS(pEntry->packet, NDIS_STATUS_REQUEST_ABORTED);
            CompletePacket(pContext, pEntry->packet);
            NdisFreeMemory(pEntry, 0, 0);
        }
    }
    else
    {
        pContext->SendState = srsEnabled;
        ParaNdis_DebugHistory(pContext, hopInternalSendResume, NULL, 0, 0, 0);
    }
    return status;
}

/**********************************************************
Pause or resume receive operation:
Parameters:
    context
    BOOLEAN bStop                       1/0 - pause or resume
    ONPAUSECOMPLETEPROC Callback        callback to call, if not completed immediately

Return value:
    SUCCESS, if there is no RX packets under NDIS management
    PENDING, if we need to wait until NDIS returns us packets
***********************************************************/
NDIS_STATUS ParaNdis5_StopReceive(
    PARANDIS_ADAPTER *pContext,
    BOOLEAN bStop,
    ONPAUSECOMPLETEPROC Callback
    )
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    if (bStop)
    {
        ParaNdis_DebugHistory(pContext, hopInternalReceivePause, NULL, 1, 0, 0);
        NdisAcquireSpinLock(&pContext->ReceiveLock);
        if (IsListEmpty(&pContext->NetReceiveBuffersWaiting))
        {
            pContext->ReceiveState = srsDisabled;
            ParaNdis_DebugHistory(pContext, hopInternalReceivePause, NULL, 0, 0, 0);
        }
        else
        {
            pContext->ReceiveState = srsPausing;
            pContext->ReceivePauseCompletionProc = Callback;
            status = NDIS_STATUS_PENDING;
        }
        NdisReleaseSpinLock(&pContext->ReceiveLock);
    }
    else
    {
        pContext->ReceiveState = srsEnabled;
        ParaNdis_DebugHistory(pContext, hopInternalReceiveResume, NULL, 0, 0, 0);
    }
    return status;
}

/*************************************************************
Required NDIS procedure, spawns regular (Common) DPC processing
*************************************************************/
VOID NTAPI ParaNdis5_HandleDPC(IN NDIS_HANDLE MiniportAdapterContext)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)MiniportAdapterContext;
    ULONG requiresProcessing;
    BOOLEAN unused;
    DEBUG_ENTRY(7);
    // we do not need the timer, as DPC will do all the job
    // this is not a problem if the timer procedure is already running,
    // we need to do our job anyway
    NdisCancelTimer(&pContext->DPCPostProcessTimer, &unused);
    requiresProcessing = ParaNdis_DPCWorkBody(pContext, PARANDIS_UNLIMITED_PACKETS_TO_INDICATE);
    if (requiresProcessing)
    {
        // we need to request additional DPC
        InterlockedOr(&pContext->InterruptStatus, requiresProcessing);
        NdisSetTimer(&pContext->DPCPostProcessTimer, 10);
    }
}

BOOLEAN ParaNdis_SynchronizeWithInterrupt(
    PARANDIS_ADAPTER *pContext,
    ULONG messageId,
    tSynchronizedProcedure procedure,
    PVOID parameter)
{
    tSynchronizedContext SyncContext;
    SyncContext.pContext  = pContext;
    SyncContext.Parameter = parameter;
    return NdisMSynchronizeWithInterrupt(&pContext->Interrupt, procedure, &SyncContext);
}
