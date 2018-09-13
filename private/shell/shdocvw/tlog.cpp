#include "priv.h"
#include <hlink.h>
#include "iface.h"
#include "resource.h"

#include <mluisupp.h>

STDAPI SafeGetItemObject(IShellView *psv, UINT uItem, REFIID riid, void **ppv);


class CTravelEntry : public ITravelEntry
{
public:
    CTravelEntry(BOOL fIsLocalAnchor);

    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** ITravelEntry specific methods
    STDMETHODIMP Update(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP Invoke(IUnknown *punk);
    STDMETHODIMP GetPidl(LPITEMIDLIST *ppidl);

    static HRESULT CreateTravelEntry(IBrowserService *pbs, BOOL fIsLocalAnchor, CTravelEntry **ppte);

    void SetPrev(CTravelEntry *ptePrev);
    void SetNext(CTravelEntry *pteNext);
    CTravelEntry *GetPrev() {return _ptePrev;}
    CTravelEntry *GetNext() {return _pteNext;}
    void RemoveSelf();
    BOOL CanInvoke(IUnknown *punk, BOOL fAllowLocalAnchor);
    HRESULT GetIndexBrowser(IUnknown *punkIn, IShellBrowser **ppsbOut);
    DWORD Size();
    DWORD ListSize();
    HRESULT Clone(CTravelEntry **ppte);
    HRESULT UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext);
    HRESULT UpdateSelf(IUnknown *punk) 
        {return Update(punk, (_type == TET_LOCALANCHOR));}
    BOOL IsExternal(void)
        { return (_type==TET_EXTERNALNAV); }
    HRESULT GetDisplayName(LPTSTR psz, DWORD cch, DWORD dwFlags);
    BOOL IsEqual(LPCITEMIDLIST pidl)
        {return ILIsEqual(pidl, _pidl);}
    BOOL IsLocalAnchor(void)
        { return (_type==TET_LOCALANCHOR);}

#ifdef DEBUG
    void TransferToThreadMemlist(DWORD id);
#endif

protected:
    CTravelEntry(void);
    HRESULT _InvokeExternal(IUnknown *punk);
    HRESULT _UpdateTravelLog(IUnknown *punk, BOOL fIsLocalAnchor);
    LONG _cRef;

    ~CTravelEntry();
    void _Reset(void);
    enum {
        TET_EMPTY   = 0,
        TET_DEFAULT = 1,
        TET_LOCALANCHOR,
        TET_EXTERNALNAV
    };

    DWORD _type;            //  flags for our own sake...
    LPITEMIDLIST _pidl;            //  pidl of the entry
    HGLOBAL _hGlobalData;       //  the stream data saved by the entry
    DWORD _bid;             //  the BrowserIndex for frame specific navigation
    DWORD _dwCookie;     //  if _hGlobalData is NULL the cookie should be set
    WCHAR *_pwzTitle;
    
    IHlink *_phl;
    IHlinkBrowseContext *_phlbc;

    CTravelEntry *_ptePrev;
    CTravelEntry *_pteNext;
};


CTravelEntry::CTravelEntry(BOOL fIsLocalAnchor) : _cRef(1)
{
    //these should always be allocated
    //  thus they will always start 0
    if (fIsLocalAnchor)
        _type = TET_LOCALANCHOR;
    else
        ASSERT(!_type);
    ASSERT(!_pwzTitle);
    ASSERT(!_pidl);
    ASSERT(!_hGlobalData);
    ASSERT(!_bid);
    ASSERT(!_dwCookie);
    ASSERT(!_ptePrev);
    ASSERT(!_pteNext);
    ASSERT(!_phl);
    ASSERT(!_phlbc);
    TraceMsg(TF_TRAVELLOG, "TE[%X] created _type = %x", this, _type);

}

CTravelEntry::CTravelEntry(void) :_cRef(1)
{
    ASSERT(!_type);
    ASSERT(!_pwzTitle);
    ASSERT(!_pidl);
    ASSERT(!_hGlobalData);
    ASSERT(!_bid);
    ASSERT(!_dwCookie);
    ASSERT(!_ptePrev);
    ASSERT(!_pteNext);
    ASSERT(!_phl);
    ASSERT(!_phlbc);

    TraceMsg(TF_TRAVELLOG, "TE[%X] created", this, _type);
}

HGLOBAL CloneHGlobal(HGLOBAL hGlobalIn)
{
    DWORD dwSize = (DWORD)GlobalSize(hGlobalIn);
    HGLOBAL hGlobalOut = GlobalAlloc(GlobalFlags(hGlobalIn), dwSize);
    HGLOBAL hGlobalResult = NULL;

    if (NULL != hGlobalOut)
    {
        LPVOID pIn= GlobalLock(hGlobalIn);

        if (NULL != pIn)
        {
            LPVOID pOut= GlobalLock(hGlobalOut);

            if (NULL != pOut)
            {
                memcpy(pOut, pIn, dwSize);
                GlobalUnlock(hGlobalOut);
                hGlobalResult = hGlobalOut;
            }

            GlobalUnlock(hGlobalIn);
        }

        if (!hGlobalResult)
        {
            GlobalFree(hGlobalOut);
        }
    }

    return hGlobalResult;
}


