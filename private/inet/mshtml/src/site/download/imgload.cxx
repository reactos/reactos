//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       imgload.cxx
//
//  Contents:   CImgLoad
//              CImgTask
//              CImgTaskExec
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_W95FIBER_H_
#define X_W95FIBER_H_
#include "w95fiber.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagImgLoad,      "Dwn", "Trace CImgLoad")
PerfDbgTag(tagImgTask,      "Dwn", "Trace CImgTask")
PerfDbgTag(tagImgTaskIO,    "Dwn", "Trace CImgTask I/O")
PerfDbgTag(tagImgProgSlow,  "Dwn", "! Slow progressive rendering")
PerfDbgTag(tagImgAnimSlow,  "Dwn", "! Slow progressive animation")

MtDefine(CImgLoad, Dwn, "CImgLoad")
MtDefine(CImgTaskExec, Dwn, "CImgTaskExec")

// Globals --------------------------------------------------------------------

CImgTaskExec *   g_pImgTaskExec;

// CImgLoad ----------------------------------------------------------------

CImgLoad::~CImgLoad()
{
    if (_pImgTask)
        _pImgTask->SubRelease();
}

void
CImgLoad::Passivate()
{
    super::Passivate();

    if (_pImgTask)
    {
        // The task is needed by the asynchronous callback methods, but here
        // we want to passivate it by releasing the last reference but
        // maintaining a secondary reference which will be released by the
        // destructor.

        _pImgTask->SubAddRef();
        _pImgTask->Release();
    }
}

