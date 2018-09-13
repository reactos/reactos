/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    addrconv.c

Abstract:

    This module contains the address conversion routines from the
    winsock2 API. This module contains the following functions.

    htonl()
    htons()
    ntohl()
    ntohs()
    inet_addr()
    inet_ntoa()
    WSAHtonl()
    WSAHtons()
    WSANtohl()
    WSANtohs()


Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

[Environment:]

[Notes:]

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h

--*/

#include "precomp.h"

// these defines are used to check if address parts are in range
#define MAX_EIGHT_BIT_VALUE       0xff
#define MAX_SIXTEEN_BIT_VALUE     0xffff
#define MAX_TWENTY_FOUR_BIT_VALUE 0xffffff

// Defines for different based numbers in an address
#define BASE_TEN     10
#define BASE_EIGHT   8
#define BASE_SIXTEEN 16

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
// This preinitialized array defines the strings to be used for
// inet_ntoa.  The index of each row corresponds to the value for a byte
// in an IP address.  The first three bytes of each row are the
// char/string value for the byte, and the fourth byte in each row is
// the length of the string required for the byte.  This approach
// allows a fast implementation with no jumps.
//

BYTE NToACharStrings[][4] = {
    '0', 'x', 'x', 1,
    '1', 'x', 'x', 1,
    '2', 'x', 'x', 1,
    '3', 'x', 'x', 1,
    '4', 'x', 'x', 1,
    '5', 'x', 'x', 1,
    '6', 'x', 'x', 1,
    '7', 'x', 'x', 1,
    '8', 'x', 'x', 1,
    '9', 'x', 'x', 1,
    '1', '0', 'x', 2,
    '1', '1', 'x', 2,
    '1', '2', 'x', 2,
    '1', '3', 'x', 2,
    '1', '4', 'x', 2,
    '1', '5', 'x', 2,
    '1', '6', 'x', 2,
    '1', '7', 'x', 2,
    '1', '8', 'x', 2,
    '1', '9', 'x', 2,
    '2', '0', 'x', 2,
    '2', '1', 'x', 2,
    '2', '2', 'x', 2,
    '2', '3', 'x', 2,
    '2', '4', 'x', 2,
    '2', '5', 'x', 2,
    '2', '6', 'x', 2,
    '2', '7', 'x', 2,
    '2', '8', 'x', 2,
    '2', '9', 'x', 2,
    '3', '0', 'x', 2,
    '3', '1', 'x', 2,
    '3', '2', 'x', 2,
    '3', '3', 'x', 2,
    '3', '4', 'x', 2,
    '3', '5', 'x', 2,
    '3', '6', 'x', 2,
    '3', '7', 'x', 2,
    '3', '8', 'x', 2,
    '3', '9', 'x', 2,
    '4', '0', 'x', 2,
    '4', '1', 'x', 2,
    '4', '2', 'x', 2,
    '4', '3', 'x', 2,
    '4', '4', 'x', 2,
    '4', '5', 'x', 2,
    '4', '6', 'x', 2,
    '4', '7', 'x', 2,
    '4', '8', 'x', 2,
    '4', '9', 'x', 2,
    '5', '0', 'x', 2,
    '5', '1', 'x', 2,
    '5', '2', 'x', 2,
    '5', '3', 'x', 2,
    '5', '4', 'x', 2,
    '5', '5', 'x', 2,
    '5', '6', 'x', 2,
    '5', '7', 'x', 2,
    '5', '8', 'x', 2,
    '5', '9', 'x', 2,
    '6', '0', 'x', 2,
    '6', '1', 'x', 2,
    '6', '2', 'x', 2,
    '6', '3', 'x', 2,
    '6', '4', 'x', 2,
    '6', '5', 'x', 2,
    '6', '6', 'x', 2,
    '6', '7', 'x', 2,
    '6', '8', 'x', 2,
    '6', '9', 'x', 2,
    '7', '0', 'x', 2,
    '7', '1', 'x', 2,
    '7', '2', 'x', 2,
    '7', '3', 'x', 2,
    '7', '4', 'x', 2,
    '7', '5', 'x', 2,
    '7', '6', 'x', 2,
    '7', '7', 'x', 2,
    '7', '8', 'x', 2,
    '7', '9', 'x', 2,
    '8', '0', 'x', 2,
    '8', '1', 'x', 2,
    '8', '2', 'x', 2,
    '8', '3', 'x', 2,
    '8', '4', 'x', 2,
    '8', '5', 'x', 2,
    '8', '6', 'x', 2,
    '8', '7', 'x', 2,
    '8', '8', 'x', 2,
    '8', '9', 'x', 2,
    '9', '0', 'x', 2,
    '9', '1', 'x', 2,
    '9', '2', 'x', 2,
    '9', '3', 'x', 2,
    '9', '4', 'x', 2,
    '9', '5', 'x', 2,
    '9', '6', 'x', 2,
    '9', '7', 'x', 2,
    '9', '8', 'x', 2,
    '9', '9', 'x', 2,
    '1', '0', '0', 3,
    '1', '0', '1', 3,
    '1', '0', '2', 3,
    '1', '0', '3', 3,
    '1', '0', '4', 3,
    '1', '0', '5', 3,
    '1', '0', '6', 3,
    '1', '0', '7', 3,
    '1', '0', '8', 3,
    '1', '0', '9', 3,
    '1', '1', '0', 3,
    '1', '1', '1', 3,
    '1', '1', '2', 3,
    '1', '1', '3', 3,
    '1', '1', '4', 3,
    '1', '1', '5', 3,
    '1', '1', '6', 3,
    '1', '1', '7', 3,
    '1', '1', '8', 3,
    '1', '1', '9', 3,
    '1', '2', '0', 3,
    '1', '2', '1', 3,
    '1', '2', '2', 3,
    '1', '2', '3', 3,
    '1', '2', '4', 3,
    '1', '2', '5', 3,
    '1', '2', '6', 3,
    '1', '2', '7', 3,
    '1', '2', '8', 3,
    '1', '2', '9', 3,
    '1', '3', '0', 3,
    '1', '3', '1', 3,
    '1', '3', '2', 3,
    '1', '3', '3', 3,
    '1', '3', '4', 3,
    '1', '3', '5', 3,
    '1', '3', '6', 3,
    '1', '3', '7', 3,
    '1', '3', '8', 3,
    '1', '3', '9', 3,
    '1', '4', '0', 3,
    '1', '4', '1', 3,
    '1', '4', '2', 3,
    '1', '4', '3', 3,
    '1', '4', '4', 3,
    '1', '4', '5', 3,
    '1', '4', '6', 3,
    '1', '4', '7', 3,
    '1', '4', '8', 3,
    '1', '4', '9', 3,
    '1', '5', '0', 3,
    '1', '5', '1', 3,
    '1', '5', '2', 3,
    '1', '5', '3', 3,
    '1', '5', '4', 3,
    '1', '5', '5', 3,
    '1', '5', '6', 3,
    '1', '5', '7', 3,
    '1', '5', '8', 3,
    '1', '5', '9', 3,
    '1', '6', '0', 3,
    '1', '6', '1', 3,
    '1', '6', '2', 3,
    '1', '6', '3', 3,
    '1', '6', '4', 3,
    '1', '6', '5', 3,
    '1', '6', '6', 3,
    '1', '6', '7', 3,
    '1', '6', '8', 3,
    '1', '6', '9', 3,
    '1', '7', '0', 3,
    '1', '7', '1', 3,
    '1', '7', '2', 3,
    '1', '7', '3', 3,
    '1', '7', '4', 3,
    '1', '7', '5', 3,
    '1', '7', '6', 3,
    '1', '7', '7', 3,
    '1', '7', '8', 3,
    '1', '7', '9', 3,
    '1', '8', '0', 3,
    '1', '8', '1', 3,
    '1', '8', '2', 3,
    '1', '8', '3', 3,
    '1', '8', '4', 3,
    '1', '8', '5', 3,
    '1', '8', '6', 3,
    '1', '8', '7', 3,
    '1', '8', '8', 3,
    '1', '8', '9', 3,
    '1', '9', '0', 3,
    '1', '9', '1', 3,
    '1', '9', '2', 3,
    '1', '9', '3', 3,
    '1', '9', '4', 3,
    '1', '9', '5', 3,
    '1', '9', '6', 3,
    '1', '9', '7', 3,
    '1', '9', '8', 3,
    '1', '9', '9', 3,
    '2', '0', '0', 3,
    '2', '0', '1', 3,
    '2', '0', '2', 3,
    '2', '0', '3', 3,
    '2', '0', '4', 3,
    '2', '0', '5', 3,
    '2', '0', '6', 3,
    '2', '0', '7', 3,
    '2', '0', '8', 3,
    '2', '0', '9', 3,
    '2', '1', '0', 3,
    '2', '1', '1', 3,
    '2', '1', '2', 3,
    '2', '1', '3', 3,
    '2', '1', '4', 3,
    '2', '1', '5', 3,
    '2', '1', '6', 3,
    '2', '1', '7', 3,
    '2', '1', '8', 3,
    '2', '1', '9', 3,
    '2', '2', '0', 3,
    '2', '2', '1', 3,
    '2', '2', '2', 3,
    '2', '2', '3', 3,
    '2', '2', '4', 3,
    '2', '2', '5', 3,
    '2', '2', '6', 3,
    '2', '2', '7', 3,
    '2', '2', '8', 3,
    '2', '2', '9', 3,
    '2', '3', '0', 3,
    '2', '3', '1', 3,
    '2', '3', '2', 3,
    '2', '3', '3', 3,
    '2', '3', '4', 3,
    '2', '3', '5', 3,
    '2', '3', '6', 3,
    '2', '3', '7', 3,
    '2', '3', '8', 3,
    '2', '3', '9', 3,
    '2', '4', '0', 3,
    '2', '4', '1', 3,
    '2', '4', '2', 3,
    '2', '4', '3', 3,
    '2', '4', '4', 3,
    '2', '4', '5', 3,
    '2', '4', '6', 3,
    '2', '4', '7', 3,
    '2', '4', '8', 3,
    '2', '4', '9', 3,
    '2', '5', '0', 3,
    '2', '5', '1', 3,
    '2', '5', '2', 3,
    '2', '5', '3', 3,
    '2', '5', '4', 3,
    '2', '5', '5', 3
};



