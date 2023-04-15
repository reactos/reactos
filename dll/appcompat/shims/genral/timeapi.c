/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     TimeAPI related shims
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <shimlib.h>
#include <strsafe.h>
#include <mmsystem.h>

#define SHIM_NS         DefaultTimerResolution1ms
#include <setup_shim.inl>

typedef DWORD (WINAPI* TIMEGETTIMEPROC)(void);
typedef MMRESULT(WINAPI* TIMEBEGINPERIODPROC)(_In_ UINT);
typedef MMRESULT(WINAPI* TIMEENDPERIODPROC)(_In_ UINT);

/* Only adjust timer resolution if timeGetTime() is called before timeBeginPeriod() */
static LONG isResolutionSet;

#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    HMODULE hWinMM;
    TIMEENDPERIODPROC ftimeEndPeriod;
    
    if (fdwReason == SHIM_REASON_DEINIT)
    {
        hWinMM = GetModuleHandleW(L"WINMM");
        if (hWinMM != NULL)
        {
            ftimeEndPeriod  = (TIMEENDPERIODPROC)GetProcAddress(hWinMM, "timeEndPeriod");
            if (ftimeEndPeriod != NULL && InterlockedAnd(&isResolutionSet, TRUE))
                ftimeEndPeriod(1);
        }
    }
    return TRUE;
}

DWORD WINAPI SHIM_OBJ_NAME(APIHook_timeGetTime)(void)
{
    if (InterlockedExchange(&isResolutionSet, TRUE) == FALSE)
    {
        HMODULE hWinMM = GetModuleHandleW(L"WINMM");
        TIMEBEGINPERIODPROC ftimeBeginPeriod  = (TIMEBEGINPERIODPROC)GetProcAddress(hWinMM, "timeBeginPeriod");
        
        /* Windows 95/98/ME timeGetTime has a default resolution of 1 ms and on */
        /* Windows NT/2000/XP this same function has a resolution of 10 ms. */
        ftimeBeginPeriod(1);
    }
    
    return CALL_SHIM(0, TIMEGETTIMEPROC)();
}

MMRESULT WINAPI SHIM_OBJ_NAME(APIHook_timeBeginPeriod)(_In_ UINT uPeriod)
{
    InterlockedExchange(&isResolutionSet, TRUE);
    return CALL_SHIM(0, TIMEBEGINPERIODPROC)(uPeriod);
}

#define SHIM_NUM_HOOKS  1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "WINMM.DLL", "timeGetTime", SHIM_OBJ_NAME(APIHook_timeGetTime)) \
    SHIM_HOOK(1, "WINMM.DLL", "timeBeginPeriod", SHIM_OBJ_NAME(APIHook_timeBeginPeriod))

#include <implement_shim.inl>

