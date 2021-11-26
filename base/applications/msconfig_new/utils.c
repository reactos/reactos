/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/utils.c
 * PURPOSE:     Memory Management, Resources, ... Utility Functions
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "utils.h"
#include "stringutils.h"

// HANDLE g_hHeap = GetProcessHeap();
#define g_hHeap GetProcessHeap()

#if 0
VOID
MemInit(IN HANDLE Heap)
{
    /* Save the heap handle */
    g_hHeap = Heap;
}
#endif

BOOL
MemFree(IN PVOID lpMem)
{
    /* Free memory back into the heap */
    return HeapFree(g_hHeap, 0, lpMem);
}

PVOID
MemAlloc(IN DWORD dwFlags,
         IN SIZE_T dwBytes)
{
    /* Allocate memory from the heap */
    return HeapAlloc(g_hHeap, dwFlags, dwBytes);
}

LPWSTR
FormatDateTime(IN LPSYSTEMTIME pDateTime)
{
    LPWSTR lpszDateTime = NULL;
    int iDateBufSize, iTimeBufSize;

    if (pDateTime == NULL) return NULL;

    iDateBufSize = GetDateFormatW(LOCALE_USER_DEFAULT,
                                  /* Only for Windows 7 : DATE_AUTOLAYOUT | */ DATE_SHORTDATE,
                                  pDateTime,
                                  NULL,
                                  NULL,
                                  0);
    iTimeBufSize = GetTimeFormatW(LOCALE_USER_DEFAULT,
                                  0,
                                  pDateTime,
                                  NULL,
                                  NULL,
                                  0);

    if ( (iDateBufSize > 0) && (iTimeBufSize > 0) )
    {
        lpszDateTime = (LPWSTR)MemAlloc(0, (iDateBufSize + iTimeBufSize) * sizeof(WCHAR));

        GetDateFormatW(LOCALE_USER_DEFAULT,
                       /* Only for Windows 7 : DATE_AUTOLAYOUT | */ DATE_SHORTDATE,
                       pDateTime,
                       NULL,
                       lpszDateTime,
                       iDateBufSize);
        if (iDateBufSize > 0) lpszDateTime[iDateBufSize-1] = L' ';
        GetTimeFormatW(LOCALE_USER_DEFAULT,
                       0,
                       pDateTime,
                       NULL,
                       lpszDateTime + iDateBufSize,
                       iTimeBufSize);
    }

    return lpszDateTime;
}

VOID
FreeDateTime(IN LPWSTR lpszDateTime)
{
    if (lpszDateTime)
        MemFree(lpszDateTime);
}

LPWSTR
LoadResourceStringEx(IN HINSTANCE hInstance,
                     IN UINT uID,
                     OUT size_t* pSize OPTIONAL)
{
    LPWSTR lpszDestBuf = NULL, lpszResourceString = NULL;
    size_t iStrSize    = 0;

    // When passing a zero-length buffer size, LoadString(...) returns a
    // read-only pointer buffer to the program's resource string.
    iStrSize = LoadStringW(hInstance, uID, (LPWSTR)&lpszResourceString, 0);

    if ( lpszResourceString && ( (lpszDestBuf = (LPWSTR)MemAlloc(0, (iStrSize + 1) * sizeof(WCHAR))) != NULL ) )
    {
        wcsncpy(lpszDestBuf, lpszResourceString, iStrSize);
        lpszDestBuf[iStrSize] = L'\0'; // NULL-terminate the string

        if (pSize)
            *pSize = iStrSize + 1;
    }
    else
    {
        if (pSize)
            *pSize = 0;
    }

    return lpszDestBuf;
}

LPWSTR
LoadConditionalResourceStringEx(IN HINSTANCE hInstance,
                                IN BOOL bCondition,
                                IN UINT uIDifTrue,
                                IN UINT uIDifFalse,
                                IN size_t* pSize OPTIONAL)
{
    return LoadResourceStringEx(hInstance,
                                (bCondition ? uIDifTrue : uIDifFalse),
                                pSize);
}

DWORD
RunCommand(IN LPCWSTR lpszCommand,
           IN LPCWSTR lpszParameters,
           IN INT nShowCmd)
{
    DWORD dwRes;

    DWORD dwNumOfChars;
    LPWSTR lpszExpandedCommand;

    dwNumOfChars = ExpandEnvironmentStringsW(lpszCommand, NULL, 0);
    lpszExpandedCommand = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszCommand, lpszExpandedCommand, dwNumOfChars);

    dwRes = (DWORD_PTR)ShellExecuteW(NULL,
                                     NULL /* and not L"open" !! */,
                                     lpszExpandedCommand,
                                     lpszParameters,
                                     NULL,
                                     nShowCmd);
    MemFree(lpszExpandedCommand);

    return dwRes;
}


////////////////////  The following comes from MSDN samples  ///////////////////
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd162826(v=vs.85).aspx
//

