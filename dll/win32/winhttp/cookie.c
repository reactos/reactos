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
#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static domain_t *add_domain( session_t *session, WCHAR *name )
{
    domain_t *domain;

    if (!(domain = heap_alloc_zero( sizeof(domain_t) ))) return NULL;

    list_init( &domain->entry );
    list_init( &domain->cookies );

    domain->name = name;
    list_add_tail( &session->cookie_cache, &domain->entry );

    TRACE("%s\n", debugstr_w(domain->name));
    return domain;
}

static cookie_t *find_cookie( domain_t *domain, const WCHAR *path, const WCHAR *name )
{
    struct list *item;
    cookie_t *cookie;

    LIST_FOR_EACH( item, &domain->cookies )
    {
        cookie = LIST_ENTRY( item, cookie_t, entry );
        if (!strcmpW( cookie->path, path ) && !strcmpiW( cookie->name, name ))
        {
            TRACE("found %s=%s\n", debugstr_w(cookie->name), debugstr_w(cookie->value));
            return cookie;
         }
    }
    return NULL;
}

static BOOL domain_match( const WCHAR *name, domain_t *domain, BOOL partial )
{
    TRACE("comparing %s with %s\n", debugstr_w(name), debugstr_w(domain->name));

    if (partial && !strstrW( name, domain->name )) return FALSE;
    else if (!partial && strcmpW( name, domain->name )) return FALSE;
    return TRUE;
}

static void free_cookie( cookie_t *cookie )
{
    heap_free( cookie->name );
    heap_free( cookie->value );
    heap_free( cookie->path );
    heap_free( cookie );
}

static void delete_cookie( cookie_t *cookie )
{
    list_remove( &cookie->entry );
    free_cookie( cookie );
}

void delete_domain( domain_t *domain )
{
    cookie_t *cookie;
    struct list *item, *next;

    LIST_FOR_EACH_SAFE( item, next, &domain->cookies )
    {
        cookie = LIST_ENTRY( item, cookie_t, entry );
        delete_cookie( cookie );
    }

    list_remove( &domain->entry );
    heap_free( domain->name );
    heap_free( domain );
}

static BOOL add_cookie( session_t *session, cookie_t *cookie, WCHAR *domain_name, WCHAR *path )
{
    domain_t *domain = NULL;
    cookie_t *old_cookie;
    struct list *item;

    LIST_FOR_EACH( item, &session->cookie_cache )
    {
        domain = LIST_ENTRY( item, domain_t, entry );
        if (domain_match( domain_name, domain, FALSE )) break;
        domain = NULL;
    }
    if (!domain)
    {
        if (!(domain = add_domain( session, domain_name ))) return FALSE;
    }
    else if ((old_cookie = find_cookie( domain, path, cookie->name ))) delete_cookie( old_cookie );

    cookie->path = path;
    list_add_tail( &domain->cookies, &cookie->entry );

    TRACE("domain %s path %s <- %s=%s\n", debugstr_w(domain_name), debugstr_w(cookie->path),
          debugstr_w(cookie->name), debugstr_w(cookie->value));
    return TRUE;
}

static cookie_t *parse_cookie( const WCHAR *string )
{
    cookie_t *cookie;
    const WCHAR *p;
    int len;

    if (!(cookie = heap_alloc_zero( sizeof(cookie_t) ))) return NULL;

    list_init( &cookie->entry );

    if (!(p = strchrW( string, '=' )))
    {
        WARN("no '=' in %s\n", debugstr_w(string));
        return NULL;
    }
    if (p == string)
    {
        WARN("empty cookie name in %s\n", debugstr_w(string));
        return NULL;
    }
    len = p - string;
    if (!(cookie->name = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( cookie );
        return NULL;
    }
    memcpy( cookie->name, string, len * sizeof(WCHAR) );
    cookie->name[len] = 0;

    p++; /* skip '=' */
    while (*p == ' ') p++;

    len = strlenW( p );
    if (!(cookie->value = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        free_cookie( cookie );
        return NULL;
    }
    memcpy( cookie->value, p, len * sizeof(WCHAR) );
    cookie->value[len] = 0;

    return cookie;
}

BOOL set_cookies( request_t *request, const WCHAR *cookies )
{
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR domainW[] = {'d','o','m','a','i','n',0};

    BOOL ret = FALSE;
    WCHAR *buffer, *p, *q, *r;
    WCHAR *cookie_domain = NULL, *cookie_path = NULL;
    session_t *session = request->connect->session;
    cookie_t *cookie;
    int len;

    len = strlenW( cookies );
    if (!(buffer = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;
    strcpyW( buffer, cookies );

    p = buffer;
    while (*p && *p != ';') p++;
    if (*p == ';') *p++ = 0;
    if (!(cookie = parse_cookie( buffer )))
    {
        heap_free( buffer );
        return FALSE;
    }
    if ((q = strstrW( p, domainW ))) /* FIXME: do real attribute parsing */
    {
        while (*q && *q != '=') q++;
        if (!*q) goto end;

        r = ++q;
        while (*r && *r != ';') r++;
        len = r - q;

        if (!(cookie_domain = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
        memcpy( cookie_domain, q, len * sizeof(WCHAR) );
        cookie_domain[len] = 0;

    }
    if ((q = strstrW( p, pathW )))
    {
        while (*q && *q != '=') q++;
        if (!*q) goto end;

        r = ++q;
        while (*r && *r != ';') r++;
        len = r - q;

        if (!(cookie_path = heap_alloc( (len + 1) * sizeof(WCHAR) ))) goto end;
        memcpy( cookie_path, q, len * sizeof(WCHAR) );
        cookie_path[len] = 0;
    }
    if (!cookie_domain && !(cookie_domain = strdupW( request->connect->servername ))) goto end;
    if (!cookie_path && !(cookie_path = strdupW( request->path ))) goto end;

    if ((p = strrchrW( cookie_path, '/' )) && p != cookie_path) *p = 0;
    ret = add_cookie( session, cookie, cookie_domain, cookie_path );

end:
    if (!ret)
    {
        free_cookie( cookie );
        heap_free( cookie_domain );
        heap_free( cookie_path );
    }
    heap_free( buffer );
    return ret;
}

BOOL add_cookie_headers( request_t *request )
{
    struct list *domain_cursor;
    session_t *session = request->connect->session;

    LIST_FOR_EACH( domain_cursor, &session->cookie_cache )
    {
        domain_t *domain = LIST_ENTRY( domain_cursor, domain_t, entry );
        if (domain_match( request->connect->servername, domain, TRUE ))
        {
            struct list *cookie_cursor;
            TRACE("found domain %s\n", debugstr_w(domain->name));

            LIST_FOR_EACH( cookie_cursor, &domain->cookies )
            {
                cookie_t *cookie = LIST_ENTRY( cookie_cursor, cookie_t, entry );

                TRACE("comparing path %s with %s\n", debugstr_w(request->path), debugstr_w(cookie->path));

                if (strstrW( request->path, cookie->path ) == request->path)
                {
                    const WCHAR format[] = {'C','o','o','k','i','e',':',' ','%','s','=','%','s',0};
                    int len;
                    WCHAR *header;

                    len = strlenW( cookie->name ) + strlenW( format ) + strlenW( cookie->value );
                    if (!(header = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;

                    sprintfW( header, format, cookie->name, cookie->value );

                    TRACE("%s\n", debugstr_w(header));
                    add_request_headers( request, header, len, WINHTTP_ADDREQ_FLAG_ADD );
                    heap_free( header );
                }
            }
        }
    }
    return TRUE;
}
