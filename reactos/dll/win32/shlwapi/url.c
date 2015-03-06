/*
 * Url functions
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
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

#include "precomp.h"

#include <wininet.h>
#include <intshcut.h>

HMODULE WINAPI MLLoadLibraryW(LPCWSTR,HMODULE,DWORD);
BOOL    WINAPI MLFreeLibrary(HMODULE);
HRESULT WINAPI MLBuildResURLW(LPCWSTR,HMODULE,DWORD,LPCWSTR,LPWSTR,DWORD);

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

/* The following schemes were identified in the native version of
 * SHLWAPI.DLL version 5.50
 */
static const struct {
    URL_SCHEME  scheme_number;
    WCHAR scheme_name[12];
} shlwapi_schemes[] = {
  {URL_SCHEME_FTP,        {'f','t','p',0}},
  {URL_SCHEME_HTTP,       {'h','t','t','p',0}},
  {URL_SCHEME_GOPHER,     {'g','o','p','h','e','r',0}},
  {URL_SCHEME_MAILTO,     {'m','a','i','l','t','o',0}},
  {URL_SCHEME_NEWS,       {'n','e','w','s',0}},
  {URL_SCHEME_NNTP,       {'n','n','t','p',0}},
  {URL_SCHEME_TELNET,     {'t','e','l','n','e','t',0}},
  {URL_SCHEME_WAIS,       {'w','a','i','s',0}},
  {URL_SCHEME_FILE,       {'f','i','l','e',0}},
  {URL_SCHEME_MK,         {'m','k',0}},
  {URL_SCHEME_HTTPS,      {'h','t','t','p','s',0}},
  {URL_SCHEME_SHELL,      {'s','h','e','l','l',0}},
  {URL_SCHEME_SNEWS,      {'s','n','e','w','s',0}},
  {URL_SCHEME_LOCAL,      {'l','o','c','a','l',0}},
  {URL_SCHEME_JAVASCRIPT, {'j','a','v','a','s','c','r','i','p','t',0}},
  {URL_SCHEME_VBSCRIPT,   {'v','b','s','c','r','i','p','t',0}},
  {URL_SCHEME_ABOUT,      {'a','b','o','u','t',0}},
  {URL_SCHEME_RES,        {'r','e','s',0}},
};

typedef struct {
    LPCWSTR pScheme;      /* [out] start of scheme                     */
    DWORD   szScheme;     /* [out] size of scheme (until colon)        */
    LPCWSTR pUserName;    /* [out] start of Username                   */
    DWORD   szUserName;   /* [out] size of Username (until ":" or "@") */
    LPCWSTR pPassword;    /* [out] start of Password                   */
    DWORD   szPassword;   /* [out] size of Password (until "@")        */
    LPCWSTR pHostName;    /* [out] start of Hostname                   */
    DWORD   szHostName;   /* [out] size of Hostname (until ":" or "/") */
    LPCWSTR pPort;        /* [out] start of Port                       */
    DWORD   szPort;       /* [out] size of Port (until "/" or eos)     */
    LPCWSTR pQuery;       /* [out] start of Query                      */
    DWORD   szQuery;      /* [out] size of Query (until eos)           */
} WINE_PARSE_URL;

typedef enum {
    SCHEME,
    HOST,
    PORT,
    USERPASS,
} WINE_URL_SCAN_TYPE;

static const CHAR hexDigits[] = "0123456789ABCDEF";

static const WCHAR fileW[] = {'f','i','l','e','\0'};

static const unsigned char HashDataLookup[256] = {
 0x01, 0x0E, 0x6E, 0x19, 0x61, 0xAE, 0x84, 0x77, 0x8A, 0xAA, 0x7D, 0x76, 0x1B,
 0xE9, 0x8C, 0x33, 0x57, 0xC5, 0xB1, 0x6B, 0xEA, 0xA9, 0x38, 0x44, 0x1E, 0x07,
 0xAD, 0x49, 0xBC, 0x28, 0x24, 0x41, 0x31, 0xD5, 0x68, 0xBE, 0x39, 0xD3, 0x94,
 0xDF, 0x30, 0x73, 0x0F, 0x02, 0x43, 0xBA, 0xD2, 0x1C, 0x0C, 0xB5, 0x67, 0x46,
 0x16, 0x3A, 0x4B, 0x4E, 0xB7, 0xA7, 0xEE, 0x9D, 0x7C, 0x93, 0xAC, 0x90, 0xB0,
 0xA1, 0x8D, 0x56, 0x3C, 0x42, 0x80, 0x53, 0x9C, 0xF1, 0x4F, 0x2E, 0xA8, 0xC6,
 0x29, 0xFE, 0xB2, 0x55, 0xFD, 0xED, 0xFA, 0x9A, 0x85, 0x58, 0x23, 0xCE, 0x5F,
 0x74, 0xFC, 0xC0, 0x36, 0xDD, 0x66, 0xDA, 0xFF, 0xF0, 0x52, 0x6A, 0x9E, 0xC9,
 0x3D, 0x03, 0x59, 0x09, 0x2A, 0x9B, 0x9F, 0x5D, 0xA6, 0x50, 0x32, 0x22, 0xAF,
 0xC3, 0x64, 0x63, 0x1A, 0x96, 0x10, 0x91, 0x04, 0x21, 0x08, 0xBD, 0x79, 0x40,
 0x4D, 0x48, 0xD0, 0xF5, 0x82, 0x7A, 0x8F, 0x37, 0x69, 0x86, 0x1D, 0xA4, 0xB9,
 0xC2, 0xC1, 0xEF, 0x65, 0xF2, 0x05, 0xAB, 0x7E, 0x0B, 0x4A, 0x3B, 0x89, 0xE4,
 0x6C, 0xBF, 0xE8, 0x8B, 0x06, 0x18, 0x51, 0x14, 0x7F, 0x11, 0x5B, 0x5C, 0xFB,
 0x97, 0xE1, 0xCF, 0x15, 0x62, 0x71, 0x70, 0x54, 0xE2, 0x12, 0xD6, 0xC7, 0xBB,
 0x0D, 0x20, 0x5E, 0xDC, 0xE0, 0xD4, 0xF7, 0xCC, 0xC4, 0x2B, 0xF9, 0xEC, 0x2D,
 0xF4, 0x6F, 0xB6, 0x99, 0x88, 0x81, 0x5A, 0xD9, 0xCA, 0x13, 0xA5, 0xE7, 0x47,
 0xE6, 0x8E, 0x60, 0xE3, 0x3E, 0xB3, 0xF6, 0x72, 0xA2, 0x35, 0xA0, 0xD7, 0xCD,
 0xB4, 0x2F, 0x6D, 0x2C, 0x26, 0x1F, 0x95, 0x87, 0x00, 0xD8, 0x34, 0x3F, 0x17,
 0x25, 0x45, 0x27, 0x75, 0x92, 0xB8, 0xA3, 0xC8, 0xDE, 0xEB, 0xF8, 0xF3, 0xDB,
 0x0A, 0x98, 0x83, 0x7B, 0xE5, 0xCB, 0x4C, 0x78, 0xD1 };

static DWORD get_scheme_code(LPCWSTR scheme, DWORD scheme_len)
{
    unsigned int i;

    for(i=0; i < sizeof(shlwapi_schemes)/sizeof(shlwapi_schemes[0]); i++) {
        if(scheme_len == strlenW(shlwapi_schemes[i].scheme_name)
           && !memcmp(scheme, shlwapi_schemes[i].scheme_name, scheme_len*sizeof(WCHAR)))
            return shlwapi_schemes[i].scheme_number;
    }

    return URL_SCHEME_UNKNOWN;
}

/*************************************************************************
 *      @	[SHLWAPI.1]
 *
 * Parse a Url into its constituent parts.
 *
 * PARAMS
 *  x [I] Url to parse
 *  y [O] Undocumented structure holding the parsed information
 *
 * RETURNS
 *  Success: S_OK. y contains the parsed Url details.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI ParseURLA(LPCSTR x, PARSEDURLA *y)
{
    WCHAR scheme[INTERNET_MAX_SCHEME_LENGTH];
    const char *ptr = x;
    int len;

    TRACE("%s %p\n", debugstr_a(x), y);

    if(y->cbSize != sizeof(*y))
        return E_INVALIDARG;

    while(*ptr && (isalnum(*ptr) || *ptr == '-'))
        ptr++;

    if (*ptr != ':' || ptr <= x+1) {
        y->pszProtocol = NULL;
        return URL_E_INVALID_SYNTAX;
    }

    y->pszProtocol = x;
    y->cchProtocol = ptr-x;
    y->pszSuffix = ptr+1;
    y->cchSuffix = strlen(y->pszSuffix);

    len = MultiByteToWideChar(CP_ACP, 0, x, ptr-x,
            scheme, sizeof(scheme)/sizeof(WCHAR));
    y->nScheme = get_scheme_code(scheme, len);

    return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.2]
 *
 * Unicode version of ParseURLA.
 */
HRESULT WINAPI ParseURLW(LPCWSTR x, PARSEDURLW *y)
{
    const WCHAR *ptr = x;

    TRACE("%s %p\n", debugstr_w(x), y);

    if(y->cbSize != sizeof(*y))
        return E_INVALIDARG;

    while(*ptr && (isalnumW(*ptr) || *ptr == '-'))
        ptr++;

    if (*ptr != ':' || ptr <= x+1) {
        y->pszProtocol = NULL;
        return URL_E_INVALID_SYNTAX;
    }

    y->pszProtocol = x;
    y->cchProtocol = ptr-x;
    y->pszSuffix = ptr+1;
    y->cchSuffix = strlenW(y->pszSuffix);
    y->nScheme = get_scheme_code(x, ptr-x);

    return S_OK;
}

/*************************************************************************
 *        UrlCanonicalizeA     [SHLWAPI.@]
 *
 * Canonicalize a Url.
 *
 * PARAMS
 *  pszUrl            [I]   Url to cCanonicalize
 *  pszCanonicalized  [O]   Destination for converted Url.
 *  pcchCanonicalized [I/O] Length of pszUrl, destination for length of pszCanonicalized
 *  dwFlags           [I]   Flags controlling the conversion.
 *
 * RETURNS
 *  Success: S_OK. The pszCanonicalized contains the converted Url.
 *  Failure: E_POINTER, if *pcchCanonicalized is too small.
 *
 * MSDN incorrectly describes the flags for this function. They should be:
 *|    URL_DONT_ESCAPE_EXTRA_INFO    0x02000000
 *|    URL_ESCAPE_SPACES_ONLY        0x04000000
 *|    URL_ESCAPE_PERCENT            0x00001000
 *|    URL_ESCAPE_UNSAFE             0x10000000
 *|    URL_UNESCAPE                  0x10000000
 *|    URL_DONT_SIMPLIFY             0x08000000
 *|    URL_ESCAPE_SEGMENT_ONLY       0x00002000
 */
