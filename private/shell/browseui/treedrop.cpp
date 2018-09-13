#include "priv.h"
#include "explband.h"
#include <shellp.h>

#define DROPEFFECT_ALL (DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK)
#define DROPEFFECT_KNOWN DROPEFFECT_SCROLL      // used to keep track of drag responses

//=================================================================
// Implementation of TreeDropTarget
//=================================================================
HRESULT TreeDropTarget::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)
    || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppvObj = SAFECAST(this, IDropTarget*);
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TreeDropTarget::AddRef()
{
    CExplorerBand* peb = IToClass(CExplorerBand, _tdt, this);

    return peb->AddRef();
}

void TreeDropTarget::_ReleaseDataObject()
{
    if (_pdtobjCur) {
        _pdtobjCur->Release();
    }
    _pdtobjCur = NULL;
}

void TreeDropTarget::_ReleaseCurrentDropTarget()
{
    if (_pdtgtCur)
    {
        _pdtgtCur->DragLeave();
        _pdtgtCur->Release();
        _pdtgtCur = NULL;
        _htiCur = NULL;
    }
}

STDMETHODIMP_(ULONG) TreeDropTarget::Release()
{
    CExplorerBand* peb = IToClass(CExplorerBand, _tdt, this);

    return peb->Release();
}

TreeDropTarget::~TreeDropTarget()
{
    _ReleaseCurrentDropTarget();
}

STDMETHODIMP TreeDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    CExplorerBand* peb = IToClass(CExplorerBand, _tdt, this);
    
    TraceMsg(DM_TRACE, "sh - TR CTreeDropTarget::DragEnter called");
    _ReleaseDataObject();
    _pdtobjCur = pdtobj;
    _grfKeyState = grfKeyState;
    pdtobj->AddRef();
    ASSERT(_pdtgtCur == NULL);
    ASSERT(_htiCur == NULL);

    // _hwndDD specifies the clipping rectangle for dragged image
    if (FAILED(SHGetTopBrowserWindow(peb->_punkSite, &_hwndDD)))
        _hwndDD = GetParent(peb->_GetTreeHWND());

    GetWindowRect(_hwndDD, &_rcLockWindow);

    _DragEnter(_hwndDD, ptl, pdtobj);

    _ptLast.x = _ptLast.y = 0x7ffffff;      // set bogus position

    return NOERROR;
}

