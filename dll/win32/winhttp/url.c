/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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
#include <stdarg.h>

#include "wine/debug.h"

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winhttp.h"
#include "shlwapi.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static const WCHAR scheme_http[] = {'h','t','t','p',0};
static const WCHAR scheme_https[] = {'h','t','t','p','s',0};

static BOOL set_component( WCHAR **str, DWORD *str_len, WCHAR *value, DWORD len, DWORD flags )
{
    if (!*str)
    {
        if (len && (flags & ICU_DECODE))
        {
            set_last_error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        *str = value;
        *str_len = len;
    }
    else
    {
        if (len > (*str_len) - 1)
        {
            *str_len = len + 1;
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        memcpy( *str, value, len * sizeof(WCHAR) );
        (*str)[len] = 0;
        *str_len = len;
    }
    return TRUE;
}

static BOOL decode_url( LPCWSTR url, LPWSTR buffer, LPDWORD buflen )
{
    HRESULT hr = UrlCanonicalizeW( url, buffer, buflen, URL_WININET_COMPATIBILITY | URL_UNESCAPE );
    if (hr == E_POINTER) set_last_error( ERROR_INSUFFICIENT_BUFFER );
    if (hr == E_INVALIDARG) set_last_error( ERROR_INVALID_PARAMETER );
    return (SUCCEEDED(hr)) ? TRUE : FALSE;
}

/***********************************************************************
 *          WinHttpCrackUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCrackUrl( LPCWSTR url, DWORD len, DWORD flags, LPURL_COMPONENTSW uc )
{
    BOOL ret = FALSE;
    WCHAR *p, *q, *r;
    WCHAR *url_decoded = NULL;

    TRACE("%s, %d, %x, %p\n", debugstr_w(url), len, flags, uc);

    if (flags & ICU_ESCAPE) FIXME("flag ICU_ESCAPE not supported\n");

    if (!url || !uc || uc->dwStructSize != sizeof(URL_COMPONENTS))
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!len) len = strlenW( url );

    if (flags & ICU_DECODE)
    {
        WCHAR *url_tmp;
        DWORD url_len = len + 1;

        if (!(url_tmp = HeapAlloc( GetProcessHeap(), 0, url_len * sizeof(WCHAR) )))
        {
            set_last_error( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        memcpy( url_tmp, url, len * sizeof(WCHAR) );
        url_tmp[len] = 0;
        if (!(url_decoded = HeapAlloc( GetProcessHeap(), 0, url_len * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, url_tmp );
            set_last_error( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        if (decode_url( url_tmp, url_decoded, &url_len ))
        {
            len = url_len;
            url = url_decoded;
        }
        HeapFree( GetProcessHeap(), 0, url_tmp );
    }
    if (!(p = strchrW( url, ':' )))
    {
        set_last_error( ERROR_WINHTTP_UNRECOGNIZED_SCHEME );
        return FALSE;
    }
    if (p - url == 4 && !strncmpiW( url, scheme_http, 4 )) uc->nScheme = INTERNET_SCHEME_HTTP;
    else if (p - url == 5 && !strncmpiW( url, scheme_https, 5 )) uc->nScheme = INTERNET_SCHEME_HTTPS;
    else
    {
        set_last_error( ERROR_WINHTTP_UNRECOGNIZED_SCHEME );
        goto exit;
    }
    if (!(set_component( &uc->lpszScheme, &uc->dwSchemeLength, (WCHAR *)url, p - url, flags ))) goto exit;

    p++; /* skip ':' */
    if (!p[0] || p[0] != '/' || p[1] != '/') goto exit;
    p += 2;

    if (!p[0]) goto exit;
    if ((q = memchrW( p, '@', len - (p - url) )))
    {
        if ((r = memchrW( p, ':', q - p )))
        {
            if (!(set_component( &uc->lpszUserName, &uc->dwUserNameLength, p, r - p, flags ))) goto exit;
            r++;
            if (!(set_component( &uc->lpszPassword, &uc->dwPasswordLength, r, q - r, flags ))) goto exit;
        }
        else
        {
            if (!(set_component( &uc->lpszUserName, &uc->dwUserNameLength, p, q - p, flags ))) goto exit;
            if (!(set_component( &uc->lpszPassword, &uc->dwPasswordLength, NULL, 0, flags ))) goto exit;
        }
        p = q + 1;
    }
    else
    {
        if (!(set_component( &uc->lpszUserName, &uc->dwUserNameLength, NULL, 0, flags ))) goto exit;
        if (!(set_component( &uc->lpszPassword, &uc->dwPasswordLength, NULL, 0, flags ))) goto exit;
    }
    if ((q = memchrW( p, '/', len - (p - url) )))
    {
        if ((r = memchrW( p, ':', q - p )))
        {
            if (!(set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, r - p, flags ))) goto exit;
            r++;
            uc->nPort = atoiW( r );
        }
        else
        {
            if (!(set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, q - p, flags ))) goto exit;
            if (uc->nScheme == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            if (uc->nScheme == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }

        if ((r = memchrW( q, '?', len - (q - url) )))
        {
            if (!(set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, q, r - q, flags ))) goto exit;
            if (!(set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, r, len - (r - url), flags ))) goto exit;
        }
        else
        {
            if (!(set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, q, len - (q - url), flags ))) goto exit;
            if (!(set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, (WCHAR *)url + len, 0, flags ))) goto exit;
        }
    }
    else
    {
        if ((r = memchrW( p, ':', len - (p - url) )))
        {
            if (!(set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, r - p, flags ))) goto exit;
            r++;
            uc->nPort = atoiW( r );
        }
        else
        {
            if (!(set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, len - (p - url), flags ))) goto exit;
            if (uc->nScheme == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            if (uc->nScheme == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }
        if (!(set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, (WCHAR *)url + len, 0, flags ))) goto exit;
        if (!(set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, (WCHAR *)url + len, 0, flags ))) goto exit;
    }

    ret = TRUE;

    TRACE("scheme(%s) host(%s) port(%d) path(%s) extra(%s)\n",
          debugstr_wn( uc->lpszScheme, uc->dwSchemeLength ),
          debugstr_wn( uc->lpszHostName, uc->dwHostNameLength ),
          uc->nPort,
          debugstr_wn( uc->lpszUrlPath, uc->dwUrlPathLength ),
          debugstr_wn( uc->lpszExtraInfo, uc->dwExtraInfoLength ));

