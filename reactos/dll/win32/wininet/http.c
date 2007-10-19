/*
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
 *
 * Ulrich Czekalla
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

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winreg.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_GDI
#include "shlwapi.h"

#include "internet.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "inet_ntop.c"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

static const WCHAR g_szHttp1_0[] = {' ','H','T','T','P','/','1','.','0',0 };
static const WCHAR g_szHttp1_1[] = {' ','H','T','T','P','/','1','.','1',0 };
static const WCHAR g_szReferer[] = {'R','e','f','e','r','e','r',0};
static const WCHAR g_szAccept[] = {'A','c','c','e','p','t',0};
static const WCHAR g_szUserAgent[] = {'U','s','e','r','-','A','g','e','n','t',0};
static const WCHAR szHost[] = { 'H','o','s','t',0 };
static const WCHAR szProxy_Authorization[] = { 'P','r','o','x','y','-','A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szStatus[] = { 'S','t','a','t','u','s',0 };

#define MAXHOSTNAME 100
#define MAX_FIELD_VALUE_LEN 256
#define MAX_FIELD_LEN 256

#define HTTP_REFERER    g_szReferer
#define HTTP_ACCEPT     g_szAccept
#define HTTP_USERAGENT  g_szUserAgent

#define HTTP_ADDHDR_FLAG_ADD				0x20000000
#define HTTP_ADDHDR_FLAG_ADD_IF_NEW			0x10000000
#define HTTP_ADDHDR_FLAG_COALESCE			0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA		0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON	0x01000000
#define HTTP_ADDHDR_FLAG_REPLACE			0x80000000
#define HTTP_ADDHDR_FLAG_REQ				0x02000000


static void HTTP_CloseHTTPRequestHandle(LPWININETHANDLEHEADER hdr);
static void HTTP_CloseHTTPSessionHandle(LPWININETHANDLEHEADER hdr);
static BOOL HTTP_OpenConnection(LPWININETHTTPREQW lpwhr);
static BOOL HTTP_GetResponseHeaders(LPWININETHTTPREQW lpwhr);
static BOOL HTTP_ProcessHeader(LPWININETHTTPREQW lpwhr, LPCWSTR field, LPCWSTR value, DWORD dwModifier);
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer);
static BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQW lpwhr, LPHTTPHEADERW lpHdr);
static INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQW lpwhr, LPCWSTR lpszField, INT index, BOOL Request);
static BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQW lpwhr, DWORD index);
static LPWSTR HTTP_build_req( LPCWSTR *list, int len );
static BOOL HTTP_InsertProxyAuthorization( LPWININETHTTPREQW lpwhr,
                       LPCWSTR username, LPCWSTR password );
static BOOL WINAPI HTTP_HttpQueryInfoW( LPWININETHTTPREQW lpwhr, DWORD
        dwInfoLevel, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD
        lpdwIndex);
static BOOL HTTP_HandleRedirect(LPWININETHTTPREQW lpwhr, LPCWSTR lpszUrl,
        LPCWSTR lpszHeaders, DWORD dwHeaderLength, LPVOID lpOptional, DWORD
        dwOptionalLength, DWORD dwContentLength);


LPHTTPHEADERW HTTP_GetHeader(LPWININETHTTPREQW req, LPCWSTR head)
{
    int HeaderIndex = 0;
    HeaderIndex = HTTP_GetCustomHeaderIndex(req, head, 0, TRUE);
    if (HeaderIndex == -1)
        return NULL;
    else
        return &req->pCustHeaders[HeaderIndex];
}

/***********************************************************************
 *           HTTP_Tokenize (internal)
 *
 *  Tokenize a string, allocating memory for the tokens.
 */
static LPWSTR * HTTP_Tokenize(LPCWSTR string, LPCWSTR token_string)
{
    LPWSTR * token_array;
    int tokens = 0;
    int i;
    LPCWSTR next_token;

    /* empty string has no tokens */
    if (*string)
        tokens++;
    /* count tokens */
    for (i = 0; string[i]; i++)
        if (!strncmpW(string+i, token_string, strlenW(token_string)))
        {
            DWORD j;
            tokens++;
            /* we want to skip over separators, but not the null terminator */
            for (j = 0; j < strlenW(token_string) - 1; j++)
                if (!string[i+j])
                    break;
            i += j;
        }

    /* add 1 for terminating NULL */
    token_array = HeapAlloc(GetProcessHeap(), 0, (tokens+1) * sizeof(*token_array));
    token_array[tokens] = NULL;
    if (!tokens)
        return token_array;
    for (i = 0; i < tokens; i++)
    {
        int len;
        next_token = strstrW(string, token_string);
        if (!next_token) next_token = string+strlenW(string);
        len = next_token - string;
        token_array[i] = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
        memcpy(token_array[i], string, len*sizeof(WCHAR));
        token_array[i][len] = '\0';
        string = next_token+strlenW(token_string);
    }
    return token_array;
}

/***********************************************************************
 *           HTTP_FreeTokens (internal)
 *
 *  Frees memory returned from HTTP_Tokenize.
 */
static void HTTP_FreeTokens(LPWSTR * token_array)
{
    int i;
    for (i = 0; token_array[i]; i++)
        HeapFree(GetProcessHeap(), 0, token_array[i]);
    HeapFree(GetProcessHeap(), 0, token_array);
}

/* **********************************************************************
 *
 * Helper functions for the HttpSendRequest(Ex) functions
 *
 */
static void HTTP_FixVerb( LPWININETHTTPREQW lpwhr )
{
    /* if the verb is NULL default to GET */
    if (NULL == lpwhr->lpszVerb)
    {
	    static const WCHAR szGET[] = { 'G','E','T', 0 };
	    lpwhr->lpszVerb = WININET_strdupW(szGET);
    }
}

static void HTTP_FixURL( LPWININETHTTPREQW lpwhr)
{
    static const WCHAR szSlash[] = { '/',0 };
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/', 0 };

    /* If we don't have a path we set it to root */
    if (NULL == lpwhr->lpszPath)
        lpwhr->lpszPath = WININET_strdupW(szSlash);
    else /* remove \r and \n*/
    {
        int nLen = strlenW(lpwhr->lpszPath);
        while ((nLen >0 ) && ((lpwhr->lpszPath[nLen-1] == '\r')||(lpwhr->lpszPath[nLen-1] == '\n')))
        {
            nLen--;
            lpwhr->lpszPath[nLen]='\0';
        }
        /* Replace '\' with '/' */
        while (nLen>0) {
            nLen--;
            if (lpwhr->lpszPath[nLen] == '\\') lpwhr->lpszPath[nLen]='/';
        }
    }

    if(CSTR_EQUAL != CompareStringW( LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       lpwhr->lpszPath, strlenW(szHttp), szHttp, strlenW(szHttp) )
       && lpwhr->lpszPath[0] != '/') /* not an absolute path ?? --> fix it !! */
    {
        WCHAR *fixurl = HeapAlloc(GetProcessHeap(), 0,
                             (strlenW(lpwhr->lpszPath) + 2)*sizeof(WCHAR));
        *fixurl = '/';
        strcpyW(fixurl + 1, lpwhr->lpszPath);
        HeapFree( GetProcessHeap(), 0, lpwhr->lpszPath );
        lpwhr->lpszPath = fixurl;
    }
}

static LPWSTR HTTP_BuildHeaderRequestString( LPWININETHTTPREQW lpwhr, LPCWSTR verb, LPCWSTR path, BOOL http1_1 )
{
    LPWSTR requestString;
    DWORD len, n;
    LPCWSTR *req;
    INT i;
    LPWSTR p;

    static const WCHAR szSpace[] = { ' ',0 };
    static const WCHAR szcrlf[] = {'\r','\n', 0};
    static const WCHAR szColon[] = { ':',' ',0 };
    static const WCHAR sztwocrlf[] = {'\r','\n','\r','\n', 0};

    /* allocate space for an array of all the string pointers to be added */
    len = (lpwhr->nCustHeaders)*4 + 9;
    req = HeapAlloc( GetProcessHeap(), 0, len*sizeof(LPCWSTR) );

    /* add the verb, path and HTTP version string */
    n = 0;
    req[n++] = verb;
    req[n++] = szSpace;
    req[n++] = path;
    req[n++] = http1_1 ? g_szHttp1_1 : g_szHttp1_0;

    /* Append custom request heades */
    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
        if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
        {
            req[n++] = szcrlf;
            req[n++] = lpwhr->pCustHeaders[i].lpszField;
            req[n++] = szColon;
            req[n++] = lpwhr->pCustHeaders[i].lpszValue;

            TRACE("Adding custom header %s (%s)\n",
                   debugstr_w(lpwhr->pCustHeaders[i].lpszField),
                   debugstr_w(lpwhr->pCustHeaders[i].lpszValue));
        }
    }

    if( n >= len )
        ERR("oops. buffer overrun\n");

    req[n] = NULL;
    requestString = HTTP_build_req( req, 4 );
    HeapFree( GetProcessHeap(), 0, req );

    /*
     * Set (header) termination string for request
     * Make sure there's exactly two new lines at the end of the request
     */
    p = &requestString[strlenW(requestString)-1];
    while ( (*p == '\n') || (*p == '\r') )
       p--;
    strcpyW( p+1, sztwocrlf );

    return requestString;
}

static void HTTP_ProcessHeaders( LPWININETHTTPREQW lpwhr )
{
    static const WCHAR szSet_Cookie[] = { 'S','e','t','-','C','o','o','k','i','e',0 };
    int HeaderIndex;
    LPHTTPHEADERW setCookieHeader;

    HeaderIndex = HTTP_GetCustomHeaderIndex(lpwhr, szSet_Cookie, 0, FALSE);
    if (HeaderIndex == -1)
            return;
    setCookieHeader = &lpwhr->pCustHeaders[HeaderIndex];

    if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES) && setCookieHeader->lpszValue)
    {
        int nPosStart = 0, nPosEnd = 0, len;
        static const WCHAR szFmt[] = { 'h','t','t','p',':','/','/','%','s','/',0};

        while (setCookieHeader->lpszValue[nPosEnd] != '\0')
        {
            LPWSTR buf_cookie, cookie_name, cookie_data;
            LPWSTR buf_url;
            LPWSTR domain = NULL;
            LPHTTPHEADERW Host;

            int nEqualPos = 0;
            while (setCookieHeader->lpszValue[nPosEnd] != ';' && setCookieHeader->lpszValue[nPosEnd] != ',' &&
                   setCookieHeader->lpszValue[nPosEnd] != '\0')
            {
                nPosEnd++;
            }
            if (setCookieHeader->lpszValue[nPosEnd] == ';')
            {
                /* fixme: not case sensitive, strcasestr is gnu only */
                int nDomainPosEnd = 0;
                int nDomainPosStart = 0, nDomainLength = 0;
                static const WCHAR szDomain[] = {'d','o','m','a','i','n','=',0};
                LPWSTR lpszDomain = strstrW(&setCookieHeader->lpszValue[nPosEnd], szDomain);
                if (lpszDomain)
                { /* they have specified their own domain, lets use it */
                    while (lpszDomain[nDomainPosEnd] != ';' && lpszDomain[nDomainPosEnd] != ',' &&
                           lpszDomain[nDomainPosEnd] != '\0')
                    {
                        nDomainPosEnd++;
                    }
                    nDomainPosStart = strlenW(szDomain);
                    nDomainLength = (nDomainPosEnd - nDomainPosStart) + 1;
                    domain = HeapAlloc(GetProcessHeap(), 0, (nDomainLength + 1)*sizeof(WCHAR));
                    lstrcpynW(domain, &lpszDomain[nDomainPosStart], nDomainLength + 1);
                }
            }
            if (setCookieHeader->lpszValue[nPosEnd] == '\0') break;
            buf_cookie = HeapAlloc(GetProcessHeap(), 0, ((nPosEnd - nPosStart) + 1)*sizeof(WCHAR));
            lstrcpynW(buf_cookie, &setCookieHeader->lpszValue[nPosStart], (nPosEnd - nPosStart) + 1);
            TRACE("%s\n", debugstr_w(buf_cookie));
            while (buf_cookie[nEqualPos] != '=' && buf_cookie[nEqualPos] != '\0')
            {
                nEqualPos++;
            }
            if (buf_cookie[nEqualPos] == '\0' || buf_cookie[nEqualPos + 1] == '\0')
            {
                HeapFree(GetProcessHeap(), 0, buf_cookie);
                break;
            }

            cookie_name = HeapAlloc(GetProcessHeap(), 0, (nEqualPos + 1)*sizeof(WCHAR));
            lstrcpynW(cookie_name, buf_cookie, nEqualPos + 1);
            cookie_data = &buf_cookie[nEqualPos + 1];

            Host = HTTP_GetHeader(lpwhr,szHost);
            len = lstrlenW((domain ? domain : (Host?Host->lpszValue:NULL))) +
                strlenW(lpwhr->lpszPath) + 9;
            buf_url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
            sprintfW(buf_url, szFmt, (domain ? domain : (Host?Host->lpszValue:NULL))); /* FIXME PATH!!! */
            InternetSetCookieW(buf_url, cookie_name, cookie_data);

            HeapFree(GetProcessHeap(), 0, buf_url);
            HeapFree(GetProcessHeap(), 0, buf_cookie);
            HeapFree(GetProcessHeap(), 0, cookie_name);
            HeapFree(GetProcessHeap(), 0, domain);
            nPosStart = nPosEnd;
        }
    }
}

