/*
 * Wininet - cookie handling stuff
 *
 * Copyright 2002 TransGaming Technologies Inc.
 *
 * David Hammerton
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

#include "ws2tcpip.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "lmcons.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME
 *     Cookies could use A LOT OF MEMORY. We need some kind of memory management here!
 */

struct _cookie_domain_t;
struct _cookie_container_t;

typedef struct _cookie_t {
    struct list entry;

    struct _cookie_container_t *container;

    WCHAR *name;
    WCHAR *data;
    DWORD flags;
    FILETIME expiry;
    FILETIME create;
} cookie_t;

typedef struct _cookie_container_t {
    struct list entry;

    WCHAR *cookie_url;
    substr_t path;
    struct _cookie_domain_t *domain;

    struct list cookie_list;
} cookie_container_t;

typedef struct _cookie_domain_t {
    struct list entry;

    WCHAR *domain;
    unsigned subdomain_len;

    struct _cookie_domain_t *parent;
    struct list subdomain_list;

    /* List of stored paths sorted by length of the path. */
    struct list path_list;
} cookie_domain_t;

static CRITICAL_SECTION cookie_cs;
static CRITICAL_SECTION_DEBUG cookie_cs_debug =
{
    0, 0, &cookie_cs,
    { &cookie_cs_debug.ProcessLocksList, &cookie_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": cookie_cs") }
};
static CRITICAL_SECTION cookie_cs = { &cookie_cs_debug, -1, 0, 0, 0, 0 };
static struct list domain_list = LIST_INIT(domain_list);

static cookie_domain_t *get_cookie_domain(substr_t domain, BOOL create)
{
    const WCHAR *ptr = domain.str + domain.len, *ptr_end, *subdomain_ptr;
    cookie_domain_t *iter, *current_domain, *prev_domain = NULL;
    struct list *current_list = &domain_list;

    while(1) {
        for(ptr_end = ptr--; ptr > domain.str && *ptr != '.'; ptr--);
        subdomain_ptr = *ptr == '.' ? ptr+1 : ptr;

        current_domain = NULL;
        LIST_FOR_EACH_ENTRY(iter, current_list, cookie_domain_t, entry) {
            if(ptr_end-subdomain_ptr == iter->subdomain_len
                    && !memcmp(subdomain_ptr, iter->domain, iter->subdomain_len*sizeof(WCHAR))) {
                current_domain = iter;
                break;
            }
        }

        if(!current_domain) {
            if(!create)
                return prev_domain;

            current_domain = heap_alloc(sizeof(*current_domain));
            if(!current_domain)
                return NULL;

            current_domain->domain = heap_strndupW(subdomain_ptr, domain.str + domain.len - subdomain_ptr);
            if(!current_domain->domain) {
                heap_free(current_domain);
                return NULL;
            }

            current_domain->subdomain_len = ptr_end-subdomain_ptr;

            current_domain->parent = prev_domain;
            list_init(&current_domain->path_list);
            list_init(&current_domain->subdomain_list);

            list_add_tail(current_list, &current_domain->entry);
        }

        if(ptr == domain.str)
            return current_domain;

        prev_domain = current_domain;
        current_list = &current_domain->subdomain_list;
    }
}

static WCHAR *create_cookie_url(substr_t domain, substr_t path, substr_t *ret_path)
{
    WCHAR user[UNLEN], *p, *url;
    DWORD len, user_len, i;

    static const WCHAR cookie_prefix[] = {'C','o','o','k','i','e',':'};

    user_len = ARRAY_SIZE(user);
    if(!GetUserNameW(user, &user_len))
        return FALSE;
    user_len--;

    len = ARRAY_SIZE(cookie_prefix) + user_len + 1 /* @ */ + domain.len + path.len;
    url = heap_alloc((len+1) * sizeof(WCHAR));
    if(!url)
        return NULL;

    memcpy(url, cookie_prefix, sizeof(cookie_prefix));
    p = url + ARRAY_SIZE(cookie_prefix);

    memcpy(p, user, user_len*sizeof(WCHAR));
    p += user_len;

    *p++ = '@';

    memcpy(p, domain.str, domain.len*sizeof(WCHAR));
    p += domain.len;

    for(i=0; i < path.len; i++)
        p[i] = tolowerW(path.str[i]);
    p[path.len] = 0;

    ret_path->str = p;
    ret_path->len = path.len;
    return url;
}

