/* Copyright 1996 Microsoft */

#include "priv.h"

#ifdef DEBUG

#define ENTERPROC EnterProc
#define EXITPROC ExitProc

void EnterProc(DWORD dwTraceLevel, LPTSTR szFmt, ...);
void ExitProc(DWORD dwTraceLevel, LPTSTR szFmt, ...);

extern DWORD g_dwIEDDETrace;

#else

#pragma warning(disable:4002)

#ifndef UNIX

#ifndef CCOVER
#define ENTERPROC()
#define EXITPROC()
#else //CCOVER

// these are needed because of a bug in cl.exe which causes 
// compilation problems with #pragma when a program is preprocessed
// and compiled separately

#define ENTERPROC 1 ? (void) 0 : (void)
#define EXITPROC 1 ? (void) 0 : (void)
#endif // CCOVER

#else
#define ENTERPROC EnterProc
#define EXITPROC ExitProc
inline void EnterProc(DWORD dwTraceLevel, LPTSTR szFmt, ...){}
inline void ExitProc(DWORD dwTraceLevel, LPTSTR szFmt, ...){}
#endif

#endif

//
// Forward reference.
//
class CIEDDEThread;

//
// Stored in _hdsaWinitem
//
typedef struct _tagWinItem
{
    DWORD           dwWindowID;     // Synthetic window ID exposed in IEDDE interfaces
    HWND            hwnd;           // Actual hwnd of browser window
    DWORD           dwThreadID;     // ThreadID for this browser window
    CIEDDEThread    *pidt;          // Thread specific data and methods
} WINITEM;

//
// Stored in _hdsaProtocolHandler
//
typedef struct _tagProtocolReg
{
    LPTSTR  pszProtocol;
    LPTSTR  pszServer;
} PROTOCOLREG;

#define TEN_SECONDS         (10 * 1000)
#define DXA_GROWTH_AMOUNT   (10)

#ifndef UNIX
#define IEXPLORE_STR "IEXPLORE"
#else
#define IEXPLORE_STR "iexplorer"
#endif

static const TCHAR c_szIExplore[] = TEXT(IEXPLORE_STR);
static const TCHAR c_szReturn[] = TEXT("Return");
static const TCHAR c_szWWWOpenURL[] = TEXT("WWW_OpenURL");
static const TCHAR c_szWWWUrlEcho[] = TEXT("WWW_URLEcho");

typedef struct _tagDDETHREADINFO
{
    DWORD       dwDDEInst;
    HSZ         hszService;
    HSZ         hszReturn;
    HDDEDATA    hddNameService;
} DDETHREADINFO;

class CIEDDEThread {
public:
    CIEDDEThread() { };
    ~CIEDDEThread() { };

    void GetDdeThreadInfo(DDETHREADINFO *pdti) { *pdti = _dti; }
    void SetDdeThreadInfo(DDETHREADINFO *pdti) { _dti = *pdti; }
    HDDEDATA OnRequestPoke(HSZ hszTopic, HSZ hszParams);
    HDDEDATA OnExecute(HSZ hszTopic, HDDEDATA hddParams);

protected:
    DDETHREADINFO   _dti;

    HDDEDATA CallTopic(DWORD dwType, LPCTSTR pszTopic, LPTSTR pszParams);
    HDDEDATA DoNavigate(LPTSTR pszLocation, HWND hwnd);
    BOOL MakeQuotedString(LPCTSTR pszInput, LPTSTR pszOutput, int cchOutput);
    HDDEDATA CreateReturnObject(LPVOID p, DWORD cb);
    HDDEDATA CreateReturnStringObject(LPTSTR pszReturnString, DWORD cch);


    BOOL ParseString(LPTSTR *ppsz, LPTSTR *ppszString);
    BOOL ParseQString(LPTSTR *ppsz, LPTSTR *ppszString);
    BOOL ParseNumber(LPTSTR *ppsz, DWORD *pdw);
    BOOL ParseWinitem(LPTSTR *ppsz, WINITEM *pwi);

    HDDEDATA WWW_GetWindowInfo(LPTSTR pszParams);
    HDDEDATA WWW_OpenURL(LPTSTR pszParams);
    HDDEDATA WWW_ShowFile(LPTSTR pszParams);
    HDDEDATA WWW_Activate(LPTSTR pszParams);
    HDDEDATA WWW_Exit(LPTSTR pszParams);
    HDDEDATA WWW_RegisterURLEcho(LPTSTR pszParams);
    HDDEDATA WWW_UnregisterURLEcho(LPTSTR pszParams);
    HDDEDATA WWW_RegisterProtocol(LPTSTR pszParams);
    HDDEDATA WWW_UnregisterProtocol(LPTSTR pszParams);
    HDDEDATA WWW_ListWindows(LPTSTR pszParams);
};

class CIEDDE {
public:
    CIEDDE() { };
    ~CIEDDE() { };

    BOOL IsAutomationReady(void) { return _fAutomationReady; }
    BOOL GetWinitemFromWindowID(DWORD dwWindowID, WINITEM *pwi);
    BOOL GetWinitemFromHwnd(HWND hwnd, WINITEM *pwi);
    BOOL AddUrlEcho(LPCTSTR pszUrlEcho);
    BOOL RemoveUrlEcho(LPCTSTR pszUrlEcho);
    BOOL AddProtocolHandler(LPCTSTR pszServer, LPCTSTR pszProtocol);
    BOOL RemoveProtocolHandler(LPCTSTR pszServer, LPCTSTR pszProtocol);
    HDSA GetHdsaWinitem(void) { return _hdsaWinitem; }
    static HDDEDATA DdeCallback(UINT dwType, UINT dwFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdd, DWORD dwData1, DWORD dwData2);
    void EnterCrit(void) { ASSERT(_fCSInitialized); EnterCriticalSection(&_csIEDDE); }
    void LeaveCrit(void) { ASSERT(_fCSInitialized); LeaveCriticalSection(&_csIEDDE); }

protected:
    BOOL _fAutomationReady;
    HDSA _hdsaWinitem;
    HDSA _hdsaProtocolHandler;
    HDPA _hdpaUrlEcho;
    BOOL _fCSInitialized;
    CRITICAL_SECTION _csIEDDE;

    HDDEDATA _SendDDEMessageHsz(DWORD dwDDEInst, HSZ hszApp, HSZ hszTopic, HSZ hszMessage, UINT wType);
    HDDEDATA _SendDDEMessageSz(DWORD dwDDEInst, LPCTSTR pszApp, LPCTSTR pszTopic, LPCTSTR pszMessage, UINT wType);

    static int _DestroyProtocol(LPVOID p1, LPVOID p2);
    static int _DestroyUrlEcho(LPVOID p1, LPVOID p2);
    static int _DestroyWinitem(LPVOID p1, LPVOID p2);

    BOOL _GetWinitemFromThread(DWORD dwThreadID, WINITEM *pwi);
    BOOL _GetDtiFromThread(DWORD dwThreadID, DDETHREADINFO *pdti);

    BOOL _CreateDdeThreadInfo(DDETHREADINFO *pdti);
    void _DestroyDdeThreadInfo(DDETHREADINFO *pdti);
    BOOL _AddWinitem(WINITEM *pwi);
    BOOL _UpdateWinitem(WINITEM *pwi);
    BOOL _DeleteWinitemByHwnd(HWND hwnd, WINITEM *pwi);

    BOOL _Initialize(void);
    void _Uninitialize(void);
    void _AutomationStarted(void);
    HRESULT _BeforeNavigate(LPCTSTR pszURL, BOOL *pfProcessed);
    HRESULT _AfterNavigate(LPCTSTR pszURL, HWND hwnd);
    BOOL _NewWindow(HWND hwnd);
    BOOL _WindowDestroyed(HWND hwnd);

    friend BOOL IEDDE_Initialize(void);
    friend void IEDDE_Uninitialize(void);
    friend void IEDDE_AutomationStarted(void);
    friend HRESULT IEDDE_BeforeNavigate(LPCWSTR pwszURL, BOOL *pfProcessed);
    friend HRESULT IEDDE_AfterNavigate(LPCWSTR pwszURL, HWND hwnd);
    friend BOOL IEDDE_NewWindow(HWND hwnd);
    friend BOOL IEDDE_WindowDestroyed(HWND hwnd);
};
CIEDDE *g_pIEDDE = NULL;

#define ENTER_IEDDE_CRIT g_pIEDDE->EnterCrit()
#define LEAVE_IEDDE_CRIT g_pIEDDE->LeaveCrit()



//
// There is one CIEDDEThread object per browser window.
// Its private data consists of DDE handles, which are
// necessarily valid only in the thread that created them.
//
// Its methods consist of three broad categories:
//      the parser
//      the dispatcher
//      one handler for each DDE topic
//







//
// CreateReturnObject - creates a dde data item.
//
#define CREATE_HDD(x) CreateReturnObject(&x, SIZEOF(x))
HDDEDATA CIEDDEThread::CreateReturnObject(LPVOID p, DWORD cb)
{
    HDDEDATA hddRet;

    ENTERPROC(2, TEXT("CreateReturnObject(p=%08X,cb=%d)"), p, cb);

    hddRet = DdeCreateDataHandle(_dti.dwDDEInst, (BYTE *)p, cb, 0, _dti.hszReturn, CF_TEXT, 0);

    if (hddRet == 0)
    {
        TraceMsg(TF_WARNING, "IEDDE: Could not create return object");
    }

    EXITPROC(2, TEXT("CreateReturnObject=%08X"), hddRet);
    return hddRet;
}

