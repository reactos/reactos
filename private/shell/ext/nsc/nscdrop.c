#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#include "debug.h"
#include "common.h"

#include "autoscrl.h"

#include "nsc.h"

// BUGBUG: do nothing for now
#define DAD_DragLeave()
#define DAD_DragEnterEx(hwndLock, pt)
#define DAD_ShowDragImage(f)
#define DAD_DragMove(pt)

typedef struct {	// tdt
    IDropTarget		dtgt;
    UINT		cRef;

    NSC			*pns;

    RECT		_rcLockWindow;
    HTREEITEM		_htiCur;	// current tree item (dragging over)
    IDropTarget		*_pdtgtCur;	// current drop target
    IDataObject		*_pdtobjCur;	// current data object
    DWORD		_dwEffectCur;	// current drag effect
    DWORD		_dwEffectIn;	// *pdwEffect passed-in on last Move/Enter
    DWORD               _grfKeyState;   // cached key state
    POINT		_ptLast;	// last dragged over position
    DWORD               _dwLastTime;
    AUTO_SCROLL_DATA	asd;
} CTreeDropTarget;

STDMETHODIMP CTreeDropTarget_QueryInterface(IDropTarget *pdtgt, REFIID riid, void **ppvObj)
{
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);

    if (IsEqualIID(riid, &IID_IDropTarget) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = pdtgt;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTreeDropTarget_AddRef(IDropTarget *pdtgt)
{
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);

    this->cRef++;
    return this->cRef;
}

void CTreeDropTarget_ReleaseDataObject(CTreeDropTarget *this)
{
    if (this->_pdtobjCur)
	Release(this->_pdtobjCur);

    this->_pdtobjCur = NULL;
}

void CTreeDropTarget_ReleaseCurrentDropTarget(CTreeDropTarget *this)
{
    if (this->_pdtgtCur)
    {
	this->_pdtgtCur->lpVtbl->DragLeave(this->_pdtgtCur);
	Release(this->_pdtgtCur);
        this->_pdtgtCur = NULL;
        this->_htiCur = NULL;
    }
    else
    {
	Assert(this->_htiCur == NULL);
    }
}

STDMETHODIMP_(ULONG) CTreeDropTarget_Release(IDropTarget * pdtgt)
{
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);

    this->cRef--;
    if (this->cRef > 0)
        return this->cRef;

    AssertMsg(this->_pdtgtCur == NULL, "drag leave was not called properly");

    // if above is true we can remove this...
    CTreeDropTarget_ReleaseCurrentDropTarget(this);

    LocalFree(this);

    return 0;
}

STDMETHODIMP CTreeDropTarget_DragEnter(IDropTarget *pdtgt, IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);
    POINT pt;
    HWND hwndLock;

    DebugMsg(DM_TRACE, "sh - TR CTreeDropTarget::DragEnter called");
    CTreeDropTarget_ReleaseDataObject(this);
    this->_pdtobjCur = pdtobj;
    this->_grfKeyState = grfKeyState;
    AddRef(pdtobj);
    Assert(this->_pdtgtCur == NULL);
    Assert(this->_htiCur == NULL);

    hwndLock = this->pns->hwndTree;	    // clip to this
    GetWindowRect(hwndLock, &this->_rcLockWindow);
    pt.x = ptl.x-this->_rcLockWindow.left;
    pt.y = ptl.y-this->_rcLockWindow.top;
    DAD_DragEnterEx(hwndLock, pt);

    this->_ptLast.x = this->_ptLast.y = 0x7ffffff;	// set bogus position

    return S_OK;
}

