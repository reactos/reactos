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

#ifndef LDAP_MAXINT
#define LDAP_MAXINT  2147483647
#endif

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_create_page_controlA     (WLDAP32.@)
 *
 * See ldap_create_page_controlW.
 */
ULONG CDECL ldap_create_page_controlA( WLDAP32_LDAP *ld, ULONG pagesize,
    struct WLDAP32_berval *cookie, UCHAR critical, PLDAPControlA *control )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW *controlW = NULL;

    TRACE( "(%p, 0x%08x, %p, 0x%02x, %p)\n", ld, pagesize, cookie,
           critical, control );

    if (!ld || !control || pagesize > LDAP_MAXINT)
        return WLDAP32_LDAP_PARAM_ERROR;

    ret = ldap_create_page_controlW( ld, pagesize, cookie, critical, &controlW );
    if (ret == LDAP_SUCCESS)
    {
        *control = controlWtoA( controlW );
        ldap_control_freeW( controlW );
    }

#endif
    return ret;
}

#ifdef HAVE_LDAP

/* create a page control by hand */
static ULONG create_page_control( ULONG pagesize, struct WLDAP32_berval *cookie,
    UCHAR critical, PLDAPControlW *control )
{
    LDAPControlW *ctrl;
    BerElement *ber;
    ber_tag_t tag;
    struct berval *berval, null_cookie = { 0, NULL };
    INT ret, len;
    char *val;

    ber = ber_alloc_t( LBER_USE_DER );
    if (!ber) return WLDAP32_LDAP_NO_MEMORY;

    if (cookie)
        tag = ber_printf( ber, "{iO}", (ber_int_t)pagesize, cookie );
    else
        tag = ber_printf( ber, "{iO}", (ber_int_t)pagesize, &null_cookie );

    ret = ber_flatten( ber, &berval );
    ber_free( ber, 1 );

    if (tag == LBER_ERROR)
        return WLDAP32_LDAP_ENCODING_ERROR;

    if (ret == -1)
        return WLDAP32_LDAP_NO_MEMORY;

    /* copy the berval so it can be properly freed by the caller */
    val = HeapAlloc( GetProcessHeap(), 0, berval->bv_len );
    if (!val) return WLDAP32_LDAP_NO_MEMORY;

    len = berval->bv_len;
    memcpy( val, berval->bv_val, len );
    ber_bvfree( berval );

    ctrl = HeapAlloc( GetProcessHeap(), 0, sizeof(LDAPControlW) );
    if (!ctrl)
    {
        HeapFree( GetProcessHeap(), 0, val );
        return WLDAP32_LDAP_NO_MEMORY;
    }

    ctrl->ldctl_oid = strAtoW( LDAP_PAGED_RESULT_OID_STRING );
    ctrl->ldctl_value.bv_len = len;
    ctrl->ldctl_value.bv_val = val;
    ctrl->ldctl_iscritical = critical;

    *control = ctrl;

    return LDAP_SUCCESS;
}

#endif /* HAVE_LDAP */

/***********************************************************************
 *      ldap_create_page_controlW     (WLDAP32.@)
 *
 * Create a control for paged search results.
 *
 * PARAMS
 *  ld        [I] Pointer to an LDAP context.
 *  pagesize  [I] Number of entries to return per page.
 *  cookie    [I] Used by the server to track its location in the
 *                search results.
 *  critical  [I] Tells the server this control is critical to the
 *                search operation.
 *  control   [O] LDAPControl created.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_create_page_controlW( WLDAP32_LDAP *ld, ULONG pagesize,
    struct WLDAP32_berval *cookie, UCHAR critical, PLDAPControlW *control )
{
#ifdef HAVE_LDAP
    TRACE( "(%p, 0x%08x, %p, 0x%02x, %p)\n", ld, pagesize, cookie,
           critical, control );

    if (!ld || !control || pagesize > LDAP_MAXINT)
        return WLDAP32_LDAP_PARAM_ERROR;

    return create_page_control( pagesize, cookie, critical, control );

#else
    return LDAP_NOT_SUPPORTED;
#endif
}

ULONG CDECL ldap_get_next_page( WLDAP32_LDAP *ld, PLDAPSearch search, ULONG pagesize,
    ULONG *message )
{
    FIXME( "(%p, %p, 0x%08x, %p)\n", ld, search, pagesize, message );

    if (!ld) return ~0UL;
    return LDAP_NOT_SUPPORTED;
}

ULONG CDECL ldap_get_next_page_s( WLDAP32_LDAP *ld, PLDAPSearch search,
    struct l_timeval *timeout, ULONG pagesize, ULONG *count,
    WLDAP32_LDAPMessage **results )
{
    FIXME( "(%p, %p, %p, 0x%08x, %p, %p)\n", ld, search, timeout,
           pagesize, count, results );

    if (!ld) return ~0UL;
    return LDAP_NOT_SUPPORTED;
}

ULONG CDECL ldap_get_paged_count( WLDAP32_LDAP *ld, PLDAPSearch search,
    ULONG *count, WLDAP32_LDAPMessage *results )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    FIXME( "(%p, %p, %p, %p)\n", ld, search, count, results );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    /* FIXME: save the cookie from the server here */

