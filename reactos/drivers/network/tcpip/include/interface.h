#ifndef _TCPIP_INTERFACE_H
#define _TCPIP_INTERFACE_H

#include <ip.h>

#define IFENT_SOFTWARE_LOOPBACK 24 /* This is an SNMP constant from rfc1213 */

NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface,
				  ULONG Type,
				  PULONG Address );
UINT CountInterfaces();
UINT CountInterfaceAddresses( PIP_INTERFACE Interface );
NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed );
NTSTATUS GetInterfaceName( PIP_INTERFACE Interface, PCHAR NameBuffer,
			   UINT NameMaxLen );
NTSTATUS GetInterfaceConnectionStatus( PIP_INTERFACE Interface,
                                       PULONG OperStatus );
PIP_INTERFACE FindOnLinkInterface(PIP_ADDRESS Address);

#endif//_TCPIP_INTERFACE_H
