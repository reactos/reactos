//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       txtelems.hxx
//
//  Contents:   CTxtElement class
//
//----------------------------------------------------------------------------

#ifndef I_TXTELEMS_HXX_
#define I_TXTELEMS_HXX_
#pragma INCMSG("--- Beg 'txtelems.hxx'")

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#define _hxx_
#include "textelem.hdl"

MtExtern(CTextElement)

class CTextElement : public CElement
{
    DECLARE_CLASS_TYPES(CTextElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTextElement))

    CTextElement(ELEMENT_TAG etag, CDoc *pDoc)
      : CElement(etag, pDoc) {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

#define _CTextElement_
#include "textelem.hdl"


    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'txtelems.hxx'")
#else
#pragma INCMSG("*** Dup 'txtelems.hxx'")
#endif
