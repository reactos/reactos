/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    i_ntoa.c

Abstract:

    This module implements a routine to convert a numerical IP address
    into a dotted-decimal character string Internet address.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created
    davidtr     9-19-95     completely rewritten for performance

Notes:

    Exports:
        inet_ntoa()

--*/

#include "winsockp.h"
#include <string.h>

#define UC(b)   (((int)b)&0xff)

#define INTOA_Buffer ( ((PWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->INTOA_Buffer )

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

char * PASCAL
inet_ntoa(
    IN  struct in_addr  in
    )

/*++

Routine Description:

    This function takes an Internet address structure specified by the
    in parameter.  It returns an ASCII string representing the address
    in ".'' notation as "a.b.c.d".  Note that the string returned by
    inet_ntoa() resides in memory which is allocated by the Windows
    Sockets implementation.  The application should not make any
    assumptions about the way in which the memory is allocated.  The
    data is guaranteed to be valid until the next Windows Sockets API
    call within the same thread, but no longer.

Arguments:

    in - A structure which represents an Internet host address.

Return Value:

    If no error occurs, inet_ntoa() returns a char pointer to a static
    buffer containing the text address in standard "." notation.
    Otherwise, it returns NULL.  The data should be copied before
    another Windows Sockets call is made.

--*/

{
    PUCHAR p;
    PUCHAR buffer;
    PUCHAR b;

    WS_ENTER( "inet_ntoa", ULongToPtr( in.s_addr ), NULL, NULL, NULL );

    //
    // A number of applications apparently depend on calling inet_ntoa()
    // without first calling WSAStartup(). Because of this, we must perform
    // our own explicit thread initialization check here.
    //

    if( GET_THREAD_DATA() == NULL ) {

        if( !SockThreadInitialize() ) {

            SetLastError( WSAENOBUFS );
            WS_EXIT( "inet_ntoa", 0, TRUE );
            return NULL;

        }

    }

    WS_ASSERT( GET_THREAD_DATA() != NULL );

    buffer = INTOA_Buffer;
    b = buffer;

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

    WS_EXIT( "inet_ntoa", (INT_PTR)INTOA_Buffer, FALSE );
    return(buffer);
}
