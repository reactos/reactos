/*
 * PROJECT:     ReactOS nslookup utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/nslookup/utility.c
 * PURPOSE:     Support functions for nslookup.c
 * COPYRIGHT:   Copyright 2009 Lucas Suggs <lucas.suggs@gmail.com>
 */

#include "nslookup.h"

BOOL SendRequest( PCHAR pInBuffer,
                  ULONG InBufferLength,
                  PCHAR pOutBuffer,
                  PULONG pOutBufferLength )
{
    int j;
    USHORT RequestID, ResponseID;
    BOOL bWait;
    SOCKET s;
    SOCKADDR_IN RecAddr, RecAddr2, SendAddr;
    int SendAddrLen = sizeof(SendAddr);

    RtlZeroMemory( &RecAddr, sizeof(SOCKADDR_IN) );
    RtlZeroMemory( &RecAddr2, sizeof(SOCKADDR_IN) );
    RtlZeroMemory( &SendAddr, sizeof(SOCKADDR_IN) );

    /* Pull the request ID from the buffer. */
    RequestID = ntohs( ((PSHORT)&pInBuffer[0])[0] );

    /* If D2 flags is enabled, then display D2 information. */
    if( State.d2 ) PrintD2( pInBuffer, InBufferLength );

    /* Create the sockets for both send and receive. */
    s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if (s == INVALID_SOCKET)
        return FALSE;

    /* Set up the structure to tell it where we are going. */
    RecAddr.sin_family = AF_INET;
    RecAddr.sin_port = htons( State.port );
    RecAddr.sin_addr.s_addr = inet_addr( State.DefaultServerAddress );

    /* Set up the structure to tell it what port to listen on. */
    RecAddr2.sin_family = AF_INET;
    RecAddr2.sin_port = htons( State.port );
    RecAddr2.sin_addr.s_addr = htonl( INADDR_ANY );

    /* Bind the receive socket. */
    bind( s, (SOCKADDR*)&RecAddr2, sizeof(RecAddr2) );

    /* Send the datagram to the DNS server. */
    j = sendto( s,
                pInBuffer,
                InBufferLength,
                0,
                (SOCKADDR*)&RecAddr,
                sizeof(RecAddr) );
    if( j == SOCKET_ERROR )
    {
        switch( WSAGetLastError() )
        {
        case WSANOTINITIALISED:
            _tprintf( _T("sendto() failed with WSANOTINITIALIZED\n") );
            break;
        case WSAENETDOWN:
            _tprintf( _T("sendto() failed with WSAENETDOWN\n") );
            break;
        case WSAEACCES:
            _tprintf( _T("sendto() failed with WSAEACCES\n") );
            break;
        case WSAEINVAL:
            _tprintf( _T("sendto() failed with WSAEINVAL\n") );
            break;
        case WSAEINTR:
            _tprintf( _T("sendto() failed with WSAEINTR\n") );
            break;
        case WSAEINPROGRESS:
            _tprintf( _T("sendto() failed with WSAEINPROGRESS\n") );
            break;
        case WSAEFAULT:
            _tprintf( _T("sendto() failed with WSAEFAULT\n") );
            break;
        case WSAENETRESET:
            _tprintf( _T("sendto() failed with WSAENETRESET\n") );
            break;
        case WSAENOBUFS:
            _tprintf( _T("sendto() failed with WSAENOBUFS\n") );
            break;
        case WSAENOTCONN:
            _tprintf( _T("sendto() failed with WSAENOTCONN\n") );
            break;
        case WSAENOTSOCK:
            _tprintf( _T("sendto() failed with WSAENOTSOCK\n") );
            break;
        case WSAEOPNOTSUPP:
            _tprintf( _T("sendto() failed with WSAEOPNOTSUPP\n") );
            break;
        case WSAESHUTDOWN:
            _tprintf( _T("sendto() failed with WSAESHUTDOWN\n") );
            break;
        case WSAEWOULDBLOCK:
            _tprintf( _T("sendto() failed with WSAEWOULDBLOCK\n") );
            break;
        case WSAEMSGSIZE:
            _tprintf( _T("sendto() failed with WSAEMSGSIZE\n") );
            break;
        case WSAEHOSTUNREACH:
            _tprintf( _T("sendto() failed with WSAEHOSTUNREACH\n") );
            break;
        case WSAECONNABORTED:
            _tprintf( _T("sendto() failed with WSAECONNABORTED\n") );
            break;
        case WSAECONNRESET:
            _tprintf( _T("sendto() failed with WSAECONNRESET\n") );
            break;
        case WSAEADDRNOTAVAIL:
            _tprintf( _T("sendto() failed with WSAEADDRNOTAVAIL\n") );
            break;
        case WSAEAFNOSUPPORT:
            _tprintf( _T("sendto() failed with WSAEAFNOSUPPORT\n") );
            break;
        case WSAEDESTADDRREQ:
            _tprintf( _T("sendto() failed with WSAEDESTADDRREQ\n") );
            break;
        case WSAENETUNREACH:
            _tprintf( _T("sendto() failed with WSAENETUNREACH\n") );
            break;
        case WSAETIMEDOUT:
            _tprintf( _T("sendto() failed with WSAETIMEDOUT\n") );
            break;
        default:
            _tprintf( _T("sendto() failed with unknown error\n") );
        }

        closesocket( s );
        return FALSE;
    }

    bWait = TRUE;

    while( bWait )
    {
        /* Wait for the DNS reply. */
        j = recvfrom( s,
                      pOutBuffer,
                      *pOutBufferLength,
                      0,
                      (SOCKADDR*)&SendAddr,
                      &SendAddrLen );
        if( j == SOCKET_ERROR )
        {
            switch( WSAGetLastError() )
            {
            case WSANOTINITIALISED:
                _tprintf( _T("recvfrom() failed with WSANOTINITIALIZED\n") );
                break;
            case WSAENETDOWN:
                _tprintf( _T("recvfrom() failed with WSAENETDOWN\n") );
                break;
            case WSAEACCES:
                _tprintf( _T("recvfrom() failed with WSAEACCES\n") );
                break;
            case WSAEINVAL:
                _tprintf( _T("recvfrom() failed with WSAEINVAL\n") );
                break;
            case WSAEINTR:
                _tprintf( _T("recvfrom() failed with WSAEINTR\n") );
                break;
            case WSAEINPROGRESS:
                _tprintf( _T("recvfrom() failed with WSAEINPROGRESS\n") );
                break;
            case WSAEFAULT:
                _tprintf( _T("recvfrom() failed with WSAEFAULT\n") );
                break;
            case WSAENETRESET:
                _tprintf( _T("recvfrom() failed with WSAENETRESET\n") );
                break;
            case WSAENOBUFS:
                _tprintf( _T("recvfrom() failed with WSAENOBUFS\n") );
                break;
            case WSAENOTCONN:
                _tprintf( _T("recvfrom() failed with WSAENOTCONN\n") );
                break;
            case WSAENOTSOCK:
                _tprintf( _T("recvfrom() failed with WSAENOTSOCK\n") );
                break;
            case WSAEOPNOTSUPP:
                _tprintf( _T("recvfrom() failed with WSAEOPNOTSUPP\n") );
                break;
            case WSAESHUTDOWN:
                _tprintf( _T("recvfrom() failed with WSAESHUTDOWN\n") );
                break;
            case WSAEWOULDBLOCK:
                _tprintf( _T("recvfrom() failed with WSAEWOULDBLOCK\n") );
                break;
            case WSAEMSGSIZE:
                _tprintf( _T("recvfrom() failed with WSAEMSGSIZE\n") );
                break;
            case WSAEHOSTUNREACH:
                _tprintf( _T("recvfrom() failed with WSAEHOSTUNREACH\n") );
                break;
            case WSAECONNABORTED:
                _tprintf( _T("recvfrom() failed with WSAECONNABORTED\n") );
                break;
            case WSAECONNRESET:
                _tprintf( _T("recvfrom() failed with WSAECONNRESET\n") );
                break;
            case WSAEADDRNOTAVAIL:
                _tprintf( _T("recvfrom() failed with WSAEADDRNOTAVAIL\n") );
                break;
            case WSAEAFNOSUPPORT:
                _tprintf( _T("recvfrom() failed with WSAEAFNOSUPPORT\n") );
                break;
            case WSAEDESTADDRREQ:
                _tprintf( _T("recvfrom() failed with WSAEDESTADDRREQ\n") );
                break;
            case WSAENETUNREACH:
                _tprintf( _T("recvfrom() failed with WSAENETUNREACH\n") );
                break;
            case WSAETIMEDOUT:
                _tprintf( _T("recvfrom() failed with WSAETIMEDOUT\n") );
                break;
            default:
                _tprintf( _T("recvfrom() failed with unknown error\n") );
            }

            closesocket( s );
            return FALSE;
        }

        ResponseID = ntohs( ((PSHORT)&pOutBuffer[0])[0] );

        if( ResponseID == RequestID ) bWait = FALSE;
    }

    /* We don't need the sockets anymore. */
    closesocket( s );

    /* If debug information then display debug information. */
    if( State.debug ) PrintDebug( pOutBuffer, j );

    /* Return the real output buffer length. */
    *pOutBufferLength = j;

    return TRUE;
}

