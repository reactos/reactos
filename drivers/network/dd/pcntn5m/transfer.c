/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    transfer.c

Abstract:

    This file contains the code to implement the MacTransferData
    API for the NDIS miniport interface.

Author:

    Anthony V. Ercolano (Tonye) 12-Sept-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include <ndis.h>
#include <lancehrd.h>
#include <lancesft.h>


extern
NDIS_STATUS
LanceTransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    )

/*++

Routine Description:

    A protocol calls the LanceTransferData request (indirectly via
    NdisTransferData) from within its Receive event handler
    to instruct the MAC to copy the contents of the received packet
    a specified paqcket buffer.

Arguments:

    Status - Status of the operation.

    MiniportAdapterContext - The context value set by the Miniport.

    MiniportReceiveContext - The context value passed by the MAC on its call
    to NdisMEthIndicateReceive.

    ByteOffset - An unsigned integer specifying the offset within the
    received packet at which the copy is to begin.  If the entire packet
    is to be copied, ByteOffset must be zero.

    BytesToTransfer - An unsigned integer specifying the number of bytes
    to copy.  It is legal to transfer zero bytes; this has no effect.  If
    the sum of ByteOffset and BytesToTransfer is greater than the size
    of the received packet, then the remainder of the packet (starting from
    ByteOffset) is transferred, and the trailing portion of the receive
    buffer is not modified.

    Packet - A pointer to a descriptor for the packet storage into which
    the MAC is to copy the received packet.

    BytesTransfered - A pointer to an unsigned integer.  The MAC writes
    the actual number of bytes transferred into this location.  This value
    is not valid if the return status is STATUS_PENDING.

Return Value:

    The function value is the status of the operation.


--*/

