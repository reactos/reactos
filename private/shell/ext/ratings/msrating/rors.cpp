//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

/*Included Files------------------------------------------------------------*/
#include "msrating.h"
#pragma hdrstop

#include <npassert.h>
#include <buffer.h>
#include "ratings.h"
#include "mslubase.h"
#include "parselbl.h"

#include "rors.h"
#include "wininet.h"


typedef HINTERNET (WINAPI *PFNInternetOpen)(
    IN LPCTSTR lpszCallerName,
    IN DWORD dwAccessType,
    IN LPCTSTR lpszServerName OPTIONAL,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags
    );
typedef BOOL (WINAPI *PFNInternetCloseHandle)(
    IN HINTERNET hInternet
    );
typedef HINTERNET (WINAPI *PFNInternetConnect)(
    IN HINTERNET hInternet,
    IN LPCTSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCTSTR lpszUsername OPTIONAL,
    IN LPCTSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
typedef BOOL (WINAPI *PFNInternetReadFile)(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );
typedef INTERNET_STATUS_CALLBACK (WINAPI *PFNInternetSetStatusCallback)(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );
typedef HINTERNET (WINAPI *PFNHttpOpenRequest)(
    IN HINTERNET hHttpSession,
    IN LPCTSTR lpszVerb,
    IN LPCTSTR lpszObjectName,
    IN LPCTSTR lpszVersion,
    IN LPCTSTR lpszReferrer OPTIONAL,
    IN LPCTSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
typedef BOOL (WINAPI *PFNHttpSendRequest)(
    IN HINTERNET hHttpRequest,
    IN LPCTSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );
typedef BOOL (WINAPI *PFNInternetCrackUrl)(
    IN LPCTSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS lpUrlComponents
    );
typedef BOOL (WINAPI *PFNInternetCanonicalizeUrl)(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );


PFNInternetReadFile pfnInternetReadFile = NULL;
PFNHttpSendRequest pfnHttpSendRequest = NULL;
PFNInternetOpen pfnInternetOpen = NULL;
PFNInternetSetStatusCallback pfnInternetSetStatusCallback = NULL;
PFNInternetConnect pfnInternetConnect = NULL;
PFNHttpOpenRequest pfnHttpOpenRequest = NULL;
PFNInternetCloseHandle pfnInternetCloseHandle = NULL;
PFNInternetCrackUrl pfnInternetCrackUrl = NULL;
PFNInternetCanonicalizeUrl pfnInternetCanonicalizeUrl = NULL;

#undef InternetReadFile
#undef HttpSendRequest
#undef InternetOpen
#undef InternetSetStatusCallback
#undef InternetConnect
#undef HttpOpenRequest
#undef InternetCloseHandle
#undef InternetCrackUrl
#undef InternetCanonicalizeUrl

#define InternetReadFile pfnInternetReadFile
#define HttpSendRequest pfnHttpSendRequest
#define InternetOpen pfnInternetOpen
#define InternetSetStatusCallback pfnInternetSetStatusCallback
#define InternetConnect pfnInternetConnect
#define HttpOpenRequest pfnHttpOpenRequest
#define InternetCloseHandle pfnInternetCloseHandle
#define InternetCrackUrl pfnInternetCrackUrl
#define InternetCanonicalizeUrl pfnInternetCanonicalizeUrl

struct {
    FARPROC *ppfn;
    LPCSTR pszName;
} aImports[] = {
#ifndef UNICODE
    { (FARPROC *)&pfnInternetReadFile, "InternetReadFile" },
    { (FARPROC *)&pfnHttpSendRequest, "HttpSendRequestA" },
    { (FARPROC *)&pfnInternetOpen, "InternetOpenA" },
    { (FARPROC *)&pfnInternetSetStatusCallback, "InternetSetStatusCallback" },
    { (FARPROC *)&pfnInternetConnect, "InternetConnectA" },
    { (FARPROC *)&pfnHttpOpenRequest, "HttpOpenRequestA" },
    { (FARPROC *)&pfnInternetCloseHandle, "InternetCloseHandle" },
    { (FARPROC *)&pfnInternetCrackUrl, "InternetCrackUrlA" },
    { (FARPROC *)&pfnInternetCanonicalizeUrl, "InternetCanonicalizeUrlA" },
#else
    { (FARPROC *)&pfnInternetReadFile, "InternetReadFile" },
    { (FARPROC *)&pfnHttpSendRequest, "HttpSendRequestW" },
    { (FARPROC *)&pfnInternetOpen, "InternetOpenW" },
    { (FARPROC *)&pfnInternetSetStatusCallback, "InternetSetStatusCallback" },
    { (FARPROC *)&pfnInternetConnect, "InternetConnectW" },
    { (FARPROC *)&pfnHttpOpenRequest, "HttpOpenRequestW" },
    { (FARPROC *)&pfnInternetCloseHandle, "InternetCloseHandle" },
    { (FARPROC *)&pfnInternetCrackUrl, "InternetCrackUrlW" },
    { (FARPROC *)&pfnInternetCanonicalizeUrl, "InternetCanonicalizeUrlW" },
#endif
};

const UINT cImports = sizeof(aImports) / sizeof(aImports[0]);

HINSTANCE hWinINet = NULL;
BOOL fTriedLoad = FALSE;
HINTERNET hI = NULL;

void _stdcall WinInetCallbackProc(HINTERNET hInternet, DWORD_PTR Context, DWORD Status, LPVOID Info, DWORD Length);
#define USER_AGENT_STRING "Batcave(bcrs)"


BOOL LoadWinINet(void)
{
    if (fTriedLoad)
        return (hWinINet != NULL);

    fTriedLoad = TRUE;

    hWinINet = ::LoadLibrary("WININET.DLL");
    if (hWinINet == NULL)
        return FALSE;

    for (UINT i=0; i<cImports; i++) {
        *(aImports[i].ppfn) = ::GetProcAddress(hWinINet, aImports[i].pszName);
        if (*(aImports[i].ppfn) == NULL) {
            CleanupWinINet();
            return FALSE;
        }
    }

    hI = InternetOpen(USER_AGENT_STRING, PRE_CONFIG_INTERNET_ACCESS, NULL, 0, INTERNET_FLAG_ASYNC);
    if (hI == NULL) {
        CleanupWinINet();
        return FALSE;
    }
    InternetSetStatusCallback(hI, WinInetCallbackProc);

    return TRUE;
}


void CleanupWinINet(void)
{
    if (hI != NULL) {
        InternetCloseHandle(hI);
        hI = NULL;
    }

    if (hWinINet != NULL) {
        for (UINT i=0; i<cImports; i++) {
            *(aImports[i].ppfn) = NULL;
        }
        ::FreeLibrary(hWinINet);
        hWinINet = NULL;
    }
}


void _stdcall WinInetCallbackProc(HINTERNET hInternet, DWORD_PTR Context, DWORD Status, LPVOID Info, DWORD Length)
{
    BOOL unknown = FALSE;
    HANDLE  hAsyncEvent = (HANDLE) Context;

    char *type$;
    switch (Status) {
        case INTERNET_STATUS_RESOLVING_NAME:
        type$ = "RESOLVING NAME";
        break;

        case INTERNET_STATUS_NAME_RESOLVED:
        type$ = "NAME RESOLVED";
        break;

        case INTERNET_STATUS_CONNECTING_TO_SERVER:
        type$ = "CONNECTING TO SERVER";
        break;

        case INTERNET_STATUS_CONNECTED_TO_SERVER:
        type$ = "CONNECTED TO SERVER";
        break;

        case INTERNET_STATUS_SENDING_REQUEST:
        type$ = "SENDING REQUEST";
        break;

        case INTERNET_STATUS_REQUEST_SENT:
        type$ = "REQUEST SENT";
        break;

        case INTERNET_STATUS_RECEIVING_RESPONSE:
        type$ = "RECEIVING RESPONSE";
        break;

        case INTERNET_STATUS_RESPONSE_RECEIVED:
        type$ = "RESPONSE RECEIVED";
        break;

        case INTERNET_STATUS_CLOSING_CONNECTION:
        type$ = "CLOSING CONNECTION";
        break;

        case INTERNET_STATUS_CONNECTION_CLOSED:
        type$ = "CONNECTION CLOSED";
        break;

        case INTERNET_STATUS_REQUEST_COMPLETE:
        type$ = "REQUEST COMPLETE";
        SetEvent(hAsyncEvent);
        break;

        default:
        type$ = "???";
        unknown = TRUE;
        break;
    }

/*
    printf("callback: handle %x [context %x ] %s \n",
        hInternet,
        Context,
        type$
        );
*/
}

#define ABORT_EVENT 0
#define ASYNC_EVENT 1


BOOL ShouldAbort(HANDLE hAbort)
{
    return (WAIT_OBJECT_0 == WaitForSingleObject(hAbort, 0));
}

BOOL WaitForAsync(HANDLE rgEvents[])
{
    BOOL fAbort;

//  if (ERROR_IO_PENDING != GetLastError()) return FALSE;       

    fAbort = (WAIT_OBJECT_0 == WaitForMultipleObjects(2, rgEvents, FALSE, INFINITE));
//  fAbort = (WAIT_OBJECT_0 == WaitForSingleObject(rgEvents[ABORT_EVENT], 0));

    return !fAbort;
}


void EncodeUrl(LPCTSTR pszTargetUrl, char *pBuf)
{
    while (*pszTargetUrl)
    {
        switch (*pszTargetUrl)
        {
        case ':':
            *pBuf++ = '%';
            *pBuf++ = '3';
            *pBuf++ = 'A';
            break;
        case '/':
            *pBuf++ = '%';
            *pBuf++ = '2';
            *pBuf++ = 'F';
            break;      
        default:
            *pBuf++ = *pszTargetUrl;
            break;
        }
        ++pszTargetUrl; 
    }
    *pBuf = 0;
}


STDMETHODIMP CRORemoteSite::QueryInterface(
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IObtainRating)) {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CRORemoteSite::AddRef(void)
{
    RefThisDLL(TRUE);

    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CRORemoteSite::Release(void)
{
    RefThisDLL(FALSE);

    if (!--m_cRef) {
        delete this;
        return 0;
    }
    else
        return m_cRef;
}


LPSTR FindRatingLabel(LPSTR pszResponse)
{
    /* pszResponse is the complete response message from the HTTP server.
     * It could be a simple response (just the PICS label we want) or it
     * could be a full response including headers.  In the former case we
     * just return the label, in the latter we have to skip the headers
     * to the message body.
     *
     * To be extra tolerant of poorly written label bureaus, we start by
     * looking at the start of the data to see if it's a left paren.  If
     * it isn't, we assume we've got some headers, so we skip to the
     * double CRLF which HTTP requires to terminate headers.  We don't
     * require a Status-Line (such as "HTTP/1.1 200 OK") even though
     * technically HTTP does.  If we don't find the double CRLF, then
     * we look for the string "(PICS-" which is usually what begins a
     * PICS label list.  If they've done everything else wrong and they're
     * also perverse enough to insert whitespace there (such as "( PICS-"),
     * tough.
     */

    SkipWhitespace(&pszResponse);       /* skip leading whitespace just in case */
    if (*pszResponse != '(') {          /* doesn't seem to start with a label */
        LPSTR pszBody = ::strstrf(pszResponse, ::szDoubleCRLF);
        if (pszBody != NULL) {          /* found double CRLF, end of HTTP headers */
            pszResponse = pszBody + 4;  /* length of CRLFCRLF */
        }
        else {                          /* no double CRLF, hunt for PICS label */
            pszBody = ::strstrf(pszResponse, ::szPicsOpening);
            if (pszBody != NULL) {
                pszResponse = pszBody;  /* beginning of PICS label */
            }
        }
    }

    return pszResponse;
}


const char szRequestTemplate[] = "?opt=normal&u=\"";
const UINT cchRequestTemplate = sizeof(szRequestTemplate) + 1;

STDMETHODIMP CRORemoteSite::ObtainRating(THIS_ LPCTSTR pszTargetUrl, HANDLE hAbortEvent,
                             IMalloc *pAllocator, LPSTR *ppRatingOut)
{
    HINTERNET hIC, hH;
    HANDLE  rgEvents[2];
    BOOL fRet;
    HRESULT hrRet = E_RATING_NOT_FOUND;
    char rgBuf[10000], *pBuf;   // BUGBUG - way too much stack!
    DWORD  nRead, nBuf = sizeof(rgBuf) - 1;
    LPSTR pszRatingServer;

    if (!gPRSI->etstrRatingBureau.fIsInit())
        return hrRet;

    if (!LoadWinINet()) {
        return hrRet;
    }

    pszRatingServer = gPRSI->etstrRatingBureau.Get();

    BUFFER bufBureauHostName(INTERNET_MAX_HOST_NAME_LENGTH);
    BUFFER bufBureauPath(INTERNET_MAX_PATH_LENGTH);

    if (!bufBureauHostName.QueryPtr() || !bufBureauPath.QueryPtr())
        return E_OUTOFMEMORY;

    URL_COMPONENTS uc;

    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 0;
    uc.lpszHostName = (LPSTR)bufBureauHostName.QueryPtr();
    uc.dwHostNameLength = bufBureauHostName.QuerySize();
    uc.lpszUserName = NULL;
    uc.dwUserNameLength = 0;
    uc.lpszPassword = NULL;
    uc.dwPasswordLength = 0;
    uc.lpszUrlPath = (LPSTR)bufBureauPath.QueryPtr();
    uc.dwUrlPathLength = bufBureauPath.QuerySize();
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 0;

    if (!InternetCrackUrl(pszRatingServer, 0, 0, &uc))
        return HRESULT_FROM_WIN32(GetLastError());

    BUFFER bufRequest(INTERNET_MAX_URL_LENGTH + uc.dwUrlPathLength + cchRequestTemplate);

    LPSTR pszRequest = (LPSTR)bufRequest.QueryPtr();
    if (pszRequest == NULL)
        return E_OUTOFMEMORY;

    LPSTR pszCurrent = pszRequest;
    ::strcpyf(pszCurrent, uc.lpszUrlPath);
    pszCurrent += uc.dwUrlPathLength;

    ::strcpyf(pszCurrent, szRequestTemplate);
    pszCurrent += ::strlenf(pszCurrent);

    /* Encode the target URL. */
    EncodeUrl(pszTargetUrl, pszCurrent);

    ::strcatf(pszCurrent, "\"");

    hIC = hH = NULL;
    
    rgEvents[ABORT_EVENT] = hAbortEvent;
    rgEvents[ASYNC_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!rgEvents[ASYNC_EVENT]) goto STATE_CLEANUP;

    hIC = InternetConnect(hI, uc.lpszHostName, uc.nPort, NULL, NULL,
                          INTERNET_SERVICE_HTTP, 0, (DWORD_PTR) rgEvents[ASYNC_EVENT]);
    if (hIC == NULL || ShouldAbort(hAbortEvent)) goto STATE_CLEANUP;

    hH = HttpOpenRequest(hIC, "GET", pszRequest, NULL, NULL, NULL,
                         INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD,
                         (DWORD_PTR) rgEvents[ASYNC_EVENT]);
    if (hH == NULL || ShouldAbort(hAbortEvent)) goto STATE_CLEANUP;

    fRet = HttpSendRequest(hH, NULL, (DWORD) 0, NULL, 0);
    if (!fRet && !WaitForAsync(rgEvents)) goto STATE_CLEANUP;

    pBuf  = rgBuf;
    nRead = 0;
    do
    {
        fRet = InternetReadFile(hH, pBuf, nBuf-nRead, &nRead);
        if (!fRet && !WaitForAsync(rgEvents)) goto STATE_CLEANUP;
        if (nRead)
        {
            pBuf += nRead;
            hrRet = NOERROR;
        }
    } while (nRead);
        

STATE_CLEANUP:
    if (hH)  InternetCloseHandle(hH);
    if (hIC) InternetCloseHandle(hIC);
    if (rgEvents[ASYNC_EVENT]) CloseHandle(rgEvents[ASYNC_EVENT]);

    if (hrRet == NOERROR)
    {
        (*ppRatingOut) = (char*) pAllocator->Alloc((int)(pBuf - rgBuf + 1));
        if (*ppRatingOut != NULL) {
            *pBuf = '\0';
            LPSTR pszLabel = FindRatingLabel(rgBuf);
            strcpyf(*ppRatingOut, pszLabel);
        }
        else
            hrRet = ResultFromScode(E_OUTOFMEMORY);
    }

    if (hrRet == NOERROR)
        hrRet = S_RATING_FOUND;

    return hrRet;
}



STDMETHODIMP_(ULONG) CRORemoteSite::GetSortOrder(THIS)
{
    return RATING_ORDER_REMOTESITE;
}
