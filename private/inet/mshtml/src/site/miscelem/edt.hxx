//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       edt.hxx
//
//  Contents:   CDTElement class
//
//----------------------------------------------------------------------------

#ifndef I_EDT_HXX_
#define I_EDT_HXX_
#pragma INCMSG("--- Beg 'edt.hxx'")

#define _hxx_
#include "dt.hdl"

MtExtern(CDTElement)

class CDTElement : public CElement
{
    DECLARE_CLASS_TYPES(CDTElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDTElement))

    CDTElement(CDoc *pDoc)
      : CElement(ETAG_DT, pDoc) {}
    ~CDTElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);


    #define _CDTElement_
    #include "dt.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CDTElement);
};

#pragma INCMSG("--- End 'edt.hxx'")
#else
#pragma INCMSG("*** Dup 'edt.hxx'")
#endif
