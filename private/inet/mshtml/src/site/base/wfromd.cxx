//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       wfromd.cxx
//
//  Contents:   Contains coordinate conversion functions.
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

//+------------------------------------------------------------------------
//
//  Member:     CDoc::DocumentFromWindow
//              CDoc::DocumentFromScreen
//              CDoc::ScreenFromDocument
//              CDoc::ScreenFromWindow
//              CDoc::HimetricFromDevice
//              CDoc::DeviceFromHimetric
//
//  Synopsis:   Coordinate conversion using the document transform
//
//-------------------------------------------------------------------------

void
CDoc::DocumentFromWindow(RECTL *prcl, const RECT *prc)
{
    DocumentFromWindow((POINTL *)&prcl->left, *(POINT *)&prc->left);
    DocumentFromWindow((POINTL *)&prcl->right, *(POINT *)&prc->right);
}

void
CDoc::DocumentFromScreen(POINTL *pptl, POINT pt)
{
    *(POINT *)pptl = pt;
    ScreenToClient(GetHWND(), (POINT *)pptl);
    DocumentFromWindow(pptl, pptl->x, pptl->y);
}

void
CDoc::ScreenFromWindow (POINT *ppt, POINT pt)
{
    *ppt = pt;
    ClientToScreen(GetHWND(), ppt);
}

void
CDoc::HimetricFromDevice(RECTL *prcl, const RECT *prc)
{
    HimetricFromDevice((POINTL *)&prcl->left, *(POINT *)&prc->left);
    HimetricFromDevice((POINTL *)&prcl->right, *(POINT *)&prc->right);
}

void
CDoc::DeviceFromHimetric(RECT *prc, const RECTL *prcl)
{
    DeviceFromHimetric((POINT *)&prc->left, *(POINTL *)&prcl->left);
    DeviceFromHimetric((POINT *)&prc->right, *(POINTL *)&prcl->right);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::DeviceFromHimetric
//              CDoc::HimetricFromDevice
//              CDoc::DocumentFromScreen
//
//  Synopsis:   Convert between himetric CDoc coordinates and
//              window coordinates, accounting for scaling.
//
//-------------------------------------------------------------------------

/*
void
CDoc::DeviceFromHimetric(SIZE *psize, int cxl, int cyl)
{
    psize->cx = MulDivQuick(cxl,
            100 * _pSiteRoot->_rc.right,
            100 * _sizel.cx);
    psize->cy = MulDivQuick(cyl,
            100 * _pSiteRoot->_rc.bottom,
            100 * _sizel.cy);
}

void
CDoc::DeviceFromHimetric(POINT *ppt, int xl, int yl)
{
    ppt->x = _pSiteRoot->_rc.left + MulDivQuick(xl,
            100 * _pSiteRoot->_rc.right,
            100 *_sizel.cx);
    ppt->y = _pSiteRoot->_rc.top + MulDivQuick(yl,
            100 * _pSiteRoot->_rc.bottom,
            100 * _sizel.cy);
}

void
CDoc::DeviceFromHimetric(RECT *prc, const RECTL *prcl)
{
    DeviceFromHimetric((POINT *)&prc->left, *(POINTL *)&prcl->left);
    DeviceFromHimetric((POINT *)&prc->right, *(POINTL *)&prcl->right);
}

void
CDoc::HimetricFromDevice(SIZEL *psizel, int cx, int cy)
{
    psizel->cx = MulDivQuick(cx,
            100 * _sizel.cx,
            100 * _pSiteRoot->_rc.right);
    psizel->cy = MulDivQuick(cy,
            100 * _sizel.cy,
            100 * _pSiteRoot->_rc.bottom);
}

void
CDoc::HimetricFromDevice(POINTL *pptl, int x, int y)
{
    pptl->x = MulDivQuick(x - _pSiteRoot->_rc.left,
            100 * _sizel.cx,
            100 * _pSiteRoot->_rc.right);
    pptl->y = MulDivQuick(y - _pSiteRoot->_rc.top,
            100 * _sizel.cy,
            100 * _pSiteRoot->_rc.bottom);
}

void
CDoc::HimetricFromDevice(RECTL *prcl, const RECT *prc)
{
    HimetricFromDevice((POINTL *)&prcl->left, *(POINT *)&prc->left);
    HimetricFromDevice((POINTL *)&prcl->right, *(POINT *)&prc->right);
}
*/
