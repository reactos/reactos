//+----------------------------------------------------------------------------
//
// File:        CKBOXLYT.HXX
//
// Contents:    Layout classes for <INPUT type=checkbox|radio>
//              CCheckboxLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_CKBOXLYT_HXX_
#define I_CKBOXLYT_HXX_
#pragma INCMSG("--- Beg 'ckboxlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

MtExtern(CCheckboxLayout)

class CShape;

class CCheckboxLayout : public CLayout
{
public:

    typedef CLayout super;

    CCheckboxLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCheckboxLayout))

    // Sizing and Positioning
    virtual void    Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape **ppShape);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'ckboxlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'ckboxlyt.hxx'")
#endif
