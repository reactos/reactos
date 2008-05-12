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

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

#ifdef HAVE_LDAP
static LDAPMod *nullmods[] = { NULL };
#endif

/***********************************************************************
 *      ldap_modifyA     (WLDAP32.@)
 *
 * See ldap_modifyW.
 */
ULONG CDECL ldap_modifyA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *mods[] )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;
    
    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), mods );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (mods) {
        modsW = modarrayAtoW( mods );
        if (!modsW) goto exit;
    }

    ret = ldap_modifyW( ld, dnW, modsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( modsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modifyW     (WLDAP32.@)
 *
 * Change an entry in a directory tree (asynchronous operation).
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  dn     [I] DN of the entry to change.
 *  mods   [I] Pointer to an array of LDAPModW structures, each
 *             specifying an attribute and its values to change.
 *
 * RETURNS
 *  Success: Message ID of the modify operation.
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. Cancel the operation by calling ldap_abandon
 *  with the message ID.
 */
ULONG CDECL ldap_modifyW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *mods[] )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **modsU = NULL;
    int msg;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), mods );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (mods) {
        modsU = modarrayWtoU( mods );
        if (!modsU) goto exit;
    }

    ret = ldap_modify_ext( ld, dn ? dnU : "", mods ? modsU : nullmods,
                           NULL, NULL, &msg );

    if (ret == LDAP_SUCCESS)
        ret = msg;
    else
        ret = ~0UL;

exit:
    strfreeU( dnU );
    modarrayfreeU( modsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_extA     (WLDAP32.@)
 *
 * See ldap_modify_extW.
 */
ULONG CDECL ldap_modify_extA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *mods[],
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn), mods,
           serverctrls, clientctrls, message );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (mods) {
        modsW = modarrayAtoW( mods );
        if (!modsW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_modify_extW( ld, dnW, modsW, serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    modarrayfreeW( modsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_extW     (WLDAP32.@)
 *
 * Change an entry in a directory tree (asynchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to change.
 *  mods        [I] Pointer to an array of LDAPModW structures, each
 *                  specifying an attribute and its values to change.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the modify operation.
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
ULONG CDECL ldap_modify_extW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *mods[],
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **modsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    int dummy;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn), mods,
           serverctrls, clientctrls, message );

    if (!ld) return ~0UL;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (mods) {
        modsU = modarrayWtoU( mods );
        if (!modsU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_modify_ext( ld, dn ? dnU : "", mods ? modsU : nullmods, serverctrlsU,
                           clientctrlsU, message ? (int *)message : &dummy );

exit:
    strfreeU( dnU );
    modarrayfreeU( modsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_ext_sA     (WLDAP32.@)
 *
 * See ldap_modify_ext_sW.
 */
ULONG CDECL ldap_modify_ext_sA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *mods[],
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_a(dn), mods,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (mods) {
        modsW = modarrayAtoW( mods );
        if (!modsW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_modify_ext_sW( ld, dnW, modsW, serverctrlsW, clientctrlsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( modsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_ext_sW     (WLDAP32.@)
 *
 * Change an entry in a directory tree (synchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to change.
 *  mods        [I] Pointer to an array of LDAPModW structures, each
 *                  specifying an attribute and its values to change.
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
ULONG CDECL ldap_modify_ext_sW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *mods[],
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **modsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_w(dn), mods,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (mods) {
        modsU = modarrayWtoU( mods );
        if (!modsU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_modify_ext_s( ld, dn ? dnU : "", mods ? modsU : nullmods,
                             serverctrlsU, clientctrlsU );

exit:
    strfreeU( dnU );
    modarrayfreeU( modsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_sA     (WLDAP32.@)
 *
 * See ldap_modify_sW.
 */
ULONG CDECL ldap_modify_sA( WLDAP32_LDAP *ld, PCHAR dn, LDAPModA *mods[] )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), mods );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (mods) {
        modsW = modarrayAtoW( mods );
        if (!modsW) goto exit;
    }

    ret = ldap_modify_sW( ld, dnW, modsW );

exit:
    strfreeW( dnW );
    modarrayfreeW( modsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_modify_sW     (WLDAP32.@)
 *
 * Change an entry in a directory tree (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of the entry to change.
 *  attrs   [I] Pointer to an array of LDAPModW structures, each
 *              specifying an attribute and its values to change.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_modify_sW( WLDAP32_LDAP *ld, PWCHAR dn, LDAPModW *mods[] )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL;
    LDAPMod **modsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), mods );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (mods) {
        modsU = modarrayWtoU( mods );
        if (!modsU) goto exit;
    }

    ret = ldap_modify_ext_s( ld, dn ? dnU : "", mods ? modsU : nullmods, NULL, NULL );

exit:
    strfreeU( dnU );
    modarrayfreeU( modsU );

#endif
    return ret;
}