HRESULT WINAPI UrlCanonicalizeA(LPCSTR pszUrl, LPSTR pszCanonicalized,
	LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    LPWSTR url, canonical;
    HRESULT ret;

    TRACE("(%s, %p, %p, 0x%08x) *pcchCanonicalized: %d\n", debugstr_a(pszUrl), pszCanonicalized,
        pcchCanonicalized, dwFlags, pcchCanonicalized ? *pcchCanonicalized : -1);

    if(!pszUrl || !pszCanonicalized || !pcchCanonicalized || !*pcchCanonicalized)
	return E_INVALIDARG;

    url = heap_strdupAtoW(pszUrl);
    canonical = HeapAlloc(GetProcessHeap(), 0, *pcchCanonicalized*sizeof(WCHAR));
    if(!url || !canonical) {
        HeapFree(GetProcessHeap(), 0, url);
        HeapFree(GetProcessHeap(), 0, canonical);
        return E_OUTOFMEMORY;
    }

    ret = UrlCanonicalizeW(url, canonical, pcchCanonicalized, dwFlags);
    if(ret == S_OK)
        WideCharToMultiByte(CP_ACP, 0, canonical, -1, pszCanonicalized,
                            *pcchCanonicalized+1, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, url);
    HeapFree(GetProcessHeap(), 0, canonical);
    return ret;
}

/*************************************************************************
 *        UrlCanonicalizeW     [SHLWAPI.@]
 *
 * See UrlCanonicalizeA.
 */
HRESULT WINAPI UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized,
				LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    DWORD EscapeFlags;
    LPCWSTR wk1, root;
    LPWSTR lpszUrlCpy, url, wk2, mp, mp2;
    INT state;
    DWORD nByteLen, nLen, nWkLen;
    BOOL is_file_url;
    WCHAR slash = '\0';

    static const WCHAR wszFile[] = {'f','i','l','e',':'};
    static const WCHAR wszRes[] = {'r','e','s',':'};
    static const WCHAR wszHttp[] = {'h','t','t','p',':'};
    static const WCHAR wszLocalhost[] = {'l','o','c','a','l','h','o','s','t'};
    static const WCHAR wszFilePrefix[] = {'f','i','l','e',':','/','/','/'};

    TRACE("(%s, %p, %p, 0x%08x) *pcchCanonicalized: %d\n", debugstr_w(pszUrl), pszCanonicalized,
        pcchCanonicalized, dwFlags, pcchCanonicalized ? *pcchCanonicalized : -1);

    if(!pszUrl || !pszCanonicalized || !pcchCanonicalized || !*pcchCanonicalized)
	return E_INVALIDARG;

    if(!*pszUrl) {
        *pszCanonicalized = 0;
        return S_OK;
    }

    /* Remove '\t' characters from URL */
    nByteLen = (strlenW(pszUrl) + 1) * sizeof(WCHAR); /* length in bytes */
    url = HeapAlloc(GetProcessHeap(), 0, nByteLen);
    if(!url)
        return E_OUTOFMEMORY;

    wk1 = pszUrl;
    wk2 = url;
    do {
        while(*wk1 == '\t')
            wk1++;
        *wk2++ = *wk1;
    } while(*wk1++);

    /* Allocate memory for simplified URL (before escaping) */
    nByteLen = (wk2-url)*sizeof(WCHAR);
    lpszUrlCpy = HeapAlloc(GetProcessHeap(), 0,
            nByteLen+sizeof(wszFilePrefix)+sizeof(WCHAR));
    if(!lpszUrlCpy) {
        HeapFree(GetProcessHeap(), 0, url);
        return E_OUTOFMEMORY;
    }

    is_file_url = !strncmpW(wszFile, url, sizeof(wszFile)/sizeof(WCHAR));

    if ((nByteLen >= sizeof(wszHttp) &&
         !memcmp(wszHttp, url, sizeof(wszHttp))) || is_file_url)
        slash = '/';

    if((dwFlags & (URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY)) && is_file_url)
        slash = '\\';

    if(nByteLen >= sizeof(wszRes) && !memcmp(wszRes, url, sizeof(wszRes))) {
        dwFlags &= ~URL_FILE_USE_PATHURL;
        slash = '\0';
    }

    /*
     * state =
     *         0   initial  1,3
     *         1   have 2[+] alnum  2,3
     *         2   have scheme (found :)  4,6,3
     *         3   failed (no location)
     *         4   have //  5,3
     *         5   have 1[+] alnum  6,3
     *         6   have location (found /) save root location
     */

    wk1 = url;
    wk2 = lpszUrlCpy;
    state = 0;

    if(url[1] == ':') { /* Assume path */
        memcpy(wk2, wszFilePrefix, sizeof(wszFilePrefix));
        wk2 += sizeof(wszFilePrefix)/sizeof(WCHAR);
        if (dwFlags & (URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY))
        {
            slash = '\\';
            --wk2;
        }
        else
            dwFlags |= URL_ESCAPE_UNSAFE;
        state = 5;
        is_file_url = TRUE;
    }

    while (*wk1) {
        switch (state) {
        case 0:
            if (!isalnumW(*wk1)) {state = 3; break;}
            *wk2++ = *wk1++;
            if (!isalnumW(*wk1)) {state = 3; break;}
            *wk2++ = *wk1++;
            state = 1;
            break;
        case 1:
            *wk2++ = *wk1;
            if (*wk1++ == ':') state = 2;
            break;
        case 2:
            *wk2++ = *wk1++;
            if (*wk1 != '/') {state = 6; break;}
            *wk2++ = *wk1++;
            if((dwFlags & URL_FILE_USE_PATHURL) && nByteLen >= sizeof(wszLocalhost)
                        && is_file_url
                        && !memcmp(wszLocalhost, wk1, sizeof(wszLocalhost))){
                wk1 += sizeof(wszLocalhost)/sizeof(WCHAR);
                while(*wk1 == '\\' && (dwFlags & URL_FILE_USE_PATHURL))
                    wk1++;
            }

            if(*wk1 == '/' && (dwFlags & URL_FILE_USE_PATHURL)){
                wk1++;
            }else if(is_file_url){
                const WCHAR *body = wk1;

                while(*body == '/')
                    ++body;

                if(isalnumW(*body) && *(body+1) == ':'){
                    if(!(dwFlags & (URL_WININET_COMPATIBILITY | URL_FILE_USE_PATHURL))){
                        if(slash)
                            *wk2++ = slash;
                        else
                            *wk2++ = '/';
                    }
                }else{
                    if(dwFlags & URL_WININET_COMPATIBILITY){
                        if(*wk1 == '/' && *(wk1+1) != '/'){
                            *wk2++ = '\\';
                        }else{
                            *wk2++ = '\\';
                            *wk2++ = '\\';
                        }
                    }else{
                        if(*wk1 == '/' && *(wk1+1) != '/'){
                            if(slash)
                                *wk2++ = slash;
                            else
                                *wk2++ = '/';
                        }
                    }
                }
                wk1 = body;
            }
            state = 4;
            break;
        case 3:
            nWkLen = strlenW(wk1);
            memcpy(wk2, wk1, (nWkLen + 1) * sizeof(WCHAR));
            mp = wk2;
            wk1 += nWkLen;
            wk2 += nWkLen;

            if(slash) {
                while(mp < wk2) {
                    if(*mp == '/' || *mp == '\\')
                        *mp = slash;
                    mp++;
                }
            }
            break;
        case 4:
            if (!isalnumW(*wk1) && (*wk1 != '-') && (*wk1 != '.') && (*wk1 != ':'))
                {state = 3; break;}
            while(isalnumW(*wk1) || (*wk1 == '-') || (*wk1 == '.') || (*wk1 == ':'))
                *wk2++ = *wk1++;
            state = 5;
            if (!*wk1) {
                if(slash)
                    *wk2++ = slash;
                else
                    *wk2++ = '/';
            }
            break;
        case 5:
            if (*wk1 != '/' && *wk1 != '\\') {state = 3; break;}
            while(*wk1 == '/' || *wk1 == '\\') {
                if(slash)
                    *wk2++ = slash;
                else
                    *wk2++ = *wk1;
                wk1++;
            }
            state = 6;
            break;
        case 6:
            if(dwFlags & URL_DONT_SIMPLIFY) {
                state = 3;
                break;
            }
 
            /* Now at root location, cannot back up any more. */
            /* "root" will point at the '/' */

            root = wk2-1;
            while (*wk1) {
                mp = strchrW(wk1, '/');
                mp2 = strchrW(wk1, '\\');
                if(mp2 && (!mp || mp2 < mp))
                    mp = mp2;
                if (!mp) {
                    nWkLen = strlenW(wk1);
                    memcpy(wk2, wk1, (nWkLen + 1) * sizeof(WCHAR));
                    wk1 += nWkLen;
                    wk2 += nWkLen;
                    continue;
                }
                nLen = mp - wk1;
                if(nLen) {
                    memcpy(wk2, wk1, nLen * sizeof(WCHAR));
                    wk2 += nLen;
                    wk1 += nLen;
                }
                if(slash)
                    *wk2++ = slash;
                else
                    *wk2++ = *wk1;
                wk1++;

                while (*wk1 == '.') {
                    TRACE("found '/.'\n");
                    if (wk1[1] == '/' || wk1[1] == '\\') {
                        /* case of /./ -> skip the ./ */
                        wk1 += 2;
                    }
                    else if (wk1[1] == '.' && (wk1[2] == '/'
                            || wk1[2] == '\\' || wk1[2] == '?'
                            || wk1[2] == '#' || !wk1[2])) {
                        /* case /../ -> need to backup wk2 */
                        TRACE("found '/../'\n");
                        *(wk2-1) = '\0';  /* set end of string */
                        mp = strrchrW(root, '/');
                        mp2 = strrchrW(root, '\\');
                        if(mp2 && (!mp || mp2 < mp))
                            mp = mp2;
                        if (mp && (mp >= root)) {
                            /* found valid backup point */
                            wk2 = mp + 1;
                            if(wk1[2] != '/' && wk1[2] != '\\')
                                wk1 += 2;
                            else
                                wk1 += 3;
                        }
                        else {
                            /* did not find point, restore '/' */
                            *(wk2-1) = slash;
                            break;
                        }
                    }
                    else
                        break;
                }
            }
            *wk2 = '\0';
            break;
        default:
            FIXME("how did we get here - state=%d\n", state);
            HeapFree(GetProcessHeap(), 0, lpszUrlCpy);
            HeapFree(GetProcessHeap(), 0, url);
            return E_INVALIDARG;
        }
        *wk2 = '\0';
	TRACE("Simplified, orig <%s>, simple <%s>\n",
	      debugstr_w(pszUrl), debugstr_w(lpszUrlCpy));
    }
    nLen = lstrlenW(lpszUrlCpy);
    while ((nLen > 0) && ((lpszUrlCpy[nLen-1] <= ' ')))
        lpszUrlCpy[--nLen]=0;

    if((dwFlags & URL_UNESCAPE) ||
       ((dwFlags & URL_FILE_USE_PATHURL) && nByteLen >= sizeof(wszFile)
                && !memcmp(wszFile, url, sizeof(wszFile))))
        UrlUnescapeW(lpszUrlCpy, NULL, &nLen, URL_UNESCAPE_INPLACE);

    if((EscapeFlags = dwFlags & (URL_ESCAPE_UNSAFE |
                                 URL_ESCAPE_SPACES_ONLY |
                                 URL_ESCAPE_PERCENT |
                                 URL_DONT_ESCAPE_EXTRA_INFO |
				 URL_ESCAPE_SEGMENT_ONLY ))) {
	EscapeFlags &= ~URL_ESCAPE_UNSAFE;
	hr = UrlEscapeW(lpszUrlCpy, pszCanonicalized, pcchCanonicalized,
			EscapeFlags);
    } else { /* No escaping needed, just copy the string */
        nLen = lstrlenW(lpszUrlCpy);
	if(nLen < *pcchCanonicalized)
	    memcpy(pszCanonicalized, lpszUrlCpy, (nLen + 1)*sizeof(WCHAR));
	else {
	    hr = E_POINTER;
	    nLen++;
	}
	*pcchCanonicalized = nLen;
    }

    HeapFree(GetProcessHeap(), 0, lpszUrlCpy);
    HeapFree(GetProcessHeap(), 0, url);

    if (hr == S_OK)
	TRACE("result %s\n", debugstr_w(pszCanonicalized));

    return hr;
}

