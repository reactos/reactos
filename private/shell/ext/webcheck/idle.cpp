//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       idle.cpp
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

#include "private.h"
#include "throttle.h"

typedef void (WINAPI* _IDLECALLBACK) (DWORD dwState);
typedef DWORD (WINAPI* _BEGINIDLEDETECTION) (_IDLECALLBACK, DWORD, DWORD);
typedef BOOL (WINAPI* _ENDIDLEDETECTION) (DWORD);

#define TF_THISMODULE TF_WEBCHECKCORE

HINSTANCE           g_hinstMSIDLE = NULL;
_BEGINIDLEDETECTION g_pfnBegin = NULL;
_ENDIDLEDETECTION   g_pfnEnd = NULL;

//
// extra stuff so we don't need msidle.dll on win95
//
BOOL    g_fWin95PerfWin = FALSE;            // using msidle.dll or not?
UINT_PTR g_uIdleTimer = 0;                  // timer handle if not
HANDLE  g_hSageVxd = INVALID_HANDLE_VALUE;  // vxd handle if not
DWORD   g_dwIdleMin = 3;                    // inactivity mins before idle
BOOL    g_fIdle = FALSE;                    // are we idle?
DWORD   g_dwIdleBeginTicks = 0;             // when did idle begin?

VOID CALLBACK OnIdleTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

//
// A little code copied from msidle.dll.  We use this on Win95 with sage.vxd
// so we don't have to load msidle.dll. 
//

//
// SetIdleTimer - decide how often to poll and set the timer appropriately
//
void SetIdleTimer(void)
{
    UINT uInterval = 1000 * 60;

    //
    // If we're idle and looking for busy, check every 2 seconds
    //
    if(g_fIdle) {
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
    TraceMsg(TF_THISMODULE,"SetIdleTimer uInterval=%d", uInterval);
    g_uIdleTimer = SetTimer(NULL, 0, uInterval, (TIMERPROC)OnIdleTimer);
}
       
//
// OnIdleTimer - idle timer has gone off
//
VOID CALLBACK OnIdleTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    DWORD dwDiff, dwLastActivityTicks;

    //
    // get last activity ticks from sage
    //
    DeviceIoControl(g_hSageVxd, 2, &dwLastActivityTicks, sizeof(DWORD),
        NULL, 0, NULL, NULL);

    //
    // check to see if we've changed state
    //
    if(g_fIdle) {
        //
        // currently in idle state
        //
        if(dwLastActivityTicks != g_dwIdleBeginTicks) {
            // activity since we became idle - stop being idle!
            g_fIdle = FALSE;

            // set timer
            SetIdleTimer();

            // call back client
            CThrottler::OnIdleStateChange(STATE_USER_IDLE_END);
        }

    } else {
        //
        // currently not in idle state
        //
        dwDiff = GetTickCount() - dwLastActivityTicks;

        if(dwDiff > 1000 * 60 * g_dwIdleMin) {
            // Nothing's happened for our threshold time.  We're now idle.
            g_fIdle = TRUE;

            // save time we became idle
            g_dwIdleBeginTicks = dwLastActivityTicks;

            // set timer
            SetIdleTimer();

            // call back client
            CThrottler::OnIdleStateChange(STATE_USER_IDLE_BEGIN);
        }
    }
}

BOOL LoadSageVxd(void)
{
    int inpVXD[3];

    if(INVALID_HANDLE_VALUE != g_hSageVxd)
        return TRUE;

    g_hSageVxd = CreateFile(TEXT("\\\\.\\sage.vxd"), 0, 0, NULL, 0,
            FILE_FLAG_DELETE_ON_CLOSE, NULL);

    // can't open it?  can't use it
    if(INVALID_HANDLE_VALUE == g_hSageVxd)
        return FALSE;

    // start it monitoring
    inpVXD[0] = -1;         // no window - will query
    inpVXD[1] = 0;          // unused
    inpVXD[2] = 0;          // post delay - not used without a window

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

void IdleBegin(HWND hwndParent)
{
    DWORD dwValue;

    // Override idle minutes with reg value if present
    if(ReadRegValue(HKEY_CURRENT_USER,
            c_szRegKey,
            TEXT("IdleMinutes"),
            &dwValue,
            sizeof(DWORD)) &&
        dwValue) {

        g_dwIdleMin = dwValue;
    }

    if(FALSE == g_fIsWinNT && LoadSageVxd()) {
        // using optimal win95 configuration
        g_fWin95PerfWin = TRUE;
        SetIdleTimer();
        return;
    }

    // Bail out if the DebuggerFriendly registry value is set on NT4.
    OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&vi);

    if(   vi.dwPlatformId == VER_PLATFORM_WIN32_NT
       && vi.dwMajorVersion == 4
       && ReadRegValue(HKEY_CURRENT_USER, c_szRegKey, TEXT("DebuggerFriendly"), &dwValue, sizeof(DWORD))
       && dwValue)
    {
        return;
    }


    // load msidle.dll
    g_hinstMSIDLE = LoadLibrary(TEXT("msidle.dll"));

    // get begin and end functions
    if(g_hinstMSIDLE) {
        g_pfnBegin = (_BEGINIDLEDETECTION)GetProcAddress(g_hinstMSIDLE, (LPSTR)3);
        g_pfnEnd = (_ENDIDLEDETECTION)GetProcAddress(g_hinstMSIDLE, (LPSTR)4);

        // call start monitoring
        if(g_pfnBegin)
            (g_pfnBegin)(CThrottler::OnIdleStateChange, g_dwIdleMin, 0);
    }
}

void IdleEnd(void)
{
    if(g_fWin95PerfWin) {
        // clean up timer
        KillTimer(NULL, g_uIdleTimer);
        UnloadSageVxd();
    } else {
        // clean up msidle.dll
        if(g_pfnEnd) {
            (g_pfnEnd)(0);
            FreeLibrary(g_hinstMSIDLE);
            g_hinstMSIDLE = NULL;
            g_pfnBegin = NULL;
            g_pfnEnd = NULL;
        }
    }
}
