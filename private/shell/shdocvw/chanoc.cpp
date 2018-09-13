//
// This Channel OC code was copied from browseui.  Once it is reenabled in
// browseui it can be removed from shdocvw.
//

#include "priv.h"
#include "cobjsafe.h"


STDAPI ChannelOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
HRESULT IUnknown_SetBandInfoSFB(IUnknown *punkBand, BANDINFOSFB *pbi);
IDeskBand * ChannelBand_Create(LPCITEMIDLIST pidlDefault);
//LPITEMIDLIST Channel_GetFolderPidl();
void Channels_SetBandInfoSFB(IUnknown* punkBand);

//////////////////////////////////////////////////
//
// ChannelOC
//
// This is an OC that wraps the above band for
// inclusion in the Active Desktop.
//
// TODO:
//  - listen to ISFBand for resizes and request them
//    from our container
//
// BUGBUG BUGBUG: We need to register as a drop target!

#undef  SUPERCLASS
#define SUPERCLASS CShellEmbedding
#undef  THISCLASS
#define THISCLASS ChannelOC

class ChannelOC : public SUPERCLASS
    , public IServiceProvider
    , public IPersistPropertyBag
    , public CObjectSafety
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) { return SUPERCLASS::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void) { return SUPERCLASS::Release(); }

    // *** CAggregatedUnknown ***
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IServiceProvider ***
    virtual STDMETHODIMP QueryService(REFGUID guidService,
        REFIID riid, void **ppvObj);

    // *** IPersistPropertyBag ***
    virtual STDMETHODIMP Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog);
    virtual STDMETHODIMP Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    virtual STDMETHODIMP InitNew();
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) { return SUPERCLASS::GetClassID(pClassID); };

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect);

protected:
    ChannelOC(IUnknown* punkOuter, LPCOBJECTINFO poi);
    ~ChannelOC();
    friend HRESULT ChannelOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    virtual LRESULT v_WndProc(HWND, UINT, WPARAM, LPARAM);

    LPITEMIDLIST _GetInitialPidl(void);
    virtual void _OnInPlaceActivate(void);      // called when we actually go in-place-active
    virtual void _OnInPlaceDeactivate(void);    // called when we actually deactivate

    IDeskBand*          _pBand;
    IWinEventHandler*   _pweh;
    COLORREF            _crBkgndOC;
    COLORREF            _crBorder;

    // for GetBandInfo, not currently used
    DESKBANDINFO _dbi;
};

ChannelOC::ChannelOC(IUnknown* punkOuter, LPCOBJECTINFO poi) : 
    CShellEmbedding(punkOuter, poi, NULL)
{
    TraceMsg(TF_SHDLIFE, "ctor ChannelOC %x", this);
    _crBkgndOC = CLR_DEFAULT;
    _crBorder = CLR_DEFAULT;
}


ChannelOC::~ChannelOC() 
{
    TraceMsg(TF_SHDLIFE, "dtor ChannelOC %x", this);
}

HRESULT ChannelOC::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(THISCLASS, IServiceProvider),
        QITABENT(THISCLASS, IPersistPropertyBag),
        QITABENT(THISCLASS, IObjectSafety),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = SUPERCLASS::v_InternalQueryInterface(riid, ppvObj);

    return hres;
}

HRESULT ChannelOC::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    return IUnknown_QueryService(_pcli, guidService, riid, ppvObj);
}

//***   ChannelOC::IPersistPropertyBag::* {
//
HRESULT THISCLASS::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    ASSERT(_crBkgndOC == CLR_DEFAULT);  // paranoia
    _crBkgndOC = PropBag_ReadInt4(pPropBag, L"BGColor", CLR_DEFAULT);
    TraceMsg(TF_WARNING, "coc.l: BGColor=%x", _crBkgndOC);
    
    _crBorder = PropBag_ReadInt4(pPropBag, L"BorderColor", CLR_DEFAULT);
    
    return S_OK;
}

HRESULT THISCLASS::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT THISCLASS::InitNew()
{
    ASSERT(_crBkgndOC == CLR_DEFAULT);
    ASSERT(_crBorder == CLR_DEFAULT);
    return E_NOTIMPL;
}

// }

LRESULT ChannelOC::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    switch(uMsg)
    {
    case WM_CONTEXTMENU:
        return 1;

    case WM_WINDOWPOSCHANGED:
        if (_hwndChild)
        {
            LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
    
            if (!(lpwp->flags & SWP_NOSIZE))
            {
                SetWindowPos(_hwndChild, NULL,
                    0,0,
                    lpwp->cx, lpwp->cy,
                    SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE|
                    (lpwp->flags&(SWP_NOREDRAW|SWP_NOCOPYBITS)));
            }
    
        }
        return 0;

    case WM_ERASEBKGND:
        if ( _crBorder == CLR_DEFAULT )
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            HDC hdc = (HDC) wParam;
            RECT rcClient;
            GetClientRect( hwnd, &rcClient );
            COLORREF crSave = SetBkColor(hdc, _crBorder);
            
            ExtTextOut(hdc,0,0,ETO_OPAQUE,&rcClient,NULL,0,NULL);
            SetBkColor(hdc, crSave);
        }

    case WM_COMMAND:
    case WM_NOTIFY:
        if (_pweh)
        {
            LRESULT lres;
            if (SUCCEEDED(_pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, &lres)))
                return lres;
        }
        // fall through

    default:
        return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
    }
}


