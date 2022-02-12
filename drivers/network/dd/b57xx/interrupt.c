/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt handlers
 * COPYRIGHT:   Copyright 2021-2022 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
MiniportDisableInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext)
{
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    NICDisableInterrupts(Adapter);
}

VOID
NTAPI
MiniportEnableInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext)
{
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    NICEnableInterrupts(Adapter);
}

VOID
NTAPI
MiniportISR(_Out_ PBOOLEAN InterruptRecognized,
            _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
            _In_  NDIS_HANDLE MiniportAdapterContext)
{
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    if (Adapter->Status.pBlock->Status & B57XX_SB_UPDATED && Adapter->Interrupt.Pending == FALSE)
    {
        NICInterruptAcknowledge(Adapter);
        
        Adapter->Status.LastTag = Adapter->Status.pBlock->StatusTag;
        Adapter->Status.pBlock->Status &= ~B57XX_SB_UPDATED;
        
        Adapter->Interrupt.Pending = TRUE;
        
        *InterruptRecognized = TRUE;
        *QueueMiniportHandleInterrupt = TRUE;
    }
    else
    {
        *InterruptRecognized = FALSE;
        *QueueMiniportHandleInterrupt = FALSE;
    }
}

VOID
NTAPI
MiniportHandleInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext)
{
    ULONG ProducerIndex;
    ULONG ConsumerIndex;
    PCHAR BufferBase;
    PB57XX_RECEIVE_BUFFER_DESCRIPTOR pReceiveDescriptor;
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    BOOLEAN LinkStateChange = FALSE;
    BOOLEAN GotAny = FALSE;
    
    if (Adapter->HardwareStatus != NdisHardwareStatusReady)
    {
        return;
    }
    
    NdisDprAcquireSpinLock(&Adapter->Interrupt.Lock);
    if (Adapter->Interrupt.Pending == FALSE)
    {
        return;
    }
    Adapter->Interrupt.Pending = FALSE;
    
    do
    {
        Adapter->Status.pBlock->Status &= ~B57XX_SB_UPDATED;
        
        /* Process any link changes */
        if (Adapter->Status.pBlock->Status & B57XX_SB_LINKSTATE)
        {
            Adapter->Status.pBlock->Status &= ~B57XX_SB_LINKSTATE;
            
            LinkStateChange = TRUE;
            NDIS_MinDbgPrint("Link state change\n");
            
            NdisDprReleaseSpinLock(&Adapter->Interrupt.Lock);
            NdisMIndicateStatus(Adapter->MiniportAdapterHandle,
                                NICUpdateLinkStatus(Adapter),
                                NULL,
                                0);
            NdisMIndicateStatusComplete(Adapter->MiniportAdapterHandle);
            NdisDprAcquireSpinLock(&Adapter->Interrupt.Lock);
        }
        
        /* Process any received frames (receive interrupts) */
        while (Adapter->ReturnConsumer[0].Index !=
               Adapter->Status.pBlock->RingIndexPairs[0].ReceiveProducerIndex)
        {
            ConsumerIndex = Adapter->ReturnConsumer[0].Index;
            ProducerIndex = Adapter->StandardProducer.Index;
            pReceiveDescriptor = Adapter->ReturnConsumer[0].pRing + ConsumerIndex;
            
            if (pReceiveDescriptor->Flags & B57XX_RBD_FRAME_HAS_ERROR)
            {
                NDIS_MinDbgPrint("RX error (0x%x)\n", pReceiveDescriptor->Flags);
                Adapter->Statistics.ReceiveErrors++;
                goto ContinueReceive;
            }
            
            BufferBase = (PCHAR)Adapter->StandardProducer.HostBuffer +
                         Adapter->StandardProducer.FrameBufferLength * pReceiveDescriptor->Index;
            
            NdisDprReleaseSpinLock(&Adapter->Interrupt.Lock);
            NdisMEthIndicateReceive(Adapter->MiniportAdapterHandle,
                                    NULL,
                                    BufferBase,
                                    sizeof(ETH_HEADER),
                                    BufferBase + sizeof(ETH_HEADER),
                                    pReceiveDescriptor->Length - sizeof(ETH_HEADER),
                                    pReceiveDescriptor->Length - sizeof(ETH_HEADER));
            NdisDprAcquireSpinLock(&Adapter->Interrupt.Lock);
            GotAny = TRUE;
            
            Adapter->Statistics.ReceiveSuccesses++;

ContinueReceive:
            ConsumerIndex = (ConsumerIndex + 1) % Adapter->ReturnConsumer[0].Count;
            Adapter->ReturnConsumer[0].Index = ConsumerIndex;
            
            ProducerIndex = (ProducerIndex + 1) % Adapter->StandardProducer.Count;
            Adapter->StandardProducer.Index = ProducerIndex;
        }
        
        if (GotAny)
        {
            NICReceiveSignalComplete(Adapter);
            NdisDprReleaseSpinLock(&Adapter->Interrupt.Lock);
            NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);
            NdisDprAcquireSpinLock(&Adapter->Interrupt.Lock);
        }
        
        /* Process any completed frames (transmit interrupts) */
        while (Adapter->SendProducer[0].ConsumerIndex !=
               Adapter->Status.pBlock->RingIndexPairs[0].SendConsumerIndex)
        {
            ConsumerIndex = Adapter->SendProducer[0].ConsumerIndex;
            
            NdisDprReleaseSpinLock(&Adapter->Interrupt.Lock);
            NdisMSendComplete(Adapter->MiniportAdapterHandle,
                             Adapter->SendProducer[0].pPacketList[ConsumerIndex],
                             NDIS_STATUS_SUCCESS);
            NdisDprAcquireSpinLock(&Adapter->Interrupt.Lock);
            Adapter->SendProducer[0].RingFull = FALSE;
            
            Adapter->Statistics.TransmitSuccesses++;
            
            ConsumerIndex = (ConsumerIndex + 1) % Adapter->SendProducer[0].Count;
            Adapter->SendProducer[0].ConsumerIndex = ConsumerIndex;
        }
    }
    while (NICInterruptCheckAvailability(Adapter));
    
    if (Adapter->Status.pBlock->Status & B57XX_SB_ERROR)
    {
        // It may "error" due to the link state change generating a MAC attention
        if (LinkStateChange == FALSE)
        {
            NDIS_MinDbgPrint("Error detected\n");
            NICOutputDebugInfo(Adapter);
        }
        
        Adapter->Status.pBlock->Status &= ~B57XX_SB_ERROR;
    }
    
    NICInterruptSignalComplete(Adapter);
    NdisDprReleaseSpinLock(&Adapter->Interrupt.Lock);
}