//
//  ClipOrCenterRectToMonitor
//
//  The most common problem apps have when running on a
//  multimonitor system is that they "clip" or "pin" windows
//  based on the SM_CXSCREEN and SM_CYSCREEN system metrics.
//  Because of app compatibility reasons these system metrics
//  return the size of the primary monitor.
//
//  This shows how you use the multi-monitor functions
//  to do the same thing.
//
VOID ClipOrCenterRectToMonitor(LPRECT prc, UINT flags)
{
    HMONITOR    hMonitor;
    MONITORINFO mi;
    RECT        rc;
    int         w = prc->right  - prc->left;
    int         h = prc->bottom - prc->top;

    //
    // Get the nearest monitor to the passed rect.
    //
    hMonitor = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);

    //
    // Get the work area or entire monitor rect.
    //
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    if (flags & MONITOR_WORKAREA)
        rc = mi.rcWork;
    else
        rc = mi.rcMonitor;

    //
    // Center or clip the passed rect to the monitor rect.
    //
    if (flags & MONITOR_CENTER)
    {
        prc->left   = rc.left + (rc.right  - rc.left - w) / 2;
        prc->top    = rc.top  + (rc.bottom - rc.top  - h) / 2;
        prc->right  = prc->left + w;
        prc->bottom = prc->top  + h;
    }
    else
    {
        prc->left   = max(rc.left, min(rc.right-w,  prc->left));
        prc->top    = max(rc.top,  min(rc.bottom-h, prc->top));
        prc->right  = prc->left + w;
        prc->bottom = prc->top  + h;
    }
}

VOID ClipOrCenterWindowToMonitor(HWND hWnd, UINT flags)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);
    ClipOrCenterRectToMonitor(&rc, flags);
    SetWindowPos(hWnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
////////////////////////////////////////////////////////////////////////////////


BOOL IsWindowsOS(VOID)
{
    BOOL bIsWindowsOS = FALSE;

    OSVERSIONINFOW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionExW(&osvi))
        return FALSE;

    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
        return FALSE;

    /* ReactOS reports as Windows NT 5.2 */

    if ( (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 2) ||
         (osvi.dwMajorVersion  > 5) )
    {
        HKEY hKey = NULL;

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            LONG ret;
            DWORD dwType = 0, dwBufSize = 0;

            ret = RegQueryValueExW(hKey, L"ProductName", NULL, &dwType, NULL, &dwBufSize);
            if (ret == ERROR_SUCCESS && dwType == REG_SZ)
            {
                LPTSTR lpszProductName = (LPTSTR)MemAlloc(0, dwBufSize);
                RegQueryValueExW(hKey, L"ProductName", NULL, &dwType, (LPBYTE)lpszProductName, &dwBufSize);

                bIsWindowsOS = (FindSubStrI(lpszProductName, _T("Windows")) != NULL);

                MemFree(lpszProductName);
            }

            RegCloseKey(hKey);
        }
    }
    else
    {
        bIsWindowsOS = TRUE;
    }

    return bIsWindowsOS;
}

BOOL IsPreVistaOSVersion(VOID)
{
    OSVERSIONINFOW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionExW(&osvi))
        return FALSE;

    /* Vista+-class OSes are NT >= 6 */
    return ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? (osvi.dwMajorVersion < 6) : FALSE );
}

LPWSTR
GetExecutableVendor(IN LPCWSTR lpszFilename)
{
    LPWSTR lpszVendor = NULL;
    DWORD dwHandle = 0;
    DWORD dwLen;

    LPVOID lpData;

    LPVOID pvData = NULL;
    UINT BufLen = 0;
    WORD wCodePage = 0, wLangID = 0;
    LPWSTR lpszStrFileInfo = NULL;

    LPWSTR lpszData = NULL;

    if (lpszFilename == NULL) return NULL;

    dwLen = GetFileVersionInfoSizeW(lpszFilename, &dwHandle);
    if (dwLen == 0) return NULL;

    lpData = MemAlloc(0, dwLen);
    if (!lpData) return NULL;

    GetFileVersionInfoW(lpszFilename, dwHandle, dwLen, lpData);

    if (VerQueryValueW(lpData, L"\\VarFileInfo\\Translation", &pvData, &BufLen))
    {
        wCodePage = LOWORD(*(DWORD*)pvData);
        wLangID   = HIWORD(*(DWORD*)pvData);

        lpszStrFileInfo = FormatString(L"StringFileInfo\\%04X%04X\\CompanyName",
                                       wCodePage,
                                       wLangID);
    }

    VerQueryValueW(lpData, lpszStrFileInfo, (LPVOID*)&lpszData, &BufLen);
    if (lpszData)
    {
        lpszVendor = (LPWSTR)MemAlloc(0, BufLen * sizeof(WCHAR));
        if (lpszVendor)
            wcscpy(lpszVendor, lpszData);
    }
    else
    {
        lpszVendor = NULL;
    }

    MemFree(lpszStrFileInfo);
    MemFree(lpData);

    return lpszVendor;
}
