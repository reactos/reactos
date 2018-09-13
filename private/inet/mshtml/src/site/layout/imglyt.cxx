//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       imglyt.cxx
//
//  Contents:   Implementation of CImageLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_MMPLAY_HXX_
#define X_MMPLAY_HXX_
#include "mmplay.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_DRAGDROP_HXX_
#define X_DRAGDROP_HXX_
#include "dragdrop.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif


MtDefine(CImgElementLayout, Layout, "CImgElementLayout")
MtDefine(CInputImageLayout, Layout, "CInputImageLayout")

IMPLEMENT_LAYOUT_FNS(CImgElement, CImgElementLayout)

//extern void SetCachedImageSize(LPCTSTR pchURL, SIZE size);
extern BOOL GetCachedImageSize(LPCTSTR pchURL, SIZE *psize);

extern HRESULT
CreateImgDataObject(CDoc * pDoc, CImgCtx * pImgCtx, CBitsCtx * pBitsCtx,
                    CElement * pElement, CGenDataObject ** ppImgDO);


class CImgDragDropSrcInfo : public CDragDropSrcInfo
{
public:
    CImgDragDropSrcInfo() : CDragDropSrcInfo()
    {
        _srcType = DRAGDROPSRCTYPE_IMAGE;
    }

    CGenDataObject *    _pImgDO;    // Data object for the image being dragged
};

const CLayout::LAYOUTDESC CImageLayout::s_layoutdesc =
{
    0, // _dwFlags
};

//+-------------------------------------------------------------------------
//
//  Method:     CImageLayout::CalcSize
//
//  Synopsis   : This function adjusts the size of the image
//               it uses the image Width and Height if present
//
//--------------------------------------------------------------------------

DWORD
CImgElementLayout::CalcSize( CCalcInfo * pci,
                             SIZE      * psize,
                             SIZE      * psizeDefault)
{
    Assert(ElementOwner());
    CImgElement     * pImg = DYNCAST(CImgElement, ElementOwner());
    CScopeFlag        csfCalcing(this);
    CElement::CLock   LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo     sci(pci, this);
    CSize             sizeOriginal;
    DWORD             grfReturn;
    HRESULT           hr;

    Assert(pci);
    Assert(psize);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    // If sizing is needed or it is a min/max request, handle it here
    if (   (    pci->_smMode != SIZEMODE_SET
            &&  _fSizeThis)
        || pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH
        )
    {
        Assert(pImg->_pImage);
        pImg->_pImage->CalcSize(pci, psize);

        if (    pci->_smMode == SIZEMODE_NATURAL
            ||  pci->_smMode == SIZEMODE_SET
            ||  pci->_smMode == SIZEMODE_FULLSIZE)
        {
            //
            // If dirty, ensure display tree nodes exist
            //

            if (    _fSizeThis
                &&  (SUCCEEDED(hr = EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)))))
            {
                if (pImg->_pImage->_pBitsCtx)
                {
                    SetPositionAware();
                }

                if (hr == S_FALSE)
                    grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
            }
            _fSizeThis    = FALSE;
            grfReturn    |= LAYOUT_THIS  |
                            (psize->cx != sizeOriginal.cx
                                    ? LAYOUT_HRESIZE
                                    : 0) |
                            (psize->cy != sizeOriginal.cy
                                    ? LAYOUT_VRESIZE
                                    : 0);

            //
            // Size display nodes if size changes occurred
            //

            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize, (grfReturn & LAYOUT_FORCE));
                pImg->_pImage->SetActivity();
            }
        }
        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            psize->cy = psize->cx;
        }
    }

    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}

void
CImgElementLayout::InvalidateFrame()
{
    CRect   rcImg;

    DYNCAST(CImgElement, ElementOwner())->_pImage->GetRectImg(&rcImg);
    Invalidate(&rcImg);
}

void
CInputImageLayout::InvalidateFrame()
{
    CRect   rcImg;

    DYNCAST(CInput, ElementOwner())->_pImage->GetRectImg(&rcImg);
    Invalidate(&rcImg);
}

