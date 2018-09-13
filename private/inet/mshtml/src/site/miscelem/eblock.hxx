//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eblock.hxx
//
//  Contents:   CBlockAttrBag, CBlockElement classes
//
//----------------------------------------------------------------------------

#ifndef I_EBLOCK_HXX_
#define I_EBLOCK_HXX_
#pragma INCMSG("--- Beg 'eblock.hxx'")

#define _hxx_
#include "block.hdl"

MtExtern(CBlockElement)

//----------------------------------------------------------------------------

class CBlockElement : public CElement
{
    DECLARE_CLASS_TYPES(CBlockElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBlockElement))

    CBlockElement (ELEMENT_TAG etag, CDoc *pDoc)
      : CElement(etag, pDoc) {}

    ~CBlockElement() {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    HRESULT Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd);


    #define _CBlockElement_
    #include "block.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CBlockElement);
};

#pragma INCMSG("--- End 'eblock.hxx'")
#else
#pragma INCMSG("*** Dup 'eblock.hxx'")
#endif