static cookie_container_t *get_cookie_container(substr_t domain, substr_t path, BOOL create)
{
    cookie_domain_t *cookie_domain;
    cookie_container_t *cookie_container, *iter;

    cookie_domain = get_cookie_domain(domain, create);
    if(!cookie_domain)
        return NULL;

    LIST_FOR_EACH_ENTRY(cookie_container, &cookie_domain->path_list, cookie_container_t, entry) {
        if(cookie_container->path.len < path.len)
            break;

        if(path.len == cookie_container->path.len && !strncmpiW(cookie_container->path.str, path.str, path.len))
            return cookie_container;
    }

    if(!create)
        return NULL;

    cookie_container = heap_alloc(sizeof(*cookie_container));
    if(!cookie_container)
        return NULL;

    cookie_container->cookie_url = create_cookie_url(substrz(cookie_domain->domain), path, &cookie_container->path);
    if(!cookie_container->cookie_url) {
        heap_free(cookie_container);
        return NULL;
    }

    cookie_container->domain = cookie_domain;
    list_init(&cookie_container->cookie_list);

    LIST_FOR_EACH_ENTRY(iter, &cookie_domain->path_list, cookie_container_t, entry) {
        if(iter->path.len <= path.len) {
            list_add_before(&iter->entry, &cookie_container->entry);
            return cookie_container;
        }
    }

    list_add_tail(&cookie_domain->path_list, &cookie_container->entry);
    return cookie_container;
}

static void delete_cookie(cookie_t *cookie)
{
    list_remove(&cookie->entry);

    heap_free(cookie->name);
    heap_free(cookie->data);
    heap_free(cookie);
}

static cookie_t *alloc_cookie(substr_t name, substr_t data, FILETIME expiry, FILETIME create_time, DWORD flags)
{
    cookie_t *new_cookie;

    new_cookie = heap_alloc_zero(sizeof(*new_cookie));
    if(!new_cookie)
        return NULL;

    new_cookie->expiry = expiry;
    new_cookie->create = create_time;
    new_cookie->flags = flags;
    list_init(&new_cookie->entry);

    if(name.str && !(new_cookie->name = heap_strndupW(name.str, name.len))) {
        delete_cookie(new_cookie);
        return NULL;
    }

    if(data.str && !(new_cookie->data = heap_strndupW(data.str, data.len))) {
        delete_cookie(new_cookie);
        return NULL;
    }

    return new_cookie;
}

static cookie_t *find_cookie(cookie_container_t *container, substr_t name)
{
    cookie_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &container->cookie_list, cookie_t, entry) {
        if(strlenW(iter->name) == name.len && !strncmpiW(iter->name, name.str, name.len))
            return iter;
    }

    return NULL;
}

static void add_cookie(cookie_container_t *container, cookie_t *new_cookie)
{
    TRACE("Adding %s=%s to %s\n", debugstr_w(new_cookie->name), debugstr_w(new_cookie->data),
          debugstr_w(container->cookie_url));

    list_add_tail(&container->cookie_list, &new_cookie->entry);
    new_cookie->container = container;
}

static void replace_cookie(cookie_container_t *container, cookie_t *new_cookie)
{
    cookie_t *old_cookie;

    old_cookie = find_cookie(container, substrz(new_cookie->name));
    if(old_cookie)
        delete_cookie(old_cookie);

    add_cookie(container, new_cookie);
}

static BOOL cookie_match_path(cookie_container_t *container, substr_t path)
{
    return path.len >= container->path.len && !strncmpiW(container->path.str, path.str, container->path.len);
}

static BOOL load_persistent_cookie(substr_t domain, substr_t path)
{
    INTERNET_CACHE_ENTRY_INFOW *info;
    cookie_container_t *cookie_container;
    cookie_t *new_cookie;
    HANDLE cookie;
    char *str = NULL, *pbeg, *pend;
    DWORD size, flags;
    WCHAR *name, *data;
    FILETIME expiry, create, time;

    cookie_container = get_cookie_container(domain, path, TRUE);
    if(!cookie_container)
        return FALSE;

    size = 0;
    RetrieveUrlCacheEntryStreamW(cookie_container->cookie_url, NULL, &size, FALSE, 0);
    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return TRUE;
    info = heap_alloc(size);
    if(!info)
        return FALSE;
    cookie = RetrieveUrlCacheEntryStreamW(cookie_container->cookie_url, info, &size, FALSE, 0);
    size = info->dwSizeLow;
    heap_free(info);
    if(!cookie)
        return FALSE;

    if(!(str = heap_alloc(size+1)) || !ReadUrlCacheEntryStream(cookie, 0, str, &size, 0)) {
        UnlockUrlCacheEntryStream(cookie, 0);
        heap_free(str);
        return FALSE;
    }
    str[size] = 0;
    UnlockUrlCacheEntryStream(cookie, 0);

    GetSystemTimeAsFileTime(&time);
    for(pbeg=str; pbeg && *pbeg; name=data=NULL) {
        pend = strchr(pbeg, '\n');
        if(!pend)
            break;
        *pend = 0;
        name = heap_strdupAtoW(pbeg);

        pbeg = pend+1;
        pend = strchr(pbeg, '\n');
        if(!pend)
            break;
        *pend = 0;
        data = heap_strdupAtoW(pbeg);

        pbeg = strchr(pend+1, '\n');
        if(!pbeg)
            break;
        sscanf(pbeg, "%u %u %u %u %u", &flags, &expiry.dwLowDateTime, &expiry.dwHighDateTime,
                &create.dwLowDateTime, &create.dwHighDateTime);

        /* skip "*\n" */
        pbeg = strchr(pbeg, '*');
        if(pbeg) {
            pbeg++;
            if(*pbeg)
                pbeg++;
        }

        if(!name || !data)
            break;

        if(CompareFileTime(&time, &expiry) <= 0) {
            new_cookie = alloc_cookie(substr(NULL, 0), substr(NULL, 0), expiry, create, flags);
            if(!new_cookie)
                break;

            new_cookie->name = name;
            new_cookie->data = data;

            replace_cookie(cookie_container, new_cookie);
        }else {
            heap_free(name);
            heap_free(data);
        }
    }
    heap_free(str);
    heap_free(name);
    heap_free(data);

    return TRUE;
}