u_long WSAAPI
htonl (
    IN u_long hostlong
    )
/*++
Routine Description:

    Convert a u_long from host to TCP/IP network byte order.

Arguments:

    hostlong - A 32-bit number in host byte order.

Returns:
    htonl() returns the value in TCP/IP network byte order.
--*/
{

    return SWAP_LONG( hostlong );

}



u_short WSAAPI
htons (
    IN u_short hostshort
    )
/*++
Routine Description:

    Convert a u_short from host to TCP/IP network byte order.

Arguments:

    hostshort - A 16-bit number in host byte order.

Returns:
    htons() returns the value in TCP/IP network byte order.
--*/
{

    return WS_SWAP_SHORT( hostshort );

}




u_long WSAAPI
ntohl (
    IN u_long netlong
    )
/*++
Routine Description:

    Convert a u_long from TCP/IP network order to host byte order.

Arguments:

    netlong   A 32-bit number in TCP/IP network byte order.

Returns:
    ntohl() returns the value in host byte order.
--*/
{

    return SWAP_LONG( netlong );

}


u_short WSAAPI
ntohs (
    IN u_short netshort
    )
/*++
Routine Description:

    Convert a u_short from TCP/IP network byte order to host byte order.

Arguments:

    netshort  A 16-bit number in TCP/IP network byte order.

Returns:
    ntohs() returns the value in host byte order.
--*/
{

    return WS_SWAP_SHORT( netshort );

}



