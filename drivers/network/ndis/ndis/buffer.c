/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/buffer.c
 * PURPOSE:     Buffer management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include <ndissys.h>

FORCEINLINE
ULONG
SkipToOffset(
    IN PNDIS_BUFFER Buffer,
    IN UINT Offset,
    IN OUT PUCHAR *Data,
    IN OUT PUINT Size)
/*
 * FUNCTION: Skips Offset bytes into a buffer chain
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
            return 0xFFFFFFFF;

        NdisQueryBuffer(Buffer, (PVOID)Data, Size);

        if (Offset < *Size) {
            *Data  = (PUCHAR) ((ULONG_PTR) *Data + Offset);
            *Size -= Offset;
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

    NDIS_DbgPrint(MAX_TRACE, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcData (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcData, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == 0xFFFFFFFF)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(DstSize, Length);

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        SrcData      = (PUCHAR) ((ULONG_PTR) SrcData + BytesToCopy);

        Length -= BytesToCopy;
        if (Length == 0)
            break;

        DstSize -= BytesToCopy;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
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

    NDIS_DbgPrint(MAX_TRACE, ("DstData 0x%X  SrcBuffer 0x%X  SrcOffset 0x%X  Length %d\n",DstData,SrcBuffer, SrcOffset, Length));

    /* Skip SrcOffset bytes in the source buffer chain */
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == 0xFFFFFFFF)
        return 0;

    /* Start copying the data */
    BytesCopied = 0;
    for (;;) {
        BytesToCopy = MIN(SrcSize, Length);

        NDIS_DbgPrint(MAX_TRACE, ("Copying (%d) bytes from 0x%X to 0x%X\n", BytesToCopy, SrcData, DstData));

        RtlCopyMemory((PVOID)DstData, (PVOID)SrcData, BytesToCopy);
        BytesCopied += BytesToCopy;
        DstData      = (PUCHAR)((ULONG_PTR) DstData + BytesToCopy);

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

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
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

    NDIS_DbgPrint(MAX_TRACE, ("DstData (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstData, SrcPacket, SrcOffset, Length));

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

    NDIS_DbgPrint(MAX_TRACE, ("DstBuffer (0x%X)  DstOffset (0x%X)  SrcPacket (0x%X)  SrcOffset (0x%X)  Length (%d)\n", DstBuffer, DstOffset, SrcPacket, SrcOffset, Length));

    /* Skip DstOffset bytes in the destination buffer chain */
    NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
    if (SkipToOffset(DstBuffer, DstOffset, &DstData, &DstSize) == 0xFFFFFFFF)
        return 0;
    /* Skip SrcOffset bytes in the source packet */
    NdisGetFirstBufferFromPacket(SrcPacket, &SrcBuffer, (PVOID*)&SrcData, &SrcSize, &Total);
    if (SkipToOffset(SrcBuffer, SrcOffset, &SrcData, &SrcSize) == 0xFFFFFFFF)
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

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    return Total;
}


/*
 * @implemented
 */
#undef NdisAdjustBufferLength
VOID
EXPORT
NdisAdjustBufferLength(
    IN PNDIS_BUFFER Buffer,
    IN UINT         Length)
/*
 * FUNCTION: Modifies the length of an NDIS buffer
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer descriptor
 *     Length = New size of buffer
 */
{
    Buffer->ByteCount = Length;
}


/*
 * @implemented
 */
#undef NDIS_BUFFER_TO_SPAN_PAGES
ULONG
EXPORT
NDIS_BUFFER_TO_SPAN_PAGES(
    IN  PNDIS_BUFFER    Buffer)
/*
 * FUNCTION: Determines how many physical pages a buffer is made of
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer descriptor
 */
{
    if (MmGetMdlByteCount(Buffer) == 0)
        return 1;

    return ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            MmGetMdlVirtualAddress(Buffer),
            MmGetMdlByteCount(Buffer));
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAllocateBuffer(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_BUFFER    * Buffer,
    IN  NDIS_HANDLE     PoolHandle,
    IN  PVOID           VirtualAddress,
    IN  UINT            Length)
/*
 * FUNCTION: Allocates an NDIS buffer descriptor
 * ARGUMENTS:
 *     Status         = Address of buffer for status
 *     Buffer         = Address of buffer for NDIS buffer descriptor
 *     PoolHandle     = Handle returned by NdisAllocateBufferPool
 *     VirtualAddress = Pointer to virtual address of data buffer
 *     Length         = Number of bytes in data buffer
 */
{
    ASSERT(VirtualAddress != NULL);
    ASSERT(Length > 0);

    *Buffer = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, NULL);
    if (*Buffer != NULL) {
        MmBuildMdlForNonPagedPool(*Buffer);
        (*Buffer)->Next = NULL;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        NDIS_DbgPrint(MIN_TRACE, ("IoAllocateMdl failed (%x, %lx)\n", VirtualAddress, Length));
        *Status = NDIS_STATUS_FAILURE;
    }
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAllocateBufferPool(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors)
/*
 * FUNCTION: Allocates storage for an NDIS buffer pool
 * ARGUMENTS:
 *     Status              = Address of buffer for status
 *     PoolHandle          = Address of buffer for pool handle
 *     NumberOfDescriptors = Size of buffer pool in number of descriptors
 */
{
    *Status = NDIS_STATUS_SUCCESS;
    *PoolHandle = 0;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAllocatePacket(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    * Packet,
    IN  NDIS_HANDLE     PoolHandle)
/*
 * FUNCTION: Allocates an NDIS packet descriptor
 * ARGUMENTS:
 *     Status     = Address of buffer for status
 *     Packet     = Address of buffer for packet descriptor
 *     PoolHandle = Handle returned by NdisAllocatePacketPool
 */
{
    PNDISI_PACKET_POOL Pool = (PNDISI_PACKET_POOL)PoolHandle;

    KeAcquireSpinLock(&Pool->SpinLock.SpinLock, &Pool->SpinLock.OldIrql);
    NdisDprAllocatePacketNonInterlocked(Status,
                                        Packet,
                                        PoolHandle);
    KeReleaseSpinLock(&Pool->SpinLock.SpinLock, Pool->SpinLock.OldIrql);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAllocatePacketPool(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors,
    IN  UINT            ProtocolReservedLength)
/*
 * FUNCTION: Allocates storage for an NDIS packet pool
 * ARGUMENTS:
 *     Status                 = Address of buffer for status
 *     PoolHandle             = Address of buffer for pool handle
 *     NumberOfDescriptors    = Size of packet pool in number of descriptors
 *     ProtocolReservedLength = Size of protocol reserved area in bytes
 */
{
    NdisAllocatePacketPoolEx(
        Status,
        PoolHandle,
        NumberOfDescriptors,
        0,
        ProtocolReservedLength);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisAllocatePacketPoolEx(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    PoolHandle,
    IN  UINT            NumberOfDescriptors,
    IN  UINT            NumberOfOverflowDescriptors,
    IN  UINT            ProtocolReservedLength)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PNDISI_PACKET_POOL Pool;
    UINT Size, Length, i;
    PNDIS_PACKET Packet, NextPacket;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  PoolHandle (0x%X)  "
        "NumberOfDescriptors (%d)  ProtocolReservedLength (%d).\n",
        Status, PoolHandle, NumberOfDescriptors, ProtocolReservedLength));

    *PoolHandle = NULL;

    if (NumberOfDescriptors > 0xffff)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Invalid number of descriptors (%lx)\n", NumberOfDescriptors))
        *Status = NDIS_STATUS_RESOURCES;
    }
    else
    {
        NumberOfDescriptors += NumberOfOverflowDescriptors;
        if (NumberOfDescriptors > 0xffff)
        {
            NDIS_DbgPrint(MIN_TRACE, ("Total number of descriptors > 0xffff (%lx)\n", NumberOfDescriptors));
            NumberOfDescriptors = 0xffff;
        }

        Length = sizeof(NDIS_PACKET) + sizeof(NDIS_PACKET_OOB_DATA) +
                 sizeof(NDIS_PACKET_EXTENSION) + ProtocolReservedLength;
        Size   = sizeof(NDISI_PACKET_POOL) + Length * NumberOfDescriptors;

        Pool   = ExAllocatePool(NonPagedPool, Size);
        if (Pool)
        {
            KeInitializeSpinLock(&Pool->SpinLock.SpinLock);
            Pool->PacketLength = Length;

            if (NumberOfDescriptors > 0)
            {
                Packet         = (PNDIS_PACKET)&Pool->Buffer;
                Pool->FreeList = Packet;

                NextPacket = (PNDIS_PACKET)((ULONG_PTR)Packet + Length);
                for (i = 1; i < NumberOfDescriptors; i++)
                {
                    Packet->Reserved[0]  = (ULONG_PTR)NextPacket;
                    Packet               = NextPacket;
                    NextPacket           = (PNDIS_PACKET)((ULONG_PTR)Packet + Length);
                }
                Packet->Reserved[0] = 0;
            }
            else {
                NDIS_DbgPrint(MIN_TRACE, ("Attempted to allocate a packet pool with 0 descriptors\n"));
                Pool->FreeList = NULL;
            }

            *Status     = NDIS_STATUS_SUCCESS;
            *PoolHandle = (PNDIS_HANDLE)Pool;
        } else {
            *Status = NDIS_STATUS_RESOURCES;
        }
    }
}


/*
 * @implemented
 */
#undef NdisBufferLength
ULONG
EXPORT
NdisBufferLength(
    IN  PNDIS_BUFFER    Buffer)
/*
 * FUNCTION: Modifies the length of an NDIS buffer
 * ARGUMENTS:
 *     Buffer = Pointer to NDIS buffer descriptor
 *     Length = New size of buffer
 * NOTES:
 *    NDIS 5.0
 * RETURNS:
 *     Length of NDIS buffer
 */
{
    return MmGetMdlByteCount(Buffer);
}


/*
 * @implemented
 */
#undef NdisBufferVirtualAddress
PVOID
EXPORT
NdisBufferVirtualAddress(
    IN  PNDIS_BUFFER    Buffer)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    return MmGetSystemAddressForMdl(Buffer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisCopyFromPacketToPacket(
    IN  PNDIS_PACKET    Destination,
    IN  UINT            DestinationOffset,
    IN  UINT            BytesToCopy,
    IN  PNDIS_PACKET    Source,
    IN  UINT            SourceOffset,
    OUT PUINT           BytesCopied)
/*
 * FUNCTION: Copies data from one packet to another
 * ARGUMENTS:
 *     Destination       = Pointer to packet to copy data to
 *     DestinationOffset = Offset in destination packet to copy data to
 *     BytesToCopy       = Number of bytes to copy
 *     Source            = Pointer to packet descriptor to copy from
 *     SourceOffset      = Offset in source packet to start copying from
 *     BytesCopied       = Address of buffer to place number of bytes copied
 */
{
    PNDIS_BUFFER SrcBuffer;
    PNDIS_BUFFER DstBuffer;
    PUCHAR DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    *BytesCopied = 0;

    /* Skip DestinationOffset bytes in the destination packet */
    NdisGetFirstBufferFromPacket(Destination, &DstBuffer, (PVOID*)&DstData, &DstSize, &Total);
    if (SkipToOffset(DstBuffer, DestinationOffset, &DstData, &DstSize) == 0xFFFFFFFF)
        return;

    /* Skip SourceOffset bytes in the source packet */
    NdisGetFirstBufferFromPacket(Source, &SrcBuffer, (PVOID*)&SrcData, &SrcSize, &Total);
    if (SkipToOffset(SrcBuffer, SourceOffset, &SrcData, &SrcSize) == 0xFFFFFFFF)
        return;

    /* Copy the data */
    for (Total = 0;;) {
        /* Find out how many bytes we can copy at one time */
        if (BytesToCopy < SrcSize)
            Count = BytesToCopy;
        else
            Count = SrcSize;
        if (DstSize < Count)
            Count = DstSize;

        RtlCopyMemory(DstData, SrcData, Count);

        Total       += Count;
        BytesToCopy -= Count;
        if (BytesToCopy == 0)
            break;

        DstSize -= Count;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBuffer(DstBuffer, (PVOID)&DstData, &DstSize);
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }
    }

    *BytesCopied = Total;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisDprAllocatePacket(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    *Packet,
    IN  NDIS_HANDLE     PoolHandle)
/*
 * FUNCTION: Allocates a packet at IRQL DISPATCH_LEVEL
 * ARGUMENTS:
 *     Status     = Address of buffer to place status of operation
 *     Packet     = Address of buffer to place a pointer to a packet descriptor
 *     PoolHandle = Handle returned by NdisAllocatePacketPool
 */
{
    PNDISI_PACKET_POOL Pool = (PNDISI_PACKET_POOL)PoolHandle;

    KeAcquireSpinLockAtDpcLevel(&Pool->SpinLock.SpinLock);
    NdisDprAllocatePacketNonInterlocked(Status,
                                        Packet,
                                        PoolHandle);
    KeReleaseSpinLockFromDpcLevel(&Pool->SpinLock.SpinLock);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisDprAllocatePacketNonInterlocked(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_PACKET    *Packet,
    IN NDIS_HANDLE      PoolHandle)
/*
 * FUNCTION: Allocates a packet at IRQL DISPATCH_LEVEL (w/o synchronization)
 * ARGUMENTS:
 *     Status     = Address of buffer to place status of operation
 *     Packet     = Address of buffer to place a pointer to a packet descriptor
 *     PoolHandle = Handle returned by NdisAllocatePacketPool
 */
{
    PNDIS_PACKET Temp;
    PNDISI_PACKET_POOL Pool = (PNDISI_PACKET_POOL)PoolHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  Packet (0x%X)  PoolHandle (0x%X).\n",
        Status, Packet, PoolHandle));

    *Packet = NULL;

    if (Pool == NULL)
    {
        *Status = NDIS_STATUS_FAILURE;
        NDIS_DbgPrint(MIN_TRACE, ("Called passed a bad pool handle\n"));
        return;
    }

    if (Pool->FreeList) {
        Temp           = Pool->FreeList;
        Pool->FreeList = (PNDIS_PACKET)Temp->Reserved[0];

        RtlZeroMemory(Temp, Pool->PacketLength);
        Temp->Private.Pool = Pool;
        Temp->Private.ValidCounts = TRUE;
        Temp->Private.NdisPacketFlags = fPACKET_ALLOCATED_BY_NDIS;
        Temp->Private.NdisPacketOobOffset = Pool->PacketLength -
                                            (sizeof(NDIS_PACKET_OOB_DATA) +
                                             sizeof(NDIS_PACKET_EXTENSION));

        *Packet = Temp;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        NDIS_DbgPrint(MIN_TRACE, ("No more free descriptors\n"));
        *Status = NDIS_STATUS_RESOURCES;
    }
}


/*
 * @implemented
 */
VOID
EXPORT
NdisDprFreePacket(
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Frees a packet at IRQL DISPATCH_LEVEL
 * ARGUMENTS:
 *     Packet = Pointer to packet to free
 */
{
    PNDISI_PACKET_POOL Pool = (PNDISI_PACKET_POOL)Packet->Private.Pool;

    KeAcquireSpinLockAtDpcLevel(&Pool->SpinLock.SpinLock);
    NdisDprFreePacketNonInterlocked(Packet);
    KeReleaseSpinLockFromDpcLevel(&Pool->SpinLock.SpinLock);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisDprFreePacketNonInterlocked(
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION: Frees a packet at IRQL DISPATCH_LEVEL (w/o synchronization)
 * ARGUMENTS:
 *     Packet = Pointer to packet to free
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Packet (0x%X).\n", Packet));

    Packet->Reserved[0]          = (ULONG_PTR)((NDISI_PACKET_POOL*)Packet->Private.Pool)->FreeList;
    ((NDISI_PACKET_POOL*)Packet->Private.Pool)->FreeList = Packet;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisFreeBufferPool(
    IN  NDIS_HANDLE PoolHandle)
/*
 * FUNCTION: Frees storage allocated for an NDIS buffer pool
 * ARGUMENTS:
 *     PoolHandle = Handle returned by NdisAllocateBufferPool
 */
{
}


/*
 * @implemented
 */
VOID
EXPORT
NdisFreePacketPool(
    IN  NDIS_HANDLE PoolHandle)
/*
 * FUNCTION: Frees storage allocated for an NDIS packet pool
 * ARGUMENTS:
 *     PoolHandle = Handle returned by NdisAllocatePacketPool
 */
{
    ExFreePool((PVOID)PoolHandle);
}


/*
 * @implemented
 */
#undef NdisFreeBuffer
VOID
EXPORT
NdisFreeBuffer(
    IN   PNDIS_BUFFER   Buffer)
/*
 * FUNCTION: Puts an NDIS buffer descriptor back in it's pool
 * ARGUMENTS:
 *     Buffer = Pointer to buffer descriptor
 */
{
    IoFreeMdl(Buffer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisFreePacket(
    IN   PNDIS_PACKET   Packet)
/*
 * FUNCTION: Puts an NDIS packet descriptor back in it's pool
 * ARGUMENTS:
 *     Packet = Pointer to packet descriptor
 */
{
    PNDISI_PACKET_POOL Pool = (PNDISI_PACKET_POOL)Packet->Private.Pool;

    KeAcquireSpinLock(&Pool->SpinLock.SpinLock, &Pool->SpinLock.OldIrql);
    NdisDprFreePacketNonInterlocked(Packet);
    KeReleaseSpinLock(&Pool->SpinLock.SpinLock, Pool->SpinLock.OldIrql);
}


/*
 * @implemented
 */
#undef NdisGetBufferPhysicalArraySize
VOID
EXPORT
NdisGetBufferPhysicalArraySize(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           ArraySize)
/*
 * FUNCTION: Returns number of discontiguous physical blocks backing a buffer
 * ARGUMENTS:
 *     Buffer    = Pointer to buffer descriptor
 *     ArraySize = Address of buffer to place number of physical blocks
 */
{
  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  ASSERT(Buffer && ArraySize);

  *ArraySize = NDIS_BUFFER_TO_SPAN_PAGES(Buffer);
}


/*
 * @implemented
 */
#undef NdisGetFirstBufferFromPacket
VOID
EXPORT
NdisGetFirstBufferFromPacket(
    IN  PNDIS_PACKET    _Packet,
    OUT PNDIS_BUFFER    *_FirstBuffer,
    OUT PVOID           *_FirstBufferVA,
    OUT PUINT           _FirstBufferLength,
    OUT PUINT           _TotalBufferLength)
/*
 * FUNCTION: Retrieves information about an NDIS packet
 * ARGUMENTS:
 *     _Packet            = Pointer to NDIS packet
 *     _FirstBuffer       = Address of buffer for pointer to first NDIS buffer
 *     _FirstBufferVA     = Address of buffer for address of first NDIS buffer
 *     _FirstBufferLength = Address of buffer for length of first buffer
 *     _TotalBufferLength = Address of buffer for total length of packet
 */
{
    PNDIS_BUFFER Buffer;

    Buffer          = _Packet->Private.Head;
    *_FirstBuffer   = Buffer;

    if (Buffer != NULL) {
        *_FirstBufferLength = MmGetMdlByteCount(Buffer);
        *_FirstBufferVA = MmGetSystemAddressForMdl(Buffer);
        Buffer = Buffer->Next;
    } else {
        NDIS_DbgPrint(MID_TRACE, ("No buffers linked to this packet\n"));
        *_FirstBufferLength = 0;
        *_FirstBufferVA = NULL;
    }

    *_TotalBufferLength = *_FirstBufferLength;

    while (Buffer != NULL) {
        *_TotalBufferLength += MmGetMdlByteCount(Buffer);
        Buffer = Buffer->Next;
    }
}

#undef NdisGetFirstBufferFromPacketSafe
/*
 * @implemented
 */
VOID
EXPORT
NdisGetFirstBufferFromPacketSafe(
    IN  PNDIS_PACKET     _Packet,
    OUT PNDIS_BUFFER     *_FirstBuffer,
    OUT PVOID            *_FirstBufferVA,
    OUT PUINT            _FirstBufferLength,
    OUT PUINT            _TotalBufferLength,
    IN  MM_PAGE_PRIORITY Priority)
{
    PNDIS_BUFFER Buffer;

    Buffer          = _Packet->Private.Head;
    *_FirstBuffer   = Buffer;

    if (Buffer != NULL) {
        *_FirstBufferLength = MmGetMdlByteCount(Buffer);
        *_FirstBufferVA = MmGetSystemAddressForMdlSafe(Buffer, Priority);
        Buffer = Buffer->Next;
    } else {
        NDIS_DbgPrint(MID_TRACE, ("No buffers linked to this packet\n"));
        *_FirstBufferLength = 0;
        *_FirstBufferVA = NULL;
    }

    *_TotalBufferLength = *_FirstBufferLength;

    while (Buffer != NULL) {
        *_TotalBufferLength += MmGetMdlByteCount(Buffer);
        Buffer = Buffer->Next;
    }
}

/*
 * @implemented
 */
#undef NdisQueryBuffer
VOID
EXPORT
NdisQueryBuffer(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length)
/*
 * FUNCTION:
 *     Queries an NDIS buffer for information
 * ARGUMENTS:
 *     Buffer         = Pointer to NDIS buffer to query
 *     VirtualAddress = Address of buffer to place virtual address
 *     Length         = Address of buffer to place length of buffer
 */
{
	if (VirtualAddress != NULL)
		*(PVOID*)VirtualAddress = MmGetSystemAddressForMdl(Buffer);

	*Length = MmGetMdlByteCount(Buffer);
}


/*
 * @implemented
 */
#undef NdisQueryBufferSafe
VOID
EXPORT
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length,
    IN  UINT            Priority)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    if (VirtualAddress != NULL)
        *VirtualAddress = MmGetSystemAddressForMdlSafe(Buffer, Priority);
    *Length = MmGetMdlByteCount(Buffer);
}


/*
 * @implemented
 */
#undef NdisQueryBufferOffset
VOID
EXPORT
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           Offset,
    OUT PUINT           Length)
{
    *((PUINT)Offset) = MmGetMdlByteOffset(Buffer);
    *((PUINT)Length) = MmGetMdlByteCount(Buffer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisUnchainBufferAtBack(
    IN OUT  PNDIS_PACKET    Packet,
    OUT     PNDIS_BUFFER    *Buffer)
/*
 * FUNCTION:
 *     Removes the last buffer in a packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet
 *     Buffer = Address of buffer to place pointer to removed NDIS buffer
 */
{
	PNDIS_BUFFER NdisBuffer, Previous;

    NdisQueryPacket(Packet,
                    NULL,
                    NULL,
                    &NdisBuffer,
                    NULL);
    if (!NdisBuffer) {
        NDIS_DbgPrint(MID_TRACE, ("No buffer to unchain\n"));
        *Buffer = NULL;
        return;
    }

    Previous = NULL;
    while (NdisBuffer->Next) {
        Previous   = NdisBuffer;
        NdisBuffer = NdisBuffer->Next;
    }

    if (Previous) {
        Previous->Next       = NULL;
        Packet->Private.Tail = Previous;
    } else {
        Packet->Private.Head = NULL;
        Packet->Private.Tail = NULL;
    }

    Packet->Private.ValidCounts = FALSE;

    *Buffer = NdisBuffer;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisUnchainBufferAtFront(
    IN OUT  PNDIS_PACKET    Packet,
    OUT     PNDIS_BUFFER    *Buffer)
/*
 * FUNCTION:
 *     Removes the first buffer in a packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet
 *     Buffer = Address of buffer to place pointer to removed NDIS buffer
 */
{
	PNDIS_BUFFER NdisBuffer;

    NdisQueryPacket(Packet,
                    NULL,
                    NULL,
                    &NdisBuffer,
                    NULL);
    if (!NdisBuffer) {
        NDIS_DbgPrint(MID_TRACE, ("No buffer to unchain\n"));
        *Buffer = NULL;
        return;
    }

    Packet->Private.Head = NdisBuffer->Next;

    if (!NdisBuffer->Next)
        Packet->Private.Tail = NULL;

    NdisBuffer->Next = NULL;

    Packet->Private.ValidCounts = FALSE;

    *Buffer = NdisBuffer;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCopyBuffer(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_BUFFER    *Buffer,
    IN  NDIS_HANDLE     PoolHandle,
    IN  PVOID           MemoryDescriptor,
    IN  UINT            Offset,
    IN  UINT            Length)
/*
 * FUNCTION: Returns a new buffer descriptor for a (partial) buffer
 * ARGUMENTS:
 *     Status           = Address of a buffer to place status of operation
 *     Buffer           = Address of a buffer to place new buffer descriptor
 *     PoolHandle       = Handle returned by NdisAllocateBufferPool
 *     MemoryDescriptor = Pointer to a memory descriptor (possibly NDIS_BUFFER)
 *     Offset           = Offset in buffer to start copying
 *     Length           = Number of bytes to copy
 */
{
    PVOID CurrentVa = (PUCHAR)(MmGetMdlVirtualAddress((PNDIS_BUFFER)MemoryDescriptor)) + Offset;

    NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

    *Buffer = IoAllocateMdl(CurrentVa, Length, FALSE, FALSE, NULL);
    if (!*Buffer)
    {
        NDIS_DbgPrint(MIN_TRACE, ("IoAllocateMdl failed (%x, %lx)\n", CurrentVa, Length));
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    IoBuildPartialMdl((PNDIS_BUFFER)MemoryDescriptor,
                      *Buffer,
                      CurrentVa,
                      Length);

    (*Buffer)->Next = NULL;
    *Status = NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
NDIS_HANDLE
EXPORT
NdisGetPoolFromPacket(
    IN PNDIS_PACKET  Packet)
{
    return Packet->Private.Pool;
}

/*
 * @implemented
 */
UINT
EXPORT
NdisPacketSize(
    IN UINT  ProtocolReservedSize)
{
    return sizeof(NDIS_PACKET) + sizeof(NDIS_PACKET_OOB_DATA) +
                 sizeof(NDIS_PACKET_EXTENSION) + ProtocolReservedSize;
}

/*
 * @implemented
 */
#undef NdisGetPacketCancelId
PVOID
EXPORT
NdisGetPacketCancelId(
    IN PNDIS_PACKET  Packet)
{
    return NDIS_GET_PACKET_CANCEL_ID(Packet);
}

/*
 * @implemented
 */
#undef NdisSetPacketCancelId
VOID
EXPORT
NdisSetPacketCancelId(
    IN PNDIS_PACKET  Packet,
    IN PVOID  CancelId)
{
    NDIS_SET_PACKET_CANCEL_ID(Packet, CancelId);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCopyFromPacketToPacketSafe(
    IN  PNDIS_PACKET     Destination,
    IN  UINT             DestinationOffset,
    IN  UINT             BytesToCopy,
    IN  PNDIS_PACKET     Source,
    IN  UINT             SourceOffset,
    OUT PUINT            BytesCopied,
    IN  MM_PAGE_PRIORITY Priority)
{
    PNDIS_BUFFER SrcBuffer;
    PNDIS_BUFFER DstBuffer;
    PUCHAR DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    *BytesCopied = 0;

    /* Skip DestinationOffset bytes in the destination packet */
    NdisGetFirstBufferFromPacketSafe(Destination, &DstBuffer, (PVOID*)&DstData, &DstSize, &Total, Priority);
    if (!DstData || SkipToOffset(DstBuffer, DestinationOffset, &DstData, &DstSize) == 0xFFFFFFFF)
        return;

    /* Skip SourceOffset bytes in the source packet */
    NdisGetFirstBufferFromPacketSafe(Source, &SrcBuffer, (PVOID*)&SrcData, &SrcSize, &Total, Priority);
    if (!SrcData || SkipToOffset(SrcBuffer, SourceOffset, &SrcData, &SrcSize) == 0xFFFFFFFF)
        return;

    /* Copy the data */
    for (Total = 0;;) {
        /* Find out how many bytes we can copy at one time */
        if (BytesToCopy < SrcSize)
            Count = BytesToCopy;
        else
            Count = SrcSize;
        if (DstSize < Count)
            Count = DstSize;

        RtlCopyMemory(DstData, SrcData, Count);

        Total       += Count;
        BytesToCopy -= Count;
        if (BytesToCopy == 0)
            break;

        DstSize -= Count;
        if (DstSize == 0) {
            /* No more bytes in destination buffer. Proceed to
               the next buffer in the destination buffer chain */
            NdisGetNextBuffer(DstBuffer, &DstBuffer);
            if (!DstBuffer)
                break;

            NdisQueryBufferSafe(DstBuffer, (PVOID)&DstData, &DstSize, Priority);
            if (!DstData)
                break;
        }

        SrcSize -= Count;
        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBufferSafe(SrcBuffer, (PVOID)&SrcData, &SrcSize, Priority);
            if (!SrcData)
                break;
        }
    }

    *BytesCopied = Total;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisIMCopySendCompletePerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    /* FIXME: What is the difference between NdisIMCopySendPerPacketInfo and
     * NdisIMCopySendCompletePerPacketInfo?
     */

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    RtlCopyMemory(NDIS_PACKET_EXTENSION_FROM_PACKET(DstPacket),
                  NDIS_PACKET_EXTENSION_FROM_PACKET(SrcPacket),
                  sizeof(NDIS_PACKET_EXTENSION));
}


/*
 * @implemented
 */
VOID
EXPORT
NdisIMCopySendPerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    /* FIXME: What is the difference between NdisIMCopySendPerPacketInfo and
     * NdisIMCopySendCompletePerPacketInfo?
     */

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    RtlCopyMemory(NDIS_PACKET_EXTENSION_FROM_PACKET(DstPacket),
                  NDIS_PACKET_EXTENSION_FROM_PACKET(SrcPacket),
                  sizeof(NDIS_PACKET_EXTENSION));
}

/* EOF */
