//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dwnutil.cxx
//
//  Contents:   CBaseFT
//              CExecFT
//              CDwnChan
//              CDwnStm
//              CDwnCtx
//              CDwnInfo
//              CDwnLoad
//              MIMEINFO
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_BITS_HXX_
#define X_BITS_HXX_
#include "bits.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_MARKUPCTX_HXX_
#define X_MARKUPCTX_HXX_
#include "markupctx.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifdef _MAC
STDAPI_(unsigned long)  RegisterMimeFormat(LPCSTR szFormat);
#ifdef __cplusplus
extern "C" {
#endif
WINBASEAPI BOOL WINAPI AfxTerminateThread(HANDLE hThread, DWORD dwExitCode);
#ifdef __cplusplus
}
#endif
#endif // _MAC

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnChan,   "Dwn",  "Trace CDwnChan")
PerfDbgTag(tagDwnStm,    "Dwn",  "Trace CDwnStm")
PerfDbgTag(tagDwnStmStm, "Dwn",  "Trace CDwnStmStm")
PerfDbgTag(tagDwnCtx,    "Dwn",  "Trace CDwnCtx")
PerfDbgTag(tagDwnInfo,   "Dwn",  "Trace CDwnInfo")
PerfDbgTag(tagDwnLoad,   "Dwn",  "Trace CDwnLoad")

// Performance Meters ---------------------------------------------------------

MtDefine(Dwn, Mem, "Download")
MtDefine(CDwnStm, Dwn, "CDwnStm")
MtDefine(CDwnStm_pbuf, CDwnStm, "CDwnStm buffer")
MtDefine(CDwnStmStm, Dwn, "CDwnStmStm")
MtDefine(CDwnInfo, Dwn, "CDwnInfo")
MtDefine(CDwnInfoGetFile, CDwnInfo, "CDwnInfo::GetFile")
MtDefine(CDwnInfo_arySink_pv, Dwn, "CDwnInfo::_arySink::_pv")

// Globals --------------------------------------------------------------------

DEFINE_CRITICAL(g_csDwnBindPend, 0); // CDwnBindData (SetPending, SetEof)
DEFINE_CRITICAL(g_csDwnChanSig,  0); // CDwnChan (Signal)
DEFINE_CRITICAL(g_csDwnStm,      0); // CDwnStm
DEFINE_CRITICAL(g_csDwnDoc,      0); // CDwnDoc
DEFINE_CRITICAL(g_csDwnPost,     0); // CDwnPost (GetStgMedium)
DEFINE_CRITICAL(g_csHtmSrc,      1); // CHtmInfo (OnSource, ReadSource)
DEFINE_CRITICAL(g_csDwnBindSig,  1); // CDwnBind (Signal)
DEFINE_CRITICAL(g_csDwnBindTerm, 1); // CDwnBind (Terminate)
DEFINE_CRITICAL(g_csDwnCache,    1); // CDwnInfo active and cache lists
DEFINE_CRITICAL(g_csDwnTaskExec, 1); // CDwnTaskExec
DEFINE_CRITICAL(g_csImgTaskExec, 1); // CImgTaskExec
DEFINE_CRITICAL(g_csImgTransBlt, 1); // CImgBitsDIB::StretchBlt

// CDwnCrit -------------------------------------------------------------------

#if DBG==1

CDwnCrit * g_pDwnCritHead;

CDwnCrit::CDwnCrit(LPCSTR pszName, UINT cLevel)
{
    ::InitializeCriticalSection(GetPcs());

    _pszName        = pszName;
    _cLevel         = cLevel;
    _dwThread       = 0;
    _cEnter         = 0;
    _pDwnCritNext   = g_pDwnCritHead;
    g_pDwnCritHead  = this;
}

void
CDwnCrit::Enter()
{
    ::EnterCriticalSection(GetPcs());

    Assert(_dwThread == 0 || _dwThread == GetCurrentThreadId());

    if (_dwThread == 0)
    {
        _dwThread = GetCurrentThreadId();

        CDwnCrit * pDwnCrit = g_pDwnCritHead;

        for (; pDwnCrit; pDwnCrit = pDwnCrit->_pDwnCritNext)
        {
            if (pDwnCrit == this)
                continue;

            if (pDwnCrit->_dwThread != GetCurrentThreadId())
                continue;
                
            if (pDwnCrit->_cLevel > _cLevel)
                continue;

            char ach[256];

            wsprintfA(ach, "CDwnCrit: %s (%d) -> %s (%d) deadlock potential",
                pDwnCrit->_pszName, pDwnCrit->_cLevel, _pszName ? _pszName : "", _cLevel);

            AssertSz(0, ach);
        }
    }

    _cEnter += 1;
}

void
CDwnCrit::Leave()
{
    Assert(_dwThread == GetCurrentThreadId());
    Assert(_cEnter > 0);

    if (--_cEnter == 0)
    {
        _dwThread = 0;
    }

    ::LeaveCriticalSection(GetPcs());
}

#endif

CDwnCrit::CDwnCrit()
{
    ::InitializeCriticalSection(GetPcs());

#if DBG==1
    _pszName        = NULL;
    _cLevel         = (UINT)-1;
    _dwThread       = 0;
    _cEnter         = 0;
    _pDwnCritNext   = NULL;
#endif
}

CDwnCrit::~CDwnCrit()
{
    Assert(_dwThread == 0);
    Assert(_cEnter == 0);
    ::DeleteCriticalSection(GetPcs());
}

// CBaseFT --------------------------------------------------------------------

CBaseFT::CBaseFT(CRITICAL_SECTION * pcs)
{
    _ulRefs     = 1;
    _ulAllRefs  = 1;
    _pcs        = pcs;
    IncrementSecondaryObjectCount(10);
}

CBaseFT::~CBaseFT()
{
    AssertSz(_ulAllRefs == 0, "Ref count messed up in derived dtor?");
    AssertSz(_ulRefs == 0, "Ref count messed up in derived dtor?");
    DecrementSecondaryObjectCount(10);
}

void CBaseFT::Passivate()
{
    AssertSz(_ulRefs == 0,
        "CBaseFT::Passivate called unexpectedly or refcnt "
        "messed up in derived Passivate");
}

ULONG CBaseFT::Release()
{
    ULONG ulRefs = (ULONG)InterlockedDecrement((LONG *)&_ulRefs);

    if (ulRefs == 0)
    {
        Passivate();
        AssertSz(_ulRefs==0, "CBaseFT::AddRef occured after last release");
        SubRelease();
    }

    return(ulRefs);
}

ULONG CBaseFT::SubRelease()
{
    ULONG ulRefs = (ULONG)InterlockedDecrement((LONG *)&_ulAllRefs);

    if (ulRefs == 0)
    {
        delete this;
    }

    return(ulRefs);
}

#if DBG==1

void CBaseFT::EnterCriticalSection()
{
    if (_pcs)
    {
        ::EnterCriticalSection(_pcs);

        Assert(_dwThread == 0 || _dwThread == GetCurrentThreadId());

        if (_dwThread == 0)
        {
            _dwThread = GetCurrentThreadId();
        }

        _cEnter += 1;
    }
}

void CBaseFT::LeaveCriticalSection()
{
    if (_pcs)
    {
        Assert(_dwThread == GetCurrentThreadId());
        Assert(_cEnter > 0);

        if (--_cEnter == 0)
        {
            _dwThread = 0;
        }

        ::LeaveCriticalSection(_pcs);
    }
}

BOOL CBaseFT::EnteredCriticalSection()
{
    //$BUGBUG (dinartem) Doesn't work across shared _pcs
    return(TRUE);
//    return(!_pcs || _dwThread == GetCurrentThreadId());
}

#endif

// CExecFT --------------------------------------------------------------------

CExecFT::CExecFT(CRITICAL_SECTION * pcs)
    : CBaseFT(pcs)
{
    _hThread = NULL;
    _hEvent  = NULL;
    _hrInit  = S_OK;
}

CExecFT::~CExecFT()
{
    CloseThread(_hThread);
    CloseEvent(_hEvent);
}

void CExecFT::Passivate()
{
}

DWORD WINAPI IF_WIN16(__loadds)
CExecFT::StaticThreadProc(void * pv)
{
    return(((CExecFT *)pv)->ThreadProc());
}

DWORD CExecFT::ThreadProc()
{
    _hrInit = ThreadInit();

    if (_hEvent)
    {
        Verify(SetEvent(_hEvent));
    }

    if (_hrInit == S_OK)
    {
        StartCAP();
        ThreadExec();
        StopCAP();
    }

    ThreadTerm();
    ThreadExit();
    return(0);
}

