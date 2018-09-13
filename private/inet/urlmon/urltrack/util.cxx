/*-------------------------------------------------------*/
//Copyright (c) 1997  Microsoft Corporation
//
//    Util.cpp
//
//Author:
//
//
//Environment:
//
//    User Mode - Win32
//
//Revision History:
/*-------------------------------------------------------*/

#include "urltrk.h"
#include <inetreg.h>

const CHAR c_szLogFormat[] = "hh':'mm':'ss";
const CHAR c_szMode[] = "U";       // unknown
const CHAR c_szLogContainerA[] = "Log";

#define MY_CACHE_ENTRY_INFO_SIZE    512
#define MY_MAX_STRING_LEN           512

BOOL   ConvertToPrefixedURL(LPCSTR lpszUrl, LPSTR *lplpPrefixedUrl);
LPINTERNET_CACHE_ENTRY_INFOA QueryCacheEntry(LPCSTR lpUrl);
HANDLE GetLogFile(LPCSTR lpUrl, LPINTERNET_CACHE_ENTRY_INFOA pce, LPSTR lpFile);
LPSTR GetLogString(LPMY_LOGGING_INFO lpLogInfo);


ULONG _IsLoggingEnabled(LPCSTR  pszUrl)
{
    LPINTERNET_CACHE_ENTRY_INFOA pce;
    LPSTR   lpPfxUrl = NULL;
    ULONG   dwTrack;

    //
    ConvertToPrefixedURL(pszUrl, &lpPfxUrl);
    if (!lpPfxUrl)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    pce = QueryCacheEntry(lpPfxUrl);
    if (!pce)
    {
        GlobalFree(lpPfxUrl);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
    }

    dwTrack = pce->CacheEntryType;
    GlobalFree(pce);
    GlobalFree(lpPfxUrl);
    return dwTrack; 
}

BOOL _WriteHitLogging(LPMY_LOGGING_INFO pmLi)
{
    LPSTR   lpLogString = NULL;
    LPSTR   lpPfxUrl = NULL;
    LPINTERNET_CACHE_ENTRY_INFOA pce = NULL;
    HANDLE  hFile;
    CHAR    lpFile[MAX_PATH];
    DWORD   dwWritten = 0;
    BOOL    fuseCache;
    BOOL    bRet = FALSE;

    //
    ConvertToPrefixedURL(pmLi->pLogInfo->lpszLoggedUrlName, &lpPfxUrl);
    if (!lpPfxUrl)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return bRet;
    }

    pce = QueryCacheEntry(lpPfxUrl);
    if (!pce)
    {
        GlobalFree(lpPfxUrl);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return bRet;
    }

    hFile = GetLogFile(lpPfxUrl, pce, &lpFile[0]);
    if (hFile == NULL)
    {
        GlobalFree(lpPfxUrl);
        GlobalFree(pce);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return bRet;
    }
    
    pmLi->fuseCache = GetUrlCacheEntryInfoExA(pmLi->pLogInfo->lpszLoggedUrlName, NULL, NULL, NULL, NULL, NULL, 0);
    lpLogString = GetLogString(pmLi);
    if (!lpLogString)
    {
        GlobalFree(lpPfxUrl);
        GlobalFree(pce);
        CloseHandle(hFile);
        return bRet; 
    }

    bRet = WriteFile(hFile, lpLogString, lstrlenA(lpLogString), &dwWritten, NULL);
       
    CloseHandle(hFile);
    GlobalFree(lpLogString);
    
    // commit change to cache
    if(bRet)
    {
        bRet = CommitUrlCacheEntry(lpPfxUrl, 
                lpFile,    //
                pce->ExpireTime,                    //ExpireTime
                pce->LastModifiedTime,              //LastModifiedTime
                pce->CacheEntryType,
                NULL,                               //lpHeaderInfo
                0,                                  //dwHeaderSize
                NULL,                               //lpszFileExtension
                0);                              //reserved
    }
    
    // free pce
    GlobalFree(pce);
    GlobalFree(lpPfxUrl);

    return bRet;
}


BOOL IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    HMODULE hModuleHandle = GetModuleHandleA("wininet.dll");

    if(!hModuleHandle)
        return FALSE;

    if(InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

//
// Helper Functions
//
LPSTR GetLogString(LPMY_LOGGING_INFO pmLi)
{
    FILETIME   ftIn, ftOut;
    ULARGE_INTEGER ulIn, ulOut, ulTotal;
    SYSTEMTIME  stIn, stOut;
    LPSTR      lpData = NULL;
    CHAR       pTimeIn[10], pTimeOut[10];
              
    lpData = (LPSTR)GlobalAlloc(LPTR, lstrlenA(pmLi->pLogInfo->lpszExtendedInfo)+MY_MAX_STRING_LEN);
    if (!lpData)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    // calculate delta of time
    SystemTimeToFileTime(&(pmLi->pLogInfo->StartTime), &ftIn);
    SystemTimeToFileTime(&(pmLi->pLogInfo->EndTime), &ftOut);

	ulIn.LowPart = ftIn.dwLowDateTime;
	ulIn.HighPart = ftIn.dwHighDateTime;
	ulOut.LowPart = ftOut.dwLowDateTime;
	ulOut.HighPart = ftOut.dwHighDateTime;
#ifndef unix
	ulTotal.QuadPart = ulOut.QuadPart - ulIn.QuadPart;
#else
        U_QUAD_PART(ulTotal) = U_QUAD_PART(ulOut) - U_QUAD_PART(ulIn);
#endif /* unix */    
    ftOut.dwLowDateTime = ulTotal.LowPart;
    ftOut.dwHighDateTime = ulTotal.HighPart;
    FileTimeToSystemTime(&ftOut, &stOut);
    stIn = pmLi->pLogInfo->StartTime;

    // log string: timeEnter+Duration
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &stIn, c_szLogFormat, pTimeIn, 10);
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &stOut, c_szLogFormat, pTimeOut, 10);

    if (!pmLi->pLogInfo->lpszExtendedInfo)
    {
        wsprintf(lpData, "%s %d %.2d-%.2d-%d %s %s\r\n", c_szMode, 
                                pmLi->fuseCache, 
                                stIn.wMonth, stIn.wDay, stIn.wYear,
                                pTimeIn, pTimeOut);
    }
    else
    {
        wsprintf(lpData, "%s %d %.2d-%.2d-%d %s %s %s\r\n", c_szMode, 
                                pmLi->fuseCache, 
                                stIn.wMonth, stIn.wDay, stIn.wYear,
                                pTimeIn, pTimeOut,
                                pmLi->pLogInfo->lpszExtendedInfo);
    }

    return lpData;
}