static BOOL save_persistent_cookie(cookie_container_t *container)
{
    static const WCHAR txtW[] = {'t','x','t',0};

    WCHAR cookie_file[MAX_PATH];
    HANDLE cookie_handle;
    cookie_t *cookie_container = NULL, *cookie_iter;
    BOOL do_save = FALSE;
    char buf[64], *dyn_buf;
    FILETIME time;
    DWORD bytes_written;
    size_t len;

    /* check if there's anything to save */
    GetSystemTimeAsFileTime(&time);
    LIST_FOR_EACH_ENTRY_SAFE(cookie_container, cookie_iter, &container->cookie_list, cookie_t, entry)
    {
        if((cookie_container->expiry.dwLowDateTime || cookie_container->expiry.dwHighDateTime)
                && CompareFileTime(&time, &cookie_container->expiry) > 0) {
            delete_cookie(cookie_container);
            continue;
        }

        if(!(cookie_container->flags & INTERNET_COOKIE_IS_SESSION)) {
            do_save = TRUE;
            break;
        }
    }

    if(!do_save) {
        DeleteUrlCacheEntryW(container->cookie_url);
        return TRUE;
    }

    if(!CreateUrlCacheEntryW(container->cookie_url, 0, txtW, cookie_file, 0))
        return FALSE;

    cookie_handle = CreateFileW(cookie_file, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if(cookie_handle == INVALID_HANDLE_VALUE) {
        DeleteFileW(cookie_file);
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY(cookie_container, &container->cookie_list, cookie_t, entry)
    {
        if(cookie_container->flags & INTERNET_COOKIE_IS_SESSION)
            continue;

        dyn_buf = heap_strdupWtoA(cookie_container->name);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &bytes_written, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);
        if(!WriteFile(cookie_handle, "\n", 1, &bytes_written, NULL)) {
            do_save = FALSE;
            break;
        }

        dyn_buf = heap_strdupWtoA(cookie_container->data);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &bytes_written, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);
        if(!WriteFile(cookie_handle, "\n", 1, &bytes_written, NULL)) {
            do_save = FALSE;
            break;
        }

        dyn_buf = heap_strdupWtoA(container->domain->domain);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &bytes_written, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);

        len = WideCharToMultiByte(CP_ACP, 0, container->path.str, container->path.len, NULL, 0, NULL, NULL);
        dyn_buf = heap_alloc(len+1);
        if(dyn_buf) {
            WideCharToMultiByte(CP_ACP, 0, container->path.str, container->path.len, dyn_buf, len, NULL, NULL);
            dyn_buf[len] = 0;
        }
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &bytes_written, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);

        sprintf(buf, "\n%u\n%u\n%u\n%u\n%u\n*\n", cookie_container->flags,
                cookie_container->expiry.dwLowDateTime, cookie_container->expiry.dwHighDateTime,
                cookie_container->create.dwLowDateTime, cookie_container->create.dwHighDateTime);
        if(!WriteFile(cookie_handle, buf, strlen(buf), &bytes_written, NULL)) {
            do_save = FALSE;
            break;
        }
    }

    CloseHandle(cookie_handle);
    if(!do_save) {
        ERR("error saving cookie file\n");
        DeleteFileW(cookie_file);
        return FALSE;
    }

    memset(&time, 0, sizeof(time));
    return CommitUrlCacheEntryW(container->cookie_url, cookie_file, time, time, 0, NULL, 0, txtW, 0);
}

static BOOL cookie_parse_url(const WCHAR *url, substr_t *host, substr_t *path)
{
    URL_COMPONENTSW comp = { sizeof(comp) };
    static const WCHAR rootW[] = {'/',0};

    comp.dwHostNameLength = 1;
    comp.dwUrlPathLength = 1;

    if(!InternetCrackUrlW(url, 0, 0, &comp) || !comp.dwHostNameLength)
        return FALSE;

    /* discard the webpage off the end of the path */
    while(comp.dwUrlPathLength && comp.lpszUrlPath[comp.dwUrlPathLength-1] != '/')
        comp.dwUrlPathLength--;

    *host = substr(comp.lpszHostName, comp.dwHostNameLength);
    *path = comp.dwUrlPathLength ? substr(comp.lpszUrlPath, comp.dwUrlPathLength) : substr(rootW, 1);
    return TRUE;
}

typedef struct {
    cookie_t **cookies;
    unsigned cnt;
    unsigned size;

    unsigned string_len;
} cookie_set_t;