LPITEMIDLIST ChannelOC::_GetInitialPidl()
{
    LPITEMIDLIST pidl = NULL;

    // Figure out what directory this ChannelOC is looking at.
    // If we're looking at a specific channel category, use it,
    // otherwise use the top-level Channels folder.
    //
    if (EVAL(_pcli))
    {
        IOleContainer *pContainer;

        if (SUCCEEDED(_pcli->GetContainer(&pContainer)))
        {
            IHTMLDocument2 *pDoc;

            if (SUCCEEDED(pContainer->QueryInterface(IID_IHTMLDocument2, (LPVOID*)&pDoc)))
            {
                IHTMLLocation *pLocation;

                if (SUCCEEDED(pDoc->get_location(&pLocation)))
                {
                    BSTR bstrURL;

                    if (SUCCEEDED(pLocation->get_href(&bstrURL)) && bstrURL)
                    {
                        TCHAR szPath[MAX_PATH];
                        DWORD cchT = ARRAYSIZE(szPath);
                        if (SUCCEEDED(PathCreateFromUrl(bstrURL, szPath, &cchT, 0)))
                        {
                            PathRemoveFileSpec(szPath);

                            // if we're not under the channels folder, then
                            // ignore this location
                            //
                            if (SUCCEEDED(IECreateFromPath(szPath, &pidl)))
                            {
                                LPITEMIDLIST pidlChannels = Channel_GetFolderPidl();

                                if (!pidlChannels || !ILIsParent(pidlChannels, pidl, FALSE))
                                {
                                    ILFree(pidl);
                                    pidl = NULL;
                                }

                                ILFree(pidlChannels);
                            }
                            TraceMsg(TF_BAND, "ChannelOC::_OnInPlaceActivate [%s] (%x)", szPath, pidl);
                        }

                        SysFreeString(bstrURL);
                    }

                    pLocation->Release();
                }

                pDoc->Release();
            }

            pContainer->Release();
        }
    }

    return pidl;
}

void ChannelOC::_OnInPlaceActivate()
{
    SUPERCLASS::_OnInPlaceActivate();

    // we should never get called twice, but might as well be safe
    //
    if (EVAL(!_pBand))
    {
        LPITEMIDLIST pidl = _GetInitialPidl();

        // Now create the band and initialize it properly
        //
        _pBand = ChannelBand_Create(pidl);
        if (_pBand)
        {
            IDropTarget* pdt;

            Channels_SetBandInfoSFB(_pBand);
            _pBand->QueryInterface(IID_IWinEventHandler, (LPVOID*)&_pweh);
        
            IUnknown_SetSite(_pBand, SAFECAST(this, IOleObject*));

            // now that band is sited and init'ed, we can override defaults
            if (_crBkgndOC != CLR_DEFAULT) {
                BANDINFOSFB bi;

                TraceMsg(TF_WARNING, "coc.oipa: BGColor=%x _pBand=%x", _crBkgndOC, _pBand);
                bi.dwMask = ISFB_MASK_BKCOLOR;
                bi.crBkgnd = _crBkgndOC;
                IUnknown_SetBandInfoSFB(_pBand, &bi);
            }

            _dbi.dwMask = DBIM_MINSIZE|DBIM_MAXSIZE|DBIM_INTEGRAL|DBIM_ACTUAL|DBIM_TITLE|DBIM_MODEFLAGS|DBIM_BKCOLOR;
            _pBand->GetBandInfo(0, DBIF_VIEWMODE_VERTICAL, &_dbi);

            _pBand->GetWindow(&_hwndChild);

            SetWindowPos(_hwndChild, NULL, 0, 0,
                         _rcPos.right - _rcPos.left,
                         _rcPos.bottom - _rcPos.top,
                         SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);

            _pBand->ShowDW(TRUE);

            // Register the band as a drop target
            if (SUCCEEDED(_pBand->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt)))
            {
                THR(RegisterDragDrop(_hwnd, pdt));
                pdt->Release();
            }

            TraceMsg(TF_BAND, "ISFBandOC::_OnInPlaceActivate()d to cx=%d cy=%d", _size.cx, _size.cy);
        }

        ILFree(pidl);
    }
}

void ChannelOC::_OnInPlaceDeactivate()
{
    IDeskBand * pBand = _pBand;

    // set to NULL to avoid re-entrancy...
    _pBand = NULL;
    
    if (pBand)
    {
        _hwndChild = NULL;

        RevokeDragDrop(_hwnd);

        pBand->ShowDW(FALSE);
        IUnknown_SetSite(pBand, NULL);
        pBand->CloseDW(0);

        TraceMsg(TF_BAND, "ISFBandOC::_OnInPlaceDeactivate()d");
    }

    // we need to keep _pweh because we need the notifies during destruction to 
    // free everything properly
    // the _pBand = NULL above is sufficient to keep from reentrancy
    ATOMICRELEASE(_pweh);
    ATOMICRELEASE(pBand);

    SUPERCLASS::_OnInPlaceDeactivate();
}

HRESULT ChannelOC::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    HRESULT hres = SUPERCLASS::SetObjectRects(lprcPosRect, lprcClipRect);

    if (_hwndChild)
    {
        SetWindowPos(_hwndChild, NULL, 0,0,
            _rcPos.right - _rcPos.left,
            _rcPos.bottom - _rcPos.top, SWP_NOZORDER);
    }

    return hres;
}

STDAPI ChannelOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;

    ChannelOC* pBand = new ChannelOC(punkOuter, poi);
    if (pBand)
    {
        *ppunk = pBand->_GetInner();
        hres = S_OK;
    }

    return hres;
}

