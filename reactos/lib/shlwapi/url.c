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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wine/unicode.h"
#include "wininet.h"
#include "winreg.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wine/debug.h"

HMODULE WINAPI MLLoadLibraryW(LPCWSTR,HMODULE,DWORD);
BOOL    WINAPI MLFreeLibrary(HMODULE);
HRESULT WINAPI MLBuildResURLW(LPCWSTR,HMODULE,DWORD,LPCWSTR,LPWSTR,DWORD);

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* The following schemes were identified in the native version of
 * SHLWAPI.DLL version 5.50
 */
typedef enum {
    URL_SCHEME_INVALID     = -1,
    URL_SCHEME_UNKNOWN     =  0,
    URL_SCHEME_FTP,
    URL_SCHEME_HTTP,
    URL_SCHEME_GOPHER,
    URL_SCHEME_MAILTO,
    URL_SCHEME_NEWS,
    URL_SCHEME_NNTP,
    URL_SCHEME_TELNET,
    URL_SCHEME_WAIS,
    URL_SCHEME_FILE,
    URL_SCHEME_MK,
    URL_SCHEME_HTTPS,
    URL_SCHEME_SHELL,
    URL_SCHEME_SNEWS,
    URL_SCHEME_LOCAL,
    URL_SCHEME_JAVASCRIPT,
    URL_SCHEME_VBSCRIPT,
    URL_SCHEME_ABOUT,
    URL_SCHEME_RES,
    URL_SCHEME_MAXVALUE
} URL_SCHEME;

typedef struct {
    URL_SCHEME  scheme_number;
    LPCSTR scheme_name;
} SHL_2_inet_scheme;

