/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Interrupt handlers
 * COPYRIGHT:   2013 Cameron Gutman (cameron.gutman@reactos.org)
 *              2018 Mark Jansen (mark.jansen@reactos.org)
 *              2019 Victor Pereertkin (victor.perevertkin@reactos.org)
 */

#include "nic.h"

#include <debug.h>

VOID
NTAPI
MiniportISR(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext)
{
    ULONG Value;
    PE1000_ADAPTER Adapter = (PE1000_ADAPTER)MiniportAdapterContext;

    /* Reading the interrupt acknowledges them */
    E1000ReadUlong(Adapter, E1000_REG_ICR, &Value);

    Value &= Adapter->InterruptMask;
    _InterlockedOr(&Adapter->InterruptPending, Value);

    if (Value)
    {
        *InterruptRecognized = TRUE;
        /* Mark the events pending service */
        *QueueMiniportHandleInterrupt = TRUE;
    }
    else
    {
        /* This is not ours. */
        *InterruptRecognized = FALSE;
        *QueueMiniportHandleInterrupt = FALSE;
    }
}

VOID
NTAPI
MiniportHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
    ULONG InterruptPending;
    PE1000_ADAPTER Adapter = (PE1000_ADAPTER)MiniportAdapterContext;
    volatile PE1000_TRANSMIT_DESCRIPTOR TransmitDescriptor;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    InterruptPending = _InterlockedExchange(&Adapter->InterruptPending, 0);


    /* Link State Changed */
    if (InterruptPending & E1000_IMS_LSC)
    {
        ULONG Status;

        InterruptPending &= ~E1000_IMS_LSC;
        NDIS_DbgPrint(MAX_TRACE, ("Link status changed!.\n"));

        NICUpdateLinkStatus(Adapter);

        Status = Adapter->MediaState == NdisMediaStateConnected ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT;

        NdisMIndicateStatus(Adapter->AdapterHandle, Status, NULL, 0);
        NdisMIndicateStatusComplete(Adapter->AdapterHandle);
    }

    /* Handling receive interrupts */
    if (InterruptPending & (E1000_IMS_RXDMT0 | E1000_IMS_RXT0))
    {
        volatile PE1000_RECEIVE_DESCRIPTOR ReceiveDescriptor;
        PETH_HEADER EthHeader;
        ULONG BufferOffset;
        BOOLEAN bGotAny = FALSE;
        ULONG RxDescHead, RxDescTail, CurrRxDesc;

        /* Clear out these interrupts */
        InterruptPending &= ~(E1000_IMS_RXDMT0 | E1000_IMS_RXT0);

        E1000ReadUlong(Adapter, E1000_REG_RDH, &RxDescHead);
        E1000ReadUlong(Adapter, E1000_REG_RDT, &RxDescTail);

        while (((RxDescTail + 1) % NUM_RECEIVE_DESCRIPTORS) != RxDescHead)
        {
            CurrRxDesc = (RxDescTail + 1) % NUM_RECEIVE_DESCRIPTORS;
            BufferOffset = CurrRxDesc * Adapter->ReceiveBufferEntrySize;
            ReceiveDescriptor = Adapter->ReceiveDescriptors + CurrRxDesc;

            /* Check if the hardware have released this descriptor (DD - Descriptor Done) */
            if (!(ReceiveDescriptor->Status & E1000_RDESC_STATUS_DD))
            {
                /* No need to check descriptors after the first unfinished one */
                break;
            }

            /* Ignoring these flags for now */
            ReceiveDescriptor->Status &= ~(E1000_RDESC_STATUS_IXSM | E1000_RDESC_STATUS_PIF);

            if (ReceiveDescriptor->Status != (E1000_RDESC_STATUS_EOP | E1000_RDESC_STATUS_DD))
            {
                NDIS_DbgPrint(MIN_TRACE, ("Unrecognized ReceiveDescriptor status flag: %u\n", ReceiveDescriptor->Status));
            }

            /* Make sure the receive indications are enabled */
            if (!Adapter->PacketFilter)
            {
                goto NextReceiveDescriptor;
            }

            if (ReceiveDescriptor->Length != 0 && ReceiveDescriptor->Address != 0)
            {
                EthHeader = (PETH_HEADER)(Adapter->ReceiveBuffer + BufferOffset);

                NdisMEthIndicateReceive(Adapter->AdapterHandle,
                                        NULL,
                                        (PCHAR)EthHeader,
                                        sizeof(ETH_HEADER),
                                        (PCHAR)(EthHeader + 1),
                                        ReceiveDescriptor->Length - sizeof(ETH_HEADER),
                                        ReceiveDescriptor->Length - sizeof(ETH_HEADER));

                bGotAny = TRUE;
            }
            else
            {
                NDIS_DbgPrint(MIN_TRACE, ("Got a NULL descriptor"));
            }

NextReceiveDescriptor:
            /* Give the descriptor back */
            ReceiveDescriptor->Status = 0;

            RxDescTail = CurrRxDesc;
        }

        if (bGotAny)
        {
            /* Write back new tail value */
            E1000WriteUlong(Adapter, E1000_REG_RDT, RxDescTail);

            NDIS_DbgPrint(MAX_TRACE, ("Rx done (RDH: %u, RDT: %u)\n", RxDescHead, RxDescTail));

            NdisMEthIndicateReceiveComplete(Adapter->AdapterHandle);
        }
    }

    /* Handling transmit interrupts */
    if (InterruptPending & (E1000_IMS_TXD_LOW | E1000_IMS_TXDW | E1000_IMS_TXQE))
    {
        PNDIS_PACKET AckPackets[40] = {0};
        ULONG NumPackets = 0, i;

        /* Clear out these interrupts */
        InterruptPending &= ~(E1000_IMS_TXD_LOW | E1000_IMS_TXDW | E1000_IMS_TXQE);

        while ((Adapter->TxFull || Adapter->LastTxDesc != Adapter->CurrentTxDesc) && NumPackets < ARRAYSIZE(AckPackets))
        {
            TransmitDescriptor = Adapter->TransmitDescriptors + Adapter->LastTxDesc;

            if (TransmitDescriptor->Status & E1000_TDESC_STATUS_DD)
            {
                if (Adapter->TransmitPackets[Adapter->LastTxDesc])
                {
                    AckPackets[NumPackets++] = Adapter->TransmitPackets[Adapter->LastTxDesc];
                    Adapter->TransmitPackets[Adapter->LastTxDesc] = NULL;
                    TransmitDescriptor->Status = 0;
                }

                Adapter->LastTxDesc = (Adapter->LastTxDesc + 1) % NUM_TRANSMIT_DESCRIPTORS;
                Adapter->TxFull = FALSE;
            }
            else
            {
                break;
            }
        }

        if (NumPackets)
        {
            NDIS_DbgPrint(MAX_TRACE, ("Tx: (TDH: %u, TDT: %u)\n", Adapter->CurrentTxDesc, Adapter->LastTxDesc));
            NDIS_DbgPrint(MAX_TRACE, ("Tx Done: %u packets to ack\n", NumPackets));

            for (i = 0; i < NumPackets; ++i)
            {
                NdisMSendComplete(Adapter->AdapterHandle, AckPackets[i], NDIS_STATUS_SUCCESS);
            }
        }
    }

    ASSERT(InterruptPending == 0);
}
