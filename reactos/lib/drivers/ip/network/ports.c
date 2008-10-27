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

    if ((Port < PortSet->StartingPort) ||
        (Port >= PortSet->StartingPort + PortSet->PortsToOversee))
    {
       return FALSE;
    }

    Port -= PortSet->StartingPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    Clear = RtlAreBitsClear( &PortSet->ProtoBitmap, Port, 1 );
    if( Clear ) RtlSetBits( &PortSet->ProtoBitmap, Port, 1 );
    ExReleaseFastMutex( &PortSet->Mutex );

    return Clear;
}

ULONG AllocateAnyPort( PPORT_SET PortSet ) {
    ULONG AllocatedPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, 0 );
    if( AllocatedPort != (ULONG)-1 ) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	ExReleaseFastMutex( &PortSet->Mutex );
	return htons(AllocatedPort);
    }
    ExReleaseFastMutex( &PortSet->Mutex );

    return -1;
}

ULONG AllocatePortFromRange( PPORT_SET PortSet, ULONG Lowest, ULONG Highest ) {
    ULONG AllocatedPort;

    if ((Lowest < PortSet->StartingPort) ||
        (Highest >= PortSet->StartingPort + PortSet->PortsToOversee))
    {
        return -1;
    }

    Lowest -= PortSet->StartingPort;
    Highest -= PortSet->StartingPort;

    ExAcquireFastMutex( &PortSet->Mutex );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, Lowest );
    if( AllocatedPort != (ULONG)-1 && AllocatedPort <= Highest) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	ExReleaseFastMutex( &PortSet->Mutex );
	return htons(AllocatedPort);
    }
    ExReleaseFastMutex( &PortSet->Mutex );

    return -1;
}
