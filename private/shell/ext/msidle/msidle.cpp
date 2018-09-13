//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       msidle.cpp
//
//  Contents:   user idle detection
//
//  Classes:
//
//  Functions:
//
//  History:    05-14-1997  darrenmi (Darren Mitchell) Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include "msidle.h"
#include "resource.h"

// useful things...
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

//
// Global unshared variables
//
DWORD   g_dwIdleMin = 0;                // inactivity minutes before idle
UINT_PTR g_uIdleTimer = 0;              // idle timer for this process
BOOL    g_fIdleNotify = FALSE;          // notify when idle
BOOL    g_fBusyNotify = FALSE;          // notify when busy
BOOL    g_fIsWinNT = FALSE;             // which platform?
BOOL    g_fIsWinNT5 = FALSE;            // are we running on NT5?
HANDLE  g_hSageVxd = INVALID_HANDLE_VALUE;
                                        // handle to sage.vxd
DWORD   g_dwIdleBeginTicks = 0;         // ticks when we became idle
HINSTANCE g_hInst = NULL;               // dll instance
_IDLECALLBACK g_pfnCallback = NULL;     // function to call back in client

//
// Global shared variables
//
#pragma data_seg(".shrdata")

HHOOK   sg_hKbdHook = NULL, sg_hMouseHook = NULL;
DWORD   sg_dwLastTickCount = 0;
POINT   sg_pt = {0,0};

#pragma data_seg()

//
// Prototypes
//
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KbdProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK OnIdleTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

//
// From winuser.h, but NT5 only
//
#if (_WIN32_WINNT < 0x0500)
typedef struct tagLASTINPUTINFO {
    UINT cbSize;
    DWORD dwTime;
} LASTINPUTINFO, * PLASTINPUTINFO;
#endif

//
// NT5 api we dynaload from user32
//
typedef WINUSERAPI BOOL (WINAPI* PFNGETLASTINPUTINFO)(PLASTINPUTINFO plii);

PFNGETLASTINPUTINFO pfnGetLastInputInfo = NULL;

///////////////////////////////////////////////////////////////////////////
//
//                     Internal functions
//
///////////////////////////////////////////////////////////////////////////

#ifdef DEBUG

BOOL ReadRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue, 
                   void *pData, DWORD dwBytes)
{
    long    lResult;
    HKEY    hkey;
    DWORD   dwType;

    lResult = RegOpenKey(hkeyRoot, pszKey, &hkey);
    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }

    lResult = RegQueryValueEx(hkey, pszValue, NULL, &dwType, (BYTE *)pData, 
        &dwBytes);
    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS) 
        return FALSE;
    
    if(dwType == REG_SZ) {
        // null terminate string
        ((TCHAR *)pData)[dwBytes] = 0;
    }

    return TRUE;
}

TCHAR *g_pszLoggingFile;
BOOL  g_fCheckedForLog = FALSE;

