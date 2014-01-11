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

typedef struct _cookie_domain cookie_domain;
typedef struct _cookie cookie;

struct _cookie
{
    struct list entry;

    struct _cookie_domain *parent;

    LPWSTR lpCookieName;
    LPWSTR lpCookieData;
    DWORD flags;
    FILETIME expiry;
    FILETIME create;
};

struct _cookie_domain
{
    struct list entry;

    LPWSTR lpCookieDomain;
    LPWSTR lpCookiePath;
    struct list cookie_list;
};

static CRITICAL_SECTION cookie_cs;
static CRITICAL_SECTION_DEBUG cookie_cs_debug =
{
    0, 0, &cookie_cs,
    { &cookie_cs_debug.ProcessLocksList, &cookie_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": cookie_cs") }
};
static CRITICAL_SECTION cookie_cs = { &cookie_cs_debug, -1, 0, 0, 0, 0 };
static struct list domain_list = LIST_INIT(domain_list);

static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data,
        FILETIME expiry, FILETIME create, DWORD flags);
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName);
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain);
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path);
static void COOKIE_deleteDomain(cookie_domain *deadDomain);
static BOOL COOKIE_matchDomain(LPCWSTR lpszCookieDomain, LPCWSTR lpszCookiePath,
        cookie_domain *searchDomain, BOOL allow_partial);

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
    cookie_domain *domain_container = NULL;
    cookie *old_cookie;
    struct list *iter;
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

    LIST_FOR_EACH(iter, &domain_list)
    {
        domain_container = LIST_ENTRY(iter, cookie_domain, entry);
        if(COOKIE_matchDomain(domain, path, domain_container, FALSE))
            break;
        domain_container = NULL;
    }
    if(!domain_container)
        domain_container = COOKIE_addDomain(domain, path);
    if(!domain_container) {
        heap_free(str);
        return FALSE;
    }

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

        pbeg = pend+1;
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
            if((old_cookie = COOKIE_findCookie(domain_container, name)))
                COOKIE_deleteCookie(old_cookie, FALSE);
            COOKIE_addCookie(domain_container, name, data, expiry, create, flags);
        }
        heap_free(name);
        heap_free(data);
    }
    heap_free(str);
    heap_free(name);
    heap_free(data);

    return TRUE;
}

