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

#include <wchar.h>
#include "windef.h"
#include "winbase.h"
#include "ws2tcpip.h"
#include "winreg.h"
#include "winhttp.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

struct url_component
{
    WCHAR **str;
    DWORD  *len;
};

static DWORD set_component( struct url_component *comp, WCHAR *value, DWORD len, DWORD flags, BOOL *overflow )
{
    if (*comp->str && !*comp->len) return ERROR_INVALID_PARAMETER;
    if (!*comp->len) return ERROR_SUCCESS;
    if (!*comp->str)
    {
        if (len && *comp->len && (flags & (ICU_DECODE|ICU_ESCAPE))) return ERROR_INVALID_PARAMETER;
        *comp->str = value;
        *comp->len = len;
    }
    else
    {
        if (len >= *comp->len)
        {
            *comp->len = len + 1;
            *overflow = TRUE;
            return ERROR_SUCCESS;
        }
        memcpy( *comp->str, value, len * sizeof(WCHAR) );
        (*comp->str)[len] = 0;
        *comp->len = len;
    }
    return ERROR_SUCCESS;
}

static WCHAR *decode_url( LPCWSTR url, DWORD *len )
{
    const WCHAR *p = url;
    WCHAR hex[3], *q, *ret;

    if (!(ret = malloc( *len * sizeof(WCHAR) ))) return NULL;
    q = ret;
    while (*len > 0)
    {
        if (p[0] == '%' && iswxdigit( p[1] ) && iswxdigit( p[2] ))
        {
            hex[0] = p[1];
            hex[1] = p[2];
            hex[2] = 0;
            *q++ = wcstol( hex, NULL, 16 );
            p += 3;
            *len -= 3;
        }
        else
        {
            *q++ = *p++;
            *len -= 1;
        }
    }
    *len = q - ret;
    return ret;
}

static inline BOOL need_escape( WCHAR ch )
{
    const WCHAR *p = L" \"#%<>[\\]^`{|}~";

    if (ch <= 31 || ch >= 127) return TRUE;
    while (*p)
    {
        if (ch == *p++) return TRUE;
    }
    return FALSE;
}

static BOOL escape_string( const WCHAR *src, DWORD src_len, WCHAR *dst, DWORD *dst_len )
{
    static const WCHAR hex[] = L"0123456789ABCDEF";
    WCHAR *p = dst;
    DWORD i;

    *dst_len = src_len;
    for (i = 0; i < src_len; i++)
    {
        if (src[i] > 0xff) return FALSE;
        if (need_escape( src[i] ))
        {
            if (dst)
            {
                p[0] = '%';
                p[1] = hex[(src[i] >> 4) & 0xf];
                p[2] = hex[src[i] & 0xf];
                p += 3;
            }
            *dst_len += 2;
        }
        else if (dst) *p++ = src[i];
    }

    if (dst) dst[*dst_len] = 0;
    return TRUE;
}

