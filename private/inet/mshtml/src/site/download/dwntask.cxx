//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dwntask.cxx
//
//  Contents:   CDwnTask
//				CDwnTaskExec
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnTask,            "Dwn", "Trace CDwnTask")
PerfDbgTag(tagDwnTaskExec,        "Dwn", "Trace CDwnTaskExec")
PerfDbgTag(tagDwnTaskExecVerbose, "Dwn", "Trace CDwnTaskExec (Verbose)")
PerfDbgTag(tagDwnTaskExec10Sec,   "Dwn", "! Timeout download threads in 10 sec")

MtDefine(CDwnTaskExec, Dwn, "CDwnTaskExec")

// Globals --------------------------------------------------------------------

CDwnTaskExec *   g_pDwnTaskExec = NULL;

// CDwnTask -------------------------------------------------------------------

void
CDwnTask::Passivate()
{
    PerfDbgLog(tagDwnTask, this, "+CDwnTask::Passivate");

    Terminate();
    super::Passivate();

    PerfDbgLog(tagDwnTask, this, "-CDwnTask::Passivate");
}

// CDwnTaskExec ---------------------------------------------------------------

CDwnTaskExec::CDwnTaskExec(CRITICAL_SECTION * pcs)
    : super(pcs)
{
    // This object will be destroyed on DllProcessDetach, so its existence
    // should not prevent the DLL from being unloaded.  This next call
    // undoes what CBaseFT::CBaseFT did.

    DecrementSecondaryObjectCount(10);
}

CDwnTaskExec::~CDwnTaskExec()
{
    // This next call temporarily puts back a secondary reference that
    // CBaseFT::~CBaseFT will decrement.

    IncrementSecondaryObjectCount(10);
}

void
CDwnTaskExec::AddTask(CDwnTask * pDwnTask)
{
    PerfDbgLog1(tagDwnTaskExec, this, "+CDwnTaskExec::AddTask %lX", pDwnTask);

    BOOL fSignal = FALSE;

    EnterCriticalSection();

    #if DBG==1
    Invariant();
    #endif

    Assert(!pDwnTask->_fEnqueued);
    Assert(!pDwnTask->_pDwnTaskExec);

    pDwnTask->_pDwnTaskExec = this;
    pDwnTask->_pDwnTaskNext = NULL;
    pDwnTask->_pDwnTaskPrev = _pDwnTaskTail;
    pDwnTask->_fEnqueued    = TRUE;
    pDwnTask->SubAddRef();

    if (_pDwnTaskTail)
        _pDwnTaskTail = _pDwnTaskTail->_pDwnTaskNext = pDwnTask;
    else
        _pDwnTaskHead = _pDwnTaskTail = pDwnTask;

    if (pDwnTask->_fActive)
    {
        fSignal = (_cDwnTaskActive == 0);
        _cDwnTaskActive += 1;
    }

    _cDwnTask += 1;

    #if DBG==1
    Invariant();
    #endif

    LeaveCriticalSection();

    if (fSignal)
    {
        Verify(SetEvent(_hevWait));
    }

    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::AddTask");
}

void
CDwnTaskExec::SetTask(CDwnTask * pDwnTask, BOOL fActive)
{
    PerfDbgLog2(tagDwnTaskExec, this, "+CDwnTaskExec::SetTask %lX %s", pDwnTask, fActive ? "Active" : "Blocked");

    BOOL fSignal = FALSE;

    EnterCriticalSection();

    #if DBG==1
    Invariant();
    #endif

    if (pDwnTask->_fEnqueued)
    {
        if (pDwnTask == _pDwnTaskRun)
        {
            // Making a running task active always wins ... the task will
            // run from the top at least one more time.

            if (_ta != TA_DELETE)
            {
                if (fActive)
                    _ta = TA_ACTIVATE;
                else if (_ta != TA_ACTIVATE)
                    _ta = TA_BLOCK;
            }
        }
        else if (!!fActive != !!pDwnTask->_fActive)
        {
            pDwnTask->_fActive = fActive;

            if (fActive)
            {
                fSignal = (_cDwnTaskActive == 0);
                _cDwnTaskActive += 1;
            }
            else
            {
                _cDwnTaskActive -= 1;
            }
        }
    }

    #if DBG==1
    Invariant();
    #endif

    LeaveCriticalSection();

    if (fSignal && GetThreadId() != GetCurrentThreadId())
    {
        Verify(SetEvent(_hevWait));
    }

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::SetTask");
}

