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

#include "config.h"
#include "wine/port.h"

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME
 *     Cookies are currently memory only.
 *     Cookies are NOT THREAD SAFE
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
    FILETIME expiry;
};

struct _cookie_domain
{
    struct list entry;

    LPWSTR lpCookieDomain;
    LPWSTR lpCookiePath;
    struct list cookie_list;
};

static struct list domain_list = LIST_INIT(domain_list);

static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data, FILETIME expiry);
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName);
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain);
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path);
static void COOKIE_deleteDomain(cookie_domain *deadDomain);


/* adds a cookie to the domain */
static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data, FILETIME expiry)
{
    cookie *newCookie = HeapAlloc(GetProcessHeap(), 0, sizeof(cookie));

    list_init(&newCookie->entry);
    newCookie->lpCookieName = NULL;
    newCookie->lpCookieData = NULL;
    newCookie->expiry = expiry;

    if (name)
    {
	newCookie->lpCookieName = HeapAlloc(GetProcessHeap(), 0, (strlenW(name) + 1)*sizeof(WCHAR));
        lstrcpyW(newCookie->lpCookieName, name);
    }
    if (data)
    {
	newCookie->lpCookieData = HeapAlloc(GetProcessHeap(), 0, (strlenW(data) + 1)*sizeof(WCHAR));
        lstrcpyW(newCookie->lpCookieData, data);
    }

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
    HeapFree(GetProcessHeap(), 0, deadCookie->lpCookieName);
    HeapFree(GetProcessHeap(), 0, deadCookie->lpCookieData);
    list_remove(&deadCookie->entry);

    /* special case: last cookie, lets remove the domain to save memory */
    if (list_empty(&deadCookie->parent->cookie_list) && deleteDomain)
        COOKIE_deleteDomain(deadCookie->parent);
    HeapFree(GetProcessHeap(), 0, deadCookie);
}

/* allocates a domain and adds it to the end */
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path)
{
    cookie_domain *newDomain = HeapAlloc(GetProcessHeap(), 0, sizeof(cookie_domain));

    list_init(&newDomain->entry);
    list_init(&newDomain->cookie_list);
    newDomain->lpCookieDomain = NULL;
    newDomain->lpCookiePath = NULL;

    if (domain)
    {
	newDomain->lpCookieDomain = HeapAlloc(GetProcessHeap(), 0, (strlenW(domain) + 1)*sizeof(WCHAR));
        strcpyW(newDomain->lpCookieDomain, domain);
    }
    if (path)
    {
	newDomain->lpCookiePath = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 1)*sizeof(WCHAR));
        lstrcpyW(newDomain->lpCookiePath, path);
    }

    list_add_tail(&domain_list, &newDomain->entry);

    TRACE("Adding domain: %p\n", newDomain);
    return newDomain;
}

