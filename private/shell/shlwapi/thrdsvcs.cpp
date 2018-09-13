/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    thrdsvcs.cxx

Abstract:

    Contains front-end to thread pool services (Tps) functions

    Contents:
        (LoadKernelEntryPoints)
        SHRegisterWaitForSingleObject
        SHUnregisterWait
        SHQueueUserWorkItem
        SHCreateTimerQueue
        SHDeleteTimerQueue
        SHSetTimerQueueTimer
        SHChangeTimerQueueTimer
        SHCancelTimerQueueTimer

Author:

    Richard L Firth (rfirth) 27-Jan-1998

Environment:

    User-mode Win32

Notes:

    On NT5 and above, these functions just call the Rtl primitives in NTDLL.DLL.
    On NT4 and Win9x, these functions call the thread pool primitives in
    thread.cxx

Revision History:

    27-Jan-1998 rfirth
        Created

--*/

#include "priv.h"
#include "threads.h"

//
// types
//

typedef HANDLE (WINAPI * t_RegisterWaitForSingleObject)(HANDLE,
                                                        WAITORTIMERCALLBACKFUNC,
                                                        LPVOID,
                                                        DWORD
                                                        );
typedef BOOL (WINAPI * t_UnregisterWait)(HANDLE);
typedef BOOL (WINAPI * t_QueueUserWorkItem)(LPTHREAD_START_ROUTINE, LPVOID, BOOL);
typedef HANDLE (WINAPI * t_CreateTimerQueue)(VOID);
typedef BOOL (WINAPI * t_DeleteTimerQueue)(HANDLE);
typedef HANDLE (WINAPI * t_SetTimerQueueTimer)(HANDLE,
                                               WAITORTIMERCALLBACKFUNC,
                                               LPVOID,
                                               DWORD,
                                               DWORD,
                                               BOOL
                                               );
typedef BOOL (WINAPI * t_ChangeTimerQueueTimer)(HANDLE, HANDLE, DWORD, DWORD);
typedef BOOL (WINAPI * t_CancelTimerQueueTimer)(HANDLE, HANDLE);

//
// global data
//

t_RegisterWaitForSingleObject   _I_RegisterWaitForSingleObject;
t_UnregisterWait                _I_UnregisterWait;
t_QueueUserWorkItem             _I_QueueUserWorkItem;
t_CreateTimerQueue              _I_CreateTimerQueue;
t_DeleteTimerQueue              _I_DeleteTimerQueue;
t_SetTimerQueueTimer            _I_SetTimerQueueTimer;
t_ChangeTimerQueueTimer         _I_ChangeTimerQueueTimer;
t_CancelTimerQueueTimer         _I_CancelTimerQueueTimer;

BOOL g_bInitDone = FALSE;

//
// functions
//

VOID
LoadKernelEntryPoints(
    VOID
    )
{
    static BOOL bLoading = FALSE;

    if (!InterlockedExchange((LPLONG)&bLoading, TRUE)) {

        HMODULE hKernel32 = GetModuleHandle("KERNEL32.DLL");
        //BOOL bUseNtEntryPoints = TRUE;
        BOOL bUseNtEntryPoints = FALSE;

#define LOAD_ENTRY_POINT(handle, name) \
        if (bUseNtEntryPoints) { \
            _I_##name = (t_##name)GetProcAddress(handle, #name); \
            bUseNtEntryPoints &= (_I_##name != NULL); \
        }

        if (hKernel32 != NULL) {
            LOAD_ENTRY_POINT(hKernel32, RegisterWaitForSingleObject);
            LOAD_ENTRY_POINT(hKernel32, UnregisterWait);
            LOAD_ENTRY_POINT(hKernel32, QueueUserWorkItem);
            LOAD_ENTRY_POINT(hKernel32, CreateTimerQueue);
            LOAD_ENTRY_POINT(hKernel32, DeleteTimerQueue);
            LOAD_ENTRY_POINT(hKernel32, SetTimerQueueTimer);
            LOAD_ENTRY_POINT(hKernel32, ChangeTimerQueueTimer);
            LOAD_ENTRY_POINT(hKernel32, CancelTimerQueueTimer);
        } else {
            bUseNtEntryPoints = FALSE;
        }
        if (!bUseNtEntryPoints) {
            _I_RegisterWaitForSingleObject = Tps::Ie_RegisterWaitForSingleObject;
            _I_UnregisterWait = Tps::Ie_UnregisterWait;
            _I_QueueUserWorkItem = Tps::Ie_QueueUserWorkItem;
            _I_CreateTimerQueue = Tps::Ie_CreateTimerQueue;
            _I_DeleteTimerQueue = Tps::Ie_DeleteTimerQueue;
            _I_SetTimerQueueTimer = Tps::Ie_SetTimerQueueTimer;
            _I_ChangeTimerQueueTimer = Tps::Ie_ChangeTimerQueueTimer;
            _I_CancelTimerQueueTimer = Tps::Ie_CancelTimerQueueTimer;
        }
        g_bInitDone = TRUE;
    } else {
        while (!g_bInitDone) {
            SleepEx(0, FALSE);
        }
    }
}

LWSTDAPI_(HANDLE)
SHRegisterWaitForSingleObject(
    IN HANDLE hObject,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwMilliseconds
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_RegisterWaitForSingleObject(hObject,
                                          pfnCallback,
                                          pContext,
                                          dwMilliseconds
                                          );
}

LWSTDAPI_(BOOL)
SHUnregisterWait(
    IN HANDLE hWait
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_UnregisterWait(hWait);
}

LWSTDAPI_(BOOL)
SHQueueUserWorkItem(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext,
    IN BOOL bPreferIo
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_QueueUserWorkItem(pfnCallback, pContext, bPreferIo);
}

LWSTDAPI_(HANDLE)
SHCreateTimerQueue(
    VOID
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_CreateTimerQueue();
}

LWSTDAPI_(BOOL)
SHDeleteTimerQueue(
    IN HANDLE hQueue
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_DeleteTimerQueue(hQueue);
}

LWSTDAPI_(HANDLE)
SHSetTimerQueueTimer(
    IN HANDLE hQueue,
    IN WAITORTIMERCALLBACKFUNC pfnCallback,
    IN LPVOID pContext,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod,
    IN BOOL bPreferIo
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_SetTimerQueueTimer(hQueue,
                                 pfnCallback,
                                 pContext,
                                 dwDueTime,
                                 dwPeriod,
                                 bPreferIo
                                 );
}

LWSTDAPI_(BOOL)
SHChangeTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer,
    IN DWORD dwDueTime,
    IN DWORD dwPeriod
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_ChangeTimerQueueTimer(hQueue, hTimer, dwDueTime, dwPeriod);
}

LWSTDAPI_(BOOL)
SHCancelTimerQueueTimer(
    IN HANDLE hQueue,
    IN HANDLE hTimer
    )
{
    if (!g_bInitDone) {
        LoadKernelEntryPoints();
    }
    return _I_CancelTimerQueueTimer(hQueue, hTimer);
}