/*************************************************************************
 *        UrlCombineA     [SHLWAPI.@]
 *
 * Combine two Urls.
 *
 * PARAMS
 *  pszBase      [I] Base Url
 *  pszRelative  [I] Url to combine with pszBase
 *  pszCombined  [O] Destination for combined Url
 *  pcchCombined [O] Destination for length of pszCombined
 *  dwFlags      [I] URL_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. pszCombined contains the combined Url, pcchCombined
 *           contains its length.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI UrlCombineA(LPCSTR pszBase, LPCSTR pszRelative,
			   LPSTR pszCombined, LPDWORD pcchCombined,
			   DWORD dwFlags)
{
    LPWSTR base, relative, combined;
    DWORD ret, len, len2;

    TRACE("(base %s, Relative %s, Combine size %d, flags %08x) using W version\n",
	  debugstr_a(pszBase),debugstr_a(pszRelative),
	  pcchCombined?*pcchCombined:0,dwFlags);

    if(!pszBase || !pszRelative || !pcchCombined)
	return E_INVALIDARG;

    base = HeapAlloc(GetProcessHeap(), 0,
			      (3*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    relative = base + INTERNET_MAX_URL_LENGTH;
    combined = relative + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, pszBase, -1, base, INTERNET_MAX_URL_LENGTH);
    MultiByteToWideChar(CP_ACP, 0, pszRelative, -1, relative, INTERNET_MAX_URL_LENGTH);
    len = *pcchCombined;

    ret = UrlCombineW(base, relative, pszCombined?combined:NULL, &len, dwFlags);
    if (ret != S_OK) {
	*pcchCombined = len;
	HeapFree(GetProcessHeap(), 0, base);
	return ret;
    }

    len2 = WideCharToMultiByte(CP_ACP, 0, combined, len, NULL, 0, NULL, NULL);
    if (len2 > *pcchCombined) {
	*pcchCombined = len2;
	HeapFree(GetProcessHeap(), 0, base);
	return E_POINTER;
    }
    WideCharToMultiByte(CP_ACP, 0, combined, len+1, pszCombined, (*pcchCombined)+1,
			NULL, NULL);
    *pcchCombined = len2;
    HeapFree(GetProcessHeap(), 0, base);
    return S_OK;
}

/*************************************************************************
 *        UrlCombineW     [SHLWAPI.@]
 *
 * See UrlCombineA.
 */
