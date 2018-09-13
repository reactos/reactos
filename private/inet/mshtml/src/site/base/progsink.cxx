//+------------------------------------------------------------------------
//
//  File:       progsink.cxx
//
//  Contents:   IProgSink implementation for CDoc
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include "prgsnk.h"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

//+---------------------------------------------------------------------------
//  Debugging Support
//----------------------------------------------------------------------------

DeclareTag(tagProgSink, "ProgSink", "Trace ProgSink");

MtDefine(CProgSink, CDoc, "CProgSink")
MtDefine(CProgSink_aryProg_pv, CProgSink, "CProgSink::_aryProg::_pv")
MtDefine(CProgSinkText, CProgSink, "CProgSink (item text)")
MtDefine(CProgSinkFormat, CProgSink, "CProgSink (formatted text)")

#if DBG==1

static LPCSTR _apchProgClass[] =
{
    "HTML",
    "MULTIMEDIA",
    "CONTROL",
    "DATABIND",
    "OTHER",
    "NOREMAIN",
    "FRAME"
};

static LPCSTR _apchProgState[] =
{
    "IDLE",
    "FINISHING",
    "CONNECTING",
    "LOADING"
};

#endif

//+---------------------------------------------------------------------------
//  Definitions
//----------------------------------------------------------------------------

#define PROGSINK_CHANGE_COUNTERS    0x0001
#define PROGSINK_CHANGE_CURRENT     0x0002
#define PROGSINK_CHANGE_RESCAN      0x0004
#define PROGSINK_CHANGE_PROGRESS    0x0008

// these numbers should be scaled all togather not individually
// These numbers are intentionally set to a large number to take into account 
// small download framents

#define PROGSINK_POTEN_HTML         4000    // potential for html
#define PROGSINK_POTEN_FRAME        2000    // potential for frame
#define PROGSINK_POTEN_BELOW        1000    // potential substract for lower level HTML/FRAME

#define PROGSINK_ADJUST_SCALE       1000    // adjust factor scale 1000 represent 
#define PROGSINK_PROG_INIT          1000    // initial minimum
#define PROGSINK_PROG_MAX           10000   // total gauge range

#define PROGSINK_POTEN_MIN          100     // PROGSINK_ADJUST_SCALE * PROGSINK_PROG_INIT/PROGSINK_PROG_MAX
#define PROGSINK_POTEN_END          1

#define PROGSINK_PROG_MIN           0       // we start from this position

#define PROGSINK_PROG_STEP          100     // small progress step

#define PROGSINK_TEXT_LENGTH        512

//+---------------------------------------------------------------------------
//
//  Member:     FormatProgress
//
//  Synopsis:   Computes the final progress string for the entry
//
//----------------------------------------------------------------------------

