/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ghname.c

Abstract:

    This module implements routines to set and retrieve the host's TCP/IP
    network name.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9/20/91     created

Notes:

    Exports:
        GETXBYYSP_gethostname()
        sethostname()

--*/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.gethostname.c
 *      @(#)gethostname.c       5.3
 *
 *      Last delta created      14:11:24 3/4/91
 *      This file extracted     11:20:27 3/8/91
 *
 *      GET/SETHOSTNAME library routines
 *
 *      Modifications:
 *
 *      2 Nov 1990 (RAE)       New File
 */
/****************************************************************************/

#include "winsockp.h"

int
_pgethostname(
    OUT char *name,
    IN int namelen
    );



int
_pgethostname(
    OUT char *name,
    IN int namelen
    )
{
    int retval = 0;
    PUCHAR     temp;
    HANDLE     myKey;
    NTSTATUS   status;
    ULONG      myType;
    PUCHAR     domain;

    WS_ENTER( "GETXBYYSP_gethostname", name, UIntToPtr( namelen ), NULL, NULL );

    if ( !SockEnterApi( FALSE, TRUE, TRUE ) ) {
        WS_EXIT( "GETXBYYSP_gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }


    status = SockOpenKeyEx( &myKey, VTCPPARM, NTCPPARM, TCPPARM );
    if (!NT_SUCCESS(status)) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("Required Registry Key is missing -- %s\n", NTCPPARM));
        }
        SetLastError( WSAEINVAL );
        WS_EXIT( "GETXBYYSP_gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    if ((temp=ALLOCATE_HEAP(HOSTDB_SIZE))==NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("Out of memory!\n"));
        }
        NtClose(myKey);
        SetLastError( WSAEINVAL );
        WS_EXIT( "GETXBYYSP_gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    status = SockGetSingleValue(myKey, "Hostname", temp, &myType, HOSTDB_SIZE);

    if (!NT_SUCCESS(status)) {
        NtClose(myKey);
        FREE_HEAP(temp);
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("ERROR - Hostname not set in Registry.\n"));
        }
        SetLastError( WSAENETDOWN );
        WS_EXIT( "GETXBYYSP_gethostname", SOCKET_ERROR, TRUE );
        return(SOCKET_ERROR);
    }

    //
    // Do not return the fully qualified name--returning this much info
    // breaks some poorly written applications.
    //

#if 0
    domain = temp + strlen(temp) + 1;

    status = SockGetSingleValue(myKey, "Domain", domain, &myType, HOSTDB_SIZE);
    if (!NT_SUCCESS(status) || (strlen(temp) == 0)) {
        status = SockGetSingleValue(myKey, "DhcpDomain", temp,
                                                &myType, HOSTDB_SIZE);
    }

    if ( NT_SUCCESS(status) && strlen(domain) > 0 ) {
        *(domain - 1) = '.';
    }
#endif

    NtClose(myKey);

    if ((strlen(temp)>(unsigned int)namelen) || namelen<0) {
        FREE_HEAP(temp);
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("ERROR - Namelen parameter too small: %ld\n", namelen));
        }
        SetLastError( WSAEFAULT );
        WS_EXIT( "GETXBYYSP_gethostname", SOCKET_ERROR, TRUE );
        return(SOCKET_ERROR);
    }

    strcpy(name, temp);
    FREE_HEAP(temp);
    WS_EXIT( "GETXBYYSP_gethostname", retval, FALSE );
    return (retval);
}


//
// BUGBUG - this function may only be performed by the sys manager. Must
//          add security. Actually, this function will probably go away
//          since the NCAP will take care of it.