STDMETHODIMP CTreeDropTarget_DragOver(IDropTarget *pdtgt, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    HRESULT hres = S_OK;
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);
    TV_HITTESTINFO tvht;
    HTREEITEM htiNew;
    POINT pt = { ptl.x, ptl.y };
    BOOL fSameImage = FALSE;
    DWORD dwEffectScroll = 0;

    ScreenToClient(this->pns->hwndTree, &pt);

    if (DAD_AutoScroll(this->pns->hwndTree, &this->asd, &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;

    tvht.pt = pt;

    htiNew = TreeView_HitTest(this->pns->hwndTree, &tvht);

    // don't allow droping on the item being dragged
    if (htiNew == this->pns->htiDragging)
        htiNew = NULL;

    if (this->_htiCur != htiNew)
    {
        // change in target

        this->_dwLastTime = GetTickCount();     // keep track for auto-expanding the tree

	CTreeDropTarget_ReleaseCurrentDropTarget(this);

	this->_dwEffectCur = 0;	// assume error

	DAD_ShowDragImage(FALSE);

	TreeView_SelectDropTarget(this->pns->hwndTree, htiNew);

	DAD_ShowDragImage(TRUE);

	if (htiNew)
	{
            // get the drop target for the item we hit

	    LPITEMIDLIST pidl = _CacheParentShellFolder(this->pns, htiNew, NULL);
	    if (pidl)
	    {
	        IShellFolder *psf = this->pns->psfCache;

		AddRef(psf);

	        if (pidl->mkid.cb == 0)
		{
		    hres = psf->lpVtbl->CreateViewObject(psf, this->pns->hwnd, &IID_IDropTarget, &this->_pdtgtCur);
		}
		else
		{
		    UINT dwAttr = SFGAO_DROPTARGET;

		    hres = psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &dwAttr);

		    if (SUCCEEDED(hres) && (dwAttr & SFGAO_DROPTARGET))
		    {
			hres = psf->lpVtbl->GetUIObjectOf(psf, this->pns->hwnd, 1, &pidl, &IID_IDropTarget, NULL, &this->_pdtgtCur);
		    }
		    else
		    {
			hres = E_INVALIDARG;
		    }
		}

		Release(psf);
	    }

	    if (SUCCEEDED(hres))
	    {
		this->_htiCur = htiNew;

		this->_dwEffectCur = *pdwEffect;	// pdwEffect is In/Out
		hres = this->_pdtgtCur->lpVtbl->DragEnter(this->_pdtgtCur, this->_pdtobjCur, grfKeyState, ptl, &this->_dwEffectCur);
	    }
	} 
    }
    else
    {
        // No target change

        // auto expand the tree
        if (this->_htiCur)
        {
            DWORD dwNow = GetTickCount();

            if ((dwNow - this->_dwLastTime) >= 1000)
            {
                this->_dwLastTime = dwNow;
                DAD_ShowDragImage(FALSE);
                this->pns->fAutoExpanding = TRUE;
                TreeView_Expand(this->pns->hwndTree, this->_htiCur, TVE_EXPAND);
                this->pns->fAutoExpanding = FALSE;
                DAD_ShowDragImage(TRUE);
            }
        }

	// maybe the key state changed

        if ((this->_grfKeyState != grfKeyState) && this->_pdtgtCur) 
	{
            this->_dwEffectCur = *pdwEffect;
            hres = this->_pdtgtCur->lpVtbl->DragOver(this->_pdtgtCur, grfKeyState, ptl, &this->_dwEffectCur);
        } 
	else 
	{
	    fSameImage = TRUE;
            hres = S_OK;
        }
    }

    DebugMsg(DM_TRACE, "sh TR - CTreeDropTarget_DragOver (In=%x, Out=%x)", *pdwEffect, this->_dwEffectCur);

    this->_grfKeyState = grfKeyState;
    *pdwEffect = this->_dwEffectCur | dwEffectScroll;

    // We need pass pt relative to the locked window (not the client).
    pt.x = ptl.x-this->_rcLockWindow.left;
    pt.y = ptl.y-this->_rcLockWindow.top;

    if (!(fSameImage && this->_ptLast.x == pt.x && this->_ptLast.y == pt.y))
    {
	DAD_DragMove(pt);
	this->_ptLast.x = pt.x;
	this->_ptLast.y = pt.y;
    }

    return hres;
}


STDMETHODIMP CTreeDropTarget_DragLeave(IDropTarget *pdtgt)
{
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);

    DebugMsg(DM_TRACE, "sh - TR CTreeDropTarget::DragLeave called");
    CTreeDropTarget_ReleaseCurrentDropTarget(this);
    CTreeDropTarget_ReleaseDataObject(this);

    DAD_DragLeave();

    TreeView_SelectDropTarget(this->pns->hwndTree, NULL);

    return S_OK;
}

STDMETHODIMP CTreeDropTarget_Drop(IDropTarget *pdtgt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;
    CTreeDropTarget * this = IToClass(CTreeDropTarget, dtgt, pdtgt);

    if (this->_pdtgtCur)
    {
	hres = this->_pdtgtCur->lpVtbl->Drop(this->_pdtgtCur, pdtobj, grfKeyState, pt, pdwEffect);
    }
    else
    {
	DebugMsg(DM_TRACE, "sh TR - CTreeDropTarget::Drop - this->_pdtgtCur==NULL");
	*pdwEffect = 0;
	hres = S_OK;
    }

    CTreeDropTarget_DragLeave(pdtgt);

    return hres;
}

const IDropTargetVtbl c_CTreeDropTargetVtbl = {
    CTreeDropTarget_QueryInterface, CTreeDropTarget_AddRef, CTreeDropTarget_Release,
    CTreeDropTarget_DragEnter,
    CTreeDropTarget_DragOver,
    CTreeDropTarget_DragLeave,
    CTreeDropTarget_Drop
};


HRESULT CTreeDropTarget_Create(NSC *pns, IDropTarget **ppdtgt)
{
    CTreeDropTarget * this = LocalAlloc(LPTR, sizeof(CTreeDropTarget));
    if (this)
    {
	this->dtgt.lpVtbl = (IDropTargetVtbl *)&c_CTreeDropTargetVtbl;
	this->cRef = 1;
	this->pns = pns;

	*ppdtgt = &this->dtgt;
	return S_OK;
    }

    *ppdtgt = NULL;
    return E_OUTOFMEMORY;
}

void CTreeDropTarget_Register(NSC *pns)
{
    IDropTarget *pdtgt;

    if (SUCCEEDED(CTreeDropTarget_Create(pns, &pdtgt)))
    {
	RegisterDragDrop(pns->hwndTree, pdtgt);
        Release(pdtgt);
    }
}

void CTreeDropTarget_Revoke(NSC *pns)
{
    RevokeDragDrop(pns->hwndTree);
}

