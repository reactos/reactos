//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       edd.hxx
//
//  Contents:   CDDElement class
//
//----------------------------------------------------------------------------

#ifndef I_EDD_HXX_
#define I_EDD_HXX_
#pragma INCMSG("--- Beg 'edd.hxx'")

#define _hxx_
#include "dd.hdl"

MtExtern(CDDElement)

class CDDElement : public CElement
{
    DECLARE_CLASS_TYPES(CDDElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDDElement))

    CDDElement(CDoc *pDoc)
      : CElement(ETAG_DD, pDoc) {}

    ~CDDElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    #define _CDDElement_
    #include "dd.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CDDElement);
};

#pragma INCMSG("--- End 'edd.hxx'")
#else
#pragma INCMSG("*** Dup 'edd.hxx'")
#endif
