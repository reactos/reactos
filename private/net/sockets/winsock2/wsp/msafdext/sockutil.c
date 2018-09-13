/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sockutil.c

Abstract:

    General socket utilities.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#include "msafdext.h"
#include <wsipx.h>
#include <wsnetbs.h>


//
//  Private constants.
//

#define ACTUAL_ADDRESS(a,s,f)   \
            ( (ULONG_PTR)(a) + ( (PUCHAR)(&(s)->f) - (PUCHAR)(s) ) )

#define IS_LIST_EMPTY(a,s,f)    \
            ( (ULONG_PTR)(s)->f.Flink == ACTUAL_ADDRESS(a,s,f) )


//
// Private globals.
//

PSTR SocketStates[] =
     {
         "Open",
         "Bound",
         "BoundSpecific",
         "Connected",
         "Closing"
     };


//
// Private prototypes.
//

PSTR
SocketStateToString(
    SOCKET_STATE State
    );

PSTR
NetbiosNameTypeToString(
    USHORT NetbiosNameType
    );

PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    );


//
// Public functions.
//

VOID
DumpSocket(
    PSOCKET_INFORMATION Socket,
    ULONG_PTR ActualAddress
    )

/*++

Routine Description:

    Dumps the specified SOCKET_INFORMATION structure.

Arguments:

    Socket - Points to the SOCKET_INFORMATION to dump.

    ActualAddress - The actual address where the structure resides in the
        target process.

Return Value:

    None.

--*/

