#ifndef _TDICONN_H
#define _TDICONN_H

#include <afd.h>

typedef VOID *PTDI_CONNECTION_INFO_PAIR;

DWORD TdiAddressSizeFromType( ULONG Type );
DWORD TdiAddressSizeFromName( LPSOCKADDR Name );
VOID TdiBuildAddressIPv4( PTA_IP_ADDRESS Address,
			  LPSOCKADDR Name );
NTSTATUS TdiBuildAddress( PTA_ADDRESS Address,
			  LPSOCKADDR Name );
NTSTATUS TdiBuildName( LPSOCKADDR Name,
		       PTA_ADDRESS Address );
NTSTATUS TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo, LPSOCKADDR Name );
NTSTATUS TdiBuildConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo, LPSOCKADDR Name );
NTSTATUS TdiBuildNullConnectionInfoToPlace
( PTDI_CONNECTION_INFORMATION ConnInfo, ULONG Type );
NTSTATUS TdiBuildNullConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo, ULONG Type );
NTSTATUS TdiBuildConnectionInfoPair
( PTDI_CONNECTION_INFO_PAIR ConnectionInfo, LPSOCKADDR From, LPSOCKADDR To );
PTA_ADDRESS TdiGetRemoteAddress( PTDI_CONNECTION_INFORMATION TdiConn );

#endif/*_TDICONN_H*/
