/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/buffer.c
 * PURPOSE:     Buffer management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <buffer.h>


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
    KIRQL OldIrql;
    PNETWORK_HEADER Temp;
    PNDIS_BUFFER_POOL Pool = (PNDIS_BUFFER_POOL)PoolHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  Buffer (0x%X)  PoolHandle (0x%X)  "
        "VirtualAddress (0x%X)  Length (%d)\n",
        Status, Buffer, PoolHandle, VirtualAddress, Length));

    KeAcquireSpinLock(&Pool->SpinLock, &OldIrql);

    if (Pool->FreeList) {
        Temp           = Pool->FreeList;
        Pool->FreeList = Temp->Next;

        KeReleaseSpinLock(&Pool->SpinLock, OldIrql);

        Temp->Next = NULL;

#ifdef _MSC_VER
        MmInitializeMdl(&Temp->Mdl, VirtualAddress, Length);
        Temp->Mdl.MdlFlags      |= (MDL_SOURCE_IS_NONPAGED_POOL | MDL_ALLOCATED_FIXED_SIZE);
        Temp->Mdl.MappedSystemVa = VirtualAddress;
#else
	    Temp->Mdl.Next = (PMDL)NULL;
	    Temp->Mdl.Size = (CSHORT)(sizeof(MDL) +
            (ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress, Length) * sizeof(ULONG)));
	    Temp->Mdl.MdlFlags   = (MDL_SOURCE_IS_NONPAGED_POOL | MDL_ALLOCATED_FIXED_SIZE);
;	    Temp->Mdl.StartVa    = (PVOID)PAGE_ROUND_DOWN(VirtualAddress);
	    Temp->Mdl.ByteOffset = (ULONG_PTR)(VirtualAddress - PAGE_ROUND_DOWN(VirtualAddress));
	    Temp->Mdl.ByteCount  = Length;
        Temp->Mdl.MappedSystemVa = VirtualAddress;
#if 0
	    //Temp->Mdl.Process    = PsGetCurrentProcess();
#else
        Temp->Mdl.Process    = NULL;
#endif
#endif
        
        Temp->BufferPool = Pool;

        *Buffer = (PNDIS_BUFFER)Temp;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        KeReleaseSpinLock(&Pool->SpinLock, OldIrql);
        *Status = NDIS_STATUS_FAILURE;
    }
}


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
    UINT i;
    PNDIS_BUFFER_POOL Pool;
    PNETWORK_HEADER Buffer;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  PoolHandle (0x%X)  NumberOfDescriptors (%d).\n",
        Status, PoolHandle, NumberOfDescriptors));

    Pool = ExAllocatePool(NonPagedPool,
                          sizeof(NDIS_BUFFER_POOL) +
                          sizeof(NETWORK_HEADER)   *
                          NumberOfDescriptors);
    if (Pool) {
        KeInitializeSpinLock(&Pool->SpinLock);

        if (NumberOfDescriptors > 0) {
            Buffer             = &Pool->Buffers[0];
            Pool->FreeList     = Buffer;
            for (i = 1; i < NumberOfDescriptors; i++) {
                Buffer->Next = &Pool->Buffers[i];
                Buffer       = Buffer->Next;
            }
            Buffer->Next = NULL;
        } else
            Pool->FreeList = NULL;

        *Status     = NDIS_STATUS_SUCCESS;
        *PoolHandle = (PNDIS_HANDLE)Pool;
    } else
        *Status = NDIS_STATUS_RESOURCES;
}


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
    KIRQL OldIrql;
    PNDIS_PACKET Temp;
    PNDIS_PACKET_POOL Pool = (PNDIS_PACKET_POOL)PoolHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  Packet (0x%X)  PoolHandle (0x%X).\n",
        Status, Packet, PoolHandle));

    KeAcquireSpinLock(&Pool->SpinLock.SpinLock, &OldIrql);

    if (Pool->FreeList) {
        Temp           = Pool->FreeList;
        Pool->FreeList = (PNDIS_PACKET)Temp->Private.Head;

        KeReleaseSpinLock(&Pool->SpinLock.SpinLock, OldIrql);

        RtlZeroMemory(&Temp->Private, sizeof(NDIS_PACKET_PRIVATE));
        Temp->Private.Pool = Pool;

        *Packet = Temp;
        *Status = NDIS_STATUS_SUCCESS;
    } else {
        *Status = NDIS_STATUS_RESOURCES;
        KeReleaseSpinLock(&Pool->SpinLock.SpinLock, OldIrql);
    }
}


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
    PNDIS_PACKET_POOL Pool;
    UINT Size, Length, i;
    PNDIS_PACKET Packet, NextPacket;

    NDIS_DbgPrint(MAX_TRACE, ("Status (0x%X)  PoolHandle (0x%X)  "
        "NumberOfDescriptors (%d)  ProtocolReservedLength (%d).\n",
        Status, PoolHandle, NumberOfDescriptors, ProtocolReservedLength));

    Length = sizeof(NDIS_PACKET) + ProtocolReservedLength;
    Size   = sizeof(NDIS_PACKET_POOL) + Length * NumberOfDescriptors;

    Pool   = ExAllocatePool(NonPagedPool, Size);
    if (Pool) {
        KeInitializeSpinLock(&Pool->SpinLock.SpinLock);
        Pool->PacketLength = Length;

        if (NumberOfDescriptors > 0) {
            Packet         = (PNDIS_PACKET)&Pool->Buffer;
            Pool->FreeList = Packet;

            NextPacket = (PNDIS_PACKET)((ULONG_PTR)Packet + Length);
            for (i = 1; i < NumberOfDescriptors; i++) {
                Packet->Private.Head = (PNDIS_BUFFER)NextPacket;
                Packet               = NextPacket;
                NextPacket           = (PNDIS_PACKET)((ULONG_PTR)Packet + Length);
            }
            Packet->Private.Head = NULL;
        } else
            Pool->FreeList = NULL;

        *Status     = NDIS_STATUS_SUCCESS;
        *PoolHandle = (PNDIS_HANDLE)Pool;
    } else
        *Status = NDIS_STATUS_RESOURCES;
}


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
    UNIMPLEMENTED
}