static void HTTP_AddProxyInfo( LPWININETHTTPREQW lpwhr )
{
    LPWININETHTTPSESSIONW lpwhs = (LPWININETHTTPSESSIONW)lpwhr->hdr.lpwhparent;
    LPWININETAPPINFOW hIC = (LPWININETAPPINFOW)lpwhs->hdr.lpwhparent;

    assert(lpwhs->hdr.htype == WH_HHTTPSESSION);
    assert(hIC->hdr.htype == WH_HINIT);

    if (hIC && (hIC->lpszProxyUsername || hIC->lpszProxyPassword ))
        HTTP_InsertProxyAuthorization(lpwhr, hIC->lpszProxyUsername,
                hIC->lpszProxyPassword);
}

/***********************************************************************
 *           HTTP_HttpAddRequestHeadersW (internal)
 */
static BOOL WINAPI HTTP_HttpAddRequestHeadersW(LPWININETHTTPREQW lpwhr,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPWSTR lpszStart;
    LPWSTR lpszEnd;
    LPWSTR buffer;
    BOOL bSuccess = FALSE;
    DWORD len;

    TRACE("copying header: %s\n", debugstr_w(lpszHeader));

    if( dwHeaderLength == ~0U )
        len = strlenW(lpszHeader);
    else
        len = dwHeaderLength;
    buffer = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR)*(len+1) );
    lstrcpynW( buffer, lpszHeader, len + 1);

    lpszStart = buffer;

    do
    {
        LPWSTR * pFieldAndValue;

        lpszEnd = lpszStart;

        while (*lpszEnd != '\0')
        {
            if (*lpszEnd == '\r' && *(lpszEnd + 1) == '\n')
                 break;
            lpszEnd++;
        }

        if (*lpszStart == '\0')
	    break;

        if (*lpszEnd == '\r')
        {
            *lpszEnd = '\0';
            lpszEnd += 2; /* Jump over \r\n */
        }
        TRACE("interpreting header %s\n", debugstr_w(lpszStart));
        pFieldAndValue = HTTP_InterpretHttpHeader(lpszStart);
        if (pFieldAndValue)
        {
            bSuccess = HTTP_ProcessHeader(lpwhr, pFieldAndValue[0],
                pFieldAndValue[1], dwModifier | HTTP_ADDHDR_FLAG_REQ);
            HTTP_FreeTokens(pFieldAndValue);
        }

        lpszStart = lpszEnd;
    } while (bSuccess);

    HeapFree(GetProcessHeap(), 0, buffer);

    return bSuccess;
}

/***********************************************************************
 *           HttpAddRequestHeadersW (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersW(HINTERNET hHttpRequest,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQW lpwhr;

    TRACE("%p, %s, %li, %li\n", hHttpRequest, debugstr_w(lpszHeader), dwHeaderLength,
          dwModifier);

    if (!lpszHeader)
      return TRUE;

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        goto lend;
    }
    bSuccess = HTTP_HttpAddRequestHeadersW( lpwhr, lpszHeader, dwHeaderLength, dwModifier );
lend:
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );

    return bSuccess;
}

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest,
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    DWORD len;
    LPWSTR hdr;
    BOOL r;

    TRACE("%p, %s, %li, %li\n", hHttpRequest, debugstr_a(lpszHeader), dwHeaderLength,
          dwModifier);

    len = MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, NULL, 0 );
    hdr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, hdr, len );
    if( dwHeaderLength != ~0U )
        dwHeaderLength = len;

    r = HttpAddRequestHeadersW( hHttpRequest, hdr, dwHeaderLength, dwModifier );

    HeapFree( GetProcessHeap(), 0, hdr );

    return r;
}

/***********************************************************************
 *           HttpEndRequestA (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestA(HINTERNET hRequest,
        LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD dwContext)
{
    LPINTERNET_BUFFERSA ptr;
    LPINTERNET_BUFFERSW lpBuffersOutW,ptrW;
    BOOL rc = FALSE;

    TRACE("(%p, %p, %08lx, %08lx): stub\n", hRequest, lpBuffersOut, dwFlags,
            dwContext);

    ptr = lpBuffersOut;
    if (ptr)
        lpBuffersOutW = (LPINTERNET_BUFFERSW)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, sizeof(INTERNET_BUFFERSW));
    else
        lpBuffersOutW = NULL;

    ptrW = lpBuffersOutW;
    while (ptr)
    {
        if (ptr->lpvBuffer && ptr->dwBufferLength)
            ptrW->lpvBuffer = HeapAlloc(GetProcessHeap(),0,ptr->dwBufferLength);
        ptrW->dwBufferLength = ptr->dwBufferLength;
        ptrW->dwBufferTotal= ptr->dwBufferTotal;

        if (ptr->Next)
            ptrW->Next = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                    sizeof(INTERNET_BUFFERSW));

        ptr = ptr->Next;
        ptrW = ptrW->Next;
    }

    rc = HttpEndRequestW(hRequest, lpBuffersOutW, dwFlags, dwContext);

    if (lpBuffersOutW)
    {
        ptrW = lpBuffersOutW;
        while (ptrW)
        {
            LPINTERNET_BUFFERSW ptrW2;

            FIXME("Do we need to translate info out of these buffer?\n");

            HeapFree(GetProcessHeap(),0,(LPVOID)ptrW->lpvBuffer);
            ptrW2 = ptrW->Next;
            HeapFree(GetProcessHeap(),0,ptrW);
            ptrW = ptrW2;
        }
    }

    return rc;
}

/***********************************************************************
 *           HttpEndRequestW (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestW(HINTERNET hRequest,
        LPINTERNET_BUFFERSW lpBuffersOut, DWORD dwFlags, DWORD dwContext)
{
    BOOL rc = FALSE;
    LPWININETHTTPREQW lpwhr;
    INT responseLen;

    TRACE("-->\n");
    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hRequest );

    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    	return FALSE;
    }

    lpwhr->hdr.dwFlags |= dwFlags;
    lpwhr->hdr.dwContext = dwContext;

    SendAsyncCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
            INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    responseLen = HTTP_GetResponseHeaders(lpwhr);
    if (responseLen)
	    rc = TRUE;

    SendAsyncCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
            INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen, sizeof(DWORD));

    /* process headers here. Is this right? */
    HTTP_ProcessHeaders(lpwhr);

    /* We appear to do nothing with the buffer.. is that correct? */

    if(!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT))
    {
        DWORD dwCode,dwCodeLength=sizeof(DWORD),dwIndex=0;
        if(HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,&dwCode,&dwCodeLength,&dwIndex) &&
            (dwCode==302 || dwCode==301))
        {
            WCHAR szNewLocation[2048];
            DWORD dwBufferSize=2048;
            dwIndex=0;
            if(HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,&dwIndex))
            {
	            static const WCHAR szGET[] = { 'G','E','T', 0 };
                /* redirects are always GETs */
                HeapFree(GetProcessHeap(),0,lpwhr->lpszVerb);
	            lpwhr->lpszVerb = WININET_strdupW(szGET);
                return HTTP_HandleRedirect(lpwhr, szNewLocation, NULL, 0, NULL, 0, 0);
            }
        }
    }

    TRACE("%i <--\n",rc);
    return rc;
}

/***********************************************************************
 *           HttpOpenRequestW (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONW lpwhs;
    HINTERNET handle = NULL;

    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",debugstr_w(lpszAcceptTypes[i]));
    }

    lpwhs = (LPWININETHTTPSESSIONW) WININET_GetObject( hHttpSession );
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	goto lend;
    }

    /*
     * My tests seem to show that the windows version does not
     * become asynchronous until after this point. And anyhow
     * if this call was asynchronous then how would you get the
     * necessary HINTERNET pointer returned by this function.
     *
     */
    handle = HTTP_HttpOpenRequestW(lpwhs, lpszVerb, lpszObjectName,
                                   lpszVersion, lpszReferrer, lpszAcceptTypes,
                                   dwFlags, dwContext);
lend:
    if( lpwhs )
        WININET_Release( &lpwhs->hdr );
    TRACE("returning %p\n", handle);
    return handle;
}


/***********************************************************************
 *           HttpOpenRequestA (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWSTR szVerb = NULL, szObjectName = NULL;
    LPWSTR szVersion = NULL, szReferrer = NULL, *szAcceptTypes = NULL;
    INT len;
    INT acceptTypesCount;
    HINTERNET rc = FALSE;
    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_a(lpszVerb), debugstr_a(lpszObjectName),
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszVerb, -1, NULL, 0 );
        szVerb = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if ( !szVerb )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszVerb, -1, szVerb, len);
    }

    if (lpszObjectName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszObjectName, -1, NULL, 0 );
        szObjectName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if ( !szObjectName )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszObjectName, -1, szObjectName, len );
    }

    if (lpszVersion)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszVersion, -1, NULL, 0 );
        szVersion = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if ( !szVersion )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszVersion, -1, szVersion, len );
    }

    if (lpszReferrer)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszReferrer, -1, NULL, 0 );
        szReferrer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if ( !szReferrer )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszReferrer, -1, szReferrer, len );
    }

    acceptTypesCount = 0;
    if (lpszAcceptTypes)
    {
        /* find out how many there are */
        while (lpszAcceptTypes[acceptTypesCount])
            acceptTypesCount++;
        szAcceptTypes = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *) * (acceptTypesCount+1));
        acceptTypesCount = 0;
        while (lpszAcceptTypes[acceptTypesCount])
        {
            len = MultiByteToWideChar(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, NULL, 0 );
            szAcceptTypes[acceptTypesCount] = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!szAcceptTypes[acceptTypesCount] )
                goto end;
            MultiByteToWideChar(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, szAcceptTypes[acceptTypesCount], len );
            acceptTypesCount++;
        }
        szAcceptTypes[acceptTypesCount] = NULL;
    }
    else szAcceptTypes = 0;

    rc = HttpOpenRequestW(hHttpSession, szVerb, szObjectName,
                          szVersion, szReferrer,
                          (LPCWSTR*)szAcceptTypes, dwFlags, dwContext);

end:
    if (szAcceptTypes)
    {
        acceptTypesCount = 0;
        while (szAcceptTypes[acceptTypesCount])
        {
            HeapFree(GetProcessHeap(), 0, szAcceptTypes[acceptTypesCount]);
            acceptTypesCount++;
        }
        HeapFree(GetProcessHeap(), 0, szAcceptTypes);
    }
    HeapFree(GetProcessHeap(), 0, szReferrer);
    HeapFree(GetProcessHeap(), 0, szVersion);
    HeapFree(GetProcessHeap(), 0, szObjectName);
    HeapFree(GetProcessHeap(), 0, szVerb);

    return rc;
}

/***********************************************************************
 *  HTTP_Base64
 */
static UINT HTTP_Base64( LPCWSTR bin, LPWSTR base64 )
{
    UINT n = 0, x;
    static LPCSTR HTTP_Base64Enc =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while( bin[0] )
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = HTTP_Base64Enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if( !bin[1] )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[1]&0xf0) >> 4 ) ];
        x = ( bin[1] & 0x0f ) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if( !bin[2] )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[2]&0xc0 ) >> 6 ) ];

        /* last 6 bits, all from bin [2] */
        base64[n++] = HTTP_Base64Enc[ bin[2] & 0x3f ];
        bin += 3;
    }
    base64[n] = 0;
    return n;
}

