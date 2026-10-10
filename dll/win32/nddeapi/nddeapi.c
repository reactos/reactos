/*
 * nddeapi main
 *
 * Copyright 2006 Benjamin Arai (Google)
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nddeapi);

/* Network DDE functionality was removed in Windows Vista, so the functions are silent stubs.
 * Since the corresponding header is no longer available in the Windows SDK, a required definition
 * is replicated here. */
#define NDDE_NOT_IMPLEMENTED 14

/***********************************************************************
 *             NDdeShareAddA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareAddA(LPSTR lpszServer, UINT nLevel, PSECURITY_DESCRIPTOR pSD,
                          LPBYTE lpBuffer, DWORD cBufSize)
{
    TRACE("(%s, %u, %p, %p, %lu)\n", debugstr_a(lpszServer), nLevel, pSD, lpBuffer, cBufSize);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareDelA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareDelA(LPSTR lpszServer, LPSTR lpszShareName, UINT wReserved)
{
    TRACE("(%s, %s, %u)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), wReserved);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareEnumA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareEnumA(LPSTR lpszServer, UINT nLevel, LPBYTE lpBuffer, DWORD cBufSize,
                           LPDWORD lpnEntriesRead, LPDWORD lpcbTotalAvailable)
{
    TRACE("(%s, %u, %p, %lu, %p, %p)\n", debugstr_a(lpszServer), nLevel, lpBuffer, cBufSize,
                                        lpnEntriesRead, lpcbTotalAvailable);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareGetInfoA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareGetInfoA(LPSTR lpszServer, LPSTR lpszShareName, UINT nLevel, LPBYTE lpBuffer,
                              DWORD cBufSize, LPDWORD lpnTotalAvailable, LPWORD lpnItems)
{
    TRACE("(%s, %s, %u, %p, %lu, %p, %p)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), nLevel,
                                            lpBuffer, cBufSize, lpnTotalAvailable, lpnItems);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareSetInfoA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareSetInfoA(LPSTR lpszServer, LPSTR lpszShareName, UINT nLevel, LPBYTE lpBuffer,
                              DWORD cBufSize, WORD sParmNum)
{
    TRACE("(%s, %s, %u, %p, %lu, %u)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), nLevel,
                                        lpBuffer, cBufSize, sParmNum);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeGetErrorStringA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetErrorStringA(UINT uErrorCode, LPSTR lpszErrorString, DWORD cBufSize)
{
    TRACE("(%u, %p, %ld)\n", uErrorCode, lpszErrorString, cBufSize);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeIsValidShareNameA   (NDDEAPI.@)
 *
 */
BOOL WINAPI NDdeIsValidShareNameA(LPSTR shareName)
{
    TRACE("(%s)\n", debugstr_a(shareName));

    return FALSE;
}

/***********************************************************************
 *             NDdeIsValidAppTopicListA   (NDDEAPI.@)
 *
 */
BOOL WINAPI NDdeIsValidAppTopicListA(LPSTR targetTopic)
{
    TRACE("(%s)\n", debugstr_a(targetTopic));

    return FALSE;
}