void CExecFT::ThreadExit()
{
    SubRelease();
}

HRESULT CExecFT::Launch(BOOL fWait)
{
    DWORD dwResult;
#ifdef WIN16
    DWORD dwStackSize = 0x3000;
#else
    DWORD dwStackSize = 0;
#endif

    if (fWait)
    {
        _hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

        if (_hEvent == NULL)
            RRETURN(GetLastWin32Error());
    }

    SubAddRef();

    _hThread = CreateThread(NULL, dwStackSize, &CExecFT::StaticThreadProc, this, 0, &_dwThreadId);

    if (_hThread == NULL)
    {
        SubRelease();
        RRETURN(GetLastWin32Error());
    }

    if (fWait)
    {
        dwResult = WaitForSingleObject(_hEvent, INFINITE);

        Assert(dwResult == WAIT_OBJECT_0);

        CloseEvent(_hEvent);
        _hEvent = NULL;

        RRETURN(_hrInit);
    }

    return(S_OK);
}

void CExecFT::Shutdown(DWORD dwTimeOut)
{
    if (_hThread && GetCurrentThreadId() != _dwThreadId)
    {
        DWORD dwExitCode;

        WaitForSingleObject(_hThread, dwTimeOut);

#ifndef WIN16
        if (    GetExitCodeThread(_hThread, &dwExitCode)
            &&  dwExitCode == STILL_ACTIVE)
        {
            TerminateThread(_hThread, 1);
        }
#endif // ndef WIN16
    }
}

// CDwnChan -------------------------------------------------------------------

CDwnChan::CDwnChan(CRITICAL_SECTION * pcs)
    : super(pcs)
{
    PerfDbgLog(tagDwnChan, this, "CDwnChan::CDwnChan");

    _fSignalled = TRUE;
}

void
CDwnChan::Passivate()
{
    PerfDbgLog(tagDwnChan, this, "+CDwnChan::Passivate");

    Disconnect();

    PerfDbgLog(tagDwnChan, this, "-CDwnChan::Passivate");
}

void
CDwnChan::SetCallback(PFNDWNCHAN pfnCallback, void * pvCallback)
{
    PerfDbgLog(tagDwnChan, this, "+CDwnChan::SetCallback %s");

#ifdef OBJCNTCHK
    DWORD dwObjCnt;
#endif

    HRESULT hr = AddRefThreadState(&dwObjCnt);

    Disconnect();

    if (hr == S_OK)
    {
        Assert(_fSignalled);

        _pts          = GetThreadState();
        _pfnCallback  = pfnCallback;
        _pvCallback   = pvCallback;
        _fSignalled   = FALSE;

#ifdef OBJCNTCHK
        _dwObjCnt     = dwObjCnt;
#endif
    }

    PerfDbgLog(tagDwnChan, this, "-CDwnChan::SetCallback");
}

void
CDwnChan::Disconnect()
{
    if (_pts && (_pts == GetThreadState()))
    {
        PerfDbgLog(tagDwnChan, this, "+CDwnChan::Disconnect");

        THREADSTATE * pts;
        BOOL fSignalled;

        g_csDwnChanSig.Enter();

        fSignalled   = _fSignalled;
        pts          = _pts;
        _pts         = NULL;
        _pfnCallback = NULL;
        _pvCallback  = NULL;
        _fSignalled  = TRUE;

        g_csDwnChanSig.Leave();

        if (fSignalled)
        {
            GWKillMethodCallEx(pts, this, ONCALL_METHOD(CDwnChan, OnMethodCall, onmethodcall), 0);
        }

        ReleaseThreadState(&_dwObjCnt);

        PerfDbgLog(tagDwnChan, this, "-CDwnChan::Disconnect");
    }
}

void CDwnChan::Signal()
{
    if (!_fSignalled)
    {
        PerfDbgLog(tagDwnChan, this, "+CDwnChan::Signal");

        g_csDwnChanSig.Enter();

        if (!_fSignalled)
        {
            _fSignalled = TRUE;
            GWPostMethodCallEx(_pts, this, ONCALL_METHOD(CDwnChan, OnMethodCall, onmethodcall), 0, FALSE, GetOnMethodCallName());
        }

        g_csDwnChanSig.Leave();

        PerfDbgLog(tagDwnChan, this, "-CDwnChan::Signal");
    }
}

void CDwnChan::OnMethodCall(DWORD_PTR dwContext)
{
    PerfDbgLog(tagDwnChan, this, "+CDwnChan::OnMethodCall");

    if (_fSignalled)
    {
        _fSignalled = FALSE;
        _pfnCallback(this, _pvCallback);
    }

    PerfDbgLog(tagDwnChan, this, "-CDwnChan::OnMethodCall");
}

// CDwnStm --------------------------------------------------------------------

CDwnStm::CDwnStm(UINT cbBuf)
    : CDwnChan(g_csDwnStm.GetPcs())
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::CDwnStm");

    _cbBuf = cbBuf;

    PerfDbgLog(tagDwnStm, this, "-CDwnStm::CDwnStm");
}

CDwnStm::~CDwnStm()
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::~CDwnStm");

    BUF * pbuf;
    BUF * pbufNext;

    for (pbuf = _pbufHead; pbuf; pbuf = pbufNext)
    {
        pbufNext = pbuf->pbufNext;
        MemFree(pbuf);
    }

    PerfDbgLog(tagDwnStm, this, "-CDwnStm::~CDwnStm");
}

HRESULT
CDwnStm::SetSeekable()
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::SetSeekable");

    _fSeekable = TRUE;

    PerfDbgLog1(tagDwnStm, this, "-CDwnStm::SetSeekable (hr=%lX)", S_OK);
    return(S_OK);
}

HRESULT CDwnStm::Write(void * pv, ULONG cb)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::Write (cb=%ld)", cb);

    void *  pvW;
    ULONG   cbW;
    HRESULT hr = S_OK;

    while (cb > 0)
    {
        hr = WriteBeg(&pvW, &cbW);
        if (hr)
            goto Cleanup;

        if (cbW > cb)
            cbW = cb;

        memcpy(pvW, pv, cbW);

        WriteEnd(cbW);

        pv = (BYTE *)pv + cbW;
        cb = cb - cbW;
    }

Cleanup:
    PerfDbgLog1(tagDwnStm, this, "-CDwnStm::Write (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT CDwnStm::WriteBeg(void ** ppv, ULONG * pcb)
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::WriteBeg");

    BUF * pbuf = _pbufWrite;
    HRESULT hr = S_OK;

    if (pbuf == NULL)
    {
        pbuf = (BUF *)MemAlloc(Mt(CDwnStm_pbuf), offsetof(BUF, ab) + _cbBuf);

        if (pbuf == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pbuf->pbufNext = NULL;
        pbuf->ib = 0;
        pbuf->cb = _cbBuf;

        g_csDwnStm.Enter();

        if (_pbufTail == NULL)
        {
            _pbufHead = pbuf;
            _pbufTail = pbuf;
        }
        else
        {
            _pbufTail->pbufNext = pbuf;
            _pbufTail = pbuf;
        }

        if (_pbufRead == NULL)
        {
            Assert(_ibRead == 0);
            _pbufRead = pbuf;
        }

        g_csDwnStm.Leave();

        _pbufWrite = pbuf;
    }

    Assert(pbuf->cb > pbuf->ib);

    *ppv = &pbuf->ab[pbuf->ib];
    *pcb = pbuf->cb - pbuf->ib;

Cleanup:
    PerfDbgLog2(tagDwnStm, this, "-CDwnStm::WriteBeg (hr=%lX,*pcb=%ld)", hr, *pcb);
    RRETURN(hr);
}

void CDwnStm::WriteEnd(ULONG cb)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::WriteEnd (cb=%ld)", cb);

    if (cb > 0)
    {
        BUF *   pbuf = _pbufWrite;
        ULONG   ib   = pbuf->ib + cb;

        Assert(ib <= pbuf->cb);

        if (ib >= pbuf->cb)
        {
            _pbufWrite = NULL;
        }

        // As soon as pbuf->ib is written and matches pbuf->cb, the reader
        // can asynchronously read and free the buffer.  Therefore, pbuf
        // cannot be accessed after this next line.

        pbuf->ib  = ib;
        _cbWrite += cb;

        Signal();
    }

    PerfDbgLog(tagDwnStm, this, "-CDwnStm::WriteEnd");
}