static const SHL_2_inet_scheme shlwapi_schemes[] = {
  {URL_SCHEME_FTP,        "ftp"},
  {URL_SCHEME_HTTP,       "http"},
  {URL_SCHEME_GOPHER,     "gopher"},
  {URL_SCHEME_MAILTO,     "mailto"},
  {URL_SCHEME_NEWS,       "news"},
  {URL_SCHEME_NNTP,       "nntp"},
  {URL_SCHEME_TELNET,     "telnet"},
  {URL_SCHEME_WAIS,       "wais"},
  {URL_SCHEME_FILE,       "file"},
  {URL_SCHEME_MK,         "mk"},
  {URL_SCHEME_HTTPS,      "https"},
  {URL_SCHEME_SHELL,      "shell"},
  {URL_SCHEME_SNEWS,      "snews"},
  {URL_SCHEME_LOCAL,      "local"},
  {URL_SCHEME_JAVASCRIPT, "javascript"},
  {URL_SCHEME_VBSCRIPT,   "vbscript"},
  {URL_SCHEME_ABOUT,      "about"},
  {URL_SCHEME_RES,        "res"},
  {0, 0}
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

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCSTR  ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCSTR  ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_1;

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCWSTR ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCWSTR ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_2;

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

static inline BOOL URL_NeedEscapeA(CHAR ch, DWORD dwFlags)
{

    if (isalnum(ch))
        return FALSE;

    if(dwFlags & URL_ESCAPE_SPACES_ONLY) {
        if(ch == ' ')
	    return TRUE;
	else
	    return FALSE;
    }

    if ((dwFlags & URL_ESCAPE_PERCENT) && (ch == '%'))
	return TRUE;

    if (ch <= 31 || ch >= 127)
	return TRUE;

    else {
        switch (ch) {
	case ' ':
	case '<':
	case '>':
	case '\"':
	case '{':
	case '}':
	case '|':
/*	case '\\': */
	case '^':
	case ']':
	case '[':
	case '`':
	case '&':
	    return TRUE;

	case '/':
	case '?':
	    if (dwFlags & URL_ESCAPE_SEGMENT_ONLY) return TRUE;
	default:
	    return FALSE;
	}
    }
}

static inline BOOL URL_NeedEscapeW(WCHAR ch, DWORD dwFlags)
{

    if (isalnumW(ch))
        return FALSE;

    if(dwFlags & URL_ESCAPE_SPACES_ONLY) {
        if(ch == L' ')
	    return TRUE;
	else
	    return FALSE;
    }

    if ((dwFlags & URL_ESCAPE_PERCENT) && (ch == L'%'))
	return TRUE;

    if (ch <= 31 || ch >= 127)
	return TRUE;

    else {
        switch (ch) {
	case L' ':
	case L'<':
	case L'>':
	case L'\"':
	case L'{':
	case L'}':
	case L'|':
	case L'\\':
	case L'^':
	case L']':
	case L'[':
	case L'`':
	case L'&':
	    return TRUE;

	case L'/':
	case L'?':
	    if (dwFlags & URL_ESCAPE_SEGMENT_ONLY) return TRUE;
	default:
	    return FALSE;
	}
    }
}

static BOOL URL_JustLocation(LPCWSTR str)
{
    while(*str && (*str == L'/')) str++;
    if (*str) {
	while (*str && ((*str == L'-') ||
			(*str == L'.') ||
			isalnumW(*str))) str++;
	if (*str == L'/') return FALSE;
    }
    return TRUE;
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
DWORD WINAPI ParseURLA(LPCSTR x, UNKNOWN_SHLWAPI_1 *y)
{
    DWORD cnt;
    const SHL_2_inet_scheme *inet_pro;

    y->fcncde = URL_SCHEME_INVALID;
    if (y->size != 0x18) return E_INVALIDARG;
    /* FIXME: leading white space generates error of 0x80041001 which
     *        is undefined
     */
    if (*x <= ' ') return 0x80041001;
    cnt = 0;
    y->sizep1 = 0;
    y->ap1 = x;
    while (*x) {
	if (*x == ':') {
	    y->sizep1 = cnt;
	    cnt = -1;
	    y->ap2 = x+1;
	    break;
	}
	x++;
	cnt++;
    }

    /* check for no scheme in string start */
    /* (apparently schemes *must* be larger than a single character)  */
    if ((*x == '\0') || (y->sizep1 <= 1)) {
	y->ap1 = 0;
	return 0x80041001;
    }

    /* found scheme, set length of remainder */
    y->sizep2 = lstrlenA(y->ap2);

    /* see if known scheme and return indicator number */
    y->fcncde = URL_SCHEME_UNKNOWN;
    inet_pro = shlwapi_schemes;
    while (inet_pro->scheme_name) {
	if (!strncasecmp(inet_pro->scheme_name, y->ap1,
		    min(y->sizep1, lstrlenA(inet_pro->scheme_name)))) {
	    y->fcncde = inet_pro->scheme_number;
	    break;
	}
	inet_pro++;
    }
    return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.2]
 *
 * Unicode version of ParseURLA.
 */
DWORD WINAPI ParseURLW(LPCWSTR x, UNKNOWN_SHLWAPI_2 *y)
{
    DWORD cnt;
    const SHL_2_inet_scheme *inet_pro;
    LPSTR cmpstr;
    INT len;

    y->fcncde = URL_SCHEME_INVALID;
    if (y->size != 0x18) return E_INVALIDARG;
    /* FIXME: leading white space generates error of 0x80041001 which
     *        is undefined
     */
    if (*x <= L' ') return 0x80041001;
    cnt = 0;
    y->sizep1 = 0;
    y->ap1 = x;
    while (*x) {
	if (*x == L':') {
	    y->sizep1 = cnt;
	    cnt = -1;
	    y->ap2 = x+1;
	    break;
	}
	x++;
	cnt++;
    }

    /* check for no scheme in string start */
    /* (apparently schemes *must* be larger than a single character)  */
    if ((*x == L'\0') || (y->sizep1 <= 1)) {
	y->ap1 = 0;
	return 0x80041001;
    }

    /* found scheme, set length of remainder */
    y->sizep2 = lstrlenW(y->ap2);

    /* see if known scheme and return indicator number */
    len = WideCharToMultiByte(0, 0, y->ap1, y->sizep1, 0, 0, 0, 0);
    cmpstr = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len+1);
    WideCharToMultiByte(0, 0, y->ap1, y->sizep1, cmpstr, len+1, 0, 0);
    y->fcncde = URL_SCHEME_UNKNOWN;
    inet_pro = shlwapi_schemes;
    while (inet_pro->scheme_name) {
	if (!strncasecmp(inet_pro->scheme_name, cmpstr,
		    min(len, lstrlenA(inet_pro->scheme_name)))) {
	    y->fcncde = inet_pro->scheme_number;
	    break;
	}
	inet_pro++;
    }
    HeapFree(GetProcessHeap(), 0, cmpstr);
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
    LPWSTR base, canonical;
    DWORD ret, len, len2;

    TRACE("(%s %p %p 0x%08lx) using W version\n",
	  debugstr_a(pszUrl), pszCanonicalized,
	  pcchCanonicalized, dwFlags);

    if(!pszUrl || !pszCanonicalized || !pcchCanonicalized)
	return E_INVALIDARG;

    base = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
			      (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    canonical = base + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszUrl, -1, base, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    ret = UrlCanonicalizeW(base, canonical, &len, dwFlags);
    if (ret != S_OK) {
	HeapFree(GetProcessHeap(), 0, base);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, canonical, len, 0, 0, 0, 0);
    if (len2 > *pcchCanonicalized) {
	*pcchCanonicalized = len;
	HeapFree(GetProcessHeap(), 0, base);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, canonical, len+1, pszCanonicalized,
			*pcchCanonicalized, 0, 0);
    *pcchCanonicalized = len2;
    HeapFree(GetProcessHeap(), 0, base);
    return S_OK;
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
    LPWSTR lpszUrlCpy, wk1, wk2, mp, root;
    INT nLen, nByteLen, state;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_w(pszUrl), pszCanonicalized,
	  pcchCanonicalized, dwFlags);

    if(!pszUrl || !pszCanonicalized || !pcchCanonicalized)
	return E_INVALIDARG;

    nByteLen = (lstrlenW(pszUrl) + 1) * sizeof(WCHAR); /* length in bytes */
    lpszUrlCpy = HeapAlloc(GetProcessHeap(), 0, nByteLen);

    if (dwFlags & URL_DONT_SIMPLIFY)
        memcpy(lpszUrlCpy, pszUrl, nByteLen);
    else {

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

	wk1 = (LPWSTR)pszUrl;
	wk2 = lpszUrlCpy;
	state = 0;
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
		if (*wk1++ == L':') state = 2;
		break;
	    case 2:
		if (*wk1 != L'/') {state = 3; break;}
		*wk2++ = *wk1++;
		if (*wk1 != L'/') {state = 6; break;}
		*wk2++ = *wk1++;
		state = 4;
		break;
	    case 3:
		strcpyW(wk2, wk1);
		wk1 += strlenW(wk1);
		wk2 += strlenW(wk2);
		break;
	    case 4:
		if (!isalnumW(*wk1) && (*wk1 != L'-') && (*wk1 != L'.')) {state = 3; break;}
		while(isalnumW(*wk1) || (*wk1 == L'-') || (*wk1 == L'.')) *wk2++ = *wk1++;
		state = 5;
		break;
	    case 5:
		if (*wk1 != L'/') {state = 3; break;}
		*wk2++ = *wk1++;
		state = 6;
		break;
	    case 6:
		/* Now at root location, cannot back up any more. */
		/* "root" will point at the '/' */
		root = wk2-1;
		while (*wk1) {
		    TRACE("wk1=%c\n", (CHAR)*wk1);
		    mp = strchrW(wk1, L'/');
		    if (!mp) {
			strcpyW(wk2, wk1);
			wk1 += strlenW(wk1);
			wk2 += strlenW(wk2);
			continue;
		    }
		    nLen = mp - wk1 + 1;
		    strncpyW(wk2, wk1, nLen);
		    wk2 += nLen;
		    wk1 += nLen;
		    if (*wk1 == L'.') {
			TRACE("found '/.'\n");
			if (*(wk1+1) == L'/') {
			    /* case of /./ -> skip the ./ */
			    wk1 += 2;
			}
			else if (*(wk1+1) == L'.') {
			    /* found /..  look for next / */
			    TRACE("found '/..'\n");
			    if (*(wk1+2) == L'/' || *(wk1+2) == L'?' || *(wk1+2) == L'#' || *(wk1+2) == 0) {
				/* case /../ -> need to backup wk2 */
				TRACE("found '/../'\n");
				*(wk2-1) = L'\0';  /* set end of string */
				mp = strrchrW(root, L'/');
				if (mp && (mp >= root)) {
				    /* found valid backup point */
				    wk2 = mp + 1;
				    if(*(wk1+2) != L'/')
				        wk1 += 2;
				    else
				        wk1 += 3;
				}
				else {
				    /* did not find point, restore '/' */
				    *(wk2-1) = L'/';
				}
			    }
			}
		    }
		}
		*wk2 = L'\0';
		break;
	    default:
		FIXME("how did we get here - state=%d\n", state);
		return E_INVALIDARG;
	    }
	}
	*wk2 = L'\0';
	TRACE("Simplified, orig <%s>, simple <%s>\n",
	      debugstr_w(pszUrl), debugstr_w(lpszUrlCpy));
    }
    nLen = lstrlenW(lpszUrlCpy);
    while ((nLen > 0) && ((lpszUrlCpy[nLen-1] == '\r')||(lpszUrlCpy[nLen-1] == '\n')))
        lpszUrlCpy[--nLen]=0;

    if(dwFlags & URL_UNESCAPE)
        UrlUnescapeW(lpszUrlCpy, NULL, NULL, URL_UNESCAPE_INPLACE);

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

    TRACE("(base %s, Relative %s, Combine size %ld, flags %08lx) using W version\n",
	  debugstr_a(pszBase),debugstr_a(pszRelative),
	  pcchCombined?*pcchCombined:0,dwFlags);

    if(!pszBase || !pszRelative || !pcchCombined)
	return E_INVALIDARG;

    base = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
			      (3*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    relative = base + INTERNET_MAX_URL_LENGTH;
    combined = relative + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszBase, -1, base, INTERNET_MAX_URL_LENGTH);
    MultiByteToWideChar(0, 0, pszRelative, -1, relative, INTERNET_MAX_URL_LENGTH);
    len = *pcchCombined;

    ret = UrlCombineW(base, relative, pszCombined?combined:NULL, &len, dwFlags);
    if (ret != S_OK) {
	*pcchCombined = len;
	HeapFree(GetProcessHeap(), 0, base);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, combined, len, 0, 0, 0, 0);
    if (len2 > *pcchCombined) {
	*pcchCombined = len2;
	HeapFree(GetProcessHeap(), 0, base);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, combined, len+1, pszCombined, (*pcchCombined)+1,
			0, 0);
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
    UNKNOWN_SHLWAPI_2 base, relative;
    DWORD myflags, sizeloc = 0;
    DWORD len, res1, res2, process_case = 0;
    LPWSTR work, preliminary, mbase, mrelative;
    static const WCHAR myfilestr[] = {'f','i','l','e',':','/','/','/','\0'};
    static const WCHAR single_slash[] = {'/','\0'};
    HRESULT ret;

    TRACE("(base %s, Relative %s, Combine size %ld, flags %08lx)\n",
	  debugstr_w(pszBase),debugstr_w(pszRelative),
	  pcchCombined?*pcchCombined:0,dwFlags);

    if(!pszBase || !pszRelative || !pcchCombined)
	return E_INVALIDARG;

    base.size = 24;
    relative.size = 24;

    /* Get space for duplicates of the input and the output */
    preliminary = HeapAlloc(GetProcessHeap(), 0, (3*INTERNET_MAX_URL_LENGTH) *
			    sizeof(WCHAR));
    mbase = preliminary + INTERNET_MAX_URL_LENGTH;
    mrelative = mbase + INTERNET_MAX_URL_LENGTH;
    *preliminary = L'\0';

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

	/* get size of location field (if it exists) */
	work = (LPWSTR)base.ap2;
	sizeloc = 0;
	if (*work++ == L'/') {
	    if (*work++ == L'/') {
		/* At this point have start of location and
		 * it ends at next '/' or end of string.
		 */
		while(*work && (*work != L'/')) work++;
		sizeloc = (DWORD)(work - base.ap2);
	    }
	}

	/* Change .sizep2 to not have the last leaf in it,
	 * Note: we need to start after the location (if it exists)
	 */
	work = strrchrW((base.ap2+sizeloc), L'/');
	if (work) {
	    len = (DWORD)(work - base.ap2 + 1);
	    base.sizep2 = len;
	}
	/*
	 * At this point:
	 *    .ap2      points to location (starting with '//')
	 *    .sizep2   length of location (above) and rest less the last
	 *              leaf (if any)
	 *    sizeloc   length of location (above) up to but not including
	 *              the last '/'
	 */

	res2 = ParseURLW(mrelative, &relative);
	if (res2) {
	    /* no scheme in pszRelative */
	    TRACE("no scheme detected in Relative\n");
	    relative.ap2 = mrelative;  /* case 3,4,5 depends on this */
	    relative.sizep2 = strlenW(mrelative);
	    if (*pszRelative  == L':') {
		/* case that is either left alone or uses pszBase */
		if (dwFlags & URL_PLUGGABLE_PROTOCOL) {
		    process_case = 5;
		    break;
		}
		process_case = 1;
		break;
	    }
	    if (isalnum(*mrelative) && (*(mrelative + 1) == L':')) {
		/* case that becomes "file:///" */
		strcpyW(preliminary, myfilestr);
		process_case = 1;
		break;
	    }
	    if ((*mrelative == L'/') && (*(mrelative+1) == L'/')) {
		/* pszRelative has location and rest */
		process_case = 3;
		break;
	    }
	    if (*mrelative == L'/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
	    process_case = (*base.ap2 == L'/') ? 5 : 3;
	    break;
	}

	/* handle cases where pszRelative has scheme */
	if ((base.sizep1 == relative.sizep1) &&
	    (strncmpW(base.ap1, relative.ap1, base.sizep1) == 0)) {

	    /* since the schemes are the same */
	    if ((*relative.ap2 == L'/') && (*(relative.ap2+1) == L'/')) {
		/* case where pszRelative replaces location and following */
		process_case = 3;
		break;
	    }
	    if (*relative.ap2 == L'/') {
		/* case where pszRelative is root to location */
		process_case = 4;
		break;
	    }
	    /* case where scheme is followed by document path */
	    process_case = 5;
	    break;
	}
	if ((*relative.ap2 == L'/') && (*(relative.ap2+1) == L'/')) {
	    /* case where pszRelative replaces scheme, location,
	     * and following and handles PLUGGABLE
	     */
	    process_case = 2;
	    break;
	}
	process_case = 1;
	break;
    } while(FALSE); /* a litte trick to allow easy exit from nested if's */


    ret = S_OK;
    switch (process_case) {

    case 1:  /*
	      * Return pszRelative appended to what ever is in pszCombined,
	      * (which may the string "file:///"
	      */
	strcatW(preliminary, mrelative);
	break;

    case 2:  /*
	      * Same as case 1, but if URL_PLUGGABLE_PROTOCOL was specified
	      * and pszRelative starts with "//", then append a "/"
	      */
	strcpyW(preliminary, mrelative);
	if (!(dwFlags & URL_PLUGGABLE_PROTOCOL) &&
	    URL_JustLocation(relative.ap2))
	    strcatW(preliminary, single_slash);
	break;

    case 3:  /*
	      * Return the pszBase scheme with pszRelative. Basically
	      * keeps the scheme and replaces the domain and following.
	      */
	strncpyW(preliminary, base.ap1, base.sizep1 + 1);
	work = preliminary + base.sizep1 + 1;
	strcpyW(work, relative.ap2);
	if (!(dwFlags & URL_PLUGGABLE_PROTOCOL) &&
	    URL_JustLocation(relative.ap2))
	    strcatW(work, single_slash);
	break;

    case 4:  /*
	      * Return the pszBase scheme and location but everything
	      * after the location is pszRelative. (Replace document
	      * from root on.)
	      */
	strncpyW(preliminary, base.ap1, base.sizep1+1+sizeloc);
	work = preliminary + base.sizep1 + 1 + sizeloc;
	if (dwFlags & URL_PLUGGABLE_PROTOCOL)
	    *(work++) = L'/';
	strcpyW(work, relative.ap2);
	break;

    case 5:  /*
	      * Return the pszBase without its document (if any) and
	      * append pszRelative after its scheme.
	      */
	strncpyW(preliminary, base.ap1, base.sizep1+1+base.sizep2);
	work = preliminary + base.sizep1+1+base.sizep2 - 1;
	if (*work++ != L'/')
	    *(work++) = L'/';
	strcpyW(work, relative.ap2);
	break;

    default:
	FIXME("How did we get here????? process_case=%ld\n", process_case);
	ret = E_INVALIDARG;
    }

    if (ret == S_OK) {
	/* Reuse mrelative as temp storage as its already allocated and not needed anymore */
	ret = UrlCanonicalizeW(preliminary, mrelative, pcchCombined, dwFlags);
	if(SUCCEEDED(ret) && pszCombined) {
	    lstrcpyW(pszCombined, mrelative);
	}
	TRACE("return-%ld len=%ld, %s\n",
	      process_case, *pcchCombined, debugstr_w(pszCombined));
    }
    HeapFree(GetProcessHeap(), 0, preliminary);
    return ret;
}