DWORD LogEvent(LPTSTR pszFormat, ...)
{

    // check registry if necessary
    if(FALSE == g_fCheckedForLog) {

        TCHAR   pszFilePath[MAX_PATH];

        if(ReadRegValue(HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\msidle"),
                TEXT("LoggingFile"), pszFilePath, MAX_PATH)) {

            g_pszLoggingFile = (TCHAR *)LocalAlloc(LPTR, lstrlen(pszFilePath) + 1);
            if(g_pszLoggingFile) {
                lstrcpy(g_pszLoggingFile, pszFilePath);
            }
        }

        g_fCheckedForLog = TRUE;
    }

    if(g_pszLoggingFile) {

        TCHAR       pszString[1024];
        SYSTEMTIME  st;
        HANDLE      hLog;
        DWORD       dwWritten;
        va_list     va;

        hLog = CreateFile(g_pszLoggingFile, GENERIC_WRITE, 0, NULL,
                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if(INVALID_HANDLE_VALUE == hLog)
            return GetLastError();

        // seek to end of file
        SetFilePointer(hLog, 0, 0, FILE_END);

        // dump time
        GetLocalTime(&st);
        wsprintf(pszString, "%02d:%02d:%02d [%x] - ", st.wHour, st.wMinute, st.wSecond, GetCurrentThreadId());
        WriteFile(hLog, pszString, lstrlen(pszString), &dwWritten, NULL);
        OutputDebugString(pszString);

        // dump passed in string
        va_start(va, pszFormat);
        wvsprintf(pszString, pszFormat, va);
        va_end(va);
        WriteFile(hLog, pszString, lstrlen(pszString), &dwWritten, NULL);
        OutputDebugString(pszString);

        // cr
        WriteFile(hLog, "\r\n", 2, &dwWritten, NULL);
        OutputDebugString("\r\n");

        // clean up
        CloseHandle(hLog);
    }

    return 0;
}

#endif // DEBUG

//
// SetIdleTimer - decide how often to poll and set the timer appropriately
//
void SetIdleTimer(void)
{
    UINT uInterval = 1000 * 60;

    //
    // If we're looking for loss of idle, check every 4 seconds
    //
    if(TRUE == g_fBusyNotify) {
        uInterval = 1000 * 4;
    }

    //
    // kill off the old timer
    //
    if(g_uIdleTimer) {
        KillTimer(NULL, g_uIdleTimer);
    }

    //
    // Set the timer
    //
    g_uIdleTimer = SetTimer(NULL, 0, uInterval, (TIMERPROC)OnIdleTimer);
}
       
DWORD GetLastActivityTicks(void)
{
    DWORD dwLastActivityTicks;

    if(g_fIsWinNT5 && pfnGetLastInputInfo) {
        // NT5: Use get last input time API
        LASTINPUTINFO lii;

        memset(&lii, 0, sizeof(lii));
        lii.cbSize = sizeof(lii);
        (*pfnGetLastInputInfo)(&lii);
        dwLastActivityTicks = lii.dwTime;
    } else {
        // NT4 or Win95: Use sage if it's loaded
        if(INVALID_HANDLE_VALUE != g_hSageVxd) {
            // query sage.vxd for tick count
            DeviceIoControl(g_hSageVxd, 2, &dwLastActivityTicks, sizeof(DWORD),
                NULL, 0, NULL, NULL);
        } else {
            // use hooks
            dwLastActivityTicks = sg_dwLastTickCount;
        }
    }

    return dwLastActivityTicks;
}

//
// OnIdleTimer - idle timer has gone off
//
VOID CALLBACK OnIdleTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    DWORD   dwDiff, dwLastActivityTicks;
    BOOL    fTempBusyNotify = g_fBusyNotify;
    BOOL    fTempIdleNotify = g_fIdleNotify;

    //
    // get last activity ticks from sage or shared segment
    //
    dwLastActivityTicks = GetLastActivityTicks();

#ifdef DEBUG
    LogEvent("OnIdleTimer: dwLastActivity=%d, CurrentTicks=%d, dwIdleBegin=%d", dwLastActivityTicks, GetTickCount(), g_dwIdleBeginTicks);
#endif

    //
    // check to see if we've changed state
    //
    if(fTempBusyNotify) {
        //
        // Want to know if we become busy
        //
        if(dwLastActivityTicks != g_dwIdleBeginTicks) {
            // activity since we became idle - stop being idle!
            g_fBusyNotify = FALSE;
            g_fIdleNotify = TRUE;

            // set the timer
            SetIdleTimer();

            // call back client
#ifdef DEBUG
            LogEvent("OnIdleTimer: Idle Ends");
#endif
            if(g_pfnCallback)
                (g_pfnCallback)(STATE_USER_IDLE_END);
        }

    }

    if(fTempIdleNotify) {
        //
        // Want to know if we become idle
        //
        dwDiff = GetTickCount() - dwLastActivityTicks;

        if(dwDiff > 1000 * 60 * g_dwIdleMin) {
            // Nothing's happened for our threshold time.  We're now idle.
            g_fIdleNotify = FALSE;
            g_fBusyNotify = TRUE;

            // save time we became idle
            g_dwIdleBeginTicks = dwLastActivityTicks;

            // set the timer
            SetIdleTimer();

            // call back client
#ifdef DEBUG
            LogEvent("OnIdleTimer: Idle Begins");
#endif
            if(g_pfnCallback)
                (g_pfnCallback)(STATE_USER_IDLE_BEGIN);
        }
    }
}

BOOL LoadSageVxd(void)
{
    int inpVXD[3];

    if(INVALID_HANDLE_VALUE != g_hSageVxd)
        return TRUE;

    g_hSageVxd = CreateFile("\\\\.\\sage.vxd", 0, 0, NULL, 0,
            FILE_FLAG_DELETE_ON_CLOSE, NULL);

    // can't open it?  can't use it
    if(INVALID_HANDLE_VALUE == g_hSageVxd)
        return FALSE;

    // start it monitoring
    inpVXD[0] = -1;                         // no window - will query
    inpVXD[1] = 0;                          // unused
    inpVXD[2] = 0;                          // how long to wait between checks

    DeviceIoControl(g_hSageVxd, 1, &inpVXD, sizeof(inpVXD), NULL, 0, NULL, NULL);

    return TRUE;
}

BOOL UnloadSageVxd(void)
{
    if(INVALID_HANDLE_VALUE != g_hSageVxd) {
        CloseHandle(g_hSageVxd);
        g_hSageVxd = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//
//                   Externally callable functions
//
///////////////////////////////////////////////////////////////////////////

//
// LibMain - dll entry point
//
EXTERN_C BOOL WINAPI LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvRes)
{
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        {
        OSVERSIONINFO vi;

        DisableThreadLibraryCalls(hInst);
        g_hInst = hInst;

        vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&vi);
        if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            g_fIsWinNT = TRUE;
            if(vi.dwMajorVersion >= 5)
                g_fIsWinNT5 = TRUE;
        }
        }
        break;
    }

    return TRUE;
}

