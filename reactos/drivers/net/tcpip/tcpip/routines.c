/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/routines.c
 * PURPOSE:     Common routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <routines.h>
#include <pool.h>


UINT RandomNumber = 0x12345678;


UINT Random(
    VOID)
/*
 * FUNCTION: Returns a pseudo random number
 * RETURNS:
 *     Pseudo random number
 */
{
    RandomNumber ^= 0x78563412;

    return RandomNumber;
}


__inline INT SkipToOffset(
    PNDIS_BUFFER Buffer,
    UINT Offset,
    PUCHAR *Data,
    PUINT Size)
/*
 * FUNCTION: Skip Offset bytes into a buffer chain
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer
 *     Offset = Number of bytes to skip
 *     Data   = Address of a pointer that on return will contain the
 *              address of the offset in the buffer
 *     Size   = Address of a pointer that on return will contain the
 *              size of the destination buffer
 * RETURNS:
 *     Offset into buffer, -1 if buffer chain was smaller than Offset bytes
 * NOTES:
 *     Buffer may be NULL
 */
{
    for (;;) {

        if (!Buffer)
            return -1;

        NdisQueryBuffer(Buffer, Data, Size);

        if (Offset < *Size) {
            ((ULONG_PTR)*Data) += Offset;
            *Size              -= Offset;
            break;
        }

        Offset -= *Size;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    return Offset;
}


UINT CopyBufferToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PUCHAR SrcData,
    UINT Length)
/*
 * FUNCTION: Copies data from a buffer to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer 
 *     DstOffset = Destination start offset
 *     SrcData   = Pointer to source buffer
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the destination
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, DstSize;
    PUCHAR DstData;

    TI_DbgPrint(DEBUG_BUFFER, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcData (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcData, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(DstSize, Length);

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied        += BytesToCopy;
        (ULONG_PTR)SrcData += BytesToCopy;

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            /* No more bytes in desination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, &DstData, &DstSize);
        }
    }

    return BytesCopied;
}


UINT CopyBufferChainToBuffer(
    PUCHAR DstData,
    PNDIS_BUFFER SrcBuffer,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS buffer chain to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcBuffer = Pointer to source NDIS buffer
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    UINT BytesCopied, BytesToCopy, SrcSize;
    PUCHAR SrcData;

    TI_DbgPrint(DEBUG_BUFFER, ("DstData 0x%X  SrcBuffer 0x%X  SrcOffset 0x%X  Length %d\n",DstData,SrcBuffer, SrcOffset, Length));
    
    /* Skip SrcOffset bytes in the source buffer chain */
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(SrcSize, Length);

        TI_DbgPrint(DEBUG_BUFFER, ("Copying (%d) bytes from 0x%X to 0x%X\n", BytesToCopy, SrcData, DstData));

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied        += BytesToCopy;
        (ULONG_PTR)DstData += BytesToCopy;

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        SrcSize -= BytesToCopy;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        }
    }

    return BytesCopied;
}


UINT CopyPacketToBuffer(
    PUCHAR DstData,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to a buffer
 * ARGUMENTS:
 *     DstData   = Pointer to destination buffer
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source
 *     buffer size
 */
{
    PNDIS_BUFFER FirstBuffer;
    PVOID Address;
    UINT FirstLength;
    UINT TotalLength;

    TI_DbgPrint(DEBUG_BUFFER, ("DstData (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstData, SrcPacket, SrcOffset, Length));

    NdisGetFirstBufferFromPacket(SrcPacket,
                                 &FirstBuffer,
                                 &Address,
                                 &FirstLength,
                                 &TotalLength);

    return CopyBufferChainToBuffer(DstData, FirstBuffer, SrcOffset, Length);
}


UINT CopyPacketToBufferChain(
    PNDIS_BUFFER DstBuffer,
    UINT DstOffset,
    PNDIS_PACKET SrcPacket,
    UINT SrcOffset,
    UINT Length)
/*
 * FUNCTION: Copies data from an NDIS packet to an NDIS buffer chain
 * ARGUMENTS:
 *     DstBuffer = Pointer to destination NDIS buffer
 *     DstOffset = Destination start offset
 *     SrcPacket = Pointer to source NDIS packet
 *     SrcOffset = Source start offset
 *     Length    = Number of bytes to copy
 * RETURNS:
 *     Number of bytes copied to destination buffer
 * NOTES:
 *     The number of bytes copied may be limited by the source and
 *     destination buffer sizes
 */
{
    PNDIS_BUFFER SrcBuffer;
    PUCHAR DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    TI_DbgPrint(DEBUG_BUFFER, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcPacket, SrcOffset, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    NdisQueryBuffer(DstBuffer, &DstData, &DstSize);
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == -1)
        return 0;

    /* Skip SrcOffset bytes in the source packet */
    NdisGetFirstBufferFromPacket(SrcPacket, &SrcBuffer, &SrcData, &SrcSize, &Total);
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == -1)
        return 0;

    /* Copy the data */
    for (Total = 0;;) {
        /* Find out how many bytes we can copy at one time */
        if (Length < SrcSize)
            Count = Length;
        else
            Count = SrcSize;
        if (DstSize < Count)
            Count = DstSize;

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, Count);

        Total  += Count;
        Length -= Count;
        if (Length == 0)
            break;

        DstSize -= Count;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, &DstData, &DstSize);
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, &SrcData, &SrcSize);
        }
    }

    return Total;
}


