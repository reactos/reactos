#include <afd.h>
#include "tdiconn.h"

DWORD TdiAddressSizeFromType(
  ULONG Type)
/*
 * FUNCTION: Returns the size of a TDI style address of given address type
 * ARGUMENTS:
 *     Type = TDI style address type
 * RETURNS:
 *     Size of TDI style address, 0 if Type is not valid
 */
{
  switch (Type) {
  case TDI_ADDRESS_TYPE_IP:
    return sizeof(TA_IP_ADDRESS);
  /* FIXME: More to come */
  }
  AFD_DbgPrint(MIN_TRACE, ("Unknown TDI address type (%d).\n", Type));
  return 0;
}

DWORD TdiAddressSizeFromName(
  LPSOCKADDR Name)
/*
 * FUNCTION: Returns the size of a TDI style address equivalent to a
 *           WinSock style name
 * ARGUMENTS:
 *     Name = WinSock style name
 * RETURNS:
 *     Size of TDI style address, 0 if Name is not valid
 */
{
  switch (Name->sa_family) {
  case AF_INET:
    return sizeof(TA_IP_ADDRESS);
  /* FIXME: More to come */
  }
  AFD_DbgPrint(MIN_TRACE, ("Unknown address family (%d).\n", Name->sa_family));
  return 0;
}


VOID TdiBuildAddressIPv4(
  PTA_IP_ADDRESS Address,
  LPSOCKADDR Name)
/*
 * FUNCTION: Builds an IPv4 TDI style address
 * ARGUMENTS:
 *     Address = Address of buffer to place TDI style IPv4 address
 *     Name    = Pointer to WinSock style IPv4 name
 */
{
    Address->TAAddressCount                 = 1;
    Address->Address[0].AddressLength       = TDI_ADDRESS_LENGTH_IP;
    Address->Address[0].AddressType         = TDI_ADDRESS_TYPE_IP;
    Address->Address[0].Address[0].sin_port = ((LPSOCKADDR_IN)Name)->sin_port;
    Address->Address[0].Address[0].in_addr  = ((LPSOCKADDR_IN)Name)->sin_addr.S_un.S_addr;
}


NTSTATUS TdiBuildAddress(
  PTA_ADDRESS Address,
  LPSOCKADDR Name)
/*
 * FUNCTION: Builds a TDI style address
 * ARGUMENTS:
 *     Address = Address of buffer to place TDI style address
 *     Name    = Pointer to WinSock style name
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status = STATUS_SUCCESS;

  switch (Name->sa_family) {
  case AF_INET:
      TdiBuildAddressIPv4((PTA_IP_ADDRESS)Address,Name);
      break;
      /* FIXME: More to come */
  default:
      AFD_DbgPrint(MID_TRACE, ("Unknown address family (%d).\n", Name->sa_family));
      Status = STATUS_INVALID_PARAMETER;
  }
  
  return Status;
}


NTSTATUS TdiBuildName(
  LPSOCKADDR Name,
  PTA_ADDRESS Address)
/*
 * FUNCTION: Builds a WinSock style address
 * ARGUMENTS:
 *     Name    = Address of buffer to place WinSock style name
 *     Address = Pointer to TDI style address
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status = STATUS_SUCCESS;

  switch (Address->AddressType) {
  case TDI_ADDRESS_TYPE_IP:
    Name->sa_family = AF_INET;
    ((LPSOCKADDR_IN)Name)->sin_port = 
      ((PTDI_ADDRESS_IP)&Address->Address[0])->sin_port;
    ((LPSOCKADDR_IN)Name)->sin_addr.S_un.S_addr = 
      ((PTDI_ADDRESS_IP)&Address->Address[0])->in_addr;
    break;
  /* FIXME: More to come */
  default:
    AFD_DbgPrint(MID_TRACE, ("Unknown TDI address type (%d).\n", Address->AddressType));
    Status = STATUS_INVALID_PARAMETER;
  }

  return Status;

}

NTSTATUS TdiBuildConnectionInfoInPlace
( PTDI_CONNECTION_INFORMATION ConnInfo,
  LPSOCKADDR Name ) 
