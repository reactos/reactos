//////////////////////////////////////////////////
//
// CBandISFOC
//
// This is a thin OC that wraps the above band.
// For quickness, use CShellEmbedding, even though
// this is heavier than we need.
//
// TODO:
//  - support IPersistPropertyBag so we can read the
//    pidl to show out of an html page
//  - listen to ISFBand for resizes and request them
//    from our container
//
// NOTE: looks like this is being dropped. I ripped
//       the code out of bandisf.cpp and put it in
//       this dead directory. [mikesh]
//

#undef  SUPERCLASS
#define SUPERCLASS CShellEmbedding

class CBandISFOC : public SUPERCLASS
{
public:
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect);

protected:
    CBandISFOC(IUnknown* punkOuter, LPCOBJECTINFO poi);
    ~CBandISFOC();
    friend HRESULT CBandISFOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual LRESULT v_WndProc(HWND, UINT, WPARAM, LPARAM);

    virtual void _OnInPlaceActivate(void);      // called when we actually go in-place-active
    virtual void _OnInPlaceDeactivate(void);    // called when we actually deactivate

    LPITEMIDLIST      _pidl;

    IShellToolband*   _pst;
    IDockingWindow*   _pdw;
    IWinEventHandler* _pweh;

    // for GetBandInfo, not currently used
    POINT _ptSizes[STBE_MAX];
    WCHAR _szTitle[40];
    DWORD _dwSizeMode;

};

CBandISFOC::CBandISFOC(IUnknown* punkOuter, LPCOBJECTINFO poi) : 
    CShellEmbedding(punkOuter, poi, NULL)
{
}

CBandISFOC::~CBandISFOC() 
{
    if (_pidl)
        ILFree(_pidl);
}

LRESULT CBandISFOC::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    switch(uMsg)
    {
    case WM_CONTEXTMENU:
        if (_pst)
        {
            IContextMenu * pcm;
            if (SUCCEEDED(_pst->QueryInterface(IID_IContextMenu, (LPVOID*)&pcm)))
            {
                POINT pt;
        
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);

                lres = OnContextMenu(hwnd, pcm, &pt);

                pcm->Release();
            }
        }
        return lres;

    case WM_ERASEBKGND:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_COMMAND:
    case WM_NOTIFY:
        if (_pweh)
        {
            LRESULT lres;
            if (SUCCEEDED(_pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, (LPDWORD)&lres)))
                return lres;
        }
        // fall through

    default:
        return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
    }
}

HRESULT CBandISFOC::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IPersistStorage))
    {
        // shembed.cpp supports this, we really want to
        // rip this out there but this is quicker.
        *ppvObj = NULL;
        return(E_NOINTERFACE);
    }
    else
    {
        return SUPERCLASS::v_InternalQueryInterface(riid, ppvObj);
    }

    AddRef();
    return S_OK;
}

void CBandISFOC::_OnInPlaceActivate()
{
    SUPERCLASS::_OnInPlaceActivate();

    // we should never get called twice, but might as well be safe
    //
    if (EVAL(!_pst))
    {
        if (!_pidl)
        {
            SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &_pidl);
        }
    
        if (EVAL(_pidl))
        {
            _pst = CISFBand_Create(NULL, _pidl, FALSE);
            if (_pst)
            {
                if (SUCCEEDED(_pst->QueryInterface(IID_IDockingWindow, (LPVOID*)&_pdw)))
                {
                    IOleWindow* pow;
            
                    _pdw->QueryInterface(IID_IWinEventHandler, (LPVOID*)&_pweh);
            
                    _pdw->SetToolbarSite(SAFECAST(this, IOleObject*));
                    _pst->GetBandInfo(STBBIF_VIEWMODE_VERTICAL, _ptSizes, _szTitle, ARRAYSIZE(_szTitle), &_dwSizeMode);

                    if (SUCCEEDED(_pdw->QueryInterface(IID_IOleWindow, (LPVOID*)&pow)))
                    {
                        pow->GetWindow(&_hwndChild);
                        pow->Release();

                        SetWindowPos(_hwndChild, NULL, 0, 0, _size.cx, _size.cy, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
                        TraceMsg(TF_BAND, "ISFBandOC::_OnInPlaceActivate()d");
                    }

                    _pdw->ShowDW(TRUE);
                }
            }
        }
    }
}

void CBandISFOC::_OnInPlaceDeactivate()
{
    if (_pdw)
    {
        _hwndChild = NULL;

        _pdw->ShowDW(FALSE);
        _pdw->SetToolbarSite(NULL);
        _pdw->CloseDW(0);

        _pdw->Release();
        _pdw = NULL;

        TraceMsg(TF_BAND, "ISFBandOC::_OnInPlaceDeactivate()d");
    }

    ATOMICRELEASE(_pst);
    ATOMICRELEASE(_pweh);

    SUPERCLASS::_OnInPlaceDeactivate();
}

HRESULT CBandISFOC::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    HRESULT hres = SUPERCLASS::SetObjectRects(lprcPosRect, lprcClipRect);

    if (_hwndChild)
    {
        SetWindowPos(_hwndChild, NULL, 0, 0, 
            _rcPos.right - _rcPos.left, _rcPos.bottom - _rcPos.top, SWP_NOZORDER);
    }

    return hres;
}

HRESULT CBandISFOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;

    CBandISFOC* pBand = new CBandISFOC(punkOuter, poi);
    if (pBand)
    {
        *ppunk = pBand->_GetInner();
        hres = S_OK;
    }

    return hres;
}

