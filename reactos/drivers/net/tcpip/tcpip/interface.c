/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/interface.c
 * PURPOSE:     Convenient abstraction for getting and setting information
 *              in IP_INTERFACE.
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <tcpip.h>
#include <lan.h>
#include <address.h>
#include <pool.h>
#include <ip.h>

NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface, 
				  ULONG TargetType,
				  PULONG Address ) {
    PLIST_ENTRY CurrentIFEntry;
    PLIST_ENTRY CurrentADEEntry;
    PADDRESS_ENTRY CurrentADE;
    
    CurrentADEEntry = Interface->ADEListHead.Flink;
    while (CurrentADEEntry != &Interface->ADEListHead)
    {
	CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
	if (CurrentADE->Type == TargetType) {
	    *Address = CurrentADE->Address->Address.IPv4Address;
	    return STATUS_SUCCESS;
	}
	CurrentADEEntry = CurrentADEEntry->Flink;
    }
    
    return STATUS_UNSUCCESSFUL;
}

UINT CountInterfaces() {
    DWORD Count = 0;
    KIRQL OldIrql;
    PLIST_ENTRY CurrentIFEntry;

    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead) {
	Count++;
	CurrentIFEntry = CurrentIFEntry->Flink;
    }

    KeReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Count;
}

UINT CountInterfaceAddresses( PIP_INTERFACE Interface ) {
    UINT AddrCount = 0;
    PADDRESS_ENTRY CurrentADE;
    PLIST_ENTRY CurrentADEntry;

    CurrentADEntry = Interface->ADEListHead.Flink;
    
    while( CurrentADEntry != &Interface->ADEListHead ) {
	CurrentADEntry = CurrentADEntry->Flink;
	CurrentADE = CONTAINING_RECORD(CurrentADEntry, 
				       ADDRESS_ENTRY, 
				       ListEntry);
	if( CurrentADE->Type == ADE_UNICAST )
	    AddrCount++;
    }

    return AddrCount;
}

NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed ) {
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    
    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          Speed,
                          sizeof(UINT));
    
    return 
	NdisStatus != NDIS_STATUS_SUCCESS ? 
	STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

NTSTATUS GetInterfaceName( PIP_INTERFACE Interface, 
			   PCHAR NameBuffer,
			   UINT Len ) {
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    
    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_FRIENDLY_NAME,
                          NameBuffer,
			  Len);
    
    return 
	NdisStatus != NDIS_STATUS_SUCCESS ? 
	STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}