VOID FreeNdisPacket(
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Frees an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet to be freed
 */
{
    PNDIS_BUFFER Buffer, NextBuffer;

    TI_DbgPrint(DEBUG_BUFFER, ("Packet (0x%X)\n", Packet));

    /* Free all the buffers in the packet first */
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        PVOID Data;
        UINT Length;

        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Data, &Length);
        NdisFreeBuffer(Buffer);
        ExFreePool(Data);
    }

    /* Finally free the NDIS packet discriptor */
    NdisFreePacket(Packet);
}


PVOID AdjustPacket(
    PNDIS_PACKET Packet,
    UINT Available,
    UINT Needed)
/*
 * FUNCTION: Adjusts the amount of unused space at the beginning of the packet
 * ARGUMENTS:
 *     Packet    = Pointer to packet
 *     Available = Number of bytes available at start of first buffer
 *     Needed    = Number of bytes needed for the header
 * RETURNS:
 *     Pointer to start of packet
 */
{
    PNDIS_BUFFER NdisBuffer;
    INT Adjust;

    TI_DbgPrint(DEBUG_BUFFER, ("Available = %d, Needed = %d.\n", Available, Needed));

    Adjust = Available - Needed;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    /* If Adjust is zero there is no need to adjust this packet as
       there is no additional space at start the of first buffer */
    if (Adjust != 0) {
#if 0
        (ULONG_PTR)(NdisBuffer->MappedSystemVa) += Adjust;
        (ULONG_PTR)(NdisBuffer->StartVa)        += Adjust;
        NdisBuffer->ByteCount                   -= Adjust;
#else
        (ULONG_PTR)(NdisBuffer->MappedSystemVa) += Adjust;
        NdisBuffer->ByteOffset                  += Adjust;
        NdisBuffer->ByteCount                   -= Adjust;
#endif
    }

    return NdisBuffer->MappedSystemVa;
}


UINT ResizePacket(
    PNDIS_PACKET Packet,
    UINT Size)
/*
 * FUNCTION: Resizes an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to packet
 *     Size   = Number of bytes in first buffer
 * RETURNS:
 *     Previous size of first buffer
 */
{
    PNDIS_BUFFER NdisBuffer;
    UINT OldSize;

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    OldSize = NdisBuffer->ByteCount;

    if (Size != OldSize)
        NdisBuffer->ByteCount = Size;

    return OldSize;
}

#ifdef DBG
VOID DisplayIPPacket(
    PIP_PACKET IPPacket)
{
    UINT i;
    PCHAR p;
    UINT Length;
    PNDIS_BUFFER Buffer;
    PNDIS_BUFFER NextBuffer;

    if ((DebugTraceLevel & MAX_TRACE) == 0)
        return;

    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Cannot display null packet.\n"));
        return;
    }

    TI_DbgPrint(MIN_TRACE, ("Header buffer is at (0x%X).\n", IPPacket->Header));
    TI_DbgPrint(MIN_TRACE, ("Header size is (%d).\n", IPPacket->HeaderSize));
    TI_DbgPrint(MIN_TRACE, ("TotalSize (%d).\n", IPPacket->TotalSize));
    TI_DbgPrint(MIN_TRACE, ("ContigSize (%d).\n", IPPacket->ContigSize));
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, (PVOID)&p, &Length);

        for (i = 0; i < Length; i++) {
            if (i % 16 == 0)
                DbgPrint("\n");
            DbgPrint("%02X ", (p[i]) & 0xFF);
        }
        DbgPrint("\n");
    }
}
#endif /* DBG */

/* EOF */
