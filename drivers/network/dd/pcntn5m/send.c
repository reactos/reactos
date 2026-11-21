/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This file contains the code for putting a packet through the
    staged allocation for transmission.

    This is a process of

    1) Calculating the what would need to be done to the
    packet so that the packet can be transmitted on the hardware.

    2) Potentially allocating adapter buffers and copying user data
    to those buffers so that the packet data is transmitted under
    the hardware constraints.

    3) Allocating enough hardware ring entries so that the packet
    can be transmitted.

    4) Relinquish thos ring entries to the hardware.

Author:

    Anthony V. Ercolano (Tonye) 12-Sept-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    31-Jul-1992  R.D. Lanser:

       Removed PhysicalBuffersContained and UsedLanceBuffer field from
       _ADAPTER structure.  SeanSe says that the code related to this
       field was used for adevice that is no longer supported.  I removed
       the dependent code(or at least what was obvious) from 'send.c'.
       This old code was generating an erroneous ring buffer count on the
       MIPS R3000.  I did not test it on the MIPS R4000.  The problem goes
       away with the removal of the offending code.

--*/

#include <ndis.h>
#include <lancehrd.h>
#include <lancesft.h>


//
// Minimum packet size that a transport can send.  We subtract 4 bytes
// because we add a 4 byte CRC on the end.
//

#define MIN_SINGLE_BUFFER ((UINT)LANCE_SMALL_BUFFER_SIZE - 4)


//
// It will poke the lance hardware into noticing that there is a packet
// available for transmit.
//
// Note that there is the assumption that the register address
// port (RAP) is already set to zero.
//
#define PROD_TRANSMIT(A) \
    LANCE_WRITE_RDP( \
    A, \
    LANCE_CSR0_TRANSMIT_DEMAND | LANCE_CSR0_INTERRUPT_ENABLE \
    );

extern
NDIS_STATUS
LanceSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    )

/*++

Routine Description:

    The LanceSend request instructs a MAC to transmit a packet through
    the adapter onto the medium.

Arguments:

    Status - The status of the operation.

    MiniportAdapterContext - The context value set by the mini-port.

    Packet - A pointer to a descriptor for the packet that is to be
    transmitted.

Return Value:

    None.

--*/

{
    //
    // Pointer to the adapter.
    //
    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)MiniportAdapterContext;

    //
    // Pointer to the ring entry to be filled with buffer information.
    //
    PLANCE_TRANSMIT_ENTRY CurrentRingElement;

    //
    // Pointer to the ring to packet entry that records the info about
    // this packet.
    //
    PLANCE_RING_TO_PACKET RingToPacket;

    //
    // Length of the packet
    //
    ULONG TotalVirtualLength;

    //
    // Holds the adapter buffer index available for allocation.
    //
    INT LanceBuffersIndex;

    //
    // Points to a successfully allocated adapter buffer descriptor.
    //
    PLANCE_BUFFER_DESCRIPTOR BufferDescriptor;

    //
    // Simple iteration variable.
    //
    INT i;

    //
    // Size of Lance Buffer needed (1==Small, 2==Medium, 3==Large)
    //
    UCHAR BufferSize;

    //
    // If we successfully acquire some ring entries, this
    // is the index of the first one.
    //
    UINT RingIndex;

    //
    // Will point into the virtual address space addressed
    // by the adapter buffer if one was successfully allocated.
    //
    PCHAR CurrentDestination;

    //
    // Will point to the current source buffer.
    //
    PNDIS_BUFFER SourceBuffer;

    //
    // Points to the virtual address of the source buffers data.
    //
    PVOID SourceData;

    //
    // Will point to the number of bytes of data in the source
    // buffer.
    //
    UINT SourceLength;
    UINT BufferListNext;

    NDIS_STATUS Status;

#if LANCE_TRACE
    DbgPrint("In LanceSend\n");