static BOOL COOKIE_crackUrlSimple(LPCWSTR lpszUrl, LPWSTR hostName, int hostNameLen, LPWSTR path, int pathLen)
{
    URL_COMPONENTSW UrlComponents;
    BOOL rc;

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

    rc = InternetCrackUrlW(lpszUrl, 0, 0, &UrlComponents);

    /* discard the webpage off the end of the path */
    if (pathLen > 0 && path[pathLen-1] != '/')
    {
        LPWSTR ptr;
        ptr = strrchrW(path,'/');
        if (ptr)
            *(++ptr) = 0;
        else
        {
            path[0] = '/';
            path[1] = 0;
        }
    }
    return rc;
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

    HeapFree(GetProcessHeap(), 0, deadDomain->lpCookieDomain);
    HeapFree(GetProcessHeap(), 0, deadDomain->lpCookiePath);

    list_remove(&deadDomain->entry);

    HeapFree(GetProcessHeap(), 0, deadDomain);
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
    BOOL ret;
    struct list * cursor;
    unsigned int cnt = 0, domain_count = 0, cookie_count = 0;
    WCHAR hostName[2048], path[2048];
    FILETIME tm;

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName),
          lpCookieData, lpdwSize);

    if (!lpszUrl)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hostName[0] = 0;
    ret = COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));
    if (!ret || !hostName[0]) return FALSE;

    GetSystemTimeAsFileTime(&tm);

    LIST_FOR_EACH(cursor, &domain_list)
    {
        cookie_domain *cookiesDomain = LIST_ENTRY(cursor, cookie_domain, entry);
        if (COOKIE_matchDomain(hostName, path, cookiesDomain, TRUE))
        {
            struct list * cursor;
            domain_count++;
            TRACE("found domain %p\n", cookiesDomain);
    
            LIST_FOR_EACH(cursor, &cookiesDomain->cookie_list)
            {
                cookie *thisCookie = LIST_ENTRY(cursor, cookie, entry);
                /* check for expiry */
                if ((thisCookie->expiry.dwLowDateTime != 0 || thisCookie->expiry.dwHighDateTime != 0) && CompareFileTime(&tm,&thisCookie->expiry)  > 0)
                {
                    TRACE("Found expired cookie. deleting\n");
                    COOKIE_deleteCookie(thisCookie, FALSE);
                    continue;
                }

                if (lpCookieData == NULL) /* return the size of the buffer required to lpdwSize */
                {
                    unsigned int len;

                    if (cookie_count) cnt += 2; /* '; ' */
                    cnt += strlenW(thisCookie->lpCookieName);
                    if ((len = strlenW(thisCookie->lpCookieData)))
                    {
                        cnt += 1; /* = */
                        cnt += len;
                    }
                }
                else
                {
                    static const WCHAR szsc[] = { ';',' ',0 };
                    static const WCHAR szname[] = { '%','s',0 };
                    static const WCHAR szdata[] = { '=','%','s',0 };

                    if (cookie_count) cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szsc);
                    cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szname, thisCookie->lpCookieName);

                    if (thisCookie->lpCookieData[0])
                        cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szdata, thisCookie->lpCookieData);

                    TRACE("Cookie: %s\n", debugstr_w(lpCookieData));
                }
                cookie_count++;
            }
        }
    }

    if (!domain_count)
    {
        TRACE("no cookies found for %s\n", debugstr_w(hostName));
        SetLastError(ERROR_NO_MORE_ITEMS);
        return FALSE;
    }

    if (lpCookieData == NULL)
    {
        *lpdwSize = (cnt + 1) * sizeof(WCHAR);
        TRACE("returning %u\n", *lpdwSize);
        return TRUE;
    }

    *lpdwSize = cnt + 1;

    TRACE("Returning %u (from %u domains): %s\n", cnt, domain_count,
           debugstr_w(lpCookieData));

    return (cnt ? TRUE : FALSE);
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
    DWORD len;
    LPWSTR szCookieData = NULL, szUrl = NULL, szCookieName = NULL;
    BOOL r;

    TRACE("(%s,%s,%p)\n", debugstr_a(lpszUrl), debugstr_a(lpszCookieName),
        lpCookieData);

    if( lpszUrl )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, NULL, 0 );
        szUrl = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, szUrl, len );
    }

    if( lpszCookieName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, NULL, 0 );
        szCookieName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, szCookieName, len );
    }

    r = InternetGetCookieW( szUrl, szCookieName, NULL, &len );
    if( r )
    {
        szCookieData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szCookieData )
        {
            r = FALSE;
        }
        else
        {
            r = InternetGetCookieW( szUrl, szCookieName, szCookieData, &len );

            *lpdwSize = WideCharToMultiByte( CP_ACP, 0, szCookieData, len,
                                    lpCookieData, *lpdwSize, NULL, NULL );
        }
    }

    HeapFree( GetProcessHeap(), 0, szCookieData );
    HeapFree( GetProcessHeap(), 0, szCookieName );
    HeapFree( GetProcessHeap(), 0, szUrl );

    return r;
}

