/*-------------------------------------------------------*/
//Copyright (c) 1997  Microsoft Corporation
//
//Module Name: Url Tracking Log Interfaces
//
//    Urltrack.cpp
//
//
//Author:
//
//    Pei-Hwa Lin (peihwal)  19-March-97
//
//Environment:
//
//    User Mode - Win32
//
//Revision History:
//    5/13/97   due to cache container type change, allow
//              OPEN_ALWAYS when CreateFile
//    5/14/97   remove IsOnTracking, TRACK_ALL, unused code
/*-------------------------------------------------------*/

#include "priv.h"
#include "wininet.h"
#include "basesb.h"
#include "bindcb.h"
const WCHAR c_szPropURL[] = L"HREF";
const WCHAR c_szProptagName[] = L"Item";
const TCHAR c_szLogContainer[] = TEXT("Log");

#define MY_MAX_STRING_LEN           512



//---------------------------------------------------------------------------
//
// IUnknown interfaces
//
//---------------------------------------------------------------------------
HRESULT
CUrlTrackingStg :: QueryInterface(REFIID riid, PVOID *ppvObj)
{
    HRESULT hr = E_NOINTERFACE;


    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IUrlTrackingStg))
    {
        AddRef();
        *ppvObj = (LPVOID) SAFECAST(this, IUrlTrackingStg *);
        hr = S_OK;

    }

    return hr;
}


ULONG
CUrlTrackingStg :: AddRef(void)
{
    _cRef ++;
    return _cRef;
}

ULONG
CUrlTrackingStg :: Release(void)
{

    ASSERT(_cRef > 0);

    _cRef--;

    if (!_cRef)
    {
        //time to go bye bye
        delete this;
        return 0;
    }

    return _cRef;
}

//---------------------------------------------------------------------------
//
// C'tor/D'tor
//
//---------------------------------------------------------------------------
CUrlTrackingStg :: CUrlTrackingStg()
{

    _hFile = NULL;
    _pRecords = NULL;
    _lpPfx = NULL;
}

CUrlTrackingStg :: ~CUrlTrackingStg()
{
    // browser exit
    while (_pRecords)
    {
        OnUnload(_pRecords->pthisUrl);
    };

//    DeleteAllNode();    

    if (_lpPfx)
        GlobalFree(_lpPfx);

    if (_hFile)
    {
        CloseHandle(_hFile);
        _hFile = NULL;
    }
}


//---------------------------------------------------------------------------
//
// Helper functions
//
//---------------------------------------------------------------------------
LRecord *
CUrlTrackingStg :: AddNode()
{
    LRecord* pTemp;
    LRecord* pNew = NULL;
    
    pNew = (LRecord *)LocalAlloc(LPTR, sizeof(LRecord));
    if (pNew == NULL)
        return NULL;
    
    pNew->pNext = NULL;
    if (_pRecords == NULL)
    {
        //special case for first node
        _pRecords = pNew;
    }
    else
    {
        for (pTemp = _pRecords; pTemp->pNext; pTemp = pTemp->pNext);
        pTemp->pNext = pNew;
    }
    
    return pNew;
}

void 
CUrlTrackingStg :: DeleteAllNode()
{
   
    do
    {
        DeleteFirstNode();
    }
    while (_pRecords);

    return;
}

void
CUrlTrackingStg :: DeleteFirstNode()
{
    LRecord *pTemp;

    if (!_pRecords)
        return;

    pTemp = _pRecords;
    _pRecords = pTemp->pNext;
    delete [] pTemp->pthisUrl;
    LocalFree(pTemp);
    return;
}

void
CUrlTrackingStg :: DeleteCurrentNode(LRecord *pThis)
{
    LRecord *pPrev;
    
    if (_pRecords == pThis)
    {
        DeleteFirstNode();
        return;
    }

    pPrev = _pRecords;
    do
    {
        if (pPrev->pNext == pThis)
        {
            pPrev->pNext = pThis->pNext;
            delete [] pThis->pthisUrl;
            LocalFree(pThis);
            break;
        }
        pPrev = pPrev->pNext;
    }
    while (pPrev);

    return;
}

//
// return Current node by comparing url strings
//
LRecord*
CUrlTrackingStg :: FindCurrentNode
(
    IN  LPCTSTR       lpUrl
)
{
    LRecord* pThis = NULL;

    ASSERT(_pRecords);
    if (!_pRecords)                 // missed OnLoad
        return NULL;

    pThis = _pRecords;
    do
    {
        if (!StrCmpI(lpUrl, pThis->pthisUrl))
            break;

        pThis = pThis->pNext;
    }
    while (pThis);

    return pThis;
}

void
CUrlTrackingStg :: cleanup()
{
    return;
}