//
// BeginIdleDetection
//
DWORD BeginIdleDetection(_IDLECALLBACK pfnCallback, DWORD dwIdleMin, DWORD dwReserved)
{
    DWORD dwValue = 0;

    // make sure reserved is 0
    if(dwReserved)
        return ERROR_INVALID_DATA;

#ifdef DEBUG
    LogEvent("BeginIdleDetection: IdleMin=%d", dwIdleMin);
#endif

    // save callback
    g_pfnCallback = pfnCallback;

    // save minutes
    g_dwIdleMin = dwIdleMin;

    // call back on idle
    g_fIdleNotify = TRUE;

    if(FALSE == g_fIsWinNT) {
        // try to load sage.vxd
        LoadSageVxd();
    }

    if(g_fIsWinNT5) {
        // we need to find our NT5 api in user
        HINSTANCE hUser = GetModuleHandle("user32.dll");
        if(hUser) {
            pfnGetLastInputInfo =
                (PFNGETLASTINPUTINFO)GetProcAddress(hUser, "GetLastInputInfo");
        }

        if(NULL == pfnGetLastInputInfo) {
            // not on NT5 - bizarre
            g_fIsWinNT5 = FALSE;
        }
    }

    if(INVALID_HANDLE_VALUE == g_hSageVxd && FALSE == g_fIsWinNT5) {

        // sage vxd not available - do it the hard way

        // hook kbd
        sg_hKbdHook = SetWindowsHookEx(WH_KEYBOARD, KbdProc, g_hInst, 0);
        if(NULL == sg_hKbdHook)
            return GetLastError();
        
        // hook mouse
        sg_hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, g_hInst, 0);
        if(NULL == sg_hMouseHook) {
            DWORD dwError = GetLastError();
            EndIdleDetection(0);
            return dwError;
        }
    }

    // Fire up the timer
    SetIdleTimer();

    return 0;
}

//
// IdleEnd - stop idle monitoring
//
BOOL EndIdleDetection(DWORD dwReserved)
{
    // ensure reserved is 0
    if(dwReserved)
        return FALSE;

    // free up sage if we're using it
    UnloadSageVxd();

    // kill timer
    if(g_uIdleTimer) {
        KillTimer(NULL, g_uIdleTimer);
        g_uIdleTimer = 0;
    }

    // callback is no longer valid
    g_pfnCallback = NULL;

    // free up hooks
    if(sg_hKbdHook) {
        UnhookWindowsHookEx(sg_hKbdHook);
        sg_hKbdHook = NULL;
    }

    if(sg_hMouseHook) {
        UnhookWindowsHookEx(sg_hMouseHook);
        sg_hMouseHook = NULL;
    }

    return TRUE;
}

//
// SetIdleMinutes - set the timout value and reset idle flag to false
//
// dwMinutes   - if non-0, set idle timeout to that many minutes
// fIdleNotify - call back when idle for at least idle minutes
// fBusyNotify - call back on activity since Idle begin
//
BOOL SetIdleTimeout(DWORD dwMinutes, DWORD dwReserved)
{
    if(dwReserved)
        return FALSE;

#ifdef DEBUG
    LogEvent("SetIdleTimeout: dwIdleMin=%d", dwMinutes);
#endif

    if(dwMinutes)
        g_dwIdleMin = dwMinutes;

    return TRUE;
}

//
// SetIdleNotify - set flag to turn on or off idle notifications
//
// fNotify - flag
// dwReserved - must be 0
//
void SetIdleNotify(BOOL fNotify, DWORD dwReserved)
{
#ifdef DEBUG
    LogEvent("SetIdleNotify: fNotify=%d", fNotify);
#endif

    g_fIdleNotify = fNotify;
}

//
// SetIdleNotify - set flag to turn on or off idle notifications
//
// fNotify - flag
// dwReserved - must be 0
//
void SetBusyNotify(BOOL fNotify, DWORD dwReserved)
{
#ifdef DEBUG
    LogEvent("SetBusyNotify: fNotify=%d", fNotify);
#endif

    g_fBusyNotify = fNotify;

    if(g_fBusyNotify)
        g_dwIdleBeginTicks = GetLastActivityTicks();

    // set the timer
    SetIdleTimer();
}

//
// GetIdleMinutes - return how many minutes since last user activity
//
DWORD GetIdleMinutes(DWORD dwReserved)
{
    if(dwReserved)
        return 0;

    return (GetTickCount() - GetLastActivityTicks()) / 60000;
}

///////////////////////////////////////////////////////////////////////////
//
//                           Hook functions
//
///////////////////////////////////////////////////////////////////////////

//
// Note: These functions can be called back in any process!
//
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MOUSEHOOKSTRUCT * pmsh = (MOUSEHOOKSTRUCT *)lParam;

    if(nCode >= 0) {
        // ignore mouse move messages to the same point as all window
        // creations cause these - it doesn't mean the user moved the mouse
        if(WM_MOUSEMOVE != wParam || pmsh->pt.x != sg_pt.x || pmsh->pt.y != sg_pt.y) {
            sg_dwLastTickCount = GetTickCount();
            sg_pt = pmsh->pt;
        }
    }

    return(CallNextHookEx(sg_hMouseHook, nCode, wParam, lParam));
}

LRESULT CALLBACK KbdProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode >= 0) {
        sg_dwLastTickCount = GetTickCount();
    }

    return(CallNextHookEx(sg_hKbdHook, nCode, wParam, lParam));
}