/***********************************************************************
 *  HTTP_EncodeBasicAuth
 *
 *  Encode the basic authentication string for HTTP 1.1
 */
static LPWSTR HTTP_EncodeBasicAuth( LPCWSTR username, LPCWSTR password)
{
    UINT len;
    LPWSTR in, out;
    static const WCHAR szBasic[] = {'B','a','s','i','c',' ',0};
    static const WCHAR szColon[] = {':',0};

    len = lstrlenW( username ) + 1 + lstrlenW ( password ) + 1;
    in = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if( !in )
        return NULL;

    len = lstrlenW(szBasic) +
          (lstrlenW( username ) + 1 + lstrlenW ( password ))*2 + 1 + 1;
    out = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if( out )
    {
        lstrcpyW( in, username );
        lstrcatW( in, szColon );
        lstrcatW( in, password );
        lstrcpyW( out, szBasic );
        HTTP_Base64( in, &out[strlenW(out)] );
    }
    HeapFree( GetProcessHeap(), 0, in );

    return out;
}

/***********************************************************************
 *  HTTP_InsertProxyAuthorization
 *
 *   Insert the basic authorization field in the request header
 */
static BOOL HTTP_InsertProxyAuthorization( LPWININETHTTPREQW lpwhr,
                       LPCWSTR username, LPCWSTR password )
{
    WCHAR *authorization = HTTP_EncodeBasicAuth( username, password );
    BOOL ret = TRUE;

    if (!authorization)
        return FALSE;

    TRACE( "Inserting authorization: %s\n", debugstr_w( authorization ) );

    HTTP_ProcessHeader(lpwhr, szProxy_Authorization, authorization,
            HTTP_ADDHDR_FLAG_REPLACE);

    HeapFree( GetProcessHeap(), 0, authorization );

    return ret;
}

/***********************************************************************
 *           HTTP_DealWithProxy
 */
static BOOL HTTP_DealWithProxy( LPWININETAPPINFOW hIC,
    LPWININETHTTPSESSIONW lpwhs, LPWININETHTTPREQW lpwhr)
{
    WCHAR buf[MAXHOSTNAME];
    WCHAR proxy[MAXHOSTNAME + 15]; /* 15 == "http://" + sizeof(port#) + ":/\0" */
    WCHAR* url;
    static const WCHAR szNul[] = { 0 };
    URL_COMPONENTSW UrlComponents;
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/',0 }, szSlash[] = { '/',0 } ;
    static const WCHAR szFormat1[] = { 'h','t','t','p',':','/','/','%','s',0 };
    static const WCHAR szFormat2[] = { 'h','t','t','p',':','/','/','%','s',':','%','d',0 };
    int len;

    memset( &UrlComponents, 0, sizeof UrlComponents );
    UrlComponents.dwStructSize = sizeof UrlComponents;
    UrlComponents.lpszHostName = buf;
    UrlComponents.dwHostNameLength = MAXHOSTNAME;

    if( CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                 hIC->lpszProxy,strlenW(szHttp),szHttp,strlenW(szHttp)) )
        sprintfW(proxy, szFormat1, hIC->lpszProxy);
    else
	strcpyW(proxy, hIC->lpszProxy);
    if( !InternetCrackUrlW(proxy, 0, 0, &UrlComponents) )
        return FALSE;
    if( UrlComponents.dwHostNameLength == 0 )
        return FALSE;

    if( !lpwhr->lpszPath )
        lpwhr->lpszPath = (LPWSTR)szNul;
    TRACE("server='%s' path='%s'\n",
          debugstr_w(lpwhs->lpszHostName), debugstr_w(lpwhr->lpszPath));
    /* for constant 15 see above */
    len = strlenW(lpwhs->lpszHostName) + strlenW(lpwhr->lpszPath) + 15;
    url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));

    if(UrlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        UrlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

    sprintfW(url, szFormat2, lpwhs->lpszHostName, lpwhs->nHostPort);

    if( lpwhr->lpszPath[0] != '/' )
        strcatW( url, szSlash );
    strcatW(url, lpwhr->lpszPath);
    if(lpwhr->lpszPath != szNul)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath = url;

    HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
    lpwhs->lpszServerName = WININET_strdupW(UrlComponents.lpszHostName);
    lpwhs->nServerPort = UrlComponents.nPort;

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpOpenRequestW (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HTTP_HttpOpenRequestW(LPWININETHTTPSESSIONW lpwhs,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETAPPINFOW hIC = NULL;
    LPWININETHTTPREQW lpwhr;
    LPWSTR lpszCookies;
    LPWSTR lpszUrl = NULL;
    DWORD nCookieSize;
    HINTERNET handle = NULL;
    static const WCHAR szUrlForm[] = {'h','t','t','p',':','/','/','%','s',0};
    DWORD len;
    LPHTTPHEADERW Host;
    char szaddr[32];

    TRACE("-->\n");

    assert( lpwhs->hdr.htype == WH_HHTTPSESSION );
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    lpwhr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPREQW));
    if (NULL == lpwhr)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        goto lend;
    }
    lpwhr->hdr.htype = WH_HHTTPREQ;
    lpwhr->hdr.lpwhparent = WININET_AddRef( &lpwhs->hdr );
    lpwhr->hdr.dwFlags = dwFlags;
    lpwhr->hdr.dwContext = dwContext;
    lpwhr->hdr.dwRefCount = 1;
    lpwhr->hdr.destroy = HTTP_CloseHTTPRequestHandle;
    lpwhr->hdr.lpfnStatusCB = lpwhs->hdr.lpfnStatusCB;

    handle = WININET_AllocHandle( &lpwhr->hdr );
    if (NULL == handle)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        goto lend;
    }

    if (!NETCON_init(&lpwhr->netConnection, dwFlags & INTERNET_FLAG_SECURE))
    {
        InternetCloseHandle( handle );
        handle = NULL;
        goto lend;
    }

    if (NULL != lpszObjectName && strlenW(lpszObjectName)) {
        HRESULT rc;

        len = 0;
        rc = UrlEscapeW(lpszObjectName, NULL, &len, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            len = strlenW(lpszObjectName)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        rc = UrlEscapeW(lpszObjectName, lpwhr->lpszPath, &len,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(lpszObjectName),rc);
            strcpyW(lpwhr->lpszPath,lpszObjectName);
        }
    }

    if (NULL != lpszReferrer && strlenW(lpszReferrer))
        HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpszReferrer, HTTP_ADDHDR_FLAG_COALESCE);

    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            HTTP_ProcessHeader(lpwhr, HTTP_ACCEPT, lpszAcceptTypes[i], HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_REQ|HTTP_ADDHDR_FLAG_ADD_IF_NEW);
    }

    if (NULL == lpszVerb)
    {
        static const WCHAR szGet[] = {'G','E','T',0};
        lpwhr->lpszVerb = WININET_strdupW(szGet);
    }
    else if (strlenW(lpszVerb))
        lpwhr->lpszVerb = WININET_strdupW(lpszVerb);

    if (NULL != lpszReferrer && strlenW(lpszReferrer))
    {
        WCHAR buf[MAXHOSTNAME];
        URL_COMPONENTSW UrlComponents;

        memset( &UrlComponents, 0, sizeof UrlComponents );
        UrlComponents.dwStructSize = sizeof UrlComponents;
        UrlComponents.lpszHostName = buf;
        UrlComponents.dwHostNameLength = MAXHOSTNAME;

        InternetCrackUrlW(lpszReferrer, 0, 0, &UrlComponents);
        if (strlenW(UrlComponents.lpszHostName))
            HTTP_ProcessHeader(lpwhr, szHost, UrlComponents.lpszHostName, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);
    }
    else
        HTTP_ProcessHeader(lpwhr, szHost, lpwhs->lpszHostName, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);

    if (lpwhs->nServerPort == INTERNET_INVALID_PORT_NUMBER)
        lpwhs->nServerPort = (dwFlags & INTERNET_FLAG_SECURE ?
                        INTERNET_DEFAULT_HTTPS_PORT :
                        INTERNET_DEFAULT_HTTP_PORT);
    lpwhs->nHostPort = lpwhs->nServerPort;

    if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
        HTTP_DealWithProxy( hIC, lpwhs, lpwhr );

    if (hIC->lpszAgent)
    {
        WCHAR *agent_header;
        static const WCHAR user_agent[] = {'U','s','e','r','-','A','g','e','n','t',':',' ','%','s','\r','\n',0 };

        len = strlenW(hIC->lpszAgent) + strlenW(user_agent);
        agent_header = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        sprintfW(agent_header, user_agent, hIC->lpszAgent );

        HTTP_HttpAddRequestHeadersW(lpwhr, agent_header, strlenW(agent_header),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, agent_header);
    }

    Host = HTTP_GetHeader(lpwhr,szHost);

    len = lstrlenW(Host->lpszValue) + strlenW(szUrlForm);
    lpszUrl = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    sprintfW( lpszUrl, szUrlForm, Host->lpszValue );

    if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES) &&
        InternetGetCookieW(lpszUrl, NULL, NULL, &nCookieSize))
    {
        int cnt = 0;
        static const WCHAR szCookie[] = {'C','o','o','k','i','e',':',' ',0};
        static const WCHAR szcrlf[] = {'\r','\n',0};

        lpszCookies = HeapAlloc(GetProcessHeap(), 0, (nCookieSize + 1 + 8)*sizeof(WCHAR));

        cnt += sprintfW(lpszCookies, szCookie);
        InternetGetCookieW(lpszUrl, NULL, lpszCookies + cnt, &nCookieSize);
        strcatW(lpszCookies, szcrlf);

        HTTP_HttpAddRequestHeadersW(lpwhr, lpszCookies, strlenW(lpszCookies),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, lpszCookies);
    }
    HeapFree(GetProcessHeap(), 0, lpszUrl);


    INTERNET_SendCallback(&lpwhs->hdr, dwContext,
                          INTERNET_STATUS_HANDLE_CREATED, &handle,
                          sizeof(handle));

    /*
     * A STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on windows
     */

    /*
     * According to my tests. The name is not resolved until a request is Opened
     */
    INTERNET_SendCallback(&lpwhr->hdr, dwContext,
                          INTERNET_STATUS_RESOLVING_NAME,
                          lpwhs->lpszServerName,
                          strlenW(lpwhs->lpszServerName)+1);

    if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    &lpwhs->socketAddress))
    {
        INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
        InternetCloseHandle( handle );
        handle = NULL;
        goto lend;
    }

    inet_ntop(lpwhs->socketAddress.sin_family, &lpwhs->socketAddress.sin_addr,
              szaddr, sizeof(szaddr));
    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_NAME_RESOLVED,
                          szaddr, strlen(szaddr)+1);

lend:
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );

    TRACE("<-- %p (%p)\n", handle, lpwhr);
    return handle;
}

typedef struct std_hdr_data
{
	const WCHAR* hdrStr;
	INT hdrIndex;
} std_hdr_data;

