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
#include "winuser.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#endif

#include "winldap_private.h"
#include "wldap32.h"

extern HINSTANCE hwldap32;

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

ULONG map_error( int error )
{
    switch (error)
    {
#ifdef HAVE_LDAP_H
    case LDAP_SERVER_DOWN:              return WLDAP32_LDAP_SERVER_DOWN;
    case LDAP_LOCAL_ERROR:              return WLDAP32_LDAP_LOCAL_ERROR;
    case LDAP_DECODING_ERROR:           return WLDAP32_LDAP_DECODING_ERROR;
    case LDAP_TIMEOUT:                  return WLDAP32_LDAP_TIMEOUT;
    case LDAP_AUTH_UNKNOWN:             return WLDAP32_LDAP_AUTH_UNKNOWN;
    case LDAP_FILTER_ERROR:             return WLDAP32_LDAP_FILTER_ERROR;
    case LDAP_USER_CANCELLED:           return WLDAP32_LDAP_USER_CANCELLED;
    case LDAP_PARAM_ERROR:              return WLDAP32_LDAP_PARAM_ERROR;
    case LDAP_NO_MEMORY:                return WLDAP32_LDAP_NO_MEMORY;
    case LDAP_CONNECT_ERROR:            return WLDAP32_LDAP_CONNECT_ERROR;
    case LDAP_NOT_SUPPORTED:            return WLDAP32_LDAP_NOT_SUPPORTED;
    case LDAP_CONTROL_NOT_FOUND:        return WLDAP32_LDAP_CONTROL_NOT_FOUND;
    case LDAP_NO_RESULTS_RETURNED:      return WLDAP32_LDAP_NO_RESULTS_RETURNED;
    case LDAP_MORE_RESULTS_TO_RETURN:   return WLDAP32_LDAP_MORE_RESULTS_TO_RETURN;
    case LDAP_CLIENT_LOOP:              return WLDAP32_LDAP_CLIENT_LOOP;
    case LDAP_REFERRAL_LIMIT_EXCEEDED:  return WLDAP32_LDAP_REFERRAL_LIMIT_EXCEEDED;
#endif
    default: return error;
    }
}

/***********************************************************************
 *      ldap_err2stringA     (WLDAP32.@)
 *
 * See ldap_err2stringW.
 */
PCHAR CDECL ldap_err2stringA( ULONG err )
{
    static char buf[256] = "";

    TRACE( "(0x%08x)\n", err );

    if (err <= WLDAP32_LDAP_REFERRAL_LIMIT_EXCEEDED)
        LoadStringA( hwldap32, err, buf, 256 );
    else
        LoadStringA( hwldap32, WLDAP32_LDAP_LOCAL_ERROR, buf, 256 );

    return buf;
}

/***********************************************************************
 *      ldap_err2stringW     (WLDAP32.@)
 *
 * Convert an error code into a string describing the error.
 *
 * PARAMS
 *  err  [I] Error code to convert.
 *
 * RETURNS
 *  Success: Pointer to a string containing the error description.
 *  Failure: NULL
 *
 * NOTES
 *  The returned string is statically allocated, you must not
 *  free this string.
 */
PWCHAR CDECL ldap_err2stringW( ULONG err )
{
    static WCHAR buf[256] = { 0 };

    TRACE( "(0x%08x)\n", err );

    if (err <= WLDAP32_LDAP_REFERRAL_LIMIT_EXCEEDED)
        LoadStringW( hwldap32, err, buf, 256 );
    else
        LoadStringW( hwldap32, WLDAP32_LDAP_LOCAL_ERROR, buf, 256 );

    return buf;
}

/***********************************************************************
 *      ldap_perror     (WLDAP32.@)
 *
 * Print a given error string.
 *
 * PARAMS
 *  ld   [I] Pointer to an LDAP context.
 *  msg  [I] Error string.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Like native, this function does nothing.
 */
void CDECL WLDAP32_ldap_perror( WLDAP32_LDAP *ld, const PCHAR msg )
{
    TRACE( "(%p, %s)\n", ld, debugstr_a(msg) );
}

/***********************************************************************
 *      ldap_result2error     (WLDAP32.@)
 *
 * Parse an LDAP message and return the error obtained from it.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  res   [I] Pointer to an LDAPMessage structure.
 *  free  [I] Ask for the LDAPMessage structure to be freed.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  If not asked for, use ldap_msgfree to free the LDAPMessage.
 */
