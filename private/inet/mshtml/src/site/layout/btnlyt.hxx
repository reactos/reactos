//+----------------------------------------------------------------------------
//
// File:        BTNLYT.HXX
//
// Contents:    Layout classes for <BUTTON>
//              CButtonLayout
//
// Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_BTNLYT_HXX_
#define I_BTNLYT_HXX_
#pragma INCMSG("--- Beg 'btnlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CBtnHelper;

MtExtern(CButtonLayout)

class CShape;

class CButtonLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CButtonLayout(CElement * pElementLayout) : super(pElementLayout)
    {
        _fCanHaveChildren = TRUE;
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CButtonLayout))

    HRESULT GetFontSize(CCalcInfo * pci,
                                SIZE * psizeFontShort, SIZE * psizeFontLong);

    virtual DWORD   CalcSize(CCalcInfo * pci,
                                SIZE * psize, SIZE * psizeDefault = NULL);
#ifdef NEVER
    void AdjustInsetForSize(
                        CCalcInfo *     pci,
                        SIZE *          psize,
                        CBorderInfo *   pbInfo,
                        SIZE *          psizeFontForShortStr,
                        LONG            lCaret,
                        BOOL            fWidthNotSet,
                        BOOL            fHeightNotSet,
                        BOOL            fRightToLeft);

#endif

    virtual void DrawClient(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pDispSurface,
                CDispNode *     pDispNode,
                void *          cookie,
                void *          pClientData,
                DWORD           dwFlags);

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    virtual BOOL    GetAutoSize() const    { return TRUE;  }
    virtual BOOL    GetMultiLine() const   { return TRUE;  }
    virtual BOOL    GetWordWrap() const    { return FALSE; }

    virtual void    DoLayout(DWORD grfLayout);

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual CBtnHelper * GetBtnHelper();
protected:

    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'btnlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'btnlyt.hxx'")
#endif
