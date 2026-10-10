/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "sspi.h"
#include "wincred.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sspicli);

/***********************************************************************
 *		SspiEncodeStringsAsAuthIdentity (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiEncodeStringsAsAuthIdentity(
    const WCHAR *username, const WCHAR *domainname, const WCHAR *creds,
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE *opaque_id )
{
    SEC_WINNT_AUTH_IDENTITY_W *id;
    DWORD len_username = 0, len_domainname = 0, len_password = 0, size;
    WCHAR *ptr;

    FIXME( "%s %s %s %p\n", debugstr_w(username), debugstr_w(domainname),
           debugstr_w(creds), opaque_id );

    if (!username && !domainname && !creds) return SEC_E_INVALID_TOKEN;

    if (username) len_username = lstrlenW( username );
    if (domainname) len_domainname = lstrlenW( domainname );
    if (creds) len_password = lstrlenW( creds );

    size = sizeof(*id);
    if (username) size += (len_username + 1) * sizeof(WCHAR);
    if (domainname) size += (len_domainname + 1) * sizeof(WCHAR);
    if (creds) size += (len_password + 1) * sizeof(WCHAR);
    if (!(id = calloc( 1, size ))) return ERROR_OUTOFMEMORY;
    ptr = (WCHAR *)(id + 1);

    if (username)
    {
        memcpy( ptr, username, (len_username + 1) * sizeof(WCHAR) );
        id->User       = ptr;
        id->UserLength = len_username;
        ptr += len_username + 1;
    }
    if (domainname)
    {
        memcpy( ptr, domainname, (len_domainname + 1) * sizeof(WCHAR) );
        id->Domain       = ptr;
        id->DomainLength = len_domainname;
        ptr += len_domainname + 1;
    }
    if (creds)
    {
        memcpy( ptr, creds, (len_password + 1) * sizeof(WCHAR) );
        id->Password       = ptr;
        id->PasswordLength = len_password;
    }

    *opaque_id = id;
    return SEC_E_OK;
}

/***********************************************************************
 *		SspiZeroAuthIdentity (SECUR32.0)
 */
void SEC_ENTRY SspiZeroAuthIdentity( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id )
{
    SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

    TRACE( "%p\n", opaque_id );

    if (!id) return;
    if (id->User) memset( id->User, 0, id->UserLength * sizeof(WCHAR) );
    if (id->Domain) memset( id->Domain, 0, id->DomainLength * sizeof(WCHAR) );
    if (id->Password) memset( id->Password, 0, id->PasswordLength * sizeof(WCHAR) );
    memset( id, 0, sizeof(*id) );
}

/***********************************************************************
 *		SspiEncodeAuthIdentityAsStrings (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiEncodeAuthIdentityAsStrings(
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id, PCWSTR *username,
    PCWSTR *domainname, PCWSTR *creds )
{
    SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

    FIXME("%p %p %p %p\n", opaque_id, username, domainname, creds);

    *username = wcsdup( id->User );
    *domainname = wcsdup( id->Domain );
    *creds = wcsdup( id->Password );

    return SEC_E_OK;
}

/***********************************************************************
 *		SspiFreeAuthIdentity (SECUR32.0)
 */
void SEC_ENTRY SspiFreeAuthIdentity( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id )
{
    TRACE( "%p\n", opaque_id );
    free( opaque_id );
}

/***********************************************************************
 *		SspiLocalFree (SECUR32.0)
 */
void SEC_ENTRY SspiLocalFree( void *ptr )
{
    TRACE( "%p\n", ptr );
    free( ptr );
}

/***********************************************************************
 *		SspiPrepareForCredWrite (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiPrepareForCredWrite( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id,
    PCWSTR target, PULONG type, PCWSTR *targetname, PCWSTR *username, PUCHAR *blob, PULONG size )
{
    SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;
    WCHAR *str, *str2;
    UCHAR *password;
    ULONG len;

    FIXME( "%p %s %p %p %p %p %p\n", opaque_id, debugstr_w(target), type, targetname, username,
           blob, size );

    if (id->DomainLength)
    {
        len = (id->DomainLength + id->UserLength + 2) * sizeof(WCHAR);
        if (!(str = malloc( len ))) return SEC_E_INSUFFICIENT_MEMORY;
        memcpy( str, id->Domain, id->DomainLength * sizeof(WCHAR) );
        str[id->DomainLength] = '\\';
        memcpy( str + id->DomainLength + 1, id->User, id->UserLength * sizeof(WCHAR) );
        str[id->DomainLength + 1 + id->UserLength] = 0;
    }
    else
    {
        len = (id->UserLength + 1) * sizeof(WCHAR);
        if (!(str = malloc( len ))) return SEC_E_INSUFFICIENT_MEMORY;
        memcpy( str, id->User, id->UserLength * sizeof(WCHAR) );
        str[id->UserLength] = 0;
    }

    str2 = target ? wcsdup( target ) : wcsdup( str );
    if (!str2)
    {
        free( str );
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    len = id->PasswordLength * sizeof(WCHAR);
    if (!(password = malloc( len )))
    {
        free( str );
        free( str2 );
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    memcpy( password, id->Password, len );

    *type = CRED_TYPE_DOMAIN_PASSWORD;
    *username = str;
    *targetname = str2;
    *blob = password;
    *size = len;

    return SEC_E_OK;
}