/*
 * FUNCTION: Builds a TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer to place connection information
 *     Name           = Pointer to WinSock style name
 * RETURNS:
 *     Status of operation
 */
{
    ULONG TdiAddressSize;
    
    TdiAddressSize = TdiAddressSizeFromName(Name);

    RtlZeroMemory(ConnInfo,
		  sizeof(TDI_CONNECTION_INFORMATION) +
		  TdiAddressSize);
    
    ConnInfo->OptionsLength = sizeof(ULONG);
    ConnInfo->RemoteAddressLength = TdiAddressSize;
    ConnInfo->RemoteAddress = (PVOID)
	(((PCHAR)ConnInfo) + sizeof(TDI_CONNECTION_INFORMATION));
    
    TdiBuildAddress(ConnInfo->RemoteAddress, Name);
    
    return STATUS_SUCCESS;
}
    

NTSTATUS TdiBuildConnectionInfo(
  PTDI_CONNECTION_INFORMATION *ConnectionInfo,
  LPSOCKADDR Name)
/*
 * FUNCTION: Builds a TDI connection information structure
 * ARGUMENTS:
 *     ConnectionInfo = Address of buffer pointer to allocate connection 
 *                      information on
 *     Name           = Pointer to WinSock style name
 * RETURNS:
 *     Status of operation
 */
{
    PTDI_CONNECTION_INFORMATION ConnInfo;
    ULONG TdiAddressSize;
    NTSTATUS Status;
    
    TdiAddressSize = TdiAddressSizeFromName(Name);
    
    ConnInfo = (PTDI_CONNECTION_INFORMATION)
	ExAllocatePool(NonPagedPool,
		       sizeof(TDI_CONNECTION_INFORMATION) +
		       TdiAddressSize);
    if (!ConnInfo)
	return STATUS_INSUFFICIENT_RESOURCES;
    
    Status = TdiBuildConnectionInfoInPlace( ConnInfo, Name );

    if( !NT_SUCCESS(Status) )
	ExFreePool( ConnInfo );
    else 
	*ConnectionInfo = ConnInfo;
    
    return Status;
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
  
  TdiAddressSize = TdiAddressSizeFromType(Type);

  RtlZeroMemory(ConnInfo,
    sizeof(TDI_CONNECTION_INFORMATION) +
    TdiAddressSize);

  ConnInfo->OptionsLength = sizeof(ULONG);
  ConnInfo->RemoteAddressLength = 0;
  ConnInfo->RemoteAddress = NULL;

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
  if (!ConnInfo)
    return STATUS_INSUFFICIENT_RESOURCES;

  Status = TdiBuildNullConnectionInfoInPlace( ConnInfo, Type );

  if (!NT_SUCCESS(Status)) 
      ExFreePool( ConnInfo );
  else
      *ConnectionInfo = ConnInfo;

  return Status;
}

NTSTATUS 
TdiBuildConnectionInfoPair
( PTDI_CONNECTION_INFO_PAIR ConnectionInfo,
  LPSOCKADDR From, LPSOCKADDR To ) 
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
    DWORD SizeOfEntry;
    ULONG TdiAddressSize;

    /* FIXME: Get from socket information */
    TdiAddressSize = TdiAddressSizeFromName(From);
    SizeOfEntry = TdiAddressSize + sizeof(TDI_CONNECTION_INFORMATION);

    LayoutFrame = (PCHAR)ExAllocatePool(NonPagedPool, 2 * SizeOfEntry);

    if (!LayoutFrame) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( LayoutFrame, 2 * SizeOfEntry );

    PTDI_CONNECTION_INFORMATION 
	FromTdiConn = (PTDI_CONNECTION_INFORMATION)LayoutFrame, 
	ToTdiConn = (PTDI_CONNECTION_INFORMATION)LayoutFrame + SizeOfEntry;

    if (From != NULL) {
	TdiBuildConnectionInfoInPlace( FromTdiConn, From );
    } else {
	TdiBuildNullConnectionInfoInPlace( FromTdiConn, 
					   From->sa_family );
    }

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

