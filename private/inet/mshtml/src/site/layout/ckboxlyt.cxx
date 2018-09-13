//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ckboxlyt.cxx
//
//  Contents:   Implementation of layout class for <INPUT type=checkbox|radio>
//
//----------------------------------------------------------------------------

#include "headers.hxx"


#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_CKBOXLYT_HXX_
#define X_CKBOXLYT_HXX_
#include "ckboxlyt.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

MtDefine(CCheckboxLayout, Layout, "CCheckboxLayout")

const CLayout::LAYOUTDESC CCheckboxLayout::s_layoutdesc =
{
    0,          // _dwFlags
};

//+-------------------------------------------------------------------------
//
//  Method:     CCheckboxLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
#define CHKBOX_SITE_SIZE_W  20
#define CHKBOX_SITE_SIZE_H  20

const RECT s_CbDefOffsetRect = {4, 4, 3, 3};

DWORD
CCheckboxLayout::CalcSize( CCalcInfo * pci,
                           SIZE *      psize,
                           SIZE *      psizeDefault)
{
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

    DWORD   grfReturn;

    _fContentsAffectSize = FALSE;

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    _fSizeThis = _fSizeThis || (pci->_grfLayout & LAYOUT_FORCE);

    if (_fSizeThis)
    {
        SIZE        sizeDefault;   

        sizeDefault.cx = pci->DocPixelsFromWindowX(CHKBOX_SITE_SIZE_W);
        sizeDefault.cy = pci->DocPixelsFromWindowY(CHKBOX_SITE_SIZE_H);
        grfReturn = super::CalcSize(pci, psize, &sizeDefault);
    }
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCheckboxElement::Draw
//
//  Synopsis:   renders the glyph for the button
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

void
CCheckboxLayout::Draw(CFormDrawInfo * pDI, CDispNode * pDispNode)
{
    RECT    rc;
    SIZE    sizeDefault;
    SIZE    sizeControl;
    SIZE    sizeDefGlyph;

    sizeDefault.cx = pDI->DocPixelsFromWindowX(CHKBOX_SITE_SIZE_W);
    sizeDefault.cy = pDI->DocPixelsFromWindowY(CHKBOX_SITE_SIZE_H);

    //use rc to hold offset
    rc.left   = pDI->DocPixelsFromWindowX(s_CbDefOffsetRect.left);
    rc.top    = pDI->DocPixelsFromWindowY(s_CbDefOffsetRect.top);
    rc.right  = pDI->DocPixelsFromWindowX(s_CbDefOffsetRect.right);
    rc.bottom = pDI->DocPixelsFromWindowY(s_CbDefOffsetRect.bottom);

    pDI->GetDC(TRUE);       // Ensure the DI has an HDC

    // GetClientRect(&rcClient);

    sizeControl.cx  = pDI->_rc.right - pDI->_rc.left;
    sizeDefGlyph.cx = sizeDefault.cx - rc.right - rc.left;
    sizeDefGlyph.cy = sizeDefault.cy - rc.bottom - rc.top;
    if (sizeControl.cx >= sizeDefault.cx)
    {
        rc.right  = pDI->_rc.right - rc.right;
        Assert(!pDispNode->IsRightToLeft() ? rc.right >= 0 : rc.right <= 0);

        rc.left   = pDI->_rc.left + rc.left;
        Assert(rc.left <= rc.right);
    }
    else if (sizeControl.cx > sizeDefGlyph.cx)
    {
        rc.left     = pDI->_rc.left + (sizeControl.cx - sizeDefGlyph.cx) / 2;
        rc.right    = rc.left + sizeDefGlyph.cx;
    }
    else
    {
        rc.right = pDI->_rc.right;
        rc.left = pDI->_rc.left;
    }

    sizeControl.cy = pDI->_rc.bottom - pDI->_rc.top;
    if (sizeControl.cy >= sizeDefault.cy)
    {
        rc.bottom = pDI->_rc.bottom - rc.bottom;
        Assert(rc.bottom >= 0);
        rc.top    = pDI->_rc.top + rc.top;
        Assert(rc.top <= rc.bottom);
    }
    else if (sizeControl.cx > sizeDefGlyph.cx)
    {
        rc.top      = pDI->_rc.top + (sizeControl.cy - sizeDefGlyph.cy) / 2;
        rc.bottom   = rc.top + sizeDefGlyph.cy;
    }
    else
    {
        rc.top = pDI->_rc.top;
        rc.bottom = pDI->_rc.bottom;
    }

    DYNCAST(CInput, ElementOwner())->RenderGlyph(pDI, &rc);
}

HRESULT
CCheckboxLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    HRESULT hr = S_FALSE;

    CLabelElement * pLabel = ElementOwner()->GetLabel();
    if (pLabel)
    {
        hr = THR(pLabel->GetFocusShape(lSubDivision, pdci, ppShape));
    }
    else
    {
        CRect           rc;
        CRectShape *    pShape;

        *ppShape = NULL;

        GetRect(&rc, COORDSYS_CONTENT);
        if (rc.IsEmpty())
            goto Cleanup;

        pShape = new CRectShape;
        if (!pShape)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pShape->_rect = rc;
        pShape->_rect.InflateRect(1, 1);
        *ppShape = pShape;

        hr = S_OK;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}
