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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

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

#include "wine/list.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME
 *     Cookies are currently memory only.
 *     Cookies are NOT THREAD SAFE
 *     Cookies could use A LOT OF MEMORY. We need some kind of memory management here!
 *     Cookies should care about the expiry time
 */

typedef struct _cookie_domain cookie_domain;
typedef struct _cookie cookie;

struct _cookie
{
    struct list entry;

    struct _cookie_domain *parent;

    LPWSTR lpCookieName;
    LPWSTR lpCookieData;
    time_t expiry; /* FIXME: not used */
};

struct _cookie_domain
{
    struct list entry;

    LPWSTR lpCookieDomain;
    LPWSTR lpCookiePath;
    struct list cookie_list;
};

static struct list domain_list = LIST_INIT(domain_list);

static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data);
static cookie *COOKIE_findCookie(cookie_domain *domain, LPCWSTR lpszCookieName);
static void COOKIE_deleteCookie(cookie *deadCookie, BOOL deleteDomain);
static cookie_domain *COOKIE_addDomain(LPCWSTR domain, LPCWSTR path);
static void COOKIE_deleteDomain(cookie_domain *deadDomain);


/* adds a cookie to the domain */
static cookie *COOKIE_addCookie(cookie_domain *domain, LPCWSTR name, LPCWSTR data)
{
    cookie *newCookie = HeapAlloc(GetProcessHeap(), 0, sizeof(cookie));

    list_init(&newCookie->entry);
    newCookie->lpCookieName = NULL;
    newCookie->lpCookieData = NULL;

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

static void COOKIE_crackUrlSimple(LPCWSTR lpszUrl, LPWSTR hostName, int hostNameLen, LPWSTR path, int pathLen)
{
    URL_COMPONENTSW UrlComponents;

    UrlComponents.lpszExtraInfo = NULL;
    UrlComponents.lpszPassword = NULL;
    UrlComponents.lpszScheme = NULL;
    UrlComponents.lpszUrlPath = path;
    UrlComponents.lpszUserName = NULL;
    UrlComponents.lpszHostName = hostName;
    UrlComponents.dwHostNameLength = hostNameLen;
    UrlComponents.dwUrlPathLength = pathLen;

    InternetCrackUrlW(lpszUrl, 0, 0, &UrlComponents);
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
        TRACE("comparing paths: %s with %s\n", debugstr_w(lpszCookiePath), debugstr_w(searchDomain->lpCookiePath));
        if (!searchDomain->lpCookiePath)
            return FALSE;
        if (strcmpW(lpszCookiePath, searchDomain->lpCookiePath))
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
    struct list * cursor;
    int cnt = 0, domain_count = 0;
    int cookie_count = 0;
    WCHAR hostName[2048], path[2048];

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(lpszUrl),debugstr_w(lpszCookieName),
	  lpCookieData, lpdwSize);

    COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));

    LIST_FOR_EACH(cursor, &domain_list)
    {
        cookie_domain *cookiesDomain = LIST_ENTRY(cursor, cookie_domain, entry);
        if (COOKIE_matchDomain(hostName, NULL /* FIXME: path */, cookiesDomain, TRUE))
        {
            struct list * cursor;
            domain_count++;
            TRACE("found domain %p\n", cookiesDomain);
    
            LIST_FOR_EACH(cursor, &cookiesDomain->cookie_list)
            {
                cookie *thisCookie = LIST_ENTRY(cursor, cookie, entry);
                if (lpCookieData == NULL) /* return the size of the buffer required to lpdwSize */
                {
                    if (cookie_count != 0)
                        cnt += 2; /* '; ' */
                    cnt += strlenW(thisCookie->lpCookieName);
                    cnt += 1; /* = */
                    cnt += strlenW(thisCookie->lpCookieData);
                }
                else
                {
                    static const WCHAR szsc[] = { ';',' ',0 };
                    static const WCHAR szpseq[] = { '%','s','=','%','s',0 };
                    if (cookie_count != 0)
                        cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szsc);
                    cnt += snprintfW(lpCookieData + cnt, *lpdwSize - cnt, szpseq,
                                    thisCookie->lpCookieName,
                                    thisCookie->lpCookieData);
                    TRACE("Cookie: %s=%s\n", debugstr_w(thisCookie->lpCookieName), debugstr_w(thisCookie->lpCookieData));
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
	cnt += 1; /* NULL */
	*lpdwSize = cnt*sizeof(WCHAR);
	TRACE("returning\n");
	return TRUE;
    }

    *lpdwSize = (cnt + 1)*sizeof(WCHAR);

    TRACE("Returning %i (from %i domains): %s\n", cnt, domain_count,
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
            return FALSE;

        r = InternetGetCookieW( szUrl, szCookieName, szCookieData, &len );

        *lpdwSize = WideCharToMultiByte( CP_ACP, 0, szCookieData, len,
                                lpCookieData, *lpdwSize, NULL, NULL );
    }

    HeapFree( GetProcessHeap(), 0, szCookieData );
    HeapFree( GetProcessHeap(), 0, szCookieName );
    HeapFree( GetProcessHeap(), 0, szUrl );

    return r;
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
    cookie_domain *thisCookieDomain = NULL;
    cookie *thisCookie;
    WCHAR hostName[2048], path[2048];
    struct list * cursor;

    TRACE("(%s,%s,%s)\n", debugstr_w(lpszUrl),
        debugstr_w(lpszCookieName), debugstr_w(lpCookieData));

    if (!lpCookieData || !strlenW(lpCookieData))
    {
        TRACE("no cookie data, not adding\n");
	return FALSE;
    }
    if (!lpszCookieName)
    {
	/* some apps (or is it us??) try to add a cookie with no cookie name, but
         * the cookie data in the form of name=data. */
	/* FIXME, probably a bug here, for now I don't care */
	WCHAR *ourCookieName, *ourCookieData;
	int ourCookieNameSize;
        BOOL ret;

	if (!(ourCookieData = strchrW(lpCookieData, '=')))
	{
            TRACE("something terribly wrong with cookie data %s\n", 
                   debugstr_w(ourCookieData));
	    return FALSE;
	}
	ourCookieNameSize = ourCookieData - lpCookieData;
	ourCookieData += 1;
	ourCookieName = HeapAlloc(GetProcessHeap(), 0, 
                              (ourCookieNameSize + 1)*sizeof(WCHAR));
	memcpy(ourCookieName, ourCookieData, ourCookieNameSize * sizeof(WCHAR));
	ourCookieName[ourCookieNameSize] = '\0';
	TRACE("setting (hacked) cookie of %s, %s\n",
               debugstr_w(ourCookieName), debugstr_w(ourCookieData));
        ret = InternetSetCookieW(lpszUrl, ourCookieName, ourCookieData);
	HeapFree(GetProcessHeap(), 0, ourCookieName);
        return ret;
    }

    COOKIE_crackUrlSimple(lpszUrl, hostName, sizeof(hostName)/sizeof(hostName[0]), path, sizeof(path)/sizeof(path[0]));

    LIST_FOR_EACH(cursor, &domain_list)
    {
        thisCookieDomain = LIST_ENTRY(cursor, cookie_domain, entry);
        if (COOKIE_matchDomain(hostName, NULL /* FIXME: path */, thisCookieDomain, FALSE))
            break;
        thisCookieDomain = NULL;
    }
    if (!thisCookieDomain)
        thisCookieDomain = COOKIE_addDomain(hostName, path);

    if ((thisCookie = COOKIE_findCookie(thisCookieDomain, lpszCookieName)))
	COOKIE_deleteCookie(thisCookie, FALSE);

    thisCookie = COOKIE_addCookie(thisCookieDomain, lpszCookieName, lpCookieData);
    return TRUE;
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
    FIXME("(%s, %s, %s, 0x%08lx, 0x%08lx) stub\n",
          debugstr_a(lpszURL), debugstr_a(lpszCookieName), debugstr_a(lpszCookieData),
          dwFlags, dwReserved);
    return TRUE;
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
    FIXME("(%s, %s, %s, 0x%08lx, 0x%08lx) stub\n",
          debugstr_w(lpszURL), debugstr_w(lpszCookieName), debugstr_w(lpszCookieData),
          dwFlags, dwReserved);
    return TRUE;
}

/***********************************************************************
 *           InternetGetCookieExA (WININET.@)
 *
 * See InternetGetCookieExW.
 */
BOOL WINAPI InternetGetCookieExA( LPCSTR pchURL, LPCSTR pchCookieName, LPSTR pchCookieData,
                                  LPDWORD pcchCookieData, DWORD dwFlags, LPVOID lpReserved)
{
    FIXME("(%s, %s, %s, %p, 0x%08lx, %p) stub\n",
          debugstr_a(pchURL), debugstr_a(pchCookieName), debugstr_a(pchCookieData),
          pcchCookieData, dwFlags, lpReserved);
    return FALSE;
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
    FIXME("(%s, %s, %s, %p, 0x%08lx, %p) stub\n",
          debugstr_w(pchURL), debugstr_w(pchCookieName), debugstr_w(pchCookieData),
          pcchCookieData, dwFlags, lpReserved);
    return FALSE;
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
BOOL WINAPI InternetEnumPerSiteCookieDecisionA( LPSTR pszSiteName, unsigned long *pcSiteNameSize,
                                                unsigned long *pdwDecision, unsigned long dwIndex )
{
    FIXME("(%s, %p, %p, 0x%08lx) stub\n",
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
BOOL WINAPI InternetEnumPerSiteCookieDecisionW( LPWSTR pszSiteName, unsigned long *pcSiteNameSize,
                                                unsigned long *pdwDecision, unsigned long dwIndex )
{
    FIXME("(%s, %p, %p, 0x%08lx) stub\n",
          debugstr_w(pszSiteName), pcSiteNameSize, pdwDecision, dwIndex);
    return FALSE;
}

/***********************************************************************
 *           InternetGetPerSiteCookieDecisionA (WININET.@)
 */
BOOL WINAPI InternetGetPerSiteCookieDecisionA( LPCSTR pwchHostName, unsigned long *pResult )
{
    FIXME("(%s, %p) stub\n", debugstr_a(pwchHostName), pResult);
    return FALSE;
}

/***********************************************************************
 *           InternetGetPerSiteCookieDecisionW (WININET.@)
 */
BOOL WINAPI InternetGetPerSiteCookieDecisionW( LPCWSTR pwchHostName, unsigned long *pResult )
{
    FIXME("(%s, %p) stub\n", debugstr_w(pwchHostName), pResult);
    return FALSE;
}

/***********************************************************************
 *           InternetSetPerSiteCookieDecisionA (WININET.@)
 */
BOOL WINAPI InternetSetPerSiteCookieDecisionA( LPCSTR pchHostName, DWORD dwDecision )
{
    FIXME("(%s, 0x%08lx) stub\n", debugstr_a(pchHostName), dwDecision);
    return FALSE;
}

/***********************************************************************
 *           InternetSetPerSiteCookieDecisionW (WININET.@)
 */
BOOL WINAPI InternetSetPerSiteCookieDecisionW( LPCWSTR pchHostName, DWORD dwDecision )
{
    FIXME("(%s, 0x%08lx) stub\n", debugstr_w(pchHostName), dwDecision);
    return FALSE;
}