static DWORD get_cookie(substr_t host, substr_t path, DWORD flags, cookie_set_t *res)
{
    static const WCHAR empty_path[] = { '/',0 };

    const WCHAR *p;
    cookie_domain_t *domain;
    cookie_container_t *container;
    FILETIME tm;

    GetSystemTimeAsFileTime(&tm);

    p = host.str + host.len;
    while(p > host.str && p[-1] != '.') p--;
    while(p != host.str) {
        p--;
        while(p > host.str && p[-1] != '.') p--;
        if(p == host.str) break;

        load_persistent_cookie(substr(p, host.str+host.len-p), substr(empty_path, 1));
    }

    p = path.str + path.len;
    do {
        load_persistent_cookie(host, substr(path.str, p-path.str));

        p--;
        while(p > path.str && p[-1] != '/') p--;
    }while(p != path.str);

    domain = get_cookie_domain(host, FALSE);
    if(!domain) {
        TRACE("Unknown host %s\n", debugstr_wn(host.str, host.len));
        return ERROR_NO_MORE_ITEMS;
    }

    for(domain = get_cookie_domain(host, FALSE); domain; domain = domain->parent) {
        LIST_FOR_EACH_ENTRY(container, &domain->path_list, cookie_container_t, entry) {
            struct list *cursor, *cursor2;

            if(!cookie_match_path(container, path))
                continue;

            LIST_FOR_EACH_SAFE(cursor, cursor2, &container->cookie_list) {
                cookie_t *cookie_iter = LIST_ENTRY(cursor, cookie_t, entry);

                /* check for expiry */
                if((cookie_iter->expiry.dwLowDateTime != 0 || cookie_iter->expiry.dwHighDateTime != 0)
                    && CompareFileTime(&tm, &cookie_iter->expiry)  > 0) {
                    TRACE("Found expired cookie. deleting\n");
                    delete_cookie(cookie_iter);
                    continue;
                }

                if((cookie_iter->flags & INTERNET_COOKIE_HTTPONLY) && !(flags & INTERNET_COOKIE_HTTPONLY))
                    continue;

                if(!res->size) {
                    res->cookies = heap_alloc(4*sizeof(*res->cookies));
                    if(!res->cookies)
                        continue;
                    res->size = 4;
                }else if(res->cnt == res->size) {
                    cookie_t **new_cookies = heap_realloc(res->cookies, res->size*2*sizeof(*res->cookies));
                    if(!new_cookies)
                        continue;
                    res->cookies = new_cookies;
                    res->size *= 2;
                }

                TRACE("%s = %s domain %s path %s\n", debugstr_w(cookie_iter->name), debugstr_w(cookie_iter->data),
                      debugstr_w(domain->domain), debugstr_wn(container->path.str, container->path.len));

                if(res->cnt)
                    res->string_len += 2; /* '; ' */
                res->cookies[res->cnt++] = cookie_iter;

                res->string_len += strlenW(cookie_iter->name);
                if(*cookie_iter->data)
                    res->string_len += 1 /* = */ + strlenW(cookie_iter->data);
            }
        }
    }

    return ERROR_SUCCESS;
}

static void cookie_set_to_string(const cookie_set_t *cookie_set, WCHAR *str)
{
    WCHAR *ptr = str;
    unsigned i, len;

    for(i=0; i<cookie_set->cnt; i++) {
        if(i) {
            *ptr++ = ';';
            *ptr++ = ' ';
        }

        len = strlenW(cookie_set->cookies[i]->name);
        memcpy(ptr, cookie_set->cookies[i]->name, len*sizeof(WCHAR));
        ptr += len;

        if(*cookie_set->cookies[i]->data) {
            *ptr++ = '=';
            len = strlenW(cookie_set->cookies[i]->data);
            memcpy(ptr, cookie_set->cookies[i]->data, len*sizeof(WCHAR));
            ptr += len;
        }
    }

    assert(ptr-str == cookie_set->string_len);
    TRACE("%s\n", debugstr_wn(str, ptr-str));
}

DWORD get_cookie_header(const WCHAR *host, const WCHAR *path, WCHAR **ret)
{
    cookie_set_t cookie_set = {0};
    DWORD res;

    static const WCHAR cookieW[] = {'C','o','o','k','i','e',':',' '};

    EnterCriticalSection(&cookie_cs);

    res = get_cookie(substrz(host), substrz(path), INTERNET_COOKIE_HTTPONLY, &cookie_set);
    if(res != ERROR_SUCCESS) {
        LeaveCriticalSection(&cookie_cs);
        return res;
    }

    if(cookie_set.cnt) {
        WCHAR *header, *ptr;

        ptr = header = heap_alloc(sizeof(cookieW) + (cookie_set.string_len + 3 /* crlf0 */) * sizeof(WCHAR));
        if(header) {
            memcpy(ptr, cookieW, sizeof(cookieW));
            ptr += ARRAY_SIZE(cookieW);

            cookie_set_to_string(&cookie_set, ptr);
            heap_free(cookie_set.cookies);
            ptr += cookie_set.string_len;

            *ptr++ = '\r';
            *ptr++ = '\n';
            *ptr++ = 0;

            *ret = header;
        }else {
            res = ERROR_NOT_ENOUGH_MEMORY;
        }
    }else {
        *ret = NULL;
    }

    LeaveCriticalSection(&cookie_cs);
    return res;
}

