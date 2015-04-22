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

#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */

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

    WCHAR *path;
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

static cookie_domain_t *get_cookie_domain(const WCHAR *domain, BOOL create)
{
    const WCHAR *ptr = domain + strlenW(domain), *ptr_end, *subdomain_ptr;
    cookie_domain_t *iter, *current_domain, *prev_domain = NULL;
    struct list *current_list = &domain_list;

    while(1) {
        for(ptr_end = ptr--; ptr > domain && *ptr != '.'; ptr--);
        subdomain_ptr = *ptr == '.' ? ptr+1 : ptr;

        current_domain = NULL;
        LIST_FOR_EACH_ENTRY(iter, current_list, cookie_domain_t, entry) {
            if(ptr_end-subdomain_ptr == iter->subdomain_len && !memcmp(subdomain_ptr, iter->domain, iter->subdomain_len)) {
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

            current_domain->domain = heap_strdupW(subdomain_ptr);
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

        if(ptr == domain)
            return current_domain;

        prev_domain = current_domain;
        current_list = &current_domain->subdomain_list;
    }
}

static cookie_container_t *get_cookie_container(const WCHAR *domain, const WCHAR *path, BOOL create)
{
    cookie_domain_t *cookie_domain;
    cookie_container_t *cookie_container, *iter;
    size_t path_len, len;

    cookie_domain = get_cookie_domain(domain, create);
    if(!cookie_domain)
        return NULL;

    path_len = strlenW(path);

    LIST_FOR_EACH_ENTRY(cookie_container, &cookie_domain->path_list, cookie_container_t, entry) {
        len = strlenW(cookie_container->path);
        if(len < path_len)
            break;

        if(!strcmpiW(cookie_container->path, path))
            return cookie_container;
    }

    if(!create)
        return NULL;

    cookie_container = heap_alloc(sizeof(*cookie_container));
    if(!cookie_container)
        return NULL;

    cookie_container->path = heap_strdupW(path);
    if(!cookie_container->path) {
        heap_free(cookie_container);
        return NULL;
    }

    cookie_container->domain = cookie_domain;
    list_init(&cookie_container->cookie_list);


    LIST_FOR_EACH_ENTRY(iter, &cookie_domain->path_list, cookie_container_t, entry) {
        if(strlenW(iter->path) <= path_len) {
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

static cookie_t *alloc_cookie(const WCHAR *name, const WCHAR *data, FILETIME expiry, FILETIME create_time, DWORD flags)
{
    cookie_t *new_cookie;

    new_cookie = heap_alloc(sizeof(*new_cookie));
    if(!new_cookie)
        return NULL;

    new_cookie->expiry = expiry;
    new_cookie->create = create_time;
    new_cookie->flags = flags;
    list_init(&new_cookie->entry);

    new_cookie->name = heap_strdupW(name);
    new_cookie->data = heap_strdupW(data);
    if((name && !new_cookie->name) || (data && !new_cookie->data)) {
        delete_cookie(new_cookie);
        return NULL;
    }

    return new_cookie;
}

static cookie_t *find_cookie(cookie_container_t *container, const WCHAR *name)
{
    cookie_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &container->cookie_list, cookie_t, entry) {
        if(!strcmpiW(iter->name, name))
            return iter;
    }

    return NULL;
}

static void add_cookie(cookie_container_t *container, cookie_t *new_cookie)
{
    TRACE("Adding %s=%s to %s %s\n", debugstr_w(new_cookie->name), debugstr_w(new_cookie->data),
          debugstr_w(container->domain->domain), debugstr_w(container->path));

    list_add_tail(&container->cookie_list, &new_cookie->entry);
    new_cookie->container = container;
}

static void replace_cookie(cookie_container_t *container, cookie_t *new_cookie)
{
    cookie_t *old_cookie;

    old_cookie = find_cookie(container, new_cookie->name);
    if(old_cookie)
        delete_cookie(old_cookie);

    add_cookie(container, new_cookie);
}

static BOOL cookie_match_path(cookie_container_t *container, const WCHAR *path)
{
    return !strncmpiW(container->path, path, strlenW(container->path));
}

static BOOL create_cookie_url(LPCWSTR domain, LPCWSTR path, WCHAR *buf, DWORD buf_len)
{
    static const WCHAR cookie_prefix[] = {'C','o','o','k','i','e',':'};

    WCHAR *p;
    DWORD len;

    if(buf_len < sizeof(cookie_prefix)/sizeof(WCHAR))
        return FALSE;
    memcpy(buf, cookie_prefix, sizeof(cookie_prefix));
    buf += sizeof(cookie_prefix)/sizeof(WCHAR);
    buf_len -= sizeof(cookie_prefix)/sizeof(WCHAR);
    p = buf;

    len = buf_len;
    if(!GetUserNameW(buf, &len))
        return FALSE;
    buf += len-1;
    buf_len -= len-1;

    if(!buf_len)
        return FALSE;
    *(buf++) = '@';
    buf_len--;

    len = strlenW(domain);
    if(len >= buf_len)
        return FALSE;
    memcpy(buf, domain, len*sizeof(WCHAR));
    buf += len;
    buf_len -= len;

    len = strlenW(path);
    if(len >= buf_len)
        return FALSE;
    memcpy(buf, path, len*sizeof(WCHAR));
    buf += len;

    *buf = 0;

    for(; *p; p++)
        *p = tolowerW(*p);
    return TRUE;
}

static BOOL load_persistent_cookie(LPCWSTR domain, LPCWSTR path)
{
    INTERNET_CACHE_ENTRY_INFOW *info;
    cookie_container_t *cookie_container;
    cookie_t *new_cookie;
    WCHAR cookie_url[MAX_PATH];
    HANDLE cookie;
    char *str = NULL, *pbeg, *pend;
    DWORD size, flags;
    WCHAR *name, *data;
    FILETIME expiry, create, time;

    if (!create_cookie_url(domain, path, cookie_url, sizeof(cookie_url)/sizeof(cookie_url[0])))
        return FALSE;

    size = 0;
    RetrieveUrlCacheEntryStreamW(cookie_url, NULL, &size, FALSE, 0);
    if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return TRUE;
    info = heap_alloc(size);
    if(!info)
        return FALSE;
    cookie = RetrieveUrlCacheEntryStreamW(cookie_url, info, &size, FALSE, 0);
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

    cookie_container = get_cookie_container(domain, path, TRUE);
    if(!cookie_container)
        return FALSE;

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
            new_cookie = alloc_cookie(NULL, NULL, expiry, create, flags);
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

    WCHAR cookie_url[MAX_PATH], cookie_file[MAX_PATH];
    HANDLE cookie_handle;
    cookie_t *cookie_container = NULL, *cookie_iter;
    BOOL do_save = FALSE;
    char buf[64], *dyn_buf;
    FILETIME time;
    DWORD bytes_written;

    if (!create_cookie_url(container->domain->domain, container->path, cookie_url, sizeof(cookie_url)/sizeof(cookie_url[0])))
        return FALSE;

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
        DeleteUrlCacheEntryW(cookie_url);
        return TRUE;
    }

    if(!CreateUrlCacheEntryW(cookie_url, 0, txtW, cookie_file, 0))
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

        dyn_buf = heap_strdupWtoA(container->path);
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
    return CommitUrlCacheEntryW(cookie_url, cookie_file, time, time, 0, NULL, 0, txtW, 0);
}

static BOOL COOKIE_crackUrlSimple(LPCWSTR lpszUrl, LPWSTR hostName, int hostNameLen, LPWSTR path, int pathLen)
{
    URL_COMPONENTSW UrlComponents;

    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.lpszScheme = NULL;
    UrlComponents.lpszUrlPath = path;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.lpszHostName = hostName;
    UrlComponents.dwExtraInfoLength = 0;
    UrlComponents.dwPasswordLength = 0;
    UrlComponents.dwSchemeLength = 0;
    UrlComponents.dwUserNameLength = 0;
    UrlComponents.dwHostNameLength = hostNameLen;
    UrlComponents.dwUrlPathLength = pathLen;

    if (!InternetCrackUrlW(lpszUrl, 0, 0, &UrlComponents)) return FALSE;

    /* discard the webpage off the end of the path */
    if (UrlComponents.dwUrlPathLength)
    {
        if (path[UrlComponents.dwUrlPathLength - 1] != '/')
        {
            WCHAR *ptr;
            if ((ptr = strrchrW(path, '/'))) *(++ptr) = 0;
            else
            {
                path[0] = '/';
                path[1] = 0;
            }
        }
    }
    else if (pathLen >= 2)
    {
        path[0] = '/';
        path[1] = 0;
    }
    return TRUE;
}

typedef struct {
    cookie_t **cookies;
    unsigned cnt;
    unsigned size;

    unsigned string_len;
} cookie_set_t;

static DWORD get_cookie(const WCHAR *host, const WCHAR *path, DWORD flags, cookie_set_t *res)
{
    static const WCHAR empty_path[] = { '/',0 };

    WCHAR *ptr, subpath[INTERNET_MAX_PATH_LENGTH];
    const WCHAR *p;
    cookie_domain_t *domain;
    cookie_container_t *container;
    unsigned len;
    FILETIME tm;

    GetSystemTimeAsFileTime(&tm);

    len = strlenW(host);
    p = host+len;
    while(p>host && p[-1]!='.') p--;
    while(p != host) {
        p--;
        while(p>host && p[-1]!='.') p--;
        if(p == host) break;

        load_persistent_cookie(p, empty_path);
    }

    len = strlenW(path);
    assert(len+1 < INTERNET_MAX_PATH_LENGTH);
    memcpy(subpath, path, (len+1)*sizeof(WCHAR));
    ptr = subpath+len;
    do {
        *ptr = 0;
        load_persistent_cookie(host, subpath);

        ptr--;
        while(ptr>subpath && ptr[-1]!='/') ptr--;
    }while(ptr != subpath);

    domain = get_cookie_domain(host, FALSE);
    if(!domain) {
        TRACE("Unknown host %s\n", debugstr_w(host));
        return ERROR_NO_MORE_ITEMS;
    }

    for(domain = get_cookie_domain(host, FALSE); domain; domain = domain->parent) {
        TRACE("Trying %s domain...\n", debugstr_w(domain->domain));

        LIST_FOR_EACH_ENTRY(container, &domain->path_list, cookie_container_t, entry) {
            struct list *cursor, *cursor2;

            TRACE("path %s\n", debugstr_w(container->path));

            if(!cookie_match_path(container, path))
                continue;

            TRACE("found domain %p\n", domain->domain);

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

    res = get_cookie(host, path, INTERNET_COOKIE_HTTPONLY, &cookie_set);
    if(res != ERROR_SUCCESS) {
        LeaveCriticalSection(&cookie_cs);
        return res;
    }

    if(cookie_set.cnt) {
        WCHAR *header, *ptr;

        ptr = header = heap_alloc(sizeof(cookieW) + (cookie_set.string_len + 3 /* crlf0 */) * sizeof(WCHAR));
        if(header) {
            memcpy(ptr, cookieW, sizeof(cookieW));
            ptr += sizeof(cookieW)/sizeof(*cookieW);

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
    return ERROR_SUCCESS;
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
    WCHAR host[INTERNET_MAX_HOST_NAME_LENGTH], path[INTERNET_MAX_PATH_LENGTH];
    cookie_set_t cookie_set = {0};
    DWORD res;
    BOOL ret;

    TRACE("(%s, %s, %p, %p, %x, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName), lpCookieData, lpdwSize, flags, reserved);

    if (flags)
        FIXME("flags 0x%08x not supported\n", flags);

    if (!lpszUrl)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    host[0] = 0;
    ret = COOKIE_crackUrlSimple(lpszUrl, host, sizeof(host)/sizeof(host[0]), path, sizeof(path)/sizeof(path[0]));
    if (!ret || !host[0]) {
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
        TRACE("no cookies found for %s\n", debugstr_w(host));
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
    DWORD len, size;
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
                *lpdwSize = size;
            }

            heap_free( szCookieData );
        }
    }
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
    TRACE("(%s, %s, %s, %p)\n", debugstr_a(url), debugstr_a(name), debugstr_a(data), size);

    return InternetGetCookieExA(url, name, data, size, 0, NULL);
}

/***********************************************************************
 *           IsDomainLegalCookieDomainW (WININET.@)
 */
BOOL WINAPI IsDomainLegalCookieDomainW( LPCWSTR s1, LPCWSTR s2 )
{
    DWORD s1_len, s2_len;

    FIXME("(%s, %s) semi-stub\n", debugstr_w(s1), debugstr_w(s2));

    if (!s1 || !s2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (s1[0] == '.' || !s1[0] || s2[0] == '.' || !s2[0])
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    if(!strchrW(s1, '.') || !strchrW(s2, '.'))
        return FALSE;

    s1_len = strlenW(s1);
    s2_len = strlenW(s2);
    if (s1_len > s2_len)
        return FALSE;

    if (strncmpiW(s1, s2+s2_len-s1_len, s1_len) || (s2_len>s1_len && s2[s2_len-s1_len-1]!='.'))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

DWORD set_cookie(const WCHAR *domain, const WCHAR *path, const WCHAR *cookie_name, const WCHAR *cookie_data, DWORD flags)
{
    cookie_container_t *container;
    cookie_t *thisCookie;
    LPWSTR data, value;
    WCHAR *ptr;
    FILETIME expiry, create;
    BOOL expired = FALSE, update_persistent = FALSE;
    DWORD cookie_flags = 0;

    TRACE("%s %s %s=%s %x\n", debugstr_w(domain), debugstr_w(path), debugstr_w(cookie_name), debugstr_w(cookie_data), flags);

    value = data = heap_strdupW(cookie_data);
    if (!data)
    {
        ERR("could not allocate the cookie data buffer\n");
        return COOKIE_STATE_UNKNOWN;
    }

    memset(&expiry,0,sizeof(expiry));
    GetSystemTimeAsFileTime(&create);

    /* lots of information can be parsed out of the cookie value */

    ptr = data;
    for (;;)
    {
        static const WCHAR szDomain[] = {'d','o','m','a','i','n','=',0};
        static const WCHAR szPath[] = {'p','a','t','h','=',0};
        static const WCHAR szExpires[] = {'e','x','p','i','r','e','s','=',0};
        static const WCHAR szSecure[] = {'s','e','c','u','r','e',0};
        static const WCHAR szHttpOnly[] = {'h','t','t','p','o','n','l','y',0};

        if (!(ptr = strchrW(ptr,';'))) break;
        *ptr++ = 0;

        if (value != data) heap_free(value);
        value = heap_alloc((ptr - data) * sizeof(WCHAR));
        if (value == NULL)
        {
            heap_free(data);
            ERR("could not allocate the cookie value buffer\n");
            return COOKIE_STATE_UNKNOWN;
        }
        strcpyW(value, data);

        while (*ptr == ' ') ptr++; /* whitespace */

        if (strncmpiW(ptr, szDomain, 7) == 0)
        {
            WCHAR *end_ptr;

            ptr += sizeof(szDomain)/sizeof(szDomain[0])-1;
            if(*ptr == '.')
                ptr++;
            end_ptr = strchrW(ptr, ';');
            if(end_ptr)
                *end_ptr = 0;

            if(!IsDomainLegalCookieDomainW(ptr, domain))
            {
                if(value != data)
                    heap_free(value);
                heap_free(data);
                return COOKIE_STATE_UNKNOWN;
            }

            if(end_ptr)
                *end_ptr = ';';

            domain = ptr;
            TRACE("Parsing new domain %s\n",debugstr_w(domain));
        }
        else if (strncmpiW(ptr, szPath, 5) == 0)
        {
            ptr+=strlenW(szPath);
            path = ptr;
            TRACE("Parsing new path %s\n",debugstr_w(path));
        }
        else if (strncmpiW(ptr, szExpires, 8) == 0)
        {
            SYSTEMTIME st;
            ptr+=strlenW(szExpires);
            if (InternetTimeToSystemTimeW(ptr, &st, 0))
            {
                SystemTimeToFileTime(&st, &expiry);

                if (CompareFileTime(&create,&expiry) > 0)
                {
                    TRACE("Cookie already expired.\n");
                    expired = TRUE;
                }
            }
        }
        else if (strncmpiW(ptr, szSecure, 6) == 0)
        {
            FIXME("secure not handled (%s)\n",debugstr_w(ptr));
            ptr += strlenW(szSecure);
        }
        else if (strncmpiW(ptr, szHttpOnly, 8) == 0)
        {
            if(!(flags & INTERNET_COOKIE_HTTPONLY)) {
                WARN("HTTP only cookie added without INTERNET_COOKIE_HTTPONLY flag\n");
                heap_free(data);
                if (value != data) heap_free(value);
                SetLastError(ERROR_INVALID_OPERATION);
                return COOKIE_STATE_REJECT;
            }

            cookie_flags |= INTERNET_COOKIE_HTTPONLY;
            ptr += strlenW(szHttpOnly);
        }
        else if (*ptr)
        {
            FIXME("Unknown additional option %s\n",debugstr_w(ptr));
            break;
        }
    }

    EnterCriticalSection(&cookie_cs);

    load_persistent_cookie(domain, path);

    container = get_cookie_container(domain, path, !expired);
    if(!container) {
        heap_free(data);
        if (value != data) heap_free(value);
        LeaveCriticalSection(&cookie_cs);
        return COOKIE_STATE_ACCEPT;
    }

    if(!expiry.dwLowDateTime && !expiry.dwHighDateTime)
        cookie_flags |= INTERNET_COOKIE_IS_SESSION;
    else
        update_persistent = TRUE;

    if ((thisCookie = find_cookie(container, cookie_name)))
    {
        if ((thisCookie->flags & INTERNET_COOKIE_HTTPONLY) && !(flags & INTERNET_COOKIE_HTTPONLY)) {
            WARN("An attempt to override httponly cookie\n");
            SetLastError(ERROR_INVALID_OPERATION);
            heap_free(data);
            if (value != data) heap_free(value);
            return COOKIE_STATE_REJECT;
        }

        if (!(thisCookie->flags & INTERNET_COOKIE_IS_SESSION))
            update_persistent = TRUE;
        delete_cookie(thisCookie);
    }

    TRACE("setting cookie %s=%s for domain %s path %s\n", debugstr_w(cookie_name),
          debugstr_w(value), debugstr_w(container->domain->domain),debugstr_w(container->path));

    if (!expired) {
        cookie_t *new_cookie;

        new_cookie = alloc_cookie(cookie_name, value, expiry, create, cookie_flags);
        if(!new_cookie) {
            heap_free(data);
            if (value != data) heap_free(value);
            LeaveCriticalSection(&cookie_cs);
            return COOKIE_STATE_UNKNOWN;
        }

        add_cookie(container, new_cookie);
    }
    heap_free(data);
    if (value != data) heap_free(value);

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
    BOOL ret;
    WCHAR hostName[INTERNET_MAX_HOST_NAME_LENGTH], path[INTERNET_MAX_PATH_LENGTH];

    TRACE("(%s, %s, %s, %x, %lx)\n", debugstr_w(lpszUrl), debugstr_w(lpszCookieName),
          debugstr_w(lpCookieData), flags, reserved);

    if (flags & ~INTERNET_COOKIE_HTTPONLY)
        FIXME("flags %x not supported\n", flags);

    if (!lpszUrl || !lpCookieData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return COOKIE_STATE_UNKNOWN;
    }

    hostName[0] = 0;
    ret = COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));
    if (!ret || !hostName[0]) return COOKIE_STATE_UNKNOWN;

    if (!lpszCookieName)
    {
        WCHAR *cookie, *data;
        DWORD res;

        cookie = heap_strdupW(lpCookieData);
        if (!cookie)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return COOKIE_STATE_UNKNOWN;
        }

        /* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name[=data].
         */
        if (!(data = strchrW(cookie, '='))) data = cookie + strlenW(cookie);
        else *data++ = 0;

        res = set_cookie(hostName, path, cookie, data, flags);

        heap_free(cookie);
        return res;
    }
    return set_cookie(hostName, path, lpszCookieName, lpCookieData, flags);
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
    DeleteCriticalSection(&cookie_cs);
}