void
CDwnTaskExec::DelTask(CDwnTask * pDwnTask)
{
    PerfDbgLog1(tagDwnTaskExec, this, "+CDwnTaskExec::DelTask %lX", pDwnTask);

    BOOL fRelease = FALSE;

    EnterCriticalSection();

    #if DBG==1
    Invariant();
    #endif

    if (pDwnTask->_fEnqueued)
    {
        if (pDwnTask == _pDwnTaskRun)
        {
            _ta = TA_DELETE;
        }
        else
        {
            if (pDwnTask->_pDwnTaskPrev)
                pDwnTask->_pDwnTaskPrev->_pDwnTaskNext = pDwnTask->_pDwnTaskNext;
            else
                _pDwnTaskHead = pDwnTask->_pDwnTaskNext;

            if (pDwnTask->_pDwnTaskNext)
                pDwnTask->_pDwnTaskNext->_pDwnTaskPrev = pDwnTask->_pDwnTaskPrev;
            else
                _pDwnTaskTail = pDwnTask->_pDwnTaskPrev;

            if (pDwnTask->_fActive)
            {
                _cDwnTaskActive -= 1;
            }

            _cDwnTask -= 1;

            if (_pDwnTaskCur == pDwnTask)
                _pDwnTaskCur = pDwnTask->_pDwnTaskNext;

            pDwnTask->_fEnqueued = FALSE;
            fRelease = TRUE;
        }
    }

    #if DBG==1
    Invariant();
    #endif

    LeaveCriticalSection();

    if (fRelease)
        pDwnTask->SubRelease();

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::DelTask");
}

BOOL
CDwnTaskExec::IsTaskTimeout()
{
    return(_fShutdown || (GetTickCount() - _dwTickRun > _dwTickSlice));
}

HRESULT
CDwnTaskExec::Launch()
{
    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::Launch");

    HRESULT hr;

    _hevWait = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (_hevWait == NULL)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    hr = THR(super::Launch(FALSE));
    if (hr)
        goto Cleanup;

Cleanup:
    PerfDbgLog1(tagDwnTaskExec, this, "-CDwnTaskExec::Launch (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnTaskExec::Passivate()
{
    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::Passivate");

    if (_hevWait)
    {
        CloseEvent(_hevWait);
    }

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::Passivate");
}

HRESULT
CDwnTaskExec::ThreadInit()
{
    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::ThreadInit");

    _dwTickTimeout  = 10 * 60 * 1000;   // Ten minutes
    _dwTickSlice    = 200;              // 0.2 seconds

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDwnTaskExec10Sec))
        _dwTickTimeout = 10 * 1000;     // Ten seconds
    #endif

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::ThreadInit (hr=0)");

    return(S_OK);
}

void
CDwnTaskExec::ThreadExec()
{
    PerfDbgLog(tagDwnTaskExecVerbose, this, "CDwnTaskExec::ThreadExec (Enter)");

    CDwnTask * pDwnTask;

    for (;;)
    {
        for (;;)
        {
            PerfDbgLog(tagDwnTaskExecVerbose, this,
                "CDwnTaskExec::ThreadExec (EnterCriticalSection)");

            EnterCriticalSection();

            #if DBG==1
            Invariant();
            #endif

            pDwnTask     = _pDwnTaskRun;
            _pDwnTaskRun = NULL;

            if (_ta == TA_DELETE)
                DelTask(pDwnTask);
            else if (_ta == TA_BLOCK)
                pDwnTask->SetBlocked(TRUE);

            _ta = TA_NONE;

            if (_cDwnTaskActive && !_fShutdown)
            {
                PerfDbgLog(tagDwnTaskExecVerbose, this,
                    "CDwnTaskExec::ThreadExec (find unblocked task)");

                while (!_pDwnTaskRun)
                {
                    if (_pDwnTaskCur == NULL)
                        _pDwnTaskCur = _pDwnTaskHead;

                    if (_pDwnTaskCur->_fActive)
                        _pDwnTaskRun = _pDwnTaskCur;

                    _pDwnTaskCur = _pDwnTaskCur->_pDwnTaskNext;
                }
            }

            #if DBG==1
            Invariant();
            #endif

            LeaveCriticalSection();

            PerfDbgLog(tagDwnTaskExecVerbose, this,
                "CDwnTaskExec::ThreadExec (LeaveCriticalSection)");

            if (!_pDwnTaskRun)
                break;

            PerfDbgLog(tagDwnTaskExecVerbose, this,
                "CDwnTaskExec::ThreadExec (run task)");

            _dwTickRun = GetTickCount();
            _pDwnTaskRun->Run();

            PerfDbgLog1(tagDwnTaskExecVerbose, this,
                "CDwnTaskExec::ThreadExec (task ran for %ld ticks)",
                GetTickCount() - _dwTickRun);
        }

        if (_fShutdown)
            break;

        PerfDbgLog(tagDwnTaskExecVerbose, this,
            "CDwnTaskExec::ThreadExec (WaitForSingleObject)");

        SuspendCAP();

        DWORD dwResult = WaitForSingleObject(_hevWait, _dwTickTimeout);

        ResumeCAP();

        PerfDbgLog1(tagDwnTaskExecVerbose, this,
            "CDwnTaskExec::ThreadExec (Wait over, dwResult=%ld)", dwResult);

        if (dwResult == WAIT_TIMEOUT)
        {
            PerfDbgLog(tagDwnTaskExecVerbose, this,
                "CDwnTaskExec::ThreadExec (WAIT_TIMEOUT)");

            EnterCriticalSection();

            if (_cDwnTask == 0)
            {
                ThreadTimeout();
            }

            LeaveCriticalSection();
        }
    }

    PerfDbgLog(tagDwnTaskExecVerbose, this, "CDwnTaskExec::ThreadExec (Leave)");
}

