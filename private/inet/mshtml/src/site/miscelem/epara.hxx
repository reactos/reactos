//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       epara.hxx
//
//  Contents:   CParaElement class
//
//----------------------------------------------------------------------------

#ifndef I_EPARA_HXX_
#define I_EPARA_HXX_
#pragma INCMSG("--- Beg 'epara.hxx'")

#ifndef X_EBLOCK_HXX_
#define X_EBLOCK_HXX_
#include "eblock.hxx"
#endif

#define _hxx_
#include "para.hdl"

class CParaElement : public CBlockElement
{
    DECLARE_CLASS_TYPES(CParaElement, CBlockElement)
    
public:

    CParaElement(CDoc *pDoc)
      : CBlockElement(ETAG_P, pDoc) {}

    ~CParaElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    #define _CParaElement_
    #include "para.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CParaElement);
};

#pragma INCMSG("--- End 'epara.hxx'")
#else
#pragma INCMSG("*** Dup 'epara.hxx'")
#endif