HRESULT WINAPI UrlCombineW(LPCWSTR pszBase, LPCWSTR pszRelative,
			   LPWSTR pszCombined, LPDWORD pcchCombined,
			   DWORD dwFlags)
{
    PARSEDURLW base, relative;
    DWORD myflags, sizeloc = 0;
    DWORD i, len, res1, res2, process_case = 0;
    LPWSTR work, preliminary, mbase, mrelative;
    static const WCHAR myfilestr[] = {'f','i','l','e',':','/','/','/','\0'};
    static const WCHAR fragquerystr[] = {'#','?',0};
    HRESULT ret;

    TRACE("(base %s, Relative %s, Combine size %d, flags %08x)\n",
	  debugstr_w(pszBase),debugstr_w(pszRelative),
	  pcchCombined?*pcchCombined:0,dwFlags);

    if(!pszBase || !pszRelative || !pcchCombined)
	return E_INVALIDARG;

    base.cbSize = sizeof(base);
    relative.cbSize = sizeof(relative);

    /* Get space for duplicates of the input and the output */
    preliminary = HeapAlloc(GetProcessHeap(), 0, (3*INTERNET_MAX_URL_LENGTH) *
			    sizeof(WCHAR));
    mbase = preliminary + INTERNET_MAX_URL_LENGTH;
    mrelative = mbase + INTERNET_MAX_URL_LENGTH;
    *preliminary = '\0';

    /* Canonicalize the base input prior to looking for the scheme */
    myflags = dwFlags & (URL_DONT_SIMPLIFY | URL_UNESCAPE);
    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlCanonicalizeW(pszBase, mbase, &len, myflags);

    /* Canonicalize the relative input prior to looking for the scheme */
    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlCanonicalizeW(pszRelative, mrelative, &len, myflags);

    /* See if the base has a scheme */
    res1 = ParseURLW(mbase, &base);
    if (res1) {
	/* if pszBase has no scheme, then return pszRelative */
	TRACE("no scheme detected in Base\n");
	process_case = 1;
    }
    else do {
        BOOL manual_search = FALSE;

        work = (LPWSTR)base.pszProtocol;
        for(i=0; i<base.cchProtocol; i++)
            work[i] = tolowerW(work[i]);

        /* mk is a special case */
        if(base.nScheme == URL_SCHEME_MK) {
            static const WCHAR wsz[] = {':',':',0};

            WCHAR *ptr = strstrW(base.pszSuffix, wsz);
            if(ptr) {
                int delta;

                ptr += 2;
                delta = ptr-base.pszSuffix;
                base.cchProtocol += delta;
                base.pszSuffix += delta;
                base.cchSuffix -= delta;
            }
        }else {
            /* get size of location field (if it exists) */
            work = (LPWSTR)base.pszSuffix;
            sizeloc = 0;
            if (*work++ == '/') {
                if (*work++ == '/') {
                    /* At this point have start of location and
                     * it ends at next '/' or end of string.
                     */
                    while(*work && (*work != '/')) work++;
                    sizeloc = (DWORD)(work - base.pszSuffix);
                }
            }
        }

        /* If there is a '?', then the remaining part can only contain a
         * query string or fragment, so start looking for the last leaf
         * from the '?'. Otherwise, if there is a '#' and the characters
         * immediately preceding it are ".htm[l]", then begin looking for
         * the last leaf starting from the '#'. Otherwise the '#' is not
         * meaningful and just start looking from the end. */
        if ((work = strpbrkW(base.pszSuffix + sizeloc, fragquerystr))) {
            const WCHAR htmlW[] = {'.','h','t','m','l',0};
            const int len_htmlW = 5;
            const WCHAR htmW[] = {'.','h','t','m',0};
            const int len_htmW = 4;

            if (*work == '?' || base.nScheme == URL_SCHEME_HTTP || base.nScheme == URL_SCHEME_HTTPS)
                manual_search = TRUE;
            else if (work - base.pszSuffix > len_htmW) {
                work -= len_htmW;
                if (strncmpiW(work, htmW, len_htmW) == 0)
                    manual_search = TRUE;
                work += len_htmW;
            }

            if (!manual_search &&
                    work - base.pszSuffix > len_htmlW) {
                work -= len_htmlW;
                if (strncmpiW(work, htmlW, len_htmlW) == 0)
                    manual_search = TRUE;
                work += len_htmlW;
            }
        }

        if (manual_search) {
            /* search backwards starting from the current position */
            while (*work != '/' && work > base.pszSuffix + sizeloc)
                --work;
            base.cchSuffix = work - base.pszSuffix + 1;
        }else {
            /* search backwards starting from the end of the string */
            work = strrchrW((base.pszSuffix+sizeloc), '/');
            if (work) {
                len = (DWORD)(work - base.pszSuffix + 1);
                base.cchSuffix = len;
            }else
                base.cchSuffix = sizeloc;
        }

	/*
	 * At this point:
	 *    .pszSuffix   points to location (starting with '//')
	 *    .cchSuffix   length of location (above) and rest less the last
	 *                 leaf (if any)
	 *    sizeloc   length of location (above) up to but not including
	 *              the last '/'
	 */

	res2 = ParseURLW(mrelative, &relative);
	if (res2) {
	    /* no scheme in pszRelative */
	    TRACE("no scheme detected in Relative\n");
	    relative.pszSuffix = mrelative;  /* case 3,4,5 depends on this */
	    relative.cchSuffix = strlenW(mrelative);
            if (*pszRelative  == ':') {
		/* case that is either left alone or uses pszBase */
		if (dwFlags & URL_PLUGGABLE_PROTOCOL) {
		    process_case = 5;
		    break;
		}
		process_case = 1;
		break;
	    }
            if (isalnum(*mrelative) && (*(mrelative + 1) == ':')) {
		/* case that becomes "file:///" */
		strcpyW(preliminary, myfilestr);
		process_case = 1;
		break;
	    }
            if ((*mrelative == '/') && (*(mrelative+1) == '/')) {
		/* pszRelative has location and rest */
		process_case = 3;
		break;
	    }
            if (*mrelative == '/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
            if (*mrelative == '#') {
                if(!(work = strchrW(base.pszSuffix+base.cchSuffix, '#')))
                    work = (LPWSTR)base.pszSuffix + strlenW(base.pszSuffix);

                memcpy(preliminary, base.pszProtocol, (work-base.pszProtocol)*sizeof(WCHAR));
                preliminary[work-base.pszProtocol] = '\0';
                process_case = 1;
                break;
            }
            process_case = (*base.pszSuffix == '/' || base.nScheme == URL_SCHEME_MK) ? 5 : 3;
	    break;
	}else {
            work = (LPWSTR)relative.pszProtocol;
            for(i=0; i<relative.cchProtocol; i++)
                work[i] = tolowerW(work[i]);
        }

	/* handle cases where pszRelative has scheme */
	if ((base.cchProtocol == relative.cchProtocol) &&
	    (strncmpW(base.pszProtocol, relative.pszProtocol, base.cchProtocol) == 0)) {

	    /* since the schemes are the same */
            if ((*relative.pszSuffix == '/') && (*(relative.pszSuffix+1) == '/')) {
		/* case where pszRelative replaces location and following */
		process_case = 3;
		break;
	    }
            if (*relative.pszSuffix == '/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
            /* replace either just location if base's location starts with a
             * slash or otherwise everything */
            process_case = (*base.pszSuffix == '/') ? 5 : 1;
	    break;
	}
        if ((*relative.pszSuffix == '/') && (*(relative.pszSuffix+1) == '/')) {
	    /* case where pszRelative replaces scheme, location,
	     * and following and handles PLUGGABLE
	     */
	    process_case = 2;
	    break;
	}
	process_case = 1;
	break;
    } while(FALSE); /* a little trick to allow easy exit from nested if's */

    ret = S_OK;
    switch (process_case) {

    case 1:  /*
	      * Return pszRelative appended to what ever is in pszCombined,
	      * (which may the string "file:///"
	      */
	strcatW(preliminary, mrelative);
	break;

    case 2:  /* case where pszRelative replaces scheme, and location */
	strcpyW(preliminary, mrelative);
	break;

    case 3:  /*
	      * Return the pszBase scheme with pszRelative. Basically
	      * keeps the scheme and replaces the domain and following.
	      */
        memcpy(preliminary, base.pszProtocol, (base.cchProtocol + 1)*sizeof(WCHAR));
	work = preliminary + base.cchProtocol + 1;
	strcpyW(work, relative.pszSuffix);
	break;

    case 4:  /*
	      * Return the pszBase scheme and location but everything
	      * after the location is pszRelative. (Replace document
	      * from root on.)
	      */
        memcpy(preliminary, base.pszProtocol, (base.cchProtocol+1+sizeloc)*sizeof(WCHAR));
	work = preliminary + base.cchProtocol + 1 + sizeloc;
	if (dwFlags & URL_PLUGGABLE_PROTOCOL)
            *(work++) = '/';
	strcpyW(work, relative.pszSuffix);
	break;

    case 5:  /*
	      * Return the pszBase without its document (if any) and
	      * append pszRelative after its scheme.
	      */
        memcpy(preliminary, base.pszProtocol,
               (base.cchProtocol+1+base.cchSuffix)*sizeof(WCHAR));
	work = preliminary + base.cchProtocol+1+base.cchSuffix - 1;
        if (*work++ != '/')
            *(work++) = '/';
	strcpyW(work, relative.pszSuffix);
	break;

    default:
	FIXME("How did we get here????? process_case=%d\n", process_case);
	ret = E_INVALIDARG;
    }

    if (ret == S_OK) {
	/* Reuse mrelative as temp storage as its already allocated and not needed anymore */
        if(*pcchCombined == 0)
            *pcchCombined = 1;
	ret = UrlCanonicalizeW(preliminary, mrelative, pcchCombined, (dwFlags & ~URL_FILE_USE_PATHURL));
	if(SUCCEEDED(ret) && pszCombined) {
	    lstrcpyW(pszCombined, mrelative);
	}
	TRACE("return-%d len=%d, %s\n",
	      process_case, *pcchCombined, debugstr_w(pszCombined));
    }
    HeapFree(GetProcessHeap(), 0, preliminary);
    return ret;
}

/*************************************************************************
 *      UrlEscapeA	[SHLWAPI.@]
 */

HRESULT WINAPI UrlEscapeA(
	LPCSTR pszUrl,
	LPSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    WCHAR bufW[INTERNET_MAX_URL_LENGTH];
    WCHAR *escapedW = bufW;
    UNICODE_STRING urlW;
    HRESULT ret;
    DWORD lenW = sizeof(bufW)/sizeof(WCHAR), lenA;

    if (!pszEscaped || !pcchEscaped || !*pcchEscaped)
        return E_INVALIDARG;

    if(!RtlCreateUnicodeStringFromAsciiz(&urlW, pszUrl))
        return E_INVALIDARG;
    if((ret = UrlEscapeW(urlW.Buffer, escapedW, &lenW, dwFlags)) == E_POINTER) {
        escapedW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        ret = UrlEscapeW(urlW.Buffer, escapedW, &lenW, dwFlags);
    }
    if(ret == S_OK) {
        RtlUnicodeToMultiByteSize(&lenA, escapedW, lenW * sizeof(WCHAR));
        if(*pcchEscaped > lenA) {
            RtlUnicodeToMultiByteN(pszEscaped, *pcchEscaped - 1, &lenA, escapedW, lenW * sizeof(WCHAR));
            pszEscaped[lenA] = 0;
            *pcchEscaped = lenA;
        } else {
            *pcchEscaped = lenA + 1;
            ret = E_POINTER;
        }
    }
    if(escapedW != bufW) HeapFree(GetProcessHeap(), 0, escapedW);
    RtlFreeUnicodeString(&urlW);
    return ret;
}

#define WINE_URL_BASH_AS_SLASH    0x01
#define WINE_URL_COLLAPSE_SLASHES 0x02
#define WINE_URL_ESCAPE_SLASH     0x04
#define WINE_URL_ESCAPE_HASH      0x08
#define WINE_URL_ESCAPE_QUESTION  0x10
#define WINE_URL_STOP_ON_HASH     0x20
#define WINE_URL_STOP_ON_QUESTION 0x40

static inline BOOL URL_NeedEscapeW(WCHAR ch, DWORD flags, DWORD int_flags)
{
    if (flags & URL_ESCAPE_SPACES_ONLY)
        return ch == ' ';

    if ((flags & URL_ESCAPE_PERCENT) && (ch == '%'))
	return TRUE;

    if (ch <= 31 || (ch >= 127 && ch <= 255) )
	return TRUE;

    if (isalnumW(ch))
        return FALSE;

    switch (ch) {
    case ' ':
    case '<':
    case '>':
    case '\"':
    case '{':
    case '}':
    case '|':
    case '\\':
    case '^':
    case ']':
    case '[':
    case '`':
    case '&':
        return TRUE;
    case '/':
        return !!(int_flags & WINE_URL_ESCAPE_SLASH);
    case '?':
        return !!(int_flags & WINE_URL_ESCAPE_QUESTION);
    case '#':
        return !!(int_flags & WINE_URL_ESCAPE_HASH);
    default:
        return FALSE;
    }
}


/*************************************************************************
 *      UrlEscapeW	[SHLWAPI.@]
 *
 * Converts unsafe characters in a Url into escape sequences.
 *
 * PARAMS
 *  pszUrl      [I]   Url to modify
 *  pszEscaped  [O]   Destination for modified Url
 *  pcchEscaped [I/O] Length of pszUrl, destination for length of pszEscaped
 *  dwFlags     [I]   URL_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. pszEscaped contains the escaped Url, pcchEscaped
 *           contains its length.
 *  Failure: E_POINTER, if pszEscaped is not large enough. In this case
 *           pcchEscaped is set to the required length.
 *
 * Converts unsafe characters into their escape sequences.
 *
 * NOTES
 * - By default this function stops converting at the first '?' or
 *  '#' character.
 * - If dwFlags contains URL_ESCAPE_SPACES_ONLY then only spaces are
 *   converted, but the conversion continues past a '?' or '#'.
 * - Note that this function did not work well (or at all) in shlwapi version 4.
 *
 * BUGS
 *  Only the following flags are implemented:
 *|     URL_ESCAPE_SPACES_ONLY
 *|     URL_DONT_ESCAPE_EXTRA_INFO
 *|     URL_ESCAPE_SEGMENT_ONLY
 *|     URL_ESCAPE_PERCENT
 */
HRESULT WINAPI UrlEscapeW(
	LPCWSTR pszUrl,
	LPWSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    LPCWSTR src;
    DWORD needed = 0, ret;
    BOOL stop_escaping = FALSE;
    WCHAR next[5], *dst, *dst_ptr;
    INT len;
    PARSEDURLW parsed_url;
    DWORD int_flags;
    DWORD slashes = 0;
    static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};

    TRACE("(%p(%s) %p %p 0x%08x)\n", pszUrl, debugstr_w(pszUrl),
            pszEscaped, pcchEscaped, dwFlags);

    if(!pszUrl || !pcchEscaped)
        return E_INVALIDARG;

    if(dwFlags & ~(URL_ESCAPE_SPACES_ONLY |
		   URL_ESCAPE_SEGMENT_ONLY |
		   URL_DONT_ESCAPE_EXTRA_INFO |
		   URL_ESCAPE_PERCENT))
        FIXME("Unimplemented flags: %08x\n", dwFlags);

    dst_ptr = dst = HeapAlloc(GetProcessHeap(), 0, *pcchEscaped*sizeof(WCHAR));
    if(!dst_ptr)
        return E_OUTOFMEMORY;

    /* fix up flags */
    if (dwFlags & URL_ESCAPE_SPACES_ONLY)
	/* if SPACES_ONLY specified, reset the other controls */
	dwFlags &= ~(URL_DONT_ESCAPE_EXTRA_INFO |
		     URL_ESCAPE_PERCENT |
		     URL_ESCAPE_SEGMENT_ONLY);

    else
	/* if SPACES_ONLY *not* specified the assume DONT_ESCAPE_EXTRA_INFO */
	dwFlags |= URL_DONT_ESCAPE_EXTRA_INFO;


    int_flags = 0;
    if(dwFlags & URL_ESCAPE_SEGMENT_ONLY) {
        int_flags = WINE_URL_ESCAPE_QUESTION | WINE_URL_ESCAPE_HASH | WINE_URL_ESCAPE_SLASH;
    } else {
        parsed_url.cbSize = sizeof(parsed_url);
        if(ParseURLW(pszUrl, &parsed_url) != S_OK)
            parsed_url.nScheme = URL_SCHEME_INVALID;

        TRACE("scheme = %d (%s)\n", parsed_url.nScheme, debugstr_wn(parsed_url.pszProtocol, parsed_url.cchProtocol));

        if(dwFlags & URL_DONT_ESCAPE_EXTRA_INFO)
            int_flags = WINE_URL_STOP_ON_HASH | WINE_URL_STOP_ON_QUESTION;

        switch(parsed_url.nScheme) {
        case URL_SCHEME_FILE:
            int_flags |= WINE_URL_BASH_AS_SLASH | WINE_URL_COLLAPSE_SLASHES | WINE_URL_ESCAPE_HASH;
            int_flags &= ~WINE_URL_STOP_ON_HASH;
            break;

        case URL_SCHEME_HTTP:
        case URL_SCHEME_HTTPS:
            int_flags |= WINE_URL_BASH_AS_SLASH;
            if(parsed_url.pszSuffix[0] != '/' && parsed_url.pszSuffix[0] != '\\')
                int_flags |= WINE_URL_ESCAPE_SLASH;
            break;

        case URL_SCHEME_MAILTO:
            int_flags |= WINE_URL_ESCAPE_SLASH | WINE_URL_ESCAPE_QUESTION | WINE_URL_ESCAPE_HASH;
            int_flags &= ~(WINE_URL_STOP_ON_QUESTION | WINE_URL_STOP_ON_HASH);
            break;

        case URL_SCHEME_INVALID:
            break;

        case URL_SCHEME_FTP:
        default:
            if(parsed_url.pszSuffix[0] != '/')
                int_flags |= WINE_URL_ESCAPE_SLASH;
            break;
        }
    }

    for(src = pszUrl; *src; ) {
        WCHAR cur = *src;
        len = 0;
        
        if((int_flags & WINE_URL_COLLAPSE_SLASHES) && src == pszUrl + parsed_url.cchProtocol + 1) {
            int localhost_len = sizeof(localhost)/sizeof(WCHAR) - 1;
            while(cur == '/' || cur == '\\') {
                slashes++;
                cur = *++src;
            }
            if(slashes == 2 && !strncmpiW(src, localhost, localhost_len)) { /* file://localhost/ -> file:/// */
                if(*(src + localhost_len) == '/' || *(src + localhost_len) == '\\')
                src += localhost_len + 1;
                slashes = 3;
            }

            switch(slashes) {
            case 1:
            case 3:
                next[0] = next[1] = next[2] = '/';
                len = 3;
                break;
            case 0:
                len = 0;
                break;
            default:
                next[0] = next[1] = '/';
                len = 2;
                break;
            }
        }
        if(len == 0) {

            if(cur == '#' && (int_flags & WINE_URL_STOP_ON_HASH))
                stop_escaping = TRUE;

            if(cur == '?' && (int_flags & WINE_URL_STOP_ON_QUESTION))
                stop_escaping = TRUE;

            if(cur == '\\' && (int_flags & WINE_URL_BASH_AS_SLASH) && !stop_escaping) cur = '/';

            if(URL_NeedEscapeW(cur, dwFlags, int_flags) && stop_escaping == FALSE) {
                next[0] = '%';
                next[1] = hexDigits[(cur >> 4) & 0xf];
                next[2] = hexDigits[cur & 0xf];
                len = 3;
            } else {
                next[0] = cur;
                len = 1;
            }
            src++;
        }

	if(needed + len <= *pcchEscaped) {
	    memcpy(dst, next, len*sizeof(WCHAR));
	    dst += len;
	}
	needed += len;
    }

    if(needed < *pcchEscaped) {
        *dst = '\0';
        memcpy(pszEscaped, dst_ptr, (needed+1)*sizeof(WCHAR));

        ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
        ret = E_POINTER;
    }
    *pcchEscaped = needed;

    HeapFree(GetProcessHeap(), 0, dst_ptr);
    return ret;
}


/*************************************************************************
 *      UrlUnescapeA	[SHLWAPI.@]
 *
 * Converts Url escape sequences back to ordinary characters.
 *
 * PARAMS
 *  pszUrl        [I/O]  Url to convert
 *  pszUnescaped  [O]    Destination for converted Url
 *  pcchUnescaped [I/O]  Size of output string
 *  dwFlags       [I]    URL_ESCAPE_ Flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. The converted value is in pszUnescaped, or in pszUrl if
 *           dwFlags includes URL_ESCAPE_INPLACE.
 *  Failure: E_POINTER if the converted Url is bigger than pcchUnescaped. In
 *           this case pcchUnescaped is set to the size required.
 * NOTES
 *  If dwFlags includes URL_DONT_ESCAPE_EXTRA_INFO, the conversion stops at
 *  the first occurrence of either a '?' or '#' character.
 */
HRESULT WINAPI UrlUnescapeA(
	LPSTR pszUrl,
	LPSTR pszUnescaped,
	LPDWORD pcchUnescaped,
	DWORD dwFlags)
{
    char *dst, next;
    LPCSTR src;
    HRESULT ret;
    DWORD needed;
    BOOL stop_unescaping = FALSE;

    TRACE("(%s, %p, %p, 0x%08x)\n", debugstr_a(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if (!pszUrl) return E_INVALIDARG;

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = pszUrl;
    else
    {
        if (!pszUnescaped || !pcchUnescaped) return E_INVALIDARG;
        dst = pszUnescaped;
    }

    for(src = pszUrl, needed = 0; *src; src++, needed++) {
        if(dwFlags & URL_DONT_UNESCAPE_EXTRA_INFO &&
	   (*src == '#' || *src == '?')) {
	    stop_unescaping = TRUE;
	    next = *src;
	} else if(*src == '%' && isxdigit(*(src + 1)) && isxdigit(*(src + 2))
		  && stop_unescaping == FALSE) {
	    INT ih;
	    char buf[3];
	    memcpy(buf, src + 1, 2);
	    buf[2] = '\0';
	    ih = strtol(buf, NULL, 16);
	    next = (CHAR) ih;
	    src += 2; /* Advance to end of escape */
	} else
	    next = *src;

	if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped)
	    *dst++ = next;
    }

    if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped) {
        *dst = '\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    if(!(dwFlags & URL_UNESCAPE_INPLACE))
        *pcchUnescaped = needed;

    if (ret == S_OK) {
	TRACE("result %s\n", (dwFlags & URL_UNESCAPE_INPLACE) ?
	      debugstr_a(pszUrl) : debugstr_a(pszUnescaped));
    }

    return ret;
}

/*************************************************************************
 *      UrlUnescapeW	[SHLWAPI.@]
 *
 * See UrlUnescapeA.
 */
HRESULT WINAPI UrlUnescapeW(
	LPWSTR pszUrl,
	LPWSTR pszUnescaped,
	LPDWORD pcchUnescaped,
	DWORD dwFlags)
{
    WCHAR *dst, next;
    LPCWSTR src;
    HRESULT ret;
    DWORD needed;
    BOOL stop_unescaping = FALSE;

    TRACE("(%s, %p, %p, 0x%08x)\n", debugstr_w(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if(!pszUrl) return E_INVALIDARG;

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = pszUrl;
    else
    {
        if (!pszUnescaped || !pcchUnescaped) return E_INVALIDARG;
        dst = pszUnescaped;
    }

    for(src = pszUrl, needed = 0; *src; src++, needed++) {
        if(dwFlags & URL_DONT_UNESCAPE_EXTRA_INFO &&
           (*src == '#' || *src == '?')) {
	    stop_unescaping = TRUE;
	    next = *src;
        } else if(*src == '%' && isxdigitW(*(src + 1)) && isxdigitW(*(src + 2))
		  && stop_unescaping == FALSE) {
	    INT ih;
	    WCHAR buf[5] = {'0','x',0};
	    memcpy(buf + 2, src + 1, 2*sizeof(WCHAR));
	    buf[4] = 0;
	    StrToIntExW(buf, STIF_SUPPORT_HEX, &ih);
	    next = (WCHAR) ih;
	    src += 2; /* Advance to end of escape */
	} else
	    next = *src;

	if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped)
	    *dst++ = next;
    }

    if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped) {
        *dst = '\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    if(!(dwFlags & URL_UNESCAPE_INPLACE))
        *pcchUnescaped = needed;

    if (ret == S_OK) {
	TRACE("result %s\n", (dwFlags & URL_UNESCAPE_INPLACE) ?
	      debugstr_w(pszUrl) : debugstr_w(pszUnescaped));
    }

    return ret;
}

/*************************************************************************
 *      UrlGetLocationA 	[SHLWAPI.@]
 *
 * Get the location from a Url.
 *
 * PARAMS
 *  pszUrl [I] Url to get the location from
 *
 * RETURNS
 *  A pointer to the start of the location in pszUrl, or NULL if there is
 *  no location.
 *
 * NOTES
 *  - MSDN erroneously states that "The location is the segment of the Url
 *    starting with a '?' or '#' character". Neither V4 nor V5 of shlwapi.dll
 *    stop at '?' and always return a NULL in this case.
 *  - MSDN also erroneously states that "If a file URL has a query string,
 *    the returned string is the query string". In all tested cases, if the
 *    Url starts with "fi" then a NULL is returned. V5 gives the following results:
 *|       Result   Url
 *|       ------   ---
 *|       NULL     file://aa/b/cd#hohoh
 *|       #hohoh   http://aa/b/cd#hohoh
 *|       NULL     fi://aa/b/cd#hohoh
 *|       #hohoh   ff://aa/b/cd#hohoh
 */
LPCSTR WINAPI UrlGetLocationA(
	LPCSTR pszUrl)
{
    PARSEDURLA base;
    DWORD res1;

    base.cbSize = sizeof(base);
    res1 = ParseURLA(pszUrl, &base);
    if (res1) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (strncmp(base.pszProtocol, "file", min(4,base.cchProtocol)) == 0) return NULL;

    /* Look for '#' and return its addr */
    return strchr(base.pszSuffix, '#');
}

/*************************************************************************
 *      UrlGetLocationW 	[SHLWAPI.@]
 *
 * See UrlGetLocationA.
 */
LPCWSTR WINAPI UrlGetLocationW(
	LPCWSTR pszUrl)
{
    PARSEDURLW base;
    DWORD res1;

    base.cbSize = sizeof(base);
    res1 = ParseURLW(pszUrl, &base);
    if (res1) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (strncmpW(base.pszProtocol, fileW, min(4,base.cchProtocol)) == 0) return NULL;

    /* Look for '#' and return its addr */
    return strchrW(base.pszSuffix, '#');
}

/*************************************************************************
 *      UrlCompareA	[SHLWAPI.@]
 *
 * Compare two Urls.
 *
 * PARAMS
 *  pszUrl1      [I] First Url to compare
 *  pszUrl2      [I] Url to compare to pszUrl1
 *  fIgnoreSlash [I] TRUE = compare only up to a final slash
 *
 * RETURNS
 *  less than zero, zero, or greater than zero indicating pszUrl2 is greater
 *  than, equal to, or less than pszUrl1 respectively.
 */
INT WINAPI UrlCompareA(
	LPCSTR pszUrl1,
	LPCSTR pszUrl2,
	BOOL fIgnoreSlash)
{
    INT ret, len, len1, len2;

    if (!fIgnoreSlash)
	return strcmp(pszUrl1, pszUrl2);
    len1 = strlen(pszUrl1);
    if (pszUrl1[len1-1] == '/') len1--;
    len2 = strlen(pszUrl2);
    if (pszUrl2[len2-1] == '/') len2--;
    if (len1 == len2)
	return strncmp(pszUrl1, pszUrl2, len1);
    len = min(len1, len2);
    ret = strncmp(pszUrl1, pszUrl2, len);
    if (ret) return ret;
    if (len1 > len2) return 1;
    return -1;
}

/*************************************************************************
 *      UrlCompareW	[SHLWAPI.@]
 *
 * See UrlCompareA.
 */
INT WINAPI UrlCompareW(
	LPCWSTR pszUrl1,
	LPCWSTR pszUrl2,
	BOOL fIgnoreSlash)
{
    INT ret;
    size_t len, len1, len2;

    if (!fIgnoreSlash)
	return strcmpW(pszUrl1, pszUrl2);
    len1 = strlenW(pszUrl1);
    if (pszUrl1[len1-1] == '/') len1--;
    len2 = strlenW(pszUrl2);
    if (pszUrl2[len2-1] == '/') len2--;
    if (len1 == len2)
	return strncmpW(pszUrl1, pszUrl2, len1);
    len = min(len1, len2);
    ret = strncmpW(pszUrl1, pszUrl2, len);
    if (ret) return ret;
    if (len1 > len2) return 1;
    return -1;
}

/*************************************************************************
 *      HashData	[SHLWAPI.@]
 *
 * Hash an input block into a variable sized digest.
 *
 * PARAMS
 *  lpSrc    [I] Input block
 *  nSrcLen  [I] Length of lpSrc
 *  lpDest   [I] Output for hash digest
 *  nDestLen [I] Length of lpDest
 *
 * RETURNS
 *  Success: TRUE. lpDest is filled with the computed hash value.
 *  Failure: FALSE, if any argument is invalid.
 */
HRESULT WINAPI HashData(const unsigned char *lpSrc, DWORD nSrcLen,
                     unsigned char *lpDest, DWORD nDestLen)
{
  INT srcCount = nSrcLen - 1, destCount = nDestLen - 1;

  if (!lpSrc || !lpDest)
    return E_INVALIDARG;

  while (destCount >= 0)
  {
    lpDest[destCount] = (destCount & 0xff);
    destCount--;
  }

  while (srcCount >= 0)
  {
    destCount = nDestLen - 1;
    while (destCount >= 0)
    {
      lpDest[destCount] = HashDataLookup[lpSrc[srcCount] ^ lpDest[destCount]];
      destCount--;
    }
    srcCount--;
  }
  return S_OK;
}

/*************************************************************************
 *      UrlHashA	[SHLWAPI.@]
 *
 * Produce a Hash from a Url.
 *
 * PARAMS
 *  pszUrl   [I] Url to hash
 *  lpDest   [O] Destinationh for hash
 *  nDestLen [I] Length of lpDest
 * 
 * RETURNS
 *  Success: S_OK. lpDest is filled with the computed hash value.
 *  Failure: E_INVALIDARG, if any argument is invalid.
 */
HRESULT WINAPI UrlHashA(LPCSTR pszUrl, unsigned char *lpDest, DWORD nDestLen)
{
  if (IsBadStringPtrA(pszUrl, -1) || IsBadWritePtr(lpDest, nDestLen))
    return E_INVALIDARG;

  HashData((const BYTE*)pszUrl, (int)strlen(pszUrl), lpDest, nDestLen);
  return S_OK;
}

/*************************************************************************
 * UrlHashW	[SHLWAPI.@]
 *
 * See UrlHashA.
 */
HRESULT WINAPI UrlHashW(LPCWSTR pszUrl, unsigned char *lpDest, DWORD nDestLen)
{
  char szUrl[MAX_PATH];

  TRACE("(%s,%p,%d)\n",debugstr_w(pszUrl), lpDest, nDestLen);

  if (IsBadStringPtrW(pszUrl, -1) || IsBadWritePtr(lpDest, nDestLen))
    return E_INVALIDARG;

  /* Win32 hashes the data as an ASCII string, presumably so that both A+W
   * return the same digests for the same URL.
   */
  WideCharToMultiByte(CP_ACP, 0, pszUrl, -1, szUrl, MAX_PATH, NULL, NULL);
  HashData((const BYTE*)szUrl, (int)strlen(szUrl), lpDest, nDestLen);
  return S_OK;
}

/*************************************************************************
 *      UrlApplySchemeA	[SHLWAPI.@]
 *
 * Apply a scheme to a Url.
 *
 * PARAMS
 *  pszIn   [I]   Url to apply scheme to
 *  pszOut  [O]   Destination for modified Url
 *  pcchOut [I/O] Length of pszOut/destination for length of pszOut
 *  dwFlags [I]   URL_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK: pszOut contains the modified Url, pcchOut contains its length.
 *  Failure: An HRESULT error code describing the error.
 */
HRESULT WINAPI UrlApplySchemeA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    LPWSTR in, out;
    HRESULT ret;
    DWORD len;

    TRACE("(%s, %p, %p:out size %d, 0x%08x)\n", debugstr_a(pszIn),
            pszOut, pcchOut, pcchOut ? *pcchOut : 0, dwFlags);

    if (!pszIn || !pszOut || !pcchOut) return E_INVALIDARG;

    in = HeapAlloc(GetProcessHeap(), 0,
                  (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    out = in + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, pszIn, -1, in, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    ret = UrlApplySchemeW(in, out, &len, dwFlags);
    if (ret != S_OK) {
        HeapFree(GetProcessHeap(), 0, in);
        return ret;
    }

    len = WideCharToMultiByte(CP_ACP, 0, out, -1, NULL, 0, NULL, NULL);
    if (len > *pcchOut) {
        ret = E_POINTER;
        goto cleanup;
    }

    WideCharToMultiByte(CP_ACP, 0, out, -1, pszOut, *pcchOut, NULL, NULL);
    len--;

cleanup:
    *pcchOut = len;
    HeapFree(GetProcessHeap(), 0, in);
    return ret;
}

static HRESULT URL_GuessScheme(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut)
{
    HKEY newkey;
    BOOL j;
    INT index;
    DWORD value_len, data_len, dwType, i;
    WCHAR reg_path[MAX_PATH];
    WCHAR value[MAX_PATH], data[MAX_PATH];
    WCHAR Wxx, Wyy;

    MultiByteToWideChar(CP_ACP, 0,
	      "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes",
			-1, reg_path, MAX_PATH);
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, reg_path, 0, 1, &newkey);
    index = 0;
    while(value_len = data_len = MAX_PATH,
	  RegEnumValueW(newkey, index, value, &value_len,
			0, &dwType, (LPVOID)data, &data_len) == 0) {
	TRACE("guess %d %s is %s\n",
	      index, debugstr_w(value), debugstr_w(data));

	j = FALSE;
	for(i=0; i<value_len; i++) {
	    Wxx = pszIn[i];
	    Wyy = value[i];
	    /* remember that TRUE is not-equal */
	    j = ChrCmpIW(Wxx, Wyy);
	    if (j) break;
	}
	if ((i == value_len) && !j) {
	    if (strlenW(data) + strlenW(pszIn) + 1 > *pcchOut) {
		*pcchOut = strlenW(data) + strlenW(pszIn) + 1;
		RegCloseKey(newkey);
		return E_POINTER;
	    }
	    strcpyW(pszOut, data);
	    strcatW(pszOut, pszIn);
	    *pcchOut = strlenW(pszOut);
	    TRACE("matched and set to %s\n", debugstr_w(pszOut));
	    RegCloseKey(newkey);
	    return S_OK;
	}
	index++;
    }
    RegCloseKey(newkey);
    return E_FAIL;
}

static HRESULT URL_CreateFromPath(LPCWSTR pszPath, LPWSTR pszUrl, LPDWORD pcchUrl)
{
    DWORD needed;
    HRESULT ret = S_OK;
    WCHAR *pszNewUrl;
    WCHAR file_colonW[] = {'f','i','l','e',':',0};
    WCHAR three_slashesW[] = {'/','/','/',0};
    PARSEDURLW parsed_url;

    parsed_url.cbSize = sizeof(parsed_url);
    if(ParseURLW(pszPath, &parsed_url) == S_OK) {
        if(parsed_url.nScheme != URL_SCHEME_INVALID && parsed_url.cchProtocol > 1) {
            needed = strlenW(pszPath);
            if (needed >= *pcchUrl) {
                *pcchUrl = needed + 1;
                return E_POINTER;
            } else {
                *pcchUrl = needed;
                return S_FALSE;
            }
        }
    }

    pszNewUrl = HeapAlloc(GetProcessHeap(), 0, (strlenW(pszPath) + 9) * sizeof(WCHAR)); /* "file:///" + pszPath_len + 1 */
    strcpyW(pszNewUrl, file_colonW);
    if(isalphaW(pszPath[0]) && pszPath[1] == ':')
        strcatW(pszNewUrl, three_slashesW);
    strcatW(pszNewUrl, pszPath);
    ret = UrlEscapeW(pszNewUrl, pszUrl, pcchUrl, URL_ESCAPE_PERCENT);
    HeapFree(GetProcessHeap(), 0, pszNewUrl);
    return ret;
}

static HRESULT URL_ApplyDefault(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut)
{
    HKEY newkey;
    DWORD data_len, dwType;
    WCHAR data[MAX_PATH];

    static const WCHAR prefix_keyW[] =
        {'S','o','f','t','w','a','r','e',
         '\\','M','i','c','r','o','s','o','f','t',
         '\\','W','i','n','d','o','w','s',
         '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',
         '\\','U','R','L',
         '\\','D','e','f','a','u','l','t','P','r','e','f','i','x',0};

    /* get and prepend default */
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, prefix_keyW, 0, 1, &newkey);
    data_len = sizeof(data);
    RegQueryValueExW(newkey, NULL, 0, &dwType, (LPBYTE)data, &data_len);
    RegCloseKey(newkey);
    if (strlenW(data) + strlenW(pszIn) + 1 > *pcchOut) {
        *pcchOut = strlenW(data) + strlenW(pszIn) + 1;
        return E_POINTER;
    }
    strcpyW(pszOut, data);
    strcatW(pszOut, pszIn);
    *pcchOut = strlenW(pszOut);
    TRACE("used default %s\n", debugstr_w(pszOut));
    return S_OK;
}

/*************************************************************************
 *      UrlApplySchemeW	[SHLWAPI.@]
 *
 * See UrlApplySchemeA.
 */
HRESULT WINAPI UrlApplySchemeW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    PARSEDURLW in_scheme;
    DWORD res1;
    HRESULT ret;

    TRACE("(%s, %p, %p:out size %d, 0x%08x)\n", debugstr_w(pszIn),
            pszOut, pcchOut, pcchOut ? *pcchOut : 0, dwFlags);

    if (!pszIn || !pszOut || !pcchOut) return E_INVALIDARG;

    if (dwFlags & URL_APPLY_GUESSFILE) {
        if (*pcchOut > 1 && ':' == pszIn[1]) {
            res1 = *pcchOut;
            ret = URL_CreateFromPath(pszIn, pszOut, &res1);
            if (ret == S_OK || ret == E_POINTER){
                *pcchOut = res1;
                return ret;
            }
            else if (ret == S_FALSE)
            {
                return ret;
            }
        }
    }

    in_scheme.cbSize = sizeof(in_scheme);
    /* See if the base has a scheme */
    res1 = ParseURLW(pszIn, &in_scheme);
    if (res1) {
	/* no scheme in input, need to see if we need to guess */
	if (dwFlags & URL_APPLY_GUESSSCHEME) {
	    if ((ret = URL_GuessScheme(pszIn, pszOut, pcchOut)) != E_FAIL)
		return ret;
	}
    }

    /* If we are here, then either invalid scheme,
     * or no scheme and can't/failed guess.
     */
    if ( ( ((res1 == 0) && (dwFlags & URL_APPLY_FORCEAPPLY)) ||
	   ((res1 != 0)) ) &&
	 (dwFlags & URL_APPLY_DEFAULT)) {
	/* find and apply default scheme */
	return URL_ApplyDefault(pszIn, pszOut, pcchOut);
    }

    return S_FALSE;
}

/*************************************************************************
 *      UrlIsA  	[SHLWAPI.@]
 *
 * Determine if a Url is of a certain class.
 *
 * PARAMS
 *  pszUrl [I] Url to check
 *  Urlis  [I] URLIS_ constant from "shlwapi.h"
 *
 * RETURNS
 *  TRUE if pszUrl belongs to the class type in Urlis.
 *  FALSE Otherwise.
 */
BOOL WINAPI UrlIsA(LPCSTR pszUrl, URLIS Urlis)
{
    PARSEDURLA base;
    DWORD res1;
    LPCSTR last;

    TRACE("(%s %d)\n", debugstr_a(pszUrl), Urlis);

    if(!pszUrl)
        return FALSE;

    switch (Urlis) {

    case URLIS_OPAQUE:
	base.cbSize = sizeof(base);
	res1 = ParseURLA(pszUrl, &base);
	if (res1) return FALSE;  /* invalid scheme */
	switch (base.nScheme)
	{
	case URL_SCHEME_MAILTO:
	case URL_SCHEME_SHELL:
	case URL_SCHEME_JAVASCRIPT:
	case URL_SCHEME_VBSCRIPT:
	case URL_SCHEME_ABOUT:
	    return TRUE;
	}
	return FALSE;

    case URLIS_FILEURL:
        return (CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, pszUrl, 5,
                               "file:", 5) == CSTR_EQUAL);

    case URLIS_DIRECTORY:
        last = pszUrl + strlen(pszUrl) - 1;
        return (last >= pszUrl && (*last == '/' || *last == '\\' ));

    case URLIS_URL:
        return PathIsURLA(pszUrl);

    case URLIS_NOHISTORY:
    case URLIS_APPLIABLE:
    case URLIS_HASQUERY:
    default:
	FIXME("(%s %d): stub\n", debugstr_a(pszUrl), Urlis);
    }
    return FALSE;
}

