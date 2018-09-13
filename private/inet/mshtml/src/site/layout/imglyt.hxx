//+----------------------------------------------------------------------------
//
// File:        imglyt.HXX
//
// Contents:    CImgBaseLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_IMGLYT_HXX_
#define I_IMGLYT_HXX_
#pragma INCMSG("--- Beg 'imglyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

MtExtern(CImageLayout)
MtExtern(CImgElementLayout)
MtExtern(CInputImageLayout)

class CImageLayout : public CLayout
{
public:

    typedef CLayout super;

    CImageLayout(CElement * pElementLayout) : super(pElementLayout)
    {
        _fNoUIActivateInDesign = TRUE;
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImageLayout))

    // Sizing and Positioning
    void DrawFrame(IMGANIMSTATE * pImgAnimState) {};

    CImgHelper * GetImgHelper();
    BOOL         IsOpaque() { return GetImgHelper()->IsOpaque(); }

    void HandleViewChange(
            DWORD          flags,
            const RECT *   prcClient,
            const RECT *   prcClip,
            CDispNode *    pDispNode);


protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

class CImgElementLayout : public CImageLayout
{
public:

    typedef CImageLayout super;

    CImgElementLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgElementLayout))

    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);

    // Drag & drop
    virtual HRESULT PreDrag(
            DWORD dwKeyState,
            IDataObject **ppDO,
            IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hr, DWORD dwEffect);
    virtual void GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    virtual DWORD CalcSize(CCalcInfo * pci, SIZE *psize, SIZE *psizeDefault);
    void InvalidateFrame();
};

class CInputImageLayout : public CImageLayout
{
public:

    typedef CImageLayout super;

    CInputImageLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputImageLayout))
    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual void GetMarginInfo(CParentInfo *ppri,
                        LONG * plLeftMargin, LONG * plTopMargin,
                        LONG * plRightMargin, LONG *plBottomMargin);
    virtual DWORD CalcSize(CCalcInfo * pci, SIZE *psize, SIZE *psizeDefault);
    void InvalidateFrame();
    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape **ppShape);
};

#pragma INCMSG("--- End 'imglyt.hxx'")
#else
#pragma INCMSG("*** Dup 'imglyt.hxx'")
#endif
