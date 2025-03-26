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

#include <ntifs.h>
#include <ipifcons.h>

ULONG NextDefaultAdapter = 0;

NTSTATUS GetInterfaceIPv4Address( PIP_INTERFACE Interface,
				  ULONG TargetType,
				  PULONG Address ) {
    switch( TargetType ) {
    case ADE_UNICAST:
	*Address = Interface->Unicast.Address.IPv4Address;
	break;

    case ADE_ADDRMASK:
	*Address = Interface->Netmask.Address.IPv4Address;
	break;

    case ADE_BROADCAST:
	*Address = Interface->Broadcast.Address.IPv4Address;
	break;

    case ADE_POINTOPOINT:
	*Address = Interface->PointToPoint.Address.IPv4Address;
	break;

    default:
	return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

UINT CountInterfaces() {
    ULONG Count = 0;
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    ForEachInterface(CurrentIF) {
	Count++;
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return Count;
}

NTSTATUS GetInterfaceSpeed( PIP_INTERFACE Interface, PUINT Speed ) {
    PLAN_ADAPTER IF = (PLAN_ADAPTER)Interface->Context;

    *Speed = IF->Speed;

    return STATUS_SUCCESS;
}

NTSTATUS GetInterfaceName( PIP_INTERFACE Interface,
			   PCHAR NameBuffer,
			   UINT Len ) {
    ULONG ResultSize = 0;
    NTSTATUS Status =
	RtlUnicodeToMultiByteN( NameBuffer,
				Len,
				&ResultSize,
				Interface->Name.Buffer,
				Interface->Name.Length );

    if( NT_SUCCESS(Status) )
	NameBuffer[ResultSize] = 0;
    else
	NameBuffer[0] = 0;

    return Status;
}

PIP_INTERFACE AddrLocateInterface(
    PIP_ADDRESS MatchAddress)
{
    KIRQL OldIrql;
    PIP_INTERFACE RetIF = NULL;
    IF_LIST_ITER(CurrentIF);

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    ForEachInterface(CurrentIF) {
	if( AddrIsEqual( &CurrentIF->Unicast, MatchAddress ) ||
            AddrIsEqual( &CurrentIF->Broadcast, MatchAddress ) ) {
            RetIF = CurrentIF;
            break;
	}
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return RetIF;
}

BOOLEAN HasPrefix(
    PIP_ADDRESS Address,
    PIP_ADDRESS Prefix,
    UINT Length)
/*
 * FUNCTION: Determines wether an address has an given prefix
 * ARGUMENTS:
 *     Address = Pointer to address to use
 *     Prefix  = Pointer to prefix to check for
 *     Length  = Length of prefix
 * RETURNS:
 *     TRUE if the address has the prefix, FALSE if not
 * NOTES:
 *     The two addresses must be of the same type
 */
{
    PUCHAR pAddress = (PUCHAR)&Address->Address;
    PUCHAR pPrefix  = (PUCHAR)&Prefix->Address;

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Address (0x%X)  Prefix (0x%X)  Length (%d).\n", Address, Prefix, Length));

#if 0
    TI_DbgPrint(DEBUG_ROUTER, ("Address (%s)  Prefix (%s).\n",
        A2S(Address), A2S(Prefix)));
#endif

    /* Don't report matches for empty prefixes */
    if (Length == 0) {
        return FALSE;
    }

    /* Check that initial integral bytes match */
    while (Length > 8) {
        if (*pAddress++ != *pPrefix++)
            return FALSE;
        Length -= 8;
    }

    /* Check any remaining bits */
    if ((Length > 0) && ((*pAddress >> (8 - Length)) != (*pPrefix >> (8 - Length))))
        return FALSE;

    return TRUE;
}

PIP_INTERFACE GetDefaultInterface(VOID)
{
   KIRQL OldIrql;
   ULONG Index = 0;
   ULONG IfStatus;

   IF_LIST_ITER(CurrentIF);

   TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
   /* DHCP hack: Always return the adapter without an IP address */
   ForEachInterface(CurrentIF) {
      if (CurrentIF->Context && AddrIsUnspecified(&CurrentIF->Unicast)) {
          TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

          GetInterfaceConnectionStatus(CurrentIF, &IfStatus);
          if (IfStatus == MIB_IF_OPER_STATUS_OPERATIONAL) {
              return CurrentIF;
          }

          TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
      }
   } EndFor(CurrentIF);

   /* Try to continue from the next adapter */
   ForEachInterface(CurrentIF) {
      if (CurrentIF->Context && (Index++ == NextDefaultAdapter)) {
          TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

          GetInterfaceConnectionStatus(CurrentIF, &IfStatus);
          if (IfStatus == MIB_IF_OPER_STATUS_OPERATIONAL) {
              NextDefaultAdapter++;
              return CurrentIF;
          }

          TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
      }
   } EndFor(CurrentIF);

   /* No luck, so we'll choose the first adapter this time */
   Index = 0;
   ForEachInterface(CurrentIF) {
      if (CurrentIF->Context) {
          Index++;
          TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

          GetInterfaceConnectionStatus(CurrentIF, &IfStatus);
          if (IfStatus == MIB_IF_OPER_STATUS_OPERATIONAL) {
              NextDefaultAdapter = Index;
              return CurrentIF;
          }

          TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);
      }
   } EndFor(CurrentIF);

   /* Even that didn't work, so we'll just go with loopback */
   NextDefaultAdapter = 0;
   TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

   /* There are no physical interfaces on the system
    * so we must pick the loopback interface */

   return Loopback;
}

PIP_INTERFACE FindOnLinkInterface(PIP_ADDRESS Address)
/*
 * FUNCTION: Checks all on-link prefixes to find out if an address is on-link
 * ARGUMENTS:
 *     Address = Pointer to address to check
 * RETURNS:
 *     Pointer to interface if address is on-link, NULL if not
 */
{
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Address (0x%X)\n", Address));
    TI_DbgPrint(DEBUG_ROUTER, ("Address (%s)\n", A2S(Address)));

    if (AddrIsUnspecified(Address))
        return GetDefaultInterface();

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    ForEachInterface(CurrentIF) {
        if (HasPrefix(Address, &CurrentIF->Unicast,
		      AddrCountPrefixBits(&CurrentIF->Netmask))) {
	    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);
            return CurrentIF;
	}
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return NULL;
}

VOID GetInterfaceConnectionStatus(PIP_INTERFACE Interface, PULONG Result)
{
    PLAN_ADAPTER Adapter = Interface->Context;

    /* Loopback has no adapter context */
    if (Adapter == NULL || Adapter->State == LAN_STATE_STARTED) {
        *Result = MIB_IF_OPER_STATUS_OPERATIONAL;
    }
    else {
        *Result = MIB_IF_OPER_STATUS_DISCONNECTED;
    }
}
