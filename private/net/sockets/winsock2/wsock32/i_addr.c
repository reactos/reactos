/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    i_addr.c

Abstract:

    This module implements a routine to convert internet address expressed
    as dotted-decimal character strings into numerical representation.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        inet_addr()

--*/

#ident "@(#)inet_addr.c 5.3     3/8/91"
/*
 *      Copyright (c) 1987,  Spider Systems Limited
 */

/*      inet_addr.c     1.0     */


/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.inet_addr.c
 *      @(#)inet_addr.c 5.3
 *
 *      Last delta created      14:10:41 3/4/91
 *      This file extracted     11:20:19 3/8/91
 *
 */
/****************************************************************************/

#include "winsockp.h"
#include <ctype.h>

/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
unsigned long PASCAL
inet_addr(
    IN const char *cp
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

    cp - A character string representing a number expressed in the
        Internet standard "." notation.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        register unsigned long val, base, n;
        register char c;
        unsigned long parts[4], *pp = parts;

        WS_ENTER( "inet_addr", (PVOID)cp, NULL, NULL, NULL );
again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*cp == '0') {
                base = 8, cp++;
                if (*cp == 'x' || *cp == 'X')
                        base = 16, cp++;
	}
	
        while (c = *cp) {
                if (isdigit(c)) {
                        val = (val * base) + (c - '0');
                        cp++;
                        continue;
                }
                if (base == 16 && isxdigit(c)) {
                        val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
                        cp++;
                        continue;
                }
                break;
        }
        if (*cp == '.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        WS_EXIT( "inet_addr", -1, TRUE );
                        return ((unsigned long) -1);
                }
                *pp++ = val, cp++;
                goto again;
        }
        /*
         * Check for trailing characters.
         */
        if (*cp && !isspace(*cp)) {
                WS_EXIT( "inet_addr", -1, TRUE );
                return (INADDR_NONE);
        }
        *pp++ = val;
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = (unsigned long)(pp - parts);
        switch ((int) n) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    WS_EXIT( "inet_addr", -1, TRUE );
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    WS_EXIT( "inet_addr", -1, TRUE );
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    WS_EXIT( "inet_addr", -1, TRUE );
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                WS_EXIT( "inet_addr", -1, TRUE );
                return (INADDR_NONE);
        }
        val = htonl(val);
        WS_EXIT( "inet_addr", val, FALSE );
        return (val);
}
