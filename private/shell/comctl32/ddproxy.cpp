#include "ctlspriv.h"
#include "olestuff.h"

//------------------------------------------------------------------------------

STDAPI GetItemObject(CONTROLINFO *pci, UINT uMsg, const IID *piid, LPNMOBJECTNOTIFY pnon)
{
    pnon->piid = piid;
    pnon->pObject = NULL;
    pnon->hResult = E_NOINTERFACE;

    CCSendNotify(pci, uMsg, &pnon->hdr);

    ASSERT(SUCCEEDED(pnon->hResult) ? (pnon->pObject != NULL) : (pnon->pObject == NULL));
    
    return pnon->hResult;
}

//------------------------------------------------------------------------------

class CDragProxy : public IDropTarget
{

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *, DWORD, POINTL, DWORD *);
    STDMETHODIMP DragOver(DWORD, POINTL, DWORD *);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *, DWORD, POINTL, DWORD *);

    CDragProxy(HWND hwnd, PFNDRAGCB pfn);
    BOOL Register();
    void RevokeAndFreeCB();

private:
    ~CDragProxy();

    int          _cRef;             // object reference count
    HWND         _hwnd;             // window that owns us
    PFNDRAGCB    _pfnCallback;      // callback for that window
    IDataObject *_pdtobj;           // data object being dragged
    IDropTarget *_pdtgtItem;        // drop target of item under mouse
    int          _idItem;           // id of item under mouse
    DWORD        _dwFlags;
    int          _idDefault;        // id to use when outside a drag etc
    DWORD        _dwEffectItem;     // DROPEFFECT returned for item under mouse
    DWORD        _fKeysLast;        // key flags from last DragOver
    POINTL       _ptLast;           // location of last DragOver
    DWORD        _dwEffectLast;     // effect available from last DragOver
    HMODULE      _hmodOLE;          // OLE32 ref, also indicates we did a Register()

    void SetTargetItem(int id, DWORD dwFlags);
    void SetDropTarget(IDropTarget *pdt);
    void UpdateSelection(DWORD dwEffect);
    LRESULT CallCB(UINT code, WPARAM wp, LPARAM lp);
};

//------------------------------------------------------------------------------

STDAPI_(HDRAGPROXY) CreateDragProxy(HWND hwnd, PFNDRAGCB pfn, BOOL bRegister)
{
    CDragProxy *pdp = new CDragProxy(hwnd, pfn);

    //
    // register as needed
    //
    if (pdp && bRegister && !pdp->Register())
    {
        pdp->Release();
        pdp = NULL;
    }

    return (HDRAGPROXY)pdp;
}

STDAPI_(void) DestroyDragProxy(HDRAGPROXY hdp)
{
    if (hdp)
    {
        ((CDragProxy *)hdp)->RevokeAndFreeCB();
        ((CDragProxy *)hdp)->Release();
    }
}

STDAPI GetDragProxyTarget(HDRAGPROXY hdp, IDropTarget **ppdtgt)
{
    if (hdp)
    {
        *ppdtgt = SAFECAST((CDragProxy *)hdp, IDropTarget *);
        ((CDragProxy *)hdp)->AddRef();
        return NOERROR;
    }

    *ppdtgt = NULL;
    return E_FAIL;
}


//------------------------------------------------------------------------------

CDragProxy::CDragProxy(HWND hwnd, PFNDRAGCB pfn)
    :   _hwnd(hwnd), _pfnCallback(pfn),
        _cRef(1), 
        _hmodOLE(NULL),
        _pdtobj(NULL), 
        _pdtgtItem(NULL),
        _dwEffectItem(DROPEFFECT_NONE)
{
    _idDefault = _idItem = (int)CallCB(DPX_DRAGHIT, 0, 0);
}

CDragProxy::~CDragProxy()
{
    DragLeave();

}

HRESULT CDragProxy::QueryInterface(REFIID iid, void **ppv)
{
    if (IsEqualIID(iid, IID_IDropTarget) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = SAFECAST(this, IDropTarget *);
    }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    _cRef++;
    return NOERROR;
}

ULONG CDragProxy::AddRef()
{
    return ++_cRef;
}

ULONG CDragProxy::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDragProxy::DragEnter(IDataObject *pdo, DWORD fKeys, POINTL pt, DWORD *pdwEffect)
{
    //
    // some sanity
    //
    ASSERT(!_pdtgtItem);
    ASSERT(!_pdtobj);

    if (!pdo)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    //
    // make sure our callback will allow us to do d/d now
    //
    if (!CallCB(DPX_ENTER, 0, 0))
        return E_FAIL;

    //
    // save away the data object
    //
    pdo->AddRef();
    _pdtobj = pdo;

    //
    // and process this like a DragOver
    //
    DragOver(fKeys, pt, pdwEffect);

    //
    // always succeed DragEnter
    //
    return NOERROR;
}

HRESULT CDragProxy::DragLeave()
{
    //
    // release any drop target that we are holding
    //
    SetDropTarget(NULL);
    _idItem = _idDefault;

    //
    // if we had a data object then we were actually dragging
    //
    if (_pdtobj)
    {
        CallCB(DPX_LEAVE, 0, 0);

        IDataObject* p = _pdtobj;
        _pdtobj = NULL;
        p->Release();
    }

    //
    // all done
    //
    return NOERROR;
}