HDDEDATA CIEDDEThread::CreateReturnStringObject(LPTSTR pszReturnString, DWORD cch)
{
    HDDEDATA hddRet = 0;

    ENTERPROC(2, TEXT("CreateReturnStringObject(p=%s,cb=%d)"), pszReturnString, cch);

    //
    // REVIEW I thought specifying CF_UNICODETEXT should have worked, but... 
    // it didn't, so always return ANSI string as out string params
    // - julianj
    //
    LPSTR pszAnsiBuf = (LPSTR)LocalAlloc(LPTR, cch+1);
    if (pszAnsiBuf)
    {
        SHUnicodeToAnsi(pszReturnString, pszAnsiBuf, cch+1);
        hddRet = DdeCreateDataHandle(_dti.dwDDEInst, (BYTE *)pszAnsiBuf, (cch+1), 0, _dti.hszReturn, CF_TEXT, 0);
        LocalFree(pszAnsiBuf);
    }
    
    if (hddRet == 0)
    {
        TraceMsg(TF_WARNING, "IEDDE: Could not create return object");
    }

    EXITPROC(2, TEXT("CreateReturnObject=%08X"), hddRet);
    return hddRet;
}


//
// OnRequestPoke - handle XTYP_REQUEST and XTYP_POKE
//
HDDEDATA CIEDDEThread::OnRequestPoke(HSZ hszTopic, HSZ hszParams)
{
    HDDEDATA hddRet = 0;
    ENTERPROC(2, TEXT("OnRequestPoke(hszTopic=%08X,hszParams=%08X)"), hszTopic, hszParams);

    TCHAR szTopic[100];
    TCHAR szParams[1000];

    if (DdeQueryString(_dti.dwDDEInst, hszTopic, szTopic, ARRAYSIZE(szTopic), CP_WINNEUTRAL) != 0)
    {
        if (DdeQueryString(_dti.dwDDEInst, hszParams, szParams, ARRAYSIZE(szParams), CP_WINNEUTRAL))
        {
            hddRet = CallTopic(XTYP_REQUEST, szTopic, szParams);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: OnRequestPoke could not query the parameters");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: OnRequestPoke could not query the topic");
    }

    EXITPROC(2, TEXT("OnRequestPoke=%08X"), hddRet);
    return hddRet;
}

//
// OnExecute - handle XTYP_EXECUTE
//
HDDEDATA CIEDDEThread::OnExecute(HSZ hszTopic, HDDEDATA hddParams)
{
    HDDEDATA hddRet = 0;
    ENTERPROC(2, TEXT("OnExecute(hszTopic=%08X,hddParams=%08X)"), hszTopic, hddParams);

    TCHAR szTopic[100];

    if (DdeQueryString(_dti.dwDDEInst, hszTopic, szTopic, ARRAYSIZE(szTopic), CP_WINNEUTRAL) != 0)
    {
        //
        // Why "cbParams + 3"?
        // UNICODE - if we cut the last unicode character in half, we need
        //           one 0 to finish the character, and two more 0 for the
        //           terminating NULL
        // ANSI - if we cut the last DBCS character in half, we need one 0
        //        to finish the character, and one 0 for the terminating NULL
        //
        //
        DWORD cbParams = DdeGetData(hddParams, NULL, 0, 0) + 3;
        LPTSTR pszParams = (LPTSTR) LocalAlloc(LPTR, cbParams);

        if(pszParams)
        {
            DdeGetData(hddParams, (BYTE *)pszParams, cbParams, 0);
            //
            // DdeGetData can't be wrapped in shlwapi since it can return non
            // string data.  Here we only expect strings so the result can be
            // safely converted.
            //
            if (g_fRunningOnNT)
            {
                hddRet = CallTopic(XTYP_EXECUTE, szTopic, pszParams);
            }
            else
            {
                WCHAR szParams[MAX_URL_STRING];
                SHAnsiToUnicode((LPCSTR)pszParams, szParams, ARRAYSIZE(szParams));
                hddRet = CallTopic(XTYP_EXECUTE, szTopic, szParams);
            }
            LocalFree(pszParams);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: OnExecute could not query the topic");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: OnExecute could not query the topic");
    }

    EXITPROC(2, TEXT("OnExecute=%08X"), hddRet);
    return hddRet;
}