HRESULT 
CTravelEntry::Clone(CTravelEntry **ppte)
{
    //  dont ever clone an external entry
    if (_type == TET_EXTERNALNAV)
        return E_FAIL;

    CTravelEntry *pte = new CTravelEntry();
    HRESULT hr = S_OK;

    if (pte)
    {
        pte->_type = _type;
        pte->_bid = _bid;
        pte->_dwCookie = _dwCookie;

        if (_pwzTitle)
        {
            pte->_pwzTitle = StrDup(_pwzTitle);
            if (!pte->_pwzTitle)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (_pidl)
        {
            pte->_pidl = ILClone(_pidl);
            if (!pte->_pidl)
                hr = E_OUTOFMEMORY;
        }
        else
            pte->_pidl = NULL;

        if (_hGlobalData)
        {
            pte->_hGlobalData = CloneHGlobal(_hGlobalData);

            if (NULL == pte->_hGlobalData)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            ASSERT(NULL == pte->_hGlobalData);
        }
    }
    else 
        hr = E_OUTOFMEMORY;

    if (FAILED(hr) && pte)
    {
        pte->Release();
        *ppte = NULL;
    }
    else
        *ppte = pte;

    TraceMsg(TF_TRAVELLOG, "TE[%X] Clone hr = %x", this, hr);

    return hr;
}

CTravelEntry::~CTravelEntry()
{
    ILFree(_pidl);

    if (_hGlobalData)
    {
        GlobalFree(_hGlobalData);
    }

    if (_pwzTitle)
    {
        LocalFree(_pwzTitle);
    }

    if (_pteNext)
    {
        _pteNext->Release();
    }

    ATOMICRELEASE(_phl);
    ATOMICRELEASE(_phlbc);

    TraceMsg(TF_TRAVELLOG, "TE[%X] destroyed ", this);
}

#ifdef DEBUG
void CTravelEntry::TransferToThreadMemlist(DWORD id)
{
    //must call this on every allocated thing ....
    transfer_to_thread_memlist( id, this);
    // i dont think that i have to catch pidls or pstms because they
    //are not part of the debug allocator
}
#endif

HRESULT CTravelEntry::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CTravelEntry, ITravelEntry), // IID_ITravelEntry
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CTravelEntry::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTravelEntry::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTravelEntry::GetIndexBrowser(IUnknown *punk, IShellBrowser **ppsb)
{
    HRESULT hr = E_FAIL;
    IBrowserService *pbs;
    ASSERT(ppsb);
    *ppsb = NULL;

    if (SUCCEEDED(punk->QueryInterface(IID_IBrowserService, (void **)&pbs)))
    {
        IUnknown *punkReal;

        if (SUCCEEDED(pbs->GetBrowserByIndex(_bid, &punkReal)))
        {
            hr = punkReal->QueryInterface(IID_IShellBrowser, (void **)ppsb);
            punkReal->Release();
        }
        pbs->Release();
    }

    TraceMsg(TF_TRAVELLOG, "TE[%X]::GetIndexBrowser _bid = %X, hr = %X", this, _bid, hr);

    return hr;
}

BOOL CTravelEntry::CanInvoke(IUnknown *punk, BOOL fAllowLocalAnchor)
{
    IShellBrowser *psb;
    BOOL fRet = IsLocalAnchor() ? fAllowLocalAnchor : TRUE;
    fRet = fRet && SUCCEEDED(GetIndexBrowser(punk, &psb)) ;
    ATOMICRELEASE(psb);

    return fRet;
}

DWORD CTravelEntry::Size()
{
    DWORD cbSize = SIZEOF(*this);

    if (_pidl)
        cbSize += ILGetSize(_pidl);

    if (_hGlobalData)
    {
        cbSize += (DWORD)GlobalSize(_hGlobalData);
    }

    if (_pwzTitle)
    {
        cbSize += (DWORD)LocalSize(_pwzTitle);
    }

    return cbSize;
}

DWORD CTravelEntry::ListSize()
{
    CTravelEntry *pte = GetNext();

    DWORD cb = Size();
    while (pte)
    {
        cb += pte->Size();
        pte = pte->GetNext();
    }
    return cb;
}


void CTravelEntry::_Reset()
{
    Pidl_Set(&_pidl, NULL);

    if (NULL != _hGlobalData)
    {
        GlobalFree(_hGlobalData);
        _hGlobalData = NULL;
    }

    ATOMICRELEASE(_phl);
    ATOMICRELEASE(_phlbc);

    _bid = 0;
    _type = TET_EMPTY;
    _dwCookie = 0;

    if (_pwzTitle)
    {
        LocalFree(_pwzTitle);
        _pwzTitle = NULL;
    }

    TraceMsg(TF_TRAVELLOG, "TE[%X]::_Reset", this);
}

HRESULT CTravelEntry::_UpdateTravelLog(IUnknown *punk, BOOL fIsLocalAnchor)
{
    IBrowserService *pbs;
    HRESULT hr = E_FAIL;
    //  we need to update here
    if (SUCCEEDED(punk->QueryInterface(IID_IBrowserService, (void **)&pbs)))
    {
        ITravelLog *ptl;
        if (SUCCEEDED(pbs->GetTravelLog(&ptl)))
        {
            hr = ptl->UpdateEntry(punk, fIsLocalAnchor);
            ptl->Release();
        }
        pbs->Release();
    }

    return hr;
}

