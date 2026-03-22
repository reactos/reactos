/*
 * PROJECT:     ReactOS nslookup utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/nslookup/utility.c
 * PURPOSE:     Support functions for nslookup.c
 * COPYRIGHT:   Copyright 2009 Lucas Suggs <lucas.suggs@gmail.com>
 */

#include "nslookup.h"

/*
 * SendRequest
 *
 * Thin wrapper around DnsResolv_SendRequest.  Populates a
 * DNS_RESOLVER_CONFIG from the global State, calls into the
 * dnsresolv library, and (when the relevant debug flags are set)
 * calls PrintD2 / PrintDebug around the network operation.
 */
BOOL SendRequest( PCHAR pInBuffer,
                  ULONG InBufferLength,
                  PCHAR pOutBuffer,
                  PULONG pOutBufferLength )
{
    DNS_RESOLVER_CONFIG config;
    BOOL bResult;

    DnsResolv_InitConfig(&config);
    strncpy(config.ServerAddress, State.DefaultServerAddress,
            sizeof(config.ServerAddress) - 1);
    config.ServerAddress[sizeof(config.ServerAddress) - 1] = '\0';
    config.Port    = State.port;
    config.Timeout = State.timeout;
    config.Retry   = State.retry;
    config.Recurse = State.recurse;

    if (State.d2)
        PrintD2(pInBuffer, InBufferLength);

    bResult = DnsResolv_SendRequest(pInBuffer, InBufferLength,
                                    pOutBuffer, pOutBufferLength,
                                    &config);

    if (bResult && State.debug)
        PrintDebug(pOutBuffer, *pOutBufferLength);

    return bResult;
}