void ReverseIP( PCHAR pIP, PCHAR pReturn )
{
    int i;
    int j;
    int k = 0;

    j = strlen( pIP ) - 1;
    i = j;

    /* We have A.B.C.D
       We will turn this into D.C.B.A and stick it in pReturn */

    /* A */
    for( ; i > 0; i -= 1 ) if( '.' == pIP[i] ) break;

    strncpy( &pReturn[k], &pIP[i + 1], (j - i) );
    k += (j - i);

    pReturn[k] = '.';
    k += 1;

    i -= 1;
    j = i;

    /* B */
    for( ; i > 0; i -= 1 ) if( '.' == pIP[i] ) break;

    strncpy( &pReturn[k], &pIP[i + 1], (j - i) );
    k += (j - i);

    pReturn[k] = '.';
    k += 1;

    i -= 1;
    j = i;

    /* C */
    for( ; i > 0; i -= 1 ) if( '.' == pIP[i] ) break;

    strncpy( &pReturn[k], &pIP[i + 1], (j - i) );
    k += (j - i);

    pReturn[k] = '.';
    k += 1;

    i -= 1;
    j = i;

    /* D */
    for( ; i > 0; i -= 1 );

    strncpy( &pReturn[k], &pIP[i], (j - i) + 1 );
    k += (j - i) + 1;

    pReturn[k] = '\0';
}

