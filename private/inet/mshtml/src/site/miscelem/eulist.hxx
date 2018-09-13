//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       eulist.hxx
//
//  Contents:   CUListElement class
//
//----------------------------------------------------------------------------

#ifndef I_EULIST_HXX_
#define I_EULIST_HXX_
#pragma INCMSG("--- Beg 'eulist.hxx'")

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#define _hxx_
#include "ulist.hdl"

class CUListElement : public CListElement
{
    DECLARE_CLASS_TYPES(CUListElement, CListElement)
    
public:
    CUListElement(CDoc *pDoc)
        : CListElement(ETAG_UL, pDoc) {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual styleListStyleType FilterHtmlListType(  styleListStyleType type, WORD wLevel );


    #define _CUListElement_
    #include "ulist.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'eulist.hxx'")
#else
#pragma INCMSG("*** Dup 'eulist.hxx'")
#endif