void
CUrlTrackingStg :: DetermineAppModule()
{
    TCHAR   szModule[MAX_PATH];
    LPTSTR  szExt;
        
    if (GetModuleFileName(NULL, szModule, MAX_PATH))        
    {
        szExt = PathFindExtension(szModule);
        TraceMsg(0, "tracking: AppModule %s", szModule);
            
        if (StrCmpI(szExt, TEXT(".SCR")) == 0)
            _fScreenSaver = TRUE;
        else
            _fScreenSaver = FALSE;
                
    }
    else
        _fScreenSaver = FALSE;

    _fModule = TRUE;
}
            
//---------------------------------------------------------------------------
//
// OnLoad(LPTSTR lpUrl, BRMODE context, BOOL fUseCache)
//      a new page is loaded
//      this function will remember time entering this page, context browsing
//      from and page URL string.
//      (lpUrl does NOT contain "track:" prefix)
//---------------------------------------------------------------------------
HRESULT
CUrlTrackingStg :: OnLoad
(
    IN  LPCTSTR    lpUrl,
    IN  BRMODE     ContextMode,
    IN  BOOL       fUseCache
)
{
    HRESULT     hr = E_OUTOFMEMORY;
    SYSTEMTIME  st;
    LRecord*    pNewNode = NULL;

    GetLocalTime(&st);

    pNewNode = AddNode();
    if (!pNewNode)
        return hr;

    int cch = lstrlen(lpUrl)+1;
    pNewNode->pthisUrl = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
    if (pNewNode->pthisUrl == NULL)
        return hr;

    // store log info
    StrCpyN(pNewNode->pthisUrl, lpUrl, cch);
    
    if (!_fModule)
        DetermineAppModule();

    // if it's from SS, the fullscreen flag will be set,
    // need to override ContextMode passed in
    if (_fScreenSaver)
        pNewNode->Context = BM_SCREENSAVER;
    else
        pNewNode->Context = (ContextMode > BM_THEATER) ? BM_UNKNOWN : ContextMode;

#if 0   //do not open till urlmon support wininet query flag
    DWORD dwOptions = 0;
    DWORD dwSize;
    WCHAR wszURL[MAX_URL_STRING];

    AnsiToUnicode(lpUrl, wszURL, ARRAYSIZE(wszURL));
    if (SUCCEEDED(CoInitialize(NULL)) && 
        SUCCEEDED(CoInternetQueryInfo(wszURL, (QUERYOPTION)INTERNET_OPTION_REQUEST_FLAGS, 
                            0, &dwOptions, sizeof(DWORD), &dwSize, 0)))
    {
        pNewNode->fuseCache = dwOptions & INTERNET_REQFLAG_FROM_CACHE;
        CoUninitialize();
    }
    else
#endif
    {
        BYTE cei[MAX_CACHE_ENTRY_INFO_SIZE];
        LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)cei;
        DWORD       cbcei = MAX_CACHE_ENTRY_INFO_SIZE;

        if (GetUrlCacheEntryInfo(lpUrl, pcei, &cbcei))
            pNewNode->fuseCache = (pcei->dwHitRate - 1) ? TRUE : FALSE;     // off 1 by download
        else
            pNewNode->fuseCache = 0;

    }

    SystemTimeToFileTime(&st, &(pNewNode->ftIn));

    return S_OK; 
}



//---------------------------------------------------------------------------
//
// OnUnLoad(LPTSTR lpUrl)
//      current page is unloaded
//      1)find url cache entry and get file handle
//      2)calculate total time duration visiting this page
//      3)commit delta log string to file cache entry
//      (lpUrl contains "Tracking: " prefix)
//
//---------------------------------------------------------------------------
HRESULT
CUrlTrackingStg :: OnUnload
(
    IN  LPCTSTR   lpUrl
)
{
    HRESULT     hr = E_FAIL;
    LPTSTR       lpPfxUrl = NULL;
    LRecord*    pNode = NULL;;
    SYSTEMTIME  st;
    LPINTERNET_CACHE_ENTRY_INFO pce = NULL;
    TCHAR       lpFile[MAX_PATH];
    

    // 
    GetLocalTime(&st);

    pNode = FindCurrentNode(lpUrl);
    if (!pNode)
    {
        TraceMsg(DM_ERROR, "CUrlTrackingStg: OnUnload (cannot find internal tracking log");
        return hr;
    }

    //QueryCacheEntry() and OpenLogFile() can be combined in one if CacheAPI supports
    //WriteUrlCacheEntryStream()
    ConvertToPrefixedURL(lpUrl, &lpPfxUrl);
    if (!lpPfxUrl)
    {
        return E_OUTOFMEMORY;
    }

    pce = QueryCacheEntry(lpPfxUrl);
    if (!pce)
    {
        TraceMsg(DM_ERROR, "CUrlTrackingStg: OnUnload (cannot find url cache entry)");
        DeleteCurrentNode(pNode);
    
        // free pce
        GlobalFree(lpPfxUrl);
        return hr;
    }

    // work around -- begin
    hr = WininetWorkAround(lpPfxUrl, pce->lpszLocalFileName, &lpFile[0]);
    if (FAILED(hr))
    {
        TraceMsg(DM_ERROR, "CUrlTrackingStg: OnUnload (failed to work around wininet)");
        DeleteCurrentNode(pNode);
        CloseHandle(_hFile);   
        GlobalFree(lpPfxUrl);
        return hr;
    }
    
    hr = UpdateLogFile(pNode, &st);

    // commit change to cache
    if(SUCCEEDED(hr))
    {
        hr = (CommitUrlCacheEntry(lpPfxUrl, 
                lpFile,    //
                pce->ExpireTime,                    //ExpireTime
                pce->LastModifiedTime,              //LastModifiedTime
                pce->CacheEntryType,
                NULL,                               //lpHeaderInfo
                0,                                  //dwHeaderSize
                NULL,                               //lpszFileExtension
                0) ) ?                              //reserved
                S_OK : E_FAIL;
    }
    
    // work around -- end

    DeleteCurrentNode(pNode);
    
    // free pce
    GlobalFree(pce);
    GlobalFree(lpPfxUrl);

    return hr;
}