BOOL IsValidIP( PCHAR pInput )
{
    int i = 0, l = 0, b = 0, c = 1;

    /* Max length of an IP, e.g. 255.255.255.255, is 15 characters. */
    l = strlen( pInput );
    if( l > 15 ) return FALSE;

    /* 'b' is the count of the current segment. It gets reset after seeing a
       '.'. */
    for( ; i < l; i += 1 )
    {
        if( '.' == pInput[i] )
        {
            if( !b ) return FALSE;
            if( b > 3 ) return FALSE;

            b = 0;
            c += 1;
        }
        else
        {
            b += 1;

            if( (pInput[i] < '0') || (pInput[i] > '9') ) return FALSE;
        }
    }

    if( b > 3 ) return FALSE;

    /* 'c' is the number of segments seen. If it's less than 4, then it's not
       a valid IP. */
    if( c < 4 ) return FALSE;

    return TRUE;
}

int ExtractName( PCHAR pBuffer, PCHAR pOutput, USHORT Offset, UCHAR Limit )
{
    int c = 0, d = 0, i = 0, j = 0, k = 0, l = 0, m = 0;

    i = Offset;

    /* If Limit == 0, then we assume "no" limit. */
    d = Limit;
    if( 0 == Limit ) d = 255;

    while( d > 0 )
    {
        l = pBuffer[i] & 0xFF;
        i += 1;
        if( !m ) c += 1;

        if( 0xC0 == l )
        {
            if( !m ) c += 1;
            m = 1;
            d += (255 - Limit);
            i = pBuffer[i];
        }
        else
        {
            for( j = 0; j < l; j += 1 )
            {
                pOutput[k] = pBuffer[i];

                i += 1;
                if( !m ) c += 1;
                k += 1;
                d -= 1;
            }

            d -= 1;

            if( !pBuffer[i] || (d < 1) ) break;

            pOutput[k] = '.';
            k += 1;
        }
    };

    if( !m )
    {
        if( !Limit ) c += 1;
    }

    pOutput[k] = '\0';

    return c;
}