static BOOL save_persistent_cookie(cookie_domain *domain)
{
    static const WCHAR txtW[] = {'t','x','t',0};

    WCHAR cookie_url[MAX_PATH], cookie_file[MAX_PATH];
    HANDLE cookie_handle;
    cookie *cookie_container = NULL, *cookie_iter;
    BOOL do_save = FALSE;
    char buf[64], *dyn_buf;
    FILETIME time;
    DWORD dwBytesWritten;

    if (!create_cookie_url(domain->lpCookieDomain, domain->lpCookiePath, cookie_url, sizeof(cookie_url)/sizeof(cookie_url[0])))
        return FALSE;

    /* check if there's anything to save */
    GetSystemTimeAsFileTime(&time);
    LIST_FOR_EACH_ENTRY_SAFE(cookie_container, cookie_iter, &domain->cookie_list, cookie, entry)
    {
        if((cookie_container->expiry.dwLowDateTime || cookie_container->expiry.dwHighDateTime)
                && CompareFileTime(&time, &cookie_container->expiry) > 0) {
            COOKIE_deleteCookie(cookie_container, FALSE);
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

    LIST_FOR_EACH_ENTRY(cookie_container, &domain->cookie_list, cookie, entry)
    {
        if(cookie_container->flags & INTERNET_COOKIE_IS_SESSION)
            continue;

        dyn_buf = heap_strdupWtoA(cookie_container->lpCookieName);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &dwBytesWritten, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);
        if(!WriteFile(cookie_handle, "\n", 1, &dwBytesWritten, NULL)) {
            do_save = FALSE;
            break;
        }

        dyn_buf = heap_strdupWtoA(cookie_container->lpCookieData);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &dwBytesWritten, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);
        if(!WriteFile(cookie_handle, "\n", 1, &dwBytesWritten, NULL)) {
            do_save = FALSE;
            break;
        }

        dyn_buf = heap_strdupWtoA(domain->lpCookieDomain);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &dwBytesWritten, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);

        dyn_buf = heap_strdupWtoA(domain->lpCookiePath);
        if(!dyn_buf || !WriteFile(cookie_handle, dyn_buf, strlen(dyn_buf), &dwBytesWritten, NULL)) {
            heap_free(dyn_buf);
            do_save = FALSE;
            break;
        }
        heap_free(dyn_buf);

        sprintf(buf, "\n%u\n%u\n%u\n%u\n%u\n*\n", cookie_container->flags,
                cookie_container->expiry.dwLowDateTime, cookie_container->expiry.dwHighDateTime,
                cookie_container->create.dwLowDateTime, cookie_container->create.dwHighDateTime);
        if(!WriteFile(cookie_handle, buf, strlen(buf), &dwBytesWritten, NULL)) {
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

/* adds a cookie to the domain */
static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data,
        FILETIME expiry, FILETIME create, DWORD flags)
{
    cookie *newCookie = heap_alloc(sizeof(cookie));
    if (!newCookie)
        return NULL;

    newCookie->lpCookieName = heap_strdupW(name);
    newCookie->lpCookieData = heap_strdupW(data);

    if (!newCookie->lpCookieName || !newCookie->lpCookieData)
    {
        heap_free(newCookie->lpCookieName);
        heap_free(newCookie->lpCookieData);
        heap_free(newCookie);

        return NULL;
    }

    newCookie->flags = flags;
    newCookie->expiry = expiry;
    newCookie->create = create;

    TRACE("added cookie %p (data is %s)\n", newCookie, debugstr_w(data) );

    list_add_tail(&domain->cookie_list, &newCookie->entry);
    newCookie->parent = domain;
    return newCookie;
}


/* finds a cookie in the domain matching the cookie name */
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName)
{
    struct list * cursor;
    TRACE("(%p, %s)\n", domain, debugstr_w(lpszCookieName));

    LIST_FOR_EACH(cursor, &domain->cookie_list)
    {
        cookie *searchCookie = LIST_ENTRY(cursor, cookie, entry);
	BOOL candidate = TRUE;
	if (candidate && lpszCookieName)
	{
	    if (candidate && !searchCookie->lpCookieName)
		candidate = FALSE;
	    if (candidate && strcmpW(lpszCookieName, searchCookie->lpCookieName) != 0)
                candidate = FALSE;
	}
	if (candidate)
	    return searchCookie;
    }
    return NULL;
}

/* removes a cookie from the list, if its the last cookie we also remove the domain */
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain)
{
    heap_free(deadCookie->lpCookieName);
    heap_free(deadCookie->lpCookieData);
    list_remove(&deadCookie->entry);

    /* special case: last cookie, lets remove the domain to save memory */
    if (list_empty(&deadCookie->parent->cookie_list) && deleteDomain)
        COOKIE_deleteDomain(deadCookie->parent);
    heap_free(deadCookie);
}

