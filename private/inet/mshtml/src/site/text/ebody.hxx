//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       ebody.hxx
//
//  Contents:   CBodyElement class
//
//----------------------------------------------------------------------------

#ifndef I_EBODY_HXX_
#define I_EBODY_HXX_
#pragma INCMSG("--- Beg 'ebody.hxx'")

#define _hxx_
#include "body.hdl"

class CBodyLayout;

//+---------------------------------------------------------------------------
//
// CBodyElement
//
//----------------------------------------------------------------------------

MtExtern(CBodyElement)

class CBodyElement : public CTxtSite
{
    DECLARE_CLASS_TYPES(CBodyElement, CTxtSite)

public:
    enum { NUM_COMMON_PROPS = 5 };

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBodyElement))

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    CBodyElement ( CDoc * pDoc );

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);

    virtual void    Notify(CNotification *pNF);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo * pCFI);

    virtual DWORD   GetBorderInfo(
                        CDocInfo * pdci,
                        CBorderInfo * pborderinfo,
                        BOOL fAll = FALSE);
    DECLARE_LAYOUT_FNS(CBodyLayout)

    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    // The following are used by the parser to determine whether or not to
    // apply the attrs of a BODY tag to an existing body element. If the
    // body was synthesized because something was seen that needed to be
    // parented by a body, and then a BODY tag was seen, the attrs of the
    // late-arriving tag will be applied to the synthesized element (which
    // will then no longer be considered synthesized).
    void            SetSynthetic(BOOL fSynthetic) { _fSynthetic = fSynthetic;}
    BOOL            GetSynthetic() { return _fSynthetic; }

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    void            WaitForRecalc();

    #define _CBodyElement_
    #include "body.hdl"

    static CElement::ACCELS             s_AccelsBodyDesign;
    static CElement::ACCELS             s_AccelsBodyRun;

protected:

    DECLARE_CLASSDESC_MEMBERS;
    static const CLSID *                s_apclsidPages[];


private:
    unsigned    _fSynthetic:1;

public:

    NO_COPY(CBodyElement);
};

#pragma INCMSG("--- End 'ebody.hxx'")
#else
#pragma INCMSG("*** Dup 'ebody.hxx'")
#endif