static const WCHAR szAccept[] = { 'A','c','c','e','p','t',0 };
static const WCHAR szAccept_Charset[] = { 'A','c','c','e','p','t','-','C','h','a','r','s','e','t', 0 };
static const WCHAR szAccept_Encoding[] = { 'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szAccept_Language[] = { 'A','c','c','e','p','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szAccept_Ranges[] = { 'A','c','c','e','p','t','-','R','a','n','g','e','s',0 };
static const WCHAR szAge[] = { 'A','g','e',0 };
static const WCHAR szAllow[] = { 'A','l','l','o','w',0 };
static const WCHAR szAuthorization[] = { 'A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szCache_Control[] = { 'C','a','c','h','e','-','C','o','n','t','r','o','l',0 };
static const WCHAR szConnection[] = { 'C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szContent_Base[] = { 'C','o','n','t','e','n','t','-','B','a','s','e',0 };
static const WCHAR szContent_Encoding[] = { 'C','o','n','t','e','n','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_ID[] = { 'C','o','n','t','e','n','t','-','I','D',0 };
static const WCHAR szContent_Language[] = { 'C','o','n','t','e','n','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szContent_Length[] = { 'C','o','n','t','e','n','t','-','L','e','n','g','t','h',0 };
static const WCHAR szContent_Location[] = { 'C','o','n','t','e','n','t','-','L','o','c','a','t','i','o','n',0 };
static const WCHAR szContent_MD5[] = { 'C','o','n','t','e','n','t','-','M','D','5',0 };
static const WCHAR szContent_Range[] = { 'C','o','n','t','e','n','t','-','R','a','n','g','e',0 };
static const WCHAR szContent_Transfer_Encoding[] = { 'C','o','n','t','e','n','t','-','T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_Type[] = { 'C','o','n','t','e','n','t','-','T','y','p','e',0 };
static const WCHAR szCookie[] = { 'C','o','o','k','i','e',0 };
static const WCHAR szDate[] = { 'D','a','t','e',0 };
static const WCHAR szFrom[] = { 'F','r','o','m',0 };
static const WCHAR szETag[] = { 'E','T','a','g',0 };
static const WCHAR szExpect[] = { 'E','x','p','e','c','t',0 };
static const WCHAR szExpires[] = { 'E','x','p','i','r','e','s',0 };
static const WCHAR szIf_Match[] = { 'I','f','-','M','a','t','c','h',0 };
static const WCHAR szIf_Modified_Since[] = { 'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szIf_None_Match[] = { 'I','f','-','N','o','n','e','-','M','a','t','c','h',0 };
static const WCHAR szIf_Range[] = { 'I','f','-','R','a','n','g','e',0 };
static const WCHAR szIf_Unmodified_Since[] = { 'I','f','-','U','n','m','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szLast_Modified[] = { 'L','a','s','t','-','M','o','d','i','f','i','e','d',0 };
static const WCHAR szLocation[] = { 'L','o','c','a','t','i','o','n',0 };
static const WCHAR szMax_Forwards[] = { 'M','a','x','-','F','o','r','w','a','r','d','s',0 };
static const WCHAR szMime_Version[] = { 'M','i','m','e','-','V','e','r','s','i','o','n',0 };
static const WCHAR szPragma[] = { 'P','r','a','g','m','a',0 };
static const WCHAR szProxy_Authenticate[] = { 'P','r','o','x','y','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };
static const WCHAR szProxy_Connection[] = { 'P','r','o','x','y','-','C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szPublic[] = { 'P','u','b','l','i','c',0 };
static const WCHAR szRange[] = { 'R','a','n','g','e',0 };
static const WCHAR szReferer[] = { 'R','e','f','e','r','e','r',0 };
static const WCHAR szRetry_After[] = { 'R','e','t','r','y','-','A','f','t','e','r',0 };
static const WCHAR szServer[] = { 'S','e','r','v','e','r',0 };
static const WCHAR szSet_Cookie[] = { 'S','e','t','-','C','o','o','k','i','e',0 };
static const WCHAR szTransfer_Encoding[] = { 'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szUnless_Modified_Since[] = { 'U','n','l','e','s','s','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szUpgrade[] = { 'U','p','g','r','a','d','e',0 };
static const WCHAR szURI[] = { 'U','R','I',0 };
static const WCHAR szUser_Agent[] = { 'U','s','e','r','-','A','g','e','n','t',0 };
static const WCHAR szVary[] = { 'V','a','r','y',0 };
static const WCHAR szVia[] = { 'V','i','a',0 };
static const WCHAR szWarning[] = { 'W','a','r','n','i','n','g',0 };
static const WCHAR szWWW_Authenticate[] = { 'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };

static const std_hdr_data SORTED_STANDARD_HEADERS[] = {
    {szAccept,			HTTP_QUERY_ACCEPT,},
    {szAccept_Charset,		HTTP_QUERY_ACCEPT_CHARSET,},
    {szAccept_Encoding,		HTTP_QUERY_ACCEPT_ENCODING,},
    {szAccept_Language,		HTTP_QUERY_ACCEPT_LANGUAGE,},
    {szAccept_Ranges,		HTTP_QUERY_ACCEPT_RANGES,},
    {szAge,			HTTP_QUERY_AGE,},
    {szAllow,			HTTP_QUERY_ALLOW,},
    {szAuthorization,		HTTP_QUERY_AUTHORIZATION,},
    {szCache_Control,		HTTP_QUERY_CACHE_CONTROL,},
    {szConnection,		HTTP_QUERY_CONNECTION,},
    {szContent_Base,		HTTP_QUERY_CONTENT_BASE,},
    {szContent_Encoding,	HTTP_QUERY_CONTENT_ENCODING,},
    {szContent_ID,		HTTP_QUERY_CONTENT_ID,},
    {szContent_Language,	HTTP_QUERY_CONTENT_LANGUAGE,},
    {szContent_Length,		HTTP_QUERY_CONTENT_LENGTH,},
    {szContent_Location,	HTTP_QUERY_CONTENT_LOCATION,},
    {szContent_MD5,		HTTP_QUERY_CONTENT_MD5,},
    {szContent_Range,		HTTP_QUERY_CONTENT_RANGE,},
    {szContent_Transfer_Encoding,HTTP_QUERY_CONTENT_TRANSFER_ENCODING,},
    {szContent_Type,		HTTP_QUERY_CONTENT_TYPE,},
    {szCookie,			HTTP_QUERY_COOKIE,},
    {szDate,			HTTP_QUERY_DATE,},
    {szETag,			HTTP_QUERY_ETAG,},
    {szExpect,			HTTP_QUERY_EXPECT,},
    {szExpires,			HTTP_QUERY_EXPIRES,},
    {szFrom,			HTTP_QUERY_DERIVED_FROM,},
    {szHost,			HTTP_QUERY_HOST,},
    {szIf_Match,		HTTP_QUERY_IF_MATCH,},
    {szIf_Modified_Since,	HTTP_QUERY_IF_MODIFIED_SINCE,},
    {szIf_None_Match,		HTTP_QUERY_IF_NONE_MATCH,},
    {szIf_Range,		HTTP_QUERY_IF_RANGE,},
    {szIf_Unmodified_Since,	HTTP_QUERY_IF_UNMODIFIED_SINCE,},
    {szLast_Modified,		HTTP_QUERY_LAST_MODIFIED,},
    {szLocation,		HTTP_QUERY_LOCATION,},
    {szMax_Forwards,		HTTP_QUERY_MAX_FORWARDS,},
    {szMime_Version,		HTTP_QUERY_MIME_VERSION,},
    {szPragma,			HTTP_QUERY_PRAGMA,},
    {szProxy_Authenticate,	HTTP_QUERY_PROXY_AUTHENTICATE,},
    {szProxy_Authorization,	HTTP_QUERY_PROXY_AUTHORIZATION,},
    {szProxy_Connection,	HTTP_QUERY_PROXY_CONNECTION,},
    {szPublic,			HTTP_QUERY_PUBLIC,},
    {szRange,			HTTP_QUERY_RANGE,},
    {szReferer,			HTTP_QUERY_REFERER,},
    {szRetry_After,		HTTP_QUERY_RETRY_AFTER,},
    {szServer,			HTTP_QUERY_SERVER,},
    {szSet_Cookie,		HTTP_QUERY_SET_COOKIE,},
    {szStatus,			HTTP_QUERY_STATUS_CODE,},
    {szTransfer_Encoding,	HTTP_QUERY_TRANSFER_ENCODING,},
    {szUnless_Modified_Since,	HTTP_QUERY_UNLESS_MODIFIED_SINCE,},
    {szUpgrade,			HTTP_QUERY_UPGRADE,},
    {szURI,			HTTP_QUERY_URI,},
    {szUser_Agent,		HTTP_QUERY_USER_AGENT,},
    {szVary,			HTTP_QUERY_VARY,},
    {szVia,			HTTP_QUERY_VIA,},
    {szWarning,			HTTP_QUERY_WARNING,},
    {szWWW_Authenticate,	HTTP_QUERY_WWW_AUTHENTICATE,},
};

/***********************************************************************
 *           HTTP_HttpQueryInfoW (internal)
 */
static BOOL WINAPI HTTP_HttpQueryInfoW( LPWININETHTTPREQW lpwhr, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    BOOL request_only = dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS;


    /* Find requested header structure */
    if ((dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK) == HTTP_QUERY_CUSTOM)
    {
        INT requested_index = (lpdwIndex)?(*lpdwIndex):0;
        INT index = HTTP_GetCustomHeaderIndex(lpwhr, (LPWSTR)lpBuffer,
                requested_index,request_only);

        if (index < 0)
            return bSuccess;
        else
            lphttpHdr = &lpwhr->pCustHeaders[index];

        if (lpdwIndex)
            (*lpdwIndex)++;
    }
    else
    {
        INT index = dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK;

        if (index == HTTP_QUERY_RAW_HEADERS_CRLF)
        {
            DWORD len = strlenW(lpwhr->lpszRawHeaders);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            memcpy(lpBuffer, lpwhr->lpszRawHeaders, (len+1)*sizeof(WCHAR));
            *lpdwBufferLength = len * sizeof(WCHAR);

            TRACE("returning data: %s\n", debugstr_wn((WCHAR*)lpBuffer, len));

            return TRUE;
        }
        else if (index == HTTP_QUERY_RAW_HEADERS)
        {
            static const WCHAR szCrLf[] = {'\r','\n',0};
            LPWSTR * ppszRawHeaderLines = HTTP_Tokenize(lpwhr->lpszRawHeaders, szCrLf);
            DWORD i, size = 0;
            LPWSTR pszString = (WCHAR*)lpBuffer;

            for (i = 0; ppszRawHeaderLines[i]; i++)
                size += strlenW(ppszRawHeaderLines[i]) + 1;

            if (size + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                HTTP_FreeTokens(ppszRawHeaderLines);
                *lpdwBufferLength = (size + 1) * sizeof(WCHAR);
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            for (i = 0; ppszRawHeaderLines[i]; i++)
            {
                DWORD len = strlenW(ppszRawHeaderLines[i]);
                memcpy(pszString, ppszRawHeaderLines[i], (len+1)*sizeof(WCHAR));
                pszString += len+1;
            }
            *pszString = '\0';

            TRACE("returning data: %s\n", debugstr_wn((WCHAR*)lpBuffer, size));

            *lpdwBufferLength = size * sizeof(WCHAR);
            HTTP_FreeTokens(ppszRawHeaderLines);

            return TRUE;
        }
        else if (index == HTTP_QUERY_STATUS_TEXT)
        {
            DWORD len = strlenW(lpwhr->lpszStatusText);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            memcpy(lpBuffer, lpwhr->lpszStatusText, (len+1)*sizeof(WCHAR));
            *lpdwBufferLength = len * sizeof(WCHAR);

            TRACE("returning data: %s\n", debugstr_wn((WCHAR*)lpBuffer, len));

            return TRUE;
        }
        else if (index == HTTP_QUERY_VERSION)
        {
            DWORD len = strlenW(lpwhr->lpszVersion);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            memcpy(lpBuffer, lpwhr->lpszVersion, (len+1)*sizeof(WCHAR));
            *lpdwBufferLength = len * sizeof(WCHAR);

            TRACE("returning data: %s\n", debugstr_wn((WCHAR*)lpBuffer, len));

            return TRUE;
        }
	    else if (index >= 0 && index <= HTTP_QUERY_MAX )
	    {
            int i;
            for (i = 0; i <  sizeof(SORTED_STANDARD_HEADERS)/sizeof(std_hdr_data) ; i++)
            {
                if (SORTED_STANDARD_HEADERS[i].hdrIndex == index)
                {
                    INT requested_index = (lpdwIndex)?(*lpdwIndex):0;
                    INT index = HTTP_GetCustomHeaderIndex(lpwhr,
                            (LPWSTR)SORTED_STANDARD_HEADERS[i].hdrStr,
                            requested_index,request_only);

                    if (index < 0)
                        return bSuccess;
                    else
                        lphttpHdr = &lpwhr->pCustHeaders[index];

                    if (lpdwIndex)
                        (*lpdwIndex)++;

                    break;
                }
            }

            if (!lphttpHdr)
            {
                SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
                return bSuccess;
            }
	    }
	    else
        {
            SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
            return bSuccess;
        }
    }

    /* Ensure header satisifies requested attributes */
    if ((dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) &&
	    (~lphttpHdr->wFlags & HDR_ISREQUEST))
    {
        SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
	return bSuccess;
    }

    /* coalesce value to reuqested type */
    if (dwInfoLevel & HTTP_QUERY_FLAG_NUMBER)
    {
	*(int *)lpBuffer = atoiW(lphttpHdr->lpszValue);
	bSuccess = TRUE;

	TRACE(" returning number : %d\n", *(int *)lpBuffer);
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        time_t tmpTime;
        struct tm tmpTM;
        SYSTEMTIME *STHook;

        tmpTime = ConvertTimeString(lphttpHdr->lpszValue);

        tmpTM = *gmtime(&tmpTime);
        STHook = (SYSTEMTIME *) lpBuffer;
        if(STHook==NULL)
            return bSuccess;

	STHook->wDay = tmpTM.tm_mday;
	STHook->wHour = tmpTM.tm_hour;
	STHook->wMilliseconds = 0;
	STHook->wMinute = tmpTM.tm_min;
	STHook->wDayOfWeek = tmpTM.tm_wday;
	STHook->wMonth = tmpTM.tm_mon + 1;
	STHook->wSecond = tmpTM.tm_sec;
	STHook->wYear = tmpTM.tm_year;

	bSuccess = TRUE;

	TRACE(" returning time : %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
	      STHook->wYear, STHook->wMonth, STHook->wDay, STHook->wDayOfWeek,
	      STHook->wHour, STHook->wMinute, STHook->wSecond, STHook->wMilliseconds);
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_COALESCE)
    {
	    if (*lpdwIndex >= lphttpHdr->wCount)
		{
	        INTERNET_SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
		}
	    else
	    {
	    /* Copy strncpyW(lpBuffer, lphttpHdr[*lpdwIndex], len); */
            (*lpdwIndex)++;
	    }
    }
    else if (lphttpHdr->lpszValue)
    {
        DWORD len = (strlenW(lphttpHdr->lpszValue) + 1) * sizeof(WCHAR);

        if (len > *lpdwBufferLength)
        {
            *lpdwBufferLength = len;
            INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return bSuccess;
        }

        memcpy(lpBuffer, lphttpHdr->lpszValue, len);
        *lpdwBufferLength = len - sizeof(WCHAR);
        bSuccess = TRUE;

	TRACE(" returning string : '%s'\n", debugstr_w(lpBuffer));
    }
    return bSuccess;
}

/***********************************************************************
 *           HttpQueryInfoW (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoW(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQW lpwhr;

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info query_flags[] = {
	    FE(HTTP_QUERY_MIME_VERSION),
	    FE(HTTP_QUERY_CONTENT_TYPE),
	    FE(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_CONTENT_ID),
	    FE(HTTP_QUERY_CONTENT_DESCRIPTION),
	    FE(HTTP_QUERY_CONTENT_LENGTH),
	    FE(HTTP_QUERY_CONTENT_LANGUAGE),
	    FE(HTTP_QUERY_ALLOW),
	    FE(HTTP_QUERY_PUBLIC),
	    FE(HTTP_QUERY_DATE),
	    FE(HTTP_QUERY_EXPIRES),
	    FE(HTTP_QUERY_LAST_MODIFIED),
	    FE(HTTP_QUERY_MESSAGE_ID),
	    FE(HTTP_QUERY_URI),
	    FE(HTTP_QUERY_DERIVED_FROM),
	    FE(HTTP_QUERY_COST),
	    FE(HTTP_QUERY_LINK),
	    FE(HTTP_QUERY_PRAGMA),
	    FE(HTTP_QUERY_VERSION),
	    FE(HTTP_QUERY_STATUS_CODE),
	    FE(HTTP_QUERY_STATUS_TEXT),
	    FE(HTTP_QUERY_RAW_HEADERS),
	    FE(HTTP_QUERY_RAW_HEADERS_CRLF),
	    FE(HTTP_QUERY_CONNECTION),
	    FE(HTTP_QUERY_ACCEPT),
	    FE(HTTP_QUERY_ACCEPT_CHARSET),
	    FE(HTTP_QUERY_ACCEPT_ENCODING),
	    FE(HTTP_QUERY_ACCEPT_LANGUAGE),
	    FE(HTTP_QUERY_AUTHORIZATION),
	    FE(HTTP_QUERY_CONTENT_ENCODING),
	    FE(HTTP_QUERY_FORWARDED),
	    FE(HTTP_QUERY_FROM),
	    FE(HTTP_QUERY_IF_MODIFIED_SINCE),
	    FE(HTTP_QUERY_LOCATION),
	    FE(HTTP_QUERY_ORIG_URI),
	    FE(HTTP_QUERY_REFERER),
	    FE(HTTP_QUERY_RETRY_AFTER),
	    FE(HTTP_QUERY_SERVER),
	    FE(HTTP_QUERY_TITLE),
	    FE(HTTP_QUERY_USER_AGENT),
	    FE(HTTP_QUERY_WWW_AUTHENTICATE),
	    FE(HTTP_QUERY_PROXY_AUTHENTICATE),
	    FE(HTTP_QUERY_ACCEPT_RANGES),
        FE(HTTP_QUERY_SET_COOKIE),
        FE(HTTP_QUERY_COOKIE),
	    FE(HTTP_QUERY_REQUEST_METHOD),
	    FE(HTTP_QUERY_REFRESH),
	    FE(HTTP_QUERY_CONTENT_DISPOSITION),
	    FE(HTTP_QUERY_AGE),
	    FE(HTTP_QUERY_CACHE_CONTROL),
	    FE(HTTP_QUERY_CONTENT_BASE),
	    FE(HTTP_QUERY_CONTENT_LOCATION),
	    FE(HTTP_QUERY_CONTENT_MD5),
	    FE(HTTP_QUERY_CONTENT_RANGE),
	    FE(HTTP_QUERY_ETAG),
	    FE(HTTP_QUERY_HOST),
	    FE(HTTP_QUERY_IF_MATCH),
	    FE(HTTP_QUERY_IF_NONE_MATCH),
	    FE(HTTP_QUERY_IF_RANGE),
	    FE(HTTP_QUERY_IF_UNMODIFIED_SINCE),
	    FE(HTTP_QUERY_MAX_FORWARDS),
	    FE(HTTP_QUERY_PROXY_AUTHORIZATION),
	    FE(HTTP_QUERY_RANGE),
	    FE(HTTP_QUERY_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_UPGRADE),
	    FE(HTTP_QUERY_VARY),
	    FE(HTTP_QUERY_VIA),
	    FE(HTTP_QUERY_WARNING),
	    FE(HTTP_QUERY_CUSTOM)
	};
	static const wininet_flag_info modifier_flags[] = {
	    FE(HTTP_QUERY_FLAG_REQUEST_HEADERS),
	    FE(HTTP_QUERY_FLAG_SYSTEMTIME),
	    FE(HTTP_QUERY_FLAG_NUMBER),
	    FE(HTTP_QUERY_FLAG_COALESCE)
	};
#undef FE
	DWORD info_mod = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
	DWORD info = dwInfoLevel & HTTP_QUERY_HEADER_MASK;
	DWORD i;

	TRACE("(%p, 0x%08lx)--> %ld\n", hHttpRequest, dwInfoLevel, dwInfoLevel);
	TRACE("  Attribute:");
	for (i = 0; i < (sizeof(query_flags) / sizeof(query_flags[0])); i++) {
	    if (query_flags[i].val == info) {
		TRACE(" %s", query_flags[i].name);
		break;
	    }
	}
	if (i == (sizeof(query_flags) / sizeof(query_flags[0]))) {
	    TRACE(" Unknown (%08lx)", info);
	}

	TRACE(" Modifier:");
	for (i = 0; i < (sizeof(modifier_flags) / sizeof(modifier_flags[0])); i++) {
	    if (modifier_flags[i].val & info_mod) {
		TRACE(" %s", modifier_flags[i].name);
		info_mod &= ~ modifier_flags[i].val;
	    }
	}

	if (info_mod) {
	    TRACE(" Unknown (%08lx)", info_mod);
	}
	TRACE("\n");
    }

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	goto lend;
    }

    bSuccess = HTTP_HttpQueryInfoW( lpwhr, dwInfoLevel,
	                            lpBuffer, lpdwBufferLength, lpdwIndex);

lend:
    if( lpwhr )
         WININET_Release( &lpwhr->hdr );

    TRACE("%d <--\n", bSuccess);
    return bSuccess;
}

/***********************************************************************
 *           HttpQueryInfoA (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoA(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL result;
    DWORD len;
    WCHAR* bufferW;

    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        return HttpQueryInfoW( hHttpRequest, dwInfoLevel, lpBuffer,
                               lpdwBufferLength, lpdwIndex );
    }

    len = (*lpdwBufferLength)*sizeof(WCHAR);
    bufferW = HeapAlloc( GetProcessHeap(), 0, len );
    /* buffer is in/out because of HTTP_QUERY_CUSTOM */
    if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
        MultiByteToWideChar(CP_ACP,0,lpBuffer,-1,bufferW,len);
    result = HttpQueryInfoW( hHttpRequest, dwInfoLevel, bufferW,
                           &len, lpdwIndex );
    if( result )
    {
        len = WideCharToMultiByte( CP_ACP,0, bufferW, len / sizeof(WCHAR) + 1,
                                     lpBuffer, *lpdwBufferLength, NULL, NULL );
        *lpdwBufferLength = len - 1;

        TRACE("lpBuffer: %s\n", debugstr_a(lpBuffer));
    }
    else
        /* since the strings being returned from HttpQueryInfoW should be
         * only ASCII characters, it is reasonable to assume that all of
         * the Unicode characters can be reduced to a single byte */
        *lpdwBufferLength = len / sizeof(WCHAR);

    HeapFree(GetProcessHeap(), 0, bufferW );

    return result;
}

/***********************************************************************
 *           HttpSendRequestExA (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExA(HINTERNET hRequest,
			       LPINTERNET_BUFFERSA lpBuffersIn,
			       LPINTERNET_BUFFERSA lpBuffersOut,
			       DWORD dwFlags, DWORD dwContext)
{
    INTERNET_BUFFERSW BuffersInW;
    BOOL rc = FALSE;
    DWORD headerlen;

    TRACE("(%p, %p, %p, %08lx, %08lx): stub\n", hRequest, lpBuffersIn,
	    lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersIn)
    {
        BuffersInW.dwStructSize = sizeof(LPINTERNET_BUFFERSW);
        if (lpBuffersIn->lpcszHeader)
        {
            headerlen = MultiByteToWideChar(CP_ACP,0,lpBuffersIn->lpcszHeader,
                    lpBuffersIn->dwHeadersLength,0,0);
            BuffersInW.lpcszHeader = HeapAlloc(GetProcessHeap(),0,headerlen*
                    sizeof(WCHAR));
            if (!BuffersInW.lpcszHeader)
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            BuffersInW.dwHeadersLength = MultiByteToWideChar(CP_ACP, 0,
                    lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                    (LPWSTR)BuffersInW.lpcszHeader, headerlen);
        }
        else
            BuffersInW.lpcszHeader = NULL;
        BuffersInW.dwHeadersTotal = lpBuffersIn->dwHeadersTotal;
        BuffersInW.lpvBuffer = lpBuffersIn->lpvBuffer;
        BuffersInW.dwBufferLength = lpBuffersIn->dwBufferLength;
        BuffersInW.dwBufferTotal = lpBuffersIn->dwBufferTotal;
        BuffersInW.Next = NULL;
    }

    rc = HttpSendRequestExW(hRequest, lpBuffersIn ? &BuffersInW : NULL, NULL, dwFlags, dwContext);

    if (lpBuffersIn)
        HeapFree(GetProcessHeap(),0,(LPVOID)BuffersInW.lpcszHeader);

    return rc;
}

/***********************************************************************
 *           HttpSendRequestExW (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExW(HINTERNET hRequest,
                   LPINTERNET_BUFFERSW lpBuffersIn,
                   LPINTERNET_BUFFERSW lpBuffersOut,
                   DWORD dwFlags, DWORD dwContext)
{
    BOOL ret;
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC;

    TRACE("(%p, %p, %p, %08lx, %08lx)\n", hRequest, lpBuffersIn,
            lpBuffersOut, dwFlags, dwContext);

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hRequest );

    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    	return FALSE;
    }

    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    assert(lpwhs->hdr.htype == WH_HHTTPSESSION);
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    assert(hIC->hdr.htype == WH_HINIT);

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncall = HTTPSENDREQUESTW;
        workRequest.hdr = WININET_AddRef( &lpwhr->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpBuffersIn)
        {
            if (lpBuffersIn->lpcszHeader)
                /* FIXME: this should use dwHeadersLength or may not be necessary at all */
                req->lpszHeader = WININET_strdupW(lpBuffersIn->lpcszHeader);
            else
                req->lpszHeader = NULL;
            req->dwHeaderLength = lpBuffersIn->dwHeadersLength;
            req->lpOptional = lpBuffersIn->lpvBuffer;
            req->dwOptionalLength = lpBuffersIn->dwBufferLength;
            req->dwContentLength = lpBuffersIn->dwBufferTotal;
        }
        else
        {
            req->lpszHeader = NULL;
            req->dwHeaderLength = 0;
            req->lpOptional = NULL;
            req->dwOptionalLength = 0;
            req->dwContentLength = 0;
        }

        req->bEndRequest = FALSE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        SetLastError(ERROR_IO_PENDING);
        ret = FALSE;
    }
    else
    {
        ret = HTTP_HttpSendRequestW(lpwhr, lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                                    lpBuffersIn->lpvBuffer, lpBuffersIn->dwBufferLength,
                                    lpBuffersIn->dwBufferTotal, FALSE);
    }

    WININET_Release(&lpwhr->hdr);
    TRACE("<---\n");
    return ret;
}

/***********************************************************************
 *           HttpSendRequestW (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;
    BOOL r;

    TRACE("%p, %p (%s), %li, %p, %li)\n", hHttpRequest,
            lpszHeaders, debugstr_w(lpszHeaders), dwHeaderLength, lpOptional, dwOptionalLength);

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	r = FALSE;
        goto lend;
    }

    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	r = FALSE;
        goto lend;
    }

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	r = FALSE;
        goto lend;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncall = HTTPSENDREQUESTW;
	workRequest.hdr = WININET_AddRef( &lpwhr->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpszHeaders)
            req->lpszHeader = WININET_strdupW(lpszHeaders);
        else
            req->lpszHeader = 0;
        req->dwHeaderLength = dwHeaderLength;
        req->lpOptional = lpOptional;
        req->dwOptionalLength = dwOptionalLength;
        req->dwContentLength = dwOptionalLength;
        req->bEndRequest = TRUE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        SetLastError(ERROR_IO_PENDING);
        r = FALSE;
    }
    else
    {
	r = HTTP_HttpSendRequestW(lpwhr, lpszHeaders,
		dwHeaderLength, lpOptional, dwOptionalLength,
		dwOptionalLength, TRUE);
    }
lend:
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );
    return r;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    LPWSTR szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        nLen=MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0);
        szHeaders=HeapAlloc(GetProcessHeap(),0,nLen*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen);
    }
    result=HttpSendRequestW(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    HeapFree(GetProcessHeap(),0,szHeaders);
    return result;
}

/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static BOOL HTTP_HandleRedirect(LPWININETHTTPREQW lpwhr, LPCWSTR lpszUrl, LPCWSTR lpszHeaders,
                                DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
                                DWORD dwContentLength)
{
    LPWININETHTTPSESSIONW lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    LPWININETAPPINFOW hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    WCHAR path[2048];
    char szaddr[32];

    if(lpszUrl[0]=='/')
    {
        /* if it's an absolute path, keep the same session info */
        lstrcpynW(path, lpszUrl, 2048);
    }
    else if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
    {
        TRACE("Redirect through proxy\n");
        lstrcpynW(path, lpszUrl, 2048);
    }
    else
    {
        URL_COMPONENTSW urlComponents;
        WCHAR protocol[32], hostName[MAXHOSTNAME], userName[1024];
        static const WCHAR szHttp[] = {'h','t','t','p',0};
        static const WCHAR szHttps[] = {'h','t','t','p','s',0};
        DWORD url_length = 0;
        LPWSTR orig_url;
        LPWSTR combined_url;

        urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
        urlComponents.lpszScheme = (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE) ? (LPWSTR)szHttps : (LPWSTR)szHttp;
        urlComponents.dwSchemeLength = 0;
        urlComponents.lpszHostName = lpwhs->lpszHostName;
        urlComponents.dwHostNameLength = 0;
        urlComponents.nPort = lpwhs->nHostPort;
        urlComponents.lpszUserName = lpwhs->lpszUserName;
        urlComponents.dwUserNameLength = 0;
        urlComponents.lpszPassword = NULL;
        urlComponents.dwPasswordLength = 0;
        urlComponents.lpszUrlPath = lpwhr->lpszPath;
        urlComponents.dwUrlPathLength = 0;
        urlComponents.lpszExtraInfo = NULL;
        urlComponents.dwExtraInfoLength = 0;

        if (!InternetCreateUrlW(&urlComponents, 0, NULL, &url_length) &&
            (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
            return FALSE;

        url_length++; /* for nul terminating character */
        orig_url = HeapAlloc(GetProcessHeap(), 0, url_length * sizeof(WCHAR));

        if (!InternetCreateUrlW(&urlComponents, 0, orig_url, &url_length))
        {
            HeapFree(GetProcessHeap(), 0, orig_url);
            return FALSE;
        }

        url_length = 0;
        if (!InternetCombineUrlW(orig_url, lpszUrl, NULL, &url_length, ICU_ENCODE_SPACES_ONLY) &&
            (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        {
            HeapFree(GetProcessHeap(), 0, orig_url);
            return FALSE;
        }
        combined_url = HeapAlloc(GetProcessHeap(), 0, url_length * sizeof(WCHAR));

        if (!InternetCombineUrlW(orig_url, lpszUrl, combined_url, &url_length, ICU_ENCODE_SPACES_ONLY))
        {
            HeapFree(GetProcessHeap(), 0, orig_url);
            HeapFree(GetProcessHeap(), 0, combined_url);
            return FALSE;
        }
        HeapFree(GetProcessHeap(), 0, orig_url);

        userName[0] = 0;
        hostName[0] = 0;
        protocol[0] = 0;

        urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
        urlComponents.lpszScheme = protocol;
        urlComponents.dwSchemeLength = 32;
        urlComponents.lpszHostName = hostName;
        urlComponents.dwHostNameLength = MAXHOSTNAME;
        urlComponents.lpszUserName = userName;
        urlComponents.dwUserNameLength = 1024;
        urlComponents.lpszPassword = NULL;
        urlComponents.dwPasswordLength = 0;
        urlComponents.lpszUrlPath = path;
        urlComponents.dwUrlPathLength = 2048;
        urlComponents.lpszExtraInfo = NULL;
        urlComponents.dwExtraInfoLength = 0;
        if(!InternetCrackUrlW(combined_url, strlenW(combined_url), 0, &urlComponents))
        {
            HeapFree(GetProcessHeap(), 0, combined_url);
            return FALSE;
        }
        HeapFree(GetProcessHeap(), 0, combined_url);

        if (!strncmpW(szHttp, urlComponents.lpszScheme, strlenW(szHttp)) &&
            (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            TRACE("redirect from secure page to non-secure page\n");
            /* FIXME: warn about from secure redirect to non-secure page */
            lpwhr->hdr.dwFlags &= ~INTERNET_FLAG_SECURE;
        }
        if (!strncmpW(szHttps, urlComponents.lpszScheme, strlenW(szHttps)) &&
            !(lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            TRACE("redirect from non-secure page to secure page\n");
            /* FIXME: notify about redirect to secure page */
            lpwhr->hdr.dwFlags |= INTERNET_FLAG_SECURE;
        }

        if (urlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        {
            if (lstrlenW(protocol)>4) /*https*/
                urlComponents.nPort = INTERNET_DEFAULT_HTTPS_PORT;
            else /*http*/
                urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;
        }

#if 0
        /*
         * This upsets redirects to binary files on sourceforge.net
         * and gives an html page instead of the target file
         * Examination of the HTTP request sent by native wininet.dll
         * reveals that it doesn't send a referrer in that case.
         * Maybe there's a flag that enables this, or maybe a referrer
         * shouldn't be added in case of a redirect.
         */

        /* consider the current host as the referrer */
        if (NULL != lpwhs->lpszServerName && strlenW(lpwhs->lpszServerName))
            HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpwhs->lpszServerName,
                           HTTP_ADDHDR_FLAG_REQ|HTTP_ADDREQ_FLAG_REPLACE|
                           HTTP_ADDHDR_FLAG_ADD_IF_NEW);
#endif

        HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
        lpwhs->lpszServerName = WININET_strdupW(hostName);
        HeapFree(GetProcessHeap(), 0, lpwhs->lpszHostName);
        if (urlComponents.nPort != INTERNET_DEFAULT_HTTP_PORT &&
                urlComponents.nPort != INTERNET_DEFAULT_HTTPS_PORT)
        {
            int len;
            static WCHAR fmt[] = {'%','s',':','%','i',0};
            len = lstrlenW(hostName);
            len += 7; /* 5 for strlen("65535") + 1 for ":" + 1 for '\0' */
            lpwhs->lpszHostName = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
            sprintfW(lpwhs->lpszHostName, fmt, hostName, urlComponents.nPort);
        }
        else
            lpwhs->lpszHostName = WININET_strdupW(hostName);

        HTTP_ProcessHeader(lpwhr, szHost, lpwhs->lpszHostName, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);


        HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
        lpwhs->lpszUserName = NULL;
        if (userName[0])
            lpwhs->lpszUserName = WININET_strdupW(userName);
        lpwhs->nServerPort = urlComponents.nPort;

        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_RESOLVING_NAME,
                              lpwhs->lpszServerName,
                              strlenW(lpwhs->lpszServerName)+1);

        if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    &lpwhs->socketAddress))
        {
            INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
            return FALSE;
        }

        inet_ntop(lpwhs->socketAddress.sin_family, &lpwhs->socketAddress.sin_addr,
              szaddr, sizeof(szaddr));
        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_NAME_RESOLVED,
                              szaddr, strlen(szaddr)+1);

        NETCON_close(&lpwhr->netConnection);

        if (!NETCON_init(&lpwhr->netConnection,lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE))
            return FALSE;
    }

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath=NULL;
    if (strlenW(path))
    {
        DWORD needed = 0;
        HRESULT rc;

        rc = UrlEscapeW(path, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlenW(path)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, needed*sizeof(WCHAR));
        rc = UrlEscapeW(path, lpwhr->lpszPath, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(path),rc);
            strcpyW(lpwhr->lpszPath,path);
        }
    }

    return HTTP_HttpSendRequestW(lpwhr, lpszHeaders, dwHeaderLength, lpOptional,
                                 dwOptionalLength, dwContentLength, TRUE);
}

/***********************************************************************
 *           HTTP_build_req (internal)
 *
 *  concatenate all the strings in the request together
 */
static LPWSTR HTTP_build_req( LPCWSTR *list, int len )
{
    LPCWSTR *t;
    LPWSTR str;

    for( t = list; *t ; t++  )
        len += strlenW( *t );
    len++;

    str = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    *str = 0;

    for( t = list; *t ; t++ )
        strcatW( str, *t );

    return str;
}

static BOOL HTTP_SecureProxyConnect(LPWININETHTTPREQW lpwhr)
{
    LPWSTR lpszPath;
    LPWSTR requestString;
    INT len;
    INT cnt;
    INT responseLen;
    char *ascii_req;
    BOOL ret;
    static const WCHAR szConnect[] = {'C','O','N','N','E','C','T',0};
    static const WCHAR szFormat[] = {'%','s',':','%','d',0};
    LPWININETHTTPSESSIONW lpwhs = (LPWININETHTTPSESSIONW)lpwhr->hdr.lpwhparent;

    TRACE("\n");

    lpszPath = HeapAlloc( GetProcessHeap(), 0, (lstrlenW( lpwhs->lpszHostName ) + 13)*sizeof(WCHAR) );
    sprintfW( lpszPath, szFormat, lpwhs->lpszHostName, lpwhs->nHostPort );
    requestString = HTTP_BuildHeaderRequestString( lpwhr, szConnect, lpszPath, FALSE );
    HeapFree( GetProcessHeap(), 0, lpszPath );

    len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                NULL, 0, NULL, NULL );
    len--; /* the nul terminator isn't needed */
    ascii_req = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                            ascii_req, len, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, requestString );

    TRACE("full request -> %s\n", debugstr_an( ascii_req, len ) );

    ret = NETCON_send( &lpwhr->netConnection, ascii_req, len, 0, &cnt );
    HeapFree( GetProcessHeap(), 0, ascii_req );
    if (!ret || cnt < 0)
        return FALSE;

    responseLen = HTTP_GetResponseHeaders( lpwhr );
    if (!responseLen)
        return FALSE;

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpSendRequestW (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HTTP_HttpSendRequestW(LPWININETHTTPREQW lpwhr, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
	DWORD dwContentLength, BOOL bEndRequest)
{
    INT cnt;
    BOOL bSuccess = FALSE;
    LPWSTR requestString = NULL;
    INT responseLen;
    BOOL loop_next = FALSE;
    INTERNET_ASYNC_RESULT iar;
    LPHTTPHEADERW Host;

    TRACE("--> %p\n", lpwhr);

    assert(lpwhr->hdr.htype == WH_HHTTPREQ);

    /* Clear any error information */
    INTERNET_SetLastError(0);

    HTTP_FixVerb(lpwhr);

    /* if we are using optional stuff, we must add the fixed header of that option length */
    if (dwContentLength > 0)
    {
        static const WCHAR szContentLength[] = {
            'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','%','l','i','\r','\n',0};
        WCHAR contentLengthStr[sizeof szContentLength/2 /* includes \n\r */ + 20 /* int */ ];
        sprintfW(contentLengthStr, szContentLength, dwContentLength);
        HTTP_HttpAddRequestHeadersW(lpwhr, contentLengthStr, -1L, HTTP_ADDREQ_FLAG_ADD);
    }

    Host = HTTP_GetHeader(lpwhr,szHost);
    do
    {
        DWORD len;
        char *ascii_req;

        TRACE("Going to url %s %s\n", debugstr_w(Host->lpszValue), debugstr_w(lpwhr->lpszPath));
        loop_next = FALSE;

        HTTP_FixURL(lpwhr);

        /* add the headers the caller supplied */
        if( lpszHeaders && dwHeaderLength )
        {
            HTTP_HttpAddRequestHeadersW(lpwhr, lpszHeaders, dwHeaderLength,
                        HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REPLACE);
        }

        /* if there's a proxy username and password, add it to the headers */
        HTTP_AddProxyInfo(lpwhr);

        requestString = HTTP_BuildHeaderRequestString(lpwhr, lpwhr->lpszVerb, lpwhr->lpszPath, FALSE);

        TRACE("Request header -> %s\n", debugstr_w(requestString) );

        /* Send the request and store the results */
        if (!HTTP_OpenConnection(lpwhr))
            goto lend;

        /* send the request as ASCII, tack on the optional data */
        if( !lpOptional )
            dwOptionalLength = 0;
        len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                   NULL, 0, NULL, NULL );
        ascii_req = HeapAlloc( GetProcessHeap(), 0, len + dwOptionalLength );
        WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                             ascii_req, len, NULL, NULL );
        if( lpOptional )
            memcpy( &ascii_req[len-1], lpOptional, dwOptionalLength );
        len = (len + dwOptionalLength - 1);
        ascii_req[len] = 0;
        TRACE("full request -> %s\n", debugstr_a(ascii_req) );

        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        NETCON_send(&lpwhr->netConnection, ascii_req, len, 0, &cnt);
        HeapFree( GetProcessHeap(), 0, ascii_req );

        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_REQUEST_SENT,
                              &len, sizeof(DWORD));

        if (bEndRequest)
        {
            INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

            if (cnt < 0)
                goto lend;

            responseLen = HTTP_GetResponseHeaders(lpwhr);
            if (responseLen)
                bSuccess = TRUE;

            INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen,
                                sizeof(DWORD));

            HTTP_ProcessHeaders(lpwhr);
        }
        else
            bSuccess = TRUE;
    }
    while (loop_next);

lend:

    HeapFree(GetProcessHeap(), 0, requestString);

    /* TODO: send notification for P3P header */

    if(!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && bSuccess && bEndRequest)
    {
        DWORD dwCode,dwCodeLength=sizeof(DWORD),dwIndex=0;
        if(HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,&dwCode,&dwCodeLength,&dwIndex) &&
            (dwCode==302 || dwCode==301))
        {
            WCHAR szNewLocation[2048];
            DWORD dwBufferSize=2048;
            dwIndex=0;
            if(HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,&dwIndex))
            {
                INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                      INTERNET_STATUS_REDIRECT, szNewLocation,
                                      dwBufferSize);
                return HTTP_HandleRedirect(lpwhr, szNewLocation, lpszHeaders,
                                           dwHeaderLength, lpOptional, dwOptionalLength,
                                           dwContentLength);
            }
        }
    }


    iar.dwResult = (DWORD)bSuccess;
    iar.dwError = bSuccess ? ERROR_SUCCESS : INTERNET_GetLastError();

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                          sizeof(INTERNET_ASYNC_RESULT));

    TRACE("<--\n");
    return bSuccess;
}


