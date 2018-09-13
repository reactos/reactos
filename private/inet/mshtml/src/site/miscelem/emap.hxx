//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       emap.hxx
//
//  Contents:   Definition of the Map Element class
//
//  Classes:    CMapElement
//
//----------------------------------------------------------------------------

#ifndef I_EMAP_HXX_
#define I_EMAP_HXX_
#pragma INCMSG("--- Beg 'emap.hxx'")

#define _hxx_
#include "map.hdl"

MtExtern(CMapElement)
MtExtern(CAreasCollection)

class CAreaElement;

class CMapElement : public CElement
{
    DECLARE_CLASS_TYPES(CMapElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMapElement))

    CMapElement(CDoc *pDoc);
    ~CMapElement();

    // Creation and Initialization
    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);
    virtual void Notify(CNotification *pNF);

    // Containment Checking
    HRESULT CheckSelect(POINT pt, HWND hwnd, RECT rc);
    HRESULT GetAreaContaining(POINT pt, long *plIndex);
    HRESULT GetAreaContaining(long lIndex, CAreaElement **ppArea);
    HRESULT SearchArea(CAreaElement *ppArea, long *plIndex);
    LONG    GetAreaCount();
    HRESULT GetAreaTabs(long *pTabs, long c);
    
    void GetBoundingRect(RECT *prc);

    // Drawing Related
    HRESULT Draw(CFormDrawInfo * pDI, CElement * pImg);

    NV_DECLARE_CREATECOL_METHOD(CreateAreaCollection, createareacollection, (IDispatch ** ppIEC, long lIndex));
    NV_DECLARE_ENSURE_METHOD(EnsureAreaCollection, ensureareacollection, (long, long *plCollectionVersion));
    NV_DECLARE_ADDNEWOBJECT_METHOD(AddNewArea, addnewarea, (long         lIndex, 
                               IDispatch *  pObject, 
                               long         index));

    HRESULT AddAreaHelper(CAreaElement * pArea, long lIndex);
    HRESULT RemoveAreaHelper(long lIndex);
    
    #define _CMapElement_
    #include "map.hdl"

private:

    // Area Collection
    HRESULT EnsureCollectionCache();
    enum { AREA_ELEMENT_COLLECTION = 0 };

protected:
    DECLARE_CLASSDESC_MEMBERS;

public:
    CCollectionCache *_pCollectionCache;
    long _lCollectionVersion;

    CMapElement *   _pMapNext;

private:
    NO_COPY(CMapElement);
};

//+------------------------------------------------------------
//
//  Class : CAreasCollection
//
//  Description: derived from CElementCollection the 
//      areas collection has the additional properties of add 
//      and remove
//
//------------------------------------------------------------
class CAreasCollection : public CElementCollectionBase,
                         public IHTMLAreasCollection
{
    typedef CElementCollectionBase super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAreasCollection))

    CAreasCollection(CCollectionCache * pCache,long lIndex) : 
      super(pCache, lIndex) {}
    ~CAreasCollection();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(super);
    DECLARE_DERIVED_DISPATCH(super);

    DECLARE_PRIVATE_QI_FUNCS(CBase)


    #define _CAreasCollection_
    #include "map.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS
};

#pragma INCMSG("--- End 'emap.hxx'")
#else
#pragma INCMSG("*** Dup 'emap.hxx'")
#endif

