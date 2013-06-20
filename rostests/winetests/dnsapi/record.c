/*
 * Tests for record handling functions
 *
 * Copyright 2006 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static char name1[] = "localhost";
static char name2[] = "LOCALHOST";

static DNS_RECORDA r1 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };
static DNS_RECORDA r2 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };
static DNS_RECORDA r3 = { NULL, name1, DNS_TYPE_A, sizeof(DNS_A_DATA), { 0 }, 1200, 0, { { 0xffffffff } } };

static void test_DnsRecordCompare( void )
{
    ok( DnsRecordCompare( &r1, &r1 ) == TRUE, "failed unexpectedly\n" );

    r2.pName = name2;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetUnicode;
    ok( DnsRecordCompare( &r1, &r2 ) == FALSE, "succeeded unexpectedly\n" );

    r2.Flags.S.CharSet = DnsCharSetAnsi;
    ok( DnsRecordCompare( &r1, &r2 ) == FALSE, "succeeded unexpectedly\n" );

    r1.Flags.S.CharSet = DnsCharSetAnsi;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r1.dwTtl = 0;
    ok( DnsRecordCompare( &r1, &r2 ) == TRUE, "failed unexpectedly\n" );

    r2.Data.A.IpAddress = 0;
    ok( DnsRecordCompare( &r1, &r2 ) == FALSE, "succeeded unexpectedly\n" );
}

static void test_DnsRecordSetCompare( void )
{
    DNS_RECORD *diff1;
    DNS_RECORD *diff2;
    DNS_RRSET rr1, rr2;

    r1.Flags.DW = 0x2019;
    r2.Flags.DW = 0x2019;
    r2.Data.A.IpAddress = 0xffffffff;

    DNS_RRSET_INIT( rr1 );
    DNS_RRSET_INIT( rr2 );

    DNS_RRSET_ADD( rr1, &r1 );
    DNS_RRSET_ADD( rr2, &r2 );

    DNS_RRSET_TERMINATE( rr1 );
    DNS_RRSET_TERMINATE( rr2 );

    ok( DnsRecordSetCompare( NULL, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsRecordSetCompare( rr1.pFirstRR, NULL, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsRecordSetCompare( NULL, rr2.pFirstRR, NULL, NULL ) == FALSE, "succeeded unexpectedly\n" );

    diff1 = NULL;
    diff2 = NULL;

    ok( DnsRecordSetCompare( NULL, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, NULL, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 != NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );
    DnsRecordListFree( diff1, DnsFreeRecordList );

    ok( DnsRecordSetCompare( NULL, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    ok( diff1 == NULL && diff2 != NULL, "unexpected result: %p, %p\n", diff1, diff2 );
    DnsRecordListFree( diff2, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff2 == NULL, "unexpected result: %p\n", diff2 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL, "unexpected result: %p\n", diff1 );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == TRUE, "failed unexpectedly\n" );
    ok( diff1 == NULL && diff2 == NULL, "unexpected result: %p, %p\n", diff1, diff2 );

    r2.Data.A.IpAddress = 0;

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, NULL, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff2, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, NULL ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff1, DnsFreeRecordList );

    ok( DnsRecordSetCompare( rr1.pFirstRR, rr2.pFirstRR, &diff1, &diff2 ) == FALSE, "succeeded unexpectedly\n" );
    DnsRecordListFree( diff1, DnsFreeRecordList );
    DnsRecordListFree( diff2, DnsFreeRecordList );
}

static void test_DnsRecordSetDetach( void )
{
    DNS_RRSET rr;
    DNS_RECORDA *r, *s;

    DNS_RRSET_INIT( rr );
    DNS_RRSET_ADD( rr, &r1 );
    DNS_RRSET_ADD( rr, &r2 );
    DNS_RRSET_ADD( rr, &r3 );
    DNS_RRSET_TERMINATE( rr );

    ok( !DnsRecordSetDetach( NULL ), "succeeded unexpectedly\n" );

    r = rr.pFirstRR;
    s = DnsRecordSetDetach( r );

    ok( s == &r3, "failed unexpectedly: got %p, expected %p\n", s, &r3 );
    ok( r == &r1, "failed unexpectedly: got %p, expected %p\n", r, &r1 );
    ok( !r2.pNext, "failed unexpectedly\n" );
}

START_TEST(record)
{
    test_DnsRecordCompare();
    test_DnsRecordSetCompare();
    test_DnsRecordSetDetach();
}