HRESULT CTravelEntry::_InvokeExternal(IUnknown *punk)
{
    HRESULT hr = E_FAIL;

    ASSERT(_phl);
    ASSERT(_phlbc);
    
    TraceMsg(TF_TRAVELLOG, "TE[%X]::InvokeExternal entered on _bid = %X, _phl = %X, _phlbc = %X", this, _bid, _phl, _phlbc);

    // set the size and position of the browser frame window, so that the
    // external target can sync up its frame window to those coordinates
    HLBWINFO hlbwi;

    hlbwi.cbSize = sizeof(hlbwi);
    hlbwi.grfHLBWIF = 0;

    IOleWindow *pow;
    HWND hwnd = NULL;
    if (SUCCEEDED(punk->QueryInterface(IID_IOleWindow, (void **)&pow)))
    {
        pow->GetWindow(&hwnd);
        pow->Release();
    }

    if (hwnd) 
    {
        WINDOWPLACEMENT wp = {0};

        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &wp);
        hlbwi.grfHLBWIF = HLBWIF_HASFRAMEWNDINFO;
        hlbwi.rcFramePos = wp.rcNormalPosition;
        if (wp.showCmd == SW_SHOWMAXIMIZED)
            hlbwi.grfHLBWIF |= HLBWIF_FRAMEWNDMAXIMIZED;
    }

    _phlbc->SetBrowseWindowInfo(&hlbwi);

    //
    //  right now we always now we are going back, but later on
    //  maybe we should ask the browser whether this is back or forward
    //
    hr = _phl->Navigate(HLNF_NAVIGATINGBACK, NULL, NULL, _phlbc);
    
    IServiceProvider *psp; 
    if (SUCCEEDED(punk->QueryInterface(IID_IServiceProvider, (void **)&psp)))
    {
        IWebBrowser2 *pwb;
        ASSERT(psp);
        if (SUCCEEDED(psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&pwb)))
        {
            ASSERT(pwb);
            pwb->put_Visible(FALSE);
            pwb->Release();
        }

        psp->Release();
    }

    _UpdateTravelLog(punk, FALSE);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::InvokeExternal exited hr = %X", this, hr);

    return hr;
}

