/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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

#include "config.h"

#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_SUCCESS        0x00
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_count_values_len     (WLDAP32.@)
 *
 * Count the number of values in an array of berval structures.
 *
 * PARAMS
 *  vals  [I] Pointer to an array of berval structures.
 *
 * RETURNS
 *  Success: The number of values counted.
 *  Failure: 0
 *
 * NOTES
 *  Call ldap_count_values_len with the result of a call to
 *  ldap_get_values_len.
 */
ULONG CDECL WLDAP32_ldap_count_values_len( struct WLDAP32_berval **vals )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", vals );
    ret = ldap_count_values_len( (struct berval **)vals );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_count_valuesA     (WLDAP32.@)
 *
 * See ldap_count_valuesW.
 */
ULONG CDECL ldap_count_valuesA( PCHAR *vals )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR **valsW = NULL;

    TRACE( "(%p)\n", vals );

    if (!vals) return 0;

    valsW = strarrayAtoW( vals );
    if (!valsW) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_count_valuesW( valsW );
    strarrayfreeW( valsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_count_valuesW     (WLDAP32.@)
 *
 * Count the number of values in a string array.
 *
 * PARAMS
 *  vals  [I] Pointer to an array of strings.
 *
 * RETURNS
 *  Success: The number of values counted.
 *  Failure: 0
 *
 * NOTES
 *  Call ldap_count_valuesW with the result of a call to
 *  ldap_get_valuesW.
 */
ULONG CDECL ldap_count_valuesW( PWCHAR *vals )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR **p = vals;

    TRACE( "(%p)\n", vals );

    if (!vals) return 0;

    ret = 0;
    while (*p++) ret++;

#endif
    return ret;
}

/***********************************************************************
 *      ldap_get_valuesA     (WLDAP32.@)
 *
 * See ldap_get_valuesW.
 */
PCHAR * CDECL ldap_get_valuesA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, PCHAR attr )
{
    PCHAR *ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *attrW = NULL, **retW;

    TRACE( "(%p, %p, %s)\n", ld, entry, debugstr_a(attr) );

    if (!ld || !entry || !attr) return NULL;

    attrW = strAtoW( attr );
    if (!attrW) return NULL;

    retW = ldap_get_valuesW( ld, entry, attrW );

    ret = strarrayWtoA( retW );
    ldap_value_freeW( retW );
    strfreeW( attrW );

#endif
    return ret;
}

#ifdef HAVE_LDAP
static char *bv2str( struct berval *bv )
{
    char *str = NULL;
    unsigned int len = bv->bv_len;

    str = HeapAlloc( GetProcessHeap(), 0, len + 1 );
    if (str)
    {
        memcpy( str, bv->bv_val, len );
        str[len] = '\0';
    }
    return str;
}

static char **bv2str_array( struct berval **bv )
{
    unsigned int len = 0, i = 0;
    struct berval **p = bv;
    char **str;

    while (*p)
    {
        len++;
        p++;
    }
    str = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(char *) );
    if (!str) return NULL;

    p = bv;
    while (*p)
    {
        str[i] = bv2str( *p );
        if (!str[i])
        {
            for (--i; i >= 0; i--)
                HeapFree( GetProcessHeap(), 0, str[i] );

            HeapFree( GetProcessHeap(), 0, str );
            return NULL;
        } 
        i++;
        p++; 
    }
    str[i] = NULL;
    return str;
}
#endif

/***********************************************************************
 *      ldap_get_valuesW     (WLDAP32.@)
 *
 * Retrieve string values for a given attribute.
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  entry  [I] Entry to retrieve values from.
 *  attr   [I] Attribute to retrieve values for.
 *
 * RETURNS
 *  Success: Pointer to a character array holding the values.
 *  Failure: NULL
 *
 * NOTES
 *  Call ldap_get_valuesW with the result of a call to
 *  ldap_first_entry or ldap_next_entry. Free the returned
 *  array with a call to ldap_value_freeW.
 */
PWCHAR * CDECL ldap_get_valuesW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, PWCHAR attr )
{
    PWCHAR *ret = NULL;
#ifdef HAVE_LDAP
    char *attrU = NULL, **retU;
    struct berval **bv;

    TRACE( "(%p, %p, %s)\n", ld, entry, debugstr_w(attr) );

    if (!ld || !entry || !attr) return NULL;

    attrU = strWtoU( attr );
    if (!attrU) return NULL;

    bv = ldap_get_values_len( ld, entry, attrU );

    retU = bv2str_array( bv );
    ret = strarrayUtoW( retU );

    ldap_value_free_len( bv );
    strarrayfreeU( retU );
    strfreeU( attrU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_get_values_lenA     (WLDAP32.@)
 *
 * See ldap_get_values_lenW.
 */
struct WLDAP32_berval ** CDECL ldap_get_values_lenA( WLDAP32_LDAP *ld,
    WLDAP32_LDAPMessage *message, PCHAR attr )
{
#ifdef HAVE_LDAP
    WCHAR *attrW = NULL;
    struct WLDAP32_berval **ret;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_a(attr) );

    if (!ld || !message || !attr) return NULL;

    attrW = strAtoW( attr );
    if (!attrW) return NULL;

    ret = ldap_get_values_lenW( ld, message, attrW );

    strfreeW( attrW );
    return ret;

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_get_values_lenW     (WLDAP32.@)
 *
 * Retrieve binary values for a given attribute.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  message [I] Entry to retrieve values from.
 *  attr    [I] Attribute to retrieve values for.
 *
 * RETURNS
 *  Success: Pointer to a berval array holding the values.
 *  Failure: NULL
 *
 * NOTES
 *  Call ldap_get_values_lenW with the result of a call to
 *  ldap_first_entry or ldap_next_entry. Free the returned
 *  array with a call to ldap_value_free_len.
 */
struct WLDAP32_berval ** CDECL ldap_get_values_lenW( WLDAP32_LDAP *ld,
    WLDAP32_LDAPMessage *message, PWCHAR attr )
{
#ifdef HAVE_LDAP
    char *attrU = NULL;
    struct berval **ret;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_w(attr) );

    if (!ld || !message || !attr) return NULL;

    attrU = strWtoU( attr );
    if (!attrU) return NULL;

    ret = ldap_get_values_len( ld, message, attrU );

    strfreeU( attrU );
    return (struct WLDAP32_berval **)ret;

#endif
    return NULL;
}

/***********************************************************************
 *      ldap_value_free_len     (WLDAP32.@)
 *
 * Free an array of berval structures.
 *
 * PARAMS
 *  vals  [I] Array of berval structures.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL WLDAP32_ldap_value_free_len( struct WLDAP32_berval **vals )
{
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", vals );
    ldap_value_free_len( (struct berval **)vals );

#endif
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeA     (WLDAP32.@)
 *
 * See ldap_value_freeW.
 */
ULONG CDECL ldap_value_freeA( PCHAR *vals )
{
    TRACE( "(%p)\n", vals );

    strarrayfreeA( vals );
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeW     (WLDAP32.@)
 *
 * Free an array of string values.
 *
 * PARAMS
 *  vals  [I] Array of string values.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_value_freeW( PWCHAR *vals )
{
    TRACE( "(%p)\n", vals );

    strarrayfreeW( vals );
    return LDAP_SUCCESS;
}