{

    SOCKADDR addr;
    ULONG result;

    dprintf ((
        "SOCKET_INFORMATION @ %08lx:\n",
        ActualAddress
        ));

    dprintf ((
        "    State                          = %s\n",
        SocketStateToString( Socket->State )
        ));

    dprintf ((
        "    ReferenceCount                 = %lu\n",
        Socket->ReferenceCount
        ));

    dprintf ((
        "    Handle                         = %08lx\n",
        Socket->Handle
        ));

    dprintf ((
        "    AddressFamily                  = %d\n",
        Socket->AddressFamily
        ));

    dprintf ((
        "    SocketType                     = %d\n",
        Socket->SocketType
        ));

    dprintf ((
        "    Protocol                       = %d\n",
        Socket->Protocol
        ));

    dprintf ((
        "    HelperDll                      = %08lx\n",
        Socket->HelperDll
        ));

    dprintf ((
        "    HelperdllContext               = %08lx\n",
        Socket->HelperDllContext
        ));

    dprintf ((
        "    HelperdllNotificationEvents    = %08lx\n",
        Socket->HelperDllNotificationEvents
        ));

    dprintf ((
        "    LocalAddress                   = %08lx\n",
        Socket->LocalAddress
        ));

    dprintf ((
        "    LocalAddressLength             = %lu\n",
        Socket->LocalAddressLength
        ));

    if( Socket->LocalAddress != NULL &&
        Socket->LocalAddressLength == sizeof(addr) ) {

        if( ReadMemory(
                (ULONG_PTR)Socket->LocalAddress,
                &addr,
                sizeof(addr),
                &result
                ) ) {

            DumpSockaddr(
                "        ",
                &addr,
                (ULONG_PTR)Socket->LocalAddress
                );

        }

    }

    dprintf ((
        "    RemoteAddress                  = %08lx\n",
        Socket->RemoteAddress
        ));

    dprintf ((
        "    RemoteAddressLength            = %lu\n",
        Socket->RemoteAddressLength
        ));

    if( Socket->RemoteAddress != NULL &&
        Socket->RemoteAddressLength == sizeof(addr) ) {

        if( ReadMemory(
                (ULONG_PTR)Socket->RemoteAddress,
                &addr,
                sizeof(addr),
                &result
                ) ) {

            DumpSockaddr(
                "        ",
                &addr,
                (ULONG_PTR)Socket->RemoteAddress
                );

        }

    }

    dprintf ((
        "    TdiAddressHandle               = %08lx\n",
        Socket->TdiAddressHandle
        ));

    dprintf ((
        "    TdiConnectionHandle            = %08lx\n",
        Socket->TdiConnectionHandle
        ));

    dprintf ((
        "    AsyncSelectSerialNumber        = %lu\n",
        Socket->AsyncSelectSerialNumber
        ));

    dprintf ((
        "    AsyncSelecthWnd                = %08lx\n",
        Socket->AsyncSelecthWnd
        ));

    dprintf ((
        "    AsyncSelectwMsg                = %08lx\n",
        Socket->AsyncSelectwMsg
        ));

    dprintf ((
        "    AsyncSelectlEvent              = %08lx\n",
        Socket->AsyncSelectlEvent
        ));

    dprintf ((
        "    DisabledAsyncSelectEvents      = %08lx\n",
        Socket->DisabledAsyncSelectEvents
        ));

    dprintf ((
        "    EventSelectEventObject         = %08lx\n",
        Socket->EventSelectEventObject
        ));

    dprintf ((
        "    EventSelectlNetworkEvents      = %08lx\n",
        Socket->EventSelectlNetworkEvents
        ));

    dprintf ((
        "    LingerInfo                     = (%d,%d)\n",
        Socket->LingerInfo.l_onoff,
        Socket->LingerInfo.l_linger
        ));

    dprintf ((
        "    SendTimeout                    = %lu\n",
        Socket->SendTimeout
        ));

    dprintf ((
        "    ReceiveTimeout                 = %lu\n",
        Socket->ReceiveTimeout
        ));

    dprintf ((
        "    ReceiveBufferSize              = %lu\n",
        Socket->ReceiveBufferSize
        ));

    dprintf ((
        "    SendBufferSize                 = %lu\n",
        Socket->SendBufferSize
        ));

    dprintf ((
        "    Listening                      = %s\n",
        BooleanToString( Socket->Listening )
        ));

    dprintf ((
        "    Broadcast                      = %s\n",
        BooleanToString( Socket->Broadcast )
        ));

    dprintf ((
        "    Debug                          = %s\n",
        BooleanToString( Socket->Debug )
        ));

    dprintf ((
        "    OobInline                      = %s\n",
        BooleanToString( Socket->OobInline )
        ));

    dprintf ((
        "    ReuseAddresses                 = %s\n",
        BooleanToString( Socket->ReuseAddresses )
        ));

    dprintf ((
        "    NonBlocking                    = %s\n",
        BooleanToString( Socket->NonBlocking )
        ));

    dprintf ((
        "    DontUseWildcard                = %s\n",
        BooleanToString( Socket->DontUseWildcard )
        ));

    dprintf ((
        "    ReceiveShutdown                = %s\n",
        BooleanToString( Socket->ReceiveShutdown )
        ));

    dprintf ((
        "    SendShutdown                   = %s\n",
        BooleanToString( Socket->SendShutdown )
        ));

    dprintf ((
        "    Lock                           @ %08lx\n",
        ACTUAL_ADDRESS(
            ActualAddress,
            Socket,
            Lock
            )
        ));

    dprintf ((
        "    CreationFlags                  = %08lx\n",
        Socket->CreationFlags
        ));

    dprintf ((
        "    CatalogEntryId                 = %08lx\n",
        Socket->CatalogEntryId
        ));

    dprintf ((
        "    LastError                      = %d\n",
        Socket->LastError
        ));

    dprintf ((
        "    GroupID                        = %d\n",
        Socket->GroupID
        ));

    dprintf ((
        "    GroupType                      = %s\n",
        GroupTypeToString( Socket->GroupType )
        ));

    dprintf ((
        "    GroupPriority                  = %d\n",
        Socket->GroupPriority
        ));

#ifdef _AFD_SAN_SWITCH_
    dprintf ((
        "    SanSocket                      = %p\n",
        Socket->SanSocket
        ));
#endif // _AFD_SAN_SWITCH_

    dprintf (( "\n" ));

}   // DumpSocket