/*************************************************************************
 *      UrlIsW  	[SHLWAPI.@]
 *
 * See UrlIsA.
 */
BOOL WINAPI UrlIsW(LPCWSTR pszUrl, URLIS Urlis)
{
    static const WCHAR file_colon[] = { 'f','i','l','e',':',0 };
    PARSEDURLW base;
    DWORD res1;
    LPCWSTR last;

    TRACE("(%s %d)\n", debugstr_w(pszUrl), Urlis);

    if(!pszUrl)
        return FALSE;

    switch (Urlis) {

    case URLIS_OPAQUE:
	base.cbSize = sizeof(base);
	res1 = ParseURLW(pszUrl, &base);
	if (res1) return FALSE;  /* invalid scheme */
	switch (base.nScheme)
	{
	case URL_SCHEME_MAILTO:
	case URL_SCHEME_SHELL:
	case URL_SCHEME_JAVASCRIPT:
	case URL_SCHEME_VBSCRIPT:
	case URL_SCHEME_ABOUT:
	    return TRUE;
	}
	return FALSE;

    case URLIS_FILEURL:
        return (CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, pszUrl, 5,
                               file_colon, 5) == CSTR_EQUAL);

    case URLIS_DIRECTORY:
        last = pszUrl + strlenW(pszUrl) - 1;
        return (last >= pszUrl && (*last == '/' || *last == '\\'));

    case URLIS_URL:
        return PathIsURLW(pszUrl);

    case URLIS_NOHISTORY:
    case URLIS_APPLIABLE:
    case URLIS_HASQUERY:
    default:
	FIXME("(%s %d): stub\n", debugstr_w(pszUrl), Urlis);
    }
    return FALSE;
}