//+----------------------------------------------------------------------------
//
// Member: GetMarginInfo
//
//  add hSpace/vSpace to the margin
//
//-----------------------------------------------------------------------------
void
CImgElementLayout::GetMarginInfo(CParentInfo *ppri,
                                LONG * plLMargin,
                                LONG * plTMargin,
                                LONG * plRMargin,
                                LONG * plBMargin)
{
    CImgElement *      pImg            = DYNCAST(CImgElement, ElementOwner());

    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);
    Assert(pImg && pImg->_pImage);
    pImg->_pImage->GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);
}

//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CImgElementLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    CImgElement *   pImg = DYNCAST(CImgElement, ElementOwner());

    // draw the image
    pImg->_pImage->Draw(pDI);

    // Draw the map if in edit mode
    if (pImg->IsEditable(TRUE))
    {
        pImg->EnsureMap();
        if (pImg->_pMap)
        {
            pImg->_pMap->Draw(pDI, pImg);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CInputImageLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    CInput *   pImg = DYNCAST(CInput, ElementOwner());
    // draw the image
    pImg->_pImage->Draw(pDI);
}


HRESULT
CImgElementLayout::PreDrag(DWORD          dwKeyState,
                    IDataObject ** ppDO,
                    IDropSource ** ppDS)
{
    HRESULT                 hr          = S_OK;
    CImgDragDropSrcInfo *   pDragInfo   = NULL;
    CGenDataObject *        pDO         = NULL;
    CImgElement *           pImgElem    = DYNCAST(CImgElement, ElementOwner());
    CImgHelper *            pImg        = pImgElem->_pImage;

    Assert(!Doc()->_pDragDropSrcInfo);

    pDragInfo = new CImgDragDropSrcInfo;
    if (!pDragInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(CreateImgDataObject(Doc(), pImg->_pImgCtx, pImg->_pBitsCtx, ElementOwner(), &pDO));
    if (hr)
    {
        goto Cleanup;
    }
    Assert(pDO);
    pDO->SetBtnState(dwKeyState);

    hr = pDO->QueryInterface(IID_IDataObject,(void **)ppDO);
    if (hr)
        goto Cleanup;

    hr = pDO->QueryInterface(IID_IDropSource,(void **)ppDS);
    if (hr)
        goto Cleanup;

    pDragInfo->_pImgDO = pDO;
    Doc()->_pDragDropSrcInfo = pDragInfo;

Cleanup:
    if (hr)
    {
        if (pDragInfo)
        {
            delete pDragInfo;
        }
        if (pDO)
        {
            pDO->Release();
        }
    }
    RRETURN(hr);
}


HRESULT
CImgElementLayout::PostDrag(HRESULT hrDrop, DWORD dwEffect)
{
    Assert(Doc()->_pDragDropSrcInfo);

    DYNCAST(CImgDragDropSrcInfo, Doc()->_pDragDropSrcInfo)->_pImgDO->Release();
    RRETURN(S_OK);
}





//+----------------------------------------------------------------------------
//
// Member: GetMarginInfo
//
//  add hSpace/vSpace to the margin
//
//-----------------------------------------------------------------------------
void
CInputImageLayout::GetMarginInfo(CParentInfo *ppri,
                                LONG * plLMargin,
                                LONG * plTMargin,
                                LONG * plRMargin,
                                LONG * plBMargin)
{
    CInput *      pImg            = DYNCAST(CInput, ElementOwner());

    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);
    Assert(pImg && pImg->_pImage);
    pImg->_pImage->GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);
}

