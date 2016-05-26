/*
 * PROJECT:     ReactOS nslookup utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/nslookup/nslookup.c
 * PURPOSE:     Perform DNS lookups
 * COPYRIGHT:   Copyright 2009 Lucas Suggs <lucas.suggs@gmail.com>
 */

#include "nslookup.h"

#include <winbase.h>
#include <iphlpapi.h>

STATE   State;
HANDLE  ProcessHeap;
ULONG   RequestID;

void PrintState()
{
    _tprintf( _T("Default Server:  (null)\n\n") );
    _tprintf( _T("Set options:\n") );

    _tprintf( _T("  ") );
    if( !State.debug ) _tprintf( _T("no") );
    _tprintf( _T("debug\n") );

    _tprintf( _T("  ") );
    if( !State.defname ) _tprintf( _T("no") );
    _tprintf( _T("defname\n") );

    _tprintf( _T("  ") );
    if( !State.search ) _tprintf( _T("no") );
    _tprintf( _T("search\n") );

    _tprintf( _T("  ") );
    if( !State.recurse ) _tprintf( _T("no") );
    _tprintf( _T("recurse\n") );

    _tprintf( _T("  ") );
    if( !State.d2 ) _tprintf( _T("no") );
    _tprintf( _T("d2\n") );

    _tprintf( _T("  ") );
    if( !State.vc ) _tprintf( _T("no") );
    _tprintf( _T("vc\n") );

    _tprintf( _T("  ") );
    if( !State.ignoretc ) _tprintf( _T("no") );
    _tprintf( _T("ignoretc\n") );

    _tprintf( _T("  port=%d\n"), State.port );
    _tprintf( _T("  type=%s\n"), State.type );
    _tprintf( _T("  class=%s\n"), State.Class );
    _tprintf( _T("  timeout=%d\n"), (int)State.timeout );
    _tprintf( _T("  retry=%d\n"), (int)State.retry );
    _tprintf( _T("  root=%s\n"), State.root );
    _tprintf( _T("  domain=%s\n"), State.domain );

    _tprintf( _T("  ") );
    if( !State.MSxfr ) _tprintf( _T("no") );
    _tprintf( _T("MSxfr\n") );

    _tprintf( _T("  IXFRversion=%d\n"), (int)State.ixfrver );

    _tprintf( _T("  srchlist=%s\n\n"), State.srchlist[0] );
}

void PrintUsage()
{
    _tprintf( _T("Usage:\n"
                 "   nslookup [-opt ...]             # interactive mode using"
                 " default server\n   nslookup [-opt ...] - server    #"
                 " interactive mode using 'server'\n   nslookup [-opt ...]"
                 " host        # just look up 'host' using default server\n"
                 "   nslookup [-opt ...] host server # just look up 'host'"
                 " using 'server'\n") );
}