exit:
    HeapFree( GetProcessHeap(), 0, url_decoded );
    return ret;
}

static INTERNET_SCHEME get_scheme( const WCHAR *scheme, DWORD len )
{
    if (!strncmpW( scheme, scheme_http, len )) return INTERNET_SCHEME_HTTP;
    if (!strncmpW( scheme, scheme_https, len )) return INTERNET_SCHEME_HTTPS;
    return 0;
}

static const WCHAR *get_scheme_string( INTERNET_SCHEME scheme )
{
    if (scheme == INTERNET_SCHEME_HTTP) return scheme_http;
    if (scheme == INTERNET_SCHEME_HTTPS) return scheme_https;
    return NULL;
}

static BOOL uses_default_port( INTERNET_SCHEME scheme, INTERNET_PORT port )
{
    if ((scheme == INTERNET_SCHEME_HTTP) && (port == INTERNET_DEFAULT_HTTP_PORT)) return TRUE;
    if ((scheme == INTERNET_SCHEME_HTTPS) && (port == INTERNET_DEFAULT_HTTPS_PORT)) return TRUE;
    return FALSE;
}

static BOOL need_escape( WCHAR c )
{
    if (isalnumW( c )) return FALSE;

    if (c <= 31 || c >= 127) return TRUE;
    else
    {
        switch (c)
        {
        case ' ':
        case '"':
        case '#':
        case '%':
        case '<':
        case '>':
        case ']':
        case '\\':
        case '[':
        case '^':
        case '`':
        case '{':
        case '|':
        case '}':
        case '~':
            return TRUE;
        default:
            return FALSE;
        }
    }
}

static DWORD copy_escape( WCHAR *dst, const WCHAR *src, DWORD len )
{
    static const WCHAR hex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    DWORD ret = len;
    unsigned int i;
    WCHAR *p = dst;

    for (i = 0; i < len; i++, p++)
    {
        if (need_escape( src[i] ))
        {
            p[0] = '%';
            p[1] = hex[(src[i] >> 4) & 0xf];
            p[2] = hex[src[i] & 0xf];
            ret += 2;
            p += 2;
        }
        else *p = src[i];
    }
    dst[ret] = 0;
    return ret;
}

static DWORD comp_length( DWORD len, DWORD flags, WCHAR *comp )
{
    DWORD ret;
    unsigned int i;

    ret = len ? len : strlenW( comp );
    if (!(flags & ICU_ESCAPE)) return ret;
    for (i = 0; i < len; i++) if (need_escape( comp[i] )) ret += 2;
    return ret;
}