/* allocates a domain and adds it to the end */
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path)
{
    cookie_domain *newDomain = heap_alloc(sizeof(cookie_domain));

    list_init(&newDomain->entry);
    list_init(&newDomain->cookie_list);
    newDomain->lpCookieDomain = heap_strdupW(domain);
    newDomain->lpCookiePath = heap_strdupW(path);

    list_add_tail(&domain_list, &newDomain->entry);

    TRACE("Adding domain: %p\n", newDomain);
    return newDomain;
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

/* match a domain. domain must match if the domain is not NULL. path must match if the path is not NULL */
static BOOL COOKIE_matchDomain(LPCWSTR lpszCookieDomain, LPCWSTR lpszCookiePath,
                               cookie_domain *searchDomain, BOOL allow_partial)
{
    TRACE("searching on domain %p\n", searchDomain);
	if (lpszCookieDomain)
	{
	    if (!searchDomain->lpCookieDomain)
            return FALSE;

	    TRACE("comparing domain %s with %s\n",
            debugstr_w(lpszCookieDomain),
            debugstr_w(searchDomain->lpCookieDomain));

        if (allow_partial && !strstrW(lpszCookieDomain, searchDomain->lpCookieDomain))
            return FALSE;
        else if (!allow_partial && lstrcmpW(lpszCookieDomain, searchDomain->lpCookieDomain) != 0)
            return FALSE;
 	}
    if (lpszCookiePath)
    {
        INT len;
        TRACE("comparing paths: %s with %s\n", debugstr_w(lpszCookiePath), debugstr_w(searchDomain->lpCookiePath));
        /* paths match at the beginning.  so a path of  /foo would match
         * /foobar and /foo/bar
         */
        if (!searchDomain->lpCookiePath)
            return FALSE;
        if (allow_partial)
        {
            len = lstrlenW(searchDomain->lpCookiePath);
            if (strncmpiW(searchDomain->lpCookiePath, lpszCookiePath, len)!=0)
                return FALSE;
        }
        else if (strcmpW(lpszCookiePath, searchDomain->lpCookiePath))
            return FALSE;

	}
	return TRUE;
}

/* remove a domain from the list and delete it */
static void COOKIE_deleteDomain(cookie_domain *deadDomain)
{
    struct list * cursor;
    while ((cursor = list_tail(&deadDomain->cookie_list)))
    {
        COOKIE_deleteCookie(LIST_ENTRY(cursor, cookie, entry), FALSE);
        list_remove(cursor);
    }
    heap_free(deadDomain->lpCookieDomain);
    heap_free(deadDomain->lpCookiePath);

    list_remove(&deadDomain->entry);

    heap_free(deadDomain);
}

DWORD get_cookie(const WCHAR *host, const WCHAR *path, WCHAR *cookie_data, DWORD *size)
{
    static const WCHAR empty_path[] = { '/',0 };

    unsigned cnt = 0, len, name_len, domain_count = 0, cookie_count = 0;
    WCHAR *ptr, subpath[INTERNET_MAX_PATH_LENGTH];
    const WCHAR *p;
    cookie_domain *domain;
    FILETIME tm;

    GetSystemTimeAsFileTime(&tm);

    EnterCriticalSection(&cookie_cs);

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

    ptr = cookie_data;
    LIST_FOR_EACH_ENTRY(domain, &domain_list, cookie_domain, entry) {
        struct list *cursor, *cursor2;

        if(!COOKIE_matchDomain(host, path, domain, TRUE))
            continue;

        domain_count++;
        TRACE("found domain %p\n", domain);

        LIST_FOR_EACH_SAFE(cursor, cursor2, &domain->cookie_list) {
            cookie *cookie_iter = LIST_ENTRY(cursor, cookie, entry);

            /* check for expiry */
            if((cookie_iter->expiry.dwLowDateTime != 0 || cookie_iter->expiry.dwHighDateTime != 0)
                && CompareFileTime(&tm, &cookie_iter->expiry)  > 0)
            {
                TRACE("Found expired cookie. deleting\n");
                COOKIE_deleteCookie(cookie_iter, FALSE);
                continue;
            }

            if (cookie_count)
                cnt += 2; /* '; ' */
            cnt += name_len = strlenW(cookie_iter->lpCookieName);
            if ((len = strlenW(cookie_iter->lpCookieData))) {
                cnt += 1; /* = */
                cnt += len;
            }

            if(ptr) {
                if(*size > cnt) {
                    if(cookie_count) {
                        *ptr++ = ';';
                        *ptr++ = ' ';
                    }

                    memcpy(ptr, cookie_iter->lpCookieName, name_len*sizeof(WCHAR));
                    ptr += name_len;

                    if(len) {
                        *ptr++ = '=';
                        memcpy(ptr, cookie_iter->lpCookieData, len*sizeof(WCHAR));
                        ptr += len;
                    }

                    assert(cookie_data+cnt == ptr);
                    TRACE("Cookie: %s\n", debugstr_wn(cookie_data, cnt));
                }else {
                    /* Stop writing data, just compute the size */
                    ptr = NULL;
                }
            }

            cookie_count++;
        }
    }

    LeaveCriticalSection(&cookie_cs);

    if(ptr)
        *ptr = 0;

    if (!cnt) {
        TRACE("no cookies found for %s\n", debugstr_w(host));
        return ERROR_NO_MORE_ITEMS;
    }

    if(!cookie_data || !ptr) {
        *size = (cnt + 1) * sizeof(WCHAR);
        TRACE("returning %u\n", *size);
        return cookie_data ? ERROR_INSUFFICIENT_BUFFER : ERROR_SUCCESS;
    }

    *size = cnt + 1;

    TRACE("Returning %u (from %u domains): %s\n", cnt, domain_count, debugstr_w(cookie_data));
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           InternetGetCookieW (WININET.@)
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
BOOL WINAPI InternetGetCookieW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
    LPWSTR lpCookieData, LPDWORD lpdwSize)
{
    WCHAR host[INTERNET_MAX_HOST_NAME_LENGTH], path[INTERNET_MAX_PATH_LENGTH];
    DWORD res;
    BOOL ret;

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName), lpCookieData, lpdwSize);

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

    res = get_cookie(host, path, lpCookieData, lpdwSize);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}