BOOL PerformInternalLookup( PCHAR pAddr, PCHAR pResult )
{
    /* Needed to issue DNS packets and parse them. */
    PCHAR Buffer = NULL, RecBuffer = NULL;
    CHAR pResolve[256];
    ULONG BufferLength = 0, RecBufferLength = 512;
    int i = 0, j = 0, k = 0, d = 0;
    BOOL bOk = FALSE;

    /* Makes things easier when parsing the response packet. */
    USHORT NumQuestions;
    USHORT Type;

    if( (strlen( pAddr ) + 1) > 255 ) return FALSE;

    Type = TYPE_A;
    if( IsValidIP( pAddr ) ) Type = TYPE_PTR;

    /* If it's a PTR lookup then append the ARPA sig to the end. */
    if( Type == TYPE_PTR )
    {
        ReverseIP( pAddr, pResolve );
        strcat( pResolve, ARPA_SIG );
    }
    else
    {
        strcpy( pResolve, pAddr );
    }

    /* Base header length + length of QNAME + length of QTYPE and QCLASS */
    BufferLength = 12 + (strlen( pResolve ) + 2) + 4;

    /* Allocate memory for the buffer. */
    Buffer = HeapAlloc( ProcessHeap, 0, BufferLength );
    if( !Buffer )
    {
        _tprintf( _T("ERROR: Out of memory\n") );
        goto cleanup;
    }

    /* Allocate the receiving buffer. */
    RecBuffer = HeapAlloc( ProcessHeap, 0, RecBufferLength );
    if( !RecBuffer )
    {
        _tprintf( _T("ERROR: Out of memory\n") );
        goto cleanup;
    }

    /* Insert the ID field. */
    ((PSHORT)&Buffer[i])[0] = htons( RequestID );
    i += 2;

    /* Bits 0-7 of the second 16 are all 0, except for when recursion is
       desired. */
    Buffer[i] = 0x00;
    if( State.recurse) Buffer[i] |= 0x01;
    i += 1;

    /* Bits 8-15 of the second 16 are 0 for a query. */
    Buffer[i] = 0x00;
    i += 1;

    /* Only 1 question. */
    ((PSHORT)&Buffer[i])[0] = htons( 1 );
    i += 2;

    /* We aren't sending a response, so 0 out the rest of the header. */
    Buffer[i] = 0x00;
    Buffer[i + 1] = 0x00;
    Buffer[i + 2] = 0x00;
    Buffer[i + 3] = 0x00;
    Buffer[i + 4] = 0x00;
    Buffer[i + 5] = 0x00;
    i += 6;

    /* Walk through the query address. Split each section delimited by '.'.
       Format of the QNAME section is length|data, etc. Last one is null */
    j = i;
    i += 1;

    for( k = 0; k < strlen( pResolve ); k += 1 )
    {
        if( pResolve[k] != '.' )
        {
            Buffer[i] = pResolve[k];
            i += 1;
        }
        else
        {
            Buffer[j] = (i - j) - 1;
            j = i;
            i += 1;
        }
    }

    Buffer[j] = (i - j) - 1;
    Buffer[i] = 0x00;
    i += 1;

    /* QTYPE */
    ((PSHORT)&Buffer[i])[0] = htons( Type );
    i += 2;

    /* QCLASS */
    ((PSHORT)&Buffer[i])[0] = htons( CLASS_IN );

    /* Ship the request off to the DNS server. */
    bOk = SendRequest( Buffer,
                       BufferLength,
                       RecBuffer,
                       &RecBufferLength );
    if( !bOk ) goto cleanup;

    /* Start parsing the received packet. */
    NumQuestions = ntohs( ((PSHORT)&RecBuffer[4])[0] );

    k = 12;

    /* We don't care about the questions section, blow through it. */
    if( NumQuestions )
    {
        for( i = 0; i < NumQuestions; i += 1 )
        {
            /* Quick way to skip the domain name section. */
            k += ExtractName( RecBuffer, pResult, k, 0 );
            k += 4;
        }
    }

    /* Skip the answer name. */
    k += ExtractName( RecBuffer, pResult, k, 0 );

    Type = ntohs( ((PUSHORT)&RecBuffer[k])[0] );
    k += 8;

    d = ntohs( ((PUSHORT)&RecBuffer[k])[0] );
    k += 2;

    if( TYPE_PTR == Type )
    {
        k += ExtractName( RecBuffer, pResult, k, d );
    }
    else if( TYPE_A == Type )
    {
        k += ExtractIP( RecBuffer, pResult, k );
    }

cleanup:
    /* Free memory. */
    if( Buffer ) HeapFree( ProcessHeap, 0, Buffer );
    if( RecBuffer ) HeapFree( ProcessHeap, 0, RecBuffer );

    RequestID += 1;

    return bOk;
}