ULONG
EXPORT
NdisBufferLength(
    IN  PNDIS_BUFFER    Buffer)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


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
    UNIMPLEMENTED

    return NULL;
}


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
    *Status = NDIS_STATUS_FAILURE;
}


__inline ULONG SkipToOffset(
    PNDIS_BUFFER Buffer,
    UINT Offset,
    PVOID *Data,
    PUINT Size)
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
    ULONG Address = 0;

    for (;;) {

        if (!Buffer)
            return -1;

        NdisQueryBuffer(Buffer, Data, Size);

        if (Offset < *Size) {
            Address += Offset;
            *Size   -= Offset;
            break;
        }

        Offset -= *Size;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    *Data = (PVOID)Address;

    return Offset;
}


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
    PVOID DstData, SrcData;
    UINT DstSize, SrcSize;
    UINT Count, Total;

    *BytesCopied = 0;

    /* Skip DestinationOffset bytes in the destination packet */
    NdisGetFirstBufferFromPacket(Destination, &DstBuffer, &DstData, &DstSize, &Total);
    if (SkipToOffset(DstBuffer, DestinationOffset, &DstData, &DstSize) == -1)
        return;

    /* Skip SourceOffset bytes in the source packet */
    NdisGetFirstBufferFromPacket(Source, &SrcBuffer, &SrcData, &SrcSize, &Total);
    if (SkipToOffset(SrcBuffer, SourceOffset, &SrcData, &SrcSize) == -1)
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

    *BytesCopied = Total;
}


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
}


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
    *Status = NDIS_STATUS_FAILURE;
}


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
}


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
}


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
    ExFreePool((PVOID)PoolHandle);
}


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
    KIRQL OldIrql;
    PNDIS_BUFFER_POOL Pool;
    PNETWORK_HEADER Temp = (PNETWORK_HEADER)Buffer;

    NDIS_DbgPrint(MAX_TRACE, ("Buffer (0x%X).\n", Buffer));

    Pool = Temp->BufferPool;

    KeAcquireSpinLock(&Pool->SpinLock, &OldIrql);
    Buffer->Next   = (PMDL)Pool->FreeList;
    Pool->FreeList = (PNETWORK_HEADER)Buffer;
    KeReleaseSpinLock(&Pool->SpinLock, OldIrql);
}


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
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("Packet (0x%X).\n", Packet));

    KeAcquireSpinLock(&Packet->Private.Pool->SpinLock.SpinLock, &OldIrql);
    Packet->Private.Head           = (PNDIS_BUFFER)Packet->Private.Pool->FreeList;
    Packet->Private.Pool->FreeList = Packet;
    KeReleaseSpinLock(&Packet->Private.Pool->SpinLock.SpinLock, OldIrql);
}


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
    UNIMPLEMENTED
}


VOID
EXPORT
NdisGetFirstBufferFromPacket(
    IN  PNDIS_PACKET    _Packet,
    OUT PNDIS_BUFFER    *_FirstBuffer,
    OUT PVOID           *_FirstBufferVA,
    OUT PUINT           _FirstBufferLength,
    OUT PUINT           _TotalBufferLength)
{
    UNIMPLEMENTED
}


VOID
EXPORT
NdisReturnPackets(
    IN  PNDIS_PACKET    *PacketsToReturn,
    IN  UINT            NumberOfPackets)
/*
 * FUNCTION: Releases ownership of one or more packets
 * ARGUMENTS:
 *     PacketsToReturn = Pointer to an array of pointers to packet descriptors
 *     NumberOfPackets = Number of pointers in descriptor pointer array
 */
{
    UNIMPLEMENTED
}


UINT
EXPORT
NdisPacketPoolUsage(
    IN  NDIS_HANDLE PoolHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


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


VOID
EXPORT
NdisQueryBufferOffset(
    IN  PNDIS_BUFFER    Buffer,
    OUT PUINT           Offset,
    OUT PUINT           Length)
{
    UNIMPLEMENTED
}


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

/* EOF */