unsigned long WSAAPI
inet_addr (
           IN const char FAR * cp
           )
/*++
Routine Description:

    Convert a string containing an Internet Protocol dotted address into an
    in_addr.

Arguments:

    cp - A null terminated character string representing a number expressed in
         the Internet standard ".'' notation.

Returns:

    If no error occurs, inet_addr() returns an unsigned long containing a
    suitable binary representation of the Internet address given.  If the
    passed-in string does not contain a legitimate Internet address, for
    example if a portion of an "a.b.c.d" address exceeds 255, inet_addr()
    returns the value INADDR_NONE.

--*/
{
    u_long value;                // value to return to the user
    u_long number_base;          // The number base in use for an
                                 // address field
    u_long address_field_count;  // How many fields where found in the
                                 // address string
    char c;                      // temp variable to hold the charater
                                 // that is being processed currently
    u_long fields[4];            // an array of unsigned longs to
                                 // recieve the values from each field
                                 // in the address
    u_long *p_fields = fields;   // a pointer used to index through
                                 // the 'fields' array
    BOOL MoreFields = TRUE;      // Are there more address fields to scan

    if( cp == NULL ) {
        SetLastError( WSAEFAULT );
        return INADDR_NONE;
    }

    __try {
        while (MoreFields) {
//
//    Collect number up to ``.''.
//    Values are specified as for C:
//    0x=hex, 0=octal, other=decimal.
//
            value = 0;
            number_base = BASE_TEN;
            // Is the first charater '0' ?
            // The default number base is base ten. If the first charater in
            // an address field is '0' then the user is using octal or hex
            // notation for the assress field
            if (*cp == '0') {
                // If the second charater in the field is x or X then this is
                // a hex number else it is an octal number.
                if (*++cp == 'x' ||
                    *cp == 'X') {
                    number_base = BASE_SIXTEEN;
                    cp++; // skip the x
                }
                else {
                    number_base = BASE_EIGHT;
                }
            }

            // Process the charaters in the address string until a non digit
            // charater is found.
            c = *cp;
            while (c) {
                if (isdigit(c)) {
                    value = (value * number_base) + (c - '0');
                    cp++;
                    c = *cp;
                    continue;
                }
                if ((number_base == BASE_SIXTEEN) && isxdigit(c)) {
                    value = (value << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
                    cp++;
                    c = *cp;
                    continue;
                }
                break;
            }

            // Is the charater following the the number a '.'. If so skip the
            // the '.' and scan the next field.
            if (*cp == '.') {
                /*
                 * Internet format:
                 *  a.b.c.d
                 *  a.b.c   (with c treated as 16-bits)
                 *  a.b (with b treated as 24 bits)
                 */
                if (p_fields >= fields + 3) {
                    // and internet address cannot have more than 4 fields so
                    // return an error
                    return (INADDR_NONE);
                }
                // set the value of this part of the addess and advance
                // the pointer to the next part
                *p_fields++ = value;
                //
                cp++;
            }
            else {
                MoreFields=FALSE;
            } //else
        } //while

        //
        //  Check for trailing characters. A valid address can end with
        //  NULL or whitespace. An address may not end with a '.'
        //
        if ((*cp == '\0' && *(cp - 1) == '.') ||
            (*cp && !isspace(*cp))) {
            return (INADDR_NONE);
        }
    }
    __except (WS2_EXCEPTION_FILTER()) {
        SetLastError (WSAEFAULT);
        return (INADDR_NONE);
    }

    // set the the value of the final field in the address
    *p_fields++ = value;


    //
    // Concoct the address according to the number of fields
    // specified.
    //
    address_field_count = (u_long)(p_fields - fields);
    switch (address_field_count) {

      case 1:               // a -- 32 bits
        value = fields[0];
        break;

      case 2:               // a.b -- 8.24 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_TWENTY_FOUR_BIT_VALUE) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            (fields[1] & MAX_TWENTY_FOUR_BIT_VALUE);
        } //else
        break;

      case 3:               // a.b.c -- 8.8.16 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_EIGHT_BIT_VALUE ||
            fields[2] > MAX_SIXTEEN_BIT_VALUE ) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            ((fields[1] & MAX_EIGHT_BIT_VALUE) << 16) |
            (fields[2] & MAX_SIXTEEN_BIT_VALUE);
        } //else
        break;

      case 4:            // a.b.c.d -- 8.8.8.8 bits
        if (fields[0] > MAX_EIGHT_BIT_VALUE ||
            fields[1] > MAX_EIGHT_BIT_VALUE ||
            fields[2] > MAX_EIGHT_BIT_VALUE ||
            fields[3] > MAX_EIGHT_BIT_VALUE ) {
            return (INADDR_NONE);
        } //if
        else {
            value = (fields[0] << 24) |
            ((fields[1] & MAX_EIGHT_BIT_VALUE) << 16) |
            ((fields[2] & MAX_EIGHT_BIT_VALUE) << 8) |
            (fields[3] & MAX_EIGHT_BIT_VALUE);
        } //else
        break;

        // if the address string handed to us has more than 4 address
        // fields return an error
      default:
        return (INADDR_NONE);
    } // switch
    // convert the value to network byte order and return it to the user.
    value = htonl(value);
    return (value);
}