void CDwnStm::WriteEof(HRESULT hrEof)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::WriteEof (hrEof=%lX)", hrEof);

    if (!_fEof || hrEof)
    {
        _hrEof = hrEof;
        _fEof  = TRUE;
        Signal();
    }

    PerfDbgLog(tagDwnStm, this, "-CDwnStm::WriteEof");
}

HRESULT CDwnStm::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::Read (cb=%ld)", cb);

    void *  pvR;
    ULONG   cbR;
    ULONG   cbRead  = 0;
    HRESULT hr      = S_OK;

    while (cb > 0)
    {
        hr = ReadBeg(&pvR, &cbR);
        if (hr)
            break;

        if (cbR == 0)
            break;

        if (cbR > cb)
            cbR = cb;

        memcpy(pv, pvR, cbR);

        pv = (BYTE *)pv + cbR;
        cb = cb - cbR;
        cbRead += cbR;

        ReadEnd(cbR);
    }

    *pcbRead = cbRead;

    PerfDbgLog2(tagDwnStm, this, "-CDwnStm::Read (hr=%lX,*pcb=%ld)", hr, *pcbRead);
    RRETURN(hr);
}

HRESULT CDwnStm::ReadBeg(void ** ppv, ULONG * pcb)
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::ReadBeg");

    BUF * pbuf = _pbufRead;
    HRESULT hr = S_OK;

    if (_fEof && _hrEof != S_OK)
    {
        *ppv = NULL;
        *pcb = 0;
        hr   = _hrEof;
        goto Cleanup;
    }

    if (pbuf)
    {
        Assert(_ibRead <= pbuf->ib);

        *ppv = &pbuf->ab[_ibRead];
        *pcb = pbuf->ib - _ibRead;
    }
    else
    {
        *ppv = NULL;
        *pcb = 0;
    }

Cleanup:
    PerfDbgLog2(tagDwnStm, this, "-CDwnStm::ReadBeg (hr=%lX,*pcb=%ld)", hr, *pcb);
    return(S_OK);
}

void CDwnStm::ReadEnd(ULONG cb)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::ReadEnd (cb=%ld)", cb);

    BUF * pbuf = _pbufRead;

    Assert(pbuf);
    Assert(_ibRead + cb <= pbuf->ib);

    _ibRead += cb;

    if (_ibRead >= pbuf->cb)
    {
        _ibRead = 0;

        g_csDwnStm.Enter();

        _pbufRead = pbuf->pbufNext;

        if (!_fSeekable)
        {
            _pbufHead = _pbufRead;
            
            if (_pbufHead == NULL)
                _pbufTail = NULL;
        }

        g_csDwnStm.Leave();

        if (!_fSeekable)
        {
            MemFree(pbuf);
        }
    }

    _cbRead += cb;

    PerfDbgLog(tagDwnStm, this, "-CDwnStm::ReadEnd");
}

BOOL CDwnStm::ReadEof(HRESULT * phrEof)
{
    if (_fEof && (_hrEof || _cbRead == _cbWrite))
    {
        PerfDbgLog1(tagDwnStm, this, "CDwnStm::ReadEof (TRUE,hrEof=%lX)", _hrEof);
        *phrEof = _hrEof;
        return(TRUE);
    }
    else
    {
        PerfDbgLog(tagDwnStm, this, "CDwnStm::ReadEof (FALSE)");
        *phrEof = S_OK;
        return(FALSE);
    }
}

HRESULT CDwnStm::Seek(ULONG ib)
{
    PerfDbgLog1(tagDwnStm, this, "+CDwnStm::Seek (ib=%ld)", ib);

    BUF *   pbuf;
    ULONG   cb;
    HRESULT hr = S_OK;
    
    if (!_fSeekable || ib > _cbWrite)
    {
        Assert(FALSE);
        hr = E_FAIL;
        goto Cleanup;
    }

    pbuf = _pbufHead;
    cb   = ib;

    if (pbuf)
    {
        while (cb > pbuf->cb)
        {
            cb  -= pbuf->cb;
            pbuf = pbuf->pbufNext;
        }
    }
    
    g_csDwnStm.Enter();

    if (!pbuf || cb < pbuf->cb)
    {
        _pbufRead = pbuf;
        _ibRead   = cb;
    }
    else
    {
        _pbufRead = pbuf->pbufNext;
        _ibRead   = 0;
    }

    g_csDwnStm.Leave();

    _cbRead = ib;

Cleanup:
    PerfDbgLog1(tagDwnStm, this, "-CDwnStm::Seek (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CDwnStm::CopyStream(IStream * pstm, ULONG * pcbCopy)
{
    PerfDbgLog(tagDwnStm, this, "+CDwnStm::CopyStream");

    void *  pv;
    ULONG   cbW;
    ULONG   cbR;
    ULONG   cbRTotal;
    HRESULT hr = S_OK;

    cbRTotal = 0;
    
    for (;;)
    {
        hr = THR(WriteBeg(&pv, &cbW));
        if (hr)
            goto Cleanup;

        Assert(cbW > 0);

        hr = THR(pstm->Read(pv, cbW, &cbR));
        if (FAILED(hr))
            goto Cleanup;

        hr = S_OK;

        Assert(cbR <= cbW);

        WriteEnd(cbR);

        cbRTotal += cbR;

        if (cbR == 0)
            break;
    }

Cleanup:
    if (pcbCopy)
        *pcbCopy = cbRTotal;
        
    PerfDbgLog2(tagDwnStm, this, "+CDwnStm::CopyStream (hr=%lX,*pcb=%ld)", hr, cbRTotal);
    RRETURN(hr);
}

// CDwnStmStm -----------------------------------------------------------------

class CDwnStmStm : public CBaseFT, public IStream
{
    typedef CBaseFT super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnStmStm))

                    CDwnStmStm(CDwnStm * pDwnStm);
    virtual void    Passivate();

    // IUnknown methods

    STDMETHOD (QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IStream

    STDMETHOD(Clone)(IStream ** ppStream);
    STDMETHOD(Commit)(DWORD dwFlags);
    STDMETHOD(CopyTo)(IStream * pStream, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWrite);
    STDMETHOD(LockRegion)(ULARGE_INTEGER ib, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Read)(void HUGEP * pv, ULONG cb, ULONG * pcb);
    STDMETHOD(Revert)();
    STDMETHOD(Seek)(LARGE_INTEGER ib, DWORD dwOrigin, ULARGE_INTEGER * pib);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(Stat)(STATSTG * pstatstg, DWORD dwFlags);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER ib, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Write)(const void HUGEP * pv, ULONG cb, ULONG * pcb);

protected:

    CDwnStm *       _pDwnStm;
    ULONG           _ib;

};

CDwnStmStm::CDwnStmStm(CDwnStm * pDwnStm)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::CDwnStmStm");

    _pDwnStm = pDwnStm;
    _pDwnStm->AddRef();

    PerfDbgLog(tagDwnStmStm, this, "-CDwnStmStm::CDwnStmStm");
}

void
CDwnStmStm::Passivate(void)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Passivate");

    _pDwnStm->Release();

    super::Passivate();

    PerfDbgLog(tagDwnStmStm, this, "-CDwnStmStm::Passivate");
}

STDMETHODIMP
CDwnStmStm::QueryInterface(REFIID iid, void ** ppv)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::QueryInterface");

    HRESULT hr;

    if (iid == IID_IUnknown || iid == IID_IStream)
    {
        *ppv = (IStream *)this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::QueryInterface (hr=%lX)", hr);
    return(hr);
}

STDMETHODIMP_(ULONG)
CDwnStmStm::AddRef()
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::AddRef");

    ULONG ulRefs = super::AddRef();

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::AddRef (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

STDMETHODIMP_(ULONG)
CDwnStmStm::Release()
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Release");

    ULONG ulRefs = super::Release();

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Release (cRefs=%ld)", ulRefs);
    return(ulRefs);
}