/***********************************************************************
 *           HTTP_Connect  (internal)
 *
 * Create http session handle
 *
 * RETURNS
 *   HINTERNET a session handle on success
 *   NULL on failure
 *
 */
HINTERNET HTTP_Connect(LPWININETAPPINFOW hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD dwContext,
	DWORD dwInternalFlags)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    HINTERNET handle = NULL;

    TRACE("-->\n");

    assert( hIC->hdr.htype == WH_HINIT );

    hIC->hdr.dwContext = dwContext;

    lpwhs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPSESSIONW));
    if (NULL == lpwhs)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    lpwhs->hdr.htype = WH_HHTTPSESSION;
    lpwhs->hdr.lpwhparent = WININET_AddRef( &hIC->hdr );
    lpwhs->hdr.dwFlags = dwFlags;
    lpwhs->hdr.dwContext = dwContext;
    lpwhs->hdr.dwInternalFlags = dwInternalFlags;
    lpwhs->hdr.dwRefCount = 1;
    lpwhs->hdr.destroy = HTTP_CloseHTTPSessionHandle;
    lpwhs->hdr.lpfnStatusCB = hIC->hdr.lpfnStatusCB;

    handle = WININET_AllocHandle( &lpwhs->hdr );
    if (NULL == handle)
    {
        ERR("Failed to alloc handle\n");
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

    if(hIC->lpszProxy && hIC->dwAccessType == INTERNET_OPEN_TYPE_PROXY) {
        if(strchrW(hIC->lpszProxy, ' '))
            FIXME("Several proxies not implemented.\n");
        if(hIC->lpszProxyBypass)
            FIXME("Proxy bypass is ignored.\n");
    }
    if (lpszServerName && lpszServerName[0])
    {
        lpwhs->lpszServerName = WININET_strdupW(lpszServerName);
        lpwhs->lpszHostName = WININET_strdupW(lpszServerName);
    }
    if (lpszUserName && lpszUserName[0])
        lpwhs->lpszUserName = WININET_strdupW(lpszUserName);
    lpwhs->nServerPort = nServerPort;
    lpwhs->nHostPort = nServerPort;

    /* Don't send a handle created callback if this handle was created with InternetOpenUrl */
    if (!(lpwhs->hdr.dwInternalFlags & INET_OPENURL))
    {
        INTERNET_SendCallback(&hIC->hdr, dwContext,
                              INTERNET_STATUS_HANDLE_CREATED, &handle,
                              sizeof(handle));
    }

    bSuccess = TRUE;

lerror:
    if( lpwhs )
        WININET_Release( &lpwhs->hdr );

/*
 * an INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p --> %p (%p)\n", hIC, handle, lpwhs);
    return handle;
}


/***********************************************************************
 *           HTTP_OpenConnection (internal)
 *
 * Connect to a web server
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on failure
 */
static BOOL HTTP_OpenConnection(LPWININETHTTPREQW lpwhr)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC = NULL;
    char szaddr[32];

    TRACE("-->\n");


    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }

    lpwhs = (LPWININETHTTPSESSIONW)lpwhr->hdr.lpwhparent;

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    inet_ntop(lpwhs->socketAddress.sin_family, &lpwhs->socketAddress.sin_addr,
              szaddr, sizeof(szaddr));
    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTING_TO_SERVER,
                          szaddr,
                          strlen(szaddr)+1);

    if (!NETCON_create(&lpwhr->netConnection, lpwhs->socketAddress.sin_family,
                         SOCK_STREAM, 0))
    {
	WARN("Socket creation failed\n");
        goto lend;
    }

    if (!NETCON_connect(&lpwhr->netConnection, (struct sockaddr *)&lpwhs->socketAddress,
                      sizeof(lpwhs->socketAddress)))
       goto lend;

    if (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE)
    {
        /* Note: we differ from Microsoft's WinINet here. they seem to have
         * a bug that causes no status callbacks to be sent when starting
         * a tunnel to a proxy server using the CONNECT verb. i believe our
         * behaviour to be more correct and to not cause any incompatibilities
         * because using a secure connection through a proxy server is a rare
         * case that would be hard for anyone to depend on */
        if (hIC->lpszProxy && !HTTP_SecureProxyConnect(lpwhr))
            goto lend;

        if (!NETCON_secure_connect(&lpwhr->netConnection, lpwhs->lpszHostName))
        {
            WARN("Couldn't connect securely to host\n");
            goto lend;
        }
    }

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTED_TO_SERVER,
                          szaddr, strlen(szaddr)+1);

    bSuccess = TRUE;

