//+----------------------------------------------------------------------------
//
// File:        INPUTLYT.HXX
//
// Contents:    Layout classes for <INPUT>
//              CInputTxtBaseLayout, CInputTextLayout, CTextAreaLayout,
//              CInputFileLayout, CCheckboxLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_INPUTLYT_HXX_
#define I_INPUTLYT_HXX_
#pragma INCMSG("--- Beg 'inputlyt.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CBtnHelper;

MtExtern(CInputLayout)
MtExtern(CInputTxtBaseLayout)
MtExtern(CInputTextLayout)
MtExtern(CInputFileLayout)
MtExtern(CInputButtonLayout)

class CShape;

class CInputLayout : public CFlowLayout
{
public:

    DECLARE_CLASS_TYPES(CInputLayout, CFlowLayout)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputLayout))

    // Construct / Destruct
    CInputLayout(CElement * pElementOwner) : super(pElementOwner)
    {
        _fContentsAffectSize = FALSE;
        // This would optimize some functions, like CParentSite::SiteArray().
        _fCanHaveChildren = FALSE;
    }


    htmlInput       GetType() const;

    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual DWORD   CalcSizeHelper(CCalcInfo * pci, SIZE *psize) { return 0; }

    virtual BOOL    GetAutoSize() const;
    virtual BOOL    GetMultiLine() const;
    virtual BOOL    GetWordWrap() const { return FALSE; }
    virtual LONG    GetMaxLength();

    HRESULT         GetFontSize(CCalcInfo * pci, SIZE * psizeFontShort, SIZE * psizeFontLong);
    virtual HRESULT GetElementsInZOrder(
                    CPtrAry<CElement *> *parySites,
                    CElement            *pElementThis,
                    RECT *               prc = NULL,
                    HRGN                 hrgn = NULL,
                    BOOL                 fIncludeNotVisible = FALSE) { return S_FALSE; }

    virtual HRESULT OnSelectionChange(void);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;

    SIZE _sizeFontLong;
    SIZE _sizeFontShort;
};


class CInputTextLayout : public CInputLayout
{
public:
    DECLARE_CLASS_TYPES(CInputTextLayout, CInputLayout)

    CInputTextLayout(CElement * pElementLayout);

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputTextLayout))

    virtual DWORD   CalcSizeHelper(CCalcInfo * pci, SIZE *psize);
    virtual void    GetMinSize(SIZE * pSize, CCalcInfo * pci) { pSize->cx = pSize->cy = 0; }
    virtual HRESULT OnTextChange(void);

    // Drag & drop
    virtual HRESULT PreDrag(
            DWORD dwKeyState,
            IDataObject **ppDO,
            IDropSource **ppDS);
};

class CInputFileLayout : public CInputTextLayout
{
    friend class CInput;
    friend class CInputSlaveLayout;
public:

    typedef CInputTextLayout super;

    CInputFileLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputFileLayout))

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual void    GetMinSize(SIZE * pSize, CCalcInfo * pci);
    HRESULT ComputeInputFileButtonSize(CCalcInfo * pci);
    void    GetButtonRect(RECT *prc);
private:
    HRESULT MeasureInputFileCaption(SIZE * psize, CCalcInfo * pci);
    void    RenderInputFileButtonBorder(CFormDrawInfo * pDI, UINT uFlag = BF_RECT);
    void    RenderInputFileButtonCaption(CFormDrawInfo * pDI);
    void    ComputeInputFileBorderInfo(CDocInfo *pdci, CBorderInfo & BorderInfo);
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
    virtual void Notify(CNotification * pnf);
private:
    DECLARE_LAYOUTDESC_MEMBERS;
    CSize    _sizeButton;
    LPTSTR   _pchButtonCaption;      //  Beware: This is not 0-terminated
    int      _cchButtonCaption;      //          and it should NOT be freed
    long     _xCaptionOffset;
    // long     _yCaptionOffset;
};


class CInputButtonLayout : public CInputLayout
{
    friend class CInputSlaveLayout;
public:

    typedef CInputLayout super;

    CInputButtonLayout(CElement * pElementLayout) : super(pElementLayout)
    {
        _fAutosizeBtn = 1;
    }

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputButtonLayout))

    virtual DWORD   CalcSizeHelper(CCalcInfo * pci, SIZE *psize);

    HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual CBtnHelper * GetBtnHelper();
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

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
    BOOL _fAutosizeBtn;
};

#pragma INCMSG("--- End 'inputlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'inputlyt.hxx'")
#endif