VOID
DumpSockaddr(
    PSTR Prefix,
    PSOCKADDR Sockaddr,
    ULONG_PTR ActualAddress
    )
{

    //
    // Hack for uninitialized addresses.
    //

    if( Sockaddr->sa_family == 0 ) {

        return;

    }

    dprintf ((
        "%sSOCKADDR @ %08lx\n",
        Prefix,
        ActualAddress
        ));

    switch( Sockaddr->sa_family ) {

    case AF_INET : {

        PSOCKADDR_IN ipAddr;

        ipAddr = (PSOCKADDR_IN)Sockaddr;

        dprintf ((
            "%s    sin_family = %d (IP)\n",
            Prefix,
            ipAddr->sin_family
            ));

        dprintf ((
            "%s    sin_port   = %u\n",
            Prefix,
            NTOHS( ipAddr->sin_port )
            ));

        dprintf ((
            "%s    sin_addr   = %d.%d.%d.%d\n",
            Prefix,
            UC( ipAddr->sin_addr.s_addr >>  0 ),
            UC( ipAddr->sin_addr.s_addr >>  8 ),
            UC( ipAddr->sin_addr.s_addr >> 16 ),
            UC( ipAddr->sin_addr.s_addr >> 24 )
            ));

        }
        break;

    case AF_IPX : {

        PSOCKADDR_IPX ipxAddr;

        ipxAddr = (PSOCKADDR_IPX)Sockaddr;

        dprintf ((
            "%s    sa_family  = %d (IPX)\n",
            Prefix,
            ipxAddr->sa_family
            ));

        dprintf ((
            "%s    sa_netnum  = %02.2X-%02.2X-%02.2X-%02.2X\n",
            Prefix,
            ipxAddr->sa_netnum[0],
            ipxAddr->sa_netnum[1],
            ipxAddr->sa_netnum[2],
            ipxAddr->sa_netnum[3]
            ));

        dprintf ((
            "%s    sa_nodenum = %02.2X-%02.2X-%02.2X-%02.2X-%02.2X-%02.2X\n",
            Prefix,
            ipxAddr->sa_nodenum[0],
            ipxAddr->sa_nodenum[1],
            ipxAddr->sa_nodenum[2],
            ipxAddr->sa_nodenum[3],
            ipxAddr->sa_nodenum[4],
            ipxAddr->sa_nodenum[5]
            ));

        dprintf ((
            "%s    sa_socket  = %04.4X\n",
            Prefix,
            ipxAddr->sa_socket
            ));

        }
        break;

    case AF_NETBIOS : {
        INT i;

        PSOCKADDR_NB netbiosAddr;
        UCHAR netbiosName[NETBIOS_NAME_LENGTH + 1];

        netbiosAddr = (PSOCKADDR_NB)Sockaddr;

        dprintf ((
            "%s    snb_family = %d (NetBIOS)\n",
            Prefix,
            netbiosAddr->snb_family
            ));

        for (i=0;i<NETBIOS_NAME_LENGTH-1;i++) {
            if (isprint (netbiosAddr->snb_name[i])) {
                dprintf (("%c", netbiosAddr->snb_name[i]));
            }
            else {
                dprintf (("."));
            }
        }
        dprintf (("("));
        for (i=0;i<NETBIOS_NAME_LENGTH-1;i++) {
            dprintf (("%2.2lx ", netbiosAddr->snb_name[i]));
        }

        dprintf (("), port 0x%lx, type 0x%lx\n",
            netbiosAddr->snb_name[NETBIOS_NAME_LENGTH-1],
            netbiosAddr->snb_type));

        }
        break;

    default : {
        INT i;
        dprintf(( "%s    Family %ld, ", Sockaddr->sa_family ));
        for (i=0; i<sizeof(Sockaddr->sa_data); i++) {
            dprintf (("%2.2lx ", Sockaddr->sa_data[i]));
        }

        dprintf (("\n"));
        }
        break;

    }

}   // DumpSockaddr


//
// Private functions.
//

PSTR
SocketStateToString(
    SOCKET_STATE State
    )
{

    static CHAR invalidString[32];

    if( State >= SocketStateOpen &&
        State <= SocketStateClosing ) {

        return SocketStates[State];

    }

    sprintf(
        invalidString,
        "INVALID STATE %d",
        State
        );

    return invalidString;

}   // SocketStateToString

PSTR
NetbiosNameTypeToString(
    USHORT NetbiosNameType
    )

/*++

Routine Description:

    Maps a NetBIOS name type to a displayable string.

Arguments:

    NetbiosNameType - The NetBIOS name type to map.

Return Value:

    PSTR - Points to the displayable form of the NetBIOS name type.

--*/

{

    static CHAR invalidString[32];

    switch( NetbiosNameType ) {

    case NETBIOS_UNIQUE_NAME :

        return "Unique";

    case NETBIOS_GROUP_NAME :

        return "Group";

    case NETBIOS_TYPE_QUICK_UNIQUE :

        return "Quick Unique";

    case NETBIOS_TYPE_QUICK_GROUP :

        return "Quick Group";

    }

    sprintf(
        invalidString,
        "INVALID TYPE %u\n",
        NetbiosNameType
        );

    return invalidString;

}   // NetbiosNameTypeToString



PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Maps an AFD_GROUP_TYPE to a displayable string.

Arguments:

    GroupType - The AFD_GROUP_TYPE to map.

Return Value:

    PSTR - Points to the displayable form of the AFD_GROUP_TYPE.

--*/

{

    static CHAR invalidString[32];

    switch( GroupType ) {

    case GroupTypeNeither :
        return "Neither";

    case GroupTypeConstrained :
        return "Constrained";

    case GroupTypeUnconstrained :
        return "Unconstrained";

    }

    sprintf(
        invalidString,
        "INVALID TYPE %d\n",
        GroupType
        );

    return invalidString;
    
}   // GroupTypeToString