STDMETHODIMP CDwnStmStm::Clone(IStream ** ppStream)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Clone");

    HRESULT hr = E_NOTIMPL;
    *ppStream = NULL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Clone (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::Commit(DWORD dwFlags)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Commit");

    HRESULT hr = S_OK;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Commit (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::CopyTo(IStream * pStream, ULARGE_INTEGER cb,
    ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWrite)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::CopyTo");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::CopyTo (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::LockRegion(ULARGE_INTEGER ib, ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::LockRegion");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::LockRegion (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::Read(void HUGEP * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnStmStm, this, "+CDwnStmStm::Read (cbReq=%ld)", cb);

    ULONG   cbRead;
    HRESULT hr;

    if (pcb == NULL)
        pcb = &cbRead;

    *pcb = 0;

    hr = THR(_pDwnStm->Seek(_ib));
    if (hr)
        goto Cleanup;

    hr = THR(_pDwnStm->Read(pv, cb, pcb));
    if (hr)
        goto Cleanup;

    _ib += *pcb;

    if (*pcb == 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    PerfDbgLog2(tagDwnStmStm, this, "-CDwnStmStm::Read (*pcb=%ld,hr=%lX)", *pcb, hr);
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP CDwnStmStm::Revert()
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Revert");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Revert (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::Seek(LARGE_INTEGER ib, DWORD dwOrigin,
    ULARGE_INTEGER * pib)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Seek");

    HRESULT hr = E_NOTIMPL;

    if (dwOrigin == STREAM_SEEK_SET)
    {
        _ib = ib.LowPart;
        hr = S_OK;
    }

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Seek (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::SetSize(ULARGE_INTEGER cb)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::SetSize");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::SetSize (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnStmStm::Stat(STATSTG * pstatstg, DWORD dwFlags)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Stat");

    memset(pstatstg, 0, sizeof(STATSTG));

    pstatstg->type           = STGTY_STREAM;
    pstatstg->grfMode        = STGM_READ;
    pstatstg->cbSize.LowPart = _pDwnStm->Size();

    PerfDbgLog(tagDwnStmStm, this, "-CDwnStmStm::Stat (hr=0)");
    return(S_OK);
}

STDMETHODIMP CDwnStmStm::UnlockRegion(ULARGE_INTEGER ib, ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::UnlockRegion");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::UnlockRegion (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP CDwnStmStm::Write(const void HUGEP * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog(tagDwnStmStm, this, "+CDwnStmStm::Write");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagDwnStmStm, this, "-CDwnStmStm::Write (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CreateStreamOnDwnStm(CDwnStm * pDwnStm, IStream ** ppStream)
{
    *ppStream = new CDwnStmStm(pDwnStm);
    RRETURN(*ppStream ? S_OK : E_OUTOFMEMORY);
}

// CDwnCtx --------------------------------------------------------------------

#if DBG==1

void
CDwnCtx::EnterCriticalSection()
{
    ((CDwnCrit *)GetPcs())->Enter();
}

void
CDwnCtx::LeaveCriticalSection()
{
    ((CDwnCrit *)GetPcs())->Leave();
}

BOOL
CDwnCtx::EnteredCriticalSection()
{
    return(((CDwnCrit *)GetPcs())->IsEntered());
}

#endif

void
CDwnCtx::Passivate()
{
    PerfDbgLog(tagDwnCtx, this, "+CDwnCtx::Passivate");

    SetLoad(FALSE, NULL, FALSE);

    super::Passivate();

    if (_pDwnInfo)
        _pDwnInfo->DelDwnCtx(this);

    ClearInterface(&_pProgSink);

    PerfDbgLog(tagDwnCtx, this, "-CDwnCtx::Passivate");
}

LPCTSTR
CDwnCtx::GetUrl()
{
    return(_pDwnInfo ? _pDwnInfo->GetUrl() : g_Zero.ach);
}

MIMEINFO *
CDwnCtx::GetMimeInfo()
{
    return(_pDwnInfo ? _pDwnInfo->GetMimeInfo() : NULL);
}

HRESULT
CDwnCtx::GetFile(LPTSTR * ppch)
{
    *ppch = NULL;
    RRETURN(_pDwnInfo ? _pDwnInfo->GetFile(ppch) : E_FAIL);
}

FILETIME
CDwnCtx::GetLastMod()
{
    if (_pDwnInfo)
    {
        return _pDwnInfo->GetLastMod();
    }
    else
    {
        FILETIME ftZ = {0};
        return ftZ;
    }
}

DWORD
CDwnCtx::GetSecFlags()
{
    return(_pDwnInfo ? _pDwnInfo->GetSecFlags() : 0);
}

HRESULT
CDwnCtx::SetProgSink(IProgSink * pProgSink)
{
    PerfDbgLog(tagDwnCtx, this, "+CDwnCtx::SetProgSink");

    HRESULT     hr = S_OK;

    EnterCriticalSection();

#if DBG == 1
    if (pProgSink)
    {
        if (!_pDwnInfo)
        {
            TraceTag((tagWarning, "CDwnCtx::SetProgSink called with no _pDwnInfo"));
            TraceCallers(tagWarning, 0, 6);
        }
        else
        {
            if (_pDwnInfo->GetFlags(DWNF_STATE) & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            {
                TraceTag((tagWarning, "CDwnCtx::SetProgSink called when _pDwnInfo is already done"));
                TraceCallers(tagWarning, 0, 6);
            }
        }
    }
#endif

    if (_pDwnInfo)
    {
        if (pProgSink)
        {
            hr = THR(_pDwnInfo->AddProgSink(pProgSink));
            if (hr)
                goto Cleanup;
        }

        if (_pProgSink)
        {
            _pDwnInfo->DelProgSink(_pProgSink);
        }

    }

    ReplaceInterface(&_pProgSink, pProgSink);

Cleanup:
    LeaveCriticalSection();
    PerfDbgLog1(tagDwnCtx, this, "-CDwnCtx::SetProgSink (hr=%lX)", hr);
    RRETURN(hr);
}

ULONG
CDwnCtx::GetState(BOOL fClear)
{
    PerfDbgLog1(tagDwnCtx, this, "+CDwnCtx::GetState (fClear=%s)",
        fClear ? "TRUE" : "FALSE");

    DWORD dwState;

    EnterCriticalSection();

    dwState = _wChg;

    if (_pDwnInfo)
    {
        dwState |= _pDwnInfo->GetFlags(DWNF_STATE);
    }

    if (fClear)
    {
        _wChg = 0;
    }

    LeaveCriticalSection();

    PerfDbgLog1(tagDwnCtx, this, "-CDwnCtx::GetState (dwState=%08lX)", dwState);
    return(dwState);
}

void
CDwnCtx::Signal(WORD wChg)
{
    PerfDbgLog1(tagDwnCtx, this, "+CDwnCtx::Signal (wChg=%04lX)", wChg);

    Assert(EnteredCriticalSection());

    wChg &= _wChgReq;   // Only light up requested bits
    wChg &= ~_wChg;     // Don't light up bits already on

    if (wChg)
    {
        _wChg |= (WORD)wChg;
        super::Signal();
    }

    PerfDbgLog(tagDwnCtx, this, "-CDwnCtx::Signal");
}

void
CDwnCtx::SetLoad(BOOL fLoad, DWNLOADINFO * pdli, BOOL fReload)
{
    PerfDbgLog2(tagDwnCtx, this, "+CDwnCtx::SetLoad (fLoad=%s,fReload=%s)",
        fLoad ? "TRUE" : "FALSE", fReload ? "TRUE" : "FALSE");

    if (    !!fLoad != !!_fLoad
        ||  (fLoad && _fLoad && fReload))
    {
        if (    fLoad 
            &&  !pdli->pDwnBindData 
            &&  !pdli->pmk 
            &&  !pdli->pstm 
            &&  !pdli->pchUrl
            &&  !pdli->fClientData)
        {
            pdli->pchUrl = _pDwnInfo->GetUrl();
        }

        _pDwnInfo->SetLoad(this, fLoad, fReload, pdli);
    }

    PerfDbgLog(tagDwnCtx, this, "-CDwnCtx::SetLoad");
}

CDwnLoad *      
CDwnCtx::GetDwnLoad()
{
    CDwnLoad * pDwnLoadRet = NULL;

    EnterCriticalSection();
    if (_pDwnInfo && _pDwnInfo->_pDwnLoad)
    {
        pDwnLoadRet = _pDwnInfo->_pDwnLoad;
        pDwnLoadRet->AddRef();
    }
    LeaveCriticalSection();

    return pDwnLoadRet;
}


HRESULT
NewDwnCtx(UINT dt, BOOL fLoad, DWNLOADINFO * pdli, CDwnCtx ** ppDwnCtx)
{
    PerfDbgLog2(tagDwnCtx, NULL, "+NewDwnCtx (dt=%d,fLoad=%s)", dt, fLoad ? "TRUE" : "FALSE");

    CDwnInfo *  pDwnInfo;
    CDwnCtx *   pDwnCtx;
    HRESULT     hr;

    hr = THR(CDwnInfo::Create(dt, pdli, &pDwnInfo));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnInfo->NewDwnCtx(&pDwnCtx));

    if (hr == S_OK)
    {
        pDwnInfo->AddDwnCtx(pDwnCtx);        
    }

    pDwnInfo->Release();

    if (hr)
        goto Cleanup;

    if (fLoad)
    {
        pDwnCtx->SetLoad(TRUE, pdli, FALSE);
    }

    *ppDwnCtx = pDwnCtx;

Cleanup:
    PerfDbgLog1(tagDwnCtx, NULL, "-NewDwnCtx (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnInfo -------------------------------------------------------------------

CDwnInfo::CDwnInfo()
    : CBaseFT(&_cs)
{
}

CDwnInfo::~CDwnInfo()
{
    if (_pDwnInfoLock)
        _pDwnInfoLock->SubRelease();
}

void
CDwnInfo::Passivate()
{
    EnterCriticalSection();

#if DBG == 1
    _fPassive = TRUE;
#endif

    if (_arySink.Size() > 0)
    {
        SINKENTRY * pSinkEntry = _arySink;
        UINT cSink = _arySink.Size();

        for (; cSink > 0; --cSink, ++pSinkEntry)
        {
            if (pSinkEntry->dwCookie)
                pSinkEntry->pProgSink->DelProgress(pSinkEntry->dwCookie);
            pSinkEntry->pProgSink->Release();
        }

        _arySink.SetSize(0);
    }

    LeaveCriticalSection();
}

HRESULT
CDwnInfo::Init(DWNLOADINFO * pdli)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::Init");

    CDwnDoc * pDwnDoc = pdli->pDwnDoc;
    HRESULT hr;

    hr = THR(_cstrUrl.Set(pdli->pchUrl));
    if (hr)
        goto Cleanup;

    _dwBindf   = pDwnDoc->GetBindf();
    _dwRefresh = pdli->fResynchronize ? IncrementLcl() : pDwnDoc->GetRefresh();
    _dwFlags   = DWNLOAD_NOTLOADED | (pDwnDoc->GetDownf() & ~DWNF_STATE);

Cleanup:
    PerfDbgLog1(tagDwnInfo, this, "-CDwnInfo::Init (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnInfo::DelProgSinks()
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::DelProgSinks");

    EnterCriticalSection();

    for (CDwnCtx * pDwnCtx = _pDwnCtxHead; pDwnCtx;
            pDwnCtx = pDwnCtx->_pDwnCtxNext)
    {
        Assert(pDwnCtx->_pDwnInfo == this);
        pDwnCtx->SetProgSink(NULL);
    }

    LeaveCriticalSection();

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::DelProgSinks");
}


void
CDwnInfo::AddDwnCtx(CDwnCtx * pDwnCtx)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::AddDwnCtx");

    AddRef();

    EnterCriticalSection();

    pDwnCtx->_pDwnInfo    = this;
    pDwnCtx->_pDwnCtxNext = _pDwnCtxHead;
    _pDwnCtxHead          = pDwnCtx;
    pDwnCtx->SetPcs(GetPcs());

    LeaveCriticalSection();

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::AddDwnCtx");
}

void
CDwnInfo::DelDwnCtx(CDwnCtx * pDwnCtx)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::DelDwnCtx");

    EnterCriticalSection();

    CDwnCtx ** ppDwnCtx = &_pDwnCtxHead;

    for (; *ppDwnCtx; ppDwnCtx = &(*ppDwnCtx)->_pDwnCtxNext)
    {
        if (*ppDwnCtx == pDwnCtx)
        {
            *ppDwnCtx = pDwnCtx->_pDwnCtxNext;

            DelProgSink(pDwnCtx->_pProgSink);
            
            Assert(pDwnCtx->_pDwnInfo == this);
            pDwnCtx->_pDwnInfo = NULL;
            goto found;
        }
    }

    AssertSz(FALSE, "Couldn't find CDwnCtx in CDwnInfo list");

found:
    LeaveCriticalSection();

    Release();

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::DelDwnCtx");
}

HRESULT
CDwnInfo::GetFile(LPTSTR * ppch)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::GetFile");

    HRESULT hr;
    LPCTSTR pchUrl = GetUrl();

    *ppch = NULL;

    if (_tcsnipre(_T("file"), 4, pchUrl, -1))
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));
        if (hr)
            goto Cleanup;

        hr = THR(MemAllocString(Mt(CDwnInfoGetFile), achPath, ppch));
    }
    else
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(pchUrl, pInfo, &cInfo, 0))
        {
            DoUnlockUrlCacheEntryFile(pchUrl, 0);
            hr = THR(MemAllocString(Mt(CDwnInfoGetFile),
                        pInfo->lpszLocalFileName, ppch));
        }
        else
        {
            hr = E_FAIL;
        }
    }