static DWORD escape_url( const WCHAR *url, DWORD *len, WCHAR **ret )
{
    const WCHAR *p;
    DWORD len_base, len_path;

    if ((p = wcsrchr( url, '/' )))
    {
        len_base = p - url;
        if (!escape_string( p, *len - len_base, NULL, &len_path )) return ERROR_INVALID_PARAMETER;
    }
    else
    {
        len_base = *len;
        len_path = 0;
    }

    if (!(*ret = malloc( (len_base + len_path + 1) * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    memcpy( *ret, url, len_base * sizeof(WCHAR) );

    if (p) escape_string( p, *len - (p - url), *ret + len_base, &len_path );
    (*ret)[len_base + len_path] = 0;

    *len = len_base + len_path;
    return ERROR_SUCCESS;
}

static DWORD parse_port( const WCHAR *str, DWORD len, INTERNET_PORT *ret )
{
    const WCHAR *p = str;
    DWORD port = 0;
    while (len && '0' <= *p && *p <= '9')
    {
        if ((port = port * 10 + *p - '0') > 65535) return ERROR_WINHTTP_INVALID_URL;
        p++; len--;
    }
    *ret = port;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *          WinHttpCrackUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCrackUrl( const WCHAR *url, DWORD len, DWORD flags, URL_COMPONENTSW *uc )
{
    WCHAR *p, *q, *r, *url_transformed = NULL;
    INTERNET_SCHEME scheme_number = 0;
    struct url_component scheme, username, password, hostname, path, extra;
    BOOL overflow = FALSE;
    DWORD err;

    TRACE( "%s, %lu, %#lx, %p\n", debugstr_wn(url, len), len, flags, uc );

    if (!url || !uc || uc->dwStructSize != sizeof(*uc))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!len) len = lstrlenW( url );

    if (flags & ICU_ESCAPE)
    {
        if ((err = escape_url( url, &len, &url_transformed )))
        {
            SetLastError( err );
            return FALSE;
        }
        url = url_transformed;
    }
    else if (flags & ICU_DECODE)
    {
        if (!(url_transformed = decode_url( url, &len )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        url = url_transformed;
    }
    if (!(p = wcschr( url, ':' )))
    {
        SetLastError( ERROR_WINHTTP_UNRECOGNIZED_SCHEME );
        free( url_transformed );
        return FALSE;
    }
    if (p - url == 4 && !wcsnicmp( url, L"http", 4 )) scheme_number = INTERNET_SCHEME_HTTP;
    else if (p - url == 5 && !wcsnicmp( url, L"https", 5 )) scheme_number = INTERNET_SCHEME_HTTPS;
    else
    {
        err = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
        goto exit;
    }

    scheme.str = &uc->lpszScheme;
    scheme.len = &uc->dwSchemeLength;

    if ((err = set_component( &scheme, (WCHAR *)url, p - url, flags, &overflow ))) goto exit;

    p++; /* skip ':' */
    if (!p[0] || p[0] != '/' || p[1] != '/')
    {
        err = ERROR_WINHTTP_INVALID_URL;
        goto exit;
    }
    p += 2;
    if (!p[0])
    {
        err = ERROR_WINHTTP_INVALID_URL;
        goto exit;
    }

    username.str = &uc->lpszUserName;
    username.len = &uc->dwUserNameLength;

    password.str = &uc->lpszPassword;
    password.len = &uc->dwPasswordLength;

    if ((q = wmemchr( p, '@', len - (p - url) )) && !(wmemchr( p, '/', q - p )))
    {

        if ((r = wmemchr( p, ':', q - p )))
        {
            if ((err = set_component( &username, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if ((err = set_component( &password, r, q - r, flags, &overflow ))) goto exit;
        }
        else
        {
            if ((err = set_component( &username, p, q - p, flags, &overflow ))) goto exit;
            if ((err = set_component( &password, NULL, 0, flags, &overflow ))) goto exit;
        }
        p = q + 1;
    }
    else
    {
        if ((err = set_component( &username, NULL, 0, flags, &overflow ))) goto exit;
        if ((err = set_component( &password, NULL, 0, flags, &overflow ))) goto exit;
    }

    hostname.str = &uc->lpszHostName;
    hostname.len = &uc->dwHostNameLength;

    path.str = &uc->lpszUrlPath;
    path.len = &uc->dwUrlPathLength;

    extra.str = &uc->lpszExtraInfo;
    extra.len = &uc->dwExtraInfoLength;

    if ((q = wmemchr( p, '/', len - (p - url) )))
    {
        if ((r = wmemchr( p, ':', q - p )))
        {
            if ((err = set_component( &hostname, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if (!(q - r))
            {
                if (scheme_number == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
                else if (scheme_number == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
            }
            else if ((err = parse_port( r, q - r, &uc->nPort ))) goto exit;
        }
        else
        {
            if ((err = set_component( &hostname, p, q - p, flags, &overflow ))) goto exit;
            if (scheme_number == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            else if (scheme_number == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }

        if ((r = wmemchr( q, '?', len - (q - url) )))
        {
            if (*extra.len)
            {
                if ((err = set_component( &path, q, r - q, flags, &overflow ))) goto exit;
                if ((err = set_component( &extra, r, len - (r - url), flags, &overflow ))) goto exit;
            }
            else if ((err = set_component( &path, q, len - (q - url), flags, &overflow ))) goto exit;
        }
        else
        {
            if ((err = set_component( &path, q, len - (q - url), flags, &overflow ))) goto exit;
            if ((err = set_component( &extra, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
        }
    }
    else
    {
        if ((r = wmemchr( p, ':', len - (p - url) )))
        {
            if ((err = set_component( &hostname, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if (!*r)
            {
                if (scheme_number == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
                else if (scheme_number == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
            }
            else if ((err = parse_port( r, len - (r - url), &uc->nPort ))) goto exit;
        }
        else
        {
            if ((err = set_component( &hostname, p, len - (p - url), flags, &overflow ))) goto exit;
            if (scheme_number == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            else if (scheme_number == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }
        if ((err = set_component( &path, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
        if ((err = set_component( &extra, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
    }

    TRACE("scheme(%s) host(%s) port(%d) path(%s) extra(%s)\n", debugstr_wn(*scheme.str, *scheme.len),
          debugstr_wn(*hostname.str, *hostname.len ), uc->nPort, debugstr_wn(*path.str, *path.len),
          debugstr_wn(*extra.str, *extra.len));

exit:
    if (!err)
    {
        if (overflow) err = ERROR_INSUFFICIENT_BUFFER;
        uc->nScheme = scheme_number;
    }
    free( url_transformed );
    SetLastError( err );
    return !err;
}

static INTERNET_SCHEME get_scheme( const WCHAR *scheme, DWORD len )
{
    if (!wcsncmp( scheme, L"http", len )) return INTERNET_SCHEME_HTTP;
    if (!wcsncmp( scheme, L"https", len )) return INTERNET_SCHEME_HTTPS;
    return 0;
}

static const WCHAR *get_scheme_string( INTERNET_SCHEME scheme )
{
    if (scheme == INTERNET_SCHEME_HTTP) return L"http";
    if (scheme == INTERNET_SCHEME_HTTPS) return L"https";
    return NULL;
}

static BOOL uses_default_port( INTERNET_SCHEME scheme, INTERNET_PORT port )
{
    if ((scheme == INTERNET_SCHEME_HTTP) && (port == INTERNET_DEFAULT_HTTP_PORT)) return TRUE;
    if ((scheme == INTERNET_SCHEME_HTTPS) && (port == INTERNET_DEFAULT_HTTPS_PORT)) return TRUE;
    return FALSE;
}

static DWORD get_comp_length( DWORD len, DWORD flags, WCHAR *comp )
{
    DWORD ret;
    unsigned int i;

    ret = len ? len : lstrlenW( comp );
    if (!(flags & ICU_ESCAPE)) return ret;
    for (i = 0; i < len; i++) if (need_escape( comp[i] )) ret += 2;
    return ret;
}

static BOOL get_url_length( URL_COMPONENTS *uc, DWORD flags, DWORD *len )
{
    INTERNET_SCHEME scheme;

    *len = 0;
    if (uc->lpszScheme)
    {
        DWORD scheme_len = get_comp_length( uc->dwSchemeLength, 0, uc->lpszScheme );
        *len += scheme_len;
        scheme = get_scheme( uc->lpszScheme, scheme_len );
    }
    else
    {
        scheme = uc->nScheme;
        if (!scheme) scheme = INTERNET_SCHEME_HTTP;
        *len += lstrlenW( get_scheme_string( scheme ) );
    }
    *len += 3; /* "://" */

    if (uc->lpszUserName)
    {
        *len += get_comp_length( uc->dwUserNameLength, 0, uc->lpszUserName );
        *len += 1; /* "@" */
    }
    else
    {
        if (uc->lpszPassword)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
    if (uc->lpszPassword)
    {
        *len += 1; /* ":" */
        *len += get_comp_length( uc->dwPasswordLength, 0, uc->lpszPassword );
    }
    if (uc->lpszHostName)
    {
        *len += get_comp_length( uc->dwHostNameLength, 0, uc->lpszHostName );

        if (!uses_default_port( scheme, uc->nPort ))
        {
            WCHAR port[sizeof("65535")];

            *len += swprintf( port, ARRAY_SIZE(port), L"%u", uc->nPort );
            *len += 1; /* ":" */
        }
        if (uc->lpszUrlPath && *uc->lpszUrlPath != '/') *len += 1; /* '/' */
    }
    if (uc->lpszUrlPath) *len += get_comp_length( uc->dwUrlPathLength, flags, uc->lpszUrlPath );
    if (uc->lpszExtraInfo) *len += get_comp_length( uc->dwExtraInfoLength, flags, uc->lpszExtraInfo );
    return TRUE;
}

/***********************************************************************
 *          WinHttpCreateUrl (winhttp.@)
 */
BOOL WINAPI WinHttpCreateUrl( URL_COMPONENTS *uc, DWORD flags, WCHAR *url, DWORD *required )
{
    DWORD len, len_escaped;
    INTERNET_SCHEME scheme;

    TRACE( "%p, %#lx, %p, %p\n", uc, flags, url, required );

    if (!uc || uc->dwStructSize != sizeof(URL_COMPONENTS) || !required)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!get_url_length( uc, flags, &len )) return FALSE;

    if (*required < len)
    {
        *required = len + 1;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (!url)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    url[0] = 0;
    *required = len;
    if (uc->lpszScheme)
    {
        len = get_comp_length( uc->dwSchemeLength, 0, uc->lpszScheme );
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
        len = lstrlenW( schemeW );
        memcpy( url, schemeW, len * sizeof(WCHAR) );
        url += len;
    }

    *url++ = ':';
    *url++ = '/';
    *url++ = '/';

    if (uc->lpszUserName)
    {
        len = get_comp_length( uc->dwUserNameLength, 0, uc->lpszUserName );
        memcpy( url, uc->lpszUserName, len * sizeof(WCHAR) );
        url += len;

        if (uc->lpszPassword)
        {
            *url++ = ':';
            len = get_comp_length( uc->dwPasswordLength, 0, uc->lpszPassword );
            memcpy( url, uc->lpszPassword, len * sizeof(WCHAR) );
            url += len;
        }
        *url++ = '@';
    }
    if (uc->lpszHostName)
    {
        len = get_comp_length( uc->dwHostNameLength, 0, uc->lpszHostName );
        memcpy( url, uc->lpszHostName, len * sizeof(WCHAR) );
        url += len;

        if (!uses_default_port( scheme, uc->nPort ))
        {
            *url++ = ':';
            url += swprintf( url, sizeof("65535"), L"%u", uc->nPort );
        }

        /* add slash between hostname and path if necessary */
        if (uc->lpszUrlPath && *uc->lpszUrlPath != '/')
        {
            *url++ = '/';
        }
    }
    if (uc->lpszUrlPath)
    {
        len = get_comp_length( uc->dwUrlPathLength, 0, uc->lpszUrlPath );
        if (flags & ICU_ESCAPE)
        {
            if (!escape_string( uc->lpszUrlPath, len, url, &len_escaped ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            url += len_escaped;
        }
        else
        {
            memcpy( url, uc->lpszUrlPath, len * sizeof(WCHAR) );
            url += len;
        }
    }
    if (uc->lpszExtraInfo)
    {
        len = get_comp_length( uc->dwExtraInfoLength, 0, uc->lpszExtraInfo );
        if (flags & ICU_ESCAPE)
        {
            if (!escape_string( uc->lpszExtraInfo, len, url, &len_escaped ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            url += len_escaped;
        }
        else
        {
            memcpy( url, uc->lpszExtraInfo, len * sizeof(WCHAR) );
            url += len;
        }
    }
    *url = 0;
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}