char FAR * WSAAPI
inet_ntoa (
    IN struct in_addr in
    )
/*++
Routine Description:

    Convert a network address into a string in dotted format.

Arguments:

        in - A structure which represents an Internet host address.

Returns:
    If no error occurs, inet_ntoa() returns a char pointer to a static buffer
    containing the text address in standard ".'' notation.  Otherwise, it
    returns NULL.  The data should be copied before another WinSock call is
    made.
--*/
{
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;
    PCHAR     Buffer=NULL;
    BOOL      AddedArtificialStartup = FALSE;
    WSADATA   wsaData;
    PUCHAR p;
    PUCHAR b;

    ErrorCode = PROLOG(&Process,&Thread);
    if (ERROR_SUCCESS != ErrorCode) {
        if( ErrorCode != WSANOTINITIALISED ) {
            SetLastError(ErrorCode);
            return(NULL);
        }

        //
        // PROLOG failed with WSANOTINITIALIZED, meaning the app has not
        // yet called WSAStartup(). For historical (hysterical?) reasons,
        // inet_ntoa() must be functional before WSAStartup() is called.
        // So, we'll add an artificial WSAStartup() and press on.
        //

        ErrorCode = WSAStartup( WINSOCK_HIGH_API_VERSION, &wsaData );

        if( ErrorCode != NO_ERROR ) {
            SetLastError( ErrorCode );
            return NULL;
        }

        AddedArtificialStartup = TRUE;

        //
        // Retry the PROLOG.
        //

        ErrorCode = PROLOG(&Process,&Thread);
        if (ErrorCode!=ERROR_SUCCESS) {
            WSACleanup();
            SetLastError(ErrorCode);
            return NULL;
        }

    } //if

    Buffer = Thread->GetResultBuffer();
    b = (PUCHAR)Buffer;

    //
    // In an unrolled loop, calculate the string value for each of the four
    // bytes in an IP address.  Note that for values less than 100 we will
    // do one or two extra assignments, but we save a test/jump with this
    // algorithm.
    //

    p = (PUCHAR)&in;

    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b = '\0';

    if( AddedArtificialStartup ) {
        WSACleanup();
    }

    return(Buffer);
}