HRESULT CTravelEntry::Invoke(IUnknown *punk)
{
    IPersistHistory *pph = NULL;
    HRESULT hr = E_FAIL;
    IShellBrowser *psb = NULL;

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Invoke entered on _bid = %X", this, _bid);
    TraceMsgW(TF_TRAVELLOG, "TE[%X]::Invoke title '%s'", this, _pwzTitle);

    if (_type == TET_EXTERNALNAV)
    {
        hr = _InvokeExternal(punk);
        goto Quit;
    }

    if (FAILED(GetIndexBrowser(punk, &psb)))
        goto Quit;

    hr = psb->QueryInterface(IID_IPersistHistory, (void **)&pph);

    if (SUCCEEDED(hr))
    {
        ASSERT(pph);

        if (_type == TET_LOCALANCHOR)
        {
            hr = pph->SetPositionCookie(_dwCookie);
        }
        else
        {
            //  we need to clone it
            ASSERT(_hGlobalData);
            
            HGLOBAL hGlobal = CloneHGlobal(_hGlobalData);

            if (NULL != hGlobal)
            {
                IStream *pstm;
                
                hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pstm);

                if (SUCCEEDED(hr))
                {
                    hr = pph->LoadHistory(pstm, NULL);
                    pstm->Release();
                }
                else
                {
                    GlobalFree(hGlobal);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        pph->Release();
    }

Quit:

    SAFERELEASE(psb);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Invoke exited on _bid = %X, hr = %X", this, _bid, hr);
    return hr;
}

HRESULT CTravelEntry::UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    TraceMsg(TF_TRAVELLOG, "TE[%X]::UpdateExternal entered on punk = %X, punkhlbc = %X", this, punk, punkHLBrowseContext);

    _Reset();
    ASSERT(punkHLBrowseContext);
    punkHLBrowseContext->QueryInterface(IID_IHlinkBrowseContext, (void **)&_phlbc);
    ASSERT(_phlbc);

    _type = TET_EXTERNALNAV;

    HRESULT hr = E_FAIL;

    //
    //  right now we only support externals being previous.  we never actually navigate
    //  to another app.  we handle everything in pane ourselves.
    //  so theoretically we never need to worry about HLID_NEXT
    _phlbc->GetHlink((ULONG) HLID_PREVIOUS, &_phl);
    
    IBrowserService *pbs;
    punk->QueryInterface(IID_IBrowserService, (void **)&pbs);


    if (pbs && _phl) 
    {
        _bid = pbs->GetBrowserIndex();

        WCHAR *pwszTarget;
        hr = _phl->GetStringReference(HLINKGETREF_ABSOLUTE, &pwszTarget, NULL);
        if (SUCCEEDED(hr))
        {
            TCHAR szName[MAX_URL_STRING];
            StrCpyN(szName, pwszTarget, ARRAYSIZE(szName));
            OleFree(pwszTarget);

            // create pidl
            hr = IECreateFromPath(szName, &_pidl);
        }
    }

    ATOMICRELEASE(pbs);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::UpdateExternal exited _bid = %X, hr = %X", this, _bid, hr);

    return hr;
}

HRESULT CTravelEntry::Update(IUnknown *punk, BOOL fIsLocalAnchor)
{
    ASSERT(punk);
    
    //  this means that we went back to an external app, 
    //  and now we are going forward again.  we dont persist 
    //  any state info about them that would be different.
    if (_type == TET_EXTERNALNAV)
    {
        TraceMsg(TF_TRAVELLOG, "TE[%X]::Update NOOP on external entry", this);
        return S_OK;
    }

    IBrowserService *pbs;
    IShellBrowser *psb = NULL;
    IShellView *psv = NULL;
    HRESULT hr = punk->QueryInterface(IID_IBrowserService, (void **)&pbs);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Update entered on pbs = %X", this, pbs);

    pbs->QueryInterface(IID_IShellBrowser, (void **)&psb);
    psb->QueryActiveShellView(&psv);

    _Reset();

    if (SUCCEEDED(hr))
    {

        if (SUCCEEDED(pbs->GetPidl(&_pidl)))
        {
            IPersistHistory *pph;
            pbs->QueryInterface(IID_IPersistHistory, (void **)&pph);
            ASSERT(_pidl);
            ASSERT(pph);

            _bid = pbs->GetBrowserIndex();

            if (psv)
            {
                //  pick up the title as a display name 
                //  for menus and the like
                WCHAR wzTitle[MAX_PATH];
                
                pbs->GetTitle(psv, wzTitle, SIZECHARS(wzTitle));
                _pwzTitle = StrDup(wzTitle);
                
                TraceMsgW(TF_TRAVELLOG, "TE[%X]::Update title '%s'", this, _pwzTitle);

            }

            if (fIsLocalAnchor)
            {
                //
                //  persist a cookie
                _type = TET_LOCALANCHOR;
                hr = pph->GetPositionCookie(&_dwCookie);
            }
            else
            {
                //
                //  persist a stream

                ASSERT(!_hGlobalData);

                IStream *pstm;

                hr = CreateStreamOnHGlobal(NULL, FALSE, &pstm);
                
                if (SUCCEEDED(hr))
                {
                    ASSERT(pph);
                    _type = TET_DEFAULT;
                    
                    hr = pph->SaveHistory(pstm);

                    STATSTG stg;
                    HRESULT hrStat = pstm->Stat(&stg, STATFLAG_NONAME);

                    hr = GetHGlobalFromStream(pstm, &_hGlobalData);

                    pstm->Release();

                    //  This little exercise here is to shrink the memory block we get from
                    //  the OLE API which allocates blocks in chunks of 8KB.  Typical stream
                    //  sizes are only a few hundred bytes.
                    
                    if (SUCCEEDED(hrStat))
                    {
                        HGLOBAL hGlobalTemp = GlobalReAlloc(_hGlobalData, stg.cbSize.LowPart, GMEM_MOVEABLE);

                        if (NULL != hGlobalTemp)
                        {
                            _hGlobalData = hGlobalTemp;
                        }
                    }

                }

            }

            pph->Release();
        }
        else //_pidl == NULL
            hr = E_OUTOFMEMORY;

    }

    if (FAILED(hr))
        _Reset();

    SAFERELEASE(psb);
    SAFERELEASE(psv);
    SAFERELEASE(pbs);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Update exited on _bid = %X, hr = %X", this, _bid, hr);

    return hr;
}

HRESULT CTravelEntry::GetPidl(LPITEMIDLIST * ppidl)
{
    if (EVAL(ppidl))
    {
        *ppidl = ILClone(_pidl);
        if (*ppidl)
            return S_OK;
    }
    return E_FAIL;
}

void CTravelEntry::SetNext(CTravelEntry *pteNext)
{
    if (_pteNext)
        _pteNext->Release();

    _pteNext = pteNext;

    if (_pteNext) 
    {
        _pteNext->_ptePrev = this;
    }
}

// the only time we prepend the link is to the top of the entire chain (bottom of the stack)
//
// the topmost element has a ref count to it that we must then assume
void CTravelEntry::SetPrev(CTravelEntry *ptePrev)
{
    ASSERT(!_ptePrev);
    _ptePrev = ptePrev;
    if (_ptePrev)
        _ptePrev->SetNext(this);
}

//
//  this is for removing from the middle of the list...
//
void CTravelEntry::RemoveSelf()
{
    if (_pteNext)
        _pteNext->_ptePrev = _ptePrev;

    // remove yourself from the list
    if (_ptePrev) 
    {
        // after this point, we may be destroyed so can't touch any more member vars
        _ptePrev->_pteNext = _pteNext;
    }

    _ptePrev = NULL;
    _pteNext = NULL;

    // we lose a reference now because we're gone from _ptePrev's _pteNext
    // (or if we were the top of the list, we're also nuked)
    Release();
}


HRESULT GetUnescapedUrlIfAppropriate(LPCITEMIDLIST pidl, LPTSTR pszUrl, DWORD cch)
{
    TCHAR szUrl[MAX_URL_STRING];

    // The SHGDN_NORMAL display name will be the pretty name (Web Page title) unless
    // it's an FTP URL or the web page didn't set a title.
    if (SUCCEEDED(IEGetDisplayName(pidl, szUrl, SHGDN_NORMAL)) &&
        UrlIs(szUrl, URLIS_URL))
    {
        // NT #279192, If an URL is escaped, it normally contains three types of
        // escaped chars.
        // 1) Seperating type chars ('#' for frag, '?' for params, etc.)
        // 2) DBCS chars,
        // 3) Data (a bitmap in the url by escaping the binary bytes)
        // Since #2 is very common, we want to try to unescape it so it has meaning
        // to the user.  UnEscaping isn't safe if the user can copy or modify the data
        // because they could loose data when it's reparsed.  One thing we need to
        // do for #2 to work is for it to be in ANSI when unescaped.  This is needed
        // or the DBCS lead and trail bytes will be in unicode as [0x<LeadByte> 0x00]
        // [0x<TrailByte> 0x00].  Being in ANSI could cause a problem if the the string normally
        // crosses code pages, but that is uncommon or non-existent in the IsURLChild()
        // case.
        CHAR szUrlAnsi[MAX_URL_STRING];

        SHTCharToAnsi(szUrl, szUrlAnsi, ARRAYSIZE(szUrlAnsi));
        UrlUnescapeA(szUrlAnsi, NULL, NULL, URL_UNESCAPE_INPLACE|URL_UNESCAPE_HIGH_ANSI_ONLY);
        SHAnsiToTChar(szUrlAnsi, pszUrl, cch);
    }
    else
    {
        StrCpyN(pszUrl, szUrl, cch);    // Truncate if needed
    }

    return S_OK;
}



#define TEGDN_FORSYSTEM     0x00000001

HRESULT CTravelEntry::GetDisplayName(LPTSTR psz, DWORD cch, DWORD dwFlags)
{
    if (!psz || !cch)
        return E_INVALIDARG;

    psz[0] = 0;
    if ((NULL != _pwzTitle) && (*_pwzTitle != 0))
    {
        StrCpyNW(psz, _pwzTitle, cch);
    }
    else if (_pidl)
    {
        GetUnescapedUrlIfAppropriate(_pidl, psz, cch);
    }

    if (dwFlags & TEGDN_FORSYSTEM)
    {
        if (!SHIsDisplayable(psz, g_fRunOnFE, g_bRunOnNT5))
        {
            // Display name isn't system-displayable.  Just use the path/url instead.
            SHTitleFromPidl(_pidl, psz, cch, FALSE);
        }
    }

    return psz[0] ? S_OK : E_FAIL;
}

class CTravelLog : public ITravelLog
{
public:
    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef() ;
    STDMETHODIMP_(ULONG) Release();

    // *** ITravelLog specific methods
    STDMETHODIMP AddEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext);
    STDMETHODIMP Travel(IUnknown *punk, int iOffset);
    STDMETHODIMP GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte);
    STDMETHODIMP FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte);
    STDMETHODIMP GetToolTipText(IUnknown *punk, int iOffset, int idsTemplate, LPWSTR pwzText, DWORD cchText);
    STDMETHODIMP InsertMenuEntries(IUnknown *punk, HMENU hmenu, int nPos, int idFirst, int idLast, DWORD dwFlags);
    STDMETHODIMP Clone(ITravelLog **pptl);
    STDMETHODIMP_(DWORD) CountEntries(IUnknown *punk);
    STDMETHODIMP Revert(void);

    CTravelLog();