LPSTR ReadTrackingPrefix(void)
{
    LPSTR  lpPfx = NULL;

    DWORD   cbPfx = 0;
    struct {
        INTERNET_CACHE_CONTAINER_INFOA cInfo;
        CHAR  szBuffer[MAX_PATH+INTERNET_MAX_URL_LENGTH+1];
    } ContainerInfo;
    DWORD   dwModified, dwContainer;
    HANDLE  hEnum;
  
    dwContainer = sizeof(ContainerInfo);
    hEnum = FindFirstUrlCacheContainerA(&dwModified,
                                       &ContainerInfo.cInfo,
                                       &dwContainer,
                                       0);

    if (hEnum)
    {

        for (;;)
        {
            if (!lstrcmpiA(ContainerInfo.cInfo.lpszName, c_szLogContainerA))
            {
                if (ContainerInfo.cInfo.lpszCachePrefix[0])
                {
				    DWORD   cb = lstrlenA(ContainerInfo.cInfo.lpszCachePrefix)+sizeof(CHAR);
                    lpPfx = (LPSTR)GlobalAlloc(LPTR, cb);
                    if (!lpPfx)
                        SetLastError(ERROR_OUTOFMEMORY);

                    lstrcpynA(lpPfx, ContainerInfo.cInfo.lpszCachePrefix, cb);
                }				
                break;
            }

            dwContainer = sizeof(ContainerInfo);
            if (!FindNextUrlCacheContainerA(hEnum, &ContainerInfo.cInfo, &dwContainer))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }

        }

        FindCloseUrlCache(hEnum);
    }
   
    return lpPfx;
}


// caller must free lplpPrefixedUrl
BOOL 
ConvertToPrefixedURL(LPCSTR lpszUrl, LPSTR *lplpPrefixedUrl)
{
    BOOL    bret = FALSE;
    LPTSTR  lpPfx = NULL;

    if (!lpszUrl)
        return bret;

    lpPfx = ReadTrackingPrefix();
    if (lpPfx)
    {
        *lplpPrefixedUrl = (LPSTR)GlobalAlloc(LPTR, lstrlenA(lpszUrl)+lstrlenA(lpPfx)+1);
        if (*lplpPrefixedUrl)
        {
            wsprintf(*lplpPrefixedUrl, "%s%s", lpPfx, lpszUrl);
            bret = TRUE;
        }
    }

    GlobalFree(lpPfx);
    return bret;
}

LPINTERNET_CACHE_ENTRY_INFOA
QueryCacheEntry(LPCSTR lpUrl)
{
    // get cache entry info
    LPINTERNET_CACHE_ENTRY_INFOA   lpCE = NULL;
    DWORD    dwEntrySize;
    BOOL     bret = FALSE;

    lpCE = (LPINTERNET_CACHE_ENTRY_INFOA)GlobalAlloc(LPTR, MY_CACHE_ENTRY_INFO_SIZE);
    if (lpCE)
    {
        dwEntrySize = MY_CACHE_ENTRY_INFO_SIZE;

        while (!(bret = GetUrlCacheEntryInfoA(lpUrl, lpCE, &dwEntrySize)))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                GlobalFree(lpCE);

                lpCE = (LPINTERNET_CACHE_ENTRY_INFOA)GlobalAlloc(LPTR, dwEntrySize);
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

HANDLE GetLogFile(LPCSTR lpUrl, LPINTERNET_CACHE_ENTRY_INFOA pce, LPSTR lpFile)
{
    HANDLE  hFile = NULL;    

    // work around -- begin
    if (!CreateUrlCacheEntry(lpUrl, 512, "log", lpFile, 0))
        return NULL;
    
    if (pce->lpszLocalFileName)
    {
        if (!CopyFile(pce->lpszLocalFileName, lpFile, FALSE))
            return NULL;

        DeleteFile(pce->lpszLocalFileName);
    }
    // work around -- end

    hFile = CreateFile(lpFile,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,  // | FILE_FLAG_SEQUENTIAL_SCAN,  
            NULL);
    
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;        

    // move file pointer to end
    if (0xFFFFFFFF == SetFilePointer(hFile, 0, 0, FILE_END))
    {
        CloseHandle(hFile);
        hFile = NULL;
    }
    
    return hFile;
}