/*************************************************************************
 *      UrlEscapeA	[SHLWAPI.@]
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

 * Converts unsafe characters into their escape sequences.
 *
 * NOTES
 * - By default this function stops converting at the first '?' or
 *  '#' character (MSDN does not document this).
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
HRESULT WINAPI UrlEscapeA(
	LPCSTR pszUrl,
	LPSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
    LPCSTR src;
    DWORD needed = 0, ret;
    BOOL stop_escaping = FALSE;
    char next[3], *dst = pszEscaped;
    INT len;

    TRACE("(%s %p %lx 0x%08lx)\n", debugstr_a(pszUrl), pszEscaped,
	  pcchEscaped?*pcchEscaped:0, dwFlags);

    if(!pszUrl || !pszEscaped || !pcchEscaped)
	return E_INVALIDARG;

    if(dwFlags & ~(URL_ESCAPE_SPACES_ONLY |
		   URL_ESCAPE_SEGMENT_ONLY |
		   URL_DONT_ESCAPE_EXTRA_INFO |
		   URL_ESCAPE_PERCENT))
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    /* fix up flags */
    if (dwFlags & URL_ESCAPE_SPACES_ONLY)
	/* if SPACES_ONLY specified, reset the other controls */
	dwFlags &= ~(URL_DONT_ESCAPE_EXTRA_INFO |
		     URL_ESCAPE_PERCENT |
		     URL_ESCAPE_SEGMENT_ONLY);

    else
	/* if SPACES_ONLY *not* specified then assume DONT_ESCAPE_EXTRA_INFO */
	dwFlags |= URL_DONT_ESCAPE_EXTRA_INFO;

    for(src = pszUrl; *src; src++) {
        if(!(dwFlags & URL_ESCAPE_SEGMENT_ONLY) &&
	   (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO) &&
	   (*src == '#' || *src == '?'))
	    stop_escaping = TRUE;

	if(URL_NeedEscapeA(*src, dwFlags) && stop_escaping == FALSE) {
	    /* TRACE("escaping %c\n", *src); */
	    next[0] = '%';
	    next[1] = hexDigits[(*src >> 4) & 0xf];
	    next[2] = hexDigits[*src & 0xf];
	    len = 3;
	} else {
	    /* TRACE("passing %c\n", *src); */
	    next[0] = *src;
	    len = 1;
	}

	if(needed + len <= *pcchEscaped) {
	    memcpy(dst, next, len);
	    dst += len;
	}
	needed += len;
    }

    if(needed < *pcchEscaped) {
        *dst = '\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    *pcchEscaped = needed;
    return ret;
}

