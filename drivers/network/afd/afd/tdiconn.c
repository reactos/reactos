/* $Id$
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/tdiconn.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */
#include <afd.h>
#include <pseh/pseh.h>
#include "debug.h"
#include "tdiconn.h"

UINT TdiAddressSizeFromType( UINT AddressType ) {
    switch( AddressType ) {
    case AF_INET:
	return sizeof(TA_IP_ADDRESS);
    default:
	AFD_DbgPrint(MID_TRACE,("TdiAddressSizeFromType - invalid type: %x\n", AddressType));
	KeBugCheck( 0 );
    }
    return 0;
}

UINT TaLengthOfAddress( PTA_ADDRESS Addr ) {
    UINT AddrLen = 2 * sizeof( USHORT ) + Addr->AddressLength;
    AFD_DbgPrint(MID_TRACE,("AddrLen %x\n", AddrLen));
    return AddrLen;
}

UINT TaLengthOfTransportAddress( PTRANSPORT_ADDRESS Addr ) {
    UINT AddrLen = 2 * sizeof( ULONG ) + Addr->Address[0].AddressLength;
    AFD_DbgPrint(MID_TRACE,("AddrLen %x\n", AddrLen));
    return AddrLen;
}

VOID TaCopyAddressInPlace( PTA_ADDRESS Target,
			   PTA_ADDRESS Source ) {
    UINT AddrLen = TaLengthOfAddress( Source );
    RtlCopyMemory( Target, Source, AddrLen );
}

PTA_ADDRESS TaCopyAddress( PTA_ADDRESS Source ) {
    UINT AddrLen = TaLengthOfAddress( Source );
    PVOID Buffer = ExAllocatePool( NonPagedPool, AddrLen );
    RtlCopyMemory( Buffer, Source, AddrLen );
    return Buffer;
}

VOID TaCopyTransportAddressInPlace( PTRANSPORT_ADDRESS Target,
				    PTRANSPORT_ADDRESS Source ) {
    UINT AddrLen = TaLengthOfTransportAddress( Source );
    RtlCopyMemory( Target, Source, AddrLen );
}

PTRANSPORT_ADDRESS TaCopyTransportAddress( PTRANSPORT_ADDRESS OtherAddress ) {
    UINT AddrLen;
    PTRANSPORT_ADDRESS A;

    ASSERT(OtherAddress->TAAddressCount == 1);
    AddrLen = TaLengthOfTransportAddress( OtherAddress );
    A = ExAllocatePool( NonPagedPool, AddrLen );

    if( A )
	TaCopyTransportAddressInPlace( A, OtherAddress );

  return A;
}

static NTSTATUS TdiBuildNullConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo,
  ULONG Type )
/*
 * FUNCTION: Builds a NULL TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer to place connection information
 *     Type           = TDI style address type (TDI_ADDRESS_TYPE_XXX).
 * RETURNS:
 *     Status of operation
 */
{
  ULONG TdiAddressSize;
  PTRANSPORT_ADDRESS TransportAddress;

  TdiAddressSize = TdiAddressSizeFromType(Type);

  RtlZeroMemory(ConnInfo,
		sizeof(TDI_CONNECTION_INFORMATION) +
		TdiAddressSize);

  ConnInfo->OptionsLength = sizeof(ULONG);
  ConnInfo->RemoteAddressLength = TdiAddressSize;
  ConnInfo->RemoteAddress = TransportAddress =
      (PTRANSPORT_ADDRESS)&ConnInfo[1];
  TransportAddress->TAAddressCount = 1;
  TransportAddress->Address[0].AddressType = Type;

  return STATUS_SUCCESS;
}

NTSTATUS TdiBuildNullConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo,
  ULONG Type )
