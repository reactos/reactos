/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    GetServ.c

Abstract:

    This module contains support for the getprotobyY WinSock APIs.

Author:

    David Treadwell (davidtr)    29-Jun-1992

Revision History:

--*/

#include "winsockp.h"

#define SERVDB          ACCESS_THREAD_DATA( SERVDB, GETSERV )
#define servf           ACCESS_THREAD_DATA( servf, GETSERV )
#define line            ACCESS_THREAD_DATA( line, GETSERV )
#define serv            ACCESS_THREAD_DATA( serv, GETSERV )
#define serv_aliases    ACCESS_THREAD_DATA( serv_aliases, GETSERV )
#define stayopen        ACCESS_THREAD_DATA( stayopen, GETSERV )

static char *any();

struct servent *
_pgetservebyport(
    IN const int port,
    IN const char *name
    );

struct servent *
_pgetservebyname(
    IN const char *name,
    IN const char *proto
    );

DWORD
BytesInServent (
    IN PSERVENT Servent
    );

DWORD
CopyServentToBuffer (
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    );


void
setservent (
    int f
    )
{
    if (servf == NULL) {
        servf = SockOpenNetworkDataBase("services", SERVDB, SERVDB_SIZE, "rt");
    } else {
        rewind(servf);
    }

    stayopen |= f;

} // setservent


void
endservent (
    void
    )
{
    if (servf && !stayopen) {
        fclose(servf);
        servf = NULL;
    }

} // endservent


struct servent *
getservent (
    void
    )
{
    char *p;
    register char *cp, **q;

    if (servf == NULL && (servf = fopen(SERVDB, "rt" )) == NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("\tERROR: cannot open services database file %s\n",
                          SERVDB));
        }
        return (NULL);
    }

again:

    if ((p = fgets(line, BUFSIZ, servf)) == NULL) {
        return (NULL);
    }

    if (*p == '#') {
        goto again;
    }

    cp = any(p, "#\n");

    if (cp == NULL) {
        goto again;
    }

    *cp = '\0';
    serv.s_name = p;
    p = any(p, " \t");

    if (p == NULL) {
        goto again;
    }

    *p++ = '\0';

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    cp = any(p, ",/");

    if (cp == NULL) {
        goto again;
    }

    *cp++ = '\0';
    serv.s_port = htons((USHORT)atoi(p));
    serv.s_proto = cp;
    q = serv.s_aliases = serv_aliases;
    cp = any(cp, " \t");

    if (cp != NULL) {
        *cp++ = '\0';
    }

    while (cp && *cp) {

        if (*cp == ' ' || *cp == '\t') {
                cp++;
                continue;
        }

        if (q < &serv_aliases[MAXALIASES - 1]) {
            *q++ = cp;
        }

        cp = any(cp, " \t");

        if (cp != NULL) {
            *cp++ = '\0';
        }
    }

    *q = NULL;
    return (&serv);

} // getservent

static char *
any (
    IN char *cp,
    IN char *match
    )
{
    register char *mp, c;

    while (c = *cp) {

        for (mp = match; *mp; mp++) {
            if (*mp == c) {
                return (cp);
            }
        }

        cp++;
    }

    return ((char *)0);

} // any



struct servent *
_pgetservebyname(
    IN const char *name,
    IN const char *proto
    )
/*++
Routine Description:
    Internal form of above
--*/
{
    register struct servent *p;
    register char **cp;

    WS_ENTER( "GETXBYYSP_getservbyname", (PVOID)name, (PVOID)proto, NULL, NULL );

    if ( !SockEnterApi( FALSE, TRUE, TRUE ) ) {
        WS_EXIT( "GETXBYYSP_getservbyname", 0, TRUE );
        return NULL;
    }

    setservent(0);

    while (p = getservent()) {

        if (_stricmp(name, p->s_name) == 0) {
            goto gotname;
        }

        for (cp = p->s_aliases; *cp; cp++) {
            if (_stricmp(name, *cp) == 0) {
                goto gotname;
            }
        }

        continue;

gotname:
        if (proto == 0 || _stricmp(p->s_proto, proto) == 0) {
            break;
        }
    }

    endservent();

    SockThreadProcessingGetXByY = FALSE;

    if ( p == NULL ) {
        SetLastError( WSANO_DATA );
    }

    WS_EXIT( "GETXBYYSP_getservbyname", (INT)((ULONG_PTR)p), FALSE );
    return (p);
}

//
// internal form of above
//
struct servent *
_pgetservebyport(
    IN const int port,
    IN const char *proto
    )
{
    register struct servent *p;

    WS_ENTER( "GETXBYYSP_getservbyport", (PVOID)port, (PVOID)proto, NULL, NULL );

    if ( !SockEnterApi( FALSE, TRUE, TRUE ) ) {
        WS_EXIT( "GETXBYYSP_getservbyport", 0, TRUE );
        return NULL;
    }

    setservent(0);

    while (p = getservent()) {

        if ((unsigned short)p->s_port != (unsigned short)port) {
            continue;
        }

        if (proto == 0 || _stricmp(p->s_proto, proto) == 0) {
            break;
        }
    }

    endservent();

    SockThreadProcessingGetXByY = FALSE;

    if ( p == NULL ) {
        SetLastError( WSANO_DATA );
    }

    WS_EXIT( "GETXBYYSP_getservbyport", (INT)((ULONG_PTR)p), FALSE );
    return (p);

} // GETXBYYSP_getservbyport



DWORD
CopyServentToBuffer (
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD i;
    PSERVENT outputServent = (PSERVENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInServent( Servent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        RtlZeroMemory( Buffer, requiredBufferLength );
    } else {
        RtlZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the servent structure if it fits.
    //

    bytesFilled = sizeof(*Servent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    RtlCopyMemory( currentLocation, Servent, sizeof(*Servent) );
    currentLocation = Buffer + bytesFilled;

    outputServent->s_name = NULL;
    outputServent->s_aliases = NULL;
    outputServent->s_proto = NULL;

    //
    // Count the service's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Servent->s_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Servent->s_aliases = NULL;
        return requiredBufferLength;
    }

    outputServent->s_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the service name if it fits.
    //

    bytesFilled += strlen( Servent->s_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_name = currentLocation;

    RtlCopyMemory( currentLocation, Servent->s_name, strlen( Servent->s_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Servent->s_proto ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_proto = currentLocation;

    RtlCopyMemory( currentLocation, Servent->s_proto, strlen( Servent->s_proto ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Servent->s_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputServent->s_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputServent->s_aliases[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Servent->s_aliases[i],
            strlen( Servent->s_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputServent->s_aliases[i] = NULL;

    return requiredBufferLength;

} // CopyServentToBuffer


DWORD
BytesInServent (
    IN PSERVENT Servent
    )
{
    DWORD total;
    int i;

    total = sizeof(SERVENT);
    total += strlen( Servent->s_name ) + 1;
    total += strlen( Servent->s_proto ) + 1;
    total += sizeof(char *);

    for ( i = 0; Servent->s_aliases[i] != NULL; i++ ) {
        total += strlen( Servent->s_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

} // BytesInServent



