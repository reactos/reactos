//+----------------------------------------------------------------------------
//
// File:        MARQLYT.HXX
//
// Contents:    CMarqueeLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_MARQLYT_HXX_
#define I_MARQLYT_HXX_
#pragma INCMSG("--- Beg 'marqlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

MtExtern(CMarqueeLayout)

class CMarqueeLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CMarqueeLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarqueeLayout))

    virtual HRESULT Init();

    // Sizing and Positioning
    virtual DWORD CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual void  GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'marqlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'marqlyt.hxx'")
#endif