/*************************************************************************
 *      UrlIsNoHistoryA  	[SHLWAPI.@]
 *
 * Determine if a Url should not be stored in the users history list.
 *
 * PARAMS
 *  pszUrl [I] Url to check
 *
 * RETURNS
 *  TRUE, if pszUrl should be excluded from the history list,
 *  FALSE otherwise.
 */
BOOL WINAPI UrlIsNoHistoryA(LPCSTR pszUrl)
{
    return UrlIsA(pszUrl, URLIS_NOHISTORY);
}

/*************************************************************************
 *      UrlIsNoHistoryW  	[SHLWAPI.@]
 *
 * See UrlIsNoHistoryA.
 */
BOOL WINAPI UrlIsNoHistoryW(LPCWSTR pszUrl)
{
    return UrlIsW(pszUrl, URLIS_NOHISTORY);
}

/*************************************************************************
 *      UrlIsOpaqueA  	[SHLWAPI.@]
 *
 * Determine if a Url is opaque.
 *
 * PARAMS
 *  pszUrl [I] Url to check
 *
 * RETURNS
 *  TRUE if pszUrl is opaque,
 *  FALSE Otherwise.
 *
 * NOTES
 *  An opaque Url is one that does not start with "<protocol>://".
 */
BOOL WINAPI UrlIsOpaqueA(LPCSTR pszUrl)
{
    return UrlIsA(pszUrl, URLIS_OPAQUE);
}