static BOOL calc_length( URL_COMPONENTS *uc, DWORD flags, LPDWORD len )
{
    static const WCHAR formatW[] = {'%','d',0};
    INTERNET_SCHEME scheme;

    *len = 0;
    if (uc->lpszScheme)
    {
        DWORD scheme_len = comp_length( uc->dwSchemeLength, 0, uc->lpszScheme );
        *len += scheme_len;
        scheme = get_scheme( uc->lpszScheme, scheme_len );
    }
    else
    {
        scheme = uc->nScheme;
        if (!scheme) scheme = INTERNET_SCHEME_HTTP;
        *len += strlenW( get_scheme_string( scheme ) );
    }
    *len += 1; /* ':' */
    if (uc->lpszHostName) *len += 2; /* "//" */

    if (uc->lpszUserName)
    {
        *len += comp_length( uc->dwUserNameLength, 0, uc->lpszUserName );
        *len += 1; /* "@" */
    }
    else
    {
        if (uc->lpszPassword)
        {
            set_last_error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
    if (uc->lpszPassword)
    {
        *len += 1; /* ":" */
        *len += comp_length( uc->dwPasswordLength, 0, uc->lpszPassword );
    }
    if (uc->lpszHostName)
    {
        *len += comp_length( uc->dwHostNameLength, 0, uc->lpszHostName );

        if (!uses_default_port( scheme, uc->nPort ))
        {
            WCHAR port[sizeof("65535")];

            sprintfW( port, formatW, uc->nPort );
            *len += strlenW( port );
            *len += 1; /* ":" */
        }
        if (uc->lpszUrlPath && *uc->lpszUrlPath != '/') *len += 1; /* '/' */
    }
    if (uc->lpszUrlPath) *len += comp_length( uc->dwUrlPathLength, flags, uc->lpszUrlPath );
    if (uc->lpszExtraInfo) *len += comp_length( uc->dwExtraInfoLength, flags, uc->lpszExtraInfo );
    return TRUE;
}

/***********************************************************************
 *          WinHttpCreateUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCreateUrl( LPURL_COMPONENTS uc, DWORD flags, LPWSTR url, LPDWORD required )
{
    static const WCHAR formatW[] = {'%','d',0};
    static const WCHAR twoslashW[] = {'/','/'};

    DWORD len;
    INTERNET_SCHEME scheme;

    TRACE("%p, 0x%08x, %p, %p\n", uc, flags, url, required);

    if (!uc || uc->dwStructSize != sizeof(URL_COMPONENTS) || !required)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!calc_length( uc, flags, &len )) return FALSE;

    if (!url || *required < len)
    {
        *required = len + 1;
        set_last_error( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    url[0] = 0;
    *required = len;
    if (uc->lpszScheme)
    {
        len = comp_length( uc->dwSchemeLength, 0, uc->lpszScheme );
        memcpy( url, uc->lpszScheme, len * sizeof(WCHAR) );
        url += len;

        scheme = get_scheme( uc->lpszScheme, len );
    }
    else
    {
        const WCHAR *schemeW;
        scheme = uc->nScheme;

        if (!scheme) scheme = INTERNET_SCHEME_HTTP;

        schemeW = get_scheme_string( scheme );
        len = strlenW( schemeW );
        memcpy( url, schemeW, len * sizeof(WCHAR) );
        url += len;
    }

    /* all schemes are followed by at least a colon */
    *url = ':';
    url++;

    if (uc->lpszHostName)
    {
        memcpy( url, twoslashW, sizeof(twoslashW) );
        url += sizeof(twoslashW) / sizeof(twoslashW[0]);
    }
    if (uc->lpszUserName)
    {
        len = comp_length( uc->dwUserNameLength, 0, uc->lpszUserName );
        memcpy( url, uc->lpszUserName, len * sizeof(WCHAR) );
        url += len;

        if (uc->lpszPassword)
        {
            *url = ':';
            url++;

            len = comp_length( uc->dwPasswordLength, 0, uc->lpszPassword );
            memcpy( url, uc->lpszPassword, len * sizeof(WCHAR) );
            url += len;
        }
        *url = '@';
        url++;
    }
    if (uc->lpszHostName)
    {
        len = comp_length( uc->dwHostNameLength, 0, uc->lpszHostName );
        memcpy( url, uc->lpszHostName, len * sizeof(WCHAR) );
        url += len;

        if (!uses_default_port( scheme, uc->nPort ))
        {
            WCHAR port[sizeof("65535")];

            sprintfW( port, formatW, uc->nPort );
            *url = ':';
            url++;

            len = strlenW( port );
            memcpy( url, port, len * sizeof(WCHAR) );
            url += len;
        }

        /* add slash between hostname and path if necessary */
        if (uc->lpszUrlPath && *uc->lpszUrlPath != '/')
        {
            *url = '/';
            url++;
        }
    }
    if (uc->lpszUrlPath)
    {
        len = comp_length( uc->dwUrlPathLength, 0, uc->lpszUrlPath );
        if (flags & ICU_ESCAPE) url += copy_escape( url, uc->lpszUrlPath, len );
        else
        {
            memcpy( url, uc->lpszUrlPath, len * sizeof(WCHAR) );
            url += len;
        }
    }
    if (uc->lpszExtraInfo)
    {
        len = comp_length( uc->dwExtraInfoLength, 0, uc->lpszExtraInfo );
        if (flags & ICU_ESCAPE) url += copy_escape( url, uc->lpszExtraInfo, len );
        else
        {
            memcpy( url, uc->lpszExtraInfo, len * sizeof(WCHAR) );
            url += len;
        }
    }
    *url = 0;
    return TRUE;
}
