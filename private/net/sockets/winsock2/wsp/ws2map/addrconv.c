/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    addrconv.c

Abstract:

    This module contains address conversion routines for the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        WSPAddressToString()
        WSPStringToAddress()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Macros for swapping the bytes in a long and a short.
//

#define SWAP_LONG(l)                                \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

#define WS_SWAP_SHORT(s)                            \
            ( ( ((s) >> 8) & 0x00FF ) |             \
              ( ((s) << 8) & 0xFF00 ) )


//
// Private prototypes.
//

ULONG
MyInetAddr(
    IN LPWSTR String,
    OUT LPWSTR * Terminator
    );


//
// Public functions.
//


INT
WSPAPI
WSPAddressToString(
    IN LPSOCKADDR lpsaAddress,
    IN DWORD dwAddressLength,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPWSTR lpszAddressString,
    IN OUT LPDWORD lpdwAddressStringLength,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine converts all components of a SOCKADDR structure into a human-
    readable string representation of the address. This is used mainly for
    display purposes.

Arguments:

    lpsaAddress - Points to a SOCKADDR structure to translate into a string.

    dwAddressLength - The length of the Address SOCKADDR.

    lpProtocolInfo - The WSAPROTOCOL_INFOW struct for a particular provider.

    lpszAddressString - A buffer which receives the human-readable address
        string.

    lpdwAddressStringLength - The length of the AddressString buffer. Returns
        the length of the string actually copied into the buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPAddressToString() returns 0. Otherwise, it returns
        SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/

{

    INT err;
    INT length;
    INT result;
    LPSOCKADDR_IN addr;
    CHAR ansiString[sizeof("aaa.aaa.aaa.aaa:ppppp")];

    SOCK_ENTER( "WSPAddressToString", lpsaAddress, (PVOID)dwAddressLength, lpProtocolInfo, lpszAddressString );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPAddressToString", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Quick sanity check.
    //

    if( lpsaAddress == NULL ||
        lpszAddressString == NULL ||
        lpdwAddressStringLength == NULL ) {

        err = WSAEFAULT;
        goto exit;

    }

    if( lpsaAddress->sa_family != AF_INET ) {

        err = WSA_INVALID_PARAMETER;
        goto exit;

    }

    //
    // Convert the address to string locally.
    //

    addr = (LPSOCKADDR_IN)lpsaAddress;

    length = wsprintf(
                 ansiString,
                 "%d.%d.%d.%d:%u",
                 ( addr->sin_addr.s_addr >>  0 ) & 0xFF,
                 ( addr->sin_addr.s_addr >>  8 ) & 0xFF,
                 ( addr->sin_addr.s_addr >> 16 ) & 0xFF,
                 ( addr->sin_addr.s_addr >> 24 ) & 0xFF
                 );

    if( addr->sin_port != 0 ) {

        length =+ wsprintf(
                      ansiString + length,
                      ":%u",
                      WS_SWAP_SHORT( addr->sin_port )
                      );

    }

    SOCK_ASSERT( length < sizeof(ansiString) );

    //
    // Map it to UNICODE.
    //

    result = MultiByteToWideChar(
                 CP_ACP,
                 0,
                 ansiString,
                 -1,
                 lpszAddressString,
                 (INT)*lpdwAddressStringLength
                 );

    if( result == 0 ) {

        err = WSAEFAULT;
        goto exit;

    }

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    *lpdwAddressStringLength = (DWORD)result;
    result = 0;

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SOCK_EXIT( "WSPAddressToString", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPAddressToString



INT
WSPAPI
WSPStringToAddress(
    IN LPWSTR AddressString,
    IN INT AddressFamily,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPSOCKADDR lpAddress,
    IN OUT LPINT lpAddressLength,
    OUT LPINT lpErrno
    )

/*++

Routine Description:

    This routine converts a human-readable string to a socket address
    structure (SOCKADDR) suitable for pass to Windows Sockets routines which
    take such a structure. Any missing components of the address will be
    defaulted to a reasonable value if possible. For example, a missing port
    number will be defaulted to zero.

Arguments:

    AddressString - Points to the zero-terminated human-readable string to
        convert.

    AddressFamily - The address family to which the string belongs, or
        AF_UNSPEC if it is unknown.

    lpProtocolInfo - The provider's WSAPROTOCOL_INFOW struct.

    lpAddress - A buffer which is filled with a single SOCKADDR structure.

    lpAddressLength - The length of the Address buffer. Returns the size of
        the resultant SOCKADDR structure.

    lpErrno - A pointer to the error code.

Return Value:

    If no error occurs, WSPStringToAddress() returns 0. Otherwise, a value
        of SOCKET_ERROR is returned, and a specific error code is available
        in lpErrno.

--*/

{

    INT err;
    INT result;
    LPWSTR terminator;
    ULONG ipAddress;
    USHORT port;
    LPSOCKADDR_IN addr;

    SOCK_ENTER( "WSPStringToAddress", AddressString, (PVOID)AddressFamily, lpProtocolInfo, lpAddress );

    SOCK_ASSERT( lpErrno != NULL );

    err = SockEnterApi( TRUE, FALSE );

    if( err != NO_ERROR ) {

        *lpErrno = err;
        SOCK_EXIT( "WSPStringToAddress", SOCKET_ERROR, lpErrno );
        return SOCKET_ERROR;

    }

    //
    // Quick sanity check.
    //

    if( AddressString == NULL ||
        lpAddress == NULL ||
        lpAddressLength == NULL ||
        *lpAddressLength < sizeof(SOCKADDR_IN) ) {

        err = WSAEFAULT;
        goto exit;

    }

    if( AddressFamily != AF_INET ) {

        err = WSA_INVALID_PARAMETER;
        goto exit;

    }

    //
    // Convert it.
    //

    //
    // BUGBUG: VERIFY THE WCHAR CRT FUNCTIONS WORK UNDER WIN95!
    //

    ipAddress = MyInetAddr( AddressString, &terminator );

    if( ipAddress == INADDR_NONE ) {

        err = WSA_INVALID_PARAMETER;
        goto exit;

    }

    if( *terminator == L':' ) {

        WCHAR ch;
        USHORT base;

        terminator++;

        port = 0;
        base = 10;

        if( *terminator == L'0' ) {
            base = 8;
            terminator++;

            if( *terminator == L'x' ) {
                base = 16;
                terminator++;
            }
        }

        while( ch = *terminator++ ) {
            if( iswdigit(ch) ) {
                port = ( port * base ) + ( ch - L'0' );
            } else if( base == 16 && iswxdigit(ch) ) {
                port = ( port << 4 );
                port += ch + 10 - ( iswlower(ch) ? L'a' : L'A' );
            } else {
                return WSA_INVALID_PARAMETER;
            }
        }

    } else {
        port = 0;
    }

    //
    // Build the address.
    //

    ZeroMemory(
        lpAddress,
        sizeof(SOCKADDR_IN)
        );

    addr = (LPSOCKADDR_IN)lpAddress;
    *lpAddressLength = sizeof(SOCKADDR_IN);

    addr->sin_family = AF_INET;
    addr->sin_port = port;
    addr->sin_addr.s_addr = ipAddress;

    //
    // Success!
    //

    SOCK_ASSERT( err == NO_ERROR );

    result = 0;

exit:

    if( err != NO_ERROR ) {

        *lpErrno = err;
        result = SOCKET_ERROR;

    }

    SOCK_EXIT( "WSPAddressToString", result, (result == SOCKET_ERROR) ? lpErrno : NULL );
    return result;

}   // WSPStringToAddress


//
// Private functions.
//


ULONG
MyInetAddr(
    IN LPWSTR String,
    OUT LPWSTR * Terminator
    )

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    String - A character string representing a number expressed in the
        Internet standard "." notation.

    Terminator - Receives a pointer to the character that terminated
        the conversion.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        ULONG val, base, n;
        WCHAR c;
        ULONG parts[4], *pp = parts;

again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*String == L'0') {
                base = 8, String++;
                if (*String == L'x' || *String == L'X')
                        base = 16, String++;
        }

        while (c = *String) {
                if (iswdigit(c)) {
                        val = (val * base) + (c - L'0');
                        String++;
                        continue;
                }
                if (base == 16 && iswxdigit(c)) {
                        val = (val << 4) + (c + 10 - (islower(c) ? L'a' : L'A'));
                        String++;
                        continue;
                }
                break;
        }
        if (*String == L'.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        *Terminator = String;
                        return ((ULONG) -1);
                }
                *pp++ = val, String++;
                goto again;
        }
        /*
         * Check for trailing characters.
         */
        if (*String && !iswspace(*String) && (*String != L':')) {
                *Terminator = String;
                return (INADDR_NONE);
        }
        *pp++ = val;
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = (ULONG)(pp - parts);
        switch ((int) n) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                *Terminator = String;
                return (INADDR_NONE);
        }

        val = SWAP_LONG(val);
        *Terminator = String;
        return (val);

}   // MyInetAddr