void
CDwnTaskExec::ThreadTimeout()
{
    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::ThreadTimeout");

    KillDwnTaskExec();

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::ThreadTimeout");
}

void
CDwnTaskExec::ThreadTerm()
{
    while (_pDwnTaskHead)
    {
        PerfDbgLog1(tagDwnTaskExec, this, "+CDwnTaskExec::ThreadExec "
            "Terminate %lX", _pDwnTaskHead);

        _pDwnTaskHead->Terminate();

        PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::ThreadExec Terminate");
    }
}

void
CDwnTaskExec::Shutdown()
{
    PerfDbgLog(tagDwnTaskExec, this, "+CDwnTaskExec::Shutdown");

    _fShutdown = TRUE;
    Verify(SetEvent(_hevWait));
    super::Shutdown();

    PerfDbgLog(tagDwnTaskExec, this, "-CDwnTaskExec::Shutdown");
}

#if DBG==1

void
CDwnTaskExec::Invariant()
{
    EnterCriticalSection();

    LONG        cDwnTask        = 0;
    LONG        cDwnTaskActive  = 0;
    CDwnTask *  pDwnTaskPrev    = NULL;
    CDwnTask *  pDwnTask        = _pDwnTaskHead;
    BOOL        fFoundCur       = _pDwnTaskCur == NULL;
    BOOL        fFoundRun       = _pDwnTaskRun == NULL;

    Assert(!_pDwnTaskHead == !_pDwnTaskTail);
    Assert(!_pDwnTaskHead || !_pDwnTaskHead->_pDwnTaskPrev);
    Assert(!_pDwnTaskTail || !_pDwnTaskTail->_pDwnTaskNext);

    for (; pDwnTask; pDwnTaskPrev = pDwnTask, pDwnTask = pDwnTask->_pDwnTaskNext)
    {
        cDwnTask += 1;

        if (pDwnTask->_fActive)
            cDwnTaskActive += 1;

        Assert(pDwnTask->_pDwnTaskPrev == pDwnTaskPrev);
        Assert(pDwnTask->_fEnqueued);

        if (pDwnTask == _pDwnTaskCur)
            fFoundCur = TRUE;

        if (pDwnTask == _pDwnTaskRun)
            fFoundRun = TRUE;
    }

    Assert(pDwnTaskPrev == _pDwnTaskTail);
    Assert(cDwnTask == _cDwnTask);
    Assert(cDwnTaskActive == _cDwnTaskActive);
    Assert(fFoundCur);
    Assert(fFoundRun);

    LeaveCriticalSection();
}

#endif

// External Functions ---------------------------------------------------------

HRESULT
StartDwnTask(CDwnTask * pDwnTask)
{
    PerfDbgLog1(tagDwnTaskExec, NULL, "+StartDwnTask %lX", pDwnTask);

    HRESULT hr = S_OK;

    g_csDwnTaskExec.Enter();

    if (g_pDwnTaskExec == NULL)
    {
        g_pDwnTaskExec = new CDwnTaskExec(g_csDwnTaskExec.GetPcs());

        if (g_pDwnTaskExec == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(g_pDwnTaskExec->Launch());

        if (hr)
        {
            g_pDwnTaskExec->Release();
            g_pDwnTaskExec = NULL;
            goto Cleanup;
        }
    }

    g_pDwnTaskExec->AddTask(pDwnTask);

Cleanup:
    g_csDwnTaskExec.Leave();
    PerfDbgLog(tagDwnTaskExec, NULL, "-StartDwnTask");
    RRETURN(hr);
}

void
KillDwnTaskExec()
{
    PerfDbgLog(tagDwnTaskExec, NULL, "+KillDwnTaskExec");

    g_csDwnTaskExec.Enter();

    CDwnTaskExec * pDwnTaskExec = g_pDwnTaskExec;
    g_pDwnTaskExec = NULL;

    g_csDwnTaskExec.Leave();

    if (pDwnTaskExec)
    {
        pDwnTaskExec->Shutdown();
        pDwnTaskExec->Release();
    }

    PerfDbgLog(tagDwnTaskExec, NULL, "-KillDwnTaskExec");
}
