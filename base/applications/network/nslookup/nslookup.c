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
    _tprintf( _T("Default Server:  %s\n"), State.DefaultServer );
    _tprintf( _T("Address:  %s\n\n"), State.DefaultServerAddress );

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
                 "   nslookup [-opt ...]             # interactive mode using default server\n"
                 "   nslookup [-opt ...] - server    # interactive mode using 'server'\n"
                 "   nslookup [-opt ...] host        # just look up 'host' using default server\n"
                 "   nslookup [-opt ...] host server # just look up 'host' using 'server'\n") );
}

void PrintHelp(void)
{
    _tprintf(_T("Commands:   (identifiers are shown in uppercase, [] means optional)\n"));
    _tprintf(_T("NAME1           - print info about host/domain NAME using default server\n"));
    _tprintf(_T("NAME1 NAME2     - as above, but use NAME2 as server\n"));
    _tprintf(_T("help or ?       - print info on common commands\n"));
    _tprintf(_T("set OPTION      - set an option\n"));
    _tprintf(_T("    all                 - print options, current server and host\n"));
    _tprintf(_T("    [no]debug           - print debugging information\n"));
    _tprintf(_T("    [no]d2              - print exhaustive debugging information\n"));
    _tprintf(_T("    [no]defname         - apend domain name to each query\n"));
    _tprintf(_T("    [no]recurse         - ask for recursive answer to query\n"));
    _tprintf(_T("    [no]search          - use domain list query\n"));
    _tprintf(_T("    [no]vc              - always use virtual circuit\n"));
    _tprintf(_T("    domain=NAME         - set default domain name to NAME\n"));
    _tprintf(_T("    srchlist=N1[/N2/.../N6] - set domain to N1 and search list to N1,N2 etc.\n"));
    _tprintf(_T("    root=NAME           - set root server to NAME\n"));
    _tprintf(_T("    retry=X             - set number of retries to X\n"));
    _tprintf(_T("    timeout=X           - set initial time-out interval to X seconds\n"));
    _tprintf(_T("    type=X              - set query type (ex. A,ANY,CNAME,MX,NS,PTR,SOA,SRV)\n"));
    _tprintf(_T("    querytype=X         - same as type\n"));
    _tprintf(_T("    class=X             - set query type (ex. IN (Internet), ANY)\n"));
    _tprintf(_T("    [no]msxfr           - use MS fast zone transfer\n"));
    _tprintf(_T("    ixfrver=X           - current version to use in IXFR transfer request\n"));
    _tprintf(_T("server NAME     - set default server to NAME, using current default server\n"));
    _tprintf(_T("lserver NAME    - set default server to NAME, using initial server\n"));
    _tprintf(_T("finger [USER]   - finger the optional NAME at the current default host\n"));
    _tprintf(_T("root            - set current default server too the root\n"));

    _tprintf(_T("exit            - exit the program\n"));
    _tprintf(_T("\n"));
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
    DWORD dwValue;
    PSTR pszEnd, pszValue;

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
                    pszValue = &argv[i][7];
                    dwValue = strtoul(pszValue, &pszEnd, 10);
                    if (pszEnd != pszValue)
                        State.retry = dwValue;
                }
                else if( !strncmp( "-timeout=", argv[i], 9 ) )
                {
                    pszValue = &argv[i][9];
                    dwValue = strtoul(pszValue, &pszEnd, 10);
                    if (pszEnd != pszValue)
                        State.timeout = dwValue;
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
                        _tprintf( _T("unknown query type: %s"), &argv[i][11] );
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
    CHAR input_line[256];
    PSTR args_vector[64];
    DWORD dwArgCount = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bDone = FALSE;
    BOOL bInQuotes = FALSE;
    PSTR ptr;
    DWORD dwValue;
    PSTR pszEnd, pszValue;

    _tprintf( _T("Default Server:  %s\n"), State.DefaultServer );
    _tprintf( _T("Address:  %s\n\n"), State.DefaultServerAddress );

    for (;;)
    {
        dwArgCount = 0;
        memset(args_vector, 0, sizeof(args_vector));

        _tprintf(_T("> "));

        /* Get input from the user. */
        fgets(input_line, 256, stdin);

        ptr = input_line;
        while (*ptr != 0)
        {
            if (*ptr == _T('\"'))
                bInQuotes = (bInQuotes) ? FALSE : TRUE;

            if ((_istspace(*ptr) && (bInQuotes == FALSE)) || *ptr == _T('\n'))
            {
                *ptr = _T('\0');
                bWhiteSpace = TRUE;
            }
            else
            {
                if ((bWhiteSpace != FALSE) && (dwArgCount < 64))
                {
                    args_vector[dwArgCount] = ptr;
                    dwArgCount++;
                }
                bWhiteSpace = FALSE;
            }
            ptr++;
        }

        if (dwArgCount > 0)
        {
            if (_stricmp(args_vector[0], "exit") == 0)
            {
                bDone = TRUE;
            }
            else if ((!_stricmp(args_vector[0], "help")) ||
                     (!_stricmp(args_vector[0], "?")))
            {
                PrintHelp();
            }
            else if (!_stricmp(args_vector[0], "set"))
            {
                if (dwArgCount > 1)
                {
                    if (!_stricmp(args_vector[1], "all"))
                    {
                        PrintState();
                    }
                    else if (!_stricmp(args_vector[1], "debug"))
                    {
                        State.debug = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "nodebug"))
                    {
                        State.debug = FALSE;
                        State.d2 = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "defname"))
                    {
                        State.defname = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "nodefname"))
                    {
                        State.defname = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "search"))
                    {
                        State.search = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "nosearch"))
                    {
                        State.search = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "recurse"))
                    {
                        State.recurse = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "norecurse"))
                    {
                        State.recurse = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "d2"))
                    {
                        State.debug = TRUE;
                        State.d2 = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "nod2"))
                    {
                        State.d2 = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "vc"))
                    {
                        State.vc = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "novc"))
                    {
                        State.vc = FALSE;
                    }
                    else if (!_stricmp(args_vector[1], "msxfr"))
                    {
                        State.MSxfr = TRUE;
                    }
                    else if (!_stricmp(args_vector[1], "nomsxfr"))
                    {
                        State.MSxfr = FALSE;
                    }
                    else if (!_strnicmp(args_vector[1], "retry=", strlen("retry=")))
                    {
                        pszValue = &args_vector[1][strlen("retry=")];
                        dwValue = strtoul(pszValue, &pszEnd, 10);
                        if (pszEnd != pszValue)
                            State.retry = dwValue;
                    }
                    else if (!_strnicmp(args_vector[1], "timeout=", strlen("timeout=")))
                    {
                        pszValue = &args_vector[1][strlen("timeout=")];
                        dwValue = strtoul(pszValue, &pszEnd, 10);
                        if (pszEnd != pszValue)
                            State.timeout = dwValue;
                    }
                    else if (!_strnicmp(args_vector[1], "domain=", strlen("domain=")))
                    {
                        strcpy(State.domain, &args_vector[1][strlen("domain=")]);
                    }
                    else if (!_strnicmp(args_vector[1], "srchlist=", strlen("srchlist=")))
                    {
                        _tprintf(_T("Option not implemented: srchlist\n"));
                    }
                    else if (!_strnicmp(args_vector[1], "root=", strlen("root=")))
                    {
                        _tprintf(_T("Option not implemented: root\n"));
                    }
                    else if (!_strnicmp(args_vector[1], "type=", strlen("type=")))
                    {
                        if (!strncmp(TypeA, &args_vector[1][6], strlen(TypeA)))
                        {
                            State.type = TypeA;
                        }
                        else if (!strncmp(TypeAAAA, &args_vector[1][6], strlen(TypeAAAA)))
                        {
                            State.type = TypeAAAA;
                        }
                        else if (!strncmp(TypeBoth, &args_vector[1][6], strlen(TypeBoth)))
                        {
                            State.type = TypeBoth;
                        }
                        else if (!strncmp(TypeAny, &args_vector[1][6], strlen(TypeAny)))
                        {
                            State.type = TypeAny;
                        }
                        else if (!strncmp(TypeCNAME, &args_vector[1][6], strlen(TypeCNAME)))
                        {
                            State.type = TypeCNAME;
                        }
                        else if (!strncmp(TypeMX, &args_vector[1][6], strlen(TypeMX)))
                        {
                            State.type = TypeMX;
                        }
                        else if (!strncmp(TypeNS, &args_vector[1][6], strlen(TypeNS)))
                        {
                            State.type = TypeNS;
                        }
                        else if (!strncmp(TypePTR, &args_vector[1][6], strlen(TypePTR)))
                        {
                            State.type = TypePTR;
                        }
                        else if (!strncmp(TypeSOA, &args_vector[1][6], strlen(TypeSOA)))
                        {
                            State.type = TypeSOA;
                        }
                        else if (!strncmp(TypeSRV, &args_vector[1][6], strlen(TypeSRV)))
                        {
                            State.type = TypeSRV;
                        }
                        else
                        {
                            _tprintf(_T("unknown query type: %s"), &args_vector[1][6]);
                        }
                    }
                    else if (!_strnicmp(args_vector[1], "querytype=", strlen("querytype=")))
                    {
                        if (!strncmp(TypeA, &args_vector[1][11], strlen(TypeA)))
                        {
                            State.type = TypeA;
                        }
                        else if (!strncmp(TypeAAAA, &args_vector[1][11], strlen(TypeAAAA)))
                        {
                            State.type = TypeAAAA;
                        }
                        else if (!strncmp(TypeBoth, &args_vector[1][11], strlen(TypeBoth)))
                        {
                            State.type = TypeBoth;
                        }
                        else if (!strncmp(TypeAny, &args_vector[1][11], strlen(TypeAny)))
                        {
                            State.type = TypeAny;
                        }
                        else if (!strncmp(TypeCNAME, &args_vector[1][11], strlen(TypeCNAME)))
                        {
                            State.type = TypeCNAME;
                        }
                        else if (!strncmp(TypeMX, &args_vector[1][11], strlen(TypeMX)))
                        {
                            State.type = TypeMX;
                        }
                        else if (!strncmp(TypeNS, &args_vector[1][11], strlen(TypeNS)))
                        {
                            State.type = TypeNS;
                        }
                        else if (!strncmp(TypePTR, &args_vector[1][11], strlen(TypePTR)))
                        {
                            State.type = TypePTR;
                        }
                        else if (!strncmp(TypeSOA, &args_vector[1][11], strlen(TypeSOA)))
                        {
                            State.type = TypeSOA;
                        }
                        else if (!strncmp(TypeSRV, &args_vector[1][11], strlen(TypeSRV)))
                        {
                            State.type = TypeSRV;
                        }
                        else
                        {
                            _tprintf(_T("unknown query type: %s"), &args_vector[1][11]);
                        }
                    }
                    else if (!_strnicmp(args_vector[1], "class=", strlen("class=")))
                    {
                        if (!strncmp(ClassIN, &args_vector[1][7], strlen(ClassIN)))
                        {
                            State.Class = ClassIN;
                        }
                        else if (!strncmp(ClassAny, &args_vector[1][7], strlen(ClassAny)))
                        {
                            State.Class = ClassAny;
                        }
                        else
                        {
                            _tprintf(_T("unknown query class: %s"), &args_vector[1][7]);
                        }
                    }
                    else if (!_strnicmp(args_vector[1], "ixfrver=", strlen("ixfrver=")))
                    {
                        _tprintf(_T("Option not implemented: ixfrver\n"));
                    }
                    else
                    {
                        _tprintf(_T("*** Invalid option: %s.\n"), args_vector[1]);
                    }
                }
            }
            else if (!_stricmp(args_vector[0], "server"))
            {
                _tprintf(_T("Command not implemented: server\n"));
            }
            else if (!_stricmp(args_vector[0], "lserver"))
            {
                _tprintf(_T("Command not implemented: lserver\n"));
            }
            else if (!_stricmp(args_vector[0], "finger"))
            {
                _tprintf(_T("Command not implemented: finger\n"));
            }
            else if (!_stricmp(args_vector[0], "root"))
            {
                _tprintf(_T("Command not implemented: root\n"));
            }
            else if (!_stricmp(args_vector[0], "ls"))
            {
                _tprintf(_T("Command not implemented: ls\n"));
            }
            else if (!_stricmp(args_vector[0], "view"))
            {
                _tprintf(_T("Command not implemented: view\n"));
            }
            else
            {
                if (dwArgCount == 1)
                {
                    PerformLookup(args_vector[0]);
                }
                else if (dwArgCount == 2)
                {
                    CHAR BackupServerAddress[16];
                    CHAR BackupServer[256];

                    CopyMemory(BackupServerAddress, State.DefaultServerAddress, sizeof(BackupServerAddress));
                    CopyMemory(BackupServer, State.DefaultServer, sizeof(BackupServer));

                    if (IsValidIP(args_vector[1]))
                    {
                        strncpy(State.DefaultServerAddress, args_vector[1], min(16, strlen(args_vector[1])));
                        State.DefaultServerAddress[min(16, strlen(args_vector[1]))] = '\0';

                        PerformInternalLookup(State.DefaultServerAddress,
                                              State.DefaultServer);
                    }
                    else
                    {
                        strncpy(State.DefaultServer, args_vector[1], min(255, strlen(args_vector[1])));
                        State.DefaultServer[min(255, strlen(args_vector[1]))] = '\0';

                        PerformInternalLookup(State.DefaultServer,
                                              State.DefaultServerAddress);
                    }

                    PerformLookup(args_vector[0]);

                    CopyMemory(State.DefaultServerAddress, BackupServerAddress, sizeof(BackupServerAddress));
                    CopyMemory(State.DefaultServer, BackupServer, sizeof(BackupServer));
                }
                else
                {
                    _tprintf(_T("Unrecognized command:"));
                    for (DWORD i = 0; i < dwArgCount; i++)
                        _tprintf(_T(" %s"), args_vector[i]);
                    _tprintf(_T("\n"));
                }
            }
        }

        if (bDone)
            break;
    }

}

int main( int argc, char* argv[] )
{
    int i;
    ULONG Status;
    PFIXED_INFO pNetInfo = NULL;
    ULONG NetBufLen = 0;
    WSADATA wsaData;
    int ret;

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

    if( Status != ERROR_BUFFER_OVERFLOW )
    {
        _tprintf( _T("Error in GetNetworkParams call\n") );

        return -2;
    }

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

    strncpy( State.domain, pNetInfo->DomainName, 255 );
    strncpy( State.srchlist[0], pNetInfo->DomainName, 255 );
    strncpy( State.DefaultServerAddress,
             pNetInfo->DnsServerList.IpAddress.String,
             15 );

    HeapFree( ProcessHeap, 0, pNetInfo );

    ret = WSAStartup( MAKEWORD(2, 2), &wsaData );
    if (ret != 0)
    {
        _tprintf( _T("Winsock initialization failed: %d\n"), ret );
        return ret;
    }

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
