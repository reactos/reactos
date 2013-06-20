/*
 * Tests for name handling functions
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"

#include "wine/test.h"

static const struct
{
    LPCSTR name;
    DNS_NAME_FORMAT format;
    DNS_STATUS status;
}
test_data[] =
{
    { "", DnsNameDomain, ERROR_INVALID_NAME },
    { ".", DnsNameDomain, ERROR_SUCCESS },
    { "..", DnsNameDomain, ERROR_INVALID_NAME },
    { ".a", DnsNameDomain, ERROR_INVALID_NAME },
    { "a.", DnsNameDomain, ERROR_SUCCESS },
    { "a..", DnsNameDomain, ERROR_INVALID_NAME },
    { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", DnsNameDomain, ERROR_INVALID_NAME },
    { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", DnsNameDomain, ERROR_INVALID_NAME },
    { "a.?", DnsNameDomain, DNS_ERROR_INVALID_NAME_CHAR },
    { "a.*", DnsNameDomain, DNS_ERROR_INVALID_NAME_CHAR },
    { "a ", DnsNameDomain, DNS_ERROR_INVALID_NAME_CHAR },
    { "a._b", DnsNameDomain, DNS_ERROR_NON_RFC_NAME },
    { "123", DnsNameDomain, DNS_ERROR_NUMERIC_NAME },
    { "123.456", DnsNameDomain, DNS_ERROR_NUMERIC_NAME },
    { "a.b", DnsNameDomain, ERROR_SUCCESS },

    { "", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { ".", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "..", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { ".c", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "c.", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "c..", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "?", DnsNameDomainLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "*", DnsNameDomainLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "c ", DnsNameDomainLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "_c", DnsNameDomainLabel, DNS_ERROR_NON_RFC_NAME },
    { "456", DnsNameDomainLabel, ERROR_SUCCESS },
    { "456.789", DnsNameDomainLabel, ERROR_INVALID_NAME },
    { "c.d", DnsNameDomainLabel, ERROR_INVALID_NAME },

    { "", DnsNameHostnameFull, ERROR_INVALID_NAME },
    { ".", DnsNameHostnameFull, ERROR_SUCCESS },
    { "..", DnsNameHostnameFull, ERROR_INVALID_NAME },
    { ".e", DnsNameHostnameFull, ERROR_INVALID_NAME },
    { "e.", DnsNameHostnameFull, ERROR_SUCCESS },
    { "e..", DnsNameHostnameFull, ERROR_INVALID_NAME },
    { "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", DnsNameDomain, ERROR_INVALID_NAME },
    { "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee.eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee.eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee.eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", DnsNameHostnameFull, ERROR_INVALID_NAME },
    { "?", DnsNameHostnameLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "e.?", DnsNameHostnameFull, DNS_ERROR_INVALID_NAME_CHAR },
    { "e.*", DnsNameHostnameFull, DNS_ERROR_INVALID_NAME_CHAR },
    { "e ", DnsNameHostnameFull, DNS_ERROR_INVALID_NAME_CHAR },
    { "e._f", DnsNameHostnameFull, DNS_ERROR_NON_RFC_NAME },
    { "789", DnsNameHostnameFull, DNS_ERROR_NUMERIC_NAME },
    { "789.456", DnsNameHostnameFull, DNS_ERROR_NUMERIC_NAME },
    { "e.f", DnsNameHostnameFull, ERROR_SUCCESS },

    { "", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { ".", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "..", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { ".g", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "g.", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "g..", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "*", DnsNameHostnameLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "g ", DnsNameHostnameLabel, DNS_ERROR_INVALID_NAME_CHAR },
    { "_g", DnsNameHostnameLabel, DNS_ERROR_NON_RFC_NAME },
    { "123", DnsNameHostnameLabel, DNS_ERROR_NUMERIC_NAME },
    { "123.456", DnsNameHostnameLabel, ERROR_INVALID_NAME },
    { "g.h", DnsNameHostnameLabel, ERROR_INVALID_NAME },

    { "", DnsNameWildcard, ERROR_INVALID_NAME },
    { ".", DnsNameWildcard, ERROR_INVALID_NAME },
    { "..", DnsNameWildcard, ERROR_INVALID_NAME },
    { ".j", DnsNameWildcard, ERROR_INVALID_NAME },
    { "j.", DnsNameWildcard, ERROR_INVALID_NAME },
    { "j..", DnsNameWildcard, ERROR_INVALID_NAME },
    { "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj", DnsNameWildcard, ERROR_INVALID_NAME },
    { "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj.jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj.jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj.jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj", DnsNameWildcard, ERROR_INVALID_NAME },
    { "?", DnsNameWildcard, ERROR_INVALID_NAME },
    { "i ", DnsNameWildcard, ERROR_INVALID_NAME },
    { "_i", DnsNameWildcard, ERROR_INVALID_NAME },
    { "123", DnsNameWildcard, ERROR_INVALID_NAME },
    { "123.456", DnsNameWildcard, ERROR_INVALID_NAME },
    { "i.j", DnsNameWildcard, ERROR_INVALID_NAME },
    { "*", DnsNameWildcard, ERROR_SUCCESS },
    { "*j", DnsNameWildcard, DNS_ERROR_INVALID_NAME_CHAR },
    { "*.j", DnsNameWildcard, ERROR_SUCCESS },
    { "i.*", DnsNameWildcard, ERROR_INVALID_NAME },

    { "", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { ".", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "..", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { ".k", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "k.", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "k..", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk.kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk.kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk.kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "?", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "k ", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "_k", DnsNameSrvRecord, ERROR_SUCCESS },
    { "123", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "123.456", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "k.l", DnsNameSrvRecord, ERROR_INVALID_NAME },
    { "_", DnsNameSrvRecord, DNS_ERROR_NON_RFC_NAME },
    { "_k.l", DnsNameSrvRecord, ERROR_SUCCESS },
    { "k._l", DnsNameSrvRecord, ERROR_INVALID_NAME }
};

static void test_DnsValidateName_A( void )
{
    unsigned int i;
    DNS_STATUS status;

    status = DnsValidateName_A( NULL, DnsNameDomain );
    ok( status == ERROR_INVALID_NAME, "succeeded unexpectedly\n" );

    for (i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        status = DnsValidateName_A( test_data[i].name, test_data[i].format );
        ok( status == test_data[i].status, "%d: \'%s\': got %d, expected %d\n",
            i, test_data[i].name, status, test_data[i].status );
    }
}

static void test_DnsNameCompare_A( void )
{
    static CHAR empty[]          = "",
                dot[]            = ".",
                dotdot[]         = "..",
                A[]              = "A",
                a[]              = "a",
                B[]              = "B",
                b[]              = "b",
                A_dot_B[]        = "A.B",
                a_dot_a[]        = "a.a",
                a_dot_b[]        = "a.b",
                a_dot_b_dot[]    = "a.b.",
                a_dot_b_dotdot[] = "a.b..",
                B_dot_A[]        = "B.A",
                b_dot_a[]        = "b.a",
                b_dot_a_dot[]    = "b.a.",
                b_dot_a_dotdot[] = "b.a..";

    ok( DnsNameCompare_A( NULL, NULL ) == TRUE, "failed unexpectedly\n" );

    ok( DnsNameCompare_A( empty, empty ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( dot, empty ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( empty, dot ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( dot, dotdot ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( dotdot, dot ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a, a ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a, A ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( A, a ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b, A_dot_B ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b, a_dot_b ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dot, a_dot_b_dot ) == TRUE, "failed unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dotdot, a_dot_b_dotdot ) == TRUE, "failed unexpectedly\n" );

    ok( DnsNameCompare_A( empty, NULL ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( NULL, empty ) == FALSE, "succeeded unexpectedly\n" );

    ok( DnsNameCompare_A( a, b ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a, B ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( A, b ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b, B_dot_A ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dot, b_dot_a_dot ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b, a_dot_a ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dotdot, b_dot_a_dotdot ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dot, b_dot_a_dotdot ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dotdot, b_dot_a_dot ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b_dot, b_dot_a ) == FALSE, "succeeded unexpectedly\n" );
    ok( DnsNameCompare_A( a_dot_b, b_dot_a_dot ) == FALSE, "succeeded unexpectedly\n" );
}

START_TEST(name)
{
    test_DnsValidateName_A();
    test_DnsNameCompare_A();
}
