/*
 * test parsing functions
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winldap.h>

#include "wine/test.h"

static void test_ldap_parse_sort_control( LDAP *ld )
{
    ULONG ret, result;
    LDAPSortKeyA *sortkeys[2], key;
    LDAPControlA *sort, *ctrls[2], **server_ctrls;
    LDAPMessage *res = NULL;
    struct l_timeval timeout;

    key.sk_attrtype = (char *)"ou";
    key.sk_matchruleoid = NULL;
    key.sk_reverseorder = FALSE;

    sortkeys[0] = &key;
    sortkeys[1] = NULL;
    ret = ldap_create_sort_controlA( ld, sortkeys, 0, &sort );
    ok( !ret, "ldap_create_sort_controlA failed 0x%x\n", ret );

    ctrls[0] = sort;
    ctrls[1] = NULL;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    ret = ldap_search_ext_sA( ld, (char *)"", LDAP_SCOPE_ONELEVEL, (char *)"(ou=*)", NULL, 0, ctrls, NULL, &timeout, 10, &res );
    if (ret == LDAP_SERVER_DOWN)
    {
        skip("test server can't be reached\n");
        ldap_control_free( sort );
        return;
    }
    ok( !ret, "ldap_search_ext_sA failed 0x%x\n", ret );
    ok( res != NULL, "expected res != NULL\n" );

    ret = ldap_parse_resultA( NULL, res, &result, NULL, NULL, NULL, &server_ctrls, 1 );
    ok( ret == LDAP_PARAM_ERROR, "ldap_parse_resultA failed 0x%x\n", ret );

    result = ~0u;
    ret = ldap_parse_resultA( ld, res, &result, NULL, NULL, NULL, &server_ctrls, 1 );
    ok( !ret, "ldap_parse_resultA failed 0x%x\n", ret );
    ok( !result, "got 0x%x expected 0\n", result );

    ret = ldap_parse_sort_controlA( NULL, NULL, NULL, NULL );
    ok( ret == LDAP_PARAM_ERROR, "ldap_parse_sort_controlA failed 0x%d\n", ret );

    ret = ldap_parse_sort_controlA( ld, NULL, NULL, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed 0x%x\n", ret );

    ret = ldap_parse_sort_controlA( ld, NULL, &result, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed 0x%x\n", ret );

    ret = ldap_parse_sort_controlA( ld, server_ctrls, &result, NULL );
    ok( ret == LDAP_CONTROL_NOT_FOUND, "ldap_parse_sort_controlA failed 0x%x\n", ret );

    ldap_control_free( sort );
    ldap_controls_free( server_ctrls );
}

START_TEST (parse)
{
    LDAP *ld;

    ld = ldap_initA((char *)"ldap.itd.umich.edu", 389 );
    ok( ld != NULL, "ldap_init failed\n" );

    test_ldap_parse_sort_control( ld );
    ldap_unbind( ld );
}
