/*
 * WININET - Gopher implementation
 *
 * Copyright 2003 Kirill Smelkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/***********************************************************************
 *           GopherCreateLocatorA (WININET.@)
 *
 * Create a Gopher locator string from its component parts
 *
 * PARAMS
 *  lpszHost            [I] host name
 *  nServerPort         [I] port in host byteorder or INTERNET_INVALID_PORT_NUMBER for default
 *  lpszDisplayString   [I] document/directory to display (NULL - default directory)
 *  lpszSelectorString  [I] selector string for server (NULL - none)
 *  dwGopherType        [I] selector type (see GOPHER_TYPE_xxx)
 *  lpszLocator         [O] buffer for locator string
 *  lpdwBufferLength    [I] locator buffer length
 *
 * RETURNS
 *  TRUE  on success
 *  FALSE on failure
 *
 */
BOOL WINAPI GopherCreateLocatorA(
 LPCSTR        lpszHost,
 INTERNET_PORT nServerPort,
 LPCSTR        lpszDisplayString,
 LPCSTR        lpszSelectorString,
 DWORD         dwGopherType,
 LPSTR         lpszLocator,
 LPDWORD       lpdwBufferLength
)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherCreateLocatorW (WININET.@)
 *
 * See GopherCreateLocatorA.
 */
BOOL WINAPI GopherCreateLocatorW(
 LPCWSTR       lpszHost,
 INTERNET_PORT nServerPort,
 LPCWSTR       lpszDisplayString,
 LPCWSTR       lpszSelectorString,
 DWORD         dwHopherType,
 LPWSTR        lpszLocator,
 LPDWORD       lpdwBufferLength
)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherFindFirstFileA (WININET.@)
 *
 * Create a session and locate the requested documents
 *
 * PARAMS
 *  hConnect        [I] Handle to a Gopher session returned by InternetConnect
 *  lpszLocator     [I] - address of a string containing the name of the item to locate.
 *                      - Locator created by the GopherCreateLocator function.
 * lpszSearchString [I] what to search for if this request is to an index server.
 *                      Otherwise, this parameter should be NULL.
 * lpFindData       [O] retrived information
 * dwFlags          [I] INTERNET_FLAG_{HYPERLINK, NEED_FILE, NO_CACHE_WRITE, RELOAD, RESYNCHRONIZE}
 * dwContext        [I] application private value
 *
 * RETURNS
 *  HINTERNET handle on success
 *  NULL on error
 */
HINTERNET WINAPI GopherFindFirstFileA(
 HINTERNET hConnect,
 LPCSTR    lpszLocator,
 LPCSTR    lpszSearchString,
 LPGOPHER_FIND_DATAA
           lpFindData,
 DWORD     dwFlags,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           GopherFindFirstFileW (WININET.@)
 *
 * See GopherFindFirstFileA.
 */
HINTERNET WINAPI GopherFindFirstFileW(
 HINTERNET hConnect,
 LPCWSTR   lpszLocator,
 LPCWSTR   lpszSearchString,
 LPGOPHER_FIND_DATAW
           lpFindData,
 DWORD     dwFlags,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           GopherGetAttributeA (WININET.@)
 *
 * Retrieves the specific attribute information from the server.
 *
 * RETURNS
 *  TRUE on success
 *  FALSE on failure
 */
BOOL WINAPI GopherGetAttributeA(
 HINTERNET hConnect,
 LPCSTR    lpszLocator,
 LPCSTR    lpszAttributeName,
 LPBYTE    lpBuffer,
 DWORD     dwBufferLength,
 LPDWORD   lpdwCharactersReturned,
 GOPHER_ATTRIBUTE_ENUMERATORA
           lpfnEnumerator,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherGetAttributeW (WININET.@)
 *
 * See GopherGetAttributeA.
 */
BOOL WINAPI GopherGetAttributeW(
 HINTERNET hConnect,
 LPCWSTR   lpszLocator,
 LPCWSTR   lpszAttributeName,
 LPBYTE    lpBuffer,
 DWORD     dwBufferLength,
 LPDWORD   lpdwCharactersReturned,
 GOPHER_ATTRIBUTE_ENUMERATORW
           lpfnEnumerator,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherGetLocatorTypeA (WININET.@)
 *
 * Parses a Gopher locator and determines its attributes.
 *
 * PARAMS
 *  lpszLocator     [I] Address of the Gopher locator string to parse
 *  lpdwGopherType  [O] destination for bitmasked type of locator
 *
 * RETURNS
 *  TRUE  on success
 *  FALSE on failure
 */
BOOL WINAPI GopherGetLocatorTypeA(LPCSTR lpszLocator, LPDWORD lpdwGopherType)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherGetLocatorTypeW (WININET.@)
 *
 * See GopherGetLocatorTypeA.
 */
BOOL WINAPI GopherGetLocatorTypeW(LPCWSTR lpszLocator, LPDWORD lpdwGopherType)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           GopherOpenFileA (WININET.@)
 *
 * Begins reading a Gopher data file from a Gopher server.
 *
 * PARAMS
 *  hConnect    [I] handle to a Gopher session
 *  lpszLocator [I] file locator
 *  lpszView    [I] file view (or default if NULL)
 *  dwFlags     [I] INTERNET_FLAG_{HYPERLINK, NEED_FILE, NO_CACHE_WRITE, RELOAD, RESYNCHRONIZE}
 *  dwContext   [I] application private value
 *
 * RETURNS
 *  handle  on success
 *  NULL    on error
 */
HINTERNET WINAPI GopherOpenFileA(
 HINTERNET hConnect,
 LPCSTR    lpszLocator,
 LPCSTR    lpszView,
 DWORD     dwFlags,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *           GopherOpenFileW (WININET.@)
 *
 * See GopherOpenFileA.
 */
HINTERNET WINAPI GopherOpenFileW(
 HINTERNET hConnect,
 LPCWSTR   lpszLocator,
 LPCWSTR   lpszView,
 DWORD     dwFlags,
 DWORD     dwContext
)
{
    FIXME("stub\n");
    return NULL;
}
