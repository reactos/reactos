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
	ExAllocatePoolWithTag( NonPagedPool, (PortSet->PortsToOversee + 7) / 8,
                               PORT_SET_TAG );
    if(!PortSet->ProtoBitBuffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlInitializeBitMap( &PortSet->ProtoBitmap,
			 PortSet->ProtoBitBuffer,
			 PortSet->PortsToOversee );
    RtlClearAllBits( &PortSet->ProtoBitmap );
    KeInitializeSpinLock( &PortSet->Lock );
    return STATUS_SUCCESS;
}

VOID PortsShutdown( PPORT_SET PortSet ) {
    ExFreePoolWithTag( PortSet->ProtoBitBuffer, PORT_SET_TAG );
}

VOID DeallocatePort( PPORT_SET PortSet, ULONG Port ) {
    KIRQL OldIrql;

    Port = htons(Port);
    ASSERT(Port >= PortSet->StartingPort);
    ASSERT(Port < PortSet->StartingPort + PortSet->PortsToOversee);

    KeAcquireSpinLock( &PortSet->Lock, &OldIrql );
    RtlClearBits( &PortSet->ProtoBitmap, Port - PortSet->StartingPort, 1 );
    KeReleaseSpinLock( &PortSet->Lock, OldIrql );
}

BOOLEAN AllocatePort( PPORT_SET PortSet, ULONG Port ) {
    BOOLEAN Clear;
    KIRQL OldIrql;

    Port = htons(Port);

    if ((Port < PortSet->StartingPort) ||
        (Port >= PortSet->StartingPort + PortSet->PortsToOversee))
    {
       return FALSE;
    }

    Port -= PortSet->StartingPort;

    KeAcquireSpinLock( &PortSet->Lock, &OldIrql );
    Clear = RtlAreBitsClear( &PortSet->ProtoBitmap, Port, 1 );
    if( Clear ) RtlSetBits( &PortSet->ProtoBitmap, Port, 1 );
    KeReleaseSpinLock( &PortSet->Lock, OldIrql );

    return Clear;
}

ULONG AllocateAnyPort( PPORT_SET PortSet ) {
    ULONG AllocatedPort;
    KIRQL OldIrql;

    KeAcquireSpinLock( &PortSet->Lock, &OldIrql );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, 0 );
    if( AllocatedPort != (ULONG)-1 ) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	KeReleaseSpinLock( &PortSet->Lock, OldIrql );
	return htons(AllocatedPort);
    }
    KeReleaseSpinLock( &PortSet->Lock, OldIrql );

    return -1;
}

ULONG AllocatePortFromRange( PPORT_SET PortSet, ULONG Lowest, ULONG Highest ) {
    ULONG AllocatedPort;
    KIRQL OldIrql;

    if ((Lowest < PortSet->StartingPort) ||
        (Highest >= PortSet->StartingPort + PortSet->PortsToOversee))
    {
        return -1;
    }

    Lowest -= PortSet->StartingPort;
    Highest -= PortSet->StartingPort;

    KeAcquireSpinLock( &PortSet->Lock, &OldIrql );
    AllocatedPort = RtlFindClearBits( &PortSet->ProtoBitmap, 1, Lowest );
    if( AllocatedPort != (ULONG)-1 && AllocatedPort <= Highest) {
	RtlSetBit( &PortSet->ProtoBitmap, AllocatedPort );
	AllocatedPort += PortSet->StartingPort;
	KeReleaseSpinLock( &PortSet->Lock, OldIrql );
	return htons(AllocatedPort);
    }
    KeReleaseSpinLock( &PortSet->Lock, OldIrql );

    return -1;
}