/*************************************************************************
 *      UrlIsOpaqueW  	[SHLWAPI.@]
 *
 * See UrlIsOpaqueA.
 */
BOOL WINAPI UrlIsOpaqueW(LPCWSTR pszUrl)
{
    return UrlIsW(pszUrl, URLIS_OPAQUE);
}

/*************************************************************************
 *  Scans for characters of type "type" and when not matching found,
 *  returns pointer to it and length in size.
 *
 * Characters tested based on RFC 1738
 */
static LPCWSTR  URL_ScanID(LPCWSTR start, LPDWORD size, WINE_URL_SCAN_TYPE type)
{
    static DWORD alwayszero = 0;
    BOOL cont = TRUE;

    *size = 0;

    switch(type){

    case SCHEME:
	while (cont) {
	    if ( (islowerW(*start) && isalphaW(*start)) ||
		 isdigitW(*start) ||
                 (*start == '+') ||
                 (*start == '-') ||
                 (*start == '.')) {
		start++;
		(*size)++;
	    }
	    else
		cont = FALSE;
	}

	if(*start != ':')
	    *size = 0;

        break;

    case USERPASS:
	while (cont) {
	    if ( isalphaW(*start) ||
		 isdigitW(*start) ||
		 /* user/password only characters */
                 (*start == ';') ||
                 (*start == '?') ||
                 (*start == '&') ||
                 (*start == '=') ||
		 /* *extra* characters */
                 (*start == '!') ||
                 (*start == '*') ||
                 (*start == '\'') ||
                 (*start == '(') ||
                 (*start == ')') ||
                 (*start == ',') ||
		 /* *safe* characters */
                 (*start == '$') ||
                 (*start == '_') ||
                 (*start == '+') ||
                 (*start == '-') ||
                 (*start == '.') ||
                 (*start == ' ')) {
		start++;
		(*size)++;
            } else if (*start == '%') {
		if (isxdigitW(*(start+1)) &&
		    isxdigitW(*(start+2))) {
		    start += 3;
		    *size += 3;
		} else
		    cont = FALSE;
	    } else
		cont = FALSE;
	}
	break;

    case PORT:
	while (cont) {
	    if (isdigitW(*start)) {
		start++;
		(*size)++;
	    }
	    else
		cont = FALSE;
	}
	break;

    case HOST:
	while (cont) {
	    if (isalnumW(*start) ||
                (*start == '-') ||
                (*start == '.') ||
                (*start == ' ') ||
                (*start == '*') ) {
		start++;
		(*size)++;
	    }
	    else
		cont = FALSE;
	}
	break;
    default:
	FIXME("unknown type %d\n", type);
	return (LPWSTR)&alwayszero;
    }
    /* TRACE("scanned %d characters next char %p<%c>\n",
     *size, start, *start); */
    return start;
}

/*************************************************************************
 *  Attempt to parse URL into pieces.
 */
static LONG URL_ParseUrl(LPCWSTR pszUrl, WINE_PARSE_URL *pl)
{
    LPCWSTR work;

    memset(pl, 0, sizeof(WINE_PARSE_URL));
    pl->pScheme = pszUrl;
    work = URL_ScanID(pl->pScheme, &pl->szScheme, SCHEME);
    if (!*work || (*work != ':')) goto ErrorExit;
    work++;
    if ((*work != '/') || (*(work+1) != '/')) goto SuccessExit;
    pl->pUserName = work + 2;
    work = URL_ScanID(pl->pUserName, &pl->szUserName, USERPASS);
    if (*work == ':' ) {
	/* parse password */
	work++;
	pl->pPassword = work;
	work = URL_ScanID(pl->pPassword, &pl->szPassword, USERPASS);
        if (*work != '@') {
	    /* what we just parsed must be the hostname and port
	     * so reset pointers and clear then let it parse */
	    pl->szUserName = pl->szPassword = 0;
	    work = pl->pUserName - 1;
	    pl->pUserName = pl->pPassword = 0;
	}
    } else if (*work == '@') {
	/* no password */
	pl->szPassword = 0;
	pl->pPassword = 0;
    } else if (!*work || (*work == '/') || (*work == '.')) {
	/* what was parsed was hostname, so reset pointers and let it parse */
	pl->szUserName = pl->szPassword = 0;
	work = pl->pUserName - 1;
	pl->pUserName = pl->pPassword = 0;
    } else goto ErrorExit;

    /* now start parsing hostname or hostnumber */
    work++;
    pl->pHostName = work;
    work = URL_ScanID(pl->pHostName, &pl->szHostName, HOST);
    if (*work == ':') {
	/* parse port */
	work++;
	pl->pPort = work;
	work = URL_ScanID(pl->pPort, &pl->szPort, PORT);
    }
    if (*work == '/') {
	/* see if query string */
        pl->pQuery = strchrW(work, '?');
	if (pl->pQuery) pl->szQuery = strlenW(pl->pQuery);
    }
  SuccessExit:
    TRACE("parse successful: scheme=%p(%d), user=%p(%d), pass=%p(%d), host=%p(%d), port=%p(%d), query=%p(%d)\n",
	  pl->pScheme, pl->szScheme,
	  pl->pUserName, pl->szUserName,
	  pl->pPassword, pl->szPassword,
	  pl->pHostName, pl->szHostName,
	  pl->pPort, pl->szPort,
	  pl->pQuery, pl->szQuery);
    return S_OK;
  ErrorExit:
    FIXME("failed to parse %s\n", debugstr_w(pszUrl));
    return E_INVALIDARG;
}

/*************************************************************************
 *      UrlGetPartA  	[SHLWAPI.@]
 *
 * Retrieve part of a Url.
 *
 * PARAMS
 *  pszIn   [I]   Url to parse
 *  pszOut  [O]   Destination for part of pszIn requested
 *  pcchOut [I]   Size of pszOut
 *          [O]   length of pszOut string EXCLUDING '\0' if S_OK, otherwise
 *                needed size of pszOut INCLUDING '\0'.
 *  dwPart  [I]   URL_PART_ enum from "shlwapi.h"
 *  dwFlags [I]   URL_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. pszOut contains the part requested, pcchOut contains its length.
 *  Failure: An HRESULT error code describing the error.
 */
