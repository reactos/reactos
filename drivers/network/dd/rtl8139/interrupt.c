/*
 * ReactOS Realtek 8139 Driver
 *
 * Copyright (C) 2013 Cameron Gutman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "nic.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
MiniportISR (
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PRTL_ADAPTER adapter = (PRTL_ADAPTER)MiniportAdapterContext;
    ULONG csConfig;

    //
    // FIXME: We need to synchronize with this ISR for changes to InterruptPending,
    // LinkChange, MediaState, and LinkSpeedMbps. We can get away with IRQL
    // synchronization on non-SMP machines because we run a DIRQL here.
    //

    adapter->InterruptPending |= NICInterruptRecognized(adapter, InterruptRecognized);
    if (!(*InterruptRecognized))
    {
        //
        // This is not ours.
        //
        *QueueMiniportHandleInterrupt = FALSE;
        return;
    }

    //
    // We have to check for a special link change interrupt before acknowledging
    //
    if (adapter->InterruptPending & R_I_RXUNDRUN)
    {
        NdisRawReadPortUlong(adapter->IoBase + R_CSCFG, &csConfig);
        if (csConfig & R_CSCR_LINKCHNG)
        {
            adapter->LinkChange = TRUE;
            NICUpdateLinkStatus(adapter);
        }
    }

    //
    // Acknowledge the interrupt and mark the events pending service
    //
    NICAcknowledgeInterrupts(adapter);
    *QueueMiniportHandleInterrupt = TRUE;
}

VOID
NTAPI
MiniportHandleInterrupt (
    IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PRTL_ADAPTER adapter = (PRTL_ADAPTER)MiniportAdapterContext;
    ULONG txStatus;
    UCHAR command;
    PPACKET_HEADER nicHeader;
    PETH_HEADER ethHeader;

    NdisDprAcquireSpinLock(&adapter->Lock);

    NDIS_DbgPrint(MAX_TRACE, ("Interrupts pending: 0x%x\n", adapter->InterruptPending));

    //
    // Handle a link change
    //
    if (adapter->LinkChange)
    {
        NdisDprReleaseSpinLock(&adapter->Lock);
        NdisMIndicateStatus(adapter->MiniportAdapterHandle,
                            adapter->MediaState == NdisMediaStateConnected ?
                                NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
                            NULL,
                            0);
        NdisMIndicateStatusComplete(adapter->MiniportAdapterHandle);
        NdisDprAcquireSpinLock(&adapter->Lock);
        adapter->LinkChange = FALSE;
    }

    //
    // Handle a TX interrupt
    //
    if (adapter->InterruptPending & (R_I_TXOK | R_I_TXERR))
    {
        while (adapter->TxFull || adapter->DirtyTxDesc != adapter->CurrentTxDesc)
        {
            NdisRawReadPortUlong(adapter->IoBase + R_TXSTS0 +
                                 (adapter->DirtyTxDesc * sizeof(ULONG)), &txStatus);

            if (!(txStatus & (R_TXS_STATOK | R_TXS_UNDERRUN | R_TXS_ABORTED)))
            {
                //
                // Not sent yet
                //
                break;
            }

            NDIS_DbgPrint(MAX_TRACE, ("Transmission for desc %d complete: 0x%x\n",
                                      adapter->DirtyTxDesc, txStatus));

            if (txStatus & R_TXS_STATOK)
            {
                adapter->TransmitOk++;
            }
            else
            {
                adapter->TransmitError++;
            }

            adapter->DirtyTxDesc++;
            adapter->DirtyTxDesc %= TX_DESC_COUNT;
            adapter->InterruptPending &= ~(R_I_TXOK | R_I_TXERR);
            adapter->TxFull = FALSE;
        }
    }

    //
    // Handle a good RX interrupt
    //
    if (adapter->InterruptPending & (R_I_RXOK | R_I_RXERR))
    {
        for (;;)
        {
            NdisRawReadPortUchar(adapter->IoBase + R_CMD, &command);
            if (command & R_CMD_RXEMPTY)
            {
                //
                // The buffer is empty
                //
                adapter->InterruptPending &= ~(R_I_RXOK | R_I_RXERR);
                break;
            }

            adapter->ReceiveOffset %= RECEIVE_BUFFER_SIZE;

            NDIS_DbgPrint(MAX_TRACE, ("Looking for a packet at offset 0x%x\n",
                            adapter->ReceiveOffset));
            nicHeader = (PPACKET_HEADER)(adapter->ReceiveBuffer + adapter->ReceiveOffset);
            if (!(nicHeader->Status & RSR_ROK))
            {
                //
                // Receive failed
                //
                NDIS_DbgPrint(MIN_TRACE, ("Receive failed: 0x%x\n", nicHeader->Status));

                if (nicHeader->Status & RSR_FAE)
                {
                    adapter->ReceiveAlignmentError++;
                }
                else if (nicHeader->Status & RSR_CRC)
                {
                    adapter->ReceiveCrcError++;
                }
                adapter->ReceiveError++;

                goto NextPacket;
            }

            NDIS_DbgPrint(MAX_TRACE, ("Indicating %d byte packet to NDIS\n",
                           nicHeader->PacketLength - RECV_CRC_LENGTH));

            ethHeader = (PETH_HEADER)(nicHeader + 1);
            NdisMEthIndicateReceive(adapter->MiniportAdapterHandle,
                                    NULL,
                                    (PVOID)(ethHeader),
                                    sizeof(ETH_HEADER),
                                    (PVOID)(ethHeader + 1),
                                    nicHeader->PacketLength - sizeof(ETH_HEADER) - RECV_CRC_LENGTH,
                                    nicHeader->PacketLength - sizeof(ETH_HEADER) - RECV_CRC_LENGTH);
            adapter->ReceiveOk++;

        NextPacket:
            adapter->ReceiveOffset += nicHeader->PacketLength + sizeof(PACKET_HEADER);
            adapter->ReceiveOffset = (adapter->ReceiveOffset + 3) & ~3;
            NdisRawWritePortUshort(adapter->IoBase + R_CAPR, adapter->ReceiveOffset - 0x10);

            if (adapter->InterruptPending & (R_I_RXOVRFLW | R_I_FIFOOVR))
            {
                //
                // We can only clear these interrupts once CAPR has been reset
                //
                NdisRawWritePortUshort(adapter->IoBase + R_IS, R_I_RXOVRFLW | R_I_FIFOOVR);
                adapter->InterruptPending &= ~(R_I_RXOVRFLW | R_I_FIFOOVR);
            }
        }

        NdisMEthIndicateReceiveComplete(adapter->MiniportAdapterHandle);
    }

    NdisDprReleaseSpinLock(&adapter->Lock);
}