#ifdef DEBUG
    void TransferToThreadMemlist(DWORD id);
#endif

protected:
    ~CTravelLog();
    HRESULT _FindEntryByOffset(IUnknown *punk, int iOffset, CTravelEntry **ppte);
    void _Prune(void);

    LONG _cRef;
    DWORD _cbMaxSize;
    DWORD _cbTotalSize;

    CTravelEntry *_pteCurrent;  //pteCurrent
    CTravelEntry *_pteUpdate;
    CTravelEntry *_pteRoot;
};

CTravelLog::CTravelLog() : _cRef(1) 
{
    ASSERT(!_pteCurrent);
    ASSERT(!_pteUpdate);
    ASSERT(!_pteRoot);

    DWORD dwType, dwSize = SIZEOF(_cbMaxSize), dwDefault = 1024 * 1024;
    
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TravelLog"), TEXT("MaxSize"), &dwType, (LPVOID)&_cbMaxSize, &dwSize, FALSE, (void *)&dwDefault, SIZEOF(dwDefault));
    TraceMsg(TF_TRAVELLOG, "TL[%X] created", this);
}

CTravelLog::~CTravelLog()
{
    //DestroyList by releasing the root
    SAFERELEASE(_pteRoot);
    TraceMsg(TF_TRAVELLOG, "TL[%X] destroyed ", this);
}

HRESULT CTravelLog::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CTravelLog, ITravelLog), // IID_ITravelLog
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CTravelLog::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTravelLog::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTravelLog::AddEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    ASSERT(punk);

    if(SHRestricted2W(REST_NoNavButtons, NULL, 0))
    {
        return S_FALSE;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::AddEntry punk = %X, IsLocal = %s", this, punk, fIsLocalAnchor ? "TRUE" : "FALSE");
    CTravelEntry *pte = new CTravelEntry(fIsLocalAnchor);
    if (pte)
    {
        //replace the current with the new

        if (_pteCurrent)
        {
            CTravelEntry *pteNext = _pteCurrent->GetNext();
            if (pteNext)
            {
                _cbTotalSize -= pteNext->ListSize();
            }

            //  the list keeps its own ref count, and only needs
            //  to be modified when passed outside of the list

            //  setnext will release the current next if necessary
            //  this will also set pte->prev = pteCurrent
            _pteCurrent->SetNext(pte);
        }
        else
            _pteRoot = pte;

        _cbTotalSize += pte->Size();

        _pteCurrent = pte;

        ASSERT(_cbTotalSize == _pteRoot->ListSize());
    }
    TraceMsg(TF_TRAVELLOG, "TL[%X]::AddEntry punk = %X, IsLocal = %d, pte = %X", this, punk, fIsLocalAnchor, pte);

    return pte ? S_OK : E_OUTOFMEMORY;
}

void CTravelLog::_Prune(void)
{
    // BUGBUGTODO need an increment or something

    ASSERT(_cbTotalSize == _pteRoot->ListSize());

    while (_cbTotalSize > _cbMaxSize && _pteRoot != _pteCurrent)
    {
        CTravelEntry *pte = _pteRoot;
        _pteRoot = _pteRoot->GetNext();

        _cbTotalSize -= pte->Size();
        pte->RemoveSelf();

        ASSERT(_cbTotalSize == _pteRoot->ListSize());
    }
}


