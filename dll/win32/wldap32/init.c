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

#include <stdio.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#endif

#include "winldap_private.h"
#include "wldap32.h"

#ifdef HAVE_LDAP
/* Should eventually be determined by the algorithm documented on MSDN. */
static const WCHAR defaulthost[] = { 'l','o','c','a','l','h','o','s','t',0 };

/* Split a space separated string of hostnames into a string array */
static char **split_hostnames( const char *hostnames )
{
    char **res, *str, *p, *q;
    unsigned int i = 0;

    str = strdupU( hostnames );
    if (!str) return NULL;

    p = str;
    while (isspace( *p )) p++;
    if (*p) i++;

    while (*p)
    {
        if (isspace( *p ))
        {
            while (isspace( *p )) p++;
            if (*p) i++;
        }
        p++;
    }

    res = HeapAlloc( GetProcessHeap(), 0, (i + 1) * sizeof(char *) );
    if (!res)
    {
        HeapFree( GetProcessHeap(), 0, str );
        return NULL;
    }

    p = str;
    while (isspace( *p )) p++;

    q = p;
    i = 0;
    
    while (*p)
    {
        if (p[1] != '\0')
        {
            if (isspace( *p ))
            {
                *p = '\0'; p++;
                res[i] = strdupU( q );
                if (!res[i]) goto oom;
                i++;
            
                while (isspace( *p )) p++;
                q = p;
            }
        }
        else
        {
            res[i] = strdupU( q );
            if (!res[i]) goto oom;
            i++;
        }
        p++;
    }
    res[i] = NULL;

    HeapFree( GetProcessHeap(), 0, str );
    return res;

oom:
    while (i > 0) strfreeU( res[--i] );

    HeapFree( GetProcessHeap(), 0, res );
    HeapFree( GetProcessHeap(), 0, str );

    return NULL;
}

/* Determine if a URL starts with a known LDAP scheme */
static int has_ldap_scheme( char *url )
{
    if (!strncasecmp( url, "ldap://", 7 ) || 
        !strncasecmp( url, "ldaps://", 8 ) ||
        !strncasecmp( url, "ldapi://", 8 ) ||
        !strncasecmp( url, "cldap://", 8 )) return 1;
    return 0;
}

/* Flatten an array of hostnames into a space separated string of URLs.
 * Prepend a given scheme and append a given portnumber to each hostname
 * if necessary.
 */
static char *join_hostnames( const char *scheme, char **hostnames, ULONG portnumber )
{
    char *res, *p, *q, **v;
    unsigned int i = 0, size = 0; 
    static const char sep[] = " ", fmt[] = ":%d";
    char port[7];

    sprintf( port, fmt, portnumber ); 

    for (v = hostnames; *v; v++)
    {
        if (!has_ldap_scheme( *v ))
        {
            size += strlen( scheme );
            q = *v;
        }
        else
            /* skip past colon in scheme prefix */
            q = strchr( *v, '/' );

        size += strlen( *v );

        if (!strchr( q, ':' )) 
            size += strlen( port );

        i++;
    }

    size += (i - 1) * strlen( sep );
 
    res = HeapAlloc( GetProcessHeap(), 0, size + 1 );
    if (!res) return NULL;

    p = res;
    for (v = hostnames; *v; v++)
    {
        if (v != hostnames)
        {
            strcpy( p, sep );
            p += strlen( sep );
        }

        if (!has_ldap_scheme( *v ))
        {
            strcpy( p, scheme );
            p += strlen( scheme );
            q = *v;
        }
        else
            /* skip past colon in scheme prefix */
            q = strchr( *v, '/' );

        strcpy( p, *v );
        p += strlen( *v );

        if (!strchr( q, ':' ))
        {
            strcpy( p, port );
            p += strlen( port );
        }
    }
    return res;
}

static char *urlify_hostnames( const char *scheme, char *hostnames, ULONG port )
{
    char *url = NULL, **strarray;

    strarray = split_hostnames( hostnames );
    if (strarray)
        url = join_hostnames( scheme, strarray, port );
    else
        return NULL;

    strarrayfreeU( strarray );
    return url;
}
#endif

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      cldap_openA     (WLDAP32.@)
 *
 * See cldap_openW.
 */
