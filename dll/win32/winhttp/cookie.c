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

#ifdef __REACTOS__
#include "wine/config.h"
#else
#include "config.h"
#endif
#include "ws2tcpip.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

struct cookie
{
    struct list entry;
    WCHAR *name;
    WCHAR *value;
    WCHAR *path;
};

struct domain
{
    struct list entry;
    WCHAR *name;
    struct list cookies;
};

static struct domain *add_domain( struct session *session, WCHAR *name )
{
    struct domain *domain;

    if (!(domain = heap_alloc_zero( sizeof(struct domain) ))) return NULL;

    list_init( &domain->entry );
    list_init( &domain->cookies );

    domain->name = strdupW( name );
    list_add_tail( &session->cookie_cache, &domain->entry );

    TRACE("%s\n", debugstr_w(domain->name));
    return domain;
}

static struct cookie *find_cookie( struct domain *domain, const WCHAR *path, const WCHAR *name )
{
    struct list *item;
    struct cookie *cookie;

    LIST_FOR_EACH( item, &domain->cookies )
    {
        cookie = LIST_ENTRY( item, struct cookie, entry );
        if (!strcmpW( cookie->path, path ) && !strcmpW( cookie->name, name ))
        {
            TRACE("found %s=%s\n", debugstr_w(cookie->name), debugstr_w(cookie->value));
            return cookie;
         }
    }
    return NULL;
}

static BOOL domain_match( const WCHAR *name, struct domain *domain, BOOL partial )
{
    TRACE("comparing %s with %s\n", debugstr_w(name), debugstr_w(domain->name));

    if (partial && !strstrW( name, domain->name )) return FALSE;
    else if (!partial && strcmpW( name, domain->name )) return FALSE;
    return TRUE;
}

static void free_cookie( struct cookie *cookie )
{
    heap_free( cookie->name );
    heap_free( cookie->value );
    heap_free( cookie->path );
    heap_free( cookie );
}

static void delete_cookie( struct cookie *cookie )
{
    list_remove( &cookie->entry );
    free_cookie( cookie );
}

static void delete_domain( struct domain *domain )
{
    struct cookie *cookie;
    struct list *item, *next;

    LIST_FOR_EACH_SAFE( item, next, &domain->cookies )
    {
        cookie = LIST_ENTRY( item, struct cookie, entry );
        delete_cookie( cookie );
    }

    list_remove( &domain->entry );
    heap_free( domain->name );
    heap_free( domain );
}

void destroy_cookies( struct session *session )
{
    struct list *item, *next;
    struct domain *domain;

    LIST_FOR_EACH_SAFE( item, next, &session->cookie_cache )
    {
        domain = LIST_ENTRY( item, struct domain, entry );
        delete_domain( domain );
    }
}

static BOOL add_cookie( struct session *session, struct cookie *cookie, WCHAR *domain_name, WCHAR *path )
{
    struct domain *domain = NULL;
    struct cookie *old_cookie;
    struct list *item;

    if (!(cookie->path = strdupW( path ))) return FALSE;

    EnterCriticalSection( &session->cs );

    LIST_FOR_EACH( item, &session->cookie_cache )
    {
        domain = LIST_ENTRY( item, struct domain, entry );
        if (domain_match( domain_name, domain, FALSE )) break;
        domain = NULL;
    }
    if (!domain) domain = add_domain( session, domain_name );
    else if ((old_cookie = find_cookie( domain, path, cookie->name ))) delete_cookie( old_cookie );

    if (domain)
    {
        list_add_head( &domain->cookies, &cookie->entry );
        TRACE("domain %s path %s <- %s=%s\n", debugstr_w(domain_name), debugstr_w(cookie->path),
              debugstr_w(cookie->name), debugstr_w(cookie->value));
    }

    LeaveCriticalSection( &session->cs );
    return domain != NULL;
}

static struct cookie *parse_cookie( const WCHAR *string )
{
    struct cookie *cookie;
    const WCHAR *p;
    int len;

    if (!(p = strchrW( string, '=' ))) p = string + strlenW( string );
    len = p - string;
    while (len && string[len - 1] == ' ') len--;
    if (!len) return NULL;

    if (!(cookie = heap_alloc_zero( sizeof(struct cookie) ))) return NULL;
    list_init( &cookie->entry );

    if (!(cookie->name = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( cookie );
        return NULL;
    }
    memcpy( cookie->name, string, len * sizeof(WCHAR) );
    cookie->name[len] = 0;

    if (*p++ == '=')
    {
        while (*p == ' ') p++;
        len = strlenW( p );
        while (len && p[len - 1] == ' ') len--;

        if (!(cookie->value = heap_alloc( (len + 1) * sizeof(WCHAR) )))
        {
            free_cookie( cookie );
            return NULL;
        }
        memcpy( cookie->value, p, len * sizeof(WCHAR) );
        cookie->value[len] = 0;
    }
    return cookie;
}

struct attr
{
    WCHAR *name;
    WCHAR *value;
};

static void free_attr( struct attr *attr )
{
    if (!attr) return;
    heap_free( attr->name );
    heap_free( attr->value );
    heap_free( attr );
}

static struct attr *parse_attr( const WCHAR *str, int *used )
{
    const WCHAR *p = str, *q;
    struct attr *attr;
    int len;