static void free_cookie_domain_list(struct list *list)
{
    cookie_container_t *container;
    cookie_domain_t *domain;

    while(!list_empty(list)) {
        domain = LIST_ENTRY(list_head(list), cookie_domain_t, entry);

        free_cookie_domain_list(&domain->subdomain_list);

        while(!list_empty(&domain->path_list)) {
            container = LIST_ENTRY(list_head(&domain->path_list), cookie_container_t, entry);

            while(!list_empty(&container->cookie_list))
                delete_cookie(LIST_ENTRY(list_head(&container->cookie_list), cookie_t, entry));

            heap_free(container->cookie_url);
            list_remove(&container->entry);
            heap_free(container);
        }

        heap_free(domain->domain);
        list_remove(&domain->entry);
        heap_free(domain);
    }
}

/***********************************************************************
 *           InternetGetCookieExW (WININET.@)
 *
 * Retrieve cookie from the specified url
 *
 *  It should be noted that on windows the lpszCookieName parameter is "not implemented".
 *    So it won't be implemented here.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieExW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
        LPWSTR lpCookieData, LPDWORD lpdwSize, DWORD flags, void *reserved)
{
    cookie_set_t cookie_set = {0};
    substr_t host, path;
    DWORD res;
    BOOL ret;

    TRACE("(%s, %s, %p, %p, %x, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName), lpCookieData, lpdwSize, flags, reserved);

    if (flags & ~INTERNET_COOKIE_HTTPONLY)
        FIXME("flags 0x%08x not supported\n", flags);

    if (!lpszUrl)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ret = cookie_parse_url(lpszUrl, &host, &path);
    if (!ret) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    EnterCriticalSection(&cookie_cs);

    res = get_cookie(host, path, flags, &cookie_set);
    if(res != ERROR_SUCCESS) {
        LeaveCriticalSection(&cookie_cs);
        SetLastError(res);
        return FALSE;
    }

    if(cookie_set.cnt) {
        if(!lpCookieData || cookie_set.string_len+1 > *lpdwSize) {
            *lpdwSize = (cookie_set.string_len + 1) * sizeof(WCHAR);
            TRACE("returning %u\n", *lpdwSize);
            if(lpCookieData) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
        }else {
            *lpdwSize = cookie_set.string_len + 1;
            cookie_set_to_string(&cookie_set, lpCookieData);
            lpCookieData[cookie_set.string_len] = 0;
        }
    }else {
        TRACE("no cookies found for %s\n", debugstr_wn(host.str, host.len));
        SetLastError(ERROR_NO_MORE_ITEMS);
        ret = FALSE;
    }

    heap_free(cookie_set.cookies);
    LeaveCriticalSection(&cookie_cs);
    return ret;
}

/***********************************************************************
 *           InternetGetCookieW (WININET.@)
 *
 * Retrieve cookie for the specified URL.
 */
BOOL WINAPI InternetGetCookieW(const WCHAR *url, const WCHAR *name, WCHAR *data, DWORD *size)
{
    TRACE("(%s, %s, %s, %p)\n", debugstr_w(url), debugstr_w(name), debugstr_w(data), size);

    return InternetGetCookieExW(url, name, data, size, 0, NULL);
}

/***********************************************************************
 *           InternetGetCookieExA (WININET.@)
 *
 * Retrieve cookie from the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieExA(LPCSTR lpszUrl, LPCSTR lpszCookieName,
        LPSTR lpCookieData, LPDWORD lpdwSize, DWORD flags, void *reserved)
{
    WCHAR *url, *name;
    DWORD len, size = 0;
    BOOL r;

    TRACE("(%s %s %p %p(%u) %x %p)\n", debugstr_a(lpszUrl), debugstr_a(lpszCookieName),
          lpCookieData, lpdwSize, lpdwSize ? *lpdwSize : 0, flags, reserved);

    url = heap_strdupAtoW(lpszUrl);
    name = heap_strdupAtoW(lpszCookieName);

    r = InternetGetCookieExW( url, name, NULL, &len, flags, reserved );
    if( r )
    {
        WCHAR *szCookieData;

        szCookieData = heap_alloc(len * sizeof(WCHAR));
        if( !szCookieData )
        {
            r = FALSE;
        }
        else
        {
            r = InternetGetCookieExW( url, name, szCookieData, &len, flags, reserved );

            if(r) {
                size = WideCharToMultiByte( CP_ACP, 0, szCookieData, len, NULL, 0, NULL, NULL);
                if(lpCookieData) {
                    if(*lpdwSize >= size) {
                        WideCharToMultiByte( CP_ACP, 0, szCookieData, len, lpCookieData, *lpdwSize, NULL, NULL);
                    }else {
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        r = FALSE;
                    }
                }
            }

            heap_free( szCookieData );
        }
    }
    *lpdwSize = size;
    heap_free( name );
    heap_free( url );
    return r;
}

/***********************************************************************
 *           InternetGetCookieA (WININET.@)
 *
 * See InternetGetCookieW.
 */