DWORD
CInputImageLayout::CalcSize( CCalcInfo * pci,
                             SIZE *      psize,
                             SIZE *      psizeDefault)
{
    Assert(ElementOwner());
    CInput          * pImg = DYNCAST(CInput, ElementOwner());
    CScopeFlag        csfCalcing(this);
    CElement::CLock   LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSize             sizeOriginal;
    DWORD             grfReturn;
    HRESULT           hr;

    Assert(pci);
    Assert(psize);

    GetSize(&sizeOriginal);

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    // If sizing is needed or it is a min/max request, handle it here
    if (   (    pci->_smMode != SIZEMODE_SET
            &&  _fSizeThis)
        || pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH
        )
    {
        Assert(pImg->_pImage);
        pImg->_pImage->CalcSize(pci, psize);

        if (    pci->_smMode == SIZEMODE_NATURAL
            ||  pci->_smMode == SIZEMODE_SET
            ||  pci->_smMode == SIZEMODE_FULLSIZE)
        {
            //
            // If dirty, ensure display tree nodes exist
            //

            if (    _fSizeThis
                &&  (SUCCEEDED(hr = EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)))))
            {
                if (pImg->_pImage->_pBitsCtx)
                {
                    SetPositionAware();
                }

                if (hr == S_FALSE)
                    grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
            }
            _fSizeThis    = FALSE;
            grfReturn    |= LAYOUT_THIS  |
                            (psize->cx != sizeOriginal.cx
                                    ? LAYOUT_HRESIZE
                                    : 0) |
                            (psize->cy != sizeOriginal.cy
                                    ? LAYOUT_VRESIZE
                                    : 0);

            //
            // Size display nodes if size changes occurred
            //

            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize, (grfReturn & LAYOUT_FORCE));
                pImg->_pImage->SetActivity();
            }
        }
        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            psize->cy = psize->cx;
        }
    }

    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}

HRESULT
CInputImageLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;
    CRectShape *    pShape;
    HRESULT         hr = S_FALSE;

    *ppShape = NULL;

    GetClientRect(&rc);
    if (rc.IsEmpty())
        goto Cleanup;

    pShape = new CRectShape;
    if (!pShape)
    {
        *ppShape = NULL;
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pShape->_rect = rc;
    pShape->_rect.InflateRect(1, 1);
    *ppShape = pShape;

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_OK);
}

CImgHelper *
CImageLayout::GetImgHelper()
{
    CElement * pElement = ElementOwner();
    CImgHelper * pImgHelper = NULL;

    Assert(pElement);
    if (pElement->Tag() == ETAG_IMG)
        pImgHelper = DYNCAST(CImgElement, pElement)->_pImage;
    else if (pElement->Tag() == ETAG_INPUT)
        pImgHelper = DYNCAST(CInput, pElement)->_pImage;

    Assert(pImgHelper);

    return pImgHelper;
}

//+---------------------------------------------------------------------------
//
//  Member:     HandleViewChange
//
//  Synopsis:   Respond to change of in view status
//
//  Arguments:  flags           flags containing state transition info
//              prcClient       client rect in global coordinates
//              prcClip         clip rect in global coordinates
//              pDispNode       node which moved
//
//----------------------------------------------------------------------------
void
CImageLayout::HandleViewChange(
    DWORD           flags,
    const RECT*     prcClient,
    const RECT*     prcClip,
    CDispNode*      pDispNode)
{
    CImgHelper * pImg = GetImgHelper();

    if (!pImg->_fVideoPositioned)
    {
        pImg->_fVideoPositioned = TRUE;
        pImg->SetVideo();
    }

    if (pImg->_hwnd)
    {           
        CRect rcClip(*prcClip);
        UINT uFlags = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS;

        if (flags & VCF_INVIEWCHANGED)
            uFlags |= (flags & VCF_INVIEW) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

// BUGBUG (lmollico): setting SWP_NOREDRAW causes some problems: the activemovie window paints itself
// between our ScrollDC and EndDeferWindowPos
/*        else if (flags & VCF_NOREDRAW)
            uFlags |= SWP_NOREDRAW;*/

        DeferSetWindowPos(pImg->_hwnd, (RECT *) prcClient, uFlags, NULL);

        rcClip.OffsetRect(-((const CRect*)prcClient)->TopLeft().AsSize());
        ::SetWindowRgn(pImg->_hwnd, ::CreateRectRgnIndirect(&rcClip), 
            !(flags & VCF_NOREDRAW));

        if (pImg->_pVideoObj)
        {
            RECT rcImg;

            rcImg.top = 0;
            rcImg.left = 0;
            rcImg.bottom = prcClient->bottom - prcClient->top;
            rcImg.right = prcClient->right - prcClient->left;
            pImg->_pVideoObj->SetWindowPosition(&rcImg);
        }
    }

    return;
}