STDMETHODIMP TreeDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    CExplorerBand* peb = IToClass(CExplorerBand, _tdt, this);
    HRESULT hres = NOERROR;
    HTREEITEM htiNew;
    POINT pt = { ptl.x, ptl.y };
    BOOL fSameImage = FALSE;
    DWORD dwEffectScroll = 0;

    ScreenToClient(peb->_GetTreeHWND(), &pt);

    if (DAD_AutoScroll(peb->_GetTreeHWND(), &_asd, &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;

    htiNew = peb->_TreeHitTest(pt, NULL);

    if (_htiCur != htiNew)
    {
        _dwLastTime = GetTickCount();     // keep track for auto-expanding the tree

        // Release previous drop target, if any.
        _ReleaseCurrentDropTarget();

        // Indicate the sink object
        _htiCur = htiNew;

        // Assume no drop target.
        ASSERT(_pdtgtCur == NULL);
        _dwEffectCur = 0; // assume error

        DAD_ShowDragImage(FALSE);
        TreeView_SelectDropTarget(peb->_GetTreeHWND(), htiNew);
        DAD_ShowDragImage(TRUE);

        if (htiNew)
        {
            //
            //  We are dragging over a treeitem which is different from
            // previoud one.
            //
            // Algorith:
            //  1. Bind to the parent node
            //  2. Get the (relative) pidl to the treeitem.
            //  3. Get its attributes to see it is a possible drop target
            //  4. If it is, get its IDropTarget and call DragEnter member.
            //
            LPOneTreeNode lpnNew = peb->_TreeGetFCTreeData(htiNew);
            LPSHELLFOLDER psf;

            if (lpnNew != s_lpnRoot)
            {
                LPITEMIDLIST pidl;

                hres = OTBindToParentFolder(lpnNew, &psf, &pidl);
                if (SUCCEEDED(hres))
                {
                    DWORD dwAttr = SFGAO_DROPTARGET;
                    hres = psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttr);
                    if (SUCCEEDED(hres) && (dwAttr & SFGAO_DROPTARGET))
                    {
                        hres = psf->GetUIObjectOf(peb->_hwnd, 1, (LPCITEMIDLIST*)&pidl,
                                                  IID_IDropTarget, NULL, (LPVOID*)&_pdtgtCur);
                    }
                    else
                    {
                        hres = E_INVALIDARG;

                        TraceMsg(DM_TRACE, "sh TR - CTree::DragOver (no drop target)");
                        ASSERT(_dwEffectCur == 0);
                    }

                    ILFree(pidl);
                    psf->Release();
                }
            }
            else 
            {
                // This is the desktop item; try getting a view object

                hres = OTBindToFolderEx(lpnNew, &psf);
                if (SUCCEEDED(hres))
                {
                    hres = psf->CreateViewObject(peb->_hwnd,
                                                 IID_IDropTarget, (LPVOID*)&_pdtgtCur);
                    
                    psf->Release();
                }
            }

            if (SUCCEEDED(hres))
            {
                _dwEffectCur = *pdwEffect;        // pdwEffect is In/Out
                hres = _pdtgtCur->DragEnter
                    (_pdtobjCur, grfKeyState, ptl, &_dwEffectCur);
            }
        } 
        else 
        {
            _dwEffectCur = 0; // No target
        }
    }
    else
    {
        // No target change

        if (_htiCur)
        {
            DWORD dwNow = GetTickCount();

            if ((dwNow - _dwLastTime) >= (GetDoubleClickTime()*4))
            {
                _dwLastTime = dwNow;
                DAD_ShowDragImage(FALSE);
                peb->_fNoInteractive = TRUE;
                TreeView_Expand(peb->_GetTreeHWND(), _htiCur, TVE_EXPAND);
                peb->_fNoInteractive = FALSE;
                DAD_ShowDragImage(TRUE);
            }
        }

        if ((_grfKeyState != grfKeyState) &&  _pdtgtCur)
        {
            _dwEffectCur = *pdwEffect;
            hres = _pdtgtCur->DragOver(grfKeyState, ptl, &_dwEffectCur);
        }
        else 
        {
            fSameImage = TRUE;
            hres = NOERROR;
        }
    }

    TraceMsg(DM_TRACE, "sh TR - TreeDropTarget::_DragOver (In=%x, Out=%x)", *pdwEffect, _dwEffectCur);
    _grfKeyState = grfKeyState;
    *pdwEffect = _dwEffectCur | dwEffectScroll;

    if (!(fSameImage && _ptLast.x==pt.x && _ptLast.y==pt.y))
    {
        _DragMove(_hwndDD, ptl);
        _ptLast.x = ptl.x;
        _ptLast.y = ptl.y;
    }

    return hres;
}

STDMETHODIMP TreeDropTarget::DragLeave()
{
    CExplorerBand* peb = IToClass(CExplorerBand, _tdt, this);

    TraceMsg(DM_TRACE, "sh - TR CTreeDropTarget::DragLeave called");
    TreeDropTarget::_ReleaseCurrentDropTarget();
    TreeDropTarget::_ReleaseDataObject();

    DAD_DragLeave();

    TreeView_SelectDropTarget(peb->_GetTreeHWND(), NULL);

    return NOERROR;
}

STDMETHODIMP TreeDropTarget::Drop(IDataObject *pdtobj,
                             DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hres;

    ASSERT(pdtobj==_pdtobjCur);

    //
    //  Drop it! Note that we don't use the drop position intentionally,
    // so that it matches to the last destination feedback.
    //
    if (_pdtgtCur)
    {
        hres = _pdtgtCur->Drop(pdtobj, grfKeyState, pt, pdwEffect);
    }
    else
    {
        TraceMsg(DM_TRACE, "sh TR - CTreeDropTarget::Drop - _pdtgtCur==NULL");
        *pdwEffect = 0;
        hres = NOERROR;
    }

    //
    // Clean it up.
    //
    DragLeave();
    return hres;
}

