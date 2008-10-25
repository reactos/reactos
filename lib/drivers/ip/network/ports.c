/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/ports.c
 * PURPOSE:     Port allocation
 * PROGRAMMERS: arty (ayerkes@speakeasy.net)
 * REVISIONS:
 *   arty 20041114 Created
 */

#include "precomp.h"

NTSTATUS PortsStartup( PPORT_SET PortSet,
		   UINT StartingPort,
		   UINT PortsToManage ) {
    PortSet->StartingPort = StartingPort;
    PortSet->PortsToOversee = PortsToManage;
    PortSet->LastAllocatedPort = PortSet->StartingPort +
                                 PortSet->PortsToOversee - 1;
    PortSet->ProtoBitBuffer =
	PoolAllocateBuffer( (PortSet->PortsToOversee + 7) / 8 );
    if(!PortSet->ProtoBitBuffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlInitializeBitMap( &PortSet->ProtoBitmap,
			 PortSet->ProtoBitBuffer,
			 PortSet->PortsToOversee );
    RtlClearAllBits( &PortSet->ProtoBitmap );
    ExInitializeFastMutex( &PortSet->Mutex );
    return STATUS_SUCCESS;
}

VOID PortsShutdown( PPORT_SET PortSet ) {
    PoolFreeBuffer( PortSet->ProtoBitBuffer );
}

VOID DeallocatePort( PPORT_SET PortSet, ULONG Port ) {
    Port = htons(Port);
    ASSERT(Port >= PortSet->StartingPort);
    ASSERT(Port < PortSet->StartingPort + PortSet->PortsToOversee);
    RtlClearBits( &PortSet->ProtoBitmap, Port - PortSet->StartingPort, 1 );
}

BOOLEAN AllocatePort( PPORT_SET PortSet, ULONG Port ) {
    BOOLEAN Clear;

    Port = htons(Port);
    ASSERT(Port >= PortSet->StartingPort);
    ASSERT(Port < PortSet->StartingPort + PortSet->PortsToOversee);
    Port -= PortSet->StartingPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    Clear = RtlAreBitsClear( &PortSet->ProtoBitmap, Port, 1 );
    if( Clear ) RtlSetBits( &PortSet->ProtoBitmap, Port, 1 );
    ExReleaseFastMutex( &PortSet->Mutex );

    return Clear;
}

ULONG AllocateAnyPort( PPORT_SET PortSet ) {
    ULONG AllocatedPort;
    ULONG Next;

    if (PortSet->StartingPort + PortSet->PortsToOversee <=
        PortSet->LastAllocatedPort + 1) {
	Next = PortSet->StartingPort;
    } else {
	Next = PortSet->LastAllocatedPort + 1;
    }
    Next -= PortSet->StartingPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, Next );
    if( AllocatedPort != (ULONG)-1 ) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	PortSet->LastAllocatedPort = AllocatedPort;
    }
    ExReleaseFastMutex( &PortSet->Mutex );

    AllocatedPort = htons(AllocatedPort);

    ASSERT(AllocatedPort >= PortSet->StartingPort);
    ASSERT(AllocatedPort < PortSet->StartingPort + PortSet->PortsToOversee);

    return AllocatedPort;
}

ULONG AllocatePortFromRange( PPORT_SET PortSet, ULONG Lowest, ULONG Highest ) {
    ULONG AllocatedPort;
    ULONG Next;

    if (PortSet->StartingPort + PortSet->PortsToOversee <=
        PortSet->LastAllocatedPort + 1) {
	Next = PortSet->StartingPort;
    } else {
	Next = PortSet->LastAllocatedPort + 1;
    }
    if (Next < Lowest || Highest <= Next) {
	Next = Lowest;
    }
    Next -= PortSet->StartingPort;
    Lowest -= PortSet->StartingPort;
    Highest -= PortSet->StartingPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, Next );
    if( AllocatedPort != (ULONG)-1 && AllocatedPort >= Lowest &&
        AllocatedPort <= Highest) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	PortSet->LastAllocatedPort = AllocatedPort;
    }
    ExReleaseFastMutex( &PortSet->Mutex );

    AllocatedPort = htons(AllocatedPort);

    ASSERT(AllocatedPort >= PortSet->StartingPort);
    ASSERT(AllocatedPort < PortSet->StartingPort + PortSet->PortsToOversee);

    return AllocatedPort;
}
