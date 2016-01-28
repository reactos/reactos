/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */

#include <wine/config.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/debug.h>

#include <winbase.h>
#include <ntsecapi.h>
#include <bcrypt.h>

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG dwAlgOperations, ULONG *pAlgCount,
                                     BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags)
{
    FIXME("%08x, %p, %p, %08x - stub\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags);

    *ppAlgList=NULL;
    *pAlgCount=0;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE algorithm, UCHAR *buffer, ULONG count, ULONG flags)
{
    const DWORD supported_flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
    TRACE("%p, %p, %u, %08x - semi-stub\n", algorithm, buffer, count, flags);

    if (!algorithm)
    {
        /* It's valid to call without an algorithm if BCRYPT_USE_SYSTEM_PREFERRED_RNG
         * is set. In this case the preferred system RNG is used.
         */
        if (!(flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
            return STATUS_INVALID_HANDLE;
    }
    if (!buffer)
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags)
        FIXME("unsupported flags %08x\n", flags & ~supported_flags);

    if (algorithm)
        FIXME("ignoring selected algorithm\n");

    /* When zero bytes are requested the function returns success too. */
    if (!count)
        return STATUS_SUCCESS;

    if (flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG)
    {
        if (RtlGenRandom(buffer, count))
            return STATUS_SUCCESS;
    }

    FIXME("called with unsupported parameters, returning error\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *algorithm, LPCWSTR algorithmId,
                                            LPCWSTR implementation, DWORD flags)
{
    FIXME("%p, %s, %s, %08x - stub\n", algorithm, wine_dbgstr_w(algorithmId), wine_dbgstr_w(implementation), flags);

    if (!algorithm)
        return STATUS_INVALID_PARAMETER;

    *algorithm = NULL;

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE algorithm, DWORD flags)
{
    FIXME("%p, %08x - stub\n", algorithm, flags);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *enabled)
{
    FIXME("%p - semi-stub\n", enabled);

    if (!enabled)
        return STATUS_INVALID_PARAMETER;

    *enabled = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptGetProperty(BCRYPT_HANDLE obj, LPCWSTR prop, UCHAR *buffer, ULONG count, ULONG *res, ULONG flags)
{
    FIXME("%p, %s, %p, %u, %p, %08x - stub\n", obj, wine_dbgstr_w(prop), buffer, count, res, flags);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE algorithm, BCRYPT_HASH_HANDLE* hash, UCHAR* hashobject,
				 ULONG hashobjectlen, UCHAR *secret, ULONG secretlen, ULONG flags)
{
    FIXME("%p, %p, %p, %u, %p, %u, %08x - stub\n", algorithm, hash, hashobject, hashobjectlen, secret, secretlen, flags);

    return STATUS_NOT_IMPLEMENTED;
}
