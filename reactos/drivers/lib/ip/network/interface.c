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

#include "precomp.h"


NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface, 
				  ULONG TargetType,
				  PULONG Address ) {
    ADE_LIST_ITER(CurrentADE);

    ForEachADE(Interface->ADEListHead,CurrentADE) {
	if (CurrentADE->Type == TargetType) {
	    *Address = CurrentADE->Address->Address.IPv4Address;
	    return STATUS_SUCCESS;
	}
    } EndFor(CurrentADE);
    
    return STATUS_UNSUCCESSFUL;
}

UINT CountInterfaces() {
    DWORD Count = 0;
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
    
    ForEachInterface(CurrentIF) {
	Count++;
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Count;
}

UINT CountInterfaceAddresses( PIP_INTERFACE Interface ) {
    UINT AddrCount = 0;
    ADE_LIST_ITER(CurrentADE);

    ForEachADE(Interface->ADEListHead,CurrentADE) {
	if( CurrentADE->Type == ADE_UNICAST )
	    AddrCount++;
    } EndFor(CurrentADE);

    return AddrCount;
}

NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed ) {
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    
#ifdef __NTDRIVER__
    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_LINK_SPEED,
                          Speed,
                          sizeof(UINT));
#else
    (void)IF;
    NdisStatus = NDIS_STATUS_SUCCESS;
    *Speed = 10000;
#endif
    
    return 
	NdisStatus != NDIS_STATUS_SUCCESS ? 
	STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

NTSTATUS GetInterfaceName( PIP_INTERFACE Interface, 
			   PCHAR NameBuffer,
			   UINT Len ) {
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;
    
#ifdef __NTDRIVER__
    /* Get maximum link speed */
    NdisStatus = NDISCall(IF,
                          NdisRequestQueryInformation,
                          OID_GEN_FRIENDLY_NAME,
                          NameBuffer,
			  Len);
#else
    (void)IF;
    NdisStatus = NDIS_STATUS_SUCCESS;
    strncpy( NameBuffer, "eth", Len );
#endif
    
    return 
	NdisStatus != NDIS_STATUS_SUCCESS ? 
	STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}