/*************************************************************************
 *      UrlEscapeW	[SHLWAPI.@]
 *
 * See UrlEscapeA.
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
    WCHAR next[5], *dst = pszEscaped;
    INT len;

    TRACE("(%s %p %p 0x%08lx)\n", debugstr_w(pszUrl), pszEscaped,
	  pcchEscaped, dwFlags);

    if(!pszUrl || !pszEscaped || !pcchEscaped)
	return E_INVALIDARG;

    if(dwFlags & ~(URL_ESCAPE_SPACES_ONLY |
		   URL_ESCAPE_SEGMENT_ONLY |
		   URL_DONT_ESCAPE_EXTRA_INFO |
		   URL_ESCAPE_PERCENT))
        FIXME("Unimplemented flags: %08lx\n", dwFlags);

    /* fix up flags */
    if (dwFlags & URL_ESCAPE_SPACES_ONLY)
	/* if SPACES_ONLY specified, reset the other controls */
	dwFlags &= ~(URL_DONT_ESCAPE_EXTRA_INFO |
		     URL_ESCAPE_PERCENT |
		     URL_ESCAPE_SEGMENT_ONLY);

    else
	/* if SPACES_ONLY *not* specified the assume DONT_ESCAPE_EXTRA_INFO */
	dwFlags |= URL_DONT_ESCAPE_EXTRA_INFO;

    for(src = pszUrl; *src; src++) {
	/*
	 * if(!(dwFlags & URL_ESCAPE_SPACES_ONLY) &&
	 *   (*src == L'#' || *src == L'?'))
	 *    stop_escaping = TRUE;
	 */
        if(!(dwFlags & URL_ESCAPE_SEGMENT_ONLY) &&
	   (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO) &&
	   (*src == L'#' || *src == L'?'))
	    stop_escaping = TRUE;

	if(URL_NeedEscapeW(*src, dwFlags) && stop_escaping == FALSE) {
	    /* TRACE("escaping %c\n", *src); */
	    next[0] = L'%';
	    /*
	     * I would have assumed that the W form would escape
	     * the character with 4 hex digits (or even 8),
	     * however, experiments show that native shlwapi escapes
	     * with only 2 hex digits.
	     *   next[1] = hexDigits[(*src >> 12) & 0xf];
	     *   next[2] = hexDigits[(*src >> 8) & 0xf];
	     *   next[3] = hexDigits[(*src >> 4) & 0xf];
	     *   next[4] = hexDigits[*src & 0xf];
	     *   len = 5;
	     */
	    next[1] = hexDigits[(*src >> 4) & 0xf];
	    next[2] = hexDigits[*src & 0xf];
	    len = 3;
	} else {
	    /* TRACE("passing %c\n", *src); */
	    next[0] = *src;
	    len = 1;
	}

	if(needed + len <= *pcchEscaped) {
	    memcpy(dst, next, len*sizeof(WCHAR));
	    dst += len;
	}
	needed += len;
    }

    if(needed < *pcchEscaped) {
        *dst = L'\0';
	ret = S_OK;
    } else {
        needed++; /* add one for the '\0' */
	ret = E_POINTER;
    }
    *pcchEscaped = needed;
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

    TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_a(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if(!pszUrl || !pszUnescaped || !pcchUnescaped)
	return E_INVALIDARG;

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = pszUrl;
    else
        dst = pszUnescaped;

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

    TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_w(pszUrl), pszUnescaped,
	  pcchUnescaped, dwFlags);

    if(!pszUrl || !pszUnescaped || !pcchUnescaped)
	return E_INVALIDARG;

    if(dwFlags & URL_UNESCAPE_INPLACE)
        dst = pszUrl;
    else
        dst = pszUnescaped;

    for(src = pszUrl, needed = 0; *src; src++, needed++) {
        if(dwFlags & URL_DONT_UNESCAPE_EXTRA_INFO &&
	   (*src == L'#' || *src == L'?')) {
	    stop_unescaping = TRUE;
	    next = *src;
	} else if(*src == L'%' && isxdigitW(*(src + 1)) && isxdigitW(*(src + 2))
		  && stop_unescaping == FALSE) {
	    INT ih;
	    WCHAR buf[3];
	    memcpy(buf, src + 1, 2*sizeof(WCHAR));
	    buf[2] = L'\0';
	    ih = StrToIntW(buf);
	    next = (WCHAR) ih;
	    src += 2; /* Advance to end of escape */
	} else
	    next = *src;

	if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped)
	    *dst++ = next;
    }

    if(dwFlags & URL_UNESCAPE_INPLACE || needed < *pcchUnescaped) {
        *dst = L'\0';
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
    UNKNOWN_SHLWAPI_1 base;
    DWORD res1;

    base.size = 24;
    res1 = ParseURLA(pszUrl, &base);
    if (res1) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (strncmp(base.ap1, "file", min(4,base.sizep1)) == 0) return NULL;

    /* Look for '#' and return its addr */
    return strchr(base.ap2, '#');
}