{

    PLANCE_ADAPTER Adapter = (PLANCE_ADAPTER)(MiniportAdapterContext);
    //
    // Keep a local for the number of receive ring entries so
    // that we aren't always accessing through the adapter.
    //
    const UINT TopReceiveIndex = Adapter->NumberOfReceiveRings - 1;

    //
    // Used for only a short time to extract the context
    // information from the parameter.
    //
    LANCE_RECEIVE_CONTEXT C;

    //
    // Holds the first and last index of the first and last
    // receive ring descriptors that hold the current packet.
    //
    UINT FirstBuffer;
    UINT LastBuffer;

    //
    // Pointer to the ring descriptor for the current buffer.
    //
    PLANCE_RECEIVE_ENTRY CurrentEntry;

    //
    // Holds the count of the number of ndis buffers comprising
    // the destination packet.
    //
    UINT DestinationBufferCount;

    //
    // Points to the buffer into which we are putting data.
    //
    PNDIS_BUFFER DestinationCurrentBuffer;

    //
    // Holds the virtual address of the current destination
    // buffer.
    //
    PVOID DestinationVirtualAddress;

    //
    // Holds the virtual address of the current source buffer.
    //
    PVOID SourceVirtualAddress;

    //
    // Holds the length of the current destination buffer.
    //
    UINT DestinationCurrentLength;

    //
    // Holds the length of the current source buffer.
    //
    UINT SourceCurrentLength;

    //
    // Keep a local variable of BytesTransferred so we aren't
    // referencing through a pointer.
    //
    UINT LocalBytesTransferred = 0;

    //
    // FAILURE if a destination buffer could not be mapped, so the
    // protocol doesn't take a short copy for a good packet.
    //
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    //
    // Bytes of the packet held in the receive buffers before the current
    // one.  The last buffer holds the message size minus this.
    //
    UINT SourceBytesBefore = 0;

    //
    // Index in the ring of the current receive ring descriptor.
    //
    UINT CurrentSourceIndex;

    ASSERT(!Adapter->ResetInitStarted);

    ByteOffset += LANCE_HEADER_SIZE;

    //
    // Take care of boundary condition of zero length copy.
    //

    *BytesTransferred = 0;

    ASSERT(sizeof(UINT) >= 2);
    ASSERT(sizeof(ULONG_PTR) == sizeof(NDIS_HANDLE));

    C.WholeThing = (ULONG_PTR)MiniportReceiveContext;
    FirstBuffer = (UINT)C.INFO.FirstBuffer;
    LastBuffer = (UINT)C.INFO.LastBuffer;

    //
    // Get the first buffer of the destination.
    //

    NdisQueryPacket(
                Packet,
                NULL,
                &DestinationBufferCount,
                &DestinationCurrentBuffer,
                NULL
                );

    //
    // Could have a null packet.
    //

    if (DestinationBufferCount && DestinationCurrentBuffer) {

        DestinationCurrentLength = 0;
        NdisQueryBufferSafe(
                    DestinationCurrentBuffer,
                    &DestinationVirtualAddress,
                    &DestinationCurrentLength,
                    NormalPagePriority
                    );

        if ((DestinationVirtualAddress == NULL) &&
            (DestinationCurrentLength != 0)) {
            Status = NDIS_STATUS_FAILURE;
            goto TransferDataComplete;
        }

        //
        // Get the information for the first buffer of the source.
        //

        SourceVirtualAddress = Adapter->ReceiveVAs[FirstBuffer];
        CurrentEntry = Adapter->ReceiveRing + FirstBuffer;
        CurrentSourceIndex = FirstBuffer;

        if (CurrentSourceIndex == LastBuffer) {

            //
            // The last buffer might only be partially filled with
            // transmitted data.  There is a field in the last
            // ring entry that has the total packet data length.
            //
            LANCE_GET_MESSAGE_SIZE(CurrentEntry, SourceCurrentLength);

        } else {

            SourceCurrentLength = Adapter->SizeOfReceiveBuffer;

        }

        while (LocalBytesTransferred < BytesToTransfer) {

            //
            // Check to see whether we've exhausted the current
            // destination buffer.  If so, move onto the next one.
            //

            if (!DestinationCurrentLength) {

                NdisGetNextBuffer(
                                DestinationCurrentBuffer,
                                &DestinationCurrentBuffer
                                );

                if (!DestinationCurrentBuffer) {

                    //
                    // We've reached the end of the packet.  We
                    // return with what we've done so far. (Which
                    // must be shorter than requested.)
                    //

                    break;

                }

                DestinationCurrentLength = 0;
                NdisQueryBufferSafe(
                                DestinationCurrentBuffer,
                                &DestinationVirtualAddress,
                                &DestinationCurrentLength,
                                NormalPagePriority
                                );
                if ((DestinationVirtualAddress == NULL) &&
                    (DestinationCurrentLength != 0)) {
                    Status = NDIS_STATUS_FAILURE;
                    break;
                }
                continue;

            }


            //
            // Check to see whether we've exhausted the current
            // source buffer.  If so, move onto the next one.
            //

            if (!SourceCurrentLength) {

                if (CurrentSourceIndex == LastBuffer) {

                    //
                    // We've reached the end of the packet.  We
                    // return with what we've done so far. (Which
                    // must be shorter than requested.)
                    //

                    break;

                }

                if (CurrentSourceIndex == TopReceiveIndex) {

                    CurrentSourceIndex = 0;
                    CurrentEntry = Adapter->ReceiveRing;

                } else {

                    CurrentSourceIndex++;
                    CurrentEntry++;

                }

                SourceBytesBefore += Adapter->SizeOfReceiveBuffer;

                if (CurrentSourceIndex == LastBuffer) {

                    //
                    // The last buffer might only be partially
                    // filled with transmitted data.  The last ring
                    // entry has the total packet length; this buffer
                    // holds what the earlier, full buffers didn't.
                    // (Subtracting the bytes copied so far, as before,
                    // ignored the header and the starting offset.)
                    //
                    LANCE_GET_MESSAGE_SIZE(CurrentEntry, SourceCurrentLength);
                    SourceCurrentLength =
                        (SourceCurrentLength > SourceBytesBefore) ?
                            (SourceCurrentLength - SourceBytesBefore) : 0;

                } else {

                    SourceCurrentLength =
                         Adapter->SizeOfReceiveBuffer;

                }

                SourceVirtualAddress =
                                Adapter->ReceiveVAs[CurrentSourceIndex];
                continue;

            }

            //
            // Try to get us up to the point to start the copy.
            //

            if (ByteOffset) {

                if (ByteOffset > SourceCurrentLength) {

                    //
                    // What we want isn't in this buffer.
                    //

                    ByteOffset -= SourceCurrentLength;
                    SourceCurrentLength = 0;
                    continue;

                } else {

                    SourceVirtualAddress =
                                    (PCHAR)SourceVirtualAddress + ByteOffset;
                    SourceCurrentLength -= ByteOffset;
                    ByteOffset = 0;

                }

            }

            //
            // Copy the data.
            //

            {

                //
                // Holds the amount of data to move.
                //
                UINT AmountToMove;

                //
                // Holds the amount desired remaining.
                //
                UINT Remaining = BytesToTransfer
                                             - LocalBytesTransferred;

                AmountToMove =
                          ((SourceCurrentLength <= DestinationCurrentLength)?
                           (SourceCurrentLength):(DestinationCurrentLength));

                AmountToMove = ((Remaining < AmountToMove)?
                                            (Remaining):(AmountToMove));

                LANCE_MOVE_HARDWARE_TO_MEMORY(
                                DestinationVirtualAddress,
                                SourceVirtualAddress,
                                AmountToMove
                                );

                DestinationVirtualAddress =
                              (PCHAR)DestinationVirtualAddress + AmountToMove;
                SourceVirtualAddress =
                                (PCHAR)SourceVirtualAddress + AmountToMove;

                LocalBytesTransferred += AmountToMove;
                SourceCurrentLength -= AmountToMove;
                DestinationCurrentLength -= AmountToMove;

            }

        }

        *BytesTransferred = LocalBytesTransferred;

    }

TransferDataComplete:
    LANCE_DO_DEFERRED(Adapter);

    return Status;
}