//---------------------------------------------------------------------------
// 
// Cache helper funcitons
// This is a workaround for Wininet cache
// Later when we commit change to URL cache will fail if localFile size is changed
//  [IN] lpszSourceUrlName and lpszLocalFileName remain the same when calling 
//       this routine
//  [OUT] new local file name 
//
//---------------------------------------------------------------------------
HRESULT CUrlTrackingStg :: WininetWorkAround(LPCTSTR lpszUrl, LPCTSTR lpOldFile, LPTSTR lpFile)
{
    HRESULT  hr = E_FAIL;

    ASSERT(!_hFile);

    if (!CreateUrlCacheEntry(lpszUrl, 512, TEXT("log"), lpFile, 0))
        return E_FAIL;
    
    if (lpOldFile)
    {
        if (!CopyFile(lpOldFile, lpFile, FALSE))
            return E_FAIL;

        DeleteFile(lpOldFile);
    }

    _hFile = OpenLogFile(lpFile);

    return (_hFile != INVALID_HANDLE_VALUE) ? S_OK : E_FAIL;        
}

LPINTERNET_CACHE_ENTRY_INFO
CUrlTrackingStg :: QueryCacheEntry
(
    IN  LPCTSTR     lpUrl
)
{
    // get cache entry info
    LPINTERNET_CACHE_ENTRY_INFO       lpCE = NULL;
    DWORD    dwEntrySize;
    BOOL     bret = FALSE;

    lpCE = (LPINTERNET_CACHE_ENTRY_INFO)GlobalAlloc(LPTR, MAX_CACHE_ENTRY_INFO_SIZE);
    if (lpCE)
    {
        dwEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;

        while (!(bret = GetUrlCacheEntryInfo(lpUrl, lpCE, &dwEntrySize)))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                GlobalFree(lpCE);

                lpCE = (LPINTERNET_CACHE_ENTRY_INFO)GlobalAlloc(LPTR, dwEntrySize);
                if (!lpCE)
                    break;
            }
            else
                break;
        }
    }

    if (!bret && lpCE)
    {
        GlobalFree(lpCE);
        lpCE = NULL;
        SetLastError(ERROR_FILE_NOT_FOUND);
    }

    return lpCE;

}


//---------------------------------------------------------------------------
// 
// File helper funcitons
//
//---------------------------------------------------------------------------

