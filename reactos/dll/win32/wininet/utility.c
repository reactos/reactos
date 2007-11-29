/*
 * Wininet - Utility functions
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 *
 * Ulrich Czekalla
 * Aric Stewart
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winnls.h"

#include "wine/debug.h"
#include "internet.h"
#define CP_UNIXCP CP_THREAD_ACP

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define TIME_STRING_LEN  30

time_t ConvertTimeString(LPCWSTR asctime)
{
    WCHAR tmpChar[TIME_STRING_LEN];
    WCHAR *tmpChar2;
    struct tm t;
    int timelen = strlenW(asctime);

    if(!timelen)
        return 0;

    /* FIXME: the atoiWs below rely on that tmpChar is \0 padded */
    memset( tmpChar, 0, sizeof(tmpChar) );
    lstrcpynW(tmpChar, asctime, TIME_STRING_LEN);

    /* Assert that the string is the expected length */
    if (strlenW(asctime) >= TIME_STRING_LEN) FIXME("\n");

    /* Convert a time such as 'Mon, 15 Nov 1999 16:09:35 GMT' into a SYSTEMTIME structure
     * We assume the time is in this format
     * and divide it into easy to swallow chunks
     */
    tmpChar[3]='\0';
    tmpChar[7]='\0';
    tmpChar[11]='\0';
    tmpChar[16]='\0';
    tmpChar[19]='\0';
    tmpChar[22]='\0';
    tmpChar[25]='\0';

    t.tm_year = atoiW(tmpChar+12) - 1900;
    t.tm_mday = atoiW(tmpChar+5);
    t.tm_hour = atoiW(tmpChar+17);
    t.tm_min = atoiW(tmpChar+20);
    t.tm_sec = atoiW(tmpChar+23);

    /* and month */
    tmpChar2 = tmpChar + 8;
    switch(tmpChar2[2])
    {
        case 'n':
            if(tmpChar2[1]=='a')
                t.tm_mon = 0;
            else
                t.tm_mon = 5;
            break;
        case 'b':
            t.tm_mon = 1;
            break;
        case 'r':
            if(tmpChar2[1]=='a')
                t.tm_mon = 2;
            else
                t.tm_mon = 3;
            break;
        case 'y':
            t.tm_mon = 4;
            break;
        case 'l':
            t.tm_mon = 6;
            break;
        case 'g':
            t.tm_mon = 7;
            break;
        case 'p':
            t.tm_mon = 8;
            break;
        case 't':
            t.tm_mon = 9;
            break;
        case 'v':
            t.tm_mon = 10;
            break;
        case 'c':
            t.tm_mon = 11;
            break;
        default:
            FIXME("\n");
    }

    return mktime(&t);
}


BOOL GetAddress(LPCWSTR lpszServerName, INTERNET_PORT nServerPort,
	struct sockaddr_in *psa)
{
    WCHAR *found;
    char *name;
    int len, sz;
    struct hostent *phe;

    TRACE("%s\n", debugstr_w(lpszServerName));

    /* Validate server name first
     * Check if there is sth. like
     * pinger.macromedia.com:80
     * if yes, eliminate the :80....
     */
    found = strchrW(lpszServerName, ':');
    if (found)
        len = found - lpszServerName;
    else
        len = strlenW(lpszServerName);

    sz = WideCharToMultiByte( CP_UNIXCP, 0, lpszServerName, len, NULL, 0, NULL, NULL );
    name = HeapAlloc(GetProcessHeap(), 0, sz+1);
    WideCharToMultiByte( CP_UNIXCP, 0, lpszServerName, len, name, sz, NULL, NULL );
    name[sz] = 0;
    phe = gethostbyname(name);
    HeapFree( GetProcessHeap(), 0, name );

    if (NULL == phe)
    {
        TRACE("Failed to get hostname: (%s)\n", debugstr_w(lpszServerName) );
        return FALSE;
    }

    memset(psa,0,sizeof(struct sockaddr_in));
    memcpy((char *)&psa->sin_addr, phe->h_addr, phe->h_length);
    psa->sin_family = phe->h_addrtype;
    psa->sin_port = htons((u_short)nServerPort);

    return TRUE;
}

/*
 * Helper function for sending async Callbacks
 */

static const char *get_callback_name(DWORD dwInternetStatus) {
    static const wininet_flag_info internet_status[] = {
#define FE(x) { x, #x }
	FE(INTERNET_STATUS_RESOLVING_NAME),
	FE(INTERNET_STATUS_NAME_RESOLVED),
	FE(INTERNET_STATUS_CONNECTING_TO_SERVER),
	FE(INTERNET_STATUS_CONNECTED_TO_SERVER),
	FE(INTERNET_STATUS_SENDING_REQUEST),
	FE(INTERNET_STATUS_REQUEST_SENT),
	FE(INTERNET_STATUS_RECEIVING_RESPONSE),
	FE(INTERNET_STATUS_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_CTL_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_PREFETCH),
	FE(INTERNET_STATUS_CLOSING_CONNECTION),
	FE(INTERNET_STATUS_CONNECTION_CLOSED),
	FE(INTERNET_STATUS_HANDLE_CREATED),
	FE(INTERNET_STATUS_HANDLE_CLOSING),
	FE(INTERNET_STATUS_REQUEST_COMPLETE),
	FE(INTERNET_STATUS_REDIRECT),
	FE(INTERNET_STATUS_INTERMEDIATE_RESPONSE),
	FE(INTERNET_STATUS_USER_INPUT_REQUIRED),
	FE(INTERNET_STATUS_STATE_CHANGE),
	FE(INTERNET_STATUS_COOKIE_SENT),
	FE(INTERNET_STATUS_COOKIE_RECEIVED),
	FE(INTERNET_STATUS_PRIVACY_IMPACTED),
	FE(INTERNET_STATUS_P3P_HEADER),
	FE(INTERNET_STATUS_P3P_POLICYREF),
	FE(INTERNET_STATUS_COOKIE_HISTORY)
#undef FE
    };
    DWORD i;

    for (i = 0; i < (sizeof(internet_status) / sizeof(internet_status[0])); i++) {
	if (internet_status[i].val == dwInternetStatus) return internet_status[i].name;
    }
    return "Unknown";
}

