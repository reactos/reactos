//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       eolist.hxx
//
//  Contents:   COListElement class
//
//----------------------------------------------------------------------------

#ifndef I_EOLIST_HXX_
#define I_EOLIST_HXX_
#pragma INCMSG("--- Beg 'eolist.hxx'")

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#define _hxx_
#include "olist.hdl"

class COListElement : public CListElement
{
    DECLARE_CLASS_TYPES(COListElement, CListElement)
    
public:
    COListElement(CDoc *pDoc)
        : CListElement(ETAG_OL, pDoc) {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual styleListStyleType FilterHtmlListType( styleListStyleType type, WORD wLevel );

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    #define _COListElement_
    #include "olist.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'eolist.hxx'")
#else
#pragma INCMSG("*** Dup 'eolist.hxx'")
#endif
