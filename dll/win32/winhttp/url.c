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

#include "winhttp_private.h"

static const WCHAR scheme_http[] = {'h','t','t','p',0};
static const WCHAR scheme_https[] = {'h','t','t','p','s',0};

static DWORD set_component( WCHAR **str, DWORD *str_len, WCHAR *value, DWORD len, DWORD flags, BOOL *overflow )
{
    if (*str && !*str_len) return ERROR_INVALID_PARAMETER;
    if (!*str_len) return ERROR_SUCCESS;
    if (!*str)
    {
        if (len && *str_len && (flags & (ICU_DECODE|ICU_ESCAPE))) return ERROR_INVALID_PARAMETER;
        *str = value;
        *str_len = len;
    }
    else
    {
        if (len >= *str_len)
        {
            *str_len = len+1;
            *overflow = TRUE;
            return ERROR_SUCCESS;
        }
        memcpy( *str, value, len * sizeof(WCHAR) );
        (*str)[len] = 0;
        *str_len = len;
    }
    return ERROR_SUCCESS;
}

static WCHAR *decode_url( LPCWSTR url, DWORD *len )
{
    const WCHAR *p = url;
    WCHAR hex[3], *q, *ret;

    if (!(ret = heap_alloc( *len * sizeof(WCHAR) ))) return NULL;
    q = ret;
    while (*len > 0)
    {
        if (p[0] == '%' && isxdigitW( p[1] ) && isxdigitW( p[2] ))
        {
            hex[0] = p[1];
            hex[1] = p[2];
            hex[2] = 0;
            *q++ = strtolW( hex, NULL, 16 );
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

static WCHAR *escape_url( LPCWSTR url, DWORD *len )
{
    WCHAR *ret;
    const WCHAR *p, *q;

    if ((p = q = strrchrW( url, '/' )))
    {
        while (*q)
        {
            if (need_escape( *q )) *len += 2;
            q++;
        }
    }
    if (!(ret = heap_alloc( (*len + 1) * sizeof(WCHAR) ))) return NULL;
    if (!p) strcpyW( ret, url );
    else
    {
        memcpy( ret, url, (p - url) * sizeof(WCHAR) );
        copy_escape( ret + (p - url), p, q - p );
    }
    return ret;
}

static DWORD parse_port( const WCHAR *str, DWORD len, INTERNET_PORT *ret )
{
    const WCHAR *p = str;
    DWORD port = 0;
    while (len && isdigitW( *p ))
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
BOOL WINAPI WinHttpCrackUrl( LPCWSTR url, DWORD len, DWORD flags, LPURL_COMPONENTSW uc )
{
    WCHAR *p, *q, *r, *url_decoded = NULL, *url_escaped = NULL;
    INTERNET_SCHEME scheme = 0;
    BOOL overflow = FALSE;
    DWORD err;

    TRACE("%s, %d, %x, %p\n", debugstr_wn(url, len), len, flags, uc);

    if (!url || !uc || uc->dwStructSize != sizeof(URL_COMPONENTS))
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!len) len = strlenW( url );

    if (flags & ICU_ESCAPE)
    {
        if (!(url_escaped = escape_url( url, &len )))
        {
            set_last_error( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        url = url_escaped;
    }
    else if (flags & ICU_DECODE)
    {
        if (!(url_decoded = decode_url( url, &len )))
        {
            set_last_error( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        url = url_decoded;
    }
    if (!(p = strchrW( url, ':' )))
    {
        set_last_error( ERROR_WINHTTP_UNRECOGNIZED_SCHEME );
        return FALSE;
    }
    if (p - url == 4 && !strncmpiW( url, scheme_http, 4 )) scheme = INTERNET_SCHEME_HTTP;
    else if (p - url == 5 && !strncmpiW( url, scheme_https, 5 )) scheme = INTERNET_SCHEME_HTTPS;
    else
    {
        err = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
        goto exit;
    }

    if ((err = set_component( &uc->lpszScheme, &uc->dwSchemeLength, (WCHAR *)url, p - url, flags, &overflow ))) goto exit;

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
    if ((q = memchrW( p, '@', len - (p - url) )) && !(memchrW( p, '/', q - p )))
    {
        if ((r = memchrW( p, ':', q - p )))
        {
            if ((err = set_component( &uc->lpszUserName, &uc->dwUserNameLength, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if ((err = set_component( &uc->lpszPassword, &uc->dwPasswordLength, r, q - r, flags, &overflow ))) goto exit;
        }
        else
        {
            if ((err = set_component( &uc->lpszUserName, &uc->dwUserNameLength, p, q - p, flags, &overflow ))) goto exit;
            if ((err = set_component( &uc->lpszPassword, &uc->dwPasswordLength, NULL, 0, flags, &overflow ))) goto exit;
        }
        p = q + 1;
    }
    else
    {
        if ((err = set_component( &uc->lpszUserName, &uc->dwUserNameLength, NULL, 0, flags, &overflow ))) goto exit;
        if ((err = set_component( &uc->lpszPassword, &uc->dwPasswordLength, NULL, 0, flags, &overflow ))) goto exit;
    }
    if ((q = memchrW( p, '/', len - (p - url) )))
    {
        if ((r = memchrW( p, ':', q - p )))
        {
            if ((err = set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if ((err = parse_port( r, q - r, &uc->nPort ))) goto exit;
        }
        else
        {
            if ((err = set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, q - p, flags, &overflow ))) goto exit;
            if (scheme == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            if (scheme == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }

        if ((r = memchrW( q, '?', len - (q - url) )))
        {
            if ((err = set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, q, r - q, flags, &overflow ))) goto exit;
            if ((err = set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, r, len - (r - url), flags, &overflow ))) goto exit;
        }
        else
        {
            if ((err = set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, q, len - (q - url), flags, &overflow ))) goto exit;
            if ((err = set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
        }
    }
    else
    {
        if ((r = memchrW( p, ':', len - (p - url) )))
        {
            if ((err = set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, r - p, flags, &overflow ))) goto exit;
            r++;
            if ((err = parse_port( r, len - (r - url), &uc->nPort ))) goto exit;
        }
        else
        {
            if ((err = set_component( &uc->lpszHostName, &uc->dwHostNameLength, p, len - (p - url), flags, &overflow ))) goto exit;
            if (scheme == INTERNET_SCHEME_HTTP) uc->nPort = INTERNET_DEFAULT_HTTP_PORT;
            if (scheme == INTERNET_SCHEME_HTTPS) uc->nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }
        if ((err = set_component( &uc->lpszUrlPath, &uc->dwUrlPathLength, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
        if ((err = set_component( &uc->lpszExtraInfo, &uc->dwExtraInfoLength, (WCHAR *)url + len, 0, flags, &overflow ))) goto exit;
    }

    TRACE("scheme(%s) host(%s) port(%d) path(%s) extra(%s)\n", debugstr_wn( uc->lpszScheme, uc->dwSchemeLength ),
          debugstr_wn( uc->lpszHostName, uc->dwHostNameLength ), uc->nPort, debugstr_wn( uc->lpszUrlPath, uc->dwUrlPathLength ),
          debugstr_wn( uc->lpszExtraInfo, uc->dwExtraInfoLength ));

exit:
    if (!err)
    {
        if (overflow) err = ERROR_INSUFFICIENT_BUFFER;
        uc->nScheme = scheme;
    }
    heap_free( url_decoded );
    heap_free( url_escaped );
    set_last_error( err );
    return !err;
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
    static const WCHAR formatW[] = {'%','u',0};
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

            *len += sprintfW( port, formatW, uc->nPort );
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
    static const WCHAR formatW[] = {'%','u',0};
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

    if (*required < len)
    {
        *required = len + 1;
        set_last_error( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (!url)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
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
            *url = ':';
            url++;

            url += sprintfW( url, formatW, uc->nPort );
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
    set_last_error( ERROR_SUCCESS );
    return TRUE;
}