VOID INTERNET_SendCallback(LPWININETHANDLEHEADER hdr, DWORD_PTR dwContext,
                           DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                           DWORD dwStatusInfoLength)
{
    LPVOID lpvNewInfo = NULL;

    if( !hdr->lpfnStatusCB )
        return;

    /* the IE5 version of wininet does not
       send callbacks if dwContext is zero */
    if( !dwContext )
        return;

    lpvNewInfo = lpvStatusInfo;
    if(hdr->dwInternalFlags & INET_CALLBACKW) {
        switch(dwInternetStatus) {
        case INTERNET_STATUS_NAME_RESOLVED:
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            lpvNewInfo = WININET_strdup_AtoW(lpvStatusInfo);
            break;
        case INTERNET_STATUS_RESOLVING_NAME:
        case INTERNET_STATUS_REDIRECT:
            lpvNewInfo = WININET_strdupW(lpvStatusInfo);
            break;
        }
    }else {
        switch(dwInternetStatus)
        {
        case INTERNET_STATUS_NAME_RESOLVED:
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            lpvNewInfo = HeapAlloc(GetProcessHeap(), 0, strlen(lpvStatusInfo) + 1);
            if (lpvNewInfo) strcpy(lpvNewInfo, lpvStatusInfo);
            break;
        case INTERNET_STATUS_RESOLVING_NAME:
        case INTERNET_STATUS_REDIRECT:
            lpvNewInfo = WININET_strdup_WtoA(lpvStatusInfo);
            break;
        }
    }
    
    TRACE(" callback(%p) (%p (%p), %08lx, %d (%s), %p, %d)\n",
	  hdr->lpfnStatusCB, hdr->hInternet, hdr, dwContext, dwInternetStatus, get_callback_name(dwInternetStatus),
	  lpvNewInfo, dwStatusInfoLength);
    
    hdr->lpfnStatusCB(hdr->hInternet, dwContext, dwInternetStatus,
                      lpvNewInfo, dwStatusInfoLength);

    TRACE(" end callback().\n");

    if(lpvNewInfo != lpvStatusInfo)
        HeapFree(GetProcessHeap(), 0, lpvNewInfo);
}

static void SendAsyncCallbackProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_SENDCALLBACK const *req = &workRequest->u.SendCallback;

    TRACE("%p\n", workRequest->hdr);

    INTERNET_SendCallback(workRequest->hdr,
                          req->dwContext, req->dwInternetStatus, req->lpvStatusInfo,
                          req->dwStatusInfoLength);

    /* And frees the copy of the status info */
    HeapFree(GetProcessHeap(), 0, req->lpvStatusInfo);
}

VOID SendAsyncCallback(LPWININETHANDLEHEADER hdr, DWORD_PTR dwContext,
                       DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                       DWORD dwStatusInfoLength)
{
    TRACE("(%p, %08lx, %d (%s), %p, %d): %sasync call with callback %p\n",
	  hdr, dwContext, dwInternetStatus, get_callback_name(dwInternetStatus),
	  lpvStatusInfo, dwStatusInfoLength,
	  hdr->dwFlags & INTERNET_FLAG_ASYNC ? "" : "non ",
	  hdr->lpfnStatusCB);
    
    if (!(hdr->lpfnStatusCB))
	return;
    
    if (hdr->dwFlags & INTERNET_FLAG_ASYNC)
    {
	WORKREQUEST workRequest;
	struct WORKREQ_SENDCALLBACK *req;
	void *lpvStatusInfo_copy = lpvStatusInfo;

	if (lpvStatusInfo)
	{
	    lpvStatusInfo_copy = HeapAlloc(GetProcessHeap(), 0, dwStatusInfoLength);
	    memcpy(lpvStatusInfo_copy, lpvStatusInfo, dwStatusInfoLength);
	}

	workRequest.asyncproc = SendAsyncCallbackProc;
	workRequest.hdr = WININET_AddRef( hdr );
	req = &workRequest.u.SendCallback;
	req->dwContext = dwContext;
	req->dwInternetStatus = dwInternetStatus;
	req->lpvStatusInfo = lpvStatusInfo_copy;
	req->dwStatusInfoLength = dwStatusInfoLength;
	
	INTERNET_AsyncCall(&workRequest);
    }
    else
	INTERNET_SendCallback(hdr, dwContext, dwInternetStatus,
			      lpvStatusInfo, dwStatusInfoLength);
}