lend:
    TRACE("%d <--\n", bSuccess);
    return bSuccess;
}


/***********************************************************************
 *           HTTP_clear_response_headers (internal)
 *
 * clear out any old response headers
 */
static void HTTP_clear_response_headers( LPWININETHTTPREQW lpwhr )
{
    DWORD i;

    for( i=0; i<lpwhr->nCustHeaders; i++)
    {
        if( !lpwhr->pCustHeaders[i].lpszField )
            continue;
        if( !lpwhr->pCustHeaders[i].lpszValue )
            continue;
        if ( lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST )
            continue;
        HTTP_DeleteCustomHeader( lpwhr, i );
        i--;
    }
}

/***********************************************************************
 *           HTTP_GetResponseHeaders (internal)
 *
 * Read server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
static INT HTTP_GetResponseHeaders(LPWININETHTTPREQW lpwhr)
{
    INT cbreaks = 0;
    WCHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    BOOL bSuccess = FALSE;
    INT  rc = 0;
    static const WCHAR szCrLf[] = {'\r','\n',0};
    char bufferA[MAX_REPLY_LEN];
    LPWSTR status_code, status_text;
    DWORD cchMaxRawHeaders = 1024;
    LPWSTR lpszRawHeaders = HeapAlloc(GetProcessHeap(), 0, (cchMaxRawHeaders+1)*sizeof(WCHAR));
    DWORD cchRawHeaders = 0;

    TRACE("-->\n");

    /* clear old response headers (eg. from a redirect response) */
    HTTP_clear_response_headers( lpwhr );

    if (!NETCON_connected(&lpwhr->netConnection))
        goto lend;

    /*
     * HACK peek at the buffer
     */