BOOL WINAPI InternetGetCookieA(const char *url, const char *name, char *data, DWORD *size)
{
    TRACE("(%s, %s, %p, %p)\n", debugstr_a(url), debugstr_a(name), data, size);

    return InternetGetCookieExA(url, name, data, size, 0, NULL);
}

static BOOL is_domain_legal_for_cookie(substr_t domain, substr_t full_domain)
{
    const WCHAR *ptr;

    if(!domain.len || *domain.str == '.' || !full_domain.len || *full_domain.str == '.') {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if(domain.len > full_domain.len || !memchrW(domain.str, '.', domain.len) || !memchrW(full_domain.str, '.', full_domain.len))
        return FALSE;

    ptr = full_domain.str + full_domain.len - domain.len;
    if (strncmpiW(domain.str, ptr, domain.len) || (full_domain.len > domain.len && ptr[-1] != '.')) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           IsDomainLegalCookieDomainW (WININET.@)
 */
BOOL WINAPI IsDomainLegalCookieDomainW(const WCHAR *domain, const WCHAR *full_domain)
{
    FIXME("(%s, %s) semi-stub\n", debugstr_w(domain), debugstr_w(full_domain));

    if (!domain || !full_domain) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return is_domain_legal_for_cookie(substrz(domain), substrz(full_domain));
}

static void substr_skip(substr_t *str, size_t len)
{
    assert(str->len >= len);
    str->str += len;
    str->len -= len;
}

DWORD set_cookie(substr_t domain, substr_t path, substr_t name, substr_t data, DWORD flags)
{
    cookie_container_t *container;
    cookie_t *thisCookie;
    substr_t value;
    const WCHAR *end_ptr;
    FILETIME expiry, create;
    BOOL expired = FALSE, update_persistent = FALSE;
    DWORD cookie_flags = 0, len;

    TRACE("%s %s %s=%s %x\n", debugstr_wn(domain.str, domain.len), debugstr_wn(path.str, path.len),
          debugstr_wn(name.str, name.len), debugstr_wn(data.str, data.len), flags);

    memset(&expiry,0,sizeof(expiry));
    GetSystemTimeAsFileTime(&create);

    /* lots of information can be parsed out of the cookie value */

    if(!(end_ptr = memchrW(data.str, ';', data.len)))
       end_ptr = data.str + data.len;
    value = substr(data.str, end_ptr-data.str);
    data.str += value.len;
    data.len -= value.len;

    for(;;) {
        static const WCHAR szDomain[] = {'d','o','m','a','i','n','='};
        static const WCHAR szPath[] = {'p','a','t','h','='};
        static const WCHAR szExpires[] = {'e','x','p','i','r','e','s','='};
        static const WCHAR szSecure[] = {'s','e','c','u','r','e'};
        static const WCHAR szHttpOnly[] = {'h','t','t','p','o','n','l','y'};
        static const WCHAR szVersion[] = {'v','e','r','s','i','o','n','='};
        static const WCHAR max_ageW[] = {'m','a','x','-','a','g','e','='};

        /* Skip ';' */
        if(data.len)
            substr_skip(&data, 1);

        while(data.len && *data.str == ' ')
            substr_skip(&data, 1);

        if(!data.len)
            break;

        if(!(end_ptr = memchrW(data.str, ';', data.len)))
            end_ptr = data.str + data.len;

        if(data.len >= (len = ARRAY_SIZE(szDomain)) && !strncmpiW(data.str, szDomain, len)) {
            substr_skip(&data, len);

            if(data.len && *data.str == '.')
                substr_skip(&data, 1);

            if(!is_domain_legal_for_cookie(substr(data.str, end_ptr-data.str), domain))
                return COOKIE_STATE_UNKNOWN;

            domain = substr(data.str, end_ptr-data.str);
            TRACE("Parsing new domain %s\n", debugstr_wn(domain.str, domain.len));
        }else if(data.len >= (len = ARRAY_SIZE(szPath)) && !strncmpiW(data.str, szPath, len)) {
            substr_skip(&data, len);
            path = substr(data.str, end_ptr - data.str);
            TRACE("Parsing new path %s\n", debugstr_wn(path.str, path.len));
        }else if(data.len >= (len = ARRAY_SIZE(szExpires)) && !strncmpiW(data.str, szExpires, len)) {
            SYSTEMTIME st;
            WCHAR buf[128];

            substr_skip(&data, len);

            if(end_ptr - data.str < ARRAY_SIZE(buf)-1) {
                memcpy(buf, data.str, data.len*sizeof(WCHAR));
                buf[data.len] = 0;

                if (InternetTimeToSystemTimeW(data.str, &st, 0)) {
                    SystemTimeToFileTime(&st, &expiry);

                    if (CompareFileTime(&create,&expiry) > 0) {
                        TRACE("Cookie already expired.\n");
                        expired = TRUE;
                    }
                }
            }
        }else if(data.len >= (len = ARRAY_SIZE(szSecure)) && !strncmpiW(data.str, szSecure, len)) {
            substr_skip(&data, len);
            FIXME("secure not handled\n");
        }else if(data.len >= (len = ARRAY_SIZE(szHttpOnly)) && !strncmpiW(data.str, szHttpOnly, len)) {
            substr_skip(&data, len);

            if(!(flags & INTERNET_COOKIE_HTTPONLY)) {
                WARN("HTTP only cookie added without INTERNET_COOKIE_HTTPONLY flag\n");
                SetLastError(ERROR_INVALID_OPERATION);
                return COOKIE_STATE_REJECT;
            }

            cookie_flags |= INTERNET_COOKIE_HTTPONLY;
        }else if(data.len >= (len = ARRAY_SIZE(szVersion)) && !strncmpiW(data.str, szVersion, len)) {
            substr_skip(&data, len);

            FIXME("version not handled (%s)\n",debugstr_wn(data.str, data.len));
        }else if(data.len >= (len = ARRAY_SIZE(max_ageW)) && !strncmpiW(data.str, max_ageW, len)) {
            /* Native doesn't support Max-Age attribute. */
            WARN("Max-Age ignored\n");
        }else if(data.len) {
            FIXME("Unknown additional option %s\n", debugstr_wn(data.str, data.len));
        }

        substr_skip(&data, end_ptr - data.str);
    }

    EnterCriticalSection(&cookie_cs);

    load_persistent_cookie(domain, path);

    container = get_cookie_container(domain, path, !expired);
    if(!container) {
        LeaveCriticalSection(&cookie_cs);
        return COOKIE_STATE_ACCEPT;
    }

    if(!expiry.dwLowDateTime && !expiry.dwHighDateTime)
        cookie_flags |= INTERNET_COOKIE_IS_SESSION;
    else
        update_persistent = TRUE;

    if ((thisCookie = find_cookie(container, name))) {
        if ((thisCookie->flags & INTERNET_COOKIE_HTTPONLY) && !(flags & INTERNET_COOKIE_HTTPONLY)) {
            WARN("An attempt to override httponly cookie\n");
            SetLastError(ERROR_INVALID_OPERATION);
            LeaveCriticalSection(&cookie_cs);
            return COOKIE_STATE_REJECT;
        }

        if (!(thisCookie->flags & INTERNET_COOKIE_IS_SESSION))
            update_persistent = TRUE;
        delete_cookie(thisCookie);
    }

    TRACE("setting cookie %s=%s for domain %s path %s\n", debugstr_wn(name.str, name.len),
          debugstr_wn(value.str, value.len), debugstr_w(container->domain->domain),
          debugstr_wn(container->path.str, container->path.len));

    if (!expired) {
        cookie_t *new_cookie;

        new_cookie = alloc_cookie(name, value, expiry, create, cookie_flags);
        if(!new_cookie) {
            LeaveCriticalSection(&cookie_cs);
            return COOKIE_STATE_UNKNOWN;
        }

        add_cookie(container, new_cookie);
    }

    if (!update_persistent || save_persistent_cookie(container))
    {
        LeaveCriticalSection(&cookie_cs);
        return COOKIE_STATE_ACCEPT;
    }
    LeaveCriticalSection(&cookie_cs);
    return COOKIE_STATE_UNKNOWN;
}

/***********************************************************************
 *           InternetSetCookieExW (WININET.@)
 *
 * Sets cookie for the specified url
 */
DWORD WINAPI InternetSetCookieExW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
        LPCWSTR lpCookieData, DWORD flags, DWORD_PTR reserved)
{
    substr_t host, path, name, data;
    BOOL ret;

    TRACE("(%s, %s, %s, %x, %lx)\n", debugstr_w(lpszUrl), debugstr_w(lpszCookieName),
          debugstr_w(lpCookieData), flags, reserved);

    if (flags & ~INTERNET_COOKIE_HTTPONLY)
        FIXME("flags %x not supported\n", flags);

    if (!lpszUrl || !lpCookieData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return COOKIE_STATE_UNKNOWN;
    }

    ret = cookie_parse_url(lpszUrl, &host, &path);
    if (!ret || !host.len) return COOKIE_STATE_UNKNOWN;

    if (!lpszCookieName) {
        const WCHAR *ptr;

        /* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name[=data].
         */
        if (!(ptr = strchrW(lpCookieData, '=')))
            ptr = lpCookieData + strlenW(lpCookieData);

        name = substr(lpCookieData, ptr - lpCookieData);
        data = substrz(*ptr == '=' ? ptr+1 : ptr);
    }else {
        name = substrz(lpszCookieName);
        data = substrz(lpCookieData);
    }

    return set_cookie(host, path, name, data, flags);
}

/***********************************************************************
 *           InternetSetCookieW (WININET.@)
 *
 * Sets a cookie for the specified URL.
 */
BOOL WINAPI InternetSetCookieW(const WCHAR *url, const WCHAR *name, const WCHAR *data)
{
    TRACE("(%s, %s, %s)\n", debugstr_w(url), debugstr_w(name), debugstr_w(data));

    return InternetSetCookieExW(url, name, data, 0, 0) == COOKIE_STATE_ACCEPT;
}

/***********************************************************************
 *           InternetSetCookieA (WININET.@)
 *
 * Sets cookie for the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetCookieA(LPCSTR lpszUrl, LPCSTR lpszCookieName,
    LPCSTR lpCookieData)
{
    LPWSTR data, url, name;
    BOOL r;

    TRACE("(%s,%s,%s)\n", debugstr_a(lpszUrl),
        debugstr_a(lpszCookieName), debugstr_a(lpCookieData));

    url = heap_strdupAtoW(lpszUrl);
    name = heap_strdupAtoW(lpszCookieName);
    data = heap_strdupAtoW(lpCookieData);

    r = InternetSetCookieW( url, name, data );

    heap_free( data );
    heap_free( name );
    heap_free( url );
    return r;
}

/***********************************************************************
 *           InternetSetCookieExA (WININET.@)
 *
 * See InternetSetCookieExW.
 */
DWORD WINAPI InternetSetCookieExA( LPCSTR lpszURL, LPCSTR lpszCookieName, LPCSTR lpszCookieData,
                                   DWORD dwFlags, DWORD_PTR dwReserved)
{
    WCHAR *data, *url, *name;
    DWORD r;

    TRACE("(%s, %s, %s, %x, %lx)\n", debugstr_a(lpszURL), debugstr_a(lpszCookieName),
          debugstr_a(lpszCookieData), dwFlags, dwReserved);

    url = heap_strdupAtoW(lpszURL);
    name = heap_strdupAtoW(lpszCookieName);
    data = heap_strdupAtoW(lpszCookieData);

    r = InternetSetCookieExW(url, name, data, dwFlags, dwReserved);

    heap_free( data );
    heap_free( name );
    heap_free( url );
    return r;
}

/***********************************************************************
 *           InternetClearAllPerSiteCookieDecisions (WININET.@)
 *
 * Clears all per-site decisions about cookies.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetClearAllPerSiteCookieDecisions( VOID )
{
    FIXME("stub\n");
    return TRUE;
}

/***********************************************************************
 *           InternetEnumPerSiteCookieDecisionA (WININET.@)
 *
 * See InternetEnumPerSiteCookieDecisionW.
 */
BOOL WINAPI InternetEnumPerSiteCookieDecisionA( LPSTR pszSiteName, ULONG *pcSiteNameSize,
                                                ULONG *pdwDecision, ULONG dwIndex )
{
    FIXME("(%s, %p, %p, 0x%08x) stub\n",
          debugstr_a(pszSiteName), pcSiteNameSize, pdwDecision, dwIndex);
    return FALSE;
}

/***********************************************************************
 *           InternetEnumPerSiteCookieDecisionW (WININET.@)
 *
 * Enumerates all per-site decisions about cookies.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetEnumPerSiteCookieDecisionW( LPWSTR pszSiteName, ULONG *pcSiteNameSize,
                                                ULONG *pdwDecision, ULONG dwIndex )
{
    FIXME("(%s, %p, %p, 0x%08x) stub\n",
          debugstr_w(pszSiteName), pcSiteNameSize, pdwDecision, dwIndex);
    return FALSE;
}

/***********************************************************************
 *           InternetGetPerSiteCookieDecisionA (WININET.@)
 */
BOOL WINAPI InternetGetPerSiteCookieDecisionA( LPCSTR pwchHostName, ULONG *pResult )
{
    FIXME("(%s, %p) stub\n", debugstr_a(pwchHostName), pResult);
    return FALSE;
}

/***********************************************************************
 *           InternetGetPerSiteCookieDecisionW (WININET.@)
 */
BOOL WINAPI InternetGetPerSiteCookieDecisionW( LPCWSTR pwchHostName, ULONG *pResult )
{
    FIXME("(%s, %p) stub\n", debugstr_w(pwchHostName), pResult);
    return FALSE;
}

/***********************************************************************
 *           InternetSetPerSiteCookieDecisionA (WININET.@)
 */
BOOL WINAPI InternetSetPerSiteCookieDecisionA( LPCSTR pchHostName, DWORD dwDecision )
{
    FIXME("(%s, 0x%08x) stub\n", debugstr_a(pchHostName), dwDecision);
    return FALSE;
}

/***********************************************************************
 *           InternetSetPerSiteCookieDecisionW (WININET.@)
 */
BOOL WINAPI InternetSetPerSiteCookieDecisionW( LPCWSTR pchHostName, DWORD dwDecision )
{
    FIXME("(%s, 0x%08x) stub\n", debugstr_w(pchHostName), dwDecision);
    return FALSE;
}

void free_cookie(void)
{
    EnterCriticalSection(&cookie_cs);

    free_cookie_domain_list(&domain_list);

    LeaveCriticalSection(&cookie_cs);
}