#endif
    return ret;
}

/***********************************************************************
 *      ldap_parse_page_controlA      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlA( WLDAP32_LDAP *ld, PLDAPControlA *ctrls,
    ULONG *count, struct WLDAP32_berval **cookie )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW **ctrlsW = NULL;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, count, cookie );

    if (!ld || !ctrls || !count || !cookie)
        return WLDAP32_LDAP_PARAM_ERROR;

    ctrlsW = controlarrayAtoW( ctrls );
    if (!ctrlsW) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_parse_page_controlW( ld, ctrlsW, count, cookie );
    controlarrayfreeW( ctrlsW );
 
#endif
    return ret;
}

/***********************************************************************
 *      ldap_parse_page_controlW      (WLDAP32.@)
 */
ULONG CDECL ldap_parse_page_controlW( WLDAP32_LDAP *ld, PLDAPControlW *ctrls,
    ULONG *count, struct WLDAP32_berval **cookie )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW *control = NULL;
    BerElement *ber;
    ber_tag_t tag;
    ULONG i;

    TRACE( "(%p, %p, %p, %p)\n", ld, ctrls, count, cookie );

    if (!ld || !ctrls || !count || !cookie)
        return WLDAP32_LDAP_PARAM_ERROR;

    for (i = 0; ctrls[i]; i++)
    {
        if (!lstrcmpW( LDAP_PAGED_RESULT_OID_STRING_W, ctrls[i]->ldctl_oid ))
            control = ctrls[i];
    }

    if (!control)
        return WLDAP32_LDAP_CONTROL_NOT_FOUND; 
            
    ber = ber_init( &((LDAPControl *)control)->ldctl_value );
    if (!ber)
        return WLDAP32_LDAP_NO_MEMORY;

    tag = ber_scanf( ber, "{iO}", count, cookie );
    if ( tag == LBER_ERROR )
        ret = WLDAP32_LDAP_DECODING_ERROR;
    else
        ret = LDAP_SUCCESS;

    ber_free( ber, 1 );
    
#endif
    return ret;
}

ULONG CDECL ldap_search_abandon_page( WLDAP32_LDAP *ld, PLDAPSearch search )
{
    FIXME( "(%p, %p)\n", ld, search );

    if (!ld) return ~0UL;
    return LDAP_SUCCESS;
}

PLDAPSearch CDECL ldap_search_init_pageA( WLDAP32_LDAP *ld, PCHAR dn, ULONG scope,
    PCHAR filter, PCHAR attrs[], ULONG attrsonly, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, ULONG timelimit, ULONG sizelimit, PLDAPSortKeyA *sortkeys )
{
    FIXME( "(%p, %s, 0x%08x, %s, %p, 0x%08x)\n", ld, debugstr_a(dn),
           scope, debugstr_a(filter), attrs, attrsonly );
    return NULL;
}

PLDAPSearch CDECL ldap_search_init_pageW( WLDAP32_LDAP *ld, PWCHAR dn, ULONG scope,
    PWCHAR filter, PWCHAR attrs[], ULONG attrsonly, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, ULONG timelimit, ULONG sizelimit, PLDAPSortKeyW *sortkeys )
{
    FIXME( "(%p, %s, 0x%08x, %s, %p, 0x%08x)\n", ld, debugstr_w(dn),
           scope, debugstr_w(filter), attrs, attrsonly );
    return NULL;
}