ULONG CDECL WLDAP32_ldap_result2error( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res, ULONG free )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    int error;

    TRACE( "(%p, %p, 0x%08x)\n", ld, res, free );

    if (!ld || !res) return ~0u;

    ret = ldap_parse_result( ld, res, &error, NULL, NULL, NULL, NULL, free );

    if (ret == LDAP_SUCCESS)
        ret = error;
    else
        ret = ~0u;

#endif
    return ret;
}

/***********************************************************************
 *      LdapGetLastError     (WLDAP32.@)
 *
 * Return the last error set by an LDAP function call.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  An LDAP error code.
 */
ULONG CDECL LdapGetLastError( void )
{
    TRACE( "\n" );
    return GetLastError();
}

static const ULONG WLDAP32_errormap[] = {
    /* LDAP_SUCCESS */                      ERROR_SUCCESS,
    /* LDAP_OPERATIONS_ERROR */             ERROR_OPEN_FAILED,
    /* LDAP_PROTOCOL_ERROR */               ERROR_INVALID_LEVEL,
    /* LDAP_TIMELIMIT_EXCEEDED */           ERROR_TIMEOUT,
    /* LDAP_SIZELIMIT_EXCEEDED */           ERROR_MORE_DATA,
    /* LDAP_COMPARE_FALSE */                ERROR_DS_GENERIC_ERROR,
    /* LDAP_COMPARE_TRUE */                 ERROR_DS_GENERIC_ERROR,
    /* LDAP_AUTH_METHOD_NOT_SUPPORTED */    ERROR_ACCESS_DENIED,
    /* LDAP_STRONG_AUTH_REQUIRED */         ERROR_ACCESS_DENIED,
    /* LDAP_REFERRAL_V2 */                  ERROR_MORE_DATA,
    /* LDAP_REFERRAL */                     ERROR_MORE_DATA,
    /* LDAP_ADMIN_LIMIT_EXCEEDED */         ERROR_NOT_ENOUGH_QUOTA,
    /* LDAP_UNAVAILABLE_CRIT_EXTENSION */   ERROR_CAN_NOT_COMPLETE,
    /* LDAP_CONFIDENTIALITY_REQUIRED */     ERROR_DS_GENERIC_ERROR,
    /* LDAP_SASL_BIND_IN_PROGRESS */        ERROR_DS_GENERIC_ERROR,
    /* 0x0f */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_NO_SUCH_ATTRIBUTE */            ERROR_INVALID_PARAMETER,
    /* LDAP_UNDEFINED_TYPE */               ERROR_DS_GENERIC_ERROR,
    /* LDAP_INAPPROPRIATE_MATCHING */       ERROR_INVALID_PARAMETER,
    /* LDAP_CONSTRAINT_VIOLATION */         ERROR_INVALID_PARAMETER,
    /* LDAP_ATTRIBUTE_OR_VALUE_EXISTS */    ERROR_ALREADY_EXISTS,
    /* LDAP_INVALID_SYNTAX */               ERROR_INVALID_NAME,
    /* 0x16 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x17 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x18 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x19 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1a */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1b */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1c */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1d */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1e */                              ERROR_DS_GENERIC_ERROR,
    /* 0x1f */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_NO_SUCH_OBJECT */               ERROR_FILE_NOT_FOUND,
    /* LDAP_ALIAS_PROBLEM */                ERROR_DS_GENERIC_ERROR,
    /* LDAP_INVALID_DN_SYNTAX */            ERROR_INVALID_PARAMETER,
    /* LDAP_IS_LEAF */                      ERROR_DS_GENERIC_ERROR,
    /* LDAP_ALIAS_DEREF_PROBLEM */          ERROR_DS_GENERIC_ERROR,
    /* 0x25 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x26 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x27 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x28 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x29 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2a */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2b */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2c */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2d */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2e */                              ERROR_DS_GENERIC_ERROR,
    /* 0x2f */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_INAPPROPRIATE_AUTH */           ERROR_ACCESS_DENIED,
    /* LDAP_INVALID_CREDENTIALS */          ERROR_WRONG_PASSWORD,
    /* LDAP_INSUFFICIENT_RIGHTS */          ERROR_ACCESS_DENIED,
    /* LDAP_BUSY */                         ERROR_BUSY,
    /* LDAP_UNAVAILABLE */                  ERROR_DEV_NOT_EXIST,
    /* LDAP_UNWILLING_TO_PERFORM */         ERROR_CAN_NOT_COMPLETE,
    /* LDAP_LOOP_DETECT */                  ERROR_DS_GENERIC_ERROR,
    /* 0x37 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x38 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x39 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x3a */                              ERROR_DS_GENERIC_ERROR,
    /* 0x3b */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_SORT_CONTROL_MISSING */         8261,
    /* LDAP_OFFSET_RANGE_ERROR */           8262,
    /* 0x3e */                              ERROR_DS_GENERIC_ERROR,
    /* 0x3f */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_NAMING_VIOLATION */             ERROR_INVALID_PARAMETER,
    /* LDAP_OBJECT_CLASS_VIOLATION */       ERROR_INVALID_PARAMETER,
    /* LDAP_NOT_ALLOWED_ON_NONLEAF */       ERROR_CAN_NOT_COMPLETE,
    /* LDAP_NOT_ALLOWED_ON_RDN */           ERROR_ACCESS_DENIED,
    /* LDAP_ALREADY_EXISTS */               ERROR_ALREADY_EXISTS,
    /* LDAP_NO_OBJECT_CLASS_MODS */         ERROR_ACCESS_DENIED,
    /* LDAP_RESULTS_TOO_LARGE */            ERROR_INSUFFICIENT_BUFFER,
    /* LDAP_AFFECTS_MULTIPLE_DSAS */        ERROR_CAN_NOT_COMPLETE,
    /* 0x48 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x49 */                              ERROR_DS_GENERIC_ERROR,
    /* 0x4a */                              ERROR_DS_GENERIC_ERROR,
    /* 0x4b */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_VIRTUAL_LIST_VIEW_ERROR */      ERROR_DS_GENERIC_ERROR,
    /* 0x4d */                              ERROR_DS_GENERIC_ERROR,
    /* 0x4e */                              ERROR_DS_GENERIC_ERROR,
    /* 0x4f */                              ERROR_DS_GENERIC_ERROR,
    /* LDAP_OTHER */                        ERROR_DS_GENERIC_ERROR,
    /* LDAP_SERVER_DOWN */                  ERROR_BAD_NET_RESP,
    /* LDAP_LOCAL_ERROR */                  ERROR_DS_GENERIC_ERROR,
    /* LDAP_ENCODING_ERROR */               ERROR_UNEXP_NET_ERR,
    /* LDAP_DECODING_ERROR */               ERROR_UNEXP_NET_ERR,
    /* LDAP_TIMEOUT */                      ERROR_SERVICE_REQUEST_TIMEOUT,
    /* LDAP_AUTH_UNKNOWN */                 ERROR_WRONG_PASSWORD,
    /* LDAP_FILTER_ERROR */                 ERROR_INVALID_PARAMETER,
    /* LDAP_USER_CANCELLED */               ERROR_CANCELLED,
    /* LDAP_PARAM_ERROR */                  ERROR_INVALID_PARAMETER,
    /* LDAP_NO_MEMORY */                    ERROR_NOT_ENOUGH_MEMORY,
    /* LDAP_CONNECT_ERROR */                ERROR_CONNECTION_REFUSED,
    /* LDAP_NOT_SUPPORTED */                ERROR_CAN_NOT_COMPLETE,
    /* LDAP_CONTROL_NOT_FOUND */            ERROR_NOT_FOUND,
    /* LDAP_NO_RESULTS_RETURNED */          ERROR_MORE_DATA,
    /* LDAP_MORE_RESULTS_TO_RETURN */       ERROR_MORE_DATA,
    /* LDAP_CLIENT_LOOP */                  ERROR_DS_GENERIC_ERROR,
    /* LDAP_REFERRAL_LIMIT_EXCEEDED */      ERROR_DS_GENERIC_ERROR
};

/***********************************************************************
 *      LdapMapErrorToWin32     (WLDAP32.@)
 *
 * Map an LDAP error code to a Win32 error code.
 *
 * PARAMS
 *  err  [I] An LDAP error code.
 *
 * RETURNS
 *  A Win32 error code.
 */
ULONG CDECL LdapMapErrorToWin32( ULONG err )
{
    TRACE( "(0x%08x)\n", err );

    if (err >= sizeof(WLDAP32_errormap)/sizeof(WLDAP32_errormap[0]))
        return ERROR_DS_GENERIC_ERROR;
    return WLDAP32_errormap[err];
}