//
// 1)open log file 
// 2)move file pointer to end of file
//
HANDLE
CUrlTrackingStg :: OpenLogFile
(
    IN LPCTSTR  lpFileName
)
{
    HANDLE hFile = NULL;
    
    hFile = CreateFile(lpFileName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,  // | FILE_FLAG_SEQUENTIAL_SCAN,  
            NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;        

    return hFile;
    
}

const TCHAR c_szLogFormat[] = TEXT("hh':'mm':'ss");
const LPTSTR c_szMode[] = { TEXT("N"),       // normal browsing
                            TEXT("S"),       // screen saver
                            TEXT("D"),       // desktop component
                            TEXT("T"),       // theater mode
                            TEXT("U"),       // unknown
                          };     

HRESULT
CUrlTrackingStg :: UpdateLogFile
(
    IN LRecord*     pNode,
    IN SYSTEMTIME*  pst
)
{
    FILETIME    ftOut;
    DWORD       dwWritten= 0;
    HRESULT     hr = E_FAIL;
    ULARGE_INTEGER ulIn, ulOut, ulTotal;

    ASSERT(_hFile);
    
    // calculate delta of time
    SystemTimeToFileTime(pst, &ftOut);

    // #34829: use 64-bit calculation
	ulIn.LowPart = pNode->ftIn.dwLowDateTime;
	ulIn.HighPart = pNode->ftIn.dwHighDateTime;
	ulOut.LowPart = ftOut.dwLowDateTime;
	ulOut.HighPart = ftOut.dwHighDateTime;
	QUAD_PART(ulTotal) = QUAD_PART(ulOut) - QUAD_PART(ulIn);
    
    ftOut.dwLowDateTime = ulTotal.LowPart;
    ftOut.dwHighDateTime = ulTotal.HighPart;

    // log string: timeEnter+Duration
    SYSTEMTIME  stOut, stIn;
    TCHAR   lpLogString[MY_MAX_STRING_LEN];
    TCHAR   pTimeIn[10], pTimeOut[10];
    
    FileTimeToSystemTime(&ftOut, &stOut);
    FileTimeToSystemTime(&(pNode->ftIn), &stIn);
    
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &stIn, c_szLogFormat, pTimeIn, 10);
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &stOut, c_szLogFormat, pTimeOut, 10);
    // #34832: add Date in logs
    // #28266: add LFCR in logs
    lpLogString[0] = '\0';
    wnsprintf(lpLogString, ARRAYSIZE(lpLogString), TEXT("%s %d %.2d-%.2d-%d %s %s\r\n"), 
                                c_szMode[pNode->Context], 
                                pNode->fuseCache, 
                                stIn.wMonth, stIn.wDay, stIn.wYear,
                                pTimeIn, pTimeOut);
    
    // move file pointer to end
    if (0xFFFFFFFF == SetFilePointer(_hFile, 0, 0, FILE_END))
    {
        CloseHandle(_hFile);
        _hFile = NULL;
        return hr;
    }
    
    // write ANSI string to file
    char szLogInfo[MY_MAX_STRING_LEN];

    SHTCharToAnsi(lpLogString, szLogInfo, ARRAYSIZE(szLogInfo));
    hr = (WriteFile(_hFile, szLogInfo, lstrlenA(szLogInfo), &dwWritten, NULL)) ?
             S_OK : E_FAIL;
       
    CloseHandle(_hFile);
    _hFile = NULL;
    return hr;  

}

//-----------------------------------------------------------------------------
//
// ReadTrackingPrefix
//
// read prefix string from registry
//-----------------------------------------------------------------------------
void
CUrlTrackingStg :: ReadTrackingPrefix(void)
{
    DWORD   cbPfx = 0;
    struct {
        INTERNET_CACHE_CONTAINER_INFO cInfo;
        TCHAR  szBuffer[MAX_PATH+MAX_PATH];
    } ContainerInfo;
    DWORD   dwModified, dwContainer;
    HANDLE  hEnum;
  
    dwContainer = sizeof(ContainerInfo);
    hEnum = FindFirstUrlCacheContainer(&dwModified,
                                       &ContainerInfo.cInfo,
                                       &dwContainer,
                                       0);

    if (hEnum)
    {

        for (;;)
        {
            if (!StrCmpI(ContainerInfo.cInfo.lpszName, c_szLogContainer))
            {
                DWORD cch = lstrlen(ContainerInfo.cInfo.lpszCachePrefix)+1;
                ASSERT(ContainerInfo.cInfo.lpszCachePrefix[0]);

                _lpPfx = (LPTSTR)GlobalAlloc(LPTR, cch * sizeof(TCHAR));
                if (!_lpPfx)
                    SetLastError(ERROR_OUTOFMEMORY);

                StrCpyN(_lpPfx, ContainerInfo.cInfo.lpszCachePrefix, cch);
                break;
            }

            dwContainer = sizeof(ContainerInfo);
            if (!FindNextUrlCacheContainer(hEnum, &ContainerInfo.cInfo, &dwContainer))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }

        }

        FindCloseUrlCache(hEnum);
    }
}


// caller must free lplpPrefixedUrl
BOOL 
CUrlTrackingStg :: ConvertToPrefixedURL(LPCTSTR lpszUrl, LPTSTR *lplpPrefixedUrl)
{
    BOOL    bret = FALSE;

    ASSERT(lpszUrl);
    if (!lpszUrl)
        return bret;

    //ASSERT(lplpPrefixedUrl);

    if (!_lpPfx)
        ReadTrackingPrefix();
    
    if (_lpPfx)
    {
        int len = lstrlen(lpszUrl) + lstrlen(_lpPfx) + 1;
        
        *lplpPrefixedUrl = (LPTSTR)GlobalAlloc(LPTR, len * sizeof(TCHAR));
        if (*lplpPrefixedUrl)
        {
            wnsprintf(*lplpPrefixedUrl, len, TEXT("%s%s"), _lpPfx, lpszUrl);
            bret = TRUE;
        }
    }

    return bret;
}
