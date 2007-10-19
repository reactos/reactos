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
#endif

#ifndef LDAP_NOT_SUPPORTED
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

#ifdef HAVE_LDAP
static LDAPMod *nullattrs[] = { NULL };
#endif

/***********************************************************************
 *      ldap_addA     (WLDAP32.@)
 *
 * See ldap_addW.
 */
ULONG CDECL ldap_addA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *attrs[] )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), attrs );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attrs) {
        attrsW = modarrayAtoW( attrs );
        if (!attrsW) goto exit;
    }

    ret = ldap_addW( ld, dnW, attrsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( attrsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_addW     (WLDAP32.@)
 *
 * Add an entry to a directory tree (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to add.
 *  attrs   [I] Pointer to an array of LDAPModW structures, each
 *              specifying an attribute and its values to add.
 *
 * RETURNS
 *  Success: Message ID of the add operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. Cancel the operation by calling ldap_abandon
 *  with the message ID.
 */
ULONG CDECL ldap_addW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *attrs[] )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;
    int msg;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), attrs );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attrs) {
        attrsU = modarrayWtoU( attrs );
        if (!attrsU) goto exit;
    }

    ret = ldap_add_ext( ld, dn ? dnU : "", attrs ? attrsU : nullattrs, NULL, NULL, &msg );

    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0UL;

exit:
    strfreeU( dnU );
    modarrayfreeU( attrsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_extA     (WLDAP32.@)
 *
 * See ldap_add_extW.
 */
ULONG CDECL ldap_add_extA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *attrs[],
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn), attrs,
           serverctrls, clientctrls, message );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attrs) {
        attrsW = modarrayAtoW( attrs );
        if (!attrsW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_add_extW( ld, dnW, attrsW, serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    modarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_extW     (WLDAP32.@)
 *
 * Add an entry to a directory tree (asynchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to add.
 *  attrs       [I] Pointer to an array of LDAPModW structures, each
 *                  specifying an attribute and its values to add.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the add operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. The serverctrls and clientctrls parameters are
 *  optional and should be set to NULL if not used.
 */
ULONG CDECL ldap_add_extW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *attrs[],
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    int dummy;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn), attrs,
           serverctrls, clientctrls, message );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attrs) {
        attrsU = modarrayWtoU( attrs );
        if (!attrsU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_add_ext( ld, dn ? dnU : "", attrs ? attrsU : nullattrs, serverctrlsU,
                        clientctrlsU, message ? (int *)message : &dummy );

exit:
    strfreeU( dnU );
    modarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_ext_sA     (WLDAP32.@)
 *
 * See ldap_add_ext_sW.
 */
ULONG CDECL ldap_add_ext_sA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *attrs[],
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_a(dn), attrs,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attrs) {
        attrsW = modarrayAtoW( attrs );
        if (!attrsW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_add_ext_sW( ld, dnW, attrsW, serverctrlsW, clientctrlsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_ext_sW     (WLDAP32.@)
 *
 * Add an entry to a directory tree (synchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to add.
 *  attrs       [I] Pointer to an array of LDAPModW structures, each
 *                  specifying an attribute and its values to add.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The serverctrls and clientctrls parameters are optional and
 *  should be set to NULL if not used.
 */
ULONG CDECL ldap_add_ext_sW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *attrs[],
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_w(dn), attrs,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attrs) {
        attrsU = modarrayWtoU( attrs );
        if (!attrsU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_add_ext_s( ld, dn ? dnU : "", attrs ? attrsU : nullattrs,
                          serverctrlsU, clientctrlsU );

exit:
    strfreeU( dnU );
    modarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_sA     (WLDAP32.@)
 *
 * See ldap_add_sW.
 */
ULONG CDECL ldap_add_sA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *attrs[] )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), attrs );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attrs) {
        attrsW = modarrayAtoW( attrs );
        if (!attrsW) goto exit;
    }

    ret = ldap_add_sW( ld, dnW, attrsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( attrsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_add_sW     (WLDAP32.@)
 *
 * Add an entry to a directory tree (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to add.
 *  attrs   [I] Pointer to an array of LDAPModW structures, each
 *              specifying an attribute and its values to add.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_add_sW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *attrs[] )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), attrs );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attrs) {
        attrsU = modarrayWtoU( attrs );
        if (!attrsU) goto exit;
    }

    ret = ldap_add_ext_s( ld, dn ? dnU : "", attrs ? attrsU : nullattrs, NULL, NULL );

exit:
    strfreeU( dnU );
    modarrayfreeU( attrsU );

#endif
    return ret;
}
