#ifndef _TCPIP_INTERFACE_H
#define _TCPIP_INTERFACE_H

#include <ip.h>

NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface,
				  ULONG Type,
				  PULONG Address );

UINT CountInterfaces();

NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed );
NTSTATUS GetInterfaceName( PIP_INTERFACE Interface, PCHAR NameBuffer,
			   UINT NameMaxLen );

#endif//_TCPIP_INTERFACE_H
