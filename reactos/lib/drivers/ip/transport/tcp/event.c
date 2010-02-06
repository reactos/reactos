/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol -- Events from oskittcp
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

int TCPSocketState(void *ClientData,
           void *WhichSocket,
           void *WhichConnection,
           OSK_UINT NewState ) {
    PCONNECTION_ENDPOINT Connection = WhichConnection;

    TI_DbgPrint(DEBUG_TCP,("Connection: %x Flags: %c%c%c%c%c\n",
               Connection,
               NewState & SEL_CONNECT ? 'C' : 'c',
               NewState & SEL_READ    ? 'R' : 'r',
               NewState & SEL_FIN     ? 'F' : 'f',
               NewState & SEL_ACCEPT  ? 'A' : 'a',
               NewState & SEL_WRITE   ? 'W' : 'w'));

    /* If this socket is missing its socket context, that means that it
     * has been created as a new connection in sonewconn but not accepted
     * yet. We can safely ignore event notifications on these sockets.
     * Once they are accepted, they will get a socket context and we will 
     * be able to process them.
     */
    if (!Connection)
        return 0;

    TI_DbgPrint(DEBUG_TCP,("Called: NewState %x (Conn %x) (Change %x)\n",
               NewState, Connection,
               Connection->SignalState ^ NewState,
               NewState));

    Connection->SignalState = NewState;

    HandleSignalledConnection(Connection);

    return 0;
}

void TCPPacketSendComplete( PVOID Context,
                PNDIS_PACKET NdisPacket,
                NDIS_STATUS NdisStatus ) {
    TI_DbgPrint(DEBUG_TCP,("called %x\n", NdisPacket));
    FreeNdisPacket(NdisPacket);
    TI_DbgPrint(DEBUG_TCP,("done\n"));
}

#define STRINGIFY(x) #x

int TCPPacketSend(void *ClientData, OSK_PCHAR data, OSK_UINT len ) {
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_PACKET Packet = { 0 };
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;

    if( *data == 0x45 ) { /* IPv4 */
    Header = (PIPv4_HEADER)data;
    LocalAddress.Type = IP_ADDRESS_V4;
    LocalAddress.Address.IPv4Address = Header->SrcAddr;
    RemoteAddress.Type = IP_ADDRESS_V4;
    RemoteAddress.Address.IPv4Address = Header->DstAddr;
    } else {
    TI_DbgPrint(MIN_TRACE,("Outgoing packet is not IPv4\n"));
    OskitDumpBuffer( data, len );
    return OSK_EINVAL;
    }

    if(!(NCE = RouteGetRouteToDestination( &RemoteAddress ))) {
    TI_DbgPrint(MIN_TRACE,("Unable to get route to %s\n", A2S(&RemoteAddress)));
    return OSK_EADDRNOTAVAIL;
    }

    NdisStatus = AllocatePacketWithBuffer( &Packet.NdisPacket, NULL, len );

    if (NdisStatus != NDIS_STATUS_SUCCESS) {
    TI_DbgPrint(DEBUG_TCP, ("Error from NDIS: %08x\n", NdisStatus));
    return OSK_ENOBUFS;
    }

    GetDataPtr( Packet.NdisPacket, 0,
        (PCHAR *)&Packet.Header, &Packet.ContigSize );

    RtlCopyMemory( Packet.Header, data, len );

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = len;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    if (!NT_SUCCESS(IPSendDatagram( &Packet, NCE, TCPPacketSendComplete, NULL )))
    {
        FreeNdisPacket(Packet.NdisPacket);
        return OSK_EINVAL;
    }

    return 0;
}

/* Memory management routines
 *
 * By far the most requests for memory are either for 128 or 2048 byte blocks,
 * so we want to satisfy those from lookaside lists. Unfortunately, the
 * TCPFree() function doesn't pass the size of the block to be freed, so we
 * need to keep track of it ourselves. We do it by prepending each block with
 * 4 bytes, indicating if this is a 'L'arge (2048), 'S'mall (128) or 'O'ther
 * block.
 */

/* Set to some non-zero value to get a profile of memory allocation sizes */
#define MEM_PROFILE 0

#define SMALL_SIZE 128
#define LARGE_SIZE 2048

#define SIGNATURE_LARGE 'LLLL'
#define SIGNATURE_SMALL 'SSSS'
#define SIGNATURE_OTHER 'OOOO'
static NPAGED_LOOKASIDE_LIST LargeLookasideList;
static NPAGED_LOOKASIDE_LIST SmallLookasideList;

