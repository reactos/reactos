/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IUserEventTimer implementation
 * COPYRIGHT:   Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IUserEventTimerCallback implementation
*/

HRESULT WINAPI CUserEventTimer::UserEventTimerProc(ULONG uUserEventTimerID, UINT uTimerElapse)
{
    UNIMPLEMENTED;
    return S_OK;
}

/***********************************************************************
*   IUserEventTimer implementation
*/

HRESULT WINAPI CUserEventTimer::SetUserEventTimer(HWND hWnd, UINT uCallbackMsg, UINT uTimerElapse,
                                                  IUserEventTimerCallback *pUserEventTimerCallback,
                                                  ULONG *puUserEventTimerID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT WINAPI CUserEventTimer::KillUserEventTimer(HWND hWnd, ULONG uUserEventTimerID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT WINAPI CUserEventTimer::GetUserEventTimerElapsed(HWND hWnd, ULONG uUserEventTimerID, UINT *puTimerElapsed)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT WINAPI CUserEventTimer::InitTimerTickInterval(UINT uTimerTickIntervalMs)
{
    UNIMPLEMENTED;
    return S_OK;
}

