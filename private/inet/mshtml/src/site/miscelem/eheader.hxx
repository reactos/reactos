//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       eheader.hxx
//
//  Contents:   CHeaderElement class
//
//----------------------------------------------------------------------------

#ifndef I_EHEADER_HXX_
#define I_EHEADER_HXX_
#pragma INCMSG("--- Beg 'eheader.hxx'")

#ifndef X_EBLOCK_HXX_
#define X_EBLOCK_HXX_
#include "eblock.hxx"
#endif

#define _hxx_
#include "header.hdl"

class CHeaderElement : public CBlockElement
{
    DECLARE_CLASS_TYPES(CHeaderElement, CBlockElement)
    
public:
    CHeaderElement(ELEMENT_TAG etag, CDoc *pDoc)
        : CBlockElement(etag, pDoc) { _nLevel = etag - ETAG_H1 + 1;}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    #define _CHeaderElement_
    #include "header.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    int _nLevel;
};

#pragma INCMSG("--- End 'eheader.hxx'")
#else
#pragma INCMSG("*** Dup 'eheader.hxx'")
#endif
