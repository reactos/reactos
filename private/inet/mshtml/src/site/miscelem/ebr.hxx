//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ebr.hxx
//
//  Contents:   CBRElement class
//
//----------------------------------------------------------------------------

#ifndef I_EBR_HXX_
#define I_EBR_HXX_
#pragma INCMSG("--- Beg 'ebr.hxx'")

#define _hxx_
#include "br.hdl"

MtExtern(CBRElement)

class CBRElement : public CElement
{
    DECLARE_CLASS_TYPES(CBRElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBRElement))

    CBRElement(CDoc *pDoc)
      : CElement(ETAG_BR, pDoc) {}

    ~CBRElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    #define _CBRElement_
    #include "br.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
    
private:
    NO_COPY(CBRElement);
};

#pragma INCMSG("--- End 'ebr.hxx'")
#else
#pragma INCMSG("*** Dup 'ebr.hxx'")
#endif