WLDAP32_LDAP * CDECL cldap_openA( PCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %d)\n", debugstr_a(hostname), portnumber );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) goto exit;
    }

    ld = cldap_openW( hostnameW, portnumber );

exit:
    strfreeW( hostnameW );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      cldap_openW     (WLDAP32.@)
 *
 * Initialize an LDAP context and create a UDP connection.
 *
 * PARAMS
 *  hostname   [I] Name of the host to connect to.
 *  portnumber [I] Portnumber to use.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 *
 * NOTES
 *  The hostname string can be a space separated string of hostnames,
 *  in which case the LDAP runtime will try to connect to the hosts
 *  in order, until a connection can be made. A hostname may have a
 *  trailing portnumber (separated from the hostname by a ':'), which 
 *  will take precedence over the portnumber supplied as a parameter
 *  to this function.
 */
WLDAP32_LDAP * CDECL cldap_openW( PWCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    LDAP *ld = NULL;
    char *hostnameU = NULL, *url = NULL;

    TRACE( "(%s, %d)\n", debugstr_w(hostname), portnumber );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    url = urlify_hostnames( "cldap://", hostnameU, portnumber );
    if (!url) goto exit;

    ldap_initialize( &ld, url );

exit:
    strfreeU( hostnameU );
    strfreeU( url );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_connect     (WLDAP32.@)
 *
 * Connect to an LDAP server. 
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  timeout [I] Pointer to an l_timeval structure specifying the
 *              timeout in seconds.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The timeout parameter may be NULL in which case a default timeout
 *  value will be used.
 */
ULONG CDECL ldap_connect( WLDAP32_LDAP *ld, struct l_timeval *timeout )
{
    TRACE( "(%p, %p)\n", ld, timeout );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    return WLDAP32_LDAP_SUCCESS; /* FIXME: do something, e.g. ping the host */
}

/***********************************************************************
 *      ldap_initA     (WLDAP32.@)
 *
 * See ldap_initW.
 */
WLDAP32_LDAP *  CDECL ldap_initA( PCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %d)\n", debugstr_a(hostname), portnumber );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) goto exit;
    }

    ld = ldap_initW( hostnameW, portnumber );

exit:
    strfreeW( hostnameW );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_initW     (WLDAP32.@)
 *
 * Initialize an LDAP context and create a TCP connection.
 *
 * PARAMS
 *  hostname   [I] Name of the host to connect to.
 *  portnumber [I] Portnumber to use.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 *
 * NOTES
 *  The hostname string can be a space separated string of hostnames,
 *  in which case the LDAP runtime will try to connect to the hosts
 *  in order, until a connection can be made. A hostname may have a
 *  trailing portnumber (separated from the hostname by a ':'), which 
 *  will take precedence over the portnumber supplied as a parameter
 *  to this function. The connection will not be made until the first
 *  LDAP function that needs it is called.
 */
WLDAP32_LDAP * CDECL ldap_initW( PWCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    LDAP *ld = NULL;
    char *hostnameU = NULL, *url = NULL;

    TRACE( "(%s, %d)\n", debugstr_w(hostname), portnumber );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    url = urlify_hostnames( "ldap://", hostnameU, portnumber );
    if (!url) goto exit;

    ldap_initialize( &ld, url );

exit:
    strfreeU( hostnameU );
    strfreeU( url );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_openA     (WLDAP32.@)
 *
 * See ldap_openW.
 */
WLDAP32_LDAP * CDECL ldap_openA( PCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %d)\n", debugstr_a(hostname), portnumber );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) goto exit;
    }

    ld = ldap_openW( hostnameW, portnumber );

exit:
    strfreeW( hostnameW );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_openW     (WLDAP32.@)
 *
 * Initialize an LDAP context and create a TCP connection.
 *
 * PARAMS
 *  hostname   [I] Name of the host to connect to.
 *  portnumber [I] Portnumber to use.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 *
 * NOTES
 *  The hostname string can be a space separated string of hostnames,
 *  in which case the LDAP runtime will try to connect to the hosts
 *  in order, until a connection can be made. A hostname may have a
 *  trailing portnumber (separated from the hostname by a ':'), which 
 *  will take precedence over the portnumber supplied as a parameter
 *  to this function.
 */
