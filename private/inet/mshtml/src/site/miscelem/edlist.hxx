//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       edlist.hxx
//
//  Contents:   CDListElement class
//
//----------------------------------------------------------------------------

#ifndef I_EDLIST_HXX_
#define I_EDLIST_HXX_
#pragma INCMSG("--- Beg 'edlist.hxx'")

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#define _hxx_
#include "dlist.hdl"

class CDListElement : public CListElement
{
    DECLARE_CLASS_TYPES(CDListElement, CListElement)
    
public:
    CDListElement(CDoc *pDoc)
        : CListElement(ETAG_DL, pDoc) {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    #define _CDListElement_
    #include "dlist.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'edlist.hxx'")
#else
#pragma INCMSG("*** Dup 'edlist.hxx'")
#endif