HRESULT
CImgLoad::Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo)
{
    PerfDbgLog(tagImgLoad, this, "+CImgLoad::Init");

    HRESULT hr;

    hr = THR(super::Init(pdli, pDwnInfo, 
                IDS_BINDSTATUS_DOWNLOADINGDATA_PICTURE,
                DWNF_GETMODTIME | DWNF_GETFLAGS));
    if (hr)
        goto Cleanup;

Cleanup:
    PerfDbgLog1(tagImgLoad, this, "-CImgLoad::Init (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CImgLoad::OnBindHeaders()
{
    PerfDbgLog(tagImgLoad, this, "+CImgLoad::OnBindHeaders");
    FILETIME ft;
    HRESULT hr = S_OK;

    if (!_pDwnInfo->TstFlags(DWNF_DOWNLOADONLY))
    {
        ft = _pDwnBindData->GetLastMod();

        if (ft.dwLowDateTime || ft.dwHighDateTime)
        {
            if (_pDwnInfo->AttachByLastMod(this, &ft, _pDwnBindData->IsFullyAvail()))
            {
                CDwnDoc * pDwnDoc = _pDwnBindData->GetDwnDoc();

                if (pDwnDoc)
                {
                    DWNPROG DwnProg;
                    _pDwnBindData->GetProgress(&DwnProg);
                    pDwnDoc->AddBytesRead(DwnProg.dwMax);
                }

                _pDwnBindData->Disconnect();
                OnDone(S_OK);

                hr = S_FALSE;
                goto Cleanup;
            }
        }
    }

    _pDwnInfo->SetSecFlags(_pDwnBindData->GetSecFlags());
    
Cleanup:    
    PerfDbgLog1(tagImgLoad, this, "-CImgLoad::OnBindHeaders (hr=%lX)", hr);
    RRETURN1(hr, S_FALSE);
}

HRESULT
CImgLoad::OnBindMime(MIMEINFO * pmi)
{
    PerfDbgLog1(tagImgLoad, this, "+CImgLoad::OnBindMime %ls",
        pmi ? pmi->pch : g_Zero.ach);

    CImgTask *  pImgTask = NULL;
    HRESULT     hr       = S_OK;

    if (!pmi || _pDwnInfo->TstFlags(DWNF_DOWNLOADONLY))
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (!pmi->pfnImg)
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    _pDwnInfo->SetMimeInfo(pmi);

    pImgTask = pmi->pfnImg();

    if (pImgTask == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pImgTask->Init(GetImgInfo(), pmi, _pDwnBindData);

    hr = THR(StartImgTask(pImgTask));
    if (hr)
        goto Cleanup;

    EnterCriticalSection();

    if (_fPassive)
        hr = E_ABORT;
    else
    {
        _pImgTask = pImgTask;
        pImgTask  = NULL;
    }

    LeaveCriticalSection();

    if (hr == S_OK)
    {
        GetImgInfo()->OnLoadTask(this, _pImgTask);
    }

Cleanup:
    if (pImgTask)
        pImgTask->Release();
    PerfDbgLog1(tagImgLoad, this, "-CImgLoad::OnBindMime (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CImgLoad::OnBindData()
{
    PerfDbgLog(tagImgLoad, this, "+CImgLoad::OnBindData");

    HRESULT hr = S_OK;

    if (_pImgTask)
    {
        _pImgTask->SetBlocked(FALSE);
    }
    else if (_pDwnInfo->TstFlags(DWNF_DOWNLOADONLY))
    {
        BYTE  ab[1024];
        ULONG cb;

        do
        {
            hr = THR(_pDwnBindData->Read(ab, sizeof(ab), &cb));
        }
        while (!hr && cb);
    }
    else
    {
#if !defined(WINCE) && !defined(WIN16)
        // If we're getting data but never got a valid mime type that we
        // know how to decode, use the data to figure out if a pluggable
        // decoder should be used.

        BYTE        ab[200];
        ULONG       cb;
        MIMEINFO *  pmi;
        
        hr = THR(_pDwnBindData->Peek(ab, ARRAY_SIZE(ab), &cb));
        if (hr)
            goto Cleanup;

        if (cb < ARRAY_SIZE(ab) && _pDwnBindData->IsPending())
            goto Cleanup;

        pmi = GetMimeInfoFromData(ab, cb, _pDwnBindData->GetContentType());
        
        if (!pmi || !pmi->pfnImg)
        {
            pmi = _pDwnBindData->GetRawMimeInfoPtr();
            if( !pmi || !pmi->pfnImg )
            {
                hr = E_ABORT;
                goto Cleanup;
            }
        }

        hr = OnBindMime(pmi);
        if (hr)
            goto Cleanup;

        _pImgTask->SetBlocked(FALSE);
#else
        hr = E_ABORT;
#endif
    }

Cleanup:
    PerfDbgLog1(tagImgLoad, this, "-CImgLoad::OnBindData (hr=%lX)", hr);
    RRETURN(hr);
}

void
CImgLoad::OnBindDone(HRESULT hrErr)
{
    PerfDbgLog1(tagImgLoad, this, "+CImgLoad::OnBindDone (hrErr=%lX)", hrErr);

    if (_pImgTask)
        _pImgTask->SetBlocked(FALSE);

    OnDone(hrErr);

    PerfDbgLog(tagImgLoad, this, "-CImgLoad::OnBindDone");
}

// CImgTask -------------------------------------------------------------------

CImgTask::~CImgTask()
{
    PerfDbgLog(tagImgTask, this, "+CImgTask::~CImgTask");

    if (_pImgInfo)
        _pImgInfo->SubRelease();

    if (_pDwnBindData)
        _pDwnBindData->Release();

    if (!_fComplete)
    {
        FreeGifAnimData(&_gad, (CImgBitsDIB *)_pImgBits);
#ifndef NO_ART
        if (_pArtPlayer)
            delete _pArtPlayer;
#endif
#ifdef _MAC
        if(_Profile)
            _Profile->Release();
#endif
        if (_pImgBits)
            delete _pImgBits;
    }

    PerfDbgLog(tagImgTask, this, "-CImgTask::~CImgTask");
}

void
CImgTask::Init(CImgInfo * pImgInfo, MIMEINFO * pmi, CDwnBindData * pDwnBindData)
{
    PerfDbgLog1(tagImgTask, this, "+CImgTask::Init %ls", pImgInfo->GetUrl());

    _colorMode  = pImgInfo->GetColorMode();
    _pmi        = pmi;
    _lTrans     = -1;
    _ySrcBot    = -2;

    _pImgInfo = pImgInfo;
    _pImgInfo->SubAddRef();

    _pDwnBindData = pDwnBindData;
    _pDwnBindData->AddRef();

    PerfDbgLog(tagImgTask, this, "-CImgTask::Init");
}

void
CImgTask::Run()
{
    PerfDbgLog(tagImgTask, this, "+CImgTask::Run");

    GetImgTaskExec()->RunTask(this);

    PerfDbgLog(tagImgTask, this, "-CImgTask::Run");
}

BOOL
CImgTask::Read(void * pv, ULONG cb, ULONG * pcbRead, ULONG cbMinReq)
{
    PerfDbgLog1(tagImgTaskIO, this, "+CImgTask::Read (req %ld)", cb);

    ULONG   cbReq = cb, cbGot, cbTot = 0;
    HRESULT hr    = S_OK;

    if (cbMinReq == 0 || cbMinReq > cb)
        cbMinReq = cb;

    for (;;)
    {
        if (_fTerminate)
        {
            hr = E_ABORT;
            break;
        }

        hr = THR(_pDwnBindData->Read(pv, cbReq, &cbGot));
        if (hr)
            break;

        cbTot += cbGot;
        cbReq -= cbGot;
        pv = (BYTE *)pv + cbGot;

        if (cbReq == 0)
            break;

        if (!cbGot || IsTimeout())
        {
            if (    _pDwnBindData->IsEof()
                ||  (!cbGot && cbTot >= cbMinReq))
                break;

            PerfDbgLog2(tagImgTask, this, "-CImgTask::Read (fiber %ld yield %s)",
                _pfi - g_pImgTaskExec->_afi, cbGot ? "timeout" : "pending");

            GetImgTaskExec()->YieldTask(this, !cbGot);

            PerfDbgLog1(tagImgTask, this, "+CImgTask::Read (fiber %ld resume)",
                _pfi - g_pImgTaskExec->_afi);
        }
    }

    if (pcbRead)
    {
        *pcbRead = cbTot;
    }

    PerfDbgLog3(tagImgTaskIO, this, "-CImgTask::Read (got %ld) %c%c",
        cbTot, _pDwnBindData->IsEof() ? 'E' : ' ',
        hr == S_OK && cbTot > 0 ? 'T' : 'F');

    return(hr == S_OK && cbTot > 0);
}

void
CImgTask::Terminate()
{
    if (!_fTerminate)
    {
        _fTerminate = TRUE;
        SetBlocked(FALSE);
    }
}

void
CImgTask::OnProg(BOOL fLast, ULONG ulBits, BOOL fAll, LONG yBot)
{
    DWORD dwTick = GetTickCount();

    _yTopProg = Union(_yTopProg, _yBotProg, fAll, yBot);
    _yBotProg = yBot;

    if (fLast || (dwTick - _dwTickProg > 1000))
    {
        _dwTickProg = dwTick;

        _pImgInfo->OnTaskProg(this, ulBits, _yTopProg == -1, _yBotProg);

        _yTopProg = _yBotProg;
    }

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagImgProgSlow))
        Sleep(100);
    #endif
}

void
CImgTask::OnAnim()
{
    _pImgInfo->OnTaskAnim(this);

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagImgAnimSlow))
        Sleep(1000);
    #endif
}

void
CImgTask::Exec()
{
    BOOL fNonProgressive = FALSE;

    _dwTickProg = GetTickCount();

    Decode(&fNonProgressive);
    if (_pImgInfo->TstFlags(DWNF_MIRRORIMAGE))
    {
        if(_pImgBits)
        {
           ((CImgBitsDIB*)(_pImgBits))->SetMirrorStatus(TRUE);
        }
    }
    
    if (_pImgBits && _ySrcBot > -2)
    {
#ifdef NO_ART
        _fComplete = _pImgInfo->OnTaskBits(this, _pImgBits,
            &_gad, NULL, _lTrans, _ySrcBot, fNonProgressive
#ifdef _MAC
            ,_Profile
#endif _MAC
            );
#else
        _fComplete = _pImgInfo->OnTaskBits(this, _pImgBits,
            &_gad, _pArtPlayer, _lTrans, _ySrcBot, fNonProgressive
#ifdef _MAC
            ,_Profile
#endif _MAC
         );
#endif
    }

    if (_ySrcBot == -2 || (_fComplete && _ySrcBot != -1))
    {
        OnProg(TRUE, _ySrcBot == -2 ? IMGBITS_NONE : IMGBITS_PARTIAL,
            TRUE, _yBot);
    }

    #if DBG==1 || defined(PERFTAGS)
    if (_ySrcBot != -1)
        PerfDbgLog4(tagImgTask, this,
            "CImgTask::Exec [%s decode (%ld of %ld) for %ls]",
            _ySrcBot == -2 ? "Failed" : "Partial", max(0L, _ySrcBot),
            _gad.pgf ? _gad.pgf->height : _yHei, GetUrl());
    #endif

    if (_fComplete && _ySrcBot == -1 && !_pDwnBindData->IsEof())
    {
        BYTE ab[512];

        // The image is fully decoded but the binding hasn't reached EOF.
        // Attempt to read the final EOF to allow the binding to complete
        // normally.  This will make sure that HTTP downloads don't delete
        // the cache file just because a decoder (such as the BMP decoder)
        // knows how many bytes to expect based on the header and doesn't
        // stick around until it sees EOF.

        Read(ab, sizeof(ab), NULL);
    }

    if (!_pDwnBindData->IsEof())
    {
        // Looks like the decoder didn't like the data.  Since it is still
        // flowing and we don't need any more of it, abort the binding.

        _pDwnBindData->Terminate(E_ABORT);
    }

    _fTerminate = TRUE;
}

#ifndef NO_ART
BOOL
CImgTask::DoTaskGetReport(CArtPlayer * pArtPlayer)
{
    BOOL fResult = FALSE;

    if ((pArtPlayer == _pArtPlayer) && _pImgBits)
        fResult = _pArtPlayer->GetArtReport((CImgBitsDIB **)_pImgBits,
                        _yHei, _colorMode);

    return (fResult);
}
#endif

// CImgTaskExec ---------------------------------------------------------------

void
CImgTaskExec::YieldTask(CImgTask * pImgTask, BOOL fBlock)
{
    if (fBlock)
    {
        pImgTask->SetBlocked(TRUE);
    }

    SuspendCAP();
    SwitchesEndTimer(SWITCHES_TIMER_DECODEIMAGE);

    FbrSwitchToFiber(_pvFiberMain);

    SwitchesBegTimer(SWITCHES_TIMER_DECODEIMAGE);
    ResumeCAP();
}

FIBERINFO *
CImgTaskExec::GetFiber(CImgTask * pImgTask)
{
    BOOL        fAll = pImgTask->IsFullyAvail() ? 0 : 1;
    FIBERINFO * pfi  = &_afi[fAll];
    UINT        cfi  = ARRAY_SIZE(_afi) - fAll;

    for (; cfi > 0; --cfi, ++pfi)
    {
        if (pfi->pImgTask == NULL)
            goto found;
    }

    return(NULL);

found:

    if (pfi->pvFiber == NULL)
    {
        pfi->pvMain = _pvFiberMain;

        if (pfi->pvMain)
        {
            pfi->pvFiber = FbrCreateFiber(0x8000, FiberProc, pfi);
        }

        if (pfi->pvFiber == NULL)
            return(NULL);
    }

    return(pfi);
}

void
CImgTaskExec::AssignFiber(FIBERINFO * pfi)
{
    BOOL fAll = (pfi == &_afi[0]);

    EnterCriticalSection();

    CImgTask * pImgTask = (CImgTask *)_pDwnTaskHead;

    for (; pImgTask; pImgTask = (CImgTask *)pImgTask->_pDwnTaskNext)
    {
        if (    pImgTask->_fWaitForFiber
            &&  !pImgTask->_fTerminate
            &&  !pImgTask->_pfi
            &&  (!fAll || pImgTask->IsFullyAvail()))
        {
            pfi->pImgTask = pImgTask;
            pImgTask->_pfi = pfi;
            pImgTask->_fWaitForFiber = FALSE;
            pImgTask->SubAddRef();
            pImgTask->SetBlocked(FALSE);
            goto Cleanup;
        }
    }

Cleanup:
    LeaveCriticalSection();    
}

void
CImgTaskExec::RunTask(CImgTask * pImgTask)
{
    FIBERINFO * pfi = pImgTask->_pfi;

    if (pfi == NULL && !pImgTask->_fTerminate)
    {
        pfi = GetFiber(pImgTask);

        if (pfi)
        {
            pfi->pImgTask = pImgTask;
            pImgTask->_pfi = pfi;
            pImgTask->_fWaitForFiber = FALSE;
            pImgTask->SubAddRef();
        }
        else
        {
            // No fiber available.  Note that when one becomes available
            // there may be a task waiting to hear about it.

            pImgTask->_fWaitForFiber = TRUE;
            pImgTask->SetBlocked(TRUE);
            goto Cleanup;
        }
    }

    if (pfi)
    {
        Assert(pfi->pImgTask == pImgTask);
        Assert(pImgTask->_pfi == pfi);

        SwitchesEndTimer(SWITCHES_TIMER_DECODEIMAGE);
        SuspendCAP();

        FbrSwitchToFiber(pfi->pvFiber);

        ResumeCAP();
        SwitchesBegTimer(SWITCHES_TIMER_DECODEIMAGE);

        if (pImgTask->_pfi == NULL)
        {
            pImgTask->SubRelease();
            AssignFiber(pfi);
        }
    }

    if (pImgTask->_fTerminate)
    {
        pImgTask->_pImgInfo->OnTaskDone(pImgTask);
        super::DelTask(pImgTask);
    }

Cleanup:
    return;
}

HRESULT
CImgTaskExec::ThreadInit()
{
    PerfDbgLog(tagImgTask, this, "+CImgTaskExec::ThreadInit");

    HRESULT hr;

    hr = THR(super::ThreadInit());
    if (hr)
        goto Cleanup;

    if (FbrAttachToBase())
    {
        _pvFiberMain = FbrConvertThreadToFiber(0);
    }

Cleanup:
    PerfDbgLog(tagImgTask, this, "-CImgTaskExec::ThreadInit");
    RRETURN(hr);
}

void
CImgTaskExec::ThreadTerm()
{
    PerfDbgLog(tagImgTask, this, "+CImgTaskExec::ThreadTerm");

    FIBERINFO * pfi = _afi;
    UINT        cfi = ARRAY_SIZE(_afi);

    for (; cfi > 0; --cfi, ++pfi)
    {
        if (pfi->pImgTask)
        {
            pfi->pImgTask->_pfi = NULL;
            pfi->pImgTask->SubRelease();
            pfi->pImgTask = NULL;
        }
        if (pfi->pvFiber)
        {
            FbrDeleteFiber(pfi->pvFiber);
            pfi->pvFiber = NULL;
        }
    }

    // Manually release any tasks remaining on the queue.  Don't call super::ThreadTerm
    // because it tries to call Terminate() on the task and expects it to dequeue.  Our
    // tasks don't dequeue in Terminate() though, so we end up in an infinite loop.

    while (_pDwnTaskHead)
    {
        CImgTask * pImgTask = (CImgTask *)_pDwnTaskHead;
        _pDwnTaskHead = pImgTask->_pDwnTaskNext;
        Assert(pImgTask->_fEnqueued);
        pImgTask->SubRelease();
    }

    PerfDbgLog(tagImgTask, this, "-CImgTaskExec::ThreadTerm");
}

void
CImgTaskExec::ThreadExit()
{
    PerfDbgLog(tagImgTask, this, "CImgTaskExec::ThreadExit");

    void * pvMain = _pvFiberMain;

    if (_fCoInit)
    {
        CoUninitialize();
    }

    super::ThreadExit();

    if (pvMain)
    {
        // Due to a bug in the Win95 and WinNT implementation of fibers,
        // we don't call FbrDeleteFiber(pvMain) anymore.  Instead we manually
        // free the fiber data for the main fiber on this thread.

        LocalFree(pvMain);
        FbrDetachFromBase();
    }
}

void
CImgTaskExec::ThreadTimeout()
{
    PerfDbgLog(tagImgTask, this, "+CImgTaskExec::ThreadTimeout");

    KillImgTaskExec();

    PerfDbgLog(tagImgTask, this, "-CDwnTaskExec::ThreadTimeout");
}

void WINAPI
CImgTaskExec::FiberProc(void * pv)
{
    FIBERINFO * pfi = (FIBERINFO *)pv;
    CImgTask * pImgTask = NULL;

    SwitchesBegTimer(SWITCHES_TIMER_DECODEIMAGE);
    StartCAP();

    while (pfi)
    {
        PerfDbgLog1(tagImgTask, pImgTask, "CImgTaskExec::FiberProc (fiber "
            "%ld attach)", pfi - g_pImgTaskExec->_afi);

        pImgTask = pfi->pImgTask;
        pImgTask->Exec();
        pfi->pImgTask = NULL;
        pImgTask->_pfi = NULL;

        PerfDbgLog1(tagImgTask, pImgTask, "CImgTaskExec::FiberProc (fiber "
            "%ld detach)", pfi - g_pImgTaskExec->_afi);

        SwitchesEndTimer(SWITCHES_TIMER_DECODEIMAGE);
        SuspendCAP();

        FbrSwitchToFiber(pfi->pvMain);

        ResumeCAP();
        SwitchesBegTimer(SWITCHES_TIMER_DECODEIMAGE);
    }

    StopCAP();
    SwitchesEndTimer(SWITCHES_TIMER_DECODEIMAGE);
}

HRESULT
CImgTaskExec::RequestCoInit()
{
    HRESULT hr = S_OK;

    if (!_fCoInit)
    {
        hr = CoInitialize(NULL);

        if (!FAILED(hr))
        {
            _fCoInit = TRUE;
            hr = S_OK;
        }
    }

    RRETURN(hr);
}

#if DBG==1

void
CImgTaskExec::Invariant()
{
    EnterCriticalSection();

    FIBERINFO * pfi = _afi;
    UINT        cfi = ARRAY_SIZE(_afi);
    CImgTask *  pImgTask;
    BOOL        fFound;

    super::Invariant();

    // Because the invariant is looking at the fiber list, this code can only be
    // run on the CImgTaskExec thread.

    if (_dwThreadId != GetCurrentThreadId())
        cfi = 0;

    for (; cfi > 0; --cfi, ++pfi)
    {
        Assert(!pfi->pImgTask || pfi->pImgTask->_pfi == pfi);

        fFound   = !pfi->pImgTask;
        pImgTask = fFound ? NULL : (CImgTask *)_pDwnTaskHead;

        for (; pImgTask; pImgTask = (CImgTask *)pImgTask->_pDwnTaskNext)
        {
            if (pImgTask == pfi->pImgTask)
            {
                fFound = TRUE;
                break;
            }
        }

        Assert(fFound);
    }

    LeaveCriticalSection();
}

#endif

// External Functions ---------------------------------------------------------

HRESULT
StartImgTask(CImgTask * pImgTask)
{
    PerfDbgLog1(tagImgTask, NULL, "+StartImgTask %lX", pImgTask);

    HRESULT hr = S_OK;

    g_csImgTaskExec.Enter();

    if (g_pImgTaskExec == NULL)
    {
        g_pImgTaskExec = new CImgTaskExec(g_csImgTaskExec.GetPcs());

        if (g_pImgTaskExec == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(g_pImgTaskExec->Launch());

        if (hr)
        {
            g_pImgTaskExec->Release();
            g_pImgTaskExec = NULL;
            goto Cleanup;
        }
    }

    g_pImgTaskExec->AddTask(pImgTask);

Cleanup:
    g_csImgTaskExec.Leave();
    PerfDbgLog(tagImgTask, NULL, "-StartImgTask");
    RRETURN(hr);
}

void
KillImgTaskExec()
{
    PerfDbgLog(tagImgTask, NULL, "+KillImgTaskExec");

    g_csImgTaskExec.Enter();

    CImgTaskExec * pImgTaskExec = g_pImgTaskExec;
    g_pImgTaskExec = NULL;

    g_csImgTaskExec.Leave();

    if (pImgTaskExec)
    {
        pImgTaskExec->Shutdown();
        pImgTaskExec->Release();
    }

    PerfDbgLog(tagImgTask, NULL, "-KillImgTaskExec");
}
