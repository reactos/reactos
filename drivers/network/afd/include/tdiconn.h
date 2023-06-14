#pragma once

typedef VOID *PTDI_CONNECTION_INFO_PAIR;

PTRANSPORT_ADDRESS TaCopyTransportAddress( PTRANSPORT_ADDRESS OtherAddress );
PTRANSPORT_ADDRESS TaBuildNullTransportAddress(UINT AddressType);
UINT TaLengthOfAddress( PTA_ADDRESS Addr );
UINT TaLengthOfTransportAddress( PTRANSPORT_ADDRESS Addr );
VOID TaCopyTransportAddressInPlace( PTRANSPORT_ADDRESS Target,
				    PTRANSPORT_ADDRESS Source );
UINT TdiAddressSizeFromType( UINT Type );
UINT TdiAddressSizeFromName( PTRANSPORT_ADDRESS Name );
NTSTATUS TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo, PTRANSPORT_ADDRESS Name );
NTSTATUS TdiBuildConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo, PTRANSPORT_ADDRESS Name );
NTSTATUS TdiBuildNullConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo, ULONG Type );
NTSTATUS TdiBuildNullConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo, ULONG Type );