HRESULT WINAPI UrlGetPartA(LPCSTR pszIn, LPSTR pszOut, LPDWORD pcchOut,
			   DWORD dwPart, DWORD dwFlags)
{
    LPWSTR in, out;
    DWORD ret, len, len2;

    if(!pszIn || !pszOut || !pcchOut || *pcchOut <= 0)
        return E_INVALIDARG;

    in = HeapAlloc(GetProcessHeap(), 0,
			      (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    out = in + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(CP_ACP, 0, pszIn, -1, in, INTERNET_MAX_URL_LENGTH);

    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlGetPartW(in, out, &len, dwPart, dwFlags);

    if (FAILED(ret)) {
	HeapFree(GetProcessHeap(), 0, in);
	return ret;
    }

    len2 = WideCharToMultiByte(CP_ACP, 0, out, len, NULL, 0, NULL, NULL);
    if (len2 > *pcchOut) {
	*pcchOut = len2+1;
	HeapFree(GetProcessHeap(), 0, in);
	return E_POINTER;
    }
    len2 = WideCharToMultiByte(CP_ACP, 0, out, len+1, pszOut, *pcchOut, NULL, NULL);
    *pcchOut = len2-1;
    HeapFree(GetProcessHeap(), 0, in);
    return ret;
}

/*************************************************************************
 *      UrlGetPartW  	[SHLWAPI.@]
 *
 * See UrlGetPartA.
 */
HRESULT WINAPI UrlGetPartW(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut,
			   DWORD dwPart, DWORD dwFlags)
{
    WINE_PARSE_URL pl;
    HRESULT ret;
    DWORD scheme, size, schsize;
    LPCWSTR addr, schaddr;

    TRACE("(%s %p %p(%d) %08x %08x)\n",
	  debugstr_w(pszIn), pszOut, pcchOut, *pcchOut, dwPart, dwFlags);

    if(!pszIn || !pszOut || !pcchOut || *pcchOut <= 0)
        return E_INVALIDARG;

    *pszOut = '\0';

    addr = strchrW(pszIn, ':');
    if(!addr)
        scheme = URL_SCHEME_UNKNOWN;
    else
        scheme = get_scheme_code(pszIn, addr-pszIn);

    ret = URL_ParseUrl(pszIn, &pl);

	switch (dwPart) {
	case URL_PART_SCHEME:
	    if (!pl.szScheme) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pScheme;
	    size = pl.szScheme;
	    break;

	case URL_PART_HOSTNAME:
            switch(scheme) {
            case URL_SCHEME_FTP:
            case URL_SCHEME_HTTP:
            case URL_SCHEME_GOPHER:
            case URL_SCHEME_TELNET:
            case URL_SCHEME_FILE:
            case URL_SCHEME_HTTPS:
                break;
            default:
                *pcchOut = 0;
                return E_FAIL;
            }

            if(scheme==URL_SCHEME_FILE && (!pl.szHostName ||
                        (pl.szHostName==1 && *(pl.pHostName+1)==':'))) {
                *pcchOut = 0;
                return S_FALSE;
            }

	    if (!pl.szHostName) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pHostName;
	    size = pl.szHostName;
	    break;

	case URL_PART_USERNAME:
	    if (!pl.szUserName) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pUserName;
	    size = pl.szUserName;
	    break;

	case URL_PART_PASSWORD:
	    if (!pl.szPassword) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pPassword;
	    size = pl.szPassword;
	    break;

	case URL_PART_PORT:
	    if (!pl.szPort) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pPort;
	    size = pl.szPort;
	    break;

	case URL_PART_QUERY:
	    if (!pl.szQuery) {
	        *pcchOut = 0;
	        return S_FALSE;
	    }
	    addr = pl.pQuery;
	    size = pl.szQuery;
	    break;

	default:
	    *pcchOut = 0;
	    return E_INVALIDARG;
	}

	if (dwFlags == URL_PARTFLAG_KEEPSCHEME) {
            if(!pl.pScheme || !pl.szScheme) {
                *pcchOut = 0;
                return E_FAIL;
            }
            schaddr = pl.pScheme;
            schsize = pl.szScheme;
            if (*pcchOut < schsize + size + 2) {
                *pcchOut = schsize + size + 2;
                return E_POINTER;
            }
            memcpy(pszOut, schaddr, schsize*sizeof(WCHAR));
            pszOut[schsize] = ':';
            memcpy(pszOut+schsize+1, addr, size*sizeof(WCHAR));
            pszOut[schsize+1+size] = 0;
            *pcchOut = schsize + 1 + size;
	}
	else {
	    if (*pcchOut < size + 1) {*pcchOut = size+1; return E_POINTER;}
            memcpy(pszOut, addr, size*sizeof(WCHAR));
            pszOut[size] = 0;
	    *pcchOut = size;
	}
	TRACE("len=%d %s\n", *pcchOut, debugstr_w(pszOut));

    return ret;
}

/*************************************************************************
 * PathIsURLA	[SHLWAPI.@]
 *
 * Check if the given path is a Url.
 *
 * PARAMS
 *  lpszPath [I] Path to check.
 *
 * RETURNS
 *  TRUE  if lpszPath is a Url.
 *  FALSE if lpszPath is NULL or not a Url.
 */
BOOL WINAPI PathIsURLA(LPCSTR lpstrPath)
{
    PARSEDURLA base;
    HRESULT hres;

    TRACE("%s\n", debugstr_a(lpstrPath));

    if (!lpstrPath || !*lpstrPath) return FALSE;

    /* get protocol        */
    base.cbSize = sizeof(base);
    hres = ParseURLA(lpstrPath, &base);
    return hres == S_OK && (base.nScheme != URL_SCHEME_INVALID);
}

/*************************************************************************
 * PathIsURLW	[SHLWAPI.@]
 *
 * See PathIsURLA.
 */
BOOL WINAPI PathIsURLW(LPCWSTR lpstrPath)
{
    PARSEDURLW base;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(lpstrPath));

    if (!lpstrPath || !*lpstrPath) return FALSE;

    /* get protocol        */
    base.cbSize = sizeof(base);
    hres = ParseURLW(lpstrPath, &base);
    return hres == S_OK && (base.nScheme != URL_SCHEME_INVALID);
}

/*************************************************************************
 *      UrlCreateFromPathA  	[SHLWAPI.@]
 * 
 * See UrlCreateFromPathW
 */
HRESULT WINAPI UrlCreateFromPathA(LPCSTR pszPath, LPSTR pszUrl, LPDWORD pcchUrl, DWORD dwReserved)
{
    WCHAR bufW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW = bufW;
    UNICODE_STRING pathW;
    HRESULT ret;
    DWORD lenW = sizeof(bufW)/sizeof(WCHAR), lenA;

    if(!RtlCreateUnicodeStringFromAsciiz(&pathW, pszPath))
        return E_INVALIDARG;
    if((ret = UrlCreateFromPathW(pathW.Buffer, urlW, &lenW, dwReserved)) == E_POINTER) {
        urlW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        ret = UrlCreateFromPathW(pathW.Buffer, urlW, &lenW, dwReserved);
    }
    if(ret == S_OK || ret == S_FALSE) {
        RtlUnicodeToMultiByteSize(&lenA, urlW, lenW * sizeof(WCHAR));
        if(*pcchUrl > lenA) {
            RtlUnicodeToMultiByteN(pszUrl, *pcchUrl - 1, &lenA, urlW, lenW * sizeof(WCHAR));
            pszUrl[lenA] = 0;
            *pcchUrl = lenA;
        } else {
            *pcchUrl = lenA + 1;
            ret = E_POINTER;
        }
    }
    if(urlW != bufW) HeapFree(GetProcessHeap(), 0, urlW);
    RtlFreeUnicodeString(&pathW);
    return ret;
}

/*************************************************************************
 *      UrlCreateFromPathW  	[SHLWAPI.@]
 *
 * Create a Url from a file path.
 *
 * PARAMS
 *  pszPath [I]    Path to convert
 *  pszUrl  [O]    Destination for the converted Url
 *  pcchUrl [I/O]  Length of pszUrl
 *  dwReserved [I] Reserved, must be 0
 *
 * RETURNS
 *  Success: S_OK pszUrl contains the converted path, S_FALSE if the path is already a Url
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI UrlCreateFromPathW(LPCWSTR pszPath, LPWSTR pszUrl, LPDWORD pcchUrl, DWORD dwReserved)
{
    HRESULT ret;

    TRACE("(%s, %p, %p, 0x%08x)\n", debugstr_w(pszPath), pszUrl, pcchUrl, dwReserved);

    /* Validate arguments */
    if (dwReserved != 0)
        return E_INVALIDARG;
    if (!pszUrl || !pcchUrl)
        return E_INVALIDARG;

    ret = URL_CreateFromPath(pszPath, pszUrl, pcchUrl);

    if (S_FALSE == ret)
        strcpyW(pszUrl, pszPath);

    return ret;
}

/*************************************************************************
 *      SHAutoComplete  	[SHLWAPI.@]
 *
 * Enable auto-completion for an edit control.
 *
 * PARAMS
 *  hwndEdit [I] Handle of control to enable auto-completion for
 *  dwFlags  [I] SHACF_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. Auto-completion is enabled for the control.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
  FIXME("stub\n");
  return S_FALSE;
}

/*************************************************************************
 *  MLBuildResURLA	[SHLWAPI.405]
 *
 * Create a Url pointing to a resource in a module.
 *
 * PARAMS
 *  lpszLibName [I] Name of the module containing the resource
 *  hMod        [I] Callers module handle
 *  dwFlags     [I] Undocumented flags for loading the module
 *  lpszRes     [I] Resource name
 *  lpszDest    [O] Destination for resulting Url
 *  dwDestLen   [I] Length of lpszDest
 *
 * RETURNS
 *  Success: S_OK. lpszDest contains the resource Url.
 *  Failure: E_INVALIDARG, if any argument is invalid, or
 *           E_FAIL if dwDestLen is too small.
 */
HRESULT WINAPI MLBuildResURLA(LPCSTR lpszLibName, HMODULE hMod, DWORD dwFlags,
                              LPCSTR lpszRes, LPSTR lpszDest, DWORD dwDestLen)
{
  WCHAR szLibName[MAX_PATH], szRes[MAX_PATH], szDest[MAX_PATH];
  HRESULT hRet;

  if (lpszLibName)
    MultiByteToWideChar(CP_ACP, 0, lpszLibName, -1, szLibName, sizeof(szLibName)/sizeof(WCHAR));

  if (lpszRes)
    MultiByteToWideChar(CP_ACP, 0, lpszRes, -1, szRes, sizeof(szRes)/sizeof(WCHAR));

  if (dwDestLen > sizeof(szLibName)/sizeof(WCHAR))
    dwDestLen = sizeof(szLibName)/sizeof(WCHAR);

  hRet = MLBuildResURLW(lpszLibName ? szLibName : NULL, hMod, dwFlags,
                        lpszRes ? szRes : NULL, lpszDest ? szDest : NULL, dwDestLen);
  if (SUCCEEDED(hRet) && lpszDest)
    WideCharToMultiByte(CP_ACP, 0, szDest, -1, lpszDest, dwDestLen, NULL, NULL);

  return hRet;
}

/*************************************************************************
 *  MLBuildResURLA	[SHLWAPI.406]
 *
 * See MLBuildResURLA.
 */
HRESULT WINAPI MLBuildResURLW(LPCWSTR lpszLibName, HMODULE hMod, DWORD dwFlags,
                              LPCWSTR lpszRes, LPWSTR lpszDest, DWORD dwDestLen)
{
  static const WCHAR szRes[] = { 'r','e','s',':','/','/','\0' };
#define szResLen ((sizeof(szRes) - sizeof(WCHAR))/sizeof(WCHAR))
  HRESULT hRet = E_FAIL;

  TRACE("(%s,%p,0x%08x,%s,%p,%d)\n", debugstr_w(lpszLibName), hMod, dwFlags,
        debugstr_w(lpszRes), lpszDest, dwDestLen);

  if (!lpszLibName || !hMod || hMod == INVALID_HANDLE_VALUE || !lpszRes ||
      !lpszDest || (dwFlags && dwFlags != 2))
    return E_INVALIDARG;

  if (dwDestLen >= szResLen + 1)
  {
    dwDestLen -= (szResLen + 1);
    memcpy(lpszDest, szRes, sizeof(szRes));

    hMod = MLLoadLibraryW(lpszLibName, hMod, dwFlags);

    if (hMod)
    {
      WCHAR szBuff[MAX_PATH];
      DWORD len;

      len = GetModuleFileNameW(hMod, szBuff, sizeof(szBuff)/sizeof(WCHAR));
      if (len && len < sizeof(szBuff)/sizeof(WCHAR))
      {
        DWORD dwPathLen = strlenW(szBuff) + 1;

        if (dwDestLen >= dwPathLen)
        {
          DWORD dwResLen;

          dwDestLen -= dwPathLen;
          memcpy(lpszDest + szResLen, szBuff, dwPathLen * sizeof(WCHAR));

          dwResLen = strlenW(lpszRes) + 1;
          if (dwDestLen >= dwResLen + 1)
          {
            lpszDest[szResLen + dwPathLen-1] = '/';
            memcpy(lpszDest + szResLen + dwPathLen, lpszRes, dwResLen * sizeof(WCHAR));
            hRet = S_OK;
          }
        }
      }
      MLFreeLibrary(hMod);
    }
  }
  return hRet;
}

/***********************************************************************
 *             UrlFixupW [SHLWAPI.462]
 *
 * Checks the scheme part of a URL and attempts to correct misspellings.
 *
 * PARAMS
 *  lpszUrl           [I] Pointer to the URL to be corrected
 *  lpszTranslatedUrl [O] Pointer to a buffer to store corrected URL
 *  dwMaxChars        [I] Maximum size of corrected URL
 *
 * RETURNS
 *  success: S_OK if URL corrected or already correct
 *  failure: S_FALSE if unable to correct / COM error code if other error
 *
 */
HRESULT WINAPI UrlFixupW(LPCWSTR url, LPWSTR translatedUrl, DWORD maxChars)
{
    DWORD srcLen;

    FIXME("(%s,%p,%d) STUB\n", debugstr_w(url), translatedUrl, maxChars);

    if (!url)
        return E_FAIL;

    srcLen = lstrlenW(url) + 1;

    /* For now just copy the URL directly */
    lstrcpynW(translatedUrl, url, (maxChars < srcLen) ? maxChars : srcLen);

    return S_OK;
}

/*************************************************************************
 * IsInternetESCEnabled [SHLWAPI.@]
 */
BOOL WINAPI IsInternetESCEnabled(void)
{
    FIXME(": stub\n");
    return FALSE;
}