void PerformLookup( PCHAR pAddr )
{
    /* Needed to issue DNS packets and parse them. */
    PCHAR Buffer = NULL, RecBuffer = NULL;
    CHAR pResolve[256];
    CHAR pResult[256];
    ULONG BufferLength = 0, RecBufferLength = 512;
    int i = 0, j = 0, k = 0, d = 0;
    BOOL bOk = FALSE;

    /* Makes things easier when parsing the response packet. */
    UCHAR Header2;
    USHORT NumQuestions;
    USHORT NumAnswers;
    USHORT NumAuthority;
    USHORT Type;

    if( (strlen( pAddr ) + 1) > 255 ) return;

    _tprintf( _T("Server:  %s\n"), State.DefaultServer );
    _tprintf( _T("Address:  %s\n\n"), State.DefaultServerAddress );

    if( !strcmp( TypeA, State.type )
        || !strcmp( TypeAAAA, State.type )
        || !strcmp( TypeBoth, State.type ) )
    {
        Type = TYPE_A;
        if( IsValidIP( pAddr ) ) Type = TYPE_PTR;
    }
    else
        Type = TypeNametoTypeID( State.type );

    /* If it's a PTR lookup then append the ARPA sig to the end. */
    if( (Type == TYPE_PTR) && IsValidIP( pAddr ) )
    {
        ReverseIP( pAddr, pResolve );
        strcat( pResolve, ARPA_SIG );
    }
    else
    {
        strcpy( pResolve, pAddr );
    }

    /* Base header length + length of QNAME + length of QTYPE and QCLASS */
    BufferLength = 12 + (strlen( pResolve ) + 2) + 4;

    /* Allocate memory for the buffer. */
    Buffer = HeapAlloc( ProcessHeap, 0, BufferLength );
    if( !Buffer )
    {
        _tprintf( _T("ERROR: Out of memory\n") );
        goto cleanup;
    }

    /* Allocate memory for the return buffer. */
    RecBuffer = HeapAlloc( ProcessHeap, 0, RecBufferLength );
    if( !RecBuffer )
    {
        _tprintf( _T("ERROR: Out of memory\n") );
        goto cleanup;
    }

    /* Insert the ID field. */
    ((PSHORT)&Buffer[i])[0] = htons( RequestID );
    i += 2;

    /* Bits 0-7 of the second 16 are all 0, except for when recursion is
    desired. */
    Buffer[i] = 0x00;
    if( State.recurse) Buffer[i] |= 0x01;
    i += 1;

    /* Bits 8-15 of the second 16 are 0 for a query. */
    Buffer[i] = 0x00;
    i += 1;

    /* Only 1 question. */
    ((PSHORT)&Buffer[i])[0] = htons( 1 );
    i += 2;

    /* We aren't sending a response, so 0 out the rest of the header. */
    Buffer[i] = 0x00;
    Buffer[i + 1] = 0x00;
    Buffer[i + 2] = 0x00;
    Buffer[i + 3] = 0x00;
    Buffer[i + 4] = 0x00;
    Buffer[i + 5] = 0x00;
    i += 6;

    /* Walk through the query address. Split each section delimited by '.'.
       Format of the QNAME section is length|data, etc. Last one is null */
    j = i;
    i += 1;

    for( k = 0; k < strlen( pResolve ); k += 1 )
    {
        if( pResolve[k] != '.' )
        {
            Buffer[i] = pResolve[k];
            i += 1;
        }
        else
        {
            Buffer[j] = (i - j) - 1;
            j = i;
            i += 1;
        }
    }

    Buffer[j] = (i - j) - 1;
    Buffer[i] = 0x00;
    i += 1;

    /* QTYPE */
    ((PSHORT)&Buffer[i])[0] = htons( Type );
    i += 2;

    /* QCLASS */
    ((PSHORT)&Buffer[i])[0] = htons( ClassNametoClassID( State.Class ) );

    /* Ship off the request to the DNS server. */
    bOk = SendRequest( Buffer,
                       BufferLength,
                       RecBuffer,
                       &RecBufferLength );
    if( !bOk ) goto cleanup;

    /* Start parsing the received packet. */
    Header2 = RecBuffer[3];
    NumQuestions = ntohs( ((PSHORT)&RecBuffer[4])[0] );
    NumAnswers = ntohs( ((PSHORT)&RecBuffer[6])[0] );
    NumAuthority = ntohs( ((PUSHORT)&RecBuffer[8])[0] );
    Type = 0;

    /* Check the RCODE for failure. */
    d = Header2 & 0x0F;
    if( d != RCODE_NOERROR )
    {
        switch( d )
        {
        case RCODE_NXDOMAIN:
            _tprintf( _T("*** %s can't find %s: Non-existant domain\n"), State.DefaultServer, pAddr );
            break;
            
        case RCODE_REFUSED:
            _tprintf( _T("*** %s can't find %s: Query refused\n"), State.DefaultServer, pAddr );
            break;

        default:
            _tprintf( _T("*** %s can't find %s: Unknown RCODE\n"), State.DefaultServer, pAddr );
        }
            
        goto cleanup;
    }

    k = 12;

    if( NumQuestions )
    {
        /* Blow through the questions section since we don't care about it. */
        for( i = 0; i < NumQuestions; i += 1 )
        {
            k += ExtractName( RecBuffer, pResult, k, 0 );
            k += 4;
        }
    }

    if( NumAnswers )
    {
        /* Skip the name. */
        k += ExtractName( RecBuffer, pResult, k, 0 );

        Type = ntohs( ((PUSHORT)&RecBuffer[k])[0] );
        k += 8;

        d = ntohs( ((PUSHORT)&RecBuffer[k])[0] );
        k += 2;

        if( TYPE_PTR == Type )
        {
            k += ExtractName( RecBuffer, pResult, k, d );
        }
        else if( TYPE_A == Type )
        {
            k += ExtractIP( RecBuffer, pResult, k );
        }
    }

    /* FIXME: This'll need to support more than PTR and A at some point. */
    if( !strcmp( State.type, TypePTR ) )
    {
        if( TYPE_PTR == Type )
        {
            _tprintf( _T("%s     name = %s\n"), pResolve, pResult );
        }
        else
        {
        }
    }
    else if( !strcmp( State.type, TypeA )
          || !strcmp( State.type, TypeAAAA )
          || !strcmp( State.type, TypeBoth ) )
    {
        if( (TYPE_A == Type) /*|| (TYPE_AAAA == Type)*/ )
        {
            if( 0 == NumAuthority )
                _tprintf( _T("Non-authoritative answer:\n") );

            _tprintf( _T("Name:    %s\n"), pAddr );
            _tprintf( _T("Address:  %s\n\n"), pResult );
        }
        else
        {
            _tprintf( _T("Name:    %s\n"), pResult );
            _tprintf( _T("Address:  %s\n\n"), pAddr );
        }
    }

cleanup:
    /* Free memory. */
    if( Buffer ) HeapFree( ProcessHeap, 0, Buffer );
    if( RecBuffer ) HeapFree( ProcessHeap, 0, RecBuffer );

    RequestID += 1;
}