int WSAAPI
WSAHtonl (
    IN SOCKET s,
    IN u_long hostlong,
    OUT u_long FAR * lpnetlong
    )
/*++
Routine Description:

    Convert a u_long from a specified host byte order to network byte
    order.

Arguments:

    s - A descriptor identifying a socket.

    hostlong - A 32-bit number in host byte order.

    lpnetlong - A pointer to a 32-bit number in network byte order.


Returns:
    If no error occurs, WSAHtonl() returns 0. Otherwise, a value of
    SOCKET_ERROR is returned.

--*/
{
    PDSOCKET            Socket;
    INT                 ErrorCode;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

		if( lpnetlong == NULL ) {
			SetLastError( WSAEFAULT );
			return(SOCKET_ERROR);
		}

		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
			CatalogEntry = Socket->GetCatalogItem();
			ProtocolInfo = CatalogEntry->GetProtocolInfo();

            __try {
			    if (LITTLEENDIAN == ProtocolInfo->iNetworkByteOrder) {
				    *lpnetlong = hostlong;
			    } //if
			    else {
				    *lpnetlong = SWAP_LONG( hostlong );
			    } //else
                ErrorCode = ERROR_SUCCESS;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
            }

			Socket->DropDSocketReference();
            if (ErrorCode==ERROR_SUCCESS)
                return ErrorCode;
		} //if
		else
			ErrorCode = WSAENOTSOCK;
	}


    SetLastError(ErrorCode);
    return (SOCKET_ERROR);
}




int WSAAPI
WSAHtons(
    IN SOCKET s,
    IN u_short hostshort,
    OUT u_short FAR * lpnetshort )