static BOOL set_cookie(LPCWSTR domain, LPCWSTR path, LPCWSTR cookie_name, LPCWSTR cookie_data)
{
    cookie_domain *thisCookieDomain = NULL;
    cookie *thisCookie;
    struct list *cursor;
    LPWSTR data, value;
    WCHAR *ptr;
    FILETIME expiry;
    BOOL expired = FALSE;

    value = data = HeapAlloc(GetProcessHeap(), 0, (strlenW(cookie_data) + 1) * sizeof(WCHAR));
    strcpyW(data,cookie_data);
    memset(&expiry,0,sizeof(expiry));

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

        value = HeapAlloc(GetProcessHeap(), 0, (ptr - data) * sizeof(WCHAR));
        strcpyW(value, data);

        while (*ptr == ' ') ptr++; /* whitespace */

        if (strncmpiW(ptr, szDomain, 7) == 0)
        {
            ptr+=strlenW(szDomain);
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
            FILETIME ft;
            SYSTEMTIME st;
            FIXME("persistent cookies not handled (%s)\n",debugstr_w(ptr));
            ptr+=strlenW(szExpires);
            if (InternetTimeToSystemTimeW(ptr, &st, 0))
            {
                SystemTimeToFileTime(&st, &expiry);
                GetSystemTimeAsFileTime(&ft);

                if (CompareFileTime(&ft,&expiry) > 0)
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
            HeapFree(GetProcessHeap(),0,data);
            if (value != data) HeapFree(GetProcessHeap(), 0, value);
            return TRUE;
        }
    }

    if ((thisCookie = COOKIE_findCookie(thisCookieDomain, cookie_name)))
        COOKIE_deleteCookie(thisCookie, FALSE);

    TRACE("setting cookie %s=%s for domain %s path %s\n", debugstr_w(cookie_name),
          debugstr_w(value), debugstr_w(thisCookieDomain->lpCookieDomain),debugstr_w(thisCookieDomain->lpCookiePath));

    if (!expired && !COOKIE_addCookie(thisCookieDomain, cookie_name, value, expiry))
    {
        HeapFree(GetProcessHeap(),0,data);
        if (value != data) HeapFree(GetProcessHeap(), 0, value);
        return FALSE;
    }

    HeapFree(GetProcessHeap(),0,data);
    if (value != data) HeapFree(GetProcessHeap(), 0, value);
    return TRUE;
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
    WCHAR hostName[2048], path[2048];

    TRACE("(%s,%s,%s)\n", debugstr_w(lpszUrl),
        debugstr_w(lpszCookieName), debugstr_w(lpCookieData));

    if (!lpszUrl || !lpCookieData)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hostName[0] = path[0] = 0;
    ret = COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));
    if (!ret || !hostName[0]) return FALSE;

    if (!lpszCookieName)
    {
        unsigned int len;
        WCHAR *cookie, *data;

        len = strlenW(lpCookieData);
        if (!(cookie = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        strcpyW(cookie, lpCookieData);

        /* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name[=data].
         */
        if (!(data = strchrW(cookie, '='))) data = cookie + len;
        else *data++ = 0;

        ret = set_cookie(hostName, path, cookie, data);

        HeapFree(GetProcessHeap(), 0, cookie);
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
    DWORD len;
    LPWSTR szCookieData = NULL, szUrl = NULL, szCookieName = NULL;
    BOOL r;

    TRACE("(%s,%s,%s)\n", debugstr_a(lpszUrl),
        debugstr_a(lpszCookieName), debugstr_a(lpCookieData));

    if( lpszUrl )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, NULL, 0 );
        szUrl = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszUrl, -1, szUrl, len );
    }

    if( lpszCookieName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, NULL, 0 );
        szCookieName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszCookieName, -1, szCookieName, len );
    }

    if( lpCookieData )
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpCookieData, -1, NULL, 0 );
        szCookieData = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpCookieData, -1, szCookieData, len );
    }

    r = InternetSetCookieW( szUrl, szCookieName, szCookieData );

    HeapFree( GetProcessHeap(), 0, szCookieData );
    HeapFree( GetProcessHeap(), 0, szCookieName );
    HeapFree( GetProcessHeap(), 0, szUrl );

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

/***********************************************************************
 *           IsDomainLegalCookieDomainW (WININET.@)
 */
BOOL WINAPI IsDomainLegalCookieDomainW( LPCWSTR s1, LPCWSTR s2 )
{
    const WCHAR *p;

    FIXME("(%s, %s)\n", debugstr_w(s1), debugstr_w(s2));

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
    if (!(p = strchrW(s2, '.'))) return FALSE;
    if (strchrW(p + 1, '.') && !strcmpW(p + 1, s1)) return TRUE;
    else if (!strcmpW(s1, s2)) return TRUE;
    return FALSE;
}