HRESULT CTravelLog::UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    CTravelEntry *pte = _pteUpdate ? _pteUpdate : _pteCurrent;

    //  this can happen under weird stress conditions, evidently
    if (!pte)
        return E_FAIL;

    _cbTotalSize -= pte->Size();
    HRESULT hr = pte->Update(punk, fIsLocalAnchor);
    _cbTotalSize += pte->Size();

    ASSERT(_cbTotalSize == _pteRoot->ListSize());

    // Debug prints need to be before _Prune() since pte can get freed by _Prune() resulting
    // in a crash if pte->Size() is called
    TraceMsg(TF_TRAVELLOG, "TL[%X]::UpdateEntry pte->Size() = %d", this, pte->Size());
    TraceMsg(TF_TRAVELLOG, "TL[%X]::UpdateEntry punk = %X, IsLocal = %d, hr = %X", this, punk, fIsLocalAnchor, hr);
    
    _Prune();

    _pteUpdate = NULL;

    return hr;
}

HRESULT CTravelLog::UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    CTravelEntry *pte = _pteUpdate ? _pteUpdate : _pteCurrent;

    ASSERT(punk);
    ASSERT(pte);
    ASSERT(punkHLBrowseContext);

    if (pte)
        return pte->UpdateExternal(punk, punkHLBrowseContext);

    return E_FAIL;
}

HRESULT CTravelLog::Travel(IUnknown *punk, int iOffset)
{
    ASSERT(punk);
    HRESULT hr = E_FAIL;

    CTravelEntry *pte;

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Travel entered with punk = %X, iOffset = %d", this, punk, iOffset);

    if (SUCCEEDED(_FindEntryByOffset(punk, iOffset, &pte)))
    {
        // we will update where we are before we move away...
        //  but external navigates dont go through the normal activation
        //  so we dont want to setup the external to be updated
        //  _pteUpdate is also what allows us to Revert().
        if (!_pteCurrent->IsExternal() && !_pteUpdate)
            _pteUpdate = _pteCurrent;

        _pteCurrent = pte;
        hr = _pteCurrent->Invoke(punk);

        //
        //  if the entry bails with an error, then we need to reset ourself
        //  to what we were.  right now, the only place this should happen
        //  is if an Abort was returned from SetPositionCookie
        //  because somebody aborted during before navigate.
        //  but i think that any error means that we can legitimately Revert().
        //
        if (FAILED(hr))
        {
            Revert();
        }
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Travel exited with hr = %X", this, hr);

    return hr;
}


HRESULT CTravelLog::_FindEntryByOffset(IUnknown *punk, int iOffset, CTravelEntry **ppte)
{
    CTravelEntry *pte = _pteCurrent;
    BOOL fAllowLocalAnchor = TRUE;

    if (iOffset < 0)
    {
        while (iOffset && pte)
        {
            pte = pte->GetPrev();
            if (pte && pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                iOffset++;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }

        }
    }
    else if (iOffset > 0)
    {
        while (iOffset && pte)
        {
            pte = pte->GetNext();
            if (pte && pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                iOffset--;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }
        }
    }

    if (pte)
    {

        *ppte = pte;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CTravelLog::GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte)
{
    HRESULT hr;
    BOOL fCheckExternal = FALSE;
    if (iOffset == TLOG_BACKEXTERNAL) 
    {
        iOffset = TLOG_BACK;
        fCheckExternal = TRUE;
    }

    if (iOffset == 0)
    {
        //  BUGBUGCOMPAT - going back and fore between external apps is dangerous - zekel 24-JUN-97
        //  we always fail if the current is external
        //  this is because word will attempt to navigate us to 
        //  the same url instead of FORE when the user selects
        //  it from the drop down.
        if (_pteCurrent && _pteCurrent->IsExternal())
        {
            hr = E_FAIL;
            ASSERT(!_pteCurrent->GetPrev());
            TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry current is External", this);
            goto Quit;
        }
    }

    CTravelEntry *pte;
    hr = _FindEntryByOffset(punk, iOffset, &pte);

    //
    // If TLOG_BACKEXTERNAL is specified, we return S_OK only if the previous
    // entry is external.
    //
    if (fCheckExternal && SUCCEEDED(hr)) {
        if (!pte->IsExternal()) {
            hr = E_FAIL;
        }
        TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry(BACKEX)", this);
    }

    if (ppte && SUCCEEDED(hr)) {
        hr = pte->QueryInterface(IID_ITravelEntry, (void **) ppte);
    }

Quit:

    TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry iOffset = %d, hr = %X", this, iOffset, hr);

    return hr;
}

HRESULT CTravelLog::FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte)
{
    CTravelEntry *pte = _pteRoot;
    BOOL fAllowLocalAnchor = TRUE;
    
    while (pte)
    {
        if (pte->CanInvoke(punk, fAllowLocalAnchor) && pte->IsEqual(pidl))
            break;

        fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();

        pte = pte->GetNext();
    }

    if (pte)
    {
        return pte->QueryInterface(IID_ITravelEntry, (void **)ppte);
    }

    *ppte =  NULL;
    return E_FAIL;
}

HRESULT CTravelLog::Clone(ITravelLog **pptl)
{
    CTravelLog *ptl = new CTravelLog();
    HRESULT hr = S_OK;

    if (ptl && _pteCurrent)
    {
        // first set the current pointer
        hr = _pteCurrent->Clone(&ptl->_pteCurrent);

        if (SUCCEEDED(hr))
        {
            ptl->_cbTotalSize = _cbTotalSize;
            
            CTravelEntry *pteSrc;
            CTravelEntry *pteClone, *pteDst = ptl->_pteCurrent;
            
            //  then we need to loop forward and set each
            for (pteSrc = _pteCurrent->GetNext(), pteDst = ptl->_pteCurrent;
                pteSrc; pteSrc = pteSrc->GetNext())
            {
                ASSERT(pteDst);
                if (FAILED(pteSrc->Clone(&pteClone)))
                    break;

                ASSERT(pteClone);
                pteDst->SetNext(pteClone);
                pteDst = pteClone;
            }
                
            //then loop back and set them all
            for (pteSrc = _pteCurrent->GetPrev(), pteDst = ptl->_pteCurrent;
                pteSrc; pteSrc = pteSrc->GetPrev())
            {
                ASSERT(pteDst);
                if (FAILED(pteSrc->Clone(&pteClone)))
                    break;

                ASSERT(pteClone);
                pteDst->SetPrev(pteClone);
                pteDst = pteClone;
            }   

            //  the root is the furthest back we could go
            ptl->_pteRoot = pteDst;

        }


    }
    else 
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        ptl->QueryInterface(IID_ITravelLog, (void **) pptl);
    }
    else 
    {
        *pptl = NULL;
    }
    
    if (ptl) 
        ptl->Release();

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Clone hr = %x, ptlClone = %X", this, hr, ptl);

    return hr;
}