#if 0
    /* This is Wine code, we don't support MSG_PEEK yet so we have to do it
       a bit different */
    NETCON_recv(&lpwhr->netConnection, buffer, buflen, MSG_PEEK, &rc);
#endif

    /*
     * We should first receive 'HTTP/1.x nnn OK' where nnn is the status code.
     */
    buflen = MAX_REPLY_LEN;
    memset(buffer, 0, MAX_REPLY_LEN);
    if (!NETCON_getNextLine(&lpwhr->netConnection, bufferA, &buflen))
        goto lend;
#if 1
    rc = buflen;
#endif
    MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

    /* regenerate raw headers */
    while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
    {
        cchMaxRawHeaders *= 2;
        lpszRawHeaders = HeapReAlloc(GetProcessHeap(), 0, lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
    }
    memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
    cchRawHeaders += (buflen-1);
    memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
    cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
    lpszRawHeaders[cchRawHeaders] = '\0';

    /* split the version from the status code */
    status_code = strchrW( buffer, ' ' );
    if( !status_code )
        goto lend;
    *status_code++=0;

    /* split the status code from the status text */
    status_text = strchrW( status_code, ' ' );
    if( !status_text )
        goto lend;
    *status_text++=0;

    TRACE("version [%s] status code [%s] status text [%s]\n",
         debugstr_w(buffer), debugstr_w(status_code), debugstr_w(status_text) );

    HTTP_ProcessHeader(lpwhr, szStatus, status_code,
            HTTP_ADDHDR_FLAG_REPLACE);

    HeapFree(GetProcessHeap(),0,lpwhr->lpszVersion);
    HeapFree(GetProcessHeap(),0,lpwhr->lpszStatusText);

    lpwhr->lpszVersion= WININET_strdupW(buffer);
    lpwhr->lpszStatusText = WININET_strdupW(status_text);

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
        if (NETCON_getNextLine(&lpwhr->netConnection, bufferA, &buflen))
        {
            LPWSTR * pFieldAndValue;

#if 1
            rc += buflen;
#endif
            TRACE("got line %s, now interpreting\n", debugstr_a(bufferA));
            MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

            while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
            {
                cchMaxRawHeaders *= 2;
                lpszRawHeaders = HeapReAlloc(GetProcessHeap(), 0, lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
            }
            memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
            cchRawHeaders += (buflen-1);
            memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
            cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
            lpszRawHeaders[cchRawHeaders] = '\0';

            pFieldAndValue = HTTP_InterpretHttpHeader(buffer);
            if (!pFieldAndValue)
                break;

            HTTP_ProcessHeader(lpwhr, pFieldAndValue[0], pFieldAndValue[1],
                HTTP_ADDREQ_FLAG_ADD );

            HTTP_FreeTokens(pFieldAndValue);
	}
	else
	{
	    cbreaks++;
	    if (cbreaks >= 2)
	       break;
	}
    }while(1);

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszRawHeaders);
    lpwhr->lpszRawHeaders = lpszRawHeaders;
    TRACE("raw headers: %s\n", debugstr_w(lpszRawHeaders));
    bSuccess = TRUE;