BOOL ParseCommandLine( int argc, char* argv[] )
{
    int i;
    BOOL NoMoreOptions = FALSE;
    BOOL Interactive = FALSE;
    CHAR AddrToResolve[256];
    CHAR Server[256];

    RtlZeroMemory( AddrToResolve, 256 );
    RtlZeroMemory( Server, 256 );

    if( 2 == argc )
    {
        /* In the Windows nslookup, usage is only displayed if /? is the only
           option specified on the command line. */
        if( !strncmp( "/?", argv[1], 2 ) )
        {
            PrintUsage();
            return 0;
        }
    }

    if( argc > 1 )
    {
        for( i = 1; i < argc; i += 1 )
        {
            if( NoMoreOptions )
            {
                strncpy( Server, argv[i], 255 );

                /* Determine which one to resolve. This is based on whether the
                   DNS server provided was an IP or an FQDN. */
                if( IsValidIP( Server ) )
                {
                    strncpy( State.DefaultServerAddress, Server, 16 );

                    PerformInternalLookup( State.DefaultServerAddress,
                                           State.DefaultServer );
                }
                else
                {
                    strncpy( State.DefaultServer, Server, 255 );

                    PerformInternalLookup( State.DefaultServer,
                                           State.DefaultServerAddress );
                }

                if( Interactive ) return 1;

                PerformLookup( AddrToResolve );

                return 0;
            }
            else
            {
                if( !strncmp( "-all", argv[i], 4 ) )
                {
                    PrintState();
                }
                else if( !strncmp( "-type=", argv[i], 6 ) )
                {
                    if( !strncmp( TypeA, &argv[i][6], strlen( TypeA ) ) )
                    {
                        State.type = TypeA;
                    }
                    else if( !strncmp( TypeAAAA, &argv[i][6], strlen( TypeAAAA ) ) )
                    {
                        State.type = TypeAAAA;
                    }
                    else if( !strncmp( TypeBoth, &argv[i][6], strlen( TypeBoth ) ) )
                    {
                        State.type = TypeBoth;
                    }
                    else if( !strncmp( TypeAny, &argv[i][6], strlen( TypeAny ) ) )
                    {
                        State.type = TypeAny;
                    }
                    else if( !strncmp( TypeCNAME, &argv[i][6], strlen( TypeCNAME ) ) )
                    {
                        State.type = TypeCNAME;
                    }
                    else if( !strncmp( TypeMX, &argv[i][6], strlen( TypeMX ) ) )
                    {
                        State.type = TypeMX;
                    }
                    else if( !strncmp( TypeNS, &argv[i][6], strlen( TypeNS ) ) )
                    {
                        State.type = TypeNS;
                    }
                    else if( !strncmp( TypePTR, &argv[i][6], strlen( TypePTR ) ) )
                    {
                        State.type = TypePTR;
                    }
                    else if( !strncmp( TypeSOA, &argv[i][6], strlen( TypeSOA ) ) )
                    {
                        State.type = TypeSOA;
                    }
                    else if( !strncmp( TypeSRV, &argv[i][6], strlen( TypeSRV ) ) )
                    {
                        State.type = TypeSRV;
                    }
                    else
                    {
                        _tprintf( _T("unknown query type: %s"), &argv[i][6] );
                    }
                }
                else if( !strncmp( "-domain=", argv[i], 8 ) )
                {
                    strcpy( State.domain, &argv[i][8] );
                }
                else if( !strncmp( "-srchlist=", argv[i], 10 ) )
                {
                }
                else if( !strncmp( "-root=", argv[i], 6 ) )
                {
                    strcpy( State.root, &argv[i][6] );
                }
                else if( !strncmp( "-retry=", argv[i], 7 ) )
                {
                }
                else if( !strncmp( "-timeout=", argv[i], 9 ) )
                {
                }
                else if( !strncmp( "-querytype=", argv[i], 11 ) )
                {
                    if( !strncmp( TypeA, &argv[i][11], strlen( TypeA ) ) )
                    {
                        State.type = TypeA;
                    }
                    else if( !strncmp( TypeAAAA, &argv[i][11], strlen( TypeAAAA ) ) )
                    {
                        State.type = TypeAAAA;
                    }
                    else if( !strncmp( TypeBoth, &argv[i][11], strlen( TypeBoth ) ) )
                    {
                        State.type = TypeBoth;
                    }
                    else if( !strncmp( TypeAny, &argv[i][11], strlen( TypeAny ) ) )
                    {
                        State.type = TypeAny;
                    }
                    else if( !strncmp( TypeCNAME, &argv[i][11], strlen( TypeCNAME ) ) )
                    {
                        State.type = TypeCNAME;
                    }
                    else if( !strncmp( TypeMX, &argv[i][11], strlen( TypeMX ) ) )
                    {
                        State.type = TypeMX;
                    }
                    else if( !strncmp( TypeNS, &argv[i][11], strlen( TypeNS ) ) )
                    {
                        State.type = TypeNS;
                    }
                    else if( !strncmp( TypePTR, &argv[i][11], strlen( TypePTR ) ) )
                    {
                        State.type = TypePTR;
                    }
                    else if( !strncmp( TypeSOA, &argv[i][11], strlen( TypeSOA ) ) )
                    {
                        State.type = TypeSOA;
                    }
                    else if( !strncmp( TypeSRV, &argv[i][11], strlen( TypeSRV ) ) )
                    {
                        State.type = TypeSRV;
                    }
                    else
                    {
                        _tprintf( _T("unknown query type: %s"), &argv[i][6] );
                    }
                }
                else if( !strncmp( "-class=", argv[i], 7 ) )
                {
                    if( !strncmp( ClassIN, &argv[i][7], strlen( ClassIN ) ) )
                    {
                        State.Class = ClassIN;
                    }
                    else if( !strncmp( ClassAny, &argv[i][7], strlen( ClassAny ) ) )
                    {
                        State.Class = ClassAny;
                    }
                    else
                    {
                        _tprintf( _T("unknown query class: %s"), &argv[i][7] );
                    }
                }
                else if( !strncmp( "-ixfrver=", argv[i], 9 ) )
                {
                }
                else if( !strncmp( "-debug", argv[i], 6 ) )
                {
                    State.debug = TRUE;
                }
                else if( !strncmp( "-nodebug", argv[i], 8 ) )
                {
                    State.debug = FALSE;
                    State.d2 = FALSE;
                }
                else if( !strncmp( "-d2", argv[i], 3 ) )
                {
                    State.d2 = TRUE;
                    State.debug = TRUE;
                }
                else if( !strncmp( "-nod2", argv[i], 5 ) )
                {
                    if( State.debug ) _tprintf( _T("d2 mode disabled; still in debug mode\n") );

                    State.d2 = FALSE;
                }
                else if( !strncmp( "-defname", argv[i], 8 ) )
                {
                    State.defname = TRUE;
                }
                else if( !strncmp( "-noddefname", argv[i], 10 ) )
                {
                    State.defname = FALSE;
                }
                else if( !strncmp( "-recurse", argv[i], 8 ) )
                {
                    State.recurse = TRUE;
                }
                else if( !strncmp( "-norecurse", argv[i], 10 ) )
                {
                    State.recurse = FALSE;
                }
                else if( !strncmp( "-search", argv[i], 7 ) )
                {
                    State.search = TRUE;
                }
                else if( !strncmp( "-nosearch", argv[i], 9 ) )
                {
                    State.search = FALSE;
                }
                else if( !strncmp( "-vc", argv[i], 3 ) )
                {
                    State.vc = TRUE;
                }
                else if( !strncmp( "-novc", argv[i], 5 ) )
                {
                    State.vc = FALSE;
                }
                else if( !strncmp( "-msxfr", argv[i], 6 ) )
                {
                    State.MSxfr = TRUE;
                }
                else if( !strncmp( "-nomsxfr", argv[i], 8 ) )
                {
                    State.MSxfr = FALSE;
                }
                else if( !strncmp( "-", argv[i], 1 ) && (strlen( argv[i] ) == 1) )
                {
                    /* Since we received just the plain - switch, we are going
                       to be entering interactive mode. We also will not be
                       parsing any more options. */
                    NoMoreOptions = TRUE;
                    Interactive = TRUE;
                }
                else
                {
                    /* Grab the address to resolve. No more options accepted
                       past this point. */
                    strncpy( AddrToResolve, argv[i], 255 );
                    NoMoreOptions = TRUE;
                }
            }
        }

        if( NoMoreOptions && !Interactive )
        {
            /* Get the FQDN of the DNS server. */
            PerformInternalLookup( State.DefaultServerAddress,
                                   State.DefaultServer );

            PerformLookup( AddrToResolve );

            return 0;
        }
    }

    /* Get the FQDN of the DNS server. */
    PerformInternalLookup( State.DefaultServerAddress,
                           State.DefaultServer );

    return 1;
}

