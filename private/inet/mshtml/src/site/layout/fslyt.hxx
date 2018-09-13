//+----------------------------------------------------------------------------
//
// File:        FSLYT.HXX
//
// Contents:    CFieldSetLayout, CLegendLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_FSLYT_HXX_
#define I_FSLYT_HXX_
#pragma INCMSG("--- Beg 'fslyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

MtExtern(CLegendLayout)
MtExtern(CFieldSetLayout)

class CLegendLayout : public CFlowLayout
{
public:

    typedef CFlowLayout super;

    CLegendLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    virtual HRESULT Init();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLegendLayout))

    // Sizing and Positioning
    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);

    //+-----------------------------------------------------------------------
    // CSite methods
    //------------------------------------------------------------------------
    virtual void  GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    
    void GetLegendInfo(SIZE *pSizeLegend, POINT *pPosLegend);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

class CFieldSetLayout : public C1DLayout
{
public:

    typedef C1DLayout super;

    CFieldSetLayout(CElement * pElementLayout) : super(pElementLayout)
    {
#if 0
#ifndef DISPLAY_TREE
        ClearScrollBarFlag(fmScrollBarsVertical, SBF_ALLOWED);
        ClearScrollBarFlag(fmScrollBarsVertical, SBF_FORCED);
        ClearScrollBarFlag(fmScrollBarsVertical, SBF_FORCEDVISIBLE);
        ClearScrollBarFlag(fmScrollBarsHorizontal, SBF_ALLOWED);
        ClearScrollBarFlag(fmScrollBarsHorizontal, SBF_FORCED);
        ClearScrollBarFlag(fmScrollBarsHorizontal, SBF_FORCEDVISIBLE);
#else
// BUGBUG: When to set these? (brendand)
#endif
#endif
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFieldSetLayout))

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);
    virtual void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);
};

#pragma INCMSG("--- End 'fslyt.hxx'")
#else
#pragma INCMSG("*** Dup 'fslyt.hxx'")
#endif