/***********************************************************************
 *           InternetGetCookieA (WININET.@)
 *
 * Retrieve cookie from the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieA(LPCSTR lpszUrl, LPCSTR lpszCookieName,
    LPSTR lpCookieData, LPDWORD lpdwSize)
{
    WCHAR *url, *name;
    DWORD len, size;
    BOOL r;

    TRACE("(%s %s %p %p(%u))\n", debugstr_a(lpszUrl), debugstr_a(lpszCookieName),
          lpCookieData, lpdwSize, lpdwSize ? *lpdwSize : 0);

    url = heap_strdupAtoW(lpszUrl);
    name = heap_strdupAtoW(lpszCookieName);

    r = InternetGetCookieW( url, name, NULL, &len );
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
            r = InternetGetCookieW( url, name, szCookieData, &len );

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

BOOL set_cookie(LPCWSTR domain, LPCWSTR path, LPCWSTR cookie_name, LPCWSTR cookie_data)
{
    cookie_domain *thisCookieDomain = NULL;
    cookie *thisCookie;
    struct list *cursor;
    LPWSTR data, value;
    WCHAR *ptr;
    FILETIME expiry, create;
    BOOL expired = FALSE, update_persistent = FALSE;
    DWORD flags = 0;

    value = data = heap_strdupW(cookie_data);
    if (!data)
    {
        ERR("could not allocate the cookie data buffer\n");
        return FALSE;
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
            return FALSE;
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
                return FALSE;
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
            FIXME("httponly not handled (%s)\n",debugstr_w(ptr));
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

    LIST_FOR_EACH(cursor, &domain_list)
    {
        thisCookieDomain = LIST_ENTRY(cursor, cookie_domain, entry);
        if (COOKIE_matchDomain(domain, path, thisCookieDomain, FALSE))
            break;
        thisCookieDomain = NULL;
    }

    if (!thisCookieDomain)
    {
        if (!expired)
            thisCookieDomain = COOKIE_addDomain(domain, path);
        else
        {
            heap_free(data);
            if (value != data) heap_free(value);
            LeaveCriticalSection(&cookie_cs);
            return TRUE;
        }
    }

    if(!expiry.dwLowDateTime && !expiry.dwHighDateTime)
        flags |= INTERNET_COOKIE_IS_SESSION;
    else
        update_persistent = TRUE;

    if ((thisCookie = COOKIE_findCookie(thisCookieDomain, cookie_name)))
    {
        if (!(thisCookie->flags & INTERNET_COOKIE_IS_SESSION))
            update_persistent = TRUE;
        COOKIE_deleteCookie(thisCookie, FALSE);
    }

    TRACE("setting cookie %s=%s for domain %s path %s\n", debugstr_w(cookie_name),
          debugstr_w(value), debugstr_w(thisCookieDomain->lpCookieDomain),debugstr_w(thisCookieDomain->lpCookiePath));

    if (!expired && !COOKIE_addCookie(thisCookieDomain, cookie_name, value, expiry, create, flags))
    {
        heap_free(data);
        if (value != data) heap_free(value);
        LeaveCriticalSection(&cookie_cs);
        return FALSE;
    }
    heap_free(data);
    if (value != data) heap_free(value);

    if (!update_persistent || save_persistent_cookie(thisCookieDomain))
    {
        LeaveCriticalSection(&cookie_cs);
        return TRUE;
    }
    LeaveCriticalSection(&cookie_cs);
    return FALSE;
}

/***********************************************************************
 *           InternetSetCookieW (WININET.@)
 *
 * Sets cookie for the specified url
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetCookieW(LPCWSTR lpszUrl, LPCWSTR lpszCookieName,
    LPCWSTR lpCookieData)
{
    BOOL ret;
    WCHAR hostName[INTERNET_MAX_HOST_NAME_LENGTH], path[INTERNET_MAX_PATH_LENGTH];

    TRACE("(%s,%s,%s)\n", debugstr_w(lpszUrl),
        debugstr_w(lpszCookieName), debugstr_w(lpCookieData));

    if (!lpszUrl || !lpCookieData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hostName[0] = 0;
    ret = COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));
    if (!ret || !hostName[0]) return FALSE;

    if (!lpszCookieName)
    {
        WCHAR *cookie, *data;

        cookie = heap_strdupW(lpCookieData);
        if (!cookie)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        /* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name[=data].
         */
        if (!(data = strchrW(cookie, '='))) data = cookie + strlenW(cookie);
        else *data++ = 0;

        ret = set_cookie(hostName, path, cookie, data);

        heap_free(cookie);
        return ret;
    }
    return set_cookie(hostName, path, lpszCookieName, lpCookieData);
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
    TRACE("(%s, %s, %s, 0x%08x, 0x%08lx)\n",
          debugstr_a(lpszURL), debugstr_a(lpszCookieName), debugstr_a(lpszCookieData),
          dwFlags, dwReserved);

    if (dwFlags) FIXME("flags 0x%08x not supported\n", dwFlags);
    return InternetSetCookieA(lpszURL, lpszCookieName, lpszCookieData);
}