// HACKHACK: 3rd parameter used to be idsTemplate, which we would use to grab the
// string template.  However, since there's no way the caller can specify the hinst
// of the module in which to look for this resource, this broke in the shdocvw /
// browseui split (callers would pass offsets into browseui.dll; we'd look for them in
// shdocvw.dll).  My solution is is to ignore this parameter entirely and assume that:
//
//  if iOffset is negative, the caller wants the "back to" text
//  else, the caller wants the "forward to" text
//
// tjgreen 14-july-98.
//
HRESULT CTravelLog::GetToolTipText(IUnknown *punk, int iOffset, int, LPWSTR pwzText, DWORD cchText)
{
    TCHAR szName[MAX_URL_STRING];
    TCHAR szTemplate[80];

    TraceMsg(TF_TRAVELLOG, "TL[%X]::ToolTip entering iOffset = %d, ptlClone = %X", this, iOffset);
    ASSERT(pwzText);
    ASSERT(cchText);

    *pwzText = 0;

    CTravelEntry *pte;
    HRESULT hr = _FindEntryByOffset(punk, iOffset, &pte);
    if (SUCCEEDED(hr))
    {
        ASSERT(pte);

        pte->GetDisplayName(szName, SIZECHARS(szName), 0);

        int idsTemplate = (iOffset < 0) ? IDS_NAVIGATEBACKTO : IDS_NAVIGATEFORWARDTO;

        if (MLLoadString(idsTemplate, szTemplate, ARRAYSIZE(szTemplate))) {
            DWORD cchTemplateLen = lstrlen(szTemplate);
            DWORD cchLen = cchTemplateLen + lstrlen(szName);
            if (cchLen > cchText) {
                // so that we don't overflow the pwzText buffer
                szName[cchText - cchTemplateLen - 1] = 0;
            }

            wnsprintf(pwzText, cchText, szTemplate, szName);
        }
        else
            hr = E_UNEXPECTED;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::ToolTip exiting hr = %X, pwzText = %ls", this, hr, pwzText);
    return hr;
}

HRESULT CTravelLog::InsertMenuEntries(IUnknown *punk, HMENU hmenu, int iIns, int idFirst, int idLast, DWORD dwFlags)
{
    ASSERT(idLast >= idFirst);
    ASSERT(hmenu);
    ASSERT(punk);

    int cItemsBack = idLast - idFirst + 1;
    int cItemsFore = 0;
    
    CTravelEntry *pte;
    LONG cAdded = 0;
    TCHAR szName[40];
    DWORD cchName = SIZECHARS(szName);
    UINT uFlags = MF_STRING | MF_ENABLED | MF_BYPOSITION;
    TraceMsg(TF_TRAVELLOG, "TL[%X]::InsertMenuEntries entered on punk = %X, hmenu = %X, iIns = %d, idRange = %d-%d, flags = %X", this, punk, hmenu, iIns, idFirst, idLast, dwFlags);


    ASSERT(cItemsFore >= 0);
    ASSERT(cItemsBack >= 0);

    if (IsFlagSet(dwFlags, TLMENUF_INCLUDECURRENT))
        cItemsBack--;

    if (IsFlagSet(dwFlags, TLMENUF_BACKANDFORTH))
    {
        cItemsFore = cItemsBack / 2;
        cItemsBack = cItemsBack - cItemsFore;
    }
    else if (IsFlagSet(dwFlags, TLMENUF_FORE))
    {
        cItemsFore = cItemsBack;
        cItemsBack = 0;
    }

    while (cItemsFore)
    {
        if (SUCCEEDED(_FindEntryByOffset(punk, cItemsFore, &pte)))
        {
            pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
            ASSERT(*szName);
            FixAmpersands(szName, ARRAYSIZE(szName));
            InsertMenu(hmenu, iIns, uFlags, idLast, szName);
            cAdded++;
            TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Fore id = %d, szName = %s", this, idLast, szName);
        }
        
        cItemsFore--;
        idLast--;
    }

    if (IsFlagSet(dwFlags, TLMENUF_INCLUDECURRENT))
    {
        // clear the name
        *szName = 0;

        //have to get the title from the actual pbs
        IBrowserService *pbs ;
        WCHAR wzTitle[MAX_PATH];

        LPITEMIDLIST pidl = NULL;

        if (SUCCEEDED(punk->QueryInterface(IID_IBrowserService, (void **)&pbs)))
        {
            pbs->GetPidl(&pidl);

            if (SUCCEEDED(pbs->GetTitle(NULL, wzTitle, SIZECHARS(wzTitle))))
            {
                StrCpyN(szName, wzTitle, cchName);
            }
            else if (pidl)
            {
                GetUnescapedUrlIfAppropriate(pidl, szName, ARRAYSIZE(szName));
            }

            pbs->Release();
        }

        if (!SHIsDisplayable(szName, g_fRunOnFE, g_bRunOnNT5) && pidl)
        {
            // Display name isn't system-displayable.  Just use the path/url instead.
            SHTitleFromPidl(pidl, szName, ARRAYSIZE(szName), FALSE);
        }

        if (!(*szName))
            TraceMsg(TF_ERROR, "CTravelLog::InsertMenuEntries -- failed to find title for current entry");

        ILFree(pidl);

        FixAmpersands(szName, ARRAYSIZE(szName));
        InsertMenu(hmenu, iIns, uFlags | (IsFlagSet(dwFlags, TLMENUF_CHECKCURRENT) ? MF_CHECKED : 0), idLast, szName);
        cAdded++;
        TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Current id = %d, szName = %s", this, idLast, szName);

        idLast--;
    }

    
    if (IsFlagSet(dwFlags, TLMENUF_BACKANDFORTH))
    {
        //  we need to reverse the order of insertion for back
        //  when both directions are displayed
        int i;
        for (i = 1; i <= cItemsBack; i++, idLast--)
        {
            if (SUCCEEDED(_FindEntryByOffset(punk, -i, &pte)))
            {
                pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
                ASSERT(*szName);
                FixAmpersands(szName, ARRAYSIZE(szName));
                InsertMenu(hmenu, iIns, uFlags, idLast, szName);
                cAdded++;
                TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Back id = %d, szName = %s", this, idLast, szName);

            }
        }
    }
    else while (cItemsBack)
    {
        if (SUCCEEDED(_FindEntryByOffset(punk, -cItemsBack, &pte)))
        {
            pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
            ASSERT(*szName);
            FixAmpersands(szName, ARRAYSIZE(szName));
            InsertMenu(hmenu, iIns, uFlags, idLast, szName);
            cAdded++;
            TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Back id = %d, szName = %s", this, idLast, szName);
        }

        cItemsBack--;
        idLast--;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::InsertMenuEntries exiting added = %d", this, cAdded);
    return cAdded ? S_OK : S_FALSE;
}

DWORD CTravelLog::CountEntries(IUnknown *punk)
{
    CTravelEntry *pte = _pteRoot;
    DWORD dw = 0;
    BOOL fAllowLocalAnchor = TRUE;

    while (pte)
    {
        if (pte->CanInvoke(punk, fAllowLocalAnchor))
            dw++;

        fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();

        pte = pte->GetNext();
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::CountEntries count = %d", this, dw);
    return dw;
}

HRESULT CTravelLog::Revert(void)
{
    // this function should only be called when
    //  we have travelled, and we stop the travel before finishing
    if (_pteUpdate)
    {
        // trade them back
        _pteCurrent = _pteUpdate;
        _pteUpdate = NULL;
        return S_OK;
    }
    return E_FAIL;
}

#ifdef DEBUG
void CTravelLog::TransferToThreadMemlist(DWORD id)
{
    //must call this on every allocated thing ....
    transfer_to_thread_memlist(id, this);

    CTravelEntry *pte = _pteRoot;
    while (pte)
    {
        pte->TransferToThreadMemlist(id);
        pte = pte->GetNext();
    }
}
#endif

#ifdef TRAVELDOCS
GetNewDocument()
{
    new CTraveledDocument();

    ptd->Init();

    DPA_Add(ptd);
    DPA_Sort();
}

// really need to use ILIsEqual() instead of this

int ILCompareFastButWrong(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    int iret;
    DWORD cb1 = ILGetSize(ptd1->_pidl); 
    DWORD cb2 = ILGetSize(ptd2->_pidl);
    iret = cb1 - cb2;
    if (0 == iret)
        iret = memcmp(pidl1, cb1, pidl2, cb2);
    return iret;
}

static int CTraveledDocument::Compare(PTRAVELEDDOCUMENT ptd1, PTRAVELEDDOCUMENT ptd2)
{
    int iret;
    
    iret = ptd1->_type - ptd2->_type;
    if (0 = iret)
    {
        iret = ptd1->_hash - ptd2->_hash;
        if (0 == iret)
        {
            switch (ptd1->_type)
            {
            case TDOC_PIDL:
                iret = ILCompareFastButWrong(ptd1->_pidl, ptd2->_pidl);
                break;

            case TDOC_URL:
                iret = UrlCompare((LPTSTR)ptd1->_strUrl, (LPTSTR)ptd2->_strUrl, FALSE);
                break;

            default:
                ASSERT(FALSE);
            }
        }
    }
    return iret;
}
#endif //TRAVELDOCS


HRESULT CreateTravelLog(ITravelLog **pptl)
{
    HRESULT hres;
    CTravelLog *ptl =  new CTravelLog();
    if (ptl)
    {
        hres = ptl->QueryInterface(IID_ITravelLog, (void **)pptl);
        ptl->Release();
    }
    else
    {
        *pptl = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

SHDOCAPI_(void) TLTransferToThreadMemlist(ITravelLog *ptl, DWORD id)
{
#ifdef DEBUG
    CTravelLog *ptlReal = (CTravelLog *)ptl;
    ptlReal->TransferToThreadMemlist(id);
#endif
}
    
