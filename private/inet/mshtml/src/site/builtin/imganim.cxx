//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       imganim.cxx
//
//  Contents:   Implementation of CImgAnim and CAnimSync classes
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"
#endif

DeclareTag(tagImgAnim, "CImgAnim", "Trace anim");

MtDefine(CAnimSync, PerThread, "CAnimSync")
MtDefine(CAnimSync_aryClients_pv, CAnimSync, "CAnimSync::_aryClients::_pv")
MtDefine(CImgAnim, PerThread, "CImgAnim")
MtDefine(CImgAnim_aryAnimSync_pv, CImgAnim, "CImgAnim::_aryAnimSync::_pv")

BOOL
CAnimSync::IsEmpty()
{
    return _aryClients.Size() == 0;
}

CImgCtx *
CAnimSync::GetImgCtx()
{
    Assert(!IsEmpty());

    if (!IsEmpty())
    {
        CLIENT * pClient = &_aryClients[0];
        CImgCtx * pImgCtx;

        pClient->pfnCallback(pClient->pvObj, ANIMSYNC_GETIMGCTX,
                             pClient->pvArg, (void **)&pImgCtx, NULL);

        return pImgCtx;
    }
    else
        return NULL;
}

HRESULT
CAnimSync::Register(void * pvObj, DWORD_PTR dwDocId, DWORD_PTR dwImgId,
                    CAnimSync::ASCALLBACK pfnCallback, void * pvArg)
{
    CLIENT client;

    if (IsEmpty())
    {
        CImgCtx * pImgCtx;

        _dwDocId = dwDocId;
        _dwImgId = dwImgId;
        _state = ANIMSTATE_PLAY;

        pfnCallback(pvObj, ANIMSYNC_GETIMGCTX, pvArg, (void **)&pImgCtx, NULL);

        pImgCtx->InitImgAnimState(&_imgAnimState);
    }

    client.pvObj       = pvObj;
    client.pfnCallback = pfnCallback;
    client.pvArg       = pvArg;

    RRETURN(_aryClients.AppendIndirect(&client));
}

void
CAnimSync::Unregister(void * pvObj)
{
    int cClients = _aryClients.Size(), iClient = 0;

    for ( ; iClient < cClients; iClient++)
    {
        if (_aryClients[iClient].pvObj == pvObj)
        {
            _aryClients.Delete(iClient);
            return;
        }
    }

    AssertSz(FALSE, "CAnimSync: Could not unregister object");
}

void
CAnimSync::OnTimer(DWORD *pdwFrameTimeMS)
{
    BOOL fInvalidated;
    CImgCtx * pImgCtx = GetImgCtx();

    _fInvalidated = FALSE;

    if (pImgCtx)
    {
        if (pImgCtx->NextFrame(&_imgAnimState, GetTickCount(), pdwFrameTimeMS))
        {
            int       cClients = _aryClients.Size();
            CLIENT *  pClient  = _aryClients;

            for ( ; cClients > 0; cClients--, pClient++ )
            {
                pClient->pfnCallback(pClient->pvObj, ANIMSYNC_TIMER,
                                     pClient->pvArg, (void **) &fInvalidated,
                                     &_imgAnimState);

                if (fInvalidated)
                    _fInvalidated = TRUE;

                TraceTag((tagImgAnim, "DrawFrame\n"));
            }

            if (_imgAnimState.fStop)
            {
                _state = ANIMSTATE_STOP;
                _imgAnimState.fStop = FALSE;
            }
        }
    }
    else
        *pdwFrameTimeMS = 0xFFFFFFFF;
}

void
CAnimSync::Invalidate()
{
    int       cClients = _aryClients.Size();
    CLIENT *  pClient  = _aryClients;
    BOOL      fInvalidated;

    _fInvalidated = FALSE;

    for ( ; cClients > 0; cClients--, pClient++ )
    {
        pClient->pfnCallback(pClient->pvObj, ANIMSYNC_INVALIDATE,
                             pClient->pvArg, (void **) &fInvalidated, NULL);

        if (fInvalidated)
            _fInvalidated = TRUE;
    }
}