lend:

    TRACE("<--\n");
    if (bSuccess)
        return rc;
    else
        return 0;
}


static void strip_spaces(LPWSTR start)
{
    LPWSTR str = start;
    LPWSTR end;

    while (*str == ' ' && *str != '\0')
        str++;

    if (str != start)
        memmove(start, str, sizeof(WCHAR) * (strlenW(str) + 1));

    end = start + strlenW(start) - 1;
    while (end >= start && *end == ' ')
    {
        *end = '\0';
        end--;
    }
}


/***********************************************************************
 *           HTTP_InterpretHttpHeader (internal)
 *
 * Parse server response
 *
 * RETURNS
 *
 *   Pointer to array of field, value, NULL on success.
 *   NULL on error.
 */
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer)
{
    LPWSTR * pTokenPair;
    LPWSTR pszColon;
    INT len;

    pTokenPair = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pTokenPair)*3);

    pszColon = strchrW(buffer, ':');
    /* must have two tokens */
    if (!pszColon)
    {
        HTTP_FreeTokens(pTokenPair);
        if (buffer[0])
            TRACE("No ':' in line: %s\n", debugstr_w(buffer));
        return NULL;
    }

    pTokenPair[0] = HeapAlloc(GetProcessHeap(), 0, (pszColon - buffer + 1) * sizeof(WCHAR));
    if (!pTokenPair[0])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[0], buffer, (pszColon - buffer) * sizeof(WCHAR));
    pTokenPair[0][pszColon - buffer] = '\0';

    /* skip colon */
    pszColon++;
    len = strlenW(pszColon);
    pTokenPair[1] = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!pTokenPair[1])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[1], pszColon, (len + 1) * sizeof(WCHAR));

    strip_spaces(pTokenPair[0]);
    strip_spaces(pTokenPair[1]);

    TRACE("field(%s) Value(%s)\n", debugstr_w(pTokenPair[0]), debugstr_w(pTokenPair[1]));
    return pTokenPair;
}

/***********************************************************************
 *           HTTP_ProcessHeader (internal)
 *
 * Stuff header into header tables according to <dwModifier>
 *
 */

#define COALESCEFLASG (HTTP_ADDHDR_FLAG_COALESCE|HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)

static BOOL HTTP_ProcessHeader(LPWININETHTTPREQW lpwhr, LPCWSTR field, LPCWSTR value, DWORD dwModifier)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    INT index = -1;
    static const WCHAR szConnection[] = { 'C','o','n','n','e','c','t','i','o','n',0 };
    BOOL request_only = dwModifier & HTTP_ADDHDR_FLAG_REQ;

    TRACE("--> %s: %s - 0x%08lx\n", debugstr_w(field), debugstr_w(value), dwModifier);

    /* Don't let applications add Connection header to request */
    if (strcmpW(szConnection,field)==0 && (dwModifier & HTTP_ADDHDR_FLAG_REQ))
    {
        return FALSE;
    }

    /* REPLACE wins out over ADD */
    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
        dwModifier &= ~HTTP_ADDHDR_FLAG_ADD;

    if (dwModifier & HTTP_ADDHDR_FLAG_ADD)
        index = -1;
    else
        index = HTTP_GetCustomHeaderIndex(lpwhr, field, 0, request_only);

    if (index >= 0)
    {
        if (dwModifier & HTTP_ADDHDR_FLAG_ADD_IF_NEW)
        {
            return FALSE;
        }
        lphttpHdr = &lpwhr->pCustHeaders[index];
    }
    else if (value)
    {
        HTTPHEADERW hdr;

        hdr.lpszField = (LPWSTR)field;
        hdr.lpszValue = (LPWSTR)value;
        hdr.wFlags = hdr.wCount = 0;

        if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
            hdr.wFlags |= HDR_ISREQUEST;

        return HTTP_InsertCustomHeader(lpwhr, &hdr);
    }

    if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
	    lphttpHdr->wFlags |= HDR_ISREQUEST;
    else
        lphttpHdr->wFlags &= ~HDR_ISREQUEST;

    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
    {
        HTTP_DeleteCustomHeader( lpwhr, index );

        if (value)
        {
            HTTPHEADERW hdr;

            hdr.lpszField = (LPWSTR)field;
            hdr.lpszValue = (LPWSTR)value;
            hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

            return HTTP_InsertCustomHeader(lpwhr, &hdr);
        }

        return TRUE;
    }
    else if (dwModifier & COALESCEFLASG)
    {
        LPWSTR lpsztmp;
        WCHAR ch = 0;
        INT len = 0;
        INT origlen = strlenW(lphttpHdr->lpszValue);
        INT valuelen = strlenW(value);

        if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA)
        {
            ch = ',';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }
        else if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)
        {
            ch = ';';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }

        len = origlen + valuelen + ((ch > 0) ? 2 : 0);

        lpsztmp = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  lphttpHdr->lpszValue, (len+1)*sizeof(WCHAR));
        if (lpsztmp)
        {
            lphttpHdr->lpszValue = lpsztmp;
    /* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
            if (ch > 0)
            {
                lphttpHdr->lpszValue[origlen] = ch;
                origlen++;
                lphttpHdr->lpszValue[origlen] = ' ';
                origlen++;
            }

            memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen*sizeof(WCHAR));
            lphttpHdr->lpszValue[len] = '\0';
            bSuccess = TRUE;
        }
        else
        {
            WARN("HeapReAlloc (%d bytes) failed\n",len+1);
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        }
    }
    TRACE("<-- %d\n",bSuccess);
    return bSuccess;
}


/***********************************************************************
 *           HTTP_CloseConnection (internal)
 *
 * Close socket connection
 *
 */
static VOID HTTP_CloseConnection(LPWININETHTTPREQW lpwhr)
{
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;

    TRACE("%p\n",lpwhr);

    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CLOSING_CONNECTION, 0, 0);

    if (NETCON_connected(&lpwhr->netConnection))
    {
        NETCON_close(&lpwhr->netConnection);
    }

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTION_CLOSED, 0, 0);
}


/***********************************************************************
 *           HTTP_CloseHTTPRequestHandle (internal)
 *
 * Deallocate request handle
 *
 */
static void HTTP_CloseHTTPRequestHandle(LPWININETHANDLEHEADER hdr)
{
    DWORD i;
    LPWININETHTTPREQW lpwhr = (LPWININETHTTPREQW) hdr;

    TRACE("\n");

    if (NETCON_connected(&lpwhr->netConnection))
        HTTP_CloseConnection(lpwhr);

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszVerb);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszRawHeaders);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszVersion);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszStatusText);

    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
        HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszField);
        HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszValue);
    }

    HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders);
    HeapFree(GetProcessHeap(), 0, lpwhr);
}


/***********************************************************************
 *           HTTP_CloseHTTPSessionHandle (internal)
 *
 * Deallocate session handle
 *
 */
static void HTTP_CloseHTTPSessionHandle(LPWININETHANDLEHEADER hdr)
{
    LPWININETHTTPSESSIONW lpwhs = (LPWININETHTTPSESSIONW) hdr;

    TRACE("%p\n", lpwhs);

    HeapFree(GetProcessHeap(), 0, lpwhs->lpszHostName);
    HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
    HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
    HeapFree(GetProcessHeap(), 0, lpwhs);
}


/***********************************************************************
 *           HTTP_GetCustomHeaderIndex (internal)
 *
 * Return index of custom header from header array
 *
 */
static INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQW lpwhr, LPCWSTR lpszField,int requested_index, BOOL request_only)
{
    DWORD index;

    TRACE("%s\n", debugstr_w(lpszField));

    for (index = 0; index < lpwhr->nCustHeaders; index++)
    {
	if (!strcmpiW(lpwhr->pCustHeaders[index].lpszField, lpszField))
    {
        if ((request_only &&
                !(lpwhr->pCustHeaders[index].wFlags & HDR_ISREQUEST))||
            (!request_only &&
                (lpwhr->pCustHeaders[index].wFlags & HDR_ISREQUEST)))
            continue;

        if (requested_index == 0)
    	    break;
        else
            requested_index --;
    }
    }

    if (index >= lpwhr->nCustHeaders)
	index = -1;

    TRACE("Return: %ld\n", index);
    return index;
}


/***********************************************************************
 *           HTTP_InsertCustomHeader (internal)
 *
 * Insert header into array
 *
 */
static BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQW lpwhr, LPHTTPHEADERW lpHdr)
{
    INT count;
    LPHTTPHEADERW lph = NULL;
    BOOL r = FALSE;

    TRACE("--> %s: %s\n", debugstr_w(lpHdr->lpszField), debugstr_w(lpHdr->lpszValue));
    count = lpwhr->nCustHeaders + 1;
    if (count > 1)
	lph = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpwhr->pCustHeaders, sizeof(HTTPHEADERW) * count);
    else
	lph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTPHEADERW) * count);

    if (NULL != lph)
    {
	lpwhr->pCustHeaders = lph;
        lpwhr->pCustHeaders[count-1].lpszField = WININET_strdupW(lpHdr->lpszField);
        lpwhr->pCustHeaders[count-1].lpszValue = WININET_strdupW(lpHdr->lpszValue);
        lpwhr->pCustHeaders[count-1].wFlags = lpHdr->wFlags;
        lpwhr->pCustHeaders[count-1].wCount= lpHdr->wCount;
	lpwhr->nCustHeaders++;
        r = TRUE;
    }
    else
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
    }

    return r;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 *  If this function is called, the indexs may change.
 */
static BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQW lpwhr, DWORD index)
{
    if( lpwhr->nCustHeaders <= 0 )
        return FALSE;
    if( index >= lpwhr->nCustHeaders )
        return FALSE;
    lpwhr->nCustHeaders--;

    memmove( &lpwhr->pCustHeaders[index], &lpwhr->pCustHeaders[index+1],
             (lpwhr->nCustHeaders - index)* sizeof(HTTPHEADERW) );
    memset( &lpwhr->pCustHeaders[lpwhr->nCustHeaders], 0, sizeof(HTTPHEADERW) );

    return TRUE;
}

/***********************************************************************
 *          IsHostInProxyBypassList (@)
 *
 * Undocumented
 *
 */
BOOL WINAPI IsHostInProxyBypassList(DWORD flags, LPCSTR szHost, DWORD length)
{
   FIXME("STUB: flags=%ld host=%s length=%ld\n",flags,szHost,length);
   return FALSE;
}