void PrintD2( PCHAR pBuffer, DWORD BufferLength )
{
    USHORT RequestID;
    UCHAR Header1, Header2;
    USHORT NumQuestions, NumAnswers, NumAuthority, NumAdditional;
    USHORT Type, Class;
    CHAR pName[255];
    int i = 0, k = 0;

    RequestID = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    Header1 = pBuffer[i];
    i += 1;

    Header2 = pBuffer[i];
    i += 1;

    NumQuestions = ntohs( ((PSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAnswers = ntohs( ((PSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAuthority = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAdditional = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    _tprintf( _T("------------\n") );
    _tprintf( _T("SendRequest(), len %d\n"), (int)BufferLength );
    _tprintf( _T("    HEADER:\n") );
    _tprintf( _T("        opcode = %s, id = %d, rcode = %s\n"),
                 DnsResolv_OpcodeIDtoOpcodeName( (Header1 & 0x78) >> 3 ),
                 (int)RequestID,
                 DnsResolv_RCodeIDtoRCodeName( Header2 & 0x0F ) );

    _tprintf( _T("        header flags:  query") );
    if( Header1 & 0x01 ) _tprintf( _T(", want recursion") );
    _tprintf( _T("\n") );

    _tprintf( _T("        questions = %d,  answers = %d,"
                 "  authority records = %d,  additional = %d\n\n"),
                 (int)NumQuestions,
                 (int)NumAnswers,
                 (int)NumAuthority,
                 (int)NumAdditional );

    if( NumQuestions )
    {
        _tprintf( _T("    QUESTIONS:\n") );

        for( k = 0; k < NumQuestions; k += 1 )
        {
            i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("        %s"), pName );

            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T(", type = %s, class = %s\n"),
                      DnsResolv_TypeIDtoTypeName( Type ),
                      DnsResolv_ClassIDtoClassName( Class ) );
        }
    }

    _tprintf( _T("\n------------\n") );
}

void PrintDebug( PCHAR pBuffer, DWORD BufferLength )
{
    USHORT ResponseID;
    UCHAR Header1, Header2;
    USHORT NumQuestions, NumAnswers, NumAuthority, NumAdditional;
    USHORT Type, Class;
    ULONG TTL;
    CHAR pName[255];
    int d = 0, i = 0, k = 0;

    ResponseID = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    Header1 = pBuffer[i];
    i += 1;

    Header2 = pBuffer[i];
    i += 1;

    NumQuestions = ntohs( ((PSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAnswers = ntohs( ((PSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAuthority = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    NumAdditional = ntohs( ((PUSHORT)&pBuffer[i])[0] );
    i += 2;

    _tprintf( _T("------------\n") );
    _tprintf( _T("Got answer (%d bytes):\n"), (int)BufferLength );
    _tprintf( _T("    HEADER:\n") );
    _tprintf( _T("        opcode = %s, id = %d, rcode = %s\n"),
                 DnsResolv_OpcodeIDtoOpcodeName( (Header1 & 0x78) >> 3 ),
                 (int)ResponseID,
                 DnsResolv_RCodeIDtoRCodeName( Header2 & 0x0F ) );

    _tprintf( _T("        header flags:  response") );
    if( Header1 & 0x01 ) _tprintf( _T(", want recursion") );
    if( Header2 & 0x80 ) _tprintf( _T(", recursion avail.") );
    _tprintf( _T("\n") );

    _tprintf( _T("        questions = %d,  answers = %d,  "
                 "authority records = %d,  additional = %d\n\n"),
                 (int)NumQuestions,
                 (int)NumAnswers,
                 (int)NumAuthority,
                 (int)NumAdditional );

    if( NumQuestions )
    {
        _tprintf( _T("    QUESTIONS:\n") );

        for( k = 0; k < NumQuestions; k += 1 )
        {
            i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("        %s"), pName );

            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T(", type = %s, class = %s\n"),
                      DnsResolv_TypeIDtoTypeName( Type ),
                      DnsResolv_ClassIDtoClassName( Class ) );
        }
    }

    if( NumAnswers )
    {
        _tprintf( _T("    ANSWERS:\n") );

        for( k = 0; k < NumAnswers; k += 1 )
        {
            _tprintf( _T("    ->  ") );

            /* Print out the name. */
            i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("%s\n"), pName );

            /* Print out the type, class and data length. */
            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            TTL = ntohl( ((PULONG)&pBuffer[i])[0] );
            i += 4;

            d = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T("        type = %s, class = %s, dlen = %d\n"),
                      DnsResolv_TypeIDtoTypeName( Type ),
                      DnsResolv_ClassIDtoClassName( Class ),
                      d );

            /* Print out the answer. */
            if( TYPE_A == Type )
            {
                i += DnsResolv_ExtractIP( pBuffer, pName, i );

                _tprintf( _T("        internet address = %s\n"), pName );
            }
            else
            {
                i += DnsResolv_ExtractName( pBuffer, pName, i, d );

                _tprintf( _T("        name = %s\n"), pName );
            }

            _tprintf( _T("        ttl = %d ()\n"), (int)TTL );
        }
    }

    if( NumAuthority )
    {
        _tprintf( _T("    AUTHORITY RECORDS:\n") );

        for( k = 0; k < NumAuthority; k += 1 )
        {
            /* Print out the zone name. */
            i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("    ->  %s\n"), pName );

            /* Print out the type, class, data length and TTL. */
            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            TTL = ntohl( ((PULONG)&pBuffer[i])[0] );
            i += 4;

            d = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T("        type = %s, class = %s, dlen = %d\n"),
                      DnsResolv_TypeIDtoTypeName( Type ),
                      DnsResolv_ClassIDtoClassName( Class ),
                      d );

            /* TODO: There might be more types? */
            if( TYPE_NS == Type )
            {
                /* Print out the NS. */
                i += DnsResolv_ExtractName( pBuffer, pName, i, d );

                _tprintf( _T("        nameserver = %s\n"), pName );

                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );
            }
            else if( TYPE_SOA == Type )
            {
                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );

                /* Print out the primary NS. */
                i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

                _tprintf( _T("        primary name server = %s\n"), pName );

                /* Print out the responsible mailbox. */
                i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

                _tprintf( _T("        responsible mail addr = %s\n"), pName );

                /* Print out the serial, refresh, retry, expire and default TTL. */
                _tprintf( _T("        serial = ()\n") );
                _tprintf( _T("        refresh = ()\n") );
                _tprintf( _T("        retry = ()\n") );
                _tprintf( _T("        expire = ()\n") );
                _tprintf( _T("        default TTL = ()\n") );
                i += 20;
            }
        }
    }

    if( NumAdditional )
    {
        _tprintf( _T("    ADDITIONAL:\n") );

        for( k = 0; k < NumAdditional; k += 1 )
        {
            /* Print the name. */
            i += DnsResolv_ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("    ->  %s\n"), pName );

            /* Print out the type, class, data length and TTL. */
            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            TTL = ntohl( ((PULONG)&pBuffer[i])[0] );
            i += 4;

            d = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T("        type = %s, class = %s, dlen = %d\n"),
                      DnsResolv_TypeIDtoTypeName( Type ),
                      DnsResolv_ClassIDtoClassName( Class ),
                      d );

            /* TODO: There might be more types? */
            if( TYPE_A == Type )
            {
                /* Print out the NS. */
                i += DnsResolv_ExtractIP( pBuffer, pName, i );

                _tprintf( _T("        internet address = %s\n"), pName );

                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );
            }
        }
    }

    _tprintf( _T("\n------------\n") );
}