void
CAnimSync::Update(HWND *pHwnd)
{
    int       cClients = _aryClients.Size();
    CLIENT *  pClient  = _aryClients;
    HWND      hwnd     = NULL;
    HWND      hwndPrev = *pHwnd;

    for ( ; cClients > 0; cClients--, pClient++ )
    {
        pClient->pfnCallback(pClient->pvObj, ANIMSYNC_GETHWND,
                             pClient->pvArg, (void **) &hwnd, NULL);

        if (hwnd && (hwnd != hwndPrev))
        {
            UpdateWindow(hwnd);
            hwndPrev = hwnd;
        }
    }

    if (hwnd)
        *pHwnd = hwnd;
}


VOID CALLBACK ImgAnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    Assert(TLS(pImgAnim));

    if (TLS(pImgAnim))
        TLS(pImgAnim)->OnTimer();
}

CImgAnim * GetImgAnim()
{
    return TLS(pImgAnim);
}

CImgAnim * CreateImgAnim()
{
    if (TLS(pImgAnim) == NULL)
    {
        TLS(pImgAnim) = new CImgAnim();
    }

    return TLS(pImgAnim);
}

void DeinitImgAnim(THREADSTATE *pts)
{
    delete pts->pImgAnim;
    pts->pImgAnim = NULL;
}

CImgAnim::CImgAnim()
    : _aryAnimSync(Mt(CImgAnim_aryAnimSync_pv))
{
    _dwInterval = 0xFFFFFFFF;
}

CImgAnim::~CImgAnim()
{
    KillTimer(TLS(gwnd.hwndGlobalWindow), TIMER_IMG_ANIM);
    Assert(_aryAnimSync.Size() == 0);
}

void
CImgAnim::SetInterval(DWORD dwInterval)
{
    _dwInterval = dwInterval;

    if (dwInterval == 0xFFFFFFFF)
    {
        KillTimer(TLS(gwnd.hwndGlobalWindow), TIMER_IMG_ANIM);
    }
    else
    {
        // Windows NT rounds the time up to 10.  If time is less
        // than 10, NT spews to the debugger.  Work around
        // this problem by rounding up to 10.

        if (dwInterval < 10)
            dwInterval = 10;

        SetTimer(TLS(gwnd.hwndGlobalWindow), TIMER_IMG_ANIM, dwInterval, &ImgAnimTimerProc);
    }
}

void
CImgAnim::SetAnimState(DWORD_PTR dwDocId, ANIMSTATE state)
{
    int cAnimSync;
    CAnimSync **ppAnimSync;
    BOOL fPlay = FALSE;

    for (cAnimSync = _aryAnimSync.Size(), ppAnimSync = _aryAnimSync;
         cAnimSync > 0;
         cAnimSync--, ppAnimSync++)
    {
        if (    *ppAnimSync
            && (*ppAnimSync)->_dwDocId == dwDocId
            && (*ppAnimSync)->_state != ANIMSTATE_STOP)
        {
            (*ppAnimSync)->_state = state;
            if (state == ANIMSTATE_PLAY)
                fPlay = TRUE;
        }
    }
    if (fPlay)
        OnTimer();
}

CAnimSync *
CImgAnim::GetAnimSync(LONG lCookie)
{
    Assert(lCookie > 0 && lCookie <= _aryAnimSync.Size());
    Assert(_aryAnimSync[lCookie - 1]);

    return _aryAnimSync[lCookie - 1];
}

IMGANIMSTATE *
CImgAnim::GetImgAnimState(LONG lCookie)
{
    return &(GetAnimSync(lCookie)->_imgAnimState);
}

HRESULT
CImgAnim::FindOrCreateAnimSync(DWORD_PTR dwDocId, DWORD_PTR dwImgId, LONG * plCookie, CAnimSync ** ppAnimSyncOut)
{
    int cAnimSync;
    CAnimSync **ppAnimSync;
    HRESULT hr = S_OK;

    for (cAnimSync = _aryAnimSync.Size(), ppAnimSync = _aryAnimSync;
         cAnimSync > 0;
         cAnimSync--, ppAnimSync++)
    {
        if (    *ppAnimSync
            && (*ppAnimSync)->_dwDocId == dwDocId
            && (*ppAnimSync)->_dwImgId == dwImgId)
        {
            *ppAnimSyncOut = *ppAnimSync;
            *plCookie = _aryAnimSync.Size() - cAnimSync + 1;
            goto Cleanup;
        }
    }

    *ppAnimSyncOut = new CAnimSync();
    if (!*ppAnimSyncOut)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _aryAnimSync.Append(*ppAnimSyncOut);
    if (hr)
        goto Cleanup;

    *plCookie = _aryAnimSync.Size();

Cleanup:
    RRETURN(hr);
}

