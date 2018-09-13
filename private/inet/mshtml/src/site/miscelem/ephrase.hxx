//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ephrase.hxx
//
//  Contents:   CPhraseElement class
//
//----------------------------------------------------------------------------

#ifndef I_EPHRASE_HXX_
#define I_EPHRASE_HXX_
#pragma INCMSG("--- Beg 'ephrase.hxx'")

#define _hxx_
#include "phrase.hdl"

MtExtern(CPhraseElement)
MtExtern(CSpanElement)

class CPhraseElement : public CElement
{
    DECLARE_CLASS_TYPES(CPhraseElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPhraseElement))

    CPhraseElement(ELEMENT_TAG etag, CDoc *pDoc)
      : CElement(etag, pDoc) {}

    ~CPhraseElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    #define _CPhraseElement_
    #include "phrase.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CPhraseElement);
};

class CSpanElement : public CPhraseElement
{
    DECLARE_CLASS_TYPES(CSpanElement, CPhraseElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSpanElement))

    CSpanElement (ELEMENT_TAG etag, CDoc *pDoc)
      : CPhraseElement(etag, pDoc) {}
    ~CSpanElement() {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
                                  
#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif


    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    #define _CSpanElement_
    #include "phrase.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CSpanElement);
};

#pragma INCMSG("--- End 'ephrase.hxx'")
#else
#pragma INCMSG("*** Dup 'ephrase.hxx'")
#endif