#endif

    if (Adapter->HardwareFailure) {

        return(NDIS_STATUS_FAILURE);

    }

    LOG(IN_SEND);

    if (!Adapter->ResetInProgress) {

        if (Adapter->NumberOfAvailableRings == 0) {

            return(NDIS_STATUS_RESOURCES);

        }

        //
        // It is reasonable to do a quick check and fail if the packet
        // is larger than the maximum an ethernet can handle.
        //

        NdisQueryPacket(
            Packet,
            NULL,
            NULL,
            &SourceBuffer,
            &TotalVirtualLength
            );

        if ((!TotalVirtualLength) ||
            (TotalVirtualLength > LANCE_LARGE_BUFFER_SIZE)) {

            //
            // This packet can never be sent.  FAILURE completes it;
            // RESOURCES would make NDIS hold it at the head of the send
            // queue and offer it again forever, blocking everything
            // behind it.
            //
            return(NDIS_STATUS_FAILURE);

        } else {

            // Send data is copied into buffers allocated from adapter memory.

            if (TotalVirtualLength <= LANCE_SMALL_BUFFER_SIZE) {

                BufferSize = 1;

            } else if (TotalVirtualLength <= LANCE_MEDIUM_BUFFER_SIZE) {

                BufferSize = 2;

            } else {

                BufferSize = 3;

            }

            //
            // Find a buffer
            //
            for (
                i = BufferSize;
                i <= 3;
                i++
                ) {

                if ((LanceBuffersIndex = Adapter->LanceBufferListHeads[i]) != -1) {

                    BufferDescriptor = Adapter->LanceBuffers + LanceBuffersIndex;
                    Adapter->LanceBufferListHeads[i] = BufferDescriptor->Next;
                    BufferListNext = BufferDescriptor->Next;
                    break;

                }

            }

            if (LanceBuffersIndex == -1) {

                //
                // Nothing available for the packet.
                //

                return NDIS_STATUS_RESOURCES;

            }

            //
            // Save the list head index in the buffer descriptor
            // to permit easy deallocation later.
            //
            BufferDescriptor->Next = i;

            //
            // Now Acquire the ring
            //
            RingIndex = (UINT)(Adapter->AllocateableRing - Adapter->TransmitRing);

            //
            // Store the info
            //
            CurrentRingElement = Adapter->AllocateableRing;

            //
            // NOTE NOTE NOTE NOTE NOTE NOTE
            //
            // We can do the next calculation because we know that the number
            // or ring entries is a power of two!
            //

            Adapter->AllocateableRing = Adapter->TransmitRing +
                    (((RingIndex) + 1) &
                     (Adapter->NumberOfTransmitRings-1));

            Adapter->NumberOfAvailableRings--;

            //
            // Copy into buffer
            //

            CurrentDestination = BufferDescriptor->VirtualLanceBuffer;

            while ( SourceBuffer != NULL ) {

                SourceLength = 0;
                NdisQueryBufferSafe(
                        SourceBuffer,
                        &SourceData,
                        &SourceLength,
                        NormalPagePriority
                        );

                if ((SourceData == NULL) && (SourceLength != 0))
                {
                    //
                    // Mapping failed (low on system PTEs).  Give the buffer
                    // and ring back and fail this one packet.  RESOURCES
                    // would make NDIS hold it until a send completes, and
                    // with nothing else in the ring none ever would.
                    //
                    BufferDescriptor->Next = BufferListNext;
                    Adapter->LanceBufferListHeads[i] = LanceBuffersIndex;
                    Adapter->AllocateableRing = CurrentRingElement;
                    Adapter->NumberOfAvailableRings++;
                    return NDIS_STATUS_FAILURE;
                }

                if (SourceLength != 0)
                {
                    LANCE_MOVE_MEMORY_TO_HARDWARE(
                        CurrentDestination,
                        SourceData,
                        SourceLength
                        );
                }

                CurrentDestination = (PCHAR)CurrentDestination + SourceLength;

                NdisGetNextBuffer(
                        SourceBuffer,
                        &SourceBuffer
                        );

            }

            //
            // If the packet is less then the minimum size then we
            // need to zero out the rest of the packet.
            //

            if (TotalVirtualLength < MIN_SINGLE_BUFFER) {

                LANCE_ZERO_MEMORY_FOR_HARDWARE(
                    CurrentDestination,
                    MIN_SINGLE_BUFFER - TotalVirtualLength
                    );

                BufferDescriptor->DataLength = MIN_SINGLE_BUFFER;

            } else {

                BufferDescriptor->DataLength = TotalVirtualLength;

            }

            //
            // Get the position for mapping ring entries to packets.
            // We record the owning packet information in the ring packet packet
            // structure.
            //
            RingToPacket = Adapter->RingToPacket + RingIndex;
            RingToPacket->OwningPacket = Packet;
            RingToPacket->LanceBuffersIndex = LanceBuffersIndex;
            RingToPacket->RingIndex = RingIndex;

            //
            // Make sure that the ring descriptor is clean.
            //

            LANCE_ZERO_MEMORY_FOR_HARDWARE((PUCHAR)CurrentRingElement,
                                           sizeof(LANCE_TRANSMIT_ENTRY)
                                          );

            LANCE_SET_TRANSMIT_BUFFER_LENGTH(
                CurrentRingElement,
                BufferDescriptor->DataLength
                );


            LANCE_SET_TRANSMIT_BUFFER_ADDRESS(
                    Adapter,
                    CurrentRingElement,
                    BufferDescriptor->VirtualLanceBuffer
                    );

            LOG(TRANSMIT);

            //
            // We update the ring ownership of the last packet under
            // the protection of the lock so that the uncommitted packet
            // pointer can be updated before the transmit post processing
            // can examine it.
            //

            LANCE_SET_RING_BITS(
              CurrentRingElement->TransmitSummaryBits,
              LANCE_TRANSMIT_START_OF_PACKET |
              LANCE_TRANSMIT_OWNED_BY_CHIP |
              LANCE_TRANSMIT_END_OF_PACKET
              );


            if (RingIndex == (Adapter->NumberOfTransmitRings-1)) {

                Adapter->FirstUncommittedRing = Adapter->TransmitRing ;

            } else {

                Adapter->FirstUncommittedRing = Adapter->TransmitRing + RingIndex + 1;

            }

            //
            // Prod the chip into checking for packets to send.
            //

            PROD_TRANSMIT(Adapter);

            Status = NDIS_STATUS_SUCCESS;

        }

    } else if (Adapter->ResetPending) {

        Status = NDIS_STATUS_RESET_IN_PROGRESS;

    } else {

        //
        // Have the upper layer try again later
        //
        Status = NDIS_STATUS_RESOURCES;

    }

    LOG(OUT_SEND);

    LANCE_DO_DEFERRED(Adapter);

#if LANCE_TRACE
    DbgPrint("Out LanceSend\n");
#endif

    return Status;
}