/***********************************************************************
 *             NDdeGetShareSecurityA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetShareSecurityA(LPSTR lpszServer, LPSTR lpszShareName, SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD, DWORD cbSD, LPDWORD lpcbsdRequired)
{
    TRACE("(%s, %s, %lu, %p, %lu, %p)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName),
                                        si, pSD, cbSD, lpcbsdRequired);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeSetShareSecurityA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeSetShareSecurityA(LPSTR lpszServer, LPSTR lpszShareName, SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD)
{
    TRACE("(%s, %s, %lu, %p)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), si, pSD);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeGetTrustedShareA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetTrustedShareA(LPSTR lpszServer, LPSTR lpszShareName, LPDWORD lpdwTrustOptions,
                                 LPDWORD lpdwShareModId0, LPDWORD lpdwShareModId1)
{
    TRACE("(%s, %s, %p, %p, %p)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), lpdwTrustOptions,
                                    lpdwShareModId0, lpdwShareModId1);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeSetTrustedShareA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeSetTrustedShareA(LPSTR lpszServer, LPSTR lpszShareName, DWORD dwTrustOptions)
{
    TRACE("(%s, %s, 0x%08lx)\n", debugstr_a(lpszServer), debugstr_a(lpszShareName), dwTrustOptions);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeTrustedShareEnumA   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeTrustedShareEnumA(LPSTR lpszServer, UINT nLevel, LPBYTE lpBuffer, DWORD cBufSize,
                                  LPDWORD lpnEntriesRead, LPDWORD lpcbTotalAvailable)
{
    TRACE("(%s, %u, %p, %lu, %p, %p)\n", debugstr_a(lpszServer), nLevel, lpBuffer, cBufSize,
                                        lpnEntriesRead, lpcbTotalAvailable);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareAddW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareAddW(LPWSTR lpszServer, UINT nLevel, PSECURITY_DESCRIPTOR pSD,
                          LPBYTE lpBuffer, DWORD cBufSize)
{
    TRACE("(%s, %u, %p, %p, %lu)\n", debugstr_w(lpszServer), nLevel, pSD, lpBuffer, cBufSize);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareDelW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareDelW(LPWSTR lpszServer, LPWSTR lpszShareName, UINT wReserved)
{
    TRACE("(%s, %s, %u)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), wReserved);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareEnumW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareEnumW(LPWSTR lpszServer, UINT nLevel, LPBYTE lpBuffer, DWORD cBufSize,
                           LPDWORD lpnEntriesRead, LPDWORD lpcbTotalAvailable)
{
    TRACE("(%s, %u, %p, %lu, %p, %p)\n", debugstr_w(lpszServer), nLevel, lpBuffer, cBufSize,
                                        lpnEntriesRead, lpcbTotalAvailable);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareGetInfoW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareGetInfoW(LPWSTR lpszServer, LPWSTR lpszShareName, UINT nLevel, LPBYTE lpBuffer,
                              DWORD cBufSize, LPDWORD lpnTotalAvailable, LPWORD lpnItems)
{
    TRACE("(%s, %s, %u, %p, %lu, %p, %p)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), nLevel,
                                            lpBuffer, cBufSize, lpnTotalAvailable, lpnItems);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeShareSetInfoW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeShareSetInfoW(LPWSTR lpszServer, LPWSTR lpszShareName, UINT nLevel, LPBYTE lpBuffer,
                              DWORD cBufSize, WORD sParmNum)
{
    TRACE("(%s, %s, %u, %p, %lu, %u)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), nLevel,
                                        lpBuffer, cBufSize, sParmNum);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeGetErrorStringW   (NDDEAPI.@)
 *
*/
UINT WINAPI NDdeGetErrorStringW(UINT uErrorCode, LPWSTR lpszErrorString, DWORD cBufSize)
{
    FIXME("(%u, %p, %ld): stub!\n", uErrorCode, lpszErrorString, cBufSize);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeIsValidShareNameW   (NDDEAPI.@)
 *
 */
BOOL WINAPI NDdeIsValidShareNameW(LPWSTR shareName)
{
    TRACE("(%s)\n", debugstr_w(shareName));

    return FALSE;
}

/***********************************************************************
 *             NDdeIsValidAppTopicListW   (NDDEAPI.@)
 *
 */
BOOL WINAPI NDdeIsValidAppTopicListW(LPWSTR targetTopic)
{
    TRACE("(%s)\n", debugstr_w(targetTopic));

    return FALSE;
}

/***********************************************************************
 *             NDdeGetShareSecurityW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetShareSecurityW(LPWSTR lpszServer, LPWSTR lpszShareName, SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD, DWORD cbSD, LPDWORD lpcbsdRequired)
{
    TRACE("(%s, %s, %lu, %p, %lu, %p)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName),
                                        si, pSD, cbSD, lpcbsdRequired);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeSetShareSecurityW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeSetShareSecurityW(LPWSTR lpszServer, LPWSTR lpszShareName, SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD)
{
    TRACE("(%s, %s, %lu, %p)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), si, pSD);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeGetTrustedShareW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeGetTrustedShareW(LPWSTR lpszServer, LPWSTR lpszShareName, LPDWORD lpdwTrustOptions,
                                 LPDWORD lpdwShareModId0, LPDWORD lpdwShareModId1)
{
    TRACE("(%s, %s, %p, %p, %p)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), lpdwTrustOptions,
                                    lpdwShareModId0, lpdwShareModId1);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeSetTrustedShareW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeSetTrustedShareW(LPWSTR lpszServer, LPWSTR lpszShareName, DWORD dwTrustOptions)
{
    TRACE("(%s, %s, 0x%08lx)\n", debugstr_w(lpszServer), debugstr_w(lpszShareName), dwTrustOptions);

    return NDDE_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             NDdeTrustedShareEnumW   (NDDEAPI.@)
 *
 */
UINT WINAPI NDdeTrustedShareEnumW(LPWSTR lpszServer, UINT nLevel, LPBYTE lpBuffer, DWORD cBufSize,
                                  LPDWORD lpnEntriesRead, LPDWORD lpcbTotalAvailable)
{
    TRACE("(%s, %u, %p, %lu, %p, %p)\n", debugstr_w(lpszServer), nLevel, lpBuffer, cBufSize,
                                        lpnEntriesRead, lpcbTotalAvailable);

    return NDDE_NOT_IMPLEMENTED;
}