HRESULT
CImgAnim::RegisterForAnim(void * pvObj, DWORD_PTR dwDocId, DWORD_PTR dwImgId,
                          CAnimSync::ASCALLBACK pfnCallback,
                          void * pvArg,
                          LONG * plCookie)
{
    HRESULT hr;
    CAnimSync * pAnimSync;

    hr = FindOrCreateAnimSync(dwDocId, dwImgId, plCookie, &pAnimSync);
    if (hr)
        goto Cleanup;

    hr = pAnimSync->Register(pvObj, dwDocId, dwImgId, pfnCallback, pvArg);
    if (hr)
    {
        if (pAnimSync->IsEmpty())
        {
            CleanupAnimSync(*plCookie);
        }
        *plCookie = 0;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

void
CImgAnim::CleanupAnimSync(LONG lCookie)
{
    Assert(lCookie > 0 && lCookie <= _aryAnimSync.Size());

    CAnimSync ** ppAnimSync = &_aryAnimSync[lCookie - 1];
    CAnimSync * pAnimSync = *ppAnimSync;

    if (pAnimSync && pAnimSync->IsEmpty())
    {
        delete pAnimSync;
        *ppAnimSync = NULL;

        if (lCookie == _aryAnimSync.Size())
        {
            for (; lCookie-- > 0 && *ppAnimSync == NULL; --ppAnimSync)
            {
                _aryAnimSync.Delete(lCookie);
            }
        }
    }
}

void
CImgAnim::UnregisterForAnim(void * pvObj, LONG lCookie)
{
    CAnimSync * pAnimSync = GetAnimSync(lCookie);

    if (pAnimSync)
    {
        pAnimSync->Unregister(pvObj);
        CleanupAnimSync(lCookie);
    }
}

void
CImgAnim::OnTimer()
{
    int cAnimSync;
    CAnimSync **ppAnimSync;
    DWORD dwInterval = 0xFFFFFFFF;
    DWORD dwFrameTimeMS;
    HWND hwnd = NULL;

    for (cAnimSync = _aryAnimSync.Size(), ppAnimSync = _aryAnimSync;
         cAnimSync > 0;
         cAnimSync--, ppAnimSync++)
    {
        if (*ppAnimSync && ((*ppAnimSync)->_state == ANIMSTATE_PLAY))
        {
            (*ppAnimSync)->OnTimer(&dwFrameTimeMS);

            if (dwFrameTimeMS < dwInterval)
                dwInterval = dwFrameTimeMS;
        }
    }

    for (cAnimSync = _aryAnimSync.Size(), ppAnimSync = _aryAnimSync;
         cAnimSync > 0;
         cAnimSync--, ppAnimSync++)
    {
        if (*ppAnimSync && ((*ppAnimSync)->_state == ANIMSTATE_PLAY) && (*ppAnimSync)->_fInvalidated)
        {
            (*ppAnimSync)->Update(&hwnd);
        }
    }

    SetInterval(dwInterval);
}

void
CImgAnim::ProgAnim(LONG lCookie)
{
    CAnimSync * pAnimSync = GetAnimSync(lCookie);
    DWORD dwFrameTimeMS;

    if (pAnimSync && (pAnimSync->_state == ANIMSTATE_PLAY))
    {
        CImgCtx * pImgCtx = pAnimSync->GetImgCtx();

        if (pImgCtx)
        {
            if (pImgCtx->NextFrame(GetImgAnimState(lCookie), GetTickCount(), &dwFrameTimeMS))
            {
                pAnimSync->Invalidate();
            }

            if (dwFrameTimeMS < _dwInterval)
                SetInterval(dwFrameTimeMS);
        }
    }
}

void
CImgAnim::StopAnim(LONG lCookie)
{
    CAnimSync * pAnimSync = GetAnimSync(lCookie);

    if (pAnimSync)
    {
        pAnimSync->_state = ANIMSTATE_STOP;
    }
}

void
CImgAnim::StartAnim(LONG lCookie)
{
    CAnimSync * pAnimSync = GetAnimSync(lCookie);

    if (pAnimSync && (pAnimSync->_state != ANIMSTATE_PLAY))
    {
        pAnimSync->_state = ANIMSTATE_PLAY;
        OnTimer();
    }
}