/*++
Routine Description:

    Convert a u_short from a specified host byte order to network byte
    order.

Arguments:

    s - A descriptor identifying a socket.

    netshort - A 16-bit number in network byte order.

    lphostshort - A pointer to a 16-bit number in host byte order.

Returns:
     If no error occurs, WSANtohs() returns 0. Otherwise, a value of
     SOCKET_ERROR is returned.

--*/
{
    PDSOCKET            Socket;
    INT                 ErrorCode;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

		if( lpnetshort == NULL ) {
			SetLastError( WSAEFAULT );
			return(SOCKET_ERROR);
		}

		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
			CatalogEntry = Socket->GetCatalogItem();
			ProtocolInfo = CatalogEntry->GetProtocolInfo();
            __try {
			    if (LITTLEENDIAN == ProtocolInfo->iNetworkByteOrder) {
				    *lpnetshort = hostshort;
			    } //if
			    else {
				    *lpnetshort = WS_SWAP_SHORT( hostshort );
			    } //else

			    ErrorCode = ERROR_SUCCESS;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
            }
			Socket->DropDSocketReference();
            if (ErrorCode==ERROR_SUCCESS)
                return ErrorCode;
		} //if
		else
			ErrorCode = WSAENOTSOCK;
	}

    SetLastError(ErrorCode);
    return (SOCKET_ERROR);
}




int WSAAPI
WSANtohl (
    IN SOCKET s,
    IN u_long netlong,
    OUT u_long FAR * lphostlong
    )
/*++
Routine Description:

    Convert a u_long from network byte order to host byte order.

Arguments:
    s - A descriptor identifying a socket.

    netlong - A 32-bit number in network byte order.

    lphostlong - A pointer to a 32-bit number in host byte order.

Returns:
     If no error occurs, WSANtohs() returns 0. Otherwise, a value of
     SOCKET_ERROR is returned.
--*/
{
    PDSOCKET            Socket;
    INT                 ErrorCode;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

		if( lphostlong == NULL ) {
			SetLastError( WSAEFAULT );
			return(SOCKET_ERROR);
		}


		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
			CatalogEntry = Socket->GetCatalogItem();
			ProtocolInfo = CatalogEntry->GetProtocolInfo();

            __try {
			    if (LITTLEENDIAN == ProtocolInfo->iNetworkByteOrder) {
				    *lphostlong = netlong;
			    } //if
			    else {
				    *lphostlong = SWAP_LONG( netlong );
			    } //else
			    ErrorCode = ERROR_SUCCESS;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
            }
			Socket->DropDSocketReference();
            if (ErrorCode==ERROR_SUCCESS)
                return ErrorCode;
		} //if
		else
			ErrorCode = WSAENOTSOCK;
	}

    SetLastError(ErrorCode);
    return (SOCKET_ERROR);
}


int WSAAPI
WSANtohs (
    IN SOCKET s,
    IN u_short netshort,
    OUT u_short FAR * lphostshort
    )
/*++
Routine Description:


Arguments:

Returns:
    Zero on success else SOCKET_ERROR. The error code is stored with
    SetErrorCode().
--*/
{
    PDSOCKET            Socket;
    INT                 ErrorCode;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = TURBO_PROLOG();
    if (ErrorCode==ERROR_SUCCESS) {

		if( lphostshort == NULL ) {
			SetLastError( WSAEFAULT );
			return(SOCKET_ERROR);
		}


		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
			CatalogEntry = Socket->GetCatalogItem();
			ProtocolInfo = CatalogEntry->GetProtocolInfo();

            __try {
			    if (LITTLEENDIAN == ProtocolInfo->iNetworkByteOrder) {
				    *lphostshort = netshort;
			    } //if
			    else {
				    *lphostshort = WS_SWAP_SHORT( netshort );
			    } //else
			    ErrorCode = ERROR_SUCCESS;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
            }
			Socket->DropDSocketReference();
            if (ErrorCode==ERROR_SUCCESS)
                return ErrorCode;
		} //if
		else
			ErrorCode = WSAENOTSOCK;
	}

    SetLastError(ErrorCode);
    return (SOCKET_ERROR);
}
