//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ediv.hxx
//
//  Contents:   CDivAttrBag, CDivElement classes
//
//----------------------------------------------------------------------------

#ifndef I_DIV_HXX_
#define I_DIV_HXX_
#pragma INCMSG("--- Beg 'div.hxx'")

#ifndef X_EBLOCK_HXX_
#define X_EBLOCK_HXX_
#include "eblock.hxx"
#endif

#define _hxx_
#include "div.hdl"

class CDivElement : public CBlockElement
{
    DECLARE_CLASS_TYPES(CDivElement, CBlockElement)
    
public:

    CDivElement(CDoc *pDoc)
      : CBlockElement(ETAG_DIV, pDoc) {}

    ~CDivElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

#ifndef NO_DATABINDING
    // databinding over-rides from CElement
    virtual const CDBindMethods *GetDBindMethods();
#endif // ndef NO_DATABINDING

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    #define _CDivElement_
    #include "div.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CDivElement);
};

#pragma INCMSG("--- End 'div.hxx'")
#else
#pragma INCMSG("*** Dup 'div.hxx'")
#endif