//
// CallTopic - Looks up the command in the DDETOPICHANDLER table and calls the
// corresponding function.
//
HDDEDATA CIEDDEThread::CallTopic(DWORD dwType, LPCTSTR pszTopic, LPTSTR pszParams)
{
    HDDEDATA hddRet = DDE_FNOTPROCESSED;
    ENTERPROC(2, TEXT("CallTopic(wType=%d,pszTopic=>%s<,pszParams=>%s<)"), dwType, pszTopic, pszParams);

#define DISPATCH_BEGIN
#define DISPATCH(topic)                                 \
    if (StrCmpI(TEXT("WWW_") TEXT(#topic), pszTopic) == 0)   \
    {                                                   \
        if (fCanRun)                                    \
        {                                               \
            hddRet = WWW_ ## topic(pszParams);          \
        }                                               \
        else                                            \
        {                                               \
            fAbortedRun = TRUE;                         \
        }                                               \
    }                                                   \
    else
#define DISPATCH_END { TraceMsg(TF_WARNING, "IEDDE: CallTopic given unknown topic"); }

    BOOL fAbortedRun = FALSE;
    BOOL fCanRun = ((dwType != XTYP_EXECUTE) || g_pIEDDE->IsAutomationReady());

    DISPATCH_BEGIN
        DISPATCH(GetWindowInfo)
        DISPATCH(OpenURL)
        DISPATCH(ShowFile)
        DISPATCH(Activate)
        DISPATCH(Exit)
        DISPATCH(RegisterURLEcho)
        DISPATCH(UnregisterURLEcho)
        DISPATCH(RegisterProtocol)
        DISPATCH(UnregisterProtocol)
        DISPATCH(ListWindows)
    DISPATCH_END

    if (fAbortedRun)
    {
        hddRet = (HDDEDATA)DDE_FACK;
        TraceMsg(TF_WARNING, "IEDDE: CallTopic received XTYP_EXECUTE before Automation was ready - not processing");
    }

    EXITPROC(2, TEXT("CallTopic=%08X"), hddRet);
    return hddRet;
}

//
// ParseString - parse one string
//
BOOL CIEDDEThread::ParseString(LPTSTR *ppsz, LPTSTR *ppszString)
{
    BOOL fRet = FALSE;

    ENTERPROC(3, TEXT("ParseString(ppsz=%08X,ppszString=%08X)"), ppsz, ppszString);

    LPTSTR pchCurrent, pchNext;
    BOOL fInQuote = FALSE;

    pchCurrent = pchNext = *ppsz;
    while (*pchNext)
    {
        switch (*pchNext)
        {
        case TEXT(' '):
        case TEXT('\t'):
            if (fInQuote)
            {
                //
                // Skip over whitespace when not inside quotes.
                //
                *pchCurrent++ = *pchNext;
            }
            pchNext++;
            break;

        case TEXT('"'):
            //
            // Always copy quote marks.
            //
            fInQuote = !fInQuote;
            *pchCurrent++ = *pchNext++;
            break;

        case TEXT(','):
            if (!fInQuote)
            {
                goto done_parsing;
            }
            *pchCurrent++ = *pchNext++;
            break;

        case TEXT('\\'):
            if (fInQuote &&
                (*(pchNext+1) == TEXT('"')))
            {
                //
                // When in quotes, a \" becomes a ".
                //
                pchNext++;
            }
            *pchCurrent++ = *pchNext++;
            break;

        default:
            *pchCurrent++ = *pchNext++;
            break;
        }
    }
done_parsing:

    //
    // Advance past the comma separator.
    //
    if (*pchNext == TEXT(','))
    {
        pchNext++;
    }

    //
    // NULL terminate the return string.
    //
    *pchCurrent = TEXT('\0');

    //
    // Set the return values.
    //
    *ppszString = *ppsz;
    *ppsz = pchNext;
    fRet = TRUE;

    EXITPROC(3, TEXT("ParseString=%d"), fRet);
    return fRet;
}

//
// ParseQString - parse one quoted string
//
BOOL CIEDDEThread::ParseQString(LPTSTR *ppsz, LPTSTR *ppszString)
{
    BOOL fRet = FALSE;

    ENTERPROC(3, TEXT("ParseQString(ppsz=%08X,ppszString=%08X)"), ppsz, ppszString);

    if (ParseString(ppsz, ppszString))
    {
        LPTSTR pszString = *ppszString;
        int cch = lstrlen(pszString);

        //
        // Strip off optional outer quotes.
        //
        if ((cch >= 2) &&
            (pszString[0] == TEXT('"')) &&
            (pszString[cch-1] == TEXT('"')))
        {
            pszString[0] = pszString[cch-1] = TEXT('\0');
            *ppszString = pszString + 1;
        }

        fRet = TRUE;
    }

    EXITPROC(3, TEXT("ParseQString=%d"), fRet);
    return fRet;
}

//
// ParseNumber - parse one numeric value
//
BOOL CIEDDEThread::ParseNumber(LPTSTR *ppsz, DWORD *pdw)
{
    BOOL fRet = FALSE;
    LPTSTR pszNumber;

    ENTERPROC(3, TEXT("GetNumber(ppsz=%08X,pdw=%08X)"), ppsz, pdw);

    if (ParseString(ppsz, &pszNumber) && pszNumber[0])
    {
        StrToIntEx(pszNumber, STIF_SUPPORT_HEX, (int *)pdw);
        fRet = TRUE;
    }

    EXITPROC(3, TEXT("GetNumber=%d"), fRet);
    return fRet;
}

//
// ParseWinitem - parse one window ID, and return the winitem
//
BOOL CIEDDEThread::ParseWinitem(LPTSTR *ppsz, WINITEM *pwi)
{
    BOOL fRet = FALSE;
    DWORD dwWindowID;

    ENTERPROC(3, TEXT("ParseWinitem(ppsz=%08X,pwi=%08X)"), ppsz, pwi);

    if (ParseNumber(ppsz, &dwWindowID))
    {
        switch (dwWindowID)
        {
        case 0:
        case -1:
            ZeroMemory(pwi, SIZEOF(*pwi));
            pwi->dwWindowID = dwWindowID;
            pwi->hwnd = (HWND)LongToHandle(dwWindowID);
            fRet = TRUE;
            break;

        default:
            fRet = g_pIEDDE->GetWinitemFromWindowID(dwWindowID, pwi);
            break;
        }
    }

    EXITPROC(3, TEXT("ParseWinitem=%d"), fRet);
    return fRet;
}

//
//  WWW_GetWindowInfo - get information about a browser window
//
//  Parameters:
//      dwWindowID - Window ID to examine (-1 = last active window)
//
//  Returns:
//      qcsURL,qcsTitle
//
HDDEDATA CIEDDEThread::WWW_GetWindowInfo(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    WINITEM wi;

    ENTERPROC(1, TEXT("WWW_GetWindowInfo(pszParams=>%s<)"), pszParams);

    if (ParseWinitem(&pszParams, &wi) &&
        (wi.hwnd != 0))
    {
        BSTR bstrURL;

        if (SUCCEEDED(CDDEAuto_get_LocationURL(&bstrURL, wi.hwnd)) && (bstrURL != (BSTR)-1))
        {
            BSTR bstrTitle;

            if (SUCCEEDED(CDDEAuto_get_LocationTitle(&bstrTitle, wi.hwnd)) && (bstrTitle != (BSTR)-1))
            {
                LPTSTR pszURL, pszTitle;


                pszURL = bstrURL;
                pszTitle = bstrTitle;

                if (pszURL && pszTitle)
                {
                    TCHAR szURLQ[MAX_URL_STRING];
                    TCHAR szTitleQ[MAX_URL_STRING];

                    if (MakeQuotedString(pszURL, szURLQ, ARRAYSIZE(szURLQ)) &&
                        MakeQuotedString(pszTitle, szTitleQ, ARRAYSIZE(szTitleQ)))
                    {
                        DWORD cchBuffer = lstrlen(szURLQ) + 1 + lstrlen(szTitleQ) + 1;
                        LPTSTR pszBuffer = (LPTSTR)LocalAlloc(LPTR, cchBuffer * SIZEOF(TCHAR));

                        if (pszBuffer)
                        {
                            wnsprintf(pszBuffer, cchBuffer, TEXT("%s,%s"), szURLQ, szTitleQ);
                            hddRet = CreateReturnStringObject(pszBuffer, lstrlen(pszBuffer));
                            LocalFree(pszBuffer);
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "IEDDE: GetWindowInfo could not alloc buffer");
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: GetWindowInfo could not quote return strings");
                    }
                }

                SysFreeString(bstrTitle);
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: GetWindowInfo could not get title");
            }

            SysFreeString(bstrURL);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: GetWindowInfo could not get URL");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: GetWindowInfo could not parse parameters");
    }

    EXITPROC(1, TEXT("WWW_GetWindowInfo=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_OpenURL - navigate to a URL
//
//  Parameters:
//      qcsURL - url to navigate to
//      qcsSaveFile - [optional] file to save contents in
//      dwWindowID - Window ID to perform navigation
//      dwFlags - flags for navigation
//      qcsPostFormData - [optional] form data to post to URL
//      qcsPostMIMEType - [optional] mime type for form data
//      csProgressServer - [optional] DDE server to get progress updates
//
//  Returns:
//      dwWindowID - window which is doing the work
//
HDDEDATA CIEDDEThread::WWW_OpenURL(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    LPTSTR pszUrl, pszFile;
    WINITEM wi;

    ENTERPROC(1, TEXT("WWW_OpenURL(pszParams=>%s<)"), pszParams);

    if (*pszParams == TEXT('\0'))
    {
        // An empty string is a NOOP.  Needed for NT #291766
    }
    else if (ParseQString(&pszParams, &pszUrl) &&
             ParseQString(&pszParams, &pszFile))
    {
        //
        // APPCOMPAT - a missing hwnd parameter implies -1.
        //
        if (!ParseWinitem(&pszParams, &wi))
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required hwnd parameter to WWW_OpenURL, assuming -1");
            wi.hwnd = (HWND)-1;
        }

#ifdef DEBUG
        DWORD dwFlags;
        if (!ParseNumber(&pszParams, &dwFlags))
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required dwFlags parameter to WWW_OpenURL");
        }
#endif

        hddRet = DoNavigate(pszUrl, wi.hwnd);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: OpenURL could not parse parameters");
    }

    EXITPROC(1, TEXT("WWW_OpenURL=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_ShowFile - navigate to a file
//
//  Parameters:
//      qcsFilename - file to load
//      qcsPostMIMEType - [optional] mime type for form data
//      dwWindowID - Window ID to perform navigation
//      qcsURL - URL of the same document
//
//  Returns:
//      dwWindowID - window which is doing the work
//
HDDEDATA CIEDDEThread::WWW_ShowFile(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    LPTSTR pszFilename, pszMIMEType;
    WINITEM wi;

    ENTERPROC(1, TEXT("WWW_ShowFile(pszParams=>%s<)"), pszParams);

    if (ParseQString(&pszParams, &pszFilename) && pszFilename[0])
    {
        if (!ParseQString(&pszParams, &pszMIMEType) || !pszMIMEType[0])
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required MIMEType parameter to WWW_ShowFile");
        }
        if (!ParseWinitem(&pszParams, &wi))
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required dwWindowID parameter to WWW_ShowFile, assuming -1");
            wi.hwnd = (HWND)-1;
        }

#ifdef DEBUG
        LPTSTR pszURL;

        if (!ParseQString(&pszParams, &pszURL) || !pszURL[0])
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required szURL parameter to WWW_ShowFile");
        }
#endif
        hddRet = DoNavigate(pszFilename, wi.hwnd);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: ShowFile could not parse parameters");
    }

    EXITPROC(1, TEXT("WWW_ShowFile=%08X"), hddRet);
    return hddRet;
}

//
// DoNavigate - navigate to a location
//
HDDEDATA CIEDDEThread::DoNavigate(LPTSTR pszLocation, HWND hwnd)
{
    HDDEDATA hddRet = 0;
    HRESULT hr = S_OK;
    TCHAR szParsedPath[MAX_URL_STRING+1];
    DWORD cchParsedPath = ARRAYSIZE(szParsedPath);

    ENTERPROC(2, TEXT("DoNavigate(pszLocation=>%s<,hwnd=%08X)"), pszLocation, hwnd);

    //
    // Convert URL from outside format to internal format.
    //
    if (ParseURLFromOutsideSource(pszLocation, szParsedPath, &cchParsedPath, NULL))
    {
        pszLocation = szParsedPath;
    }

    //
    // In the case of a file:// URL, convert the location to a path.
    //
    cchParsedPath = ARRAYSIZE(szParsedPath);
    if (IsFileUrlW(pszLocation) && SUCCEEDED(PathCreateFromUrl(pszLocation, szParsedPath, &cchParsedPath, 0)))
    {
        pszLocation = szParsedPath;
    }

    LPWSTR pwszPath;

    pwszPath = pszLocation;

    if (SUCCEEDED(hr))
    {
        hr = CDDEAuto_Navigate(pwszPath, &hwnd, 0);
    }

    DWORD dwServicingWindow = SUCCEEDED(hr) ? -2 : -3;

    hddRet = CREATE_HDD(dwServicingWindow);

    EXITPROC(2, TEXT("DoNavigate=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_Activate - activate a browser window
//
//  Parameters:
//      dwWindowID - Window ID to activate
//      dwFlags - should always zero
//
//  Returns:
//      dwWindowID - window ID that got activated
//
HDDEDATA CIEDDEThread::WWW_Activate(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    WINITEM wi;

    ENTERPROC(1, TEXT("WWW_Activate(pszParams=>%s<)"), pszParams);

    if (ParseWinitem(&pszParams, &wi) &&
        wi.dwWindowID != 0)
    {
#ifdef DEBUG
        DWORD dwFlags;
        if (ParseNumber(&pszParams, &dwFlags))
        {
            //
            // Netscape spec says this should always be zero.
            //
            ASSERT(dwFlags == 0);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: Some bozo isn't giving the required dwFlags parameter to WWW_Activate");
        }
#endif

        //
        // dwWindowID of -1 means use the active window.
        //
        if (wi.dwWindowID == -1)
        {
            HWND hwnd;

            CDDEAuto_get_HWND((long *)&hwnd);

            if (hwnd)
            {
                if (g_pIEDDE->GetWinitemFromHwnd(hwnd, &wi) == FALSE)
                {
                    wi.dwWindowID = (DWORD)-1;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: Activate could not find an active window");
            }
        }

        //
        // Activate the window.
        //
        if (wi.dwWindowID != -1)
        {
            if ((GetForegroundWindow() == wi.hwnd) || (SetForegroundWindow(wi.hwnd)))
            {
                if (IsIconic(wi.hwnd))
                {
                    ShowWindow(wi.hwnd, SW_RESTORE);
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: Activate could not set foreground window");
            }
            
            hddRet = CREATE_HDD(wi.dwWindowID);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: Activate could not find a browser window to activate");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: Activate could not parse parameters");
    }

    EXITPROC(1, TEXT("WWW_Activate=%08X"), hddRet);
    return hddRet;
}


//
//  WWW_Exit - close all browser windows
//
//  Parameters:
//      none
//
//  Returns:
//      none
//
HDDEDATA CIEDDEThread::WWW_Exit(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;

    ENTERPROC(1, TEXT("WWW_Exit(pszParams=>%s<)"), pszParams);

    CDDEAuto_Exit();

    EXITPROC(1, TEXT("WWW_Exit=%08X"), hddRet);
    return hddRet;
}


//
//  WWW_RegisterURLEcho - register a server for URL change notifications
//
//  Parameters:
//      qcsServer - the DDE server to get notifications
//
//  Returns:
//      fSuccess
//
HDDEDATA CIEDDEThread::WWW_RegisterURLEcho(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    BOOL fSuccess = FALSE;
    LPTSTR pszServer;

    ENTERPROC(1, TEXT("WWW_RegisterURLEcho(pszParams=>%s<)"), pszParams);

    if (ParseQString(&pszParams, &pszServer) && pszServer[0])
    {
        LPTSTR pszServerCopy = StrDup(pszServer);

        if (pszServerCopy)
        {
            if (g_pIEDDE->AddUrlEcho(pszServerCopy))
            {
                fSuccess = TRUE;
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: RegisterURLEcho could not add an URLEcho");
            }

            if (!fSuccess)
            {
                LocalFree(pszServerCopy);
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: RegisterURLEcho could not dup a string");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: RegisterURLEcho could not parse parameters");
    }

    hddRet = CREATE_HDD(fSuccess);

    EXITPROC(1, TEXT("WWW_RegisterURLEcho=%08X"), hddRet);
    return hddRet;
}


//
//  WWW_UnregisterURLEcho - unregister a DDE server
//
//  Parameters:
//      qcsServer - the DDE server to stop getting notifications
//
//  Returns:
//      fSuccess
//
HDDEDATA CIEDDEThread::WWW_UnregisterURLEcho(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    BOOL fSuccess = FALSE;
    LPTSTR pszServer;

    ENTERPROC(1, TEXT("WWW_UnregisterURLEcho(pszParams=>%s<)"), pszParams);

    if (ParseQString(&pszParams, &pszServer) && pszServer[0])
    {
        if (g_pIEDDE->RemoveUrlEcho(pszServer))
        {
            fSuccess = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: UnregisterURLEcho could not find the server");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: UnregisterURLEcho could not parse parameters");
    }

    hddRet = CREATE_HDD(fSuccess);

    EXITPROC(1, TEXT("WWW_UnregisterURLEcho=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_RegisterProtocol - register a server for handling a protocol
//
//  Parameters:
//      qcsServer - the DDE server to handle URLs
//      qcsProtocol - the protocol to handle
//
//  Returns:
//      fSuccess - this is the first server to register the protocol
//
HDDEDATA CIEDDEThread::WWW_RegisterProtocol(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    BOOL fSuccess = FALSE;
    LPTSTR pszServer, pszProtocol;

    ENTERPROC(1, TEXT("WWW_RegisterProtocol(pszParams=>%s<)"), pszParams);

    if (ParseQString(&pszParams, &pszServer) && pszServer[0] &&
        ParseQString(&pszParams, &pszProtocol) && pszProtocol[0])
    {
        if (g_pIEDDE->AddProtocolHandler(pszServer, pszProtocol))
        {
            fSuccess = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: RegisterProtocol unable to register");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: RegisterProtocol could not parse parameters");
    }

    hddRet = CREATE_HDD(fSuccess);

    EXITPROC(1, TEXT("WWW_RegisterProtocol=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_UnregisterProtocol - unregister a server handling a protocol
//
//  Parameters:
//      qcsServer - the DDE server which is handling URLs
//      qcsProtocol - the protocol getting handled
//
//  Returns:
//      fSuccess - this server was registered, but now isn't
//
HDDEDATA CIEDDEThread::WWW_UnregisterProtocol(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    BOOL fSuccess = FALSE;
    LPTSTR pszServer, pszProtocol;

    ENTERPROC(1, TEXT("WWW_UnregisterProtocol(pszParams=>%s<)"), pszParams);

    if (ParseQString(&pszParams, &pszServer) && pszServer[0] &&
        ParseQString(&pszParams, &pszProtocol) && pszProtocol[0])
    {
        if (g_pIEDDE->RemoveProtocolHandler(pszServer, pszProtocol))
        {
            fSuccess = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: UnregisterProtocol unable to unregister");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: UnregisterProtocol could not parse parameters");
    }

    hddRet = CREATE_HDD(fSuccess);

    EXITPROC(1, TEXT("WWW_UnregisterProtocol=%08X"), hddRet);
    return hddRet;
}

//
//  WWW_ListWindows - Get a list of DDE supported browser window IDs
//
//  Parameters:
//      none
//
//  Returns:
//      pdwWindowID (terminated with 0)
//
HDDEDATA CIEDDEThread::WWW_ListWindows(LPTSTR pszParams)
{
    HDDEDATA hddRet = 0;
    ENTERPROC(1, TEXT("WWW_ListWindows(pszParams=>%s<)"), pszParams);

    ENTER_IEDDE_CRIT;

    DWORD cbAlloc, *pdwWindowID;
    int cWindows = 0;
    HDSA hdsaWinitem = g_pIEDDE->GetHdsaWinitem();

    if (hdsaWinitem)
    {
        cWindows = DSA_GetItemCount(hdsaWinitem);
    }

    //
    // Note: we are following the Netscape spec (null terminated pdw) here,
    // whereas IE3 followed the Spyglass spec (pdw[0] = count of windows).
    //

    cbAlloc = (cWindows + 1) * SIZEOF(DWORD);

    pdwWindowID = (DWORD *)LocalAlloc(LPTR, cbAlloc);
    if (pdwWindowID)
    {
        DWORD *pdw;

        pdw = pdwWindowID;

        for (int i=0; i<cWindows; i++)
        {
            WINITEM wi;

            int iResult = DSA_GetItem(hdsaWinitem, i, &wi);

            if (iResult != -1)
            {
                *pdw++ = wi.dwWindowID;
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: ListWindows could not get a DSA item");
            }
        }

        hddRet = CreateReturnObject(pdwWindowID, cbAlloc);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: ListWindows could not allocate a window list");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(1, TEXT("WWW_ListWindows=%08X"), hddRet);
    return hddRet;
}

//
// MakeQuotedString - wrap a string in " marks, escaping internal "s as \"
//
BOOL CIEDDEThread::MakeQuotedString(LPCTSTR pszInput, LPTSTR pszOutput, int cchOutput)
{
    BOOL fRet = FALSE;

    ENTERPROC(2, TEXT("MakeQuotedString(pszInput=>%s<,pszOutput=%08X,cchOutput=%08X)"), pszInput, pszOutput, cchOutput);

    if (cchOutput < 3)
    {
        TraceMsg(TF_WARNING, "IEDDE: MakeQuotedString has no room for minimal quoted string");
    }
    else if ((pszInput == NULL) || (*pszInput == TEXT('\0')))
    {
        StrCpyN(pszOutput, TEXT("\"\""), cchOutput);
        fRet = TRUE;
    }
    else
    {
        //
        // Copy first quote mark.
        //
        *pszOutput++ = TEXT('"');
        cchOutput--;

        //
        // Copy pszInput, escaping quote marks and making
        // sure to leave room for final quote and NULL.
        //
        while ((cchOutput > 2) && (*pszInput))
        {
            if (*pszInput == TEXT('"'))
            {
                *pszOutput++ = TEXT('\\');
                cchOutput--;
            }
            *pszOutput++ = *pszInput++;
            cchOutput--;
        }

        //
        // Copy final quote and NULL if we're done and there is room.
        //
        if ((*pszInput == TEXT('\0')) && (cchOutput >= 2))
        {
            StrCpyN(pszOutput, TEXT("\""), cchOutput);
            fRet = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: MakeQuotedString ran out of room in output buffer");
        }
    }

    EXITPROC(2, TEXT("MakeQuotedString=%d"), fRet);
    return fRet;
}
















#undef CIEDDEThread

//
// There is one global CIEDDE object per process.
// It maintains the global information, such as
// the list of all browsers & what threads they are on,
// and the list of all apps who have registered an URL Echo.
//
// Its methods consist of these categories:
//      the DDE callback function
//      an internal handler for each exposed IEDDE_ function
//      database (hdsa, hdpa) access and manipulation functions
//
// This object creates and destroys CIEDDEThread objects
// (at NewWindow and WindowDestroyed time) and also initializes /
// uninitializes DDE services on a per thread (not per hwnd!) basis.
//



//
// DdeCallback - DDE callback function for IEDDE.
//
#define DDETYPESTR(x) (x == XTYP_REQUEST ? TEXT("Request") : \
                       (x == XTYP_POKE ? TEXT("Poke") : \
                       (x == XTYP_EXECUTE ? TEXT("Execute") : \
                       (x == XTYP_CONNECT ? TEXT("Connect") : TEXT("Unknown")))))
HDDEDATA CIEDDE::DdeCallback(UINT dwType, UINT dwFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdd, DWORD dwData1, DWORD dwData2)
{
    HDDEDATA    hddRet = 0;
    ENTERPROC(2, TEXT("DdeCallback(dwType=%08X(%s),dwFmt=%d,hconv=%d,hsz1=%08X,hsz2=%08X,hdd=%08X,dwData1=%08X,dwData2=%08X)"),
                dwType, DDETYPESTR(dwType), dwFmt, hconv, hsz1, hsz2, hdd, dwData1, dwData2);

    WINITEM wi;

    switch (dwType)
    {
    case XTYP_REQUEST:
    case XTYP_POKE:
        if (g_pIEDDE->_GetWinitemFromThread(GetCurrentThreadId(), &wi))
        {
            hddRet = wi.pidt->OnRequestPoke(hsz1, hsz2);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: DdeCallback unable to get thread info on request / poke");
        }
        break;

    case XTYP_EXECUTE:
        if (g_pIEDDE->_GetWinitemFromThread(GetCurrentThreadId(), &wi))
        {
            hddRet = wi.pidt->OnExecute(hsz1, hdd);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: DdeCallback unable to get thread info on execute");
        }
        break;

    case XTYP_CONNECT:
        if (g_pIEDDE->_GetWinitemFromThread(GetCurrentThreadId(), &wi))
        {
            DDETHREADINFO dti;
            wi.pidt->GetDdeThreadInfo(&dti);
            hddRet = (HDDEDATA)(hsz2 == dti.hszService);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: DdeCallback unable to get thread info on connect");
        }
        break;

    case XTYP_ADVREQ:
    case XTYP_ADVSTOP:
        hddRet = DDE_FNOTPROCESSED;
        break;
    }

    EXITPROC(2, TEXT("DdeCallback=%08X"), hddRet);
    return hddRet;
}

//
// SendDDEMessageHsz - handle based wrapper for doing one DDE client transaction
//
HDDEDATA CIEDDE::_SendDDEMessageHsz(DWORD dwDDEInst, HSZ hszApp, HSZ hszTopic, HSZ hszMessage, UINT wType)
{
    HDDEDATA hddRet = 0;

    ENTERPROC(2, TEXT("_SendDDEMessageHsz(dwDDEInst=%08X,hszApp=%08X,hszTopic=%08X,hszMessage=%08X,wType=%d)"), dwDDEInst, hszApp, hszTopic, hszMessage, wType);

    if (hszApp && hszTopic)
    {
        HCONV hconv;
        
        hconv = DdeConnect(dwDDEInst, hszApp, hszTopic, NULL);
        if (hconv)
        {
            hddRet = DdeClientTransaction(NULL, 0, hconv, hszMessage, CF_TEXT, wType, TEN_SECONDS, NULL);
            DdeDisconnect(hconv);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _SendDDEMessageHsz could not connect to app");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _SendDDEMessageHsz is missing either App or Topic");
    }

    EXITPROC(2, TEXT("_SendDDEMessageHsz=%08X"), hddRet);
    return hddRet;
}

//
// SendDDEMessageSz - string based wrapper for doing one DDE client transaction
//
HDDEDATA CIEDDE::_SendDDEMessageSz(DWORD dwDDEInst, LPCTSTR pszApp, LPCTSTR pszTopic, LPCTSTR pszMessage, UINT wType)
{
    HDDEDATA hddRet = 0;

    ENTERPROC(2, TEXT("_SendDDEMessageSz(dwDDEInst=%08X,pszApp=>%s<,pszTopic=>%s<,pszMessage=>%s<,wType=%d)"), dwDDEInst, pszApp, pszTopic, pszMessage, wType);

    HSZ hszApp = DdeCreateStringHandle(dwDDEInst, pszApp, CP_WINNEUTRAL);
    if (hszApp)
    {
        HSZ hszTopic = DdeCreateStringHandle(dwDDEInst, pszTopic, CP_WINNEUTRAL);
        if (hszTopic)
        {
            HSZ hszMessage = DdeCreateStringHandle(dwDDEInst, pszMessage, CP_WINNEUTRAL);
            if (hszMessage)
            {
                hddRet = _SendDDEMessageHsz(dwDDEInst, hszApp, hszTopic, hszMessage, wType);
                DdeFreeStringHandle(dwDDEInst, hszMessage);
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _SendDDEMessageSz could not convert message");
            }
            DdeFreeStringHandle(dwDDEInst, hszTopic);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _SendDDEMessageSz could not convert topic");
        }
        DdeFreeStringHandle(dwDDEInst, hszApp);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _SendDDEMessageSz could not convert app");
    }

    EXITPROC(2, TEXT("_SendDDEMessageSz=%08X"), hddRet);
    return hddRet;
}

//
// Initialize - called when ready to start IEDDE server
//
BOOL CIEDDE::_Initialize(void)
{
    BOOL fSuccess = TRUE;
    ENTERPROC(2, TEXT("_Initialize()"));

    ASSERT(_fCSInitialized == FALSE);
    InitializeCriticalSection(&_csIEDDE);
    _fCSInitialized = TRUE;

    EXITPROC(2, TEXT("_Initialize=%d"), fSuccess);
    return fSuccess;
}

//
// _DestroyWinitem - DSA callback to partially free the contents of a WINITEM*
//  In practice this should never get called, the hdsaWinItem list should be
//  empty at uninit time.
//
int CIEDDE::_DestroyWinitem(LPVOID p1, LPVOID p2)
{
    WINITEM *pwi = (WINITEM *)p1;
    ASSERT(IS_VALID_READ_PTR(pwi, WINITEM));
    ASSERT(IS_VALID_READ_PTR(pwi->pidt, CIEDDEThread));

    //
    // It would be good to unregister the DDE server at this point,
    // but we'd need to be on its thread to do it.
    //

    delete pwi->pidt;

    return 1;
}

//
// _DestroyProtocol - DSA callback to free the contents of a PROTOCOLREG*
//
int CIEDDE::_DestroyProtocol(LPVOID p1, LPVOID p2)
{
    PROTOCOLREG *pr = (PROTOCOLREG *)p1;
    ASSERT(IS_VALID_READ_PTR(pr, PROTOCOLREG));

    LocalFree(pr->pszProtocol);
    LocalFree(pr->pszServer);

    return 1;
}

//
// _DestroyUrlEcho - DPA callback to free allocated memory
//
int CIEDDE::_DestroyUrlEcho(LPVOID p1, LPVOID p2)
{
    ASSERT(IS_VALID_STRING_PTR((LPTSTR)p1, -1));
    LocalFree(p1);

    return 1;
}

//
// Uninitialize - called when ready to stop IEDDE server
//
void CIEDDE::_Uninitialize(void)
{
    ENTERPROC(2, TEXT("_Uninitialize()"));

    _fAutomationReady = FALSE;

    if (_hdsaWinitem)
    {
        if (DSA_GetItemCount(_hdsaWinitem))
        {
            //ASSERT(DSA_GetItemCount(_hdsaWinitem)==0);
            TraceMsg(TF_ERROR, "IEDDE: Browser windows still open on uninitialize");

            DSA_DestroyCallback(_hdsaWinitem, _DestroyWinitem, 0);
        }
        _hdsaWinitem = NULL;
    }

    if (_hdsaProtocolHandler)
    {
        DSA_DestroyCallback(_hdsaProtocolHandler, _DestroyProtocol, 0);
        _hdsaProtocolHandler = NULL;
    }

    if (_hdpaUrlEcho)
    {
        DPA_DestroyCallback(_hdpaUrlEcho, _DestroyUrlEcho, 0);
        _hdpaUrlEcho = NULL;
    }

    if (_fCSInitialized)
    {
        DeleteCriticalSection(&_csIEDDE);
    }

    EXITPROC(2, TEXT("_Uninitialize!"));
}


//
// _AutomationStarted - called when automation support can be called
//
void CIEDDE::_AutomationStarted(void)
{
    ENTERPROC(1, TEXT("_AutomationStarted()"));

    _fAutomationReady = TRUE;

    EXITPROC(1, TEXT("_AutomationStarted!"));
}

//
// _BeforeNavigate - called before a navigation occurs.
//
HRESULT CIEDDE::_BeforeNavigate(LPCTSTR pszURL, BOOL *pfProcessed)
{
    ENTERPROC(1, TEXT("_BeforeNavigate(pszURL=>%s<,pfProcessed=%08X)"), pszURL, pfProcessed);

    SHSTR shstrMsg;
    HRESULT hr = S_OK;
    int cProtocols = 0;

    ENTER_IEDDE_CRIT;
    if (_hdsaProtocolHandler)
    {
        cProtocols = DSA_GetItemCount(_hdsaProtocolHandler);
    }
    LEAVE_IEDDE_CRIT;

    if (cProtocols)
    {
        DDETHREADINFO dti;

        if (_GetDtiFromThread(GetCurrentThreadId(), &dti))
        {
            PARSEDURL pu;

            pu.cbSize = SIZEOF(pu);

            if (SUCCEEDED(ParseURL(pszURL, &pu)))
            {
                int i;

                for (i=0; i<cProtocols; i++)
                {
                    PROTOCOLREG pr;

                    ENTER_IEDDE_CRIT;
                    int iResult = DSA_GetItem(_hdsaProtocolHandler, i, &pr);
                    LEAVE_IEDDE_CRIT;

                    if (iResult != -1)
                    {
                        //
                        // Check to see if the protocol to navigate
                        // matches one of our registered protocols.
                        // We do a case insensitive compare.  Note
                        // that:
                        //
                        //   (1) ParseURL does not null terminate the
                        //       pu.pszProtocol (its length is stored
                        //       in pu.cchProtocol).
                        //
                        //   (2) pu.pszProtocol is a LPCTSTR so we
                        //       can't modify the pszProtocol ourselves.
                        //
                        //   (3) There is no win32 lstrncmpi() API.
                        //
                        // Therefore in order to do a case insensitive
                        // compare we must copy the pu.pszProtocol into
                        // a writable buffer at some point.
                        //
                        if (lstrlen(pr.pszProtocol) == (int)pu.cchProtocol)
                        {
                            shstrMsg.SetStr(pu.pszProtocol, pu.cchProtocol);
                            if (StrCmpI(pr.pszProtocol, shstrMsg) == 0)
                            {
                                //
                                // BUGBUG - use MakeQuotedString here?
                                //
                                shstrMsg.SetStr(TEXT("\""));
                                shstrMsg.Append(pszURL);
                                shstrMsg.Append(TEXT("\",,-1,0,,,,"));

                                if (_SendDDEMessageSz(dti.dwDDEInst, pr.pszServer, c_szWWWOpenURL, shstrMsg, XTYP_REQUEST))
                                {
                                    if (pfProcessed)
                                    {
                                        *pfProcessed = TRUE;
                                    }
                                }
                                else
                                {
                                    TraceMsg(TF_WARNING, "IEDDE: _BeforeNavigate could not DDE to protocol handler");
                                }

                                break;
                            }
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: _BeforeNavigate could not get item from DSA");
                    }
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _BeforeNavigate could not parse URL");
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _BeforeNavigate unable to get thread info, can't use DDE");
        }
    }

    EXITPROC(1, TEXT("_BeforeNavigate=%08X"), hr);
    return hr;
}

//
// _AfterNavigate - called after a navigation occurs
//
HRESULT CIEDDE::_AfterNavigate(LPCTSTR pszURL, HWND hwnd)
{
    ENTERPROC(1, TEXT("_AfterNavigate(pszURL=>%s<,hwnd=%08X)"), pszURL, hwnd);

    int cURLHooks = 0;
    SHSTR shstrMsg;
    HRESULT hr = S_OK;

    ENTER_IEDDE_CRIT;
    if (_hdpaUrlEcho)
    {
        cURLHooks = DPA_GetPtrCount(_hdpaUrlEcho);
    }
    LEAVE_IEDDE_CRIT;

    if (cURLHooks)
    {
        SHSTR shstrMime;

        // BUGBUG (mattsq 1-97)
        // this is a temporary lie - it should be fixed to use the real mimetype
        // with something like:
        //      GetMimeTypeFromUrl(pszURL, shstrMime);
        // talk to URLMON people
        shstrMime.SetStr(TEXT("text/html"));

        DDETHREADINFO dti={0};
        WINITEM wi;
        DWORD dwWindowID;
        if (GetWinitemFromHwnd(hwnd, &wi))
        {
            dwWindowID = wi.dwWindowID;
            wi.pidt->GetDdeThreadInfo(&dti);
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to find browser window ID, using -1");
            dwWindowID = (DWORD)-1;

            WINITEM wiThread;

            if (_GetWinitemFromThread(GetCurrentThreadId(), &wiThread))
            {
                ASSERT(wiThread.pidt);
                wiThread.pidt->GetDdeThreadInfo(&dti);
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to find DDE thread info");
            }
        }

        if (dti.dwDDEInst)
        {
            HSZ hszTopic = DdeCreateStringHandle(dti.dwDDEInst, c_szWWWUrlEcho, CP_WINNEUTRAL);
            if (hszTopic)
            {
                TCHAR szFinish[16];

                shstrMsg.SetStr(TEXT("\""));                // Quote
                shstrMsg.Append(pszURL);                    // URL
                shstrMsg.Append(TEXT("\",\""));             // Quote Comma Quote
                shstrMsg.Append(shstrMime);                 // Mime
                wnsprintf(szFinish, ARRAYSIZE(szFinish), TEXT("\",%d"), dwWindowID);    //
                shstrMsg.Append(szFinish);                  // Quote Comma dwWindowID NULL

                HSZ hszMsg = DdeCreateStringHandle(dti.dwDDEInst, shstrMsg, CP_WINNEUTRAL);

                if (hszMsg)
                {
                    //
                    // Enumerate in reverse order because calling a hook may destroy it.
                    //
                    for (int i=cURLHooks-1; i>=0; --i)
                    {
                        ENTER_IEDDE_CRIT;
                        LPTSTR pszService = (LPTSTR)DPA_GetPtr(_hdpaUrlEcho, i);
                        LEAVE_IEDDE_CRIT;

                        if (pszService != NULL)
                        {
                            HSZ hszService = DdeCreateStringHandle(dti.dwDDEInst, pszService, CP_WINNEUTRAL);

                            if (hszService)
                            {
                                if (_SendDDEMessageHsz(dti.dwDDEInst, hszService, hszTopic, hszMsg, XTYP_POKE) == 0)
                                {
                                    TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate could not DDE to URLHook handler");
                                }

                                DdeFreeStringHandle(dti.dwDDEInst, hszService);
                            }
                            else
                            {
                                TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to create hszService");
                            }
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to enumerate an URL hook");
                        }
                    }

                    DdeFreeStringHandle(dti.dwDDEInst, hszMsg);
                }
                else
                {
                    TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to create hszMsg");
                }

                DdeFreeStringHandle(dti.dwDDEInst, hszTopic);
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _AfterNavigate unable to create hszTopic");
            }
        }
    }

    EXITPROC(1, TEXT("_AfterNavigate=%08X"), hr);
    return hr;
}

//
// GetWinitemFromHwnd - return the winitem associated with an hwnd
//
BOOL CIEDDE::GetWinitemFromHwnd(HWND hwnd, WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("GetWinitemFromHwnd(hwnd=%08X,pwi=%08X)"), hwnd, pwi);

    ENTER_IEDDE_CRIT;

    if (_hdsaWinitem)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaWinitem); i++)
        {
            WINITEM wi;

            if (DSA_GetItem(_hdsaWinitem, i, &wi) != -1)
            {
                if (wi.hwnd == hwnd)
                {
                    *pwi = wi;
                    fSuccess = TRUE;
                    break;
                }
            }
        }
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("GetWinitemFromHwnd=%d"), fSuccess); 
    return fSuccess;
}

//
// GetWinitemFromWindowID - return the winitem associated with a window ID
//
BOOL CIEDDE::GetWinitemFromWindowID(DWORD dwWindowID, WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(3, TEXT("GetWinitemFromWindowID(dwWindowID=%08X,pwi=%08X)"), dwWindowID, pwi);

    ENTER_IEDDE_CRIT;

    if (_hdsaWinitem)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaWinitem); i++)
        {
            WINITEM wi;

            if (DSA_GetItem(_hdsaWinitem, i, &wi) != -1)
            {
                if (wi.dwWindowID == dwWindowID)
                {
                    *pwi = wi;
                    fSuccess = TRUE;
                    break;
                }
            }
        }
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("GetWinitemFromWindowID=%d"), fSuccess); 
    return fSuccess;
}

//
// _GetWinitemFromThread - return the first winitem associated with a thread
//
BOOL CIEDDE::_GetWinitemFromThread(DWORD dwThreadID, WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_GetWinitemFromThread(dwThreadID=%08X,pwi=%08X)"), dwThreadID, pwi);

    ENTER_IEDDE_CRIT;

    if (_hdsaWinitem)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaWinitem); i++)
        {
            WINITEM wi;

            if (DSA_GetItem(_hdsaWinitem, i, &wi) != -1)
            {
                if (wi.dwThreadID == dwThreadID)
                {
                    *pwi = wi;
                    fSuccess = TRUE;
                    break;
                }
            }
        }
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("_GetWinitemFromThread=%d"), fSuccess); 
    return fSuccess;
}

//
// _GetDtiFromThread - return the threadinfo associated with a thread
//
BOOL CIEDDE::_GetDtiFromThread(DWORD dwThreadID, DDETHREADINFO *pdti)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_GetDtiFromThread(dwThreadID=%08X,pdti=%08X)"), dwThreadID, pdti);

    ENTER_IEDDE_CRIT;

    WINITEM wi;
    if (_GetWinitemFromThread(dwThreadID, &wi))
    {
        wi.pidt->GetDdeThreadInfo(pdti);
        fSuccess = TRUE;
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _GetDtiFromThread unable to find winitem");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("_GetDtiFromThread=%d"), fSuccess); 
    return fSuccess;
}

//
// _CreateDdeThreadInfo - Initialize DDE services and names for this thread
//
BOOL CIEDDE::_CreateDdeThreadInfo(DDETHREADINFO *pdti)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_CreateDdeThreadInfo(pdti=%08X)"), pdti);

    UINT uiDDE;
    DDETHREADINFO dti={0};

    //
    // Initialize DDEML, register our service.
    //

    uiDDE = DdeInitialize(&dti.dwDDEInst, (PFNCALLBACK)DdeCallback,
                           APPCLASS_STANDARD | CBF_FAIL_ADVISES |
                           CBF_SKIP_REGISTRATIONS | CBF_SKIP_UNREGISTRATIONS, 0);

    if (uiDDE == DMLERR_NO_ERROR)
    {
        dti.hszReturn = DdeCreateStringHandle(dti.dwDDEInst, c_szReturn, CP_WINNEUTRAL);
        if (dti.hszReturn)
        {
            dti.hszService = DdeCreateStringHandle(dti.dwDDEInst, c_szIExplore, CP_WINNEUTRAL);
            if (dti.hszService)
            {
                *pdti = dti;
                fSuccess = TRUE;
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _CreateDdeThreadInfo unable to convert service");
            }

            if (!fSuccess)
            {
                DdeFreeStringHandle(dti.dwDDEInst, dti.hszReturn);
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _CreateDdeThreadInfo unable to convert return");
        }

        if (!fSuccess)
        {
            DdeUninitialize(dti.dwDDEInst);
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _CreateDdeThreadInfo unable to init DDE");
    }

    EXITPROC(2, TEXT("_CreateDdeThreadInfo=%d"), fSuccess);
    return fSuccess;
}

//
// _DestroyDdeThreadInfo - Free up any resources in a dti structure.
//
void CIEDDE::_DestroyDdeThreadInfo(DDETHREADINFO *pdti)
{
    ENTERPROC(2, TEXT("_DestroyDdeThreadInfo(pdti=%08X)"), pdti);

    if (pdti->hddNameService)
    {
        ASSERT(pdti->hszService);
        DdeNameService(pdti->dwDDEInst, pdti->hszService, 0, DNS_UNREGISTER);
        pdti->hddNameService = 0;
    }

    if (pdti->hszService)
    {
        DdeFreeStringHandle(pdti->dwDDEInst, pdti->hszService);
        pdti->hszService = 0;
    }

    if (pdti->hszReturn)
    {
        DdeFreeStringHandle(pdti->dwDDEInst, pdti->hszReturn);
        pdti->hszReturn = 0;
    }

    if (pdti->dwDDEInst)
    {
        DdeUninitialize(pdti->dwDDEInst);
        pdti->dwDDEInst = 0;
    }

    EXITPROC(2, TEXT("_DestroyDdeThreadInfo!"));
    return;
}

//
// _AddWinitem - adds a winitem to _hdsaWinitem
//
BOOL CIEDDE::_AddWinitem(WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_AddWinitem(pwi=%08X)"), pwi);

    ENTER_IEDDE_CRIT;

    if (!_hdsaWinitem)
    {
        _hdsaWinitem = DSA_Create(SIZEOF(WINITEM), DXA_GROWTH_AMOUNT);
    }

    if (_hdsaWinitem)
    {
        if (DSA_AppendItem(_hdsaWinitem, pwi) != -1)
        {
            fSuccess = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _AddWinitem could not append an item");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _AddWinitem could not create hdsa");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("_AddWinitem=%d"), fSuccess);
    return fSuccess;
}

//
// _UpdateWinitem - updates a winitem based on the dwWindowID.
//
BOOL CIEDDE::_UpdateWinitem(WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_UpdateWinitem(pwi=%08X)"), pwi);

    ENTER_IEDDE_CRIT;

    if (_hdsaWinitem)
    {
        int cItems = DSA_GetItemCount(_hdsaWinitem);

        for (int i=0; i<cItems; i++)
        {
            WINITEM wi;

            if (DSA_GetItem(_hdsaWinitem, i, &wi) != -1)
            {
                if (wi.dwWindowID == pwi->dwWindowID)
                {
                    if (DSA_SetItem(_hdsaWinitem, i, pwi))
                    {
                        fSuccess = TRUE;
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: _UpdateWinitem could not update an item");
                    }
                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _UpdateWinitem could not get an item");
            }
        }
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("_UpdateWinitem=%d"), fSuccess);
    return fSuccess;
}

//
// AddUrlEcho - adds an UrlEcho entry to the dpa
//
BOOL CIEDDE::AddUrlEcho(LPCTSTR pszUrlEcho)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("AddUrlEcho(pszUrlEcho=>%s<)"), pszUrlEcho);

    ENTER_IEDDE_CRIT;

    if (!_hdpaUrlEcho)
    {
        _hdpaUrlEcho = DPA_Create(DXA_GROWTH_AMOUNT);
    }

    if (_hdpaUrlEcho)
    {
        if (DPA_AppendPtr(_hdpaUrlEcho, (LPVOID)pszUrlEcho) != -1)
        {
            fSuccess = TRUE;
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: AddUrlEcho unable to append a ptr");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: AddUrlEcho unable to create a dpa");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("AddUrlEcho=%d"), fSuccess);
    return fSuccess;
}

//
// RemoveUrlEcho - remove an UrlEcho entry from the dpa
//
BOOL CIEDDE::RemoveUrlEcho(LPCTSTR pszUrlEcho)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("RemoveUrlEcho(pszUrlEcho=>%s<)"), pszUrlEcho);

    ENTER_IEDDE_CRIT;

    if (_hdpaUrlEcho)
    {
        for (int i=0; i<DPA_GetPtrCount(_hdpaUrlEcho); i++)
        {
            LPTSTR pszList = (LPTSTR)DPA_GetPtr(_hdpaUrlEcho, i);
            if (pszList)
            {
                if (StrCmpI(pszList, pszUrlEcho) == 0)
                {
                    DPA_DeletePtr(_hdpaUrlEcho, i);
                    LocalFree((HANDLE)pszList);
                    fSuccess = TRUE;
                    break;
                }
            }
            else
            {
                TraceMsg(TF_ALWAYS, "IEDDE: RemoveUrlEcho unable to get dpa ptr");
            }
        }

        if (!fSuccess)
        {
            TraceMsg(TF_WARNING, "IEDDE: RemoveUrlEcho unable to find server");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: RemoveUrlEcho unable to find dpa");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("RemoveUrlEcho=%d"), fSuccess);
    return fSuccess;
}

//
// AddProtocolHandler - add a PROTOCOLREG entry to the dsa
//
BOOL CIEDDE::AddProtocolHandler(LPCTSTR pszServer, LPCTSTR pszProtocol)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("AddProtocolHandler(pszServer=>%s<,pszProtocol=>%s<)"), pszServer, pszProtocol);

    ENTER_IEDDE_CRIT;

    PROTOCOLREG pr;
    //
    // First, see if anybody else grabbed the protocol first.
    //
    BOOL fFoundHandler = FALSE;
    if (_hdsaProtocolHandler)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaProtocolHandler); i++)
        {
            if (DSA_GetItem(_hdsaProtocolHandler, i, &pr) != -1)
            {
                if (StrCmpI(pr.pszProtocol, pszProtocol) == 0)
                {
                    TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler already has a handler");
                    fFoundHandler = TRUE;
                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler unable to get an item");
            }
        }
    }

    if (!fFoundHandler)
    {
        if (!_hdsaProtocolHandler)
        {
            _hdsaProtocolHandler = DSA_Create(SIZEOF(PROTOCOLREG), DXA_GROWTH_AMOUNT);
        }

        if (_hdsaProtocolHandler)
        {
            pr.pszServer = StrDup(pszServer);
            if (pr.pszServer)
            {
                pr.pszProtocol = StrDup(pszProtocol);
                if (pr.pszProtocol)
                {
                    if (DSA_AppendItem(_hdsaProtocolHandler, &pr) != -1)
                    {
                        fSuccess = TRUE;
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler unable to append to dsa");
                    }

                    if (!fSuccess)
                    {
                        LocalFree((HANDLE)pr.pszProtocol);
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler unable to dup protocol");
                }

                if (!fSuccess)
                {
                    LocalFree((HANDLE)pr.pszServer);
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler unable to dup server");
            }

        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: AddProtocolHandler unable to create dsa");
        }
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("AddProtocolHandler=%d"), fSuccess);
    return fSuccess;
}

//
// RemoveProtocolHandler - removes a PROTOCOLREG item from the dsa
//
BOOL CIEDDE::RemoveProtocolHandler(LPCTSTR pszServer, LPCTSTR pszProtocol)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("RemoveProtocolHandler(pszServer=>%s<,pszProtocol=>%s<)"), pszServer, pszProtocol);

    ENTER_IEDDE_CRIT;

    if (_hdsaProtocolHandler)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaProtocolHandler); i++)
        {
            PROTOCOLREG pr;

            if (DSA_GetItem(_hdsaProtocolHandler, i, &pr) != -1)
            {
                if (StrCmpI(pr.pszProtocol, pszProtocol) == 0)
                {
                    if (StrCmpI(pr.pszServer, pszServer) == 0)
                    {
                        if (DSA_DeleteItem(_hdsaProtocolHandler, i) != -1)
                        {
                            LocalFree((HANDLE)pr.pszServer);
                            LocalFree((HANDLE)pr.pszProtocol);
                            fSuccess = TRUE;
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "IEDDE: RemoveProtocolHandler unable to remove item");
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: RemoveProtocolHandler says server didn't match");
                    }

                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: RemoveProtocolHandler unable to get item");
            }
        }

        if (!fSuccess)
        {
            TraceMsg(TF_WARNING, "IEDDE: RemoveProtocolHandler unable to complete mission");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: RemoveProtocolHandler can't find the dsa");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("RemoveProtocolHandler=%d"), fSuccess);
    return fSuccess;
}

//
// _DeleteWinitemByHwnd - removes a winitem from _hdsaWinitem
//
BOOL CIEDDE::_DeleteWinitemByHwnd(HWND hwnd, WINITEM *pwi)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(2, TEXT("_DeleteWinitemByHwnd(hwnd=%08X,pwi=%08X)"), hwnd, pwi);

    ENTER_IEDDE_CRIT;

    if (_hdsaWinitem)
    {
        for (int i=0; i<DSA_GetItemCount(_hdsaWinitem); i++)
        {
            WINITEM wi;

            if (DSA_GetItem(_hdsaWinitem, i, &wi) != -1)
            {
                if (wi.hwnd == hwnd)
                {
                    if (DSA_DeleteItem(_hdsaWinitem, i) != -1)
                    {
                        *pwi = wi;
                        fSuccess = TRUE;
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: _DeleteWinitemByHwnd could note delete an item");
                    }

                    break;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _DeleteWinitemByHwnd could note get an item");
            }
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _DeleteWinitemByHwnd has no _hdsaWinitem");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(2, TEXT("_DeleteWinitemByHwnd=%d"), fSuccess);
    return fSuccess;
}

//
// _NewWindow - Add a browser window to the internal list
//
BOOL CIEDDE::_NewWindow(HWND hwnd)
{
    BOOL fSuccess = FALSE;

    ENTERPROC(1, TEXT("NewWindow(hwnd=%08X)"), hwnd);

    ASSERT(IS_VALID_HANDLE(hwnd, WND));

    ENTER_IEDDE_CRIT;

    WINITEM wi;
    if (GetWinitemFromHwnd(hwnd, &wi) == FALSE)
    {
        CIEDDEThread *pidt = new CIEDDEThread();

        if (pidt)
        {
            DDETHREADINFO dti = {0};
            DWORD dwThreadID = GetCurrentThreadId();
            WINITEM wi;
            BOOL fCreatedDTI = FALSE;

            if (_GetWinitemFromThread(dwThreadID, &wi))
            {
                wi.pidt->GetDdeThreadInfo(&dti);
            }
            else
            {
                LEAVE_IEDDE_CRIT;
                _CreateDdeThreadInfo(&dti);
                ENTER_IEDDE_CRIT;
                fCreatedDTI = TRUE;
            }

            if (dti.dwDDEInst)
            {
                static DWORD dwNextWindowID = 1;

                pidt->SetDdeThreadInfo(&dti);

                wi.dwThreadID = dwThreadID;
                wi.pidt = pidt;
                wi.hwnd = hwnd;
                wi.dwWindowID = dwNextWindowID++;

                if (_AddWinitem(&wi))
                {
                    //
                    // Now that we have a (partial) winitem in the winitem
                    // database, we can register our name service.  If we
                    // registered any sooner, there is a risk that an app
                    // will try to connect to us while we are registering,
                    // and we will fail the connect because the winitem
                    // is not in the registry yet.
                    //
                    LEAVE_IEDDE_CRIT;
                    dti.hddNameService = DdeNameService(dti.dwDDEInst, dti.hszService, 0, DNS_REGISTER);
                    ENTER_IEDDE_CRIT;

                    //
                    // Now that we have hddNameService, we can update the
                    // winitem in the database.
                    //
                    if (dti.hddNameService)
                    {
                        pidt->SetDdeThreadInfo(&dti);
                        if (_UpdateWinitem(&wi))
                        {
                            fSuccess = TRUE;
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "IEDDE: _NewWindow unable to update a win item");
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "IEDDE: _NewWindow unable to register service");
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "IEDDE: _NewWindow could not append win item");
                }

                if (!fSuccess && fCreatedDTI)
                {
                    LEAVE_IEDDE_CRIT;
                    _DestroyDdeThreadInfo(&dti);
                    ENTER_IEDDE_CRIT;
                }
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _NewWindow could not get/create dde thread info");
            }

            if (!fSuccess)
            {
                delete pidt;
            }
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: _NewWindow could not create iedde thread object");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _NewWindow says window already registered?!?");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(1, TEXT("_NewWindow=%d"), fSuccess);
    return fSuccess;
}

//
// _WindowDestroyed - Remove a browser window from the internal list
//
BOOL CIEDDE::_WindowDestroyed(HWND hwnd)
{
    BOOL fSuccess = FALSE;
    ENTERPROC(1, TEXT("_WindowDestroyed(hwnd=%08X)"), hwnd);

    ENTER_IEDDE_CRIT;

    WINITEM wi;
    if (_DeleteWinitemByHwnd(hwnd, &wi))
    {
        fSuccess = TRUE;

        ASSERT(wi.pidt);
        WINITEM wiThread;

        if (_GetWinitemFromThread(wi.dwThreadID, &wiThread) == FALSE)
        {
            if (wi.dwThreadID == GetCurrentThreadId())
            {
                DDETHREADINFO dti;

                wi.pidt->GetDdeThreadInfo(&dti);
                // Don't hold onto critical section while doing this...
                LEAVE_IEDDE_CRIT;
                _DestroyDdeThreadInfo(&dti);
                ENTER_IEDDE_CRIT;
            }
            else
            {
                TraceMsg(TF_WARNING, "IEDDE: _WindowDestroyed called on wrong thread");
            }
        }

        delete wi.pidt;
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: _WindowDestroyed could not find hwnd");
    }

    LEAVE_IEDDE_CRIT;

    EXITPROC(1, TEXT("_WindowDestroyed=%d"), fSuccess);
    return fSuccess;
}





//
// IEDDE_ functions are those exported for other parts of shdocvw to call.
// They pretty much just call the equivalent function in g_pIEDDE.
//

BOOL IEDDE_Initialize(void)
{
    BOOL fRet = FALSE;

    ENTERPROC(2, TEXT("IEDDE_Initialize()"));

    if (g_pIEDDE == NULL)
    {
        g_pIEDDE = new CIEDDE;

        if (g_pIEDDE)
        {
            fRet = g_pIEDDE->_Initialize();
        }
        else
        {
            TraceMsg(TF_WARNING, "IEDDE: IEDDE_Initialize could not allocate an IEDDE object");
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_Initialize says already initialized");
    }

    EXITPROC(2, TEXT("IEDDE_Initialize=%d"), fRet);
    return fRet;
}

void IEDDE_Uninitialize(void)
{
    ENTERPROC(2, TEXT("IEDDE_Uninitialize()"));

    if (g_pIEDDE)
    {
        g_pIEDDE->_Uninitialize();
        delete g_pIEDDE;
        g_pIEDDE = NULL;
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_Uninitialize has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_Uninitialize!"));
}

void IEDDE_AutomationStarted(void)
{
    ENTERPROC(2, TEXT("IEDDE_AutomationStarted()"));

    if (g_pIEDDE)
    {
        g_pIEDDE->_AutomationStarted();
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_AutomationStarted has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_AutomationStarted!"));
}

HRESULT IEDDE_BeforeNavigate(LPCWSTR pwszURL, BOOL *pfCanceled)
{
    HRESULT hr = E_FAIL;

    ENTERPROC(2, TEXT("IEDDE_BeforeNavigate(pwszURL=%08X,pfCanceled=%08X)"), pwszURL, pfCanceled);

    if (g_pIEDDE)
    {
        LPCTSTR pszURL;

        pszURL = pwszURL;

        if (pszURL)
        {
            hr = g_pIEDDE->_BeforeNavigate(pszURL, pfCanceled);
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_BeforeNavigate has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_BeforeNavigate=%08X"), hr);
    return hr;
}

HRESULT IEDDE_AfterNavigate(LPCWSTR pwszURL, HWND hwnd)
{
    HRESULT hr = E_FAIL;

    ENTERPROC(2, TEXT("IEDDE_AfterNavigate(pwszURL=%08X,hwnd=%08X)"), pwszURL, hwnd);

    if (g_pIEDDE)
    {
        LPCTSTR pszURL;

        pszURL = pwszURL;

        if (pszURL)
        {
            hr = g_pIEDDE->_AfterNavigate(pszURL, hwnd);
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_AfterNavigate has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_AfterNavigate=%08X"), hr);
    return hr;
}

BOOL IEDDE_NewWindow(HWND hwnd)
{
    BOOL fRet = FALSE;

    ENTERPROC(2, TEXT("IEDDE_NewWindow(hwnd=%08X)"), hwnd);

    if (g_pIEDDE)
    {
        fRet = g_pIEDDE->_NewWindow(hwnd);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_NewWindow has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_NewWindow=%d"), fRet);
    return fRet;
}

BOOL IEDDE_WindowDestroyed(HWND hwnd)
{
    BOOL fRet = FALSE;

    ENTERPROC(2, TEXT("IEDDE_WindowDestroyed(hwnd=%08X)"), hwnd);

    if (g_pIEDDE)
    {
        fRet = g_pIEDDE->_WindowDestroyed(hwnd);
    }
    else
    {
        TraceMsg(TF_WARNING, "IEDDE: IEDDE_WindowDestroyed has no IEDDE object");
    }

    EXITPROC(2, TEXT("IEDDE_WindowDestroyed=%d"), fRet);
    return fRet;
}





#ifdef DEBUG

//
// BUGBUG - Move g_dwIEDDETrace into ccshell.ini to prevent recompiles.
//

DWORD g_dwIEDDETrace = 0;
static DWORD g_dwIndent = 0;
static const TCHAR c_szDotDot[] = TEXT("..");

#define MAX_INDENTATION_VALUE   0x10

void EnterProc(DWORD dwTraceLevel, LPTSTR szFmt, ...)
{
    TCHAR szOutput[1000];
    va_list arglist;

    if (dwTraceLevel <= g_dwIEDDETrace)
    {
        szOutput[0] = TEXT('\0');

        for (DWORD i=0; i<g_dwIndent; i++)
        {
            StrCatBuff(szOutput, c_szDotDot, ARRAYSIZE(szOutput));
        }

        va_start(arglist, szFmt);
        wvnsprintf(szOutput + lstrlen(szOutput), ARRAYSIZE(szOutput) - lstrlen(szOutput), szFmt, arglist);
        va_end(arglist);

        TraceMsg(TF_ALWAYS, "%s", szOutput);

        // This value can get out of hand if EnterProc and ExitProc
        // calls do not match. This can trash the stack.
        if(g_dwIndent < MAX_INDENTATION_VALUE)
            g_dwIndent++;
    }
}

void ExitProc(DWORD dwTraceLevel, LPTSTR szFmt, ...)
{
    TCHAR szOutput[1000];
    va_list arglist;

    if (dwTraceLevel <= g_dwIEDDETrace)
    {
        // This can happen if the EnterProc and 
        // ExitProc calls do not match.
        if(g_dwIndent > 0)
            g_dwIndent--;

        szOutput[0] = TEXT('\0');

        for (DWORD i=0; i<g_dwIndent; i++)
        {
            StrCatBuff(szOutput, c_szDotDot, ARRAYSIZE(szOutput));
        }

        va_start(arglist, szFmt);
        wvnsprintf(szOutput + lstrlen(szOutput), ARRAYSIZE(szOutput) - lstrlen(szOutput), szFmt, arglist);
        va_end(arglist);

        TraceMsg(TF_ALWAYS, "%s", szOutput);
    }
}

#endif
