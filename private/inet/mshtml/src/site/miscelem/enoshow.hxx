//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       escript.hxx
//
//  Contents:   CScriptElement
//
//  History:    09-Jul-1996     AnandRa     Created
//
//----------------------------------------------------------------------------

#ifndef I_ENOSHOW_HXX_
#define I_ENOSHOW_HXX_
#pragma INCMSG("--- Beg 'enoshow.hxx'")

#define _hxx_
#include "noshow.hdl"

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

MtExtern(CNoShowElement)
MtExtern(CShowElement)

//+---------------------------------------------------------------------------
//
// CNoShowElement
//
//----------------------------------------------------------------------------


class CNoShowElement : public CElement
{
    DECLARE_CLASS_TYPES(CNoShowElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CNoShowElement))

    CNoShowElement(ELEMENT_TAG etag, CDoc *pDoc) : CElement(etag, pDoc) {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    #define _CNoShowElement_
    #include "noshow.hdl"

    HRESULT SetContents(CBuffer2 *pbuf2) { RRETURN(pbuf2->SetCStr(&_cstrContents)); }
    HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

    NO_COPY(CNoShowElement);

    CStr _cstrContents;
};

#pragma INCMSG("--- End 'enoshow.hxx'")
#else
#pragma INCMSG("*** Dup 'enoshow.hxx'")
#endif