/*************************************************************************
 *      UrlGetLocationW 	[SHLWAPI.@]
 *
 * See UrlGetLocationA.
 */
LPCWSTR WINAPI UrlGetLocationW(
	LPCWSTR pszUrl)
{
    UNKNOWN_SHLWAPI_2 base;
    DWORD res1;

    base.size = 24;
    res1 = ParseURLW(pszUrl, &base);
    if (res1) return NULL;  /* invalid scheme */

    /* if scheme is file: then never return pointer */
    if (strncmpW(base.ap1, fileW, min(4,base.sizep1)) == 0) return NULL;

    /* Look for '#' and return its addr */
    return strchrW(base.ap2, L'#');
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
HRESULT WINAPI HashData(LPBYTE lpSrc, DWORD nSrcLen,
                     LPBYTE lpDest, DWORD nDestLen)
{
  INT srcCount = nSrcLen - 1, destCount = nDestLen - 1;

  if (IsBadReadPtr(lpSrc, nSrcLen) ||
      IsBadWritePtr(lpDest, nDestLen))
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

  HashData((PBYTE)pszUrl, (int)strlen(pszUrl), lpDest, nDestLen);
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

  TRACE("(%s,%p,%ld)\n",debugstr_w(pszUrl), lpDest, nDestLen);

  if (IsBadStringPtrW(pszUrl, -1) || IsBadWritePtr(lpDest, nDestLen))
    return E_INVALIDARG;

  /* Win32 hashes the data as an ASCII string, presumably so that both A+W
   * return the same digests for the same URL.
   */
  WideCharToMultiByte(0, 0, pszUrl, -1, szUrl, MAX_PATH, 0, 0);
  HashData((PBYTE)szUrl, (int)strlen(szUrl), lpDest, nDestLen);
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
    DWORD ret, len, len2;

    TRACE("(in %s, out size %ld, flags %08lx) using W version\n",
	  debugstr_a(pszIn), *pcchOut, dwFlags);

    in = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
			      (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    out = in + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszIn, -1, in, INTERNET_MAX_URL_LENGTH);
    len = INTERNET_MAX_URL_LENGTH;

    ret = UrlApplySchemeW(in, out, &len, dwFlags);
    if ((ret != S_OK) && (ret != S_FALSE)) {
	HeapFree(GetProcessHeap(), 0, in);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, out, len+1, 0, 0, 0, 0);
    if (len2 > *pcchOut) {
	*pcchOut = len2;
	HeapFree(GetProcessHeap(), 0, in);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, out, len+1, pszOut, *pcchOut, 0, 0);
    *pcchOut = len2;
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

    MultiByteToWideChar(0, 0,
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
    return -1;
}

static HRESULT URL_ApplyDefault(LPCWSTR pszIn, LPWSTR pszOut, LPDWORD pcchOut)
{
    HKEY newkey;
    DWORD data_len, dwType;
    WCHAR reg_path[MAX_PATH];
    WCHAR value[MAX_PATH], data[MAX_PATH];

    /* get and prepend default */
    MultiByteToWideChar(0, 0,
	 "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix",
			-1, reg_path, MAX_PATH);
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, reg_path, 0, 1, &newkey);
    data_len = MAX_PATH;
    value[0] = L'@';
    value[1] = L'\0';
    RegQueryValueExW(newkey, value, 0, &dwType, (LPBYTE)data, &data_len);
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
    UNKNOWN_SHLWAPI_2 in_scheme;
    DWORD res1;
    HRESULT ret;

    TRACE("(in %s, out size %ld, flags %08lx)\n",
	  debugstr_w(pszIn), *pcchOut, dwFlags);

    if (dwFlags & URL_APPLY_GUESSFILE) {
	FIXME("(%s %p %p(%ld) 0x%08lx): stub URL_APPLY_GUESSFILE not implemented\n",
	      debugstr_w(pszIn), pszOut, pcchOut, *pcchOut, dwFlags);
	strcpyW(pszOut, pszIn);
	*pcchOut = strlenW(pszOut);
	return S_FALSE;
    }

    in_scheme.size = 24;
    /* See if the base has a scheme */
    res1 = ParseURLW(pszIn, &in_scheme);
    if (res1) {
	/* no scheme in input, need to see if we need to guess */
	if (dwFlags & URL_APPLY_GUESSSCHEME) {
	    if ((ret = URL_GuessScheme(pszIn, pszOut, pcchOut)) != -1)
		return ret;
	}
    }
    else {
	/* we have a scheme, see if valid (known scheme) */
	if (in_scheme.fcncde) {
	    /* have valid scheme, so just copy and exit */
	    if (strlenW(pszIn) + 1 > *pcchOut) {
		*pcchOut = strlenW(pszIn) + 1;
		return E_POINTER;
	    }
	    strcpyW(pszOut, pszIn);
	    *pcchOut = strlenW(pszOut);
	    TRACE("valid scheme, returing copy\n");
	    return S_OK;
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

    /* just copy and give proper return code */
    if (strlenW(pszIn) + 1 > *pcchOut) {
	*pcchOut = strlenW(pszIn) + 1;
	return E_POINTER;
    }
    strcpyW(pszOut, pszIn);
    *pcchOut = strlenW(pszOut);
    TRACE("returning copy, left alone\n");
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
    UNKNOWN_SHLWAPI_1 base;
    DWORD res1;

    switch (Urlis) {

    case URLIS_OPAQUE:
	base.size = 24;
	res1 = ParseURLA(pszUrl, &base);
	if (res1) return FALSE;  /* invalid scheme */
	if ((*base.ap2 == '/') && (*(base.ap2+1) == '/'))
	    /* has scheme followed by 2 '/' */
	    return FALSE;
	return TRUE;

    case URLIS_URL:
    case URLIS_NOHISTORY:
    case URLIS_FILEURL:
    case URLIS_APPLIABLE:
    case URLIS_DIRECTORY:
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
    UNKNOWN_SHLWAPI_2 base;
    DWORD res1;

    switch (Urlis) {

    case URLIS_OPAQUE:
	base.size = 24;
	res1 = ParseURLW(pszUrl, &base);
	if (res1) return FALSE;  /* invalid scheme */
	if ((*base.ap2 == L'/') && (*(base.ap2+1) == L'/'))
	    /* has scheme followed by 2 '/' */
	    return FALSE;
	return TRUE;

    case URLIS_URL:
    case URLIS_NOHISTORY:
    case URLIS_FILEURL:
    case URLIS_APPLIABLE:
    case URLIS_DIRECTORY:
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
		 (*start == L'+') ||
		 (*start == L'-') ||
		 (*start == L'.')) {
		start++;
		(*size)++;
	    }
	    else
		cont = FALSE;
	}
        break;

    case USERPASS:
	while (cont) {
	    if ( isalphaW(*start) ||
		 isdigitW(*start) ||
		 /* user/password only characters */
		 (*start == L';') ||
		 (*start == L'?') ||
		 (*start == L'&') ||
		 (*start == L'=') ||
		 /* *extra* characters */
		 (*start == L'!') ||
		 (*start == L'*') ||
		 (*start == L'\'') ||
		 (*start == L'(') ||
		 (*start == L')') ||
		 (*start == L',') ||
		 /* *safe* characters */
		 (*start == L'$') ||
		 (*start == L'_') ||
		 (*start == L'+') ||
		 (*start == L'-') ||
		 (*start == L'.')) {
		start++;
		(*size)++;
	    } else if (*start == L'%') {
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
		(*start == L'-') ||
		(*start == L'.') ) {
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
    /* TRACE("scanned %ld characters next char %p<%c>\n",
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
    if (!*work || (*work != L':')) goto ErrorExit;
    work++;
    if ((*work != L'/') || (*(work+1) != L'/')) goto ErrorExit;
    pl->pUserName = work + 2;
    work = URL_ScanID(pl->pUserName, &pl->szUserName, USERPASS);
    if (*work == L':' ) {
	/* parse password */
	work++;
	pl->pPassword = work;
	work = URL_ScanID(pl->pPassword, &pl->szPassword, USERPASS);
	if (*work != L'@') {
	    /* what we just parsed must be the hostname and port
	     * so reset pointers and clear then let it parse */
	    pl->szUserName = pl->szPassword = 0;
	    work = pl->pUserName - 1;
	    pl->pUserName = pl->pPassword = 0;
	}
    } else if (*work == L'@') {
	/* no password */
	pl->szPassword = 0;
	pl->pPassword = 0;
    } else if (!*work || (*work == L'/') || (*work == L'.')) {
	/* what was parsed was hostname, so reset pointers and let it parse */
	pl->szUserName = pl->szPassword = 0;
	work = pl->pUserName - 1;
	pl->pUserName = pl->pPassword = 0;
    } else goto ErrorExit;

    /* now start parsing hostname or hostnumber */
    work++;
    pl->pHostName = work;
    work = URL_ScanID(pl->pHostName, &pl->szHostName, HOST);
    if (*work == L':') {
	/* parse port */
	work++;
	pl->pPort = work;
	work = URL_ScanID(pl->pPort, &pl->szPort, PORT);
    }
    if (*work == L'/') {
	/* see if query string */
	pl->pQuery = strchrW(work, L'?');
	if (pl->pQuery) pl->szQuery = strlenW(pl->pQuery);
    }
    TRACE("parse successful: scheme=%p(%ld), user=%p(%ld), pass=%p(%ld), host=%p(%ld), port=%p(%ld), query=%p(%ld)\n",
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
 *  pcchOut [I/O] Length of pszOut/destination for length of pszOut
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

    in = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
			      (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
    out = in + INTERNET_MAX_URL_LENGTH;

    MultiByteToWideChar(0, 0, pszIn, -1, in, INTERNET_MAX_URL_LENGTH);

    len = INTERNET_MAX_URL_LENGTH;
    ret = UrlGetPartW(in, out, &len, dwPart, dwFlags);

    if (ret != S_OK) {
	HeapFree(GetProcessHeap(), 0, in);
	return ret;
    }

    len2 = WideCharToMultiByte(0, 0, out, len, 0, 0, 0, 0);
    if (len2 > *pcchOut) {
	*pcchOut = len2;
	HeapFree(GetProcessHeap(), 0, in);
	return E_POINTER;
    }
    WideCharToMultiByte(0, 0, out, len+1, pszOut, *pcchOut, 0, 0);
    *pcchOut = len2;
    HeapFree(GetProcessHeap(), 0, in);
    return S_OK;
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
    DWORD size, schsize;
    LPCWSTR addr, schaddr;
    LPWSTR work;

    TRACE("(%s %p %p(%ld) %08lx %08lx)\n",
	  debugstr_w(pszIn), pszOut, pcchOut, *pcchOut, dwPart, dwFlags);

    ret = URL_ParseUrl(pszIn, &pl);
    if (!ret) {
	schaddr = pl.pScheme;
	schsize = pl.szScheme;

	switch (dwPart) {
	case URL_PART_SCHEME:
	    if (!pl.szScheme) return E_INVALIDARG;
	    addr = pl.pScheme;
	    size = pl.szScheme;
	    break;

	case URL_PART_HOSTNAME:
	    if (!pl.szHostName) return E_INVALIDARG;
	    addr = pl.pHostName;
	    size = pl.szHostName;
	    break;

	case URL_PART_USERNAME:
	    if (!pl.szUserName) return E_INVALIDARG;
	    addr = pl.pUserName;
	    size = pl.szUserName;
	    break;

	case URL_PART_PASSWORD:
	    if (!pl.szPassword) return E_INVALIDARG;
	    addr = pl.pPassword;
	    size = pl.szPassword;
	    break;

	case URL_PART_PORT:
	    if (!pl.szPort) return E_INVALIDARG;
	    addr = pl.pPort;
	    size = pl.szPort;
	    break;

	case URL_PART_QUERY:
	    if (!pl.szQuery) return E_INVALIDARG;
	    addr = pl.pQuery;
	    size = pl.szQuery;
	    break;

	default:
	    return E_INVALIDARG;
	}

	if (dwFlags == URL_PARTFLAG_KEEPSCHEME) {
	    if (*pcchOut < size + schsize + 2) {
		*pcchOut = size + schsize + 2;
		return E_POINTER;
	    }
	    strncpyW(pszOut, schaddr, schsize);
	    work = pszOut + schsize;
	    *work = L':';
	    strncpyW(work+1, addr, size);
	    *pcchOut = size + schsize + 1;
	    work += (size + 1);
	    *work = L'\0';
	}
	else {
	    if (*pcchOut < size + 1) {*pcchOut = size+1; return E_POINTER;}
	    strncpyW(pszOut, addr, size);
	    *pcchOut = size;
	    work = pszOut + size;
	    *work = L'\0';
	}
	TRACE("len=%ld %s\n", *pcchOut, debugstr_w(pszOut));
    }
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
    UNKNOWN_SHLWAPI_1 base;
    DWORD res1;

    if (!lpstrPath || !*lpstrPath) return FALSE;

    /* get protocol        */
    base.size = sizeof(base);
    res1 = ParseURLA(lpstrPath, &base);
    return (base.fcncde > 0);
}

/*************************************************************************
 * PathIsURLW	[SHLWAPI.@]
 *
 * See PathIsURLA.
 */
BOOL WINAPI PathIsURLW(LPCWSTR lpstrPath)
{
    UNKNOWN_SHLWAPI_2 base;
    DWORD res1;

    if (!lpstrPath || !*lpstrPath) return FALSE;

    /* get protocol        */
    base.size = sizeof(base);
    res1 = ParseURLW(lpstrPath, &base);
    return (base.fcncde > 0);
}

/*************************************************************************
 *      UrlCreateFromPathA  	[SHLWAPI.@]
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
 *  Success: S_OK. pszUrl contains the converted path.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI UrlCreateFromPathA(LPCSTR pszPath, LPSTR pszUrl, LPDWORD pcchUrl, DWORD dwReserved)
{
	DWORD nCharBeforeColon = 0;
	DWORD nSlashes = 0;
	DWORD dwChRequired = 0;
	LPSTR pszNewUrl = NULL;
	LPCSTR pszConstPointer = NULL;
	LPSTR pszPointer = NULL;
	DWORD i;
	HRESULT ret;

	TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_a(pszPath), pszUrl, pcchUrl, dwReserved);

	/* Validate arguments */
	if (dwReserved != 0)
	{
		FIXME("dwReserved should be 0: 0x%08lx\n", dwReserved);
		return E_INVALIDARG;
	}
	if (!pszUrl || !pcchUrl || !pszUrl)
	{
		ERR("Invalid argument\n");
		return E_INVALIDARG;
	}

	for (pszConstPointer = pszPath; *pszConstPointer; pszConstPointer++)
	{
		if (isalpha(*pszConstPointer) || isdigit(*pszConstPointer) ||
			*pszConstPointer == '.' || *pszConstPointer == '-')
			nCharBeforeColon++;
		else break;
	}
	if (*pszConstPointer == ':') /* then already in URL format, so copy */
	{
		dwChRequired = lstrlenA(pszPath);
		if (dwChRequired > *pcchUrl)
		{
			*pcchUrl = dwChRequired;
			return E_POINTER;
		}
		else
		{
			*pcchUrl = dwChRequired;
			StrCpyA(pszUrl, pszPath);
			return S_FALSE;
		}
	}
	/* then must need converting to file: format */

	/* Strip off leading slashes */
	while (*pszPath == '\\' || *pszPath == '/')
	{
		pszPath++;
		nSlashes++;
	}

	dwChRequired = *pcchUrl; /* UrlEscape will fill this in with the correct amount */
	TRACE("pszUrl: %s\n", debugstr_a(pszPath));
	pszNewUrl = HeapAlloc(GetProcessHeap(), 0, dwChRequired + 1);
	ret = UrlEscapeA(pszPath, pszNewUrl, &dwChRequired, URL_ESCAPE_PERCENT);
	TRACE("ret: 0x%08lx, pszUrl: %s\n", ret, debugstr_a(pszNewUrl));
	TRACE("%ld\n", dwChRequired);
	if (ret != E_POINTER && FAILED(ret))
		return ret;
	dwChRequired += 5; /* "file:" */
	if ((lstrlenA(pszUrl) > 1) && isalpha(pszUrl[0]) && (pszUrl[1] == ':'))
	{
		dwChRequired += 3; /* "///" */
		nSlashes = 3;
	}
	else
		switch (nSlashes)
		{
		case 0: /* no slashes */
			break;
		case 2: /* two slashes */
		case 4:
		case 5:
		case 6:
			dwChRequired += 2;
			nSlashes = 2;
			break;
		default: /* three slashes */
			dwChRequired += 3;
			nSlashes = 3;
		}

	if (dwChRequired > *pcchUrl)
		return E_POINTER;
	*pcchUrl = dwChRequired; /* Return number of chars required (not including termination) */
	StrCpyA(pszUrl, "file:");
	pszPointer = pszUrl + lstrlenA(pszUrl);
	for (i=0; i < nSlashes; i++)
	{
		*pszPointer = '/';
		pszPointer++;
	}
	StrCpyA(pszPointer, pszNewUrl);
	TRACE("<- %s\n", debugstr_a(pszUrl));
	return S_OK;
}

/*************************************************************************
 *      UrlCreateFromPathW  	[SHLWAPI.@]
 *
 * See UrlCreateFromPathA.
 */
HRESULT WINAPI UrlCreateFromPathW(LPCWSTR pszPath, LPWSTR pszUrl, LPDWORD pcchUrl, DWORD dwReserved)
{
	DWORD nCharBeforeColon = 0;
	DWORD nSlashes = 0;
	DWORD dwChRequired = 0;
	LPWSTR pszNewUrl = NULL;
	LPCWSTR pszConstPointer = NULL;
	LPWSTR pszPointer = NULL;
	DWORD i;
	HRESULT ret;

	TRACE("(%s, %p, %p, 0x%08lx)\n", debugstr_w(pszPath), pszUrl, pcchUrl, dwReserved);

	/* Validate arguments */
	if (dwReserved != 0)
		return E_INVALIDARG;
	if (!pszUrl || !pcchUrl || !pszUrl)
		return E_INVALIDARG;

	for (pszConstPointer = pszPath; *pszConstPointer; pszConstPointer++)
	{
		if (isalphaW(*pszConstPointer) || isdigitW(*pszConstPointer) ||
			*pszConstPointer == '.' || *pszConstPointer == '-')
			nCharBeforeColon++;
		else break;
	}
	if (*pszConstPointer == ':') /* then already in URL format, so copy */
	{
		dwChRequired = lstrlenW(pszPath);
		*pcchUrl = dwChRequired;
		if (dwChRequired > *pcchUrl)
			return E_POINTER;
		else
		{
			StrCpyW(pszUrl, pszPath);
			return S_FALSE;
		}
	}
	/* then must need converting to file: format */

	/* Strip off leading slashes */
	while (*pszPath == '\\' || *pszPath == '/')
	{
		pszPath++;
		nSlashes++;
	}

	dwChRequired = *pcchUrl; /* UrlEscape will fill this in with the correct amount */
	ret = UrlEscapeW(pszPath, pszUrl, &dwChRequired, URL_ESCAPE_PERCENT);
	if (ret != E_POINTER && FAILED(ret))
		return ret;
	dwChRequired += 5; /* "file:" */
	if ((lstrlenW(pszUrl) > 1) && isalphaW(pszUrl[0]) && (pszUrl[1] == ':'))
	{
		dwChRequired += 3; /* "///" */
		nSlashes = 3;
	}
	else
		switch (nSlashes)
		{
		case 0: /* no slashes */
			break;
		case 2: /* two slashes */
		case 4:
		case 5:
		case 6:
			dwChRequired += 2;
			nSlashes = 2;
			break;
		default: /* three slashes */
			dwChRequired += 3;
			nSlashes = 3;
		}

	*pcchUrl = dwChRequired; /* Return number of chars required (not including termination) */
	if (dwChRequired > *pcchUrl)
		return E_POINTER;
	pszNewUrl = HeapAlloc(GetProcessHeap(), 0, (dwChRequired + 1) * sizeof(WCHAR));
	StrCpyW(pszNewUrl, fileW);
	pszPointer = pszNewUrl + 4;
	*pszPointer = ':';
	pszPointer++;
	for (i=0; i < nSlashes; i++)
	{
		*pszPointer = '/';
		pszPointer++;
	}
	StrCpyW(pszPointer, pszPath);
	StrCpyW(pszUrl, pszNewUrl);
	return S_OK;
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
  FIXME("SHAutoComplete stub\n");
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
 *  Success: S_OK. lpszDest constains the resource Url.
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
    WideCharToMultiByte(CP_ACP, 0, szDest, -1, lpszDest, dwDestLen, 0, 0);

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

  TRACE("(%s,%p,0x%08lx,%s,%p,%ld)\n", debugstr_w(lpszLibName), hMod, dwFlags,
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

      if (GetModuleFileNameW(hMod, szBuff, sizeof(szBuff)/sizeof(WCHAR)))
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
            lpszDest[szResLen + dwPathLen + dwResLen] = '/';
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
