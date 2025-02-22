/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/net/afd/afd/tdiconn.c
 * PURPOSE:          Ancillary functions driver
 * PROGRAMMER:       Art Yerkes (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 * 20040708 Created
 */

#include <afd.h>

UINT TdiAddressSizeFromType( UINT AddressType ) {
    switch( AddressType ) {
    case TDI_ADDRESS_TYPE_IP:
        return TDI_ADDRESS_LENGTH_IP;
    case TDI_ADDRESS_TYPE_APPLETALK:
        return TDI_ADDRESS_LENGTH_APPLETALK;
    case TDI_ADDRESS_TYPE_NETBIOS:
        return TDI_ADDRESS_LENGTH_NETBIOS;
    /* case TDI_ADDRESS_TYPE_NS: */
    case TDI_ADDRESS_TYPE_IPX:
        return TDI_ADDRESS_LENGTH_IPX;
    case TDI_ADDRESS_TYPE_VNS:
        return TDI_ADDRESS_LENGTH_VNS;
    default:
        DbgPrint("TdiAddressSizeFromType - invalid type: %x\n", AddressType);
        return 0;
    }
}

UINT TaLengthOfAddress( PTA_ADDRESS Addr )
{
    UINT AddrLen = Addr->AddressLength;

    if (!AddrLen)
        return 0;

    AddrLen += 2 * sizeof( USHORT );

    AFD_DbgPrint(MID_TRACE,("AddrLen %x\n", AddrLen));

    return AddrLen;
}

UINT TaLengthOfTransportAddress( PTRANSPORT_ADDRESS Addr )
{
    UINT AddrLen = TaLengthOfAddress(&Addr->Address[0]);

    if (!AddrLen)
        return 0;

    AddrLen += sizeof(ULONG);

    AFD_DbgPrint(MID_TRACE,("AddrLen %x\n", AddrLen));

    return AddrLen;
}

UINT TaLengthOfTransportAddressByType(UINT AddressType)
{
    UINT AddrLen = TdiAddressSizeFromType(AddressType);

    if (!AddrLen)
        return 0;

    AddrLen += sizeof(ULONG) + 2 * sizeof(USHORT);

    AFD_DbgPrint(MID_TRACE,("AddrLen %x\n", AddrLen));

    return AddrLen;
}

VOID TaCopyTransportAddressInPlace( PTRANSPORT_ADDRESS Target,
                                    PTRANSPORT_ADDRESS Source ) {
    UINT AddrLen = TaLengthOfTransportAddress( Source );
    RtlCopyMemory( Target, Source, AddrLen );
}

PTRANSPORT_ADDRESS TaCopyTransportAddress( PTRANSPORT_ADDRESS OtherAddress ) {
    UINT AddrLen;
    PTRANSPORT_ADDRESS A;

    AddrLen = TaLengthOfTransportAddress( OtherAddress );
    if (!AddrLen)
        return NULL;

    A = ExAllocatePoolWithTag(NonPagedPool,
                              AddrLen,
                              TAG_AFD_TRANSPORT_ADDRESS);

    if( A )
        TaCopyTransportAddressInPlace( A, OtherAddress );

    return A;
}

NTSTATUS TdiBuildNullTransportAddressInPlace(PTRANSPORT_ADDRESS A, UINT AddressType)
{
    A->TAAddressCount = 1;

    A->Address[0].AddressLength = TdiAddressSizeFromType(AddressType);
    if (!A->Address[0].AddressLength)
        return STATUS_INVALID_PARAMETER;

    A->Address[0].AddressType = AddressType;

    RtlZeroMemory(A->Address[0].Address, A->Address[0].AddressLength);

    return STATUS_SUCCESS;
}

PTRANSPORT_ADDRESS TaBuildNullTransportAddress(UINT AddressType)
{
    UINT AddrLen;
    PTRANSPORT_ADDRESS A;

    AddrLen = TaLengthOfTransportAddressByType(AddressType);
    if (!AddrLen)
        return NULL;

    A = ExAllocatePoolWithTag(NonPagedPool, AddrLen, TAG_AFD_TRANSPORT_ADDRESS);

    if (A)
    {
        if (TdiBuildNullTransportAddressInPlace(A, AddressType) != STATUS_SUCCESS)
        {
            ExFreePoolWithTag(A, TAG_AFD_TRANSPORT_ADDRESS);
            return NULL;
        }
    }

    return A;
}

NTSTATUS TdiBuildNullConnectionInfoInPlace
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

    TdiAddressSize = TaLengthOfTransportAddressByType(Type);
    if (!TdiAddressSize)
    {
        AFD_DbgPrint(MIN_TRACE,("Invalid parameter\n"));
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(ConnInfo,
                  sizeof(TDI_CONNECTION_INFORMATION) +
                  TdiAddressSize);

    ConnInfo->OptionsLength = sizeof(ULONG);
    ConnInfo->RemoteAddressLength = TdiAddressSize;
    ConnInfo->RemoteAddress = TransportAddress =
        (PTRANSPORT_ADDRESS)&ConnInfo[1];

    return TdiBuildNullTransportAddressInPlace(TransportAddress, Type);
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

    TdiAddressSize = TaLengthOfTransportAddressByType(Type);
    if (!TdiAddressSize) {
        AFD_DbgPrint(MIN_TRACE,("Invalid parameter\n"));
        *ConnectionInfo = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    ConnInfo = (PTDI_CONNECTION_INFORMATION)
        ExAllocatePoolWithTag(NonPagedPool,
                              sizeof(TDI_CONNECTION_INFORMATION) + TdiAddressSize,
                              TAG_AFD_TDI_CONNECTION_INFORMATION);
    if (!ConnInfo) {
        *ConnectionInfo = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = TdiBuildNullConnectionInfoInPlace( ConnInfo, Type );

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(ConnInfo, TAG_AFD_TDI_CONNECTION_INFORMATION);
        ConnInfo = NULL;
    }

    *ConnectionInfo = ConnInfo;

    return Status;
}


NTSTATUS
TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnectionInfo,
  PTRANSPORT_ADDRESS Address ) {
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY {
        RtlCopyMemory( ConnectionInfo->RemoteAddress,
                       Address,
                       ConnectionInfo->RemoteAddressLength );
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

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