void
FormatProgress(CDoc * pDoc, PROGDATA * ppd)
{
    Assert(ppd->pchFormat == NULL);

    TCHAR * pch = ppd->pchText;

    if (ppd->dwIds)
    {
        TCHAR * pchArg   = pch ? pch : g_Zero.ach;
        TCHAR * pchAlloc = NULL;

        if (    *pchArg
            &&  (   ppd->dwIds == IDS_BINDSTATUS_DOWNLOADINGDATA_PICTURE
                ||  ppd->dwIds == IDS_BINDSTATUS_DOWNLOADINGDATA_TEXT
                ||  ppd->dwIds == IDS_BINDSTATUS_DOWNLOADINGDATA_BITS))
        {
            pchAlloc = GetFriendlyUrl(pchArg, NULL,
                pDoc->_pOptionSettings->fShowFriendlyUrl, FALSE);

            if (pchAlloc && *pchAlloc)
            {
                pchArg = pchAlloc;
            }
        }

        IGNORE_HR(Format(FMT_OUT_ALLOC, &ppd->pchFormat, 0,
            MAKEINTRESOURCE(ppd->dwIds), pchArg));

        MemFree(pchAlloc);

        if (ppd->pchFormat)
        {
            IGNORE_HR(MemRealloc(Mt(CProgSinkFormat), (void **)&ppd->pchFormat,
                (_tcslen(ppd->pchFormat) + 1) * sizeof(TCHAR)));
        }
    }
    else if (pch)
    {
        IGNORE_HR(MemAllocString(Mt(CProgSinkFormat), pch, &ppd->pchFormat));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::CProgSink
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------

CProgSink::CProgSink(CDoc * pDoc, CMarkup * pMarkup)
    : CBaseFT(&_cs)
{
    InitializeCriticalSection(&_cs);

    _pDoc = pDoc;
    _pDoc->SubAddRef();

    _pMarkup = pMarkup;
    _pMarkup->SubAddRef();

    _pts = GetThreadState();

    SubAddRef();

    while (pDoc->_pDocParent)
    {
        pDoc = pDoc->_pDocParent;
    }

    if (pDoc != _pDoc)
    {
        ReplaceInterface(&_pProgSinkFwd, pDoc->GetProgSink());
    }

#if DBG==1
    MemSetName((this, "CProgSink pDoc=%08x pProgSinkFwd=%08x", _pDoc, _pProgSinkFwd));

    if (_pProgSinkFwd)
        TraceTag((tagProgSink, "[%08lX] Construct (Forward to [%08lX])",
            this, _pProgSinkFwd));
    else
        TraceTag((tagProgSink, "[%08lX] Construct", this));
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::~CProgSink
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------

CProgSink::~CProgSink()
{
    DeleteCriticalSection(&_cs);

    TraceTag((tagProgSink, "[%08lX] Destruct", this));
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::Passivate
//
//  Synopsis:   Releases all resources used by this object.  Puts it into
//              the passive state whereby all further progress calls are
//              rejected.
//
//----------------------------------------------------------------------------

void
CProgSink::Passivate()
{
    PROGDATA * ppd;
    UINT cSize;
    
    TraceTag((tagProgSink, "[%08lX] Passivate (enter)", this));

    EnterCriticalSection();

    _fPassive = TRUE;

    LeaveCriticalSection();

    cSize = _aryProg.Size();
    ppd = _aryProg;

    for (; cSize > 0; --cSize, ++ppd)
    {
        if (ppd->bFlags & PDF_FREE)
            continue;

        if (ppd->pchText)
            MemFree(ppd->pchText);

        if (ppd->pchFormat)
            MemFree(ppd->pchFormat);
    }

    _aryProg.DeleteAll();

    if (_fGotDefault)
    {
        MemFree(_pdDefault.pchText);
        _pdDefault.pchText = NULL;

        MemFree(_pdDefault.pchFormat);
        _pdDefault.pchFormat = NULL;

        _fGotDefault = FALSE;
    }

    _pDoc->SubRelease();
    _pDoc = NULL;

    _pMarkup->SubRelease();
    _pMarkup = NULL;

    ReleaseInterface(_pProgSinkFwd);

    GWKillMethodCallEx(_pts, this, ONCALL_METHOD(CProgSink, OnMethodCall, onmethodcall), 0);
    _fSentSignal = FALSE;

    TraceTag((tagProgSink, "[%08lX] Passivate (leave)", this));
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::Detach
//
//  Synopsis:   Called by CDoc before it releases it's final reference on
//              the CProgSink object.
//
//----------------------------------------------------------------------------

void
CProgSink::Detach()
{
    TraceTag((tagProgSink, "[%08lX] Detach", this));
    super::Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::QueryInterface, IUnknown
//
//  Synopsis:   As per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgSink::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IProgSink || iid == IID_IUnknown)
    {
        *ppv = (IProgSink *)this;
    }
    else
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::AddRef, IUnknown
//
//  Synopsis:   As per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CProgSink::AddRef()
{
    return(super::SubAddRef());
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::Release, IUnknown
//
//  Synopsis:   As per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CProgSink::Release()
{
    return(super::SubRelease());
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::AddProgress, IProgSink
//
//  Synopsis:   Allocates a slot in the progress vector
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgSink::AddProgress(DWORD dwClass, DWORD * pdwCookie)
{
    PROGDATA    pd;
    PROGDATA *  ppd;
    UINT        cSize;
    DWORD       dwCookie;
    HRESULT     hr = S_OK;

    if ((dwClass & ~(PROGSINK_CLASS_FORWARDED|PROGSINK_CLASS_NOSPIN)) > PROGSINK_CLASS_FRAME)
        RRETURN(E_INVALIDARG);

    memset(&pd, 0, sizeof(PROGDATA));
    pd.bClass = (BYTE)dwClass;
    pd.bState = PROGSINK_STATE_IDLE;
    pd.bBelow = !!(dwClass & PROGSINK_CLASS_FORWARDED);
    pd.bFlags = (dwClass & PROGSINK_CLASS_NOSPIN) ? PDF_NOSPIN : 0;

    EnterCriticalSection();

    if (_fPassive)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
        
    if (_pProgSinkFwd)
    {
        hr = THR(_pProgSinkFwd->AddProgress(dwClass | PROGSINK_CLASS_FORWARDED,
                pdwCookie));
        if (hr)
            goto Cleanup;

        SetCounters(pd.bClass, FALSE, TRUE, FALSE);
        goto Cleanup;
    }

    if (_cFree > 0)
    {
        // Look for a free slot to reuse

        ppd   = &_aryProg[0];
        cSize = _aryProg.Size();

        for (; cSize > 0; --cSize, ++ppd)
            if (ppd->bFlags & PDF_FREE)
                break;

        Assert(cSize > 0);

        *ppd = pd;
        _cFree -= 1;

        dwCookie = _aryProg.Size() - cSize + 1;
    }
    else
    {
        // Add a new slot to the end of the array

        hr = THR(_aryProg.AppendIndirect(&pd));
        if (hr)
            goto Cleanup;

        dwCookie = _aryProg.Size();
    }

    *pdwCookie = (dwCookie << 8) | pd.bClass;

    SetCounters(pd.bClass, !!pd.bBelow, TRUE, !(pd.bFlags & PDF_NOSPIN));

    TraceTag((tagProgSink, "[%08lX] AddProgress %2d%s %s",
        this, *pdwCookie >> 8, pd.bBelow ? "/b" : "",
        _apchProgClass[pd.bClass]));

Cleanup:
    LeaveCriticalSection();
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::SetProgress, IProgSink
//
//  Synopsis:   Sets the data associated with a progress slot
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgSink::SetProgress(DWORD dwCookie, DWORD dwFlags, DWORD dwState,
    LPCTSTR pchText, DWORD dwIds, DWORD dwPos, DWORD dwMax)
{
    PROGDATA * ppd;
    DWORD dwSlot;
    HRESULT hr = S_OK;

    EnterCriticalSection();

    if (_fPassive)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (_pProgSinkFwd)
    {
        hr = THR(_pProgSinkFwd->SetProgress(dwCookie, dwFlags, dwState,
                pchText, dwIds, dwPos, dwMax));
        goto Cleanup;
    }

    dwSlot = (dwCookie >> 8);

    if (dwSlot == 0 || dwSlot > (UINT)_aryProg.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ppd = &_aryProg[dwSlot - 1];

    if (ppd->bFlags & PDF_FREE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (dwFlags & PROGSINK_SET_STATE)
    {
        ppd->bState = (BYTE)dwState;
    }

    if (dwFlags & PROGSINK_SET_IDS)
    {
        ppd->dwIds = dwIds;

        if (ppd->pchFormat)
        {
            MemFree(ppd->pchFormat);
            ppd->pchFormat = NULL;
        }
    }

    if (dwFlags & PROGSINK_SET_TEXT)
    {
        if (pchText == NULL)
        {
            MemFree(ppd->pchText);
            ppd->pchText = NULL;
        }
        else
        {
            hr = THR(MemReplaceString(Mt(CProgSinkText), pchText, &ppd->pchText));
            if (hr)
                goto Cleanup;
        }

        if (ppd->pchFormat)
        {
            MemFree(ppd->pchFormat);
            ppd->pchFormat = NULL;
        }
    }

    if (dwFlags & (PROGSINK_SET_MAX | PROGSINK_SET_POS))
    {
        // update the progress
        // signal only if a progress is made
        if (UpdateProgress(ppd, dwFlags, dwPos, dwMax)
                    && !(_uChange & (PROGSINK_CHANGE_PROGRESS
                                    | PROGSINK_CHANGE_RESCAN
                                    | PROGSINK_CHANGE_CURRENT)))
        {
            Signal(PROGSINK_CHANGE_PROGRESS);
        }
    }

    if (!(_uChange & PROGSINK_CHANGE_RESCAN))
    {
        if (dwSlot == _dwSlotCur)
        {
            if (ppd->bState < _bStateCur || (!ppd->pchText && !ppd->dwIds))
                Signal(PROGSINK_CHANGE_RESCAN);
            else
                Signal(PROGSINK_CHANGE_CURRENT);
        }
        else if (   (ppd->pchText || ppd->dwIds)
                &&  ppd->bState > PROGSINK_STATE_IDLE
                &&  (   ppd->bBelow < _bBelowCur
                    ||  ppd->bState > _bStateCur))
        {
            Signal(PROGSINK_CHANGE_RESCAN);
        }
    }

#if DBG==1
    TraceTagEx((tagProgSink, TAG_NONEWLINE, "[%08lX] SetProgress %2d%s ",
        this, dwSlot, ppd->bBelow ? "/b" : ""));
    if (dwFlags & PROGSINK_SET_STATE)
        TraceTagEx((tagProgSink, TAG_NONAME|TAG_NONEWLINE, "%s ",
            _apchProgState[dwState]));
    if (dwFlags & (PROGSINK_SET_POS|PROGSINK_SET_MAX))
        TraceTagEx((tagProgSink, TAG_NONAME|TAG_NONEWLINE, "[%d/%d] ",
            ppd->dwPos, ppd->dwMax));
    if (dwFlags & PROGSINK_SET_IDS)
        TraceTagEx((tagProgSink, TAG_NONAME|TAG_NONEWLINE, "ids=%ld ",
            ppd->dwIds));
    if (dwFlags & PROGSINK_SET_TEXT)
        TraceTagEx((tagProgSink, TAG_NONAME|TAG_NONEWLINE, "\"%ls\"",
            pchText ? pchText : _T("")));
    TraceTagEx((tagProgSink, TAG_NONAME, ""));
#endif

Cleanup:
    LeaveCriticalSection();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::DelProgress, IProgSink
//
//  Synopsis:   Frees the given progress slot
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgSink::DelProgress(DWORD dwCookie)
{
    PROGDATA * ppd;
    DWORD dwSlot;
    HRESULT hr = S_OK;

    EnterCriticalSection();

    if (_fPassive)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (_pProgSinkFwd)
    {
        hr = THR(_pProgSinkFwd->DelProgress(dwCookie));
        if (hr)
            goto Cleanup;

        SetCounters((BYTE)dwCookie, FALSE, FALSE, FALSE);
        goto Cleanup;
    }

    dwSlot = (dwCookie >> 8);

    if (dwSlot == 0 || dwSlot > (UINT)_aryProg.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    ppd = &_aryProg[dwSlot - 1];

    if (ppd->bFlags & PDF_FREE)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!ppd->bBelow && ppd->bClass == PROGSINK_CLASS_HTML && !_fGotDefault)
    {
        _pdDefault = *ppd;
        _fGotDefault = TRUE;
    }
    else
    {
        if (ppd->pchText)
            MemFree(ppd->pchText);
        if (ppd->pchFormat)
            MemFree(ppd->pchFormat);
    }

    if (ppd->dwPos && ppd->dwPos < ppd->dwMax)  // aborted download
    {
        _cOngoing --;
        // adjust dwMaxTotal so that we can let other download take the bar
        _dwMaxTotal = _dwMaxTotal - (ppd->dwMax - ppd->dwPos);
    }
    Assert(_dwMaxTotal>=0);

    SetCounters(ppd->bClass, !!ppd->bBelow, FALSE, !(ppd->bFlags & PDF_NOSPIN));

    ppd->bFlags |= PDF_FREE;
    ppd->dwPos   = 0;
    ppd->dwMax   = 0;
    _cFree += 1;

    TraceTag((tagProgSink, "[%08lX] DelProgress %2d%s", this, dwSlot,
        ppd->bBelow ? "/b" : ""));

    if (_cFree == (UINT)_aryProg.Size())
    {
        _aryProg.DeleteAll();
        _cFree = 0;
    }

    // If we are currently displaying this progress slot, it's time to
    // look for a different one.

    if (dwSlot == _dwSlotCur)
    {
        Signal(PROGSINK_CHANGE_RESCAN);
    }

Cleanup:
    LeaveCriticalSection();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::SetCounters
//
//  Synopsis:   Adjusts the class counters to reflect the addition or
//              deletion of a progress slot.
//
//----------------------------------------------------------------------------

void
CProgSink::SetCounters(DWORD dwClass, BOOL fFwd, BOOL fAdd, BOOL fSpin)
{
    UINT *  pacClass = fFwd ? _acClassBelow : _acClass;
    UINT    cClass   = pacClass[dwClass];

    Assert(!fSpin || !_pProgSinkFwd);

    if (fAdd)
    {
        cClass += 1;
        _cActive += 1;
        _cSpin += !!fSpin;

        if (_cActive == 1)
        {
            _fSendProgDone = TRUE;
        }
    }
    else
    {
        Assert(cClass > 0);
        cClass -= 1;
        Assert(_cActive > 0);
        _cActive -= 1;
        Assert(!fSpin || _cSpin > 0);
        _cSpin -= !!fSpin;
        Assert(_cActive || !_cSpin);
    }

    pacClass[dwClass] = cClass;

    AdjustProgress(dwClass, fFwd, fAdd);

    // Certain transitions are interesting enough to wake up the client:
    //  a) If the HTML file is parsed
    //  b) If the HTML file, frames and controls have finished loading (connect scripts)
    //  c) If the total number of downloads reaches zero (send all done)
    //  d) If the total number of spin requests reaches zero (stop spinning)
    //  e) If the total number of spin requests reaches one (start spinning)

    if (_acClass[PROGSINK_CLASS_HTML] == 1)
    {
        _fSendParseDone = TRUE;
    }

    if (    !(_uChange & PROGSINK_CHANGE_COUNTERS)
        &&  (   (   !fFwd
                &&  !fAdd
                &&  dwClass == PROGSINK_CLASS_HTML
                &&  _acClass[PROGSINK_CLASS_HTML] == 0)                
            ||  (   !fFwd
                &&  !fAdd
                &&  (   dwClass == PROGSINK_CLASS_HTML
                    ||  dwClass == PROGSINK_CLASS_FRAME
                    ||  dwClass == PROGSINK_CLASS_CONTROL)
                &&  _acClass[PROGSINK_CLASS_HTML] == 0
                &&  _acClass[PROGSINK_CLASS_FRAME] == 0
                &&  _acClass[PROGSINK_CLASS_CONTROL] == 0)
            ||  (!fAdd && _cActive == 0)
            ||  (!fAdd && fSpin && _cSpin == 0)
            ||  ( fAdd && fSpin && _cSpin == 1)))
    {
        Signal(PROGSINK_CHANGE_COUNTERS);
    }

    // If we are showing (n items remaining) and we've just removed one of
    // those items, then signal that client to update it.

    if (    !(_uChange & PROGSINK_CHANGE_RESCAN)
        &&  (!fAdd && _fShowItemsRemaining))
    {
        Signal(_dwSlotCur ? PROGSINK_CHANGE_CURRENT : PROGSINK_CHANGE_RESCAN);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::Signal
//
//  Synopsis:   Posts a message to the creation thread and requests a call
//              to OnProgSink.
//
//----------------------------------------------------------------------------

void
CProgSink::Signal(UINT uChange)
{
    TraceTag((tagProgSink, "[%08lX] Signal %s%s%s%s", this,
        (uChange & PROGSINK_CHANGE_COUNTERS) ? "COUNTERS " : "",
        (uChange & PROGSINK_CHANGE_CURRENT) ? "CURRENT " : "",
        (uChange & PROGSINK_CHANGE_RESCAN) ? "RESCAN " : "",
        (uChange & PROGSINK_CHANGE_PROGRESS) ? "PROGRESS " : ""));

    _uChange |= uChange;

    if (!_fSentSignal)
    {
        _fSentSignal = TRUE;

        IGNORE_HR(GWPostMethodCallEx(_pts, this,
            ONCALL_METHOD(CProgSink, OnMethodCall, onmethodcall), (DWORD_PTR)this, FALSE, "CProgSink::OnMethodCall"));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::OnMethodCall
//
//  Synopsis:   Updates the status bar and sends progress feedback to the
//              doc in response to changes in the state of the progress sink.
//
//----------------------------------------------------------------------------

void BUGCALL
CProgSink::OnMethodCall(DWORD_PTR dwContext)
{
    BOOL    fSendProgDone  = FALSE;
    BOOL    fSendSpin      = FALSE;
    BOOL    fSpin          = FALSE;
    DWORD   dwFlagsProg    = 0;
    DWORD   dwPos          = 0;
    DWORD   dwMax          = 0;
    TCHAR   achText[PROGSINK_TEXT_LENGTH];
    LOADSTATUS loadstatus  = LOADSTATUS_UNINITIALIZED;
    CDoc *  pDoc;
    
    Assert(dwContext == (DWORD_PTR)this);
    Assert(GetCurrentThreadId() == _pts->dll.idThread);
    Assert(!_fPassive);

    TraceTag((tagProgSink, "[%08lX] OnMethodCall (enter)", this));


    // BUGBUG (MohanB) Hack for ThumbNailView (IE5 bug #66938, #68010)
    // May need to compute formats now to initiate download of background
    // images, etc. Hosts like THUMBVW never make us inplace-active or give
    // us a window/view. This causes us to fire onload without ever needing
    // to compute formats. However, they call Draw() on us as soon as we fire
    // onload. We need the images loaded by then to be be able to render them.
    if (    _pDoc
        &&  _pDoc->_fThumbNailView
        &&  _pMarkup
        &&  (_uChange & PROGSINK_CHANGE_COUNTERS)
        &&  _fSendParseDone
        &&  !_fSentParseDone
        && _acClass[PROGSINK_CLASS_HTML] == 0
       )
    {
        _pMarkup->EnsureFormats();
    }

    EnterCriticalSection();

    if (_uChange & PROGSINK_CHANGE_COUNTERS)
    {
        if (_fSendParseDone)
        {
            if (!_fSentParseDone && _acClass[PROGSINK_CLASS_HTML] == 0)
            {
                TraceTag((tagProgSink, "[%08lX]   Sending ParseDone", this));
                _fSentParseDone = TRUE;
                if (loadstatus < LOADSTATUS_PARSE_DONE)
                    loadstatus = LOADSTATUS_PARSE_DONE;
            }

            if (    !_fSentQuickDone && _fSentParseDone
                &&  _acClass[PROGSINK_CLASS_CONTROL] == 0
                &&  _acClass[PROGSINK_CLASS_FRAME] == 0)
            {
                TraceTag((tagProgSink, "[%08lX]   Sending QuickDone", this));
                _fSentQuickDone = TRUE;
                if (loadstatus < LOADSTATUS_QUICK_DONE)
                    loadstatus = LOADSTATUS_QUICK_DONE;
            }

            if (!_fSentDone && _cActive == 0)
            {
                TraceTag((tagProgSink, "[%08lX]   Sending Done", this));
                _fSentDone = TRUE;
                if (loadstatus < LOADSTATUS_DONE)
                    loadstatus = LOADSTATUS_DONE;
            }
        }

        if (!_pProgSinkFwd)
        {
            if (!!_fSpin != !!_cSpin)
            {
                TraceTag((tagProgSink, "[%08lX]   Setting fSpin to %s", this,
                    _fSpin ? "FALSE" : "TRUE"));
                _fSpin = !_fSpin;
                fSendSpin = TRUE;
                fSpin     = _fSpin;
            }

            if (_fSendProgDone && _cActive == 0)
            {
                fSendProgDone  = TRUE;
                _fSendProgDone = FALSE;
            }
        }
    }

    if (    (_uChange & (PROGSINK_CHANGE_RESCAN | PROGSINK_CHANGE_CURRENT | PROGSINK_CHANGE_PROGRESS))
        ||  fSendProgDone)
    {
        Assert(!_pProgSinkFwd);

        _bStateCur = PROGSINK_STATE_IDLE;
        achText[0] = 0;

        if (fSendProgDone)
        {
            Assert(_cActive == 0);
            LoadString(GetResourceHInst(), IDS_DONE,
                achText, ARRAY_SIZE(achText));

            if (_fGotDefault)
            {
                MemFree(_pdDefault.pchText);
                _pdDefault.pchText = NULL;

                MemFree(_pdDefault.pchFormat);
                _pdDefault.pchFormat = NULL;

                _fGotDefault = FALSE;
            }

            dwFlagsProg  = PROGSINK_SET_POS|PROGSINK_SET_MAX|PROGSINK_SET_TEXT;
            dwPos        = (ULONG)(-1);
            dwMax        = 0;

            _dwSlotCur   = 0;
            _bBelowCur   = TRUE;
            _dwMaxTotal  = 0;
            _dwProgCur   = 0;
            _dwProgMax   = 0;
            _dwProgDelta = 0;
            _cOngoing    = 0;
            _cPotential  = 0;
            _cPotTotal   = PROGSINK_POTEN_MIN;
            _cPotDelta   = 0;
            _fShowItemsRemaining = FALSE;
        }
        else
        {
            // If necessary, rescan for the best progress slot to show.

            if (_uChange & PROGSINK_CHANGE_RESCAN)
            {
                PROGDATA * ppd = &_aryProg[0];
                UINT cSize = _aryProg.Size();

                _dwSlotCur = 0;
                _bBelowCur = TRUE;

                for (UINT iSlot = 1; iSlot <= cSize; ++iSlot, ++ppd)
                {
                    if (    (ppd->bFlags & PDF_FREE)
                        ||  (!ppd->pchText && !ppd->dwIds)
                        ||  ppd->bState <= PROGSINK_STATE_IDLE
                        ||  ppd->bBelow > _bBelowCur
                        ||  (   ppd->bBelow == _bBelowCur
                            &&  ppd->bState <= _bStateCur))
                        continue;

                    _dwSlotCur = iSlot;
                    _bStateCur = ppd->bState;
                    _bBelowCur = ppd->bBelow;

                    if (_bStateCur == PROGSINK_STATE_LOADING && !_bBelowCur)
                        break;
                }
            }

            if (_uChange & (PROGSINK_CHANGE_RESCAN|PROGSINK_CHANGE_CURRENT))
            {
                // The progress text needs to be recomputed.

                LPTSTR pchText = achText;
                UINT   cchText = PROGSINK_TEXT_LENGTH;
                LPTSTR pchFormat = NULL;
                PROGDATA * ppd = NULL;

                if (_dwSlotCur)
                {
                    Assert(_dwSlotCur <= (UINT)_aryProg.Size());
                    ppd = &_aryProg[_dwSlotCur - 1];
                    _bStateCur = ppd->bState;
                }
                else if (_fGotDefault)
                {
                    ppd = &_pdDefault;
                }

                if (ppd)
                {
                    pchFormat = ppd->pchFormat;

                    if (pchFormat == NULL)
                    {
                        FormatProgress(_pDoc, ppd);
                        pchFormat = ppd->pchFormat;
                    }

                    if (pchFormat && !*pchFormat)
                    {
                        pchFormat = NULL;
                    }
                }

                // Prepend "(n items remaining)" if desired.

                if (_cPotential == 0)
                {
                    UINT cActive = _cActive -
                            _acClass[PROGSINK_CLASS_NOREMAIN] -
                            _acClassBelow[PROGSINK_CLASS_NOREMAIN];

                    if (_fShowItemsRemaining || cActive > 1)
                    {
                        _fShowItemsRemaining = TRUE;

                        if (cActive > 0)
                        {
                            Format(0, achText, cchText,
                                MAKEINTRESOURCE(IDS_BINDSTATUS_DOWNLOADING),
                                cActive);

                            UINT cch = _tcslen(achText);
                            pchText += cch;
                            cchText -= cch;
                        }
                    }
                }

                // Now append either the selected slot or the default string.

                if (pchFormat && cchText > 1)
                {
                    _tcsncpy(pchText, pchFormat, cchText - 1);
                    achText[ARRAY_SIZE(achText) - 1] = 0;
                }

                if (achText[0])
                {
                    dwFlagsProg |= PROGSINK_SET_TEXT;
                }
            }

            if (_uChange & PROGSINK_CHANGE_PROGRESS)
            {
                dwPos = _dwProgCur;
                dwMax = PROGSINK_PROG_MAX;
                dwFlagsProg |= PROGSINK_SET_POS | PROGSINK_SET_MAX;
            }
        }

        #if DBG==1
        if (dwFlagsProg)
            TraceTag((tagProgSink,
                "[%08lX]   SetProgress %c%c%c [%ld/%ld] \"%ls\" %s",
                this, (dwFlagsProg & PROGSINK_SET_POS) ? 'P' : ' ',
                (dwFlagsProg & PROGSINK_SET_MAX) ? 'M' : ' ',
                (dwFlagsProg & PROGSINK_SET_TEXT) ? 'T' : ' ',
                dwPos, dwMax, achText, fSendProgDone ? "(flash)" : ""));
        #endif
    }

     _fSentSignal = FALSE;
    _uChange = 0;

    LeaveCriticalSection();

    Assert(_pDoc->GetRefs());
// NOTE (lmollico): we need a local copy of _pDoc because CDoc::OnLoadStatus can cause 
// this (CProgSink) to be destroyed if the doc is unloaded
    pDoc = _pDoc;
    // BUGBUG (sramani): The doc could be passivated and if someone holds on to the markup
    // we could get here, so need to safeguard against it.
    if (pDoc->_pPrimaryMarkup)
        pDoc->AddRef();

    if (dwFlagsProg)
        pDoc->SetProgress(dwFlagsProg, achText, dwPos, dwMax, fSendProgDone);
    if (fSendSpin)
        pDoc->SetSpin(fSpin);
    if (loadstatus > LOADSTATUS_UNINITIALIZED)
        _pMarkup->OnLoadStatus(loadstatus);

    // BUGBUG (sramani): The doc could be passivated and if someone holds on to the markup
    // we could get here, so need to safeguard against it.
    if (pDoc->_pPrimaryMarkup)
        pDoc->Release();

    TraceTag((tagProgSink, "[%08lX] OnMethodCall (leave)", this));
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::AdjustProgress
//            
//  Synopsis:   Adjust the current progress
//
//
//----------------------------------------------------------------------------

void CProgSink::AdjustProgress(DWORD dwClass, BOOL fFwd, BOOL fAdd)
{
    UINT    uiDelta = 0;
    UINT    cHtml   = _acClassBelow[PROGSINK_CLASS_HTML] +  _acClass[PROGSINK_CLASS_HTML];
    UINT    cFrame  = _acClassBelow[PROGSINK_CLASS_FRAME] +  _acClass[PROGSINK_CLASS_FRAME];

    switch(dwClass)
    {
    case    PROGSINK_CLASS_HTML:
        uiDelta = PROGSINK_POTEN_HTML;
        break;
    case    PROGSINK_CLASS_FRAME:
        uiDelta= PROGSINK_POTEN_FRAME;
        break;
    }

    if (!_cPotTotal)
    {
        _cPotTotal = PROGSINK_POTEN_MIN;
    }

    if (!cHtml && !cFrame)
    {
        if (!_cPotential)
        {
            return;
        }
        else if (!fAdd)
        {
            _cPotential     = 0;
            _dwProgMax      = PROGSINK_PROG_MAX;
            return;
        }
    }

    if (uiDelta)
    {
        uiDelta -= (!!fFwd) * PROGSINK_POTEN_BELOW;
        if (fAdd)
        {
            _cPotential += uiDelta;
            _cPotTotal  += uiDelta;
        }
        else
        {
            _cPotential += _cPotDelta;
            if (_cPotential <= uiDelta)
            {
                _cPotential = 1;            // set to 0 later
            }
            else
            {
                _cPotential -= uiDelta;
            }
            _cPotDelta = 0;
            Assert(_cPotential >= 0);
        }
    }

    Assert((cHtml + cFrame) || !_cPotential);
    TraceTag((tagProgSink,
        "[%08lX] AdjustProgress: a/f/h/p/pt/pd [%2d-(%2d, %2d) - %6d %6d %6d]",
                    this, _cActive, cFrame,
                    cHtml, _cPotential, _cPotTotal, _cPotDelta));
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::UpdateProgress
//            
//  Synopsis:   Recalculate the current progress status
//
//  BUGBUG:     need to add Databinding/OLE Controls
//----------------------------------------------------------------------------

BOOL
CProgSink::UpdateProgress(PROGDATA *ppd, DWORD dwFlags, DWORD dwPos, DWORD dwMax)
{
    DWORD       dwMaxOld    = ppd->dwMax;
    DWORD       dwPosOld    = ppd->dwPos;
    DWORD       dwProgDlt   = 0;            // progress delta
    BOOL        fSignal     = FALSE;

    TraceTag((tagProgSink,
            "[%08lX] UpdateProgress BEGIN [%08lX: <class=%2d,state=%2d,below=%2d,flags=%2d,pos=%2d,max=%2d> dwpos=%2d dwmax=%2d, setmax=%d, setpos=%d]",
            this, ppd, ppd->bClass, ppd->bState, ppd->bBelow, ppd->bFlags, ppd->dwPos, ppd->dwMax, dwPos, dwMax, dwFlags & PROGSINK_SET_MAX, dwFlags & PROGSINK_SET_POS));

    Assert (_dwProgMax <= PROGSINK_PROG_MAX);

    if (dwFlags & PROGSINK_SET_MAX)
    {
        ppd->dwMax = dwMax;
        if (ppd->dwMax < 0 || ppd->dwMax < dwMaxOld)
            goto Cleanup;

        if (!dwMaxOld && dwMax)
        {
            _dwMaxTotal += ppd->dwMax;              // add the delta max to the total max
        }
    }

    if (dwFlags & PROGSINK_SET_POS)
    {
        ppd->dwPos = dwPos;
        if (dwPos < dwPosOld)
            goto Cleanup;

        // adjust bogus data
        if (dwPos > ppd->dwMax)
        {
            ppd->dwMax = dwPos;
        }

        if (dwPos == 0 && _dwProgMax == 0)
        {            
            _dwProgMax = PROGSINK_PROG_INIT;
            _dwProgCur = PROGSINK_PROG_MIN;
        }
        else if (_cPotential && (ppd->dwPos > dwPosOld) &&
                    (ppd->bClass == PROGSINK_CLASS_HTML || 
                    ppd->bClass == PROGSINK_POTEN_FRAME))
        {
            UINT    uiPotCur    = 0;              // Current potential pos
            UINT    uiDelta     = 0;

            switch (ppd->bClass)
            {
            case PROGSINK_CLASS_HTML:
                uiDelta = PROGSINK_POTEN_HTML;
                break;
            case PROGSINK_CLASS_FRAME:
                uiDelta = PROGSINK_POTEN_FRAME;
                break;
            }

            Assert(uiDelta);

            uiDelta = uiDelta  + (ppd->bBelow ? -PROGSINK_POTEN_BELOW : 0);
            Assert(ppd->dwMax);
            uiDelta = MulDivQuick(ppd->dwPos - dwPosOld,
                                    uiDelta, ppd->dwMax);

            if (_cPotential <= uiDelta)
            {
                // we consumed all the potential
                // but we will set _cPotential to 0 only later
                _cPotential     = PROGSINK_POTEN_END;
            }
            else
            {
                _cPotential -= uiDelta;

                _cPotDelta += uiDelta;

                Assert(_cPotTotal && _cPotTotal >= _cPotential);
                uiPotCur = MulDivQuick(_cPotTotal - _cPotential,
                                        PROGSINK_ADJUST_SCALE, _cPotTotal);

                if (uiPotCur < PROGSINK_POTEN_MIN)
                    uiPotCur = PROGSINK_POTEN_MIN;

                _dwProgMax = MulDivQuick(uiPotCur, PROGSINK_PROG_MAX,
                                            PROGSINK_ADJUST_SCALE);
                Assert(_dwProgMax <= PROGSINK_PROG_MAX);
            }
        }
    }

    if (!dwMaxOld && ppd->dwMax)
    {
        _cOngoing ++;
    }

    if (_dwMaxTotal <= 0)
    {
        goto Cleanup;                       // we setPos/setMax to 0, no progress
    }

    // sometime, we get bogus data
    if (_cOngoing >= _cActive)
        _cOngoing = _cActive;

    if (dwMaxOld && ppd->dwMax > dwMaxOld || ppd->dwPos <= dwPosOld) // we receive contradictory info
    {
        goto Cleanup;
    }

    if (_dwProgMax < _dwProgCur)
    {
        _dwProgCur = _dwProgMax;
        _dwProgDelta = 0;
        goto Cleanup;
    }

    // calculate the weight of this new progress against the current total
    // get relative delta
    // Assert(_cOngoing && _cActive >= _cOngoing);
    if (_cOngoing == 0)
        goto Cleanup;

    // We adjust the divider so that we allocate 100% more space for the
    // the last 50% of active downloads
    dwProgDlt = MulDivQuick(dwPos - dwPosOld,
                            _cOngoing * (_dwProgMax - _dwProgCur),
                            _dwMaxTotal * (_cActive + (_cActive >> 1)));

    _dwMaxTotal    = _dwMaxTotal - (dwPos - dwPosOld);

    if (_dwMaxTotal < 0)
        _dwMaxTotal = 0;

    // get absolute delta and add it to the last remaining delta
    _dwProgDelta += dwProgDlt;

    if (_dwProgDelta > (_dwProgMax - _dwProgCur))
    {
        _dwProgDelta = _dwProgMax - _dwProgCur;
    }

    if (ppd->dwMax > 0 &&  ppd->dwMax == ppd->dwPos)
    {
        Assert(_cOngoing);
        _cOngoing --;
    }
    if (_cPotential == PROGSINK_POTEN_END)
    {
         _dwProgMax      = PROGSINK_PROG_MAX;
    }

Cleanup:

    if (_dwProgDelta >= PROGSINK_PROG_STEP)
    {
        _dwProgCur += _dwProgDelta;
        _dwProgCur  = (_dwProgCur > _dwProgMax)? _dwProgMax : _dwProgCur;
        _dwProgDelta = 0;
        fSignal = TRUE;
    }

#if  DBG==1
    TraceTag((tagProgSink,
            "[%08lX] UpdateProgress [%2d/%2d/%2d - max = %-6d delta = %-3d - a/o %2d/%2d]",
            this, _dwProgCur, _dwProgMax, PROGSINK_PROG_MAX, _dwMaxTotal,
            _dwProgDelta, _cActive,_cOngoing));
#endif

    return fSignal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgSink::GetClassCounter
//
//  Synopsis:   Returns quantity of class we are waiting for.
//
//----------------------------------------------------------------------------

UINT
CProgSink::GetClassCounter(DWORD dwClass, BOOL fBelow)
{
    UINT cQuantity;

    // IMPORTANT: We are not using a critical section here because we check
    // up often on this, and faulty readings will not make a difference
    // because they will be corrected in time.

    if (dwClass == (DWORD) -1)
        return _cActive;

    Assert(dwClass >= PROGSINK_CLASS_HTML && dwClass <= PROGSINK_CLASS_FRAME);

    cQuantity = (fBelow ? (_acClassBelow[dwClass]) : (_acClass[dwClass]));

    return cQuantity;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetProgSink
//
//  Synopsis:   Returns progsink interface of the current primary markup
//
//----------------------------------------------------------------------------

IProgSink *
CDoc::GetProgSink()
{
    return(PrimaryMarkup() ? PrimaryMarkup()->GetProgSink() : NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetProgSinkC
//
//  Synopsis:   Returns progsink of the current primary markup
//
//----------------------------------------------------------------------------

CProgSink *
CDoc::GetProgSinkC()
{
    return(PrimaryMarkup() ? PrimaryMarkup()->GetProgSinkC() : NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::HtmCtx()
//
//----------------------------------------------------------------------------

CHtmCtx *
CDoc::HtmCtx()
{
    return(PrimaryMarkup() ? PrimaryMarkup()->HtmCtx() : NULL);
}

