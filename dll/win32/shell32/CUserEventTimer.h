/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IUserEventTimer header
 * COPYRIGHT:   Copyright 2021 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#ifndef _CUSEREVENTTIMER_H_
#define _CUSEREVENTTIMER_H_

class CUserEventTimer :
    public CComCoClass<CUserEventTimer, &CLSID_UserEventTimer>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IUserEventTimer
{
public:

    /*** IUserEventTimerCallback methods ***/
    virtual HRESULT WINAPI UserEventTimerProc(ULONG uUserEventTimerID, UINT uTimerElapse);

    /*** IUserEventTimer methods ***/
    virtual HRESULT WINAPI SetUserEventTimer(HWND hWnd, UINT uCallbackMsg, UINT uTimerElapse,
                                             IUserEventTimerCallback *pUserEventTimerCallback,
                                             ULONG *puUserEventTimerID);
    virtual HRESULT WINAPI KillUserEventTimer(HWND hWnd, ULONG uUserEventTimerID);
    virtual HRESULT WINAPI GetUserEventTimerElapsed(HWND hWnd, ULONG uUserEventTimerID, UINT *puTimerElapsed);
    virtual HRESULT WINAPI InitTimerTickInterval(UINT uTimerTickIntervalMs);


DECLARE_REGISTRY_RESOURCEID(IDR_USEREVENTTIMER)
DECLARE_NOT_AGGREGATABLE(CUserEventTimer)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUserEventTimer)
    COM_INTERFACE_ENTRY_IID(IID_IUserEventTimer, IUserEventTimer)
END_COM_MAP()
};

#endif // _CUSEREVENTTIMER_H_