int ExtractIP( PCHAR pBuffer, PCHAR pOutput, USHORT Offset )
{
    int c = 0, l = 0, i = 0, v = 0;

    i = Offset;

    v = (UCHAR)pBuffer[i];
    l += 1;
    i += 1;

    sprintf( &pOutput[c], "%d.", v );
    c += strlen( &pOutput[c] );

    v = (UCHAR)pBuffer[i];
    l += 1;
    i += 1;

    sprintf( &pOutput[c], "%d.", v );
    c += strlen( &pOutput[c] );

    v = (UCHAR)pBuffer[i];
    l += 1;
    i += 1;

    sprintf( &pOutput[c], "%d.", v );
    c += strlen( &pOutput[c] );

    v = (UCHAR)pBuffer[i];
    l += 1;
    i += 1;

    sprintf( &pOutput[c], "%d", v );
    c += strlen( &pOutput[c] );

    pOutput[c] = '\0';

    return l;
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
                 OpcodeIDtoOpcodeName( (Header1 & 0x78) >> 3 ),
                 (int)RequestID,
                 RCodeIDtoRCodeName( Header2 & 0x0F ) );

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
            i += ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("        %s"), pName );

            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T(", type = %s, class = %s\n"),
                      TypeIDtoTypeName( Type ),
                      ClassIDtoClassName( Class ) );
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
                 OpcodeIDtoOpcodeName( (Header1 & 0x78) >> 3 ),
                 (int)ResponseID,
                 RCodeIDtoRCodeName( Header2 & 0x0F ) );

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
            i += ExtractName( pBuffer, pName, i, 0 );

            _tprintf( _T("        %s"), pName );

            Type = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            Class = ntohs( ((PUSHORT)&pBuffer[i])[0] );
            i += 2;

            _tprintf( _T(", type = %s, class = %s\n"),
                      TypeIDtoTypeName( Type ),
                      ClassIDtoClassName( Class ) );
        }
    }

    if( NumAnswers )
    {
        _tprintf( _T("    ANSWERS:\n") );

        for( k = 0; k < NumAnswers; k += 1 )
        {
            _tprintf( _T("    ->  ") );

            /* Print out the name. */
            i += ExtractName( pBuffer, pName, i, 0 );

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
                      TypeIDtoTypeName( Type ),
                      ClassIDtoClassName( Class ),
                      d );

            /* Print out the answer. */
            if( TYPE_A == Type )
            {
                i += ExtractIP( pBuffer, pName, i );

                _tprintf( _T("        internet address = %s\n"), pName );
            }
            else
            {
                i += ExtractName( pBuffer, pName, i, d );

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
            i += ExtractName( pBuffer, pName, i, 0 );

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
                      TypeIDtoTypeName( Type ),
                      ClassIDtoClassName( Class ),
                      d );

            /* TODO: There might be more types? */
            if( TYPE_NS == Type )
            {
                /* Print out the NS. */
                i += ExtractName( pBuffer, pName, i, d );

                _tprintf( _T("        nameserver = %s\n"), pName );

                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );
            }
            else if( TYPE_SOA == Type )
            {
                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );

                /* Print out the primary NS. */
                i += ExtractName( pBuffer, pName, i, 0 );

                _tprintf( _T("        primary name server = %s\n"), pName );

                /* Print out the responsible mailbox. */
                i += ExtractName( pBuffer, pName, i, 0 );

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
            i += ExtractName( pBuffer, pName, i, 0 );

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
                      TypeIDtoTypeName( Type ),
                      ClassIDtoClassName( Class ),
                      d );

            /* TODO: There might be more types? */
            if( TYPE_A == Type )
            {
                /* Print out the NS. */
                i += ExtractIP( pBuffer, pName, i );

                _tprintf( _T("        internet address = %s\n"), pName );

                _tprintf( _T("        ttl = %d ()\n"), (int)TTL );
            }
        }
    }

    _tprintf( _T("\n------------\n") );
}

