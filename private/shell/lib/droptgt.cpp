#include "priv.h"
#include "droptgt.h"

#define TF_DRAGDROP TF_BAND


#define MAX_DROPTARGETS 3

class CDropTargetWrap : public IDropTarget
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    CDropTargetWrap(IDropTarget** ppdtg, HWND hwnd);
protected:
    ~CDropTargetWrap();

private:
    int             _cRef;

    int             _count;
    IDropTarget*    _rgpdt[MAX_DROPTARGETS];
    DWORD           _rgdwEffect[MAX_DROPTARGETS];
    HWND            _hwnd;
};

CDropTargetWrap::CDropTargetWrap(IDropTarget** ppdt, HWND hwnd)
    : _hwnd(hwnd)
{
    _cRef = 1;

    for (int i = 0; i < MAX_DROPTARGETS; i++, ppdt++) {
        if (*ppdt) {
            _rgpdt[_count] = *ppdt;
            _rgpdt[_count]->AddRef();
            _count++;
        }
    }
}

CDropTargetWrap::~CDropTargetWrap()
{
    for (int i = 0 ; i < _count ; i++)
    {
        _rgpdt[i]->Release();
    }
}

HRESULT CDropTargetWrap::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDropTargetWrap, IDropTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDropTargetWrap::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CDropTargetWrap::Release(void)
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragEnter method

         The *pdwEffect that is returned is the first valid value
         of all the drop targets' returned effects.

*/
HRESULT CDropTargetWrap::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DWORD dwEffectOut = DROPEFFECT_NONE;

    for (int i = 0 ; i < _count ; i++)
    {
        _rgdwEffect[i] = *pdwEffect;

        if (SUCCEEDED(_rgpdt[i]->DragEnter(pdtobj, grfKeyState, ptl, &_rgdwEffect[i])))
        {
            if (dwEffectOut == DROPEFFECT_NONE)
            {
                dwEffectOut = _rgdwEffect[i];
            }
        }
        else
        {
            _rgdwEffect[i] = DROPEFFECT_NONE;
        }
    }
    *pdwEffect = dwEffectOut;
    return(S_OK);
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragOver method

*/
HRESULT CDropTargetWrap::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DWORD dwEffectOut = DROPEFFECT_NONE;
        
    for (int i = 0 ; i < _count ; i++)
    {
        _rgdwEffect[i] = *pdwEffect;

        if (SUCCEEDED(_rgpdt[i]->DragOver(grfKeyState, ptl, &_rgdwEffect[i])))
        {
            if (dwEffectOut == DROPEFFECT_NONE)
                dwEffectOut = _rgdwEffect[i];
        }
        else
        {
            _rgdwEffect[i] = DROPEFFECT_NONE;
        }
    }

    *pdwEffect = dwEffectOut;
    return(S_OK);
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragLeave method

*/
HRESULT CDropTargetWrap::DragLeave(void)
{
    for (int i = 0 ; i < _count ; i++)
    {
        _rgpdt[i]->DragLeave();
    }

    return(S_OK);
}


/*----------------------------------------------------------
Purpose: IDropTarget::Drop method

*/
HRESULT CDropTargetWrap::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    DWORD dwEffectOut = DROPEFFECT_NONE;
    int i;
    BOOL fDropTried = FALSE;

    for (i = 0 ; (DROPEFFECT_NONE == dwEffectOut) && i < _count ; i++)
    {
        if ((_rgdwEffect[i] && *pdwEffect) && !fDropTried)
        {
            dwEffectOut = *pdwEffect;
            _rgpdt[i]->Drop(pdtobj, grfKeyState, pt, &dwEffectOut);
            fDropTried = TRUE;
        }
        else
        {
            _rgpdt[i]->DragLeave();
        }
    }

    *pdwEffect = dwEffectOut;
    return(S_OK);
}


//=============================================================================
// CDelegateDropTarget
//
// This class implements IDropTarget given an IDelegateDropTargetCB interface.
// It handles all hit testing, caching, and scrolling for you.
//
//=============================================================================
#undef  CDropTargetWrap

CDelegateDropTarget::CDelegateDropTarget()
{
    TraceMsg(TF_SHDLIFE, "ctor CDelegateDropTarget %x", this);

}

CDelegateDropTarget::~CDelegateDropTarget()
{
    TraceMsg(TF_SHDLIFE, "dtor CDelegateDropTarget %x", this);

    ASSERT(!_pDataObj);
    ATOMICRELEASE(_pDataObj);
    ASSERT(!_pdtCur);
    ATOMICRELEASE(_pdtCur);
}

