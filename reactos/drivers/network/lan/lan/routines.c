#include "precomp.h"

NDIS_HANDLE    GlobalPacketPool = NULL;
NDIS_HANDLE    GlobalBufferPool = NULL;

NDIS_STATUS InitNdisPools() {
    NDIS_STATUS NdisStatus;
    /* Last argument is extra space size */
    NdisAllocatePacketPool( &NdisStatus, &GlobalPacketPool, 100, 0 );
    if( !NT_SUCCESS(NdisStatus) ) return NdisStatus;

    NdisAllocateBufferPool( &NdisStatus, &GlobalBufferPool, 100 );
    if( !NT_SUCCESS(NdisStatus) )
	NdisFreePacketPool(GlobalPacketPool);

    return NdisStatus;
}

VOID CloseNdisPools() {
    if( GlobalPacketPool ) NdisFreePacketPool( GlobalPacketPool );
    if( GlobalBufferPool ) NdisFreeBufferPool( GlobalBufferPool );
}

__inline INT SkipToOffset(
    PNDIS_BUFFER Buffer,
    UINT Offset,
    PCHAR *Data,
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

        NdisQueryBuffer(Buffer, (PVOID)Data, Size);

        if (Offset < *Size) {
            *Data = (PCHAR)((ULONG_PTR) *Data + Offset);
            *Size              -= Offset;
            break;
        }

        Offset -= *Size;

        NdisGetNextBuffer(Buffer, &Buffer);
    }

    return Offset;
}

void GetDataPtr( PNDIS_PACKET Packet,
		 UINT Offset,
		 PCHAR *DataOut,
		 PUINT Size ) {
    PNDIS_BUFFER Buffer;

    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    if( !Buffer ) return;
    SkipToOffset( Buffer, Offset, DataOut, Size );
}


#undef NdisAllocatePacket
#undef NdisAllocateBuffer
#undef NdisFreeBuffer
#undef NdisFreePacket

NDIS_STATUS AllocatePacketWithBufferX( PNDIS_PACKET *NdisPacket,
				       PCHAR Data, UINT Len,
				       PCHAR File, UINT Line ) {
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;
    PCHAR NewData;

    NewData = exAllocatePool( NonPagedPool, Len );
    if( !NewData ) return NDIS_STATUS_NOT_ACCEPTED; // XXX

    if( Data )
	RtlCopyMemory(NewData, Data, Len);

    NdisAllocatePacket( &Status, &Packet, GlobalPacketPool );
    if( Status != NDIS_STATUS_SUCCESS ) {
	exFreePool( NewData );
	return Status;
    }
    TrackWithTag(NDIS_PACKET_TAG, Packet, File, Line);

    NdisAllocateBuffer( &Status, &Buffer, GlobalBufferPool, NewData, Len );
    if( Status != NDIS_STATUS_SUCCESS ) {
	exFreePool( NewData );
	FreeNdisPacket( Packet );
    }
    TrackWithTag(NDIS_BUFFER_TAG, Buffer, File, Line);

    NdisChainBufferAtFront( Packet, Buffer );
    *NdisPacket = Packet;

    return NDIS_STATUS_SUCCESS;
}


VOID FreeNdisPacketX
( PNDIS_PACKET Packet,
  PCHAR File,
  UINT Line )
/*
 * FUNCTION: Frees an NDIS packet
 * ARGUMENTS:
 *     Packet = Pointer to NDIS packet to be freed
 */
{
    PNDIS_BUFFER Buffer, NextBuffer;

    LA_DbgPrint(DEBUG_PBUFFER, ("Packet (0x%X)\n", Packet));

    /* Free all the buffers in the packet first */
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        PVOID Data;
        UINT Length;

        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Data, &Length);
        NdisFreeBuffer(Buffer);
	UntrackFL(File,Line,Buffer);
        exFreePool(Data);
    }

    /* Finally free the NDIS packet descriptor */
    NdisFreePacket(Packet);
    UntrackFL(File,Line,Packet);
}