WLDAP32_LDAP * CDECL ldap_openW( PWCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    LDAP *ld = NULL;
    char *hostnameU = NULL, *url = NULL;

    TRACE( "(%s, %d)\n", debugstr_w(hostname), portnumber );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    url = urlify_hostnames( "ldap://", hostnameU, portnumber );
    if (!url) goto exit;

    ldap_initialize( &ld, url );

exit:
    strfreeU( hostnameU );
    strfreeU( url );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_sslinitA     (WLDAP32.@)
 *
 * See ldap_sslinitW.
 */
WLDAP32_LDAP * CDECL ldap_sslinitA( PCHAR hostname, ULONG portnumber, int secure )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %d, 0x%08x)\n", debugstr_a(hostname), portnumber, secure );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) return NULL;
    }

    ld  = ldap_sslinitW( hostnameW, portnumber, secure );

    strfreeW( hostnameW );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_sslinitW     (WLDAP32.@)
 *
 * Initialize an LDAP context and create a secure TCP connection.
 *
 * PARAMS
 *  hostname   [I] Name of the host to connect to.
 *  portnumber [I] Portnumber to use.
 *  secure     [I] Ask the server to create an SSL connection.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 *
 * NOTES
 *  The hostname string can be a space separated string of hostnames,
 *  in which case the LDAP runtime will try to connect to the hosts
 *  in order, until a connection can be made. A hostname may have a
 *  trailing portnumber (separated from the hostname by a ':'), which 
 *  will take precedence over the portnumber supplied as a parameter
 *  to this function. The connection will not be made until the first
 *  LDAP function that needs it is called.
 */
WLDAP32_LDAP * CDECL ldap_sslinitW( PWCHAR hostname, ULONG portnumber, int secure )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    char *hostnameU = NULL, *url = NULL;

    TRACE( "(%s, %d, 0x%08x)\n", debugstr_w(hostname), portnumber, secure );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    if (secure)
        url = urlify_hostnames( "ldaps://", hostnameU, portnumber );
    else
        url = urlify_hostnames( "ldap://", hostnameU, portnumber );

    if (!url) goto exit;
    ldap_initialize( &ld, url );

exit:
    strfreeU( hostnameU );
    strfreeU( url );
    return ld;

#else
    return NULL;
#endif
}

/***********************************************************************
 *      ldap_start_tls_sA     (WLDAP32.@)
 *
 * See ldap_start_tls_sW.
 */
ULONG CDECL ldap_start_tls_sA( WLDAP32_LDAP *ld, PULONG retval, WLDAP32_LDAPMessage **result,
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %p, %p, %p, %p)\n", ld, retval, result, serverctrls, clientctrls );

    if (!ld) return ~0u;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_start_tls_sW( ld, retval, result, serverctrlsW, clientctrlsW );

exit:
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_start_tls_s     (WLDAP32.@)
 *
 * Start TLS encryption on an LDAP connection.
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  retval      [I] Return value from the server.
 *  result      [I] Response message from the server.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  LDAP function that needs it is called.
 */
ULONG CDECL ldap_start_tls_sW( WLDAP32_LDAP *ld, PULONG retval, WLDAP32_LDAPMessage **result,
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %p, %p, %p, %p)\n", ld, retval, result, serverctrls, clientctrls );

    if (!ld) return ~0u;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = map_error( ldap_start_tls_s( ld, serverctrlsU, clientctrlsU ));

exit:
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

/***********************************************************************
 *      ldap_startup     (WLDAP32.@)
 */
ULONG CDECL ldap_startup( PLDAP_VERSION_INFO version, HANDLE *instance )
{
    TRACE( "(%p, %p)\n", version, instance );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_stop_tls_s     (WLDAP32.@)
 *
 * Stop TLS encryption on an LDAP connection.
 *
 * PARAMS
 *  ld [I] Pointer to an LDAP context.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOLEAN CDECL ldap_stop_tls_s( WLDAP32_LDAP *ld )
{
    TRACE( "(%p)\n", ld );
    return TRUE; /* FIXME: find a way to stop tls on a connection */
}