/***********************************************************************
 *           InternetSetCookieExW (WININET.@)
 *
 * Sets a cookie for the specified URL.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
DWORD WINAPI InternetSetCookieExW( LPCWSTR lpszURL, LPCWSTR lpszCookieName, LPCWSTR lpszCookieData,
                                   DWORD dwFlags, DWORD_PTR dwReserved)
{
    TRACE("(%s, %s, %s, 0x%08x, 0x%08lx)\n",
          debugstr_w(lpszURL), debugstr_w(lpszCookieName), debugstr_w(lpszCookieData),
          dwFlags, dwReserved);

    if (dwFlags) FIXME("flags 0x%08x not supported\n", dwFlags);
    return InternetSetCookieW(lpszURL, lpszCookieName, lpszCookieData);
}

/***********************************************************************
 *           InternetGetCookieExA (WININET.@)
 *
 * See InternetGetCookieExW.
 */
BOOL WINAPI InternetGetCookieExA( LPCSTR pchURL, LPCSTR pchCookieName, LPSTR pchCookieData,
                                  LPDWORD pcchCookieData, DWORD dwFlags, LPVOID lpReserved)
{
    TRACE("(%s, %s, %s, %p, 0x%08x, %p)\n",
          debugstr_a(pchURL), debugstr_a(pchCookieName), debugstr_a(pchCookieData),
          pcchCookieData, dwFlags, lpReserved);

    if (dwFlags) FIXME("flags 0x%08x not supported\n", dwFlags);
    return InternetGetCookieA(pchURL, pchCookieName, pchCookieData, pcchCookieData);
}

/***********************************************************************
 *           InternetGetCookieExW (WININET.@)
 *
 * Retrieve cookie for the specified URL.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetCookieExW( LPCWSTR pchURL, LPCWSTR pchCookieName, LPWSTR pchCookieData,
                                  LPDWORD pcchCookieData, DWORD dwFlags, LPVOID lpReserved)
{
    TRACE("(%s, %s, %s, %p, 0x%08x, %p)\n",
          debugstr_w(pchURL), debugstr_w(pchCookieName), debugstr_w(pchCookieData),
          pcchCookieData, dwFlags, lpReserved);

    if (dwFlags) FIXME("flags 0x%08x not supported\n", dwFlags);
    return InternetGetCookieW(pchURL, pchCookieName, pchCookieData, pcchCookieData);
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