    while (*p == ' ') p++;
    q = p;
    while (*q && *q != ' ' && *q != '=' && *q != ';') q++;
    len = q - p;
    if (!len) return NULL;

    if (!(attr = heap_alloc( sizeof(struct attr) ))) return NULL;
    if (!(attr->name = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( attr );
        return NULL;
    }
    memcpy( attr->name, p, len * sizeof(WCHAR) );
    attr->name[len] = 0;
    attr->value = NULL;

    p = q;
    while (*p == ' ') p++;
    if (*p++ == '=')
    {
        while (*p == ' ') p++;
        q = p;
        while (*q && *q != ';') q++;
        len = q - p;
        while (len && p[len - 1] == ' ') len--;

        if (!(attr->value = heap_alloc( (len + 1) * sizeof(WCHAR) )))
        {
            free_attr( attr );
            return NULL;
        }
        memcpy( attr->value, p, len * sizeof(WCHAR) );
        attr->value[len] = 0;
    }

    while (*q == ' ') q++;
    if (*q == ';') q++;
    *used = q - str;

    return attr;
}

BOOL set_cookies( struct request *request, const WCHAR *cookies )
{
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR domainW[] = {'d','o','m','a','i','n',0};
    BOOL ret = FALSE;
    WCHAR *buffer, *p;
    WCHAR *cookie_domain = NULL, *cookie_path = NULL;
    struct attr *attr, *domain = NULL, *path = NULL;
    struct session *session = request->connect->session;
    struct cookie *cookie;
    int len, used;

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
    len = strlenW( p );
    while (len && (attr = parse_attr( p, &used )))
    {
        if (!strcmpiW( attr->name, domainW ))
        {
            domain = attr;
            cookie_domain = attr->value;
        }
        else if (!strcmpiW( attr->name, pathW ))
        {
            path = attr;
            cookie_path = attr->value;
        }
        else
        {
            FIXME( "unhandled attribute %s\n", debugstr_w(attr->name) );
            free_attr( attr );
        }
        len -= used;
        p += used;
    }
    if (!cookie_domain && !(cookie_domain = strdupW( request->connect->servername ))) goto end;
    if (!cookie_path && !(cookie_path = strdupW( request->path ))) goto end;

    if ((p = strrchrW( cookie_path, '/' )) && p != cookie_path) *p = 0;
    ret = add_cookie( session, cookie, cookie_domain, cookie_path );

end:
    if (!ret) free_cookie( cookie );
    if (domain) free_attr( domain );
    else heap_free( cookie_domain );
    if (path) free_attr( path );
    else heap_free( cookie_path );
    heap_free( buffer );
    return ret;
}

BOOL add_cookie_headers( struct request *request )
{
    struct list *domain_cursor;
    struct session *session = request->connect->session;

    EnterCriticalSection( &session->cs );

    LIST_FOR_EACH( domain_cursor, &session->cookie_cache )
    {
        struct domain *domain = LIST_ENTRY( domain_cursor, struct domain, entry );
        if (domain_match( request->connect->servername, domain, TRUE ))
        {
            struct list *cookie_cursor;
            TRACE("found domain %s\n", debugstr_w(domain->name));

            LIST_FOR_EACH( cookie_cursor, &domain->cookies )
            {
                struct cookie *cookie = LIST_ENTRY( cookie_cursor, struct cookie, entry );

                TRACE("comparing path %s with %s\n", debugstr_w(request->path), debugstr_w(cookie->path));

                if (strstrW( request->path, cookie->path ) == request->path)
                {
                    static const WCHAR cookieW[] = {'C','o','o','k','i','e',':',' '};
                    int len, len_cookie = ARRAY_SIZE( cookieW ), len_name = strlenW( cookie->name );
                    WCHAR *header;

                    len = len_cookie + len_name;
                    if (cookie->value) len += strlenW( cookie->value ) + 1;
                    if (!(header = heap_alloc( (len + 1) * sizeof(WCHAR) )))
                    {
                        LeaveCriticalSection( &session->cs );
                        return FALSE;
                    }

                    memcpy( header, cookieW, len_cookie * sizeof(WCHAR) );
                    strcpyW( header + len_cookie, cookie->name );
                    if (cookie->value)
                    {
                        header[len_cookie + len_name] = '=';
                        strcpyW( header + len_cookie + len_name + 1, cookie->value );
                    }

                    TRACE("%s\n", debugstr_w(header));
                    add_request_headers( request, header, len,
                                         WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON );
                    heap_free( header );
                }
            }
        }
    }

    LeaveCriticalSection( &session->cs );
    return TRUE;
}