Cleanup:
    PerfDbgLog1(tagDwnInfo, this, "+CDwnInfo::GetFile (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnInfo::Signal(WORD wChg)
{
    PerfDbgLog1(tagDwnInfo, this, "+CDwnInfo::Signal (wChg=%04lX)", wChg);

    if (_pDwnCtxHead)
    {
        EnterCriticalSection();

        for (CDwnCtx * pDwnCtx = _pDwnCtxHead; pDwnCtx;
                pDwnCtx = pDwnCtx->_pDwnCtxNext)
        {
            pDwnCtx->Signal(wChg);
        }

        LeaveCriticalSection();
    }

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::Signal");
}

void
CDwnInfo::SetLoad(CDwnCtx * pDwnCtx, BOOL fLoad, BOOL fReload,
    DWNLOADINFO * pdli)
{
    PerfDbgLog2(tagDwnInfo, this, "+CDwnInfo::SetLoad (fLoad=%d,fReload=%d)", fLoad, fReload);

    CDwnLoad * pDwnLoadOld = NULL;
    CDwnLoad * pDwnLoadNew = NULL;
    HRESULT hr = S_OK;

    Assert(!EnteredCriticalSection());

    EnterCriticalSection();

    int cLoad = fReload ? (pDwnCtx->_fLoad ? 0 : 1) : (fLoad ? 1 : -1);

    if (cLoad == 0 && !(TstFlags(DWNLOAD_ERROR | DWNLOAD_STOPPED)))
        goto Cleanup;

    Assert(!(_cLoad == 0 && cLoad == -1));

    _cLoad += cLoad;
    pDwnCtx->_fLoad = fLoad;

    #if DBG==1
    {
        UINT        cLoad    = 0;
        CDwnCtx *   pDwnCtxT = _pDwnCtxHead;

        for (; pDwnCtxT; pDwnCtxT = pDwnCtxT->_pDwnCtxNext)
        {
            cLoad += !!pDwnCtxT->_fLoad;
        }

        AssertSz(cLoad == _cLoad, "CDwnInfo _cLoad is inconistent with "
            "sum of CDwnCtx _fLoad");
    }
    #endif

    if (    (cLoad  > 0 && _cLoad == 1 && !TstFlags(DWNLOAD_COMPLETE))
        ||  (cLoad == 0 && _cLoad  > 0))
    {
        if (!TstFlags(DWNLOAD_NOTLOADED))
        {
            Abort(E_ABORT, &pDwnLoadOld);
            Reset();
        }
        else
        {
            StartProgress();
        }

        Assert(_pDwnLoad == NULL);

        UpdFlags(DWNLOAD_MASK, DWNLOAD_LOADING);

        hr = THR(NewDwnLoad(&_pDwnLoad));

        if (hr == S_OK)
            hr = THR(_pDwnLoad->Init(pdli, this));

        if (hr == S_OK)
        {
            pDwnLoadNew = _pDwnLoad;
            pDwnLoadNew->AddRef();
        }
        else
        {
            Abort(hr, &pDwnLoadOld);
        }
    }
    else if (cLoad < 0 && _cLoad == 0)
    {
        Abort(S_OK, &pDwnLoadOld);
    }

Cleanup:
    LeaveCriticalSection();

    Assert(!EnteredCriticalSection());

    if (pDwnLoadOld)
        pDwnLoadOld->Release();

    if (pDwnLoadNew)
    {
        pDwnLoadNew->SetCallback();
        pDwnLoadNew->Release();
    }
            
    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::SetLoad");
}

void
CDwnInfo::OnLoadDone(CDwnLoad * pDwnLoad, HRESULT hrErr)
{
    PerfDbgLog1(tagDwnInfo, this, "+CDwnInfo::OnLoadDone (hrErr=%lX)", hrErr);

    Assert(!EnteredCriticalSection());
    
    EnterCriticalSection();

    if (pDwnLoad == _pDwnLoad)
    {
        OnLoadDone(hrErr);
        _pDwnLoad = NULL;
    }
    else
    {
        pDwnLoad = NULL;
    }

    LeaveCriticalSection();

    if (pDwnLoad)
        pDwnLoad->Release();

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::OnLoadDone");
}

void
CDwnInfo::Abort(HRESULT hrErr, CDwnLoad ** ppDwnLoad)
{
    PerfDbgLog1(tagDwnInfo, this, "+CDwnInfo::Abort (hrErr=%lX)", hrErr);

    Assert(EnteredCriticalSection());

    if (TstFlags(DWNLOAD_LOADING))
    {
        UpdFlags(DWNLOAD_MASK, hrErr ? DWNLOAD_ERROR : DWNLOAD_STOPPED);
        Signal(IMGCHG_COMPLETE);
    }

    *ppDwnLoad = _pDwnLoad;
    _pDwnLoad  = NULL;

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::Abort");
}

HRESULT
CDwnInfo::AddProgSink(IProgSink * pProgSink)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::AddProgSink");

    Assert(EnteredCriticalSection());
    Assert(!_fPassive);

    SINKENTRY * pSinkEntry  = _arySink;
    UINT        cSink       = _arySink.Size();
    DWORD       dwCookie;
    HRESULT     hr = S_OK;

    for (; cSink > 0; --cSink, ++pSinkEntry)
    {
        if (pSinkEntry->pProgSink == pProgSink)
        {
            pSinkEntry->ulRefs += 1;
            goto Cleanup;
        }
    }

    hr = THR(_arySink.AppendIndirect(NULL, &pSinkEntry));
    if (hr)
        goto Cleanup;

    // Don't add the progress if we're still at the (nonpending) NOTLOADED state
    
    if (!TstFlags(DWNLOAD_NOTLOADED))
    {
        hr = THR(pProgSink->AddProgress(GetProgSinkClass(), &dwCookie));

        if (hr == S_OK && _pDwnLoad)
        {
            hr = THR(_pDwnLoad->RequestProgress(pProgSink, dwCookie));

            if (hr)
            {
                pProgSink->DelProgress(dwCookie);
                dwCookie = 0;
            }
        }
    }
    else
    {
        dwCookie = 0; // not a valid cookie
    }

    if (hr)
    {
        _arySink.Delete(_arySink.Size() - 1);
        goto Cleanup;
    }
    
    pSinkEntry->pProgSink = pProgSink;
    pSinkEntry->ulRefs    = 1;
    pSinkEntry->dwCookie  = dwCookie;
    pProgSink->AddRef();

Cleanup:
    PerfDbgLog1(tagDwnInfo, this, "-CDwnLoad::AddProgSink (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnInfo::StartProgress()
{
    SINKENTRY * pSinkEntry  = _arySink;
    UINT        cSink       = _arySink.Size();
    HRESULT     hr;

    Assert(TstFlags(DWNLOAD_NOTLOADED));
    
    for (; cSink; --cSink, ++pSinkEntry)
    {
        Assert(pSinkEntry->ulRefs && !pSinkEntry->dwCookie);
        
        hr = THR(pSinkEntry->pProgSink->AddProgress(GetProgSinkClass(), &(pSinkEntry->dwCookie)));

        if (hr == S_OK && _pDwnLoad)
        {
            hr = THR(_pDwnLoad->RequestProgress(pSinkEntry->pProgSink, pSinkEntry->dwCookie));

            if (hr)
            {
                pSinkEntry->pProgSink->DelProgress(pSinkEntry->dwCookie);
                pSinkEntry->dwCookie = 0;
            }
        }
    }
}


void
CDwnInfo::DelProgSink(IProgSink * pProgSink)
{
    PerfDbgLog(tagDwnInfo, this, "+CDwnInfo::DelProgSink");

    Assert(EnteredCriticalSection());

    SINKENTRY * pSinkEntry = _arySink;
    UINT cSink = _arySink.Size();

    for (; cSink > 0; --cSink, ++pSinkEntry)
    {
        if (pSinkEntry->pProgSink == pProgSink)
        {
            if (--pSinkEntry->ulRefs == 0)
            {
                if (pSinkEntry->dwCookie)
                    pProgSink->DelProgress(pSinkEntry->dwCookie);
                pProgSink->Release();
                _arySink.Delete(_arySink.Size() - cSink);
            }
            break;
        }
    }

    PerfDbgLog(tagDwnInfo, this, "-CDwnInfo::DelProgSink");
}

HRESULT
CDwnInfo::SetProgress(DWORD dwFlags, DWORD dwState,
    LPCTSTR pch, DWORD dwIds, DWORD dwPos, DWORD dwMax)
{
    if (dwFlags && _arySink.Size())
    {
        EnterCriticalSection();

        SINKENTRY * pSinkEntry = _arySink;
        UINT cSink = _arySink.Size();

        for (; cSink > 0; --cSink, ++pSinkEntry)
        {
            pSinkEntry->pProgSink->SetProgress(pSinkEntry->dwCookie,
                dwFlags, dwState, pch, dwIds, dwPos, dwMax);
        }

        LeaveCriticalSection();
    }

    return S_OK;
}


// CDwnLoad ----------------------------------------------------------------

CDwnLoad::~CDwnLoad()
{
    PerfDbgLog(tagDwnLoad, this, "+CDwnLoad::~CDwnLoad");

    if (_pDwnInfo)
        _pDwnInfo->SubRelease();

    if (_pDwnBindData)
        _pDwnBindData->Release();

    if (_pDownloadNotify)
        _pDownloadNotify->Release();

    PerfDbgLog(tagDwnLoad, this, "-CDwnLoad::~CDwnLoad");
}

HRESULT
CDwnLoad::Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo,
    UINT idsLoad, DWORD dwFlagsExtra)
{
    PerfDbgLog(tagDwnLoad, this, "+CDwnLoad::Init");
    
    HRESULT hr;
    TCHAR *pchAlloc = NULL;
    const TCHAR *pchUrl;

    Assert(!_pDwnInfo && !_pDwnBindData);
    Assert(!_pDownloadNotify);

    _cDone    = 1;
    _idsLoad  = idsLoad;
    _pDwnInfo = pDwnInfo;
    _pDwnInfo->SubAddRef();

    SetPcs(_pDwnInfo->GetPcs());

    // Only notify IDownloadNotify if load is _NOT_ from
    // an IStream, an existing bind context, or client-supplied data
    
    _pDownloadNotify = pdli->pDwnDoc && !pdli->pstm && !pdli->pbc && !pdli->fClientData ? pdli->pDwnDoc->GetDownloadNotify() : NULL;

    if (_pDownloadNotify)
    {
        _pDownloadNotify->AddRef();

        pchUrl = pdli->pchUrl;
        
        // If all we have is a moniker, we can try to extract the URL anyway
        if (!pchUrl && pdli->pmk)
        {
            hr = THR(pdli->pmk->GetDisplayName(NULL, NULL, &pchAlloc));
            if (hr)
                goto Cleanup;

            pchUrl = pchAlloc;
        }
        
//$ WIN64: IDownloadNotify::DownloadStart needs a DWORD_PTR as second argument

        hr = THR_NOTRACE(_pDownloadNotify->DownloadStart(pchUrl, (DWORD)(DWORD_PTR)this, pDwnInfo->GetType(), 0));
        if (hr)
            goto Cleanup;
    }

    hr = THR(NewDwnBindData(pdli, &_pDwnBindData, dwFlagsExtra));

    // initial guess for security based on url (may be updated in onbindheaders)
    _pDwnInfo->SetSecFlags(_pDwnBindData->GetSecFlags());

Cleanup:
    CoTaskMemFree(pchAlloc);
    PerfDbgLog1(tagDwnLoad, this, "-CDwnLoad::Init (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnLoad::SetCallback()
{
    PerfDbgLog(tagDwnLoad, this, "+CDwnLoad::SetCallback");

    _pDwnBindData->SetCallback(this);

    PerfDbgLog(tagDwnLoad, this, "-CDwnLoad::SetCallback");
}

void
CDwnLoad::OnBindCallback(DWORD dwFlags)
{
    PerfDbgLog1(tagDwnLoad, this, "+CDwnLoad::OnBindCallback (dwFlags=%04lX)", dwFlags);

    HRESULT hr = S_OK;

    Assert(!EnteredCriticalSection());
    
    if (dwFlags & DBF_PROGRESS)
    {
        DWNPROG DwnProg;

        _pDwnBindData->GetProgress(&DwnProg);

        hr = THR(OnBindProgress(&DwnProg));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DBF_REDIRECT)
    {
        hr = THR(OnBindRedirect(_pDwnBindData->GetRedirect(), _pDwnBindData->GetMethod()));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DBF_HEADERS)
    {
        hr = THR(OnBindHeaders());
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DBF_MIME)
    {
        hr = THR(OnBindMime(_pDwnBindData->GetMimeInfo()));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DBF_DATA)
    {
        hr = THR(OnBindData());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (FAILED(hr))
    {
        if (!_fDwnBindTerm)
        {
            _pDwnBindData->Disconnect();
            _pDwnBindData->Terminate(hr);
        }

        dwFlags |= DBF_DONE;
    }

    if (dwFlags & DBF_DONE)
    {
        OnBindDone(_pDwnBindData->GetBindResult());
    }

    PerfDbgLog1(tagDwnLoad, this, "-CDwnLoad::OnBindCallback (hr=%lX)", hr);
}

void
CDwnLoad::Passivate()
{
    PerfDbgLog(tagDwnLoad, this, "+CDwnLoad::Passivate");

    _fPassive = TRUE;
    
    if (_pDwnBindData && !_fDwnBindTerm)
    {
        _pDwnBindData->Disconnect();
        _pDwnBindData->Terminate(E_ABORT);
    }

    super::Passivate();

    PerfDbgLog(tagDwnLoad, this, "-CDwnLoad::Passivate");
}

HRESULT
CDwnLoad::RequestProgress(IProgSink * pProgSink, DWORD dwCookie)
{
    HRESULT hr;
    
    hr = THR(pProgSink->SetProgress(dwCookie,
            PROGSINK_SET_STATE|PROGSINK_SET_TEXT|PROGSINK_SET_IDS|
            PROGSINK_SET_POS|PROGSINK_SET_MAX,
            _dwState, GetProgText(), _dwIds, _dwPos, _dwMax));

    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

HRESULT
CDwnLoad::OnBindProgress(DWNPROG * pDwnProg)
{
    PerfDbgLog(tagDwnLoad, this, "+CDwnLoad::OnBindProgress");

    DWORD   dwFlags = 0;
    DWORD   dwState = _dwState;
    DWORD   dwPos   = _dwPos;
    DWORD   dwMax   = _dwMax;
    UINT    dwIds   = _dwIds;

    if (_fPassive)
        goto Cleanup;

    switch (pDwnProg->dwStatus)
    {
        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
            dwState  = PROGSINK_STATE_CONNECTING;
            dwIds    = _idsLoad;
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
            dwState  = PROGSINK_STATE_LOADING;
            dwIds    = _idsLoad;
            dwPos    = pDwnProg->dwPos;
            dwMax    = pDwnProg->dwMax;

            if (    _dwState != PROGSINK_STATE_LOADING
                &&  _pDwnBindData->GetRedirect())
            {
                // Looks like we got redirected somewhere else.  Force the
                // progress text to get recalculated.

                _dwIds = 0;
            }

            break;

        default:
            goto Cleanup;
    }

    if (_dwState != dwState)
    {
        _dwState = dwState;
        dwFlags |= PROGSINK_SET_STATE;
    }

    if (_dwPos != dwPos)
    {
        _dwPos = dwPos;
        dwFlags |= PROGSINK_SET_POS;
    }

    if (_dwMax != dwMax)
    {
        _dwMax = dwMax;
        dwFlags |= PROGSINK_SET_MAX;
    }

    if (_dwIds != dwIds)
    {
        _dwIds = dwIds;
        dwFlags |= PROGSINK_SET_TEXT | PROGSINK_SET_IDS;
    }

    if (dwFlags)
    {
        const TCHAR *pch = (dwFlags & PROGSINK_SET_TEXT) ? GetProgText() : NULL;
        
        _pDwnInfo->SetProgress(dwFlags, _dwState, pch, _dwIds, _dwPos, _dwMax);
    }
    
Cleanup:
    PerfDbgLog(tagDwnLoad, this, "-CDwnLoad::OnBindProgress");
    return(S_OK);
}

LPCTSTR
CDwnLoad::GetProgText()
{
    if (_dwState == PROGSINK_STATE_LOADING)
    {
        LPCTSTR pch = _pDwnBindData->GetRedirect();
        if (pch)
            return(pch);
    }

    return(GetUrl());
}

void
CDwnLoad::OnDone(HRESULT hrErr)
{
    PerfDbgLog1(tagDwnLoad, this, "+CDwnLoad::OnDone (hrErr=%lX)", hrErr);

    if (InterlockedDecrement(&_cDone) == 0)
    {
        if (_pDownloadNotify)
        {
//$ WIN64: IDownloadNotify::DownloadComplete needs a DWORD_PTR as first argument
             THR_NOTRACE(_pDownloadNotify->DownloadComplete((DWORD)(DWORD_PTR)this, hrErr, 0));
        }
             
        _hrErr = hrErr;
        _pDwnInfo->OnLoadDone(this, hrErr);
    }

    PerfDbgLog(tagDwnLoad, this, "-CDwnLoad::OnDone");
}

// AnsiToWideTrivial ----------------------------------------------------------

void
AnsiToWideTrivial(const CHAR * pchA, WCHAR * pchW, LONG cch)
{
    for (; cch >= 0; --cch)
        *pchW++ = *pchA++;
}

// MIMEINFO -------------------------------------------------------------------

NEWIMGTASKFN NewImgTaskGif;
NEWIMGTASKFN NewImgTaskJpg;
NEWIMGTASKFN NewImgTaskBmp;
#ifndef NO_ART
NEWIMGTASKFN NewImgTaskArt;
#endif // ndef WIN16
NEWIMGTASKFN NewImgTaskXbm;
#ifndef NO_METAFILE
NEWIMGTASKFN NewImgTaskWmf;
NEWIMGTASKFN NewImgTaskEmf;
#endif // NO_METAFILE
#if !defined(WINCE) && !defined(WIN16)
NEWIMGTASKFN NewImgTaskPlug;
#endif // WINCE
NEWIMGTASKFN NewImgTaskIco;

MIMEINFO g_rgMimeInfo[] =
{
    { 0, CFSTR_MIME_HTML,       0,               0 },
    { 0, CFSTR_MIME_TEXT,       0,               0 },
    { 0, TEXT("text/x-component"),0,             0 },
    { 0, CFSTR_MIME_GIF,        NewImgTaskGif,   IDS_SAVEPICTUREAS_GIF },
    { 0, CFSTR_MIME_JPEG,       NewImgTaskJpg,   IDS_SAVEPICTUREAS_JPG },
    { 0, CFSTR_MIME_PJPEG,      NewImgTaskJpg,   IDS_SAVEPICTUREAS_JPG },
    { 0, CFSTR_MIME_BMP,        NewImgTaskBmp,   IDS_SAVEPICTUREAS_BMP },
#ifndef NO_ART
    { 0, CFSTR_MIME_X_ART,      NewImgTaskArt,   IDS_SAVEPICTUREAS_ART },
    { 0, TEXT("image/x-art"),   NewImgTaskArt,   IDS_SAVEPICTUREAS_ART },
#endif // ndef NO_ART
    { 0, CFSTR_MIME_XBM,        NewImgTaskXbm,   IDS_SAVEPICTUREAS_XBM },
    { 0, CFSTR_MIME_X_BITMAP,   NewImgTaskXbm,   IDS_SAVEPICTUREAS_XBM },
#ifndef NO_METAFILE
    { 0, CFSTR_MIME_X_WMF,      NewImgTaskWmf,   IDS_SAVEPICTUREAS_WMF },
    { 0, CFSTR_MIME_X_EMF,      NewImgTaskEmf,   IDS_SAVEPICTUREAS_EMF },
#endif // NO_METAFILE
    { 0, CFSTR_MIME_AVI,        0,               IDS_SAVEPICTUREAS_AVI },
    { 0, CFSTR_MIME_X_MSVIDEO,  0,               IDS_SAVEPICTUREAS_AVI },
    { 0, CFSTR_MIME_MPEG,       0,               IDS_SAVEPICTUREAS_MPG },
    { 0, CFSTR_MIME_QUICKTIME,  0,               IDS_SAVEPICTUREAS_MOV },
#ifndef _MAC
    { 0, CFSTR_MIME_X_PNG,      NewImgTaskPlug,  IDS_SAVEPICTUREAS_PNG },
    { 0, TEXT("image/png"),     NewImgTaskPlug,  IDS_SAVEPICTUREAS_PNG },
    { 0, TEXT("image/x-icon"),  NewImgTaskIco,   IDS_SAVEPICTUREAS_BMP },
#endif // _MAC
};

LPCTSTR g_pchWebviewMimeWorkaround = _T("text/webviewhtml");


#define MIME_TYPE_COUNT         ARRAY_SIZE(g_rgMimeInfo)

#if !defined(WINCE) && !defined(WIN16)
MIMEINFO g_miImagePlug =
    { 0, _T("image/x-ms-plug"), NewImgTaskPlug,  0 };
#endif

const LPCSTR g_rgpchMimeType[MIME_TYPE_COUNT] =
{
    "text/html",
    "text/plain",
    "text/x-component",
    "image/gif",
    "image/jpeg",
    "image/pjpeg",
    "image/bmp",
#ifndef NO_ART
    "image/x-jg",
    "image/x-art",
#endif // ndef NO_ART
    "image/xbm",
    "image/x-xbitmap",
#ifndef NO_METAFILE
    "image/x-wmf",
    "image/x-emf",
#endif // NO_METAFILE
    "video/avi",
    "video/x-msvideo",
    "video/mpeg",
    "video/quicktime",
#ifndef _MAC
    "image/x-png",
    "image/png",
    "image/x-icon",
#endif // _MAC
};

MIMEINFO *  g_pmiTextHtml       = &g_rgMimeInfo[0];
MIMEINFO *  g_pmiTextPlain      = &g_rgMimeInfo[1];
MIMEINFO *  g_pmiTextComponent  = &g_rgMimeInfo[2];

#if !defined(WINCE) && !defined(WIN16)
MIMEINFO *  g_pmiImagePlug = &g_miImagePlug;
#endif

BOOL g_fInitMimeInfo = FALSE;

void InitMimeInfo()
{
    MIMEINFO *  pmi  = g_rgMimeInfo;
    const LPCSTR *    ppch = g_rgpchMimeType;
    int         c    = MIME_TYPE_COUNT;
	
	Assert(ARRAY_SIZE(g_rgMimeInfo) == ARRAY_SIZE(g_rgpchMimeType));

    for (; --c >= 0; ++pmi, ++ppch)
    {
		Assert(*ppch);
#ifdef _MAC
        pmi->cf = RegisterMimeFormat(*ppch);
#else
        pmi->cf = (CLIPFORMAT)RegisterClipboardFormatA(*ppch);
#endif
    }

    g_fInitMimeInfo = TRUE;
}

MIMEINFO * GetMimeInfoFromClipFormat(CLIPFORMAT cf)
{
    MIMEINFO * pmi;
    UINT c;

    if (!g_fInitMimeInfo)
    {
        InitMimeInfo();
    }

    for (c = MIME_TYPE_COUNT, pmi = g_rgMimeInfo; c > 0; --c, ++pmi)
    {
        if (pmi->cf == cf)
        {
            return(pmi);
        }
    }

    return(NULL);
}

MIMEINFO * GetMimeInfoFromMimeType(const TCHAR * pchMime)
{
    MIMEINFO * pmi;
    UINT c;

    if (!g_fInitMimeInfo)
    {
        InitMimeInfo();
    }

    for (c = MIME_TYPE_COUNT, pmi = g_rgMimeInfo; c > 0; --c, ++pmi)
    {
        if (StrCmpIC(pmi->pch, pchMime) == 0)
        {
            return(pmi);
        }
    }

    // BUGBUG: the following works around urlmon bug NT 175191:
    // Mime filters do not change the mime type correctly.
    // Remove this exception when that bug is fixed.
    if (StrCmpIC(g_pchWebviewMimeWorkaround, pchMime) == 0)
    {
    	return g_pmiTextHtml;
    }

    return(NULL);
}

#if !defined(WINCE) && !defined(WIN16)
MIMEINFO * GetMimeInfoFromData(void * pvData, ULONG cbData, const TCHAR *pchProposed)
{
    HRESULT hr;
    MIMEINFO * pmi = NULL;
    TCHAR * pchMimeType = NULL;

    hr = FindMimeFromData(NULL,             // bind context - can be NULL                                     
                          NULL,             // url - can be null
                          pvData,           // buffer with data to sniff - can be null (pwzUrl must be valid) 
                          cbData,           // size of buffer                                                 
                          pchProposed,      // proposed mime if - can be null                                 
                          0,                // will be defined                                                
                          &pchMimeType,     // the suggested mime                                             
                          0);
    if (!hr)
    {
        pmi = GetMimeInfoFromMimeType(pchMimeType);
        if (pmi)
            goto Cleanup;
    }

    if (cbData && IsPluginImgFormat((BYTE *)pvData, cbData))
    {
        pmi = g_pmiImagePlug;
    }

Cleanup:
    CoTaskMemFree(pchMimeType);
    return(pmi);
}
#endif

// Shutdown -------------------------------------------------------------------

void DeinitDownload()
{
    if (g_pImgBitsNotLoaded)
        delete g_pImgBitsNotLoaded;

    if (g_pImgBitsMissing)
        delete g_pImgBitsMissing;

    DwnCacheDeinit();
}

//+------------------------------------------------------------------------
//
//  Helper:     GetBuiltinGenericTag
//
//-------------------------------------------------------------------------

CBuiltinGenericTagDesc g_aryBuiltinGenericTags[] = 
{
    {_T("HTC"),                 CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_DESC},
    {_T("COMPONENT"),           CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_DESC},
    {_T("PROPERTY"),            CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_PROPERTY},
    {_T("METHOD"),              CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_METHOD},
    {_T("EVENT"),               CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_EVENT},
    {_T("ATTACH"),              CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_ATTACH},
    {_T("PUT"),                 CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_NONE},
    {_T("GET"),                 CBuiltinGenericTagDesc::TYPE_HTC,      HTC_BEHAVIOR_NONE},
    {NULL}
};

CBuiltinGenericTagDesc *
GetBuiltinGenericTag(LPTSTR pchName)
{
    CBuiltinGenericTagDesc * pTagDesc;

    Assert (NULL == StrChr(pchName, _T(':')));

    //BUGBUG (alexz): optimize to use assoc tables for fast hashed lookup
    for (pTagDesc = g_aryBuiltinGenericTags; pTagDesc->pchName; pTagDesc++)
    {
        if (0 == StrCmpIC(pchName, pTagDesc->pchName))
        {
            return pTagDesc;
        }
    }

    return GetBuiltinLiteralGenericTag(pchName);
}

//+------------------------------------------------------------------------
//
//  Helper:     GetBuiltinLiteralGenericTag
//
//-------------------------------------------------------------------------

CBuiltinGenericTagDesc g_aryBuiltinLiteralGenericTags[] = 
{
    {_T("XML"),     CBuiltinGenericTagDesc::TYPE_OLE,
        {0x379E501F, 0xB231, 0x11d1, 0xad, 0xc1, 0x00, 0x80, 0x5F, 0xc7, 0x52, 0xd8}},
    {NULL}
};

CBuiltinGenericTagDesc *
GetBuiltinLiteralGenericTag(LPTSTR pchName, LONG cchName)
{
    CBuiltinGenericTagDesc * pTagDesc;

    if (-1 != cchName)
    {
        // note that (TRUE == _tcsnipre(_T("XML"), 3, pchName, cchName)) will not work correctly here
        // (if pchName is XML:NAMESPACE for example)
        for (pTagDesc = g_aryBuiltinLiteralGenericTags; pTagDesc->pchName; pTagDesc++)
        {
            if ((LONG)_tcslen(pTagDesc->pchName) == cchName &&
                0 == StrCmpNIC(pTagDesc->pchName, pchName, cchName))
            {
                return pTagDesc;
            }
        }
    }
    else
    {
        for (pTagDesc = g_aryBuiltinLiteralGenericTags; pTagDesc->pchName; pTagDesc++)
        {
            if (0 == StrCmpIC(pTagDesc->pchName, pchName))
            {
                return pTagDesc;
            }
        }
    }
    return NULL;
}