void InteractiveMode()
{
    _tprintf( _T("Default Server:  %s\n"), State.DefaultServer );
    _tprintf( _T("Address:  %s\n\n"), State.DefaultServerAddress );

    /* TODO: Implement interactive mode. */

    _tprintf( _T("ERROR: Feature not implemented.\n") );
}

int main( int argc, char* argv[] )
{
    int i;
    ULONG Status;
    PFIXED_INFO pNetInfo = NULL;
    ULONG NetBufLen = 0;
    WSADATA wsaData;

    ProcessHeap = GetProcessHeap();
    RequestID = 1;

    /* Set up the initial state. */
    State.debug = FALSE;
    State.defname = TRUE;
    State.search = TRUE;
    State.recurse = TRUE;
    State.d2 = FALSE;
    State.vc = FALSE;
    State.ignoretc = FALSE;
    State.port = 53;
    State.type = TypeBoth;
    State.Class = ClassIN;
    State.timeout = 2;
    State.retry = 1;
    State.MSxfr = TRUE;
    State.ixfrver = 1;

    RtlZeroMemory( State.root, 256 );
    RtlZeroMemory( State.domain, 256 );
    for( i = 0; i < 6; i += 1 ) RtlZeroMemory( State.srchlist[i], 256 );
    RtlZeroMemory( State.DefaultServer, 256 );
    RtlZeroMemory( State.DefaultServerAddress, 16 );

    memcpy( State.root, DEFAULT_ROOT, sizeof(DEFAULT_ROOT) );

    /* We don't know how long of a buffer it will want to return. So we'll
       pass an empty one now and let it fail only once, instead of guessing. */
    Status = GetNetworkParams( pNetInfo, &NetBufLen );
    if( Status == ERROR_BUFFER_OVERFLOW )
    {
        pNetInfo = (PFIXED_INFO)HeapAlloc( ProcessHeap, 0, NetBufLen );
        if( pNetInfo == NULL )
        {
            _tprintf( _T("ERROR: Out of memory\n") );

            return -1;
        }

        /* For real this time. */
        Status = GetNetworkParams( pNetInfo, &NetBufLen );
        if( Status != NO_ERROR )
        {
            _tprintf( _T("Error in GetNetworkParams call\n") );

            HeapFree( ProcessHeap, 0, pNetInfo );

            return -2;
        }
    }

    strncpy( State.domain, pNetInfo->DomainName, 255 );
    strncpy( State.srchlist[0], pNetInfo->DomainName, 255 );
    strncpy( State.DefaultServerAddress,
             pNetInfo->DnsServerList.IpAddress.String,
             15 );

    HeapFree( ProcessHeap, 0, pNetInfo );

    WSAStartup( MAKEWORD(2,2), &wsaData );

    switch( ParseCommandLine( argc, argv ) )
    {
    case 0:
        /* This means that it was a /? parameter. */
        break;

    default:
        /* Anything else means we enter interactive mode. The only exception
           to this is when the host to resolve was provided on the command
           line. */
        InteractiveMode();
    }

    WSACleanup();
    return 0;
}
