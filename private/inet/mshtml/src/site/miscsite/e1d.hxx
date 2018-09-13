//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       e1d.hxx
//
//  Contents:   CFlowSite, CSpanSite, C1DElement, and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_E1D_HXX_
#define I_E1D_HXX_
#pragma INCMSG("--- Beg 'e1h.hxx'")

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#define _hxx_
#include "e1d.hdl"

#define _hxx_
#include "espan.hdl"

class C1DLayout;
class CLegendLayout;
class CFieldSetLayout;

//+---------------------------------------------------------------------------
//
//  Class:      C1DElement, CSpanSite
//
//  Purpose:    1D container element: <DIV>
//              Span site <SPAN>
//
//----------------------------------------------------------------------------

class CFlowSite : public CTxtSite
{
    DECLARE_CLASS_TYPES(CFlowSite, CTxtSite)

    DECLARE_LAYOUT_FNS(C1DLayout)

private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:

    CFlowSite (ELEMENT_TAG etag, CDoc *pDoc);

#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif // ndef NO_DATABINDING

};

MtExtern(C1DElement)

class C1DElement : public CFlowSite
{
    DECLARE_CLASS_TYPES(C1DElement, CFlowSite)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(C1DElement))
    C1DElement (CDoc *pDoc)
            : CFlowSite(ETAG_DIV, pDoc) {};
    C1DElement (ELEMENT_TAG etag, CDoc *pDoc)
            : CFlowSite(etag, pDoc) {};

    #define _C1DElement_
    #include "e1d.hdl"

    static const CLSID * s_apclsidPages[];

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(C1DElement);
};

MtExtern(CSpanSite)

class CSpanSite : public CFlowSite
{
    DECLARE_CLASS_TYPES(CSpanSite, CFlowSite)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSpanSite))
    CSpanSite (CDoc *pDoc)
            : CFlowSite(ETAG_SPAN, pDoc) {};
    CSpanSite (ELEMENT_TAG etag, CDoc *pDoc)
            : CFlowSite(etag, pDoc) {};

    #define _CSpanSite_
    #include "espan.hdl"

    static const CLSID * s_apclsidPages[];

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CSpanSite);
};

//+---------------------------------------------------------------------------
//
//  Class:      CLegendElement
//
//  Purpose:    HTML Legend object <LEGEND>
//
//----------------------------------------------------------------------------

MtExtern(CLegendElement)

class CLegendElement : public CTxtSite
{
    DECLARE_CLASS_TYPES(CLegendElement, CTxtSite)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLegendElement))

    DECLARE_LAYOUT_FNS(CLegendLayout)

    CLegendElement(ELEMENT_TAG etag, CDoc *pDoc): CTxtSite(etag, pDoc)
                    {}

    virtual void Notify(CNotification * pNF);

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
                                 
#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif

    virtual HRESULT ApplyDefaultFormat ( CFormatInfo * pCFI );


    //+-----------------------------------------------------------------------
    //  ILegendElement methods
    //------------------------------------------------------------------------

    #define _CLegendElement_
    #include "e1d.hdl"

    DECLARE_CLASSDESC_MEMBERS;
private:
    NO_COPY(CLegendElement);
};

//+---------------------------------------------------------------------------
//
//  Class:      CFieldSetElement
//
//  Purpose:    FieldSet element: <FIELDSET>
//
//----------------------------------------------------------------------------


#define FIELDSET_BORDER_OFFSET  5
#define FIELDSET_CAPTION_OFFSET 2

MtExtern(CFieldSetElement)

class CFieldSetElement : public C1DElement
{
    friend class CFieldSetLayout;

    DECLARE_CLASS_TYPES(CFieldSetElement, C1DElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFieldSetElement))

    DECLARE_LAYOUT_FNS(CFieldSetLayout)

    CFieldSetElement(CDoc *pDoc)
        : C1DElement(ETAG_FIELDSET, pDoc) {};

    CFieldSetElement (ELEMENT_TAG etag, CDoc *pDoc)
            : C1DElement(etag, pDoc) {};

    ~CFieldSetElement() {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
    virtual DWORD   GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    virtual HRESULT ApplyDefaultFormat ( CFormatInfo * pCFI );

    CLegendLayout * GetLegendLayout();

#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif

    #define _CFieldSetElement_
    #include "e1d.hdl"

protected:
    unsigned    _fDrawing:1;
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CFieldSetElement);
};

#pragma INCMSG("--- End 'e1h.hxx'")
#else
#pragma INCMSG("*** Dup 'e1h.hxx'")
#endif