HRESULT CDelegateDropTarget::Init()
{
    HRESULT hres = GetWindowsDDT(&_hwndLock, &_hwndScroll);
    // We lock _hwndLock and do scrolling against _hwndScroll.
    // These can be different hwnds, but certain restrictions apply:
    if (_hwndLock != _hwndScroll)
    {
        BOOL fValid = IsChild(_hwndLock, _hwndScroll);
        if (!fValid)
        {
            TraceMsg(TF_DRAGDROP, "ctor CDelegateDropTarget: invalid windows %x and %x!", _hwndLock, _hwndScroll);
            _hwndLock = _hwndScroll = NULL;
        }
    }
    return hres;
}

void CDelegateDropTarget::_ReleaseCurrentDropTarget()
{
    if (_pdtCur)
    {
        _pdtCur->DragLeave();
        ATOMICRELEASE(_pdtCur);
    }
}

/*----------------------------------------------------------
Purpose: IDropTarget::DragEnter method

*/
HRESULT CDelegateDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    // We can be re-entered due to ui on thread
    if (_pDataObj != NULL)       
    {
        TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::DragEnter called a second time!");
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
    TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::DragEnter with *pdwEffect=%x", *pdwEffect);

    ASSERT(!_pDataObj);
    _pDataObj = pdtobj;
    _pDataObj->AddRef();

    // cache state
    //
    // wait until first DragOver to get valid info
    //
    _fPrime = FALSE;
    _dwEffectOut = DROPEFFECT_NONE;

    // set up auto-scroll info
    //
    ASSERT(pdtobj);
    _DragEnter(_hwndLock, ptl, pdtobj);

    DAD_InitScrollData(&_asd);

    _ptLast.x = _ptLast.y = 0x7fffffff; // put bogus value to force redraw

    HitTestDDT(HTDDT_ENTER, NULL, NULL, NULL);

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragOver method

*/
HRESULT CDelegateDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    HRESULT hres = S_OK;
    DWORD itemNew;
    POINT pt;
    DWORD dwEffectScroll = 0;
    DWORD dwEffectOut = 0;
    BOOL fSameImage = FALSE;
    DWORD   dwCustDropEffect = 0;

    if (_pDataObj == NULL)
    {
        ASSERT(0);      // DragEnter should be called before.
        return E_FAIL;
    }

    // convert to window coords
    pt.x = ptl.x;
    pt.y = ptl.y;
    ScreenToClient(_hwndScroll, &pt);

    if (DAD_AutoScroll(_hwndScroll, &_asd, &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;

    //
    //  If we are dragging over on a different item, get its IDropTarget
    // interface or adjust itemNew to -1.
    //
    if (SUCCEEDED(HitTestDDT(HTDDT_OVER, &pt, &itemNew, &dwCustDropEffect)) &&
        (itemNew != _itemOver || !_fPrime))
    {
        _fPrime = TRUE;

        _ReleaseCurrentDropTarget();

        _itemOver = itemNew;
        GetObjectDDT(_itemOver, IID_IDropTarget, (LPVOID*)&_pdtCur);

        if (_pdtCur)
        {
            // There's an IDropTarget for this hit, use it
            dwEffectOut = *pdwEffect;

            hres = _pdtCur->DragEnter(_pDataObj, grfKeyState, ptl, &dwEffectOut);
            if (FAILED(hres))
                dwEffectOut = DROPEFFECT_NONE;
        }
        else
        {
            // No IDropTarget, no effect
            dwEffectOut = DROPEFFECT_NONE;
        }
    }
    else
    {
        //
        // No change in the selection. We assume that *pdwEffect stays
        // the same during the same drag-loop as long as the key state doesn't change.
        //
        if ((_grfKeyState != grfKeyState) && _pdtCur)
        {
            dwEffectOut = *pdwEffect;

            hres = _pdtCur->DragOver(grfKeyState, ptl, &dwEffectOut);

            TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::DragOver DragOver()d id:%d dwEffect:%4x hres:%d", _itemOver, dwEffectOut, hres);
        }
        else
        {
            // Same item and same key state. Use the previous dwEffectOut.
            dwEffectOut = _dwEffectOut;
            fSameImage = TRUE;
        }
    }

    _grfKeyState = grfKeyState;    // store these for the next Drop
    _dwEffectOut = dwEffectOut;    // and DragOver

    // Is the Custdrop effect valid ?
    if (dwCustDropEffect != DROPEFFECT_NONE)    
    {
        //Yes then set the effect to Custdrop effect along with scroll effect
        *pdwEffect = dwCustDropEffect | dwEffectScroll;
    }
    else 
    {
        //No , set the effect to dwEffectOut along with scroll effect
        *pdwEffect = dwEffectOut | dwEffectScroll;
    }
        TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::DragOver (*pdwEffect=%x)", *pdwEffect);


    if (!(fSameImage && pt.x==_ptLast.x && pt.y==_ptLast.y))
    {
        _DragMove(_hwndLock, ptl);
        _ptLast.x = ptl.x;
        _ptLast.y = ptl.y;
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragLeave method

*/
HRESULT CDelegateDropTarget::DragLeave()
{
    HitTestDDT(HTDDT_LEAVE, NULL, NULL, NULL);
    _ReleaseCurrentDropTarget();

    TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::DragLeave");
    ATOMICRELEASE(_pDataObj);

    DAD_DragLeave();

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IDropTarget::Drop method

*/
HRESULT CDelegateDropTarget::Drop(IDataObject *pdtobj,
                             DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hres = S_OK;
    BOOL bDropHandled = FALSE;

    TraceMsg(TF_DRAGDROP, "CDelegateDropTarget::Drop (*pdwEffect=%x)", *pdwEffect);

    //
    // According to AlexGo (OLE), this is by-design. We should make it sure
    // that we use pdtobj instead of pdtobj.
    //
    //ASSERT(pdtobj == _pDataObj);
    pdtobj->AddRef();
    _pDataObj->Release();
    _pDataObj = pdtobj;

    //
    // Note that we don't use the drop position intentionally,
    // so that it matches to the last destination feedback.
    //
    if (_pdtCur)
    {
        // use this local because if _pdtCur::Drop does a UnlockWindow
        // then hits an error and needs to put up a dialog,
        // we could get re-entered
        IDropTarget *pdtCur = _pdtCur;
        _pdtCur = NULL;

        // HACK ALERT!!!!
        //
        //  If we don't call LVUtil_DragEnd here, we'll be able to leave
        // dragged icons visible when the menu is displayed. However, because
        // we are calling IDropTarget::Drop() which may create some modeless
        // dialog box or something, we can not ensure the locked state of
        // the list view -- LockWindowUpdate() can lock only one window at
        // a time. Therefore, we skip this call only if the _pdtCur
        // is a subclass of CIDLDropTarget, assuming its Drop calls
        // CDefView::DragEnd (or CIDLDropTarget_DragDropMenu) appropriately.
        //
#if 0 // later
        if (!IsIDLDropTarget(pdtCur))
#endif
        {
            //
            // This will hide the dragged image.
            //
            DAD_DragLeave();

            //
            //  We need to reset the drag image list so that the user
            // can start another drag&drop while we are in this
            // Drop() member function call.
            //
            // NOTE: we don't have to worry about the DAD_DragLeave
            // (called from during the DragLeave call at the end of
            // this function) cancelling the potential above-mentioned
            // drag&drop loop. If such a beast is going on, it should
            // complete before pdtCur->Drop returns.
            //
            DAD_SetDragImage(NULL, NULL);
        }

        if (S_FALSE != OnDropDDT(pdtCur, _pDataObj, &grfKeyState, pt, pdwEffect))
            pdtCur->Drop(_pDataObj, grfKeyState, pt, pdwEffect);
        else
            pdtCur->DragLeave(); // should be okay even if OnDrop did this already

        pdtCur->Release();
    }
    else
    {
        //
        // We come here if Drop is called without DragMove (with DragEnter).
        //
        *pdwEffect = DROPEFFECT_NONE;
    }

    //
    // Clean up everything (OLE won't call DragLeave after Drop).
    //
    DragLeave();

    return hres;
}

// ******************************************************************
// dummy drop target to only call DAD_DragEnterEx() on DragEnter();
// ******************************************************************

HRESULT CDropDummy::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDropDummy, IDropTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDropDummy::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CDropDummy::Release(void)
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragEnter method

         simply call DAD_DragEnterEx2() to get custom drag cursor 
         drawing.

*/
HRESULT CDropDummy::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    ASSERT(pdtobj);
    _DragEnter(_hwndLock, ptl, pdtobj);
    *pdwEffect = DROPEFFECT_NONE;
    return(S_OK);
}

HRESULT CDropDummy::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _DragMove(_hwndLock, ptl);
    *pdwEffect = DROPEFFECT_NONE;
    return  S_OK;
}