PCHAR OpcodeIDtoOpcodeName( UCHAR Opcode )
{
    switch( Opcode & 0x0F )
    {
    case OPCODE_QUERY:
        return OpcodeQuery;

    case OPCODE_IQUERY:
        return OpcodeIQuery;

    case OPCODE_STATUS:
        return OpcodeStatus;

    default:
        return OpcodeReserved;
    }
}

PCHAR RCodeIDtoRCodeName( UCHAR RCode )
{
    switch( RCode & 0x0F )
    {
    case RCODE_NOERROR:
        return RCodeNOERROR;

    case RCODE_FORMERR:
        return RCodeFORMERR;

    case RCODE_FAILURE:
        return RCodeFAILURE;

    case RCODE_NXDOMAIN:
        return RCodeNXDOMAIN;

    case RCODE_NOTIMP:
        return RCodeNOTIMP;

    case RCODE_REFUSED:
        return RCodeREFUSED;

    default:
        return RCodeReserved;
    }
}

PCHAR TypeIDtoTypeName( USHORT TypeID )
{
    switch( TypeID )
    {
    case TYPE_A:
        return TypeA;

    case TYPE_NS:
        return TypeNS;

    case TYPE_CNAME:
        return TypeCNAME;

    case TYPE_SOA:
        return TypeSOA;

    case TYPE_WKS:
        return TypeSRV;

    case TYPE_PTR:
        return TypePTR;

    case TYPE_MX:
        return TypeMX;

    case TYPE_ANY:
        return TypeAny;

    default:
        return "Unknown";
    }
}

USHORT TypeNametoTypeID( PCHAR TypeName )
{
    if( !strncmp( TypeName, TypeA, strlen( TypeA ) ) ) return TYPE_A;
    if( !strncmp( TypeName, TypeNS, strlen( TypeNS ) ) ) return TYPE_NS;
    if( !strncmp( TypeName, TypeCNAME, strlen( TypeCNAME ) ) ) return TYPE_CNAME;
    if( !strncmp( TypeName, TypeSOA, strlen( TypeSOA ) ) ) return TYPE_SOA;
    if( !strncmp( TypeName, TypeSRV, strlen( TypeSRV ) ) ) return TYPE_WKS;
    if( !strncmp( TypeName, TypePTR, strlen( TypePTR ) ) ) return TYPE_PTR;
    if( !strncmp( TypeName, TypeMX, strlen( TypeMX ) ) ) return TYPE_MX;
    if( !strncmp( TypeName, TypeAny, strlen( TypeAny ) ) ) return TYPE_ANY;

    return 0;
}

PCHAR ClassIDtoClassName( USHORT ClassID )
{
    switch( ClassID )
    {
    case CLASS_IN:
        return ClassIN;

    case CLASS_ANY:
        return ClassAny;

    default:
        return "Unknown";
    }
}

USHORT ClassNametoClassID( PCHAR ClassName )
{
    if( !strncmp( ClassName, ClassIN, strlen( ClassIN ) ) ) return CLASS_IN;
    if( !strncmp( ClassName, ClassAny, strlen( ClassAny ) ) ) return CLASS_ANY;

    return 0;
}