/*
 * FUNCTION: Builds a NULL TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer pointer to allocate connection
 *                      information in
 *     Type           = TDI style address type (TDI_ADDRESS_TYPE_XXX).
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_CONNECTION_INFORMATION ConnInfo;
  ULONG TdiAddressSize;
  NTSTATUS Status;

  TdiAddressSize = TdiAddressSizeFromType(Type);

  ConnInfo = (PTDI_CONNECTION_INFORMATION)
    ExAllocatePool(NonPagedPool,
		   sizeof(TDI_CONNECTION_INFORMATION) +
		   TdiAddressSize);
  if (!ConnInfo) {
      *ConnectionInfo = NULL;
      return STATUS_INSUFFICIENT_RESOURCES;
  }

  Status = TdiBuildNullConnectionInfoInPlace( ConnInfo, Type );

  if (!NT_SUCCESS(Status)) {
      ExFreePool( ConnInfo );
      *ConnectionInfo = NULL;
      return Status;
  } else
      *ConnectionInfo = ConnInfo;

  ConnInfo->RemoteAddress = (PTA_ADDRESS)&ConnInfo[1];
  ConnInfo->RemoteAddressLength = TdiAddressSize;

  return Status;
}


NTSTATUS
TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnectionInfo,
  PTRANSPORT_ADDRESS Address ) {
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH_TRY {
    	RtlCopyMemory( ConnectionInfo->RemoteAddress,
		       Address,
		       ConnectionInfo->RemoteAddressLength );
    } _SEH_HANDLE {
	Status = _SEH_GetExceptionCode();
    } _SEH_END;

    return Status;
}


NTSTATUS
TdiBuildConnectionInfo
( PTDI_CONNECTION_INFORMATION *ConnectionInfo,
  PTRANSPORT_ADDRESS Address ) {
    NTSTATUS Status = TdiBuildNullConnectionInfo
	( ConnectionInfo, Address->Address[0].AddressType );

    if( NT_SUCCESS(Status) )
	TdiBuildConnectionInfoInPlace( *ConnectionInfo, Address );

    return Status;
}

NTSTATUS
TdiBuildConnectionInfoPair
( PTDI_CONNECTION_INFO_PAIR ConnectionInfo,
  PTRANSPORT_ADDRESS From, PTRANSPORT_ADDRESS To )
    /*
     * FUNCTION: Fill a TDI_CONNECTION_INFO_PAIR struct will the two addresses
     *           given.
     * ARGUMENTS:
     *   ConnectionInfo: The pair
     *   From:           The from address
     *   To:             The to address
     * RETURNS:
     *   Status of the operation
     */
{
    PCHAR LayoutFrame;
    UINT SizeOfEntry;
    ULONG TdiAddressSize;
    PTDI_CONNECTION_INFORMATION FromTdiConn, ToTdiConn;

    if (!From) return STATUS_INVALID_PARAMETER;

    /* FIXME: Get from socket information */
    TdiAddressSize = TdiAddressSizeFromType(From->Address[0].AddressType);
    SizeOfEntry = TdiAddressSize + sizeof(TDI_CONNECTION_INFORMATION);

    LayoutFrame = (PCHAR)ExAllocatePool(NonPagedPool, 2 * SizeOfEntry);

    if (!LayoutFrame) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( LayoutFrame, 2 * SizeOfEntry );

    FromTdiConn = (PTDI_CONNECTION_INFORMATION)LayoutFrame;
    ToTdiConn = (PTDI_CONNECTION_INFORMATION)LayoutFrame + SizeOfEntry;

    TdiBuildConnectionInfoInPlace( FromTdiConn, From );

    TdiBuildConnectionInfoInPlace( ToTdiConn, To );

    return STATUS_SUCCESS;
}

PTA_ADDRESS TdiGetRemoteAddress( PTDI_CONNECTION_INFORMATION TdiConn )
    /*
     * Convenience function that rounds out the abstraction of
     * the TDI_CONNECTION_INFORMATION struct.
     */
{
    return TdiConn->RemoteAddress;
}