NTSTATUS
TCPMemStartup( void )
{
    ExInitializeNPagedLookasideList( &LargeLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     LARGE_SIZE + sizeof( ULONG ),
                                     OSK_LARGE_TAG,
                                     0 );
    ExInitializeNPagedLookasideList( &SmallLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     SMALL_SIZE + sizeof( ULONG ),
                                     OSK_SMALL_TAG,
                                     0 );

    return STATUS_SUCCESS;
}

void *TCPMalloc( void *ClientData,
         OSK_UINT Bytes, OSK_PCHAR File, OSK_UINT Line ) {
    void *v;
    ULONG Signature;

#if 0 != MEM_PROFILE
    static OSK_UINT *Sizes = NULL, *Counts = NULL, ArrayAllocated = 0;
    static OSK_UINT ArrayUsed = 0, AllocationCount = 0;
    OSK_UINT i, NewSize, *NewArray;
    int Found;

    Found = 0;
    for ( i = 0; i < ArrayUsed && ! Found; i++ ) {
    Found = ( Sizes[i] == Bytes );
    if ( Found ) {
        Counts[i]++;
    }
    }
    if ( ! Found ) {
    if ( ArrayAllocated <= ArrayUsed ) {
        NewSize = ( 0 == ArrayAllocated ? 16 : 2 * ArrayAllocated );
        NewArray = exAllocatePool( NonPagedPool, 2 * NewSize * sizeof( OSK_UINT ) );
        if ( NULL != NewArray ) {
        if ( 0 != ArrayAllocated ) {
            memcpy( NewArray, Sizes,
                    ArrayAllocated * sizeof( OSK_UINT ) );
            exFreePool( Sizes );
            memcpy( NewArray + NewSize, Counts,
                    ArrayAllocated * sizeof( OSK_UINT ) );
            exFreePool( Counts );
        }
        Sizes = NewArray;
        Counts = NewArray + NewSize;
        ArrayAllocated = NewSize;
        } else if ( 0 != ArrayAllocated ) {
        exFreePool( Sizes );
        exFreePool( Counts );
        ArrayAllocated = 0;
        }
    }
    if ( ArrayUsed < ArrayAllocated ) {
        Sizes[ArrayUsed] = Bytes;
        Counts[ArrayUsed] = 1;
        ArrayUsed++;
    }
    }

    if ( 0 == (++AllocationCount % MEM_PROFILE) ) {
    TI_DbgPrint(DEBUG_TCP, ("Memory allocation size profile:\n"));
    for ( i = 0; i < ArrayUsed; i++ ) {
        TI_DbgPrint(DEBUG_TCP,
                    ("Size %4u Count %5u\n", Sizes[i], Counts[i]));
    }
    TI_DbgPrint(DEBUG_TCP, ("End of memory allocation size profile\n"));
    }
#endif /* MEM_PROFILE */

    if ( SMALL_SIZE == Bytes ) {
    v = ExAllocateFromNPagedLookasideList( &SmallLookasideList );
    Signature = SIGNATURE_SMALL;
    } else if ( LARGE_SIZE == Bytes ) {
    v = ExAllocateFromNPagedLookasideList( &LargeLookasideList );
    Signature = SIGNATURE_LARGE;
    } else {
    v = ExAllocatePoolWithTag( NonPagedPool, Bytes + sizeof(ULONG),
                               OSK_OTHER_TAG );
    Signature = SIGNATURE_OTHER;
    }
    if( v ) {
    *((ULONG *) v) = Signature;
    v = (void *)((char *) v + sizeof(ULONG));
    }

    return v;
}

void TCPFree( void *ClientData,
          void *data, OSK_PCHAR File, OSK_UINT Line ) {
    ULONG Signature;

    data = (void *)((char *) data - sizeof(ULONG));
    Signature = *((ULONG *) data);
    if ( SIGNATURE_SMALL == Signature ) {
    ExFreeToNPagedLookasideList( &SmallLookasideList, data );
    } else if ( SIGNATURE_LARGE == Signature ) {
    ExFreeToNPagedLookasideList( &LargeLookasideList, data );
    } else if ( SIGNATURE_OTHER == Signature ) {
    ExFreePoolWithTag( data, OSK_OTHER_TAG );
    } else {
    ASSERT( FALSE );
    }
}

void
TCPMemShutdown( void )
{
    ExDeleteNPagedLookasideList( &SmallLookasideList );
    ExDeleteNPagedLookasideList( &LargeLookasideList );
}