HRESULT CDragProxy::DragOver(DWORD fKeys, POINTL pt, DWORD *pdwEffect)
{
    DWORD dwFlags = 0;
    HRESULT hres;
    int id;
    ASSERT(_pdtobj);

    //
    // save the current drag state
    //
    _fKeysLast    = fKeys;
    _ptLast       = pt;
    _dwEffectLast = *pdwEffect;

    //
    // make sure we have the correct drop target for this location
    //
    id = (int)CallCB(DPX_DRAGHIT, (WPARAM)&dwFlags, (LPARAM)&pt);
    SetTargetItem(id, dwFlags);
    //
    // do we have a target to drop on?
    //
    if (_pdtgtItem)
    {
        //
        // forward the DragOver along to the item's drop target (if any)
        //
        hres = _pdtgtItem->DragOver(fKeys, pt, pdwEffect);
    }
    else
    {
        //
        // can't drop here
        //
        *pdwEffect = DROPEFFECT_NONE;
        hres = NOERROR;
    }

    //
    // and update our selection state accordingly
    //
    UpdateSelection(*pdwEffect);

    return hres;
}

HRESULT CDragProxy::Drop(IDataObject *pdo, DWORD fKeys, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;

    AddRef();

    //
    // do we have a target to drop on?
    //
    if (_pdtgtItem)
    {
        // From a comment in browseui, there's apparently a chance to put up UI
        // which could cause us to get re-entered.  Hard to believe, but see if
        // this fixes the fault:
        //
        IDropTarget * pdtCur = _pdtgtItem;
        _pdtgtItem = NULL;
        
        //
        // do the drop
        //
        hres = pdtCur->Drop(pdo, fKeys, pt, pdwEffect);

        //
        // we call our DragLeave below but we don't want the item's to be
        // called (since it already saw the Drop) so we release right away
        //
        pdtCur->Release();
    }
    else
    {
        //
        // can't drop here
        //
        *pdwEffect = DROPEFFECT_NONE;
        hres = NOERROR;
    }

    //
    // now clean up
    //
    DragLeave();

    Release();
    
    return hres;
}

void CDragProxy::SetTargetItem(int id, DWORD dwFlags)
{
    //
    // anything to do?
    //
    if (id == _idItem && dwFlags == _dwFlags)
        return;

    //
    // deselect the old item (if any)
    //
    // the GETOBJECT below could take a long time and we don't want a
    // lingering highlight on the object we are leaving
    //

    UpdateSelection(DROPEFFECT_NONE);

    //
    // get a drop target for the new item
    //
    _idItem = id;
    _dwFlags = dwFlags;

    NMOBJECTNOTIFY non;
    non.iItem = id;
    non.dwFlags = dwFlags;
    if (!_pdtobj || FAILED((HRESULT)CallCB(DPX_GETOBJECT, 0, (LPARAM)&non)))
        non.pObject = NULL;

        //
    // use this drop target (if any)
    //
    SetDropTarget((IDropTarget*)non.pObject);

    //
    // release our ref from the GETOBJECT above
    //
    if (non.pObject)
        ((IDropTarget*)non.pObject)->Release();
}

void CDragProxy::SetDropTarget(IDropTarget *pdt)
{
    //
    // NOTE: we intentionally skip the test for drop-target equality here
    // this allows controls owners to share a target among multiple items
    // while retaining the proper leave/enter sequence...
    //
    // BOGUS: we should actually compare here when the Internet Toolbar gets
    //  fixed (see comment in CDragProxy::SetTargetItem).  anybody who wants
    //  to share a target like this should just do the right hit-testing in
    //  their DragOver implementation
    //


    //
    // make sure nothing is selected
    //
    UpdateSelection(DROPEFFECT_NONE);

    //
    // leave/release the old item
    //
    if (_pdtgtItem)
    {
        _pdtgtItem->DragLeave();
        _pdtgtItem->Release();
    }

    //
    // store the new item
    //
    _pdtgtItem = pdt;

    //
    // addref/enter the new item
    //
    if (_pdtgtItem)
    {
        ASSERT(_pdtobj);    // must have a data object by now

        _pdtgtItem->AddRef();

        DWORD dwEffect = _dwEffectLast;
        if (FAILED(_pdtgtItem->DragEnter(_pdtobj, _fKeysLast, _ptLast, &dwEffect)))
            dwEffect = DROPEFFECT_NONE;

        //
        // update the selection
        //
        UpdateSelection(dwEffect);
    }
}

void CDragProxy::UpdateSelection(DWORD dwEffect)
{
    //
    // anything to do?
    //
    if (dwEffect == _dwEffectItem)
        return;

    //
    // update the flags and tell the callback they changed
    //
    _dwEffectItem = dwEffect;
    CallCB(DPX_SELECT, (WPARAM)_idItem, (LPARAM)dwEffect);
}

LRESULT CDragProxy::CallCB(UINT code, WPARAM wp, LPARAM lp)
{
    return _pfnCallback ? _pfnCallback(_hwnd, code, wp, lp) : (LRESULT)-1;
}

BOOL CDragProxy::Register()
{
    _hmodOLE = PrivLoadOleLibrary();
    if (_hmodOLE)
    {
        if (SUCCEEDED(PrivCoInitialize(_hmodOLE)))
        {
            if (SUCCEEDED(PrivRegisterDragDrop(_hmodOLE, _hwnd, this)))
                return TRUE;

            PrivCoUninitialize(_hmodOLE);
        }

        PrivFreeOleLibrary(_hmodOLE);
        _hmodOLE = NULL;
    }

    return FALSE;
}

void CDragProxy::RevokeAndFreeCB()
{
    if (_hmodOLE)
    {
        PrivRevokeDragDrop(_hmodOLE, _hwnd);
        PrivCoUninitialize(_hmodOLE);
        PrivFreeOleLibrary(_hmodOLE);
    }
    _pfnCallback = NULL;
}

