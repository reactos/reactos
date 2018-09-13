//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       collect.hxx
//
//  Contents:   CCollectionCache
//
//----------------------------------------------------------------------------

#ifndef I_COLLECT_HXX_
#define I_COLLECT_HXX_
#pragma INCMSG("--- Beg 'collect.hxx'")

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#define _hxx_
#include "collect.hdl"


MtExtern(CCollectionCache)
MtExtern(CElementCollection)
MtExtern(CCollectionCacheItem)
MtExtern(CElementAryCacheItem)
MtExtern(CElementAryCacheItem_aryElements)
MtExtern(CElementAryCacheItem_aryElements_pv)

#ifdef WIN16

typedef HRESULT (BUGCALL *PFN_CVOID_ENSURE)(CVoid *, long lIndex, long * plVersionCookie);

#define NV_DECLARE_ENSURE_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args

#define ENSURE_METHOD(klass, fn, FN)\        
        (PFN_CVOID_ENSURE)&klass::FN

typedef HRESULT (BUGCALL *PFN_CVOID_CREATECOL)(CVoid *, IDispatch **pIEC,
                                                      long        lIndex);
#define NV_DECLARE_CREATECOL_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args
#define CREATECOL_METHOD(klass, fn, FN)\        
        (PFN_CVOID_CREATECOL)&klass::FN

typedef HRESULT (BUGCALL *PFN_CVOID_REMOVEOBJECT)(CVoid *, long     lCollection,
                                                  long     lIndex);

#define NV_DECLARE_REMOVEOBJECT_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args
#define REMOVEOBJECT_METHOD(klass, fn, FN)\
        (PFN_CVOID_REMOVEOBJECT)&klass::FN


typedef HRESULT (BUGCALL *PFN_CVOID_ADDNEWOBJECT)(CVoid *, long lIndex, 
                                                      IDispatch *pObject, 
                                                      long index);
#define NV_DECLARE_ADDNEWOBJECT_METHOD(fn, FN, args)\
        static HRESULT BUGCALL FN args;\
        HRESULT BUGCALL fn args
#define ADDNEWOBJECT_METHOD(klass, fn, FN)\        
        (PFN_CVOID_ADDNEWOBJECT)&klass::FN

#else

typedef HRESULT (BUGCALL CVoid::*PFN_CVOID_ENSURE)(long lIndex, long * plVersionCookie);

#define NV_DECLARE_ENSURE_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define ENSURE_METHOD(klass, fn, FN)\
        (PFN_CVOID_ENSURE)&klass::fn

typedef HRESULT (BUGCALL CVoid::*PFN_CVOID_CREATECOL)(IDispatch **pIEC,
                                                      long        lIndex);

#define NV_DECLARE_CREATECOL_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args

#define CREATECOL_METHOD(klass, fn, FN)\
        (PFN_CVOID_CREATECOL)&klass::fn

typedef HRESULT (BUGCALL CVoid::*PFN_CVOID_REMOVEOBJECT)(long     lCollection,
                                                         long     lIndex);

#define NV_DECLARE_REMOVEOBJECT_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args
#define REMOVEOBJECT_METHOD(klass, fn, FN)\
        (PFN_CVOID_REMOVEOBJECT)&klass::fn

typedef HRESULT (BUGCALL CVoid::*PFN_CVOID_ADDNEWOBJECT)(long lIndex, 
                                                      IDispatch *pObject, 
                                                      long index);
#define NV_DECLARE_ADDNEWOBJECT_METHOD(fn, FN, args)\
        HRESULT BUGCALL fn args
#define ADDNEWOBJECT_METHOD(klass, fn, FN)\
        (PFN_CVOID_ADDNEWOBJECT)&klass::fn


#endif

// Cache types for non-fixed cache items
enum CollCacheType {
    CacheType_FreeEntry,
    CacheType_Named,
    CacheType_Tag,
    CacheType_Children,
    CacheType_AllChildren,
    CacheType_DOMChildNodes,
    CacheType_CellRange,
    CacheType_Urn
};

class CCollectionCacheItem
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

    virtual ~CCollectionCacheItem(){}
    //
    // Get at current index, move currency to next item, return NULL if beyond end 
    // 
    //
    virtual CElement *GetNext ( void ) = 0; 
    //
    // Set the currency, return element at given index. 
    // Caller is responsible for bounds checking
    //
    virtual CElement *MoveTo ( long lIndex ) = 0; 

    virtual BOOL  IsRangeSyntaxSupported ( void )
    {
        return FALSE;   // Only cell collection supports cell range syntax a1:a10
    }
    
    //
    virtual void ResetContents ( void )
    {
        Assert ( 0 ); // Should never be called on this collection
    }
    //
    virtual void DeleteContents ( void )
    {
        // Just ignore it.
    }
    //
    virtual long Length ( void ) = 0;
    //
    //
    // Get at index, do not adjust currency
    //
    virtual CElement *GetAt ( long lIndex ) = 0;
    //
    // Insert before an index, maintain currency
    //
    virtual HRESULT InsertElementAt ( long lIndex, CElement *pElement )
    {
        Assert ( 0 ); // Should never be called on this collection
        return E_NOTIMPL;
    }
    //
    // Append an Element, retain currency
    //
    virtual HRESULT AppendElement ( CElement *pElement ) 
    {
        Assert ( 0 ); // Should never be called on this collection
        return E_NOTIMPL;
    }

    //
    virtual void DeleteAt ( long lIndex ) 
    {
        Assert ( 0 ); // Should never be called on this collection
    }
};


class CElementAryCacheItem : public CCollectionCacheItem
{
private:
    LONG _lCurrentIndex;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElementAryCacheItem))
    CPtrAry<CElement *> _aryElements;
    CElementAryCacheItem() : _aryElements(Mt(CElementAryCacheItem_aryElements_pv)) {}
    CElement *GetNext ( void )
    {
        //
        // GetNext will create an invalid currency if it indexes beyond the
        // End of the collection. This is OK
        //
        Assert ( _lCurrentIndex >=0 );
        if ( _lCurrentIndex >= _aryElements.Size() )
            return NULL;
        else
            return _aryElements [ _lCurrentIndex++ ];
    }
    CElement *MoveTo ( long lIndex )
    {
        Assert ( lIndex >=0 );
        if ( lIndex >= _aryElements.Size() )
            return NULL;
        else
        {
            _lCurrentIndex = lIndex;
            return _aryElements [ _lCurrentIndex ];
        }
    }  
    CElement *GetAt ( long lIndex )
    {
        Assert ( lIndex >=0 );
        if ( lIndex < _aryElements.Size() )
            return _aryElements [ lIndex ];
        else
            return NULL;
    }
    long Length ( void )
    {
        return _aryElements.Size();
    }
    void ResetContents ( void )
    {
        _aryElements.SetSize(0);
        _lCurrentIndex = 0;
    }
    void DeleteContents ( void )
    {
        _aryElements.DeleteAll();
    }
    HRESULT AppendElement ( CElement *pElement )
    {
        Assert(pElement);
        return _aryElements.Append ( pElement );
    }
    HRESULT InsertElementAt ( long lIndex, CElement *pElement )
    {
        HRESULT hr = _aryElements.Insert ( lIndex, pElement );
        Assert(pElement);
        if ( !hr && _lCurrentIndex >= lIndex ) // Retain currency 
            _lCurrentIndex++; 
        RRETURN(hr);
    }
    void DeleteAt ( long lIndex )
    {
        Assert ( lIndex >=0 && lIndex < _aryElements.Size() );
        _aryElements.Delete ( lIndex );
        if ( _lCurrentIndex > lIndex ) // Retain currency
            _lCurrentIndex--;
    }
    void CopyAry ( CElementAryCacheItem *pItem )
    {
        Assert(pItem);
        _aryElements.Copy (pItem->_aryElements, FALSE);
    }
};  

enum RETCOLLECT_KIND {RETCOLLECT_ALL, RETCOLLECT_FIRSTITEM, RETCOLLECT_LASTITEM};

class CDoc;
class CAtomTable;
class CElement;
class CCollectionCache
{
    struct CacheItem {
        CollCacheType           Type;
        IDispatch *             pdisp;
        CCollectionCacheItem *  _pCacheItem;
        CStr                    cstrName; // Name if name-based
        RECT                    rectCellRange; // For cell range type collections
        CElement*               pElementBase;// Element if child collection
        short                   sIndex;      // Index of item that this depends.
        unsigned                fIdentity:1; // set when a collection is Identity with
                                             //  its container/base object
        unsigned                fOKToDelete:1;  // TRUE for collections that the cache cooks up 
                                                // FALSE when Base Obj provided this CPtrAry
        unsigned                fDontPromoteNames:1; // TRUE if we don't promote names from the object
        unsigned                fDontPromoteOrdinals:1; // TRUE if we don't promote ordinals from the object
        unsigned                fGetLastCollectionItem:1; // TRUE to fetch last item only in collection
        unsigned                fIsCaseSensitive:1;     // TRUE if item's name must be compared in case sensitive manner
        unsigned                fSettableNULL:1; // TRUE when collection[n]=NULL is valid. normally FALSE.
		unsigned				_fIsValid;	 //  TRUE if collection is valid (Used for custom inval)
        DISPID                  dispidMin;    // Offset to add/subtract
        DISPID                  dispidMax;    // Offset to add/subtract
        long                    _lCollectionVersion;
        void    Init ( void )
        {
            Type = CacheType_FreeEntry;
            fIdentity = FALSE;
            fOKToDelete = TRUE;
            fDontPromoteNames = FALSE;
            fDontPromoteOrdinals = FALSE;
            fGetLastCollectionItem = FALSE;
            fIsCaseSensitive = FALSE;
            fSettableNULL=FALSE;
            rectCellRange.right = -1;
            dispidMin = DISPID_COLLECTION_MIN;
            dispidMax = DISPID_COLLECTION_MAX;
            _lCollectionVersion = 0;
			_fIsValid = FALSE;
        }
    };

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCollectionCache))
    CCollectionCache(
            CBase * pBase,
            CDoc *pDoc,
            PFN_CVOID_ENSURE    pfnEnsure   = NULL,
            PFN_CVOID_CREATECOL pfnCreation = NULL,
            PFN_CVOID_REMOVEOBJECT pfnRemove   = NULL,
            PFN_CVOID_ADDNEWOBJECT pfnAddNewObject = NULL);
    ~CCollectionCache();

    HRESULT InitReservedCacheItems(long lReservedSize, long lFromIndex = 0,
                       long lIdentityIndex  = -1 );

    HRESULT InitCacheItem(long lCacheIndex, CCollectionCacheItem *pCacheItem);
    CCollectionCacheItem * GetCacheItem(int lIndex) { return _aryItems[lIndex]._pCacheItem; }

    // Methods for accessing the element array associated with an index.
    long    SizeAry(long lIndex)
                { return _aryItems[lIndex]._pCacheItem->Length(); }
    void    ResetAry(long lIndex)
                { _aryItems[lIndex]._pCacheItem->ResetContents(); }

    HRESULT UnloadContents();

    HRESULT SetIntoAry(long lIndex, CElement * pElem)
                { return _aryItems[lIndex]._pCacheItem->AppendElement(pElem); }
    void    DontPromoteNames(long lIndex)
                { _aryItems[lIndex].fDontPromoteNames = TRUE; }
    BOOL    CanPromoteNames(long lIndex)
                { return !_aryItems[lIndex].fDontPromoteNames;  }
    void    DontPromoteOrdinals(long lIndex)
                { _aryItems[lIndex].fDontPromoteOrdinals = TRUE; }
    BOOL    CanPromoteOrdinals(long lIndex)
                { return !_aryItems[lIndex].fDontPromoteOrdinals;  }
    // If set, always fetch the last item from a named collection
    void    AlwaysGetLastMatchingCollectionItem(long lIndex)
                { _aryItems[lIndex].fGetLastCollectionItem = TRUE; }
    // Do we want to get the last name that matches, rather than a collection ?
    BOOL    DoGetLastMatchedName (long lIndex)
                { return !!_aryItems[lIndex].fGetLastCollectionItem; }



    void    SetDISPIDRange(long lIndex, DISPID dispidMin, DISPID dispidMax)
                { 
                    _aryItems[lIndex].dispidMin = dispidMin;
                    _aryItems[lIndex].dispidMax = dispidMax;
                }
    BOOL    IsDISPIDInCollection ( long lIndex, DISPID dispidMember )
                {
                    return ( dispidMember >= _aryItems[lIndex].dispidMin && 
                        dispidMember <= _aryItems[lIndex].dispidMax );

                }
    DISPID  GetMinDISPID ( long lIndex ) { return _aryItems[lIndex].dispidMin; }
    DISPID  GetMaxDISPID ( long lIndex ) { return _aryItems[lIndex].dispidMax; }

    // Divide the range into 2 subranges with first one for names and
    //    ordianls
    // First half of the named range is used for case sensitive names, second
    //     half for not sensitive ones
    DISPID  GetNamedMemberMin ( long lIndex ) {
        return _aryItems[lIndex].dispidMin; }
    DISPID  GetNamedMemberMax ( long lIndex ) { 
        return (_aryItems[lIndex].dispidMin  + _aryItems[lIndex].dispidMax) / 2; }

    DISPID  GetSensitiveNamedMemberMin ( long lIndex ) {
        return GetNamedMemberMin(lIndex); }
    DISPID  GetSensitiveNamedMemberMax ( long lIndex ) { 
        return (3 * _aryItems[lIndex].dispidMin + _aryItems[lIndex].dispidMax) / 4; }

    DISPID  GetNotSensitiveNamedMemberMin ( long lIndex ) {
        return GetSensitiveNamedMemberMax(lIndex)+1; }
    DISPID  GetNotSensitiveNamedMemberMax ( long lIndex ) { 
        return GetNamedMemberMax(lIndex); }

    DISPID  GetOrdinalMemberMin ( long lIndex ) {
        return GetNamedMemberMax(lIndex)+1; }
    DISPID  GetOrdinalMemberMax ( long lIndex ) {
        return _aryItems[lIndex].dispidMax; }

    BOOL    IsNamedCollectionMember ( long lIndex, DISPID dispidMember )
                {
                    return ( dispidMember >= GetNamedMemberMin(lIndex) && 
                        dispidMember <= GetNamedMemberMax(lIndex) ) ;
                }
    BOOL    IsSensitiveNamedCollectionMember ( long lIndex, DISPID dispidMember )
                {
                    return ( dispidMember >= GetSensitiveNamedMemberMin(lIndex) && 
                        dispidMember <= GetSensitiveNamedMemberMax(lIndex) ) ;
                }
    BOOL    IsNotSensitiveNamedCollectionMember ( long lIndex, DISPID dispidMember )
                {
                    return ( dispidMember >= GetNotSensitiveNamedMemberMin(lIndex) && 
                        dispidMember <= GetNotSensitiveNamedMemberMax(lIndex) ) ;
                }
    BOOL    IsOrdinalCollectionMember ( long lIndex, DISPID dispidMember )
                {
                    return ( dispidMember >= GetOrdinalMemberMin(lIndex) && 
                        dispidMember <= GetOrdinalMemberMax(lIndex) ) ;
                }

    long    GetNamedMemberOffset(long lCollection, DISPID dispid, BOOL *pfCaseSensitive =  NULL);
   
    HRESULT SetIntoAry(long lIndex, long lIndexElement, CElement * pElem)
                { return _aryItems[lIndex]._pCacheItem->InsertElementAt(lIndexElement, pElem); }
    void    RemoveFromAry(long lIndex, long lIndexElement)
                { _aryItems[lIndex]._pCacheItem->DeleteAt(lIndexElement); }

    HRESULT GetIntoAry(long lIndex, long lIndexElement, CElement * * ppElem);
    HRESULT GetIntoAry(long lIndex, CElement * pElem, long * plIndexElement);

    HRESULT GetIntoAry(long lIndex, LPCTSTR Name, BOOL fTagName, CElement * * ppElem, 
        long iStartFrom=0, BOOL fCaseSensitive = FALSE);
    HRESULT EnsureAry(long lIndex);

    // Methods to manipulate the smart collection cache item versions
    void InvalidateItem ( long lIndex ) { _aryItems[lIndex]._fIsValid = FALSE; }
	BOOL IsValidItem ( long lIndex ) { return !!_aryItems[lIndex]._fIsValid; }
	void ValidateItem ( long lIndex ) { _aryItems[lIndex]._fIsValid = TRUE; }

    long GetVersion ( long lIndex ) { return _aryItems[lIndex]._lCollectionVersion; }

	// Only call this function if your collections are using the _fIsValid technique
    void InvalidateAllSmartCollections ( void );
	// Otherwise call this one.
	void Invalidate ( void );


    // Methods for accessing the dispatch associated with an index.
    HRESULT ClearDisp(long lIndex);
    HRESULT GetDisp(long lIndex, IDispatch ** ppdisp);
    HRESULT GetDisp(long lIndex, LPCTSTR Name, CollCacheType CacheType, IDispatch ** ppdisp, BOOL fCaseSensitive, RECT *pRange = NULL, BOOL fAlwaysCollection = FALSE);
    HRESULT GetDisp(long lIndex, long lIndexElement, IDispatch ** ppdisp);
    HRESULT GetDisp(long lIndex, LPCTSTR Name, long lIndexElement, IDispatch ** ppdisp, BOOL fCaseSensitive);
    HRESULT CreateChildrenCollection(long lIndex, CElement *pElement, IDispatch **ppDisp, BOOL fAll, BOOL fDOMCollection = FALSE);
    HRESULT GetElemAt(long lCollection, long *plCurrIndex, IDispatch **ppCurrElem);

    // IElementCollection Method helpers 
    HRESULT STDMETHODCALLTYPE GetLength(long lIndex, long*plength);
    HRESULT STDMETHODCALLTYPE GetNewEnum(long lIndex, IUnknown**p_newEnum);
    HRESULT STDMETHODCALLTYPE Tags(long lIndex, VARIANT tagName,IDispatch**ptags);
    HRESULT STDMETHODCALLTYPE Urns(long lIndex, VARIANT urn, IDispatch**ptags);
    HRESULT STDMETHODCALLTYPE Remove(long lCollectionIndex, long index);
    HRESULT STDMETHODCALLTYPE Item(long lIndex, VARIANT name,VARIANT index,IDispatch** ppResult);

    // Some helper and administrative methods:
    HRESULT Tags(long lIndex, LPCTSTR szTagName, IDispatch**ptags);
    HRESULT CloseErrorInfo(HRESULT hr)
                { return _pBase->CloseErrorInfo(hr); }

    CBase * GetBase()
           { return _pBase; }

    long GetNumCollections()
           { return _aryItems.Size(); }

    void ReleaseNameDependency(long lIndex);
    HRESULT CreateCollectionHelper(IDispatch ** ppIEC, long lIndex);

    // IDispatch helpers
    HRESULT GetIDsOfNames(
          long              lIndex,
          REFIID            riid,
          LPTSTR          * rgszNames,
          UINT              cNames,
          LCID              lcid,
          DISPID FAR      * rgdispid);
    HRESULT Invoke(
          long              lIndex,
          DISPID            dispidMember,
          REFIID            riid,
          LCID              lcid,
          WORD              wFlags,
          DISPPARAMS FAR  * pdispparams,
          VARIANT FAR     * pvarResult,
          EXCEPINFO FAR   * pexcepinfo,
          UINT FAR        * puArgErr,
          RETCOLLECT_KIND   returnCollection = RETCOLLECT_ALL );

    // The IDispatchEx helpers:
    HRESULT GetDispID(long lIndex, BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT STDMETHODCALLTYPE InvokeEx(
        DISPID dispidMember,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider);


    HRESULT GetMemberProperties(
                long lIndex,
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    HRESULT GetNextDispID(
                long lIndex,
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);

    HRESULT GetMemberName(
                long lIndex,
                DISPID id,
                BSTR *pbstrName);

    HRESULT GetNameSpaceParent(IUnknown **ppunk);

    CAtomTable * GetAtomTable (BOOL *pfExpando = NULL);

    CDoc * Doc() 
        { return _pDoc; }

    BOOL GetCollectionSetableNULL(long lIndex)
    {
        return _aryItems[lIndex].fSettableNULL;
    }

    void SetCollectionSetableNULL(long lIndex, BOOL fSet)
    {
        Assert(lIndex < _aryItems.Size());
        _aryItems[lIndex].fSettableNULL = fSet;
    }

    HRESULT BuildChildArray( 
        long lCollectionIndex, 
        CElement* pRootElement,
        CCollectionCacheItem *pIntoCacheItem,
        BOOL fAll );

    HRESULT BuildNamedArray(
            long lIndex,
            LPCTSTR Name,
            BOOL fTagName,
            CCollectionCacheItem *pIntoCacheItem,
            long iStartFrom ,
            BOOL fCaseSensitive,
            BOOL fUrn = FALSE);

    HRESULT BuildCellRangeArray(long lCollectionIndex, LPCTSTR szRange, 
                             RECT *pRect,  CCollectionCacheItem *pIntoCacheItem );

private:
    HRESULT GetFreeIndex(long * plIndex);
    BOOL    CompareName(CElement * pElement,LPCTSTR Name, BOOL fTagName, BOOL fCaseSensitive = FALSE);

    CDataAry<CacheItem>     _aryItems;
    long                    _lReservedSize;
    CBase *                 _pBase;
    PFN_CVOID_ENSURE        _pfnEnsure;
    PFN_CVOID_REMOVEOBJECT  _pfnRemoveObject; 
    PFN_CVOID_CREATECOL     _pfnCreateCollection;
    PFN_CVOID_ADDNEWOBJECT  _pfnAddNewObject;
    CDoc *                  _pDoc;                  // The document with the correct Atom Table
};

//=============================================================
//
//  Class CElementCollectionBase
//
//=============================================================
class CElementCollectionBase :  public CBase,
                                public IDispatchEx
{
    typedef CBase super;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:

    CElementCollectionBase(CCollectionCache * pCollectionCache, long lIndex);
    ~CElementCollectionBase();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CElementCollectionBase);

    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IDispatchEx methods
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT STDMETHODCALLTYPE InvokeEx(
        DISPID dispidMember,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);

    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    HRESULT STDMETHODCALLTYPE GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    STDMETHOD(GetMemberName) (DISPID id,
                              BSTR *pbstrName);

    HRESULT STDMETHODCALLTYPE GetNextDispID(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);

    HRESULT STDMETHODCALLTYPE GetNameSpaceParent(IUnknown **ppunk);

    // Override CBase GetAtomTable.
    CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { return _pCollectionCache->GetAtomTable(pfExpando); }


protected:
    DECLARE_CLASSDESC_MEMBERS;

    virtual HRESULT CloseErrorInfo(HRESULT hr);

    CCollectionCache  * _pCollectionCache;
    long                _lIndex;
};

//=============================================================
//
//  Class CElementCollection
//
//=============================================================
class CElementCollection :  public CElementCollectionBase,
                            public IHTMLElementCollection
{
    typedef CElementCollectionBase super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CElementCollection))

    CElementCollection(CCollectionCache *pCollectionCache, long lIndex) :
        super(pCollectionCache, lIndex) {}
    ~CElementCollection();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CElementCollectionBase);
    DECLARE_DERIVED_DISPATCH(CElementCollectionBase);

    DECLARE_PRIVATE_QI_FUNCS(super)

    HRESULT Tags(LPCTSTR szTagName, IDispatch**ptags);

    // IHTMLElementCollection

    #define _CElementCollection_
    #include "collect.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};


////////////////////////////////////////////////////////////////////////////////
//
//  Automation helper routines used by collection classes.
//
////////////////////////////////////////////////////////////////////////////////

typedef HRESULT (STDMETHODCALLTYPE CBase::*InvokeExPROC)(DISPID,
                                                    LCID,
                                                    WORD,
                                                    DISPPARAMS *,
                                                    VARIANT *,
                                                    EXCEPINFO *,
                                                    IServiceProvider *);

typedef HRESULT (STDMETHODCALLTYPE  CBase::*GetDispIDPROC)(BSTR,
                                                           DWORD,
                                                           DISPID *);

typedef HRESULT (STDMETHODCALLTYPE  CBase::*GetNextDispIDPROC)(DWORD,
                                                            DISPID,
                                                            DISPID *);

typedef HRESULT (STDMETHODCALLTYPE  CBase::*GetGetMemberNamePROC)(DISPID,
                                                                  BSTR *);

HRESULT
DispatchInvokeCollection(CBase *             pThis,
                         InvokeExPROC        SuperInvokeFunction,
                         CCollectionCache *  pCollectionCache,
                         long                lCollectionIdx,
                         DISPID              dispidMember,
                         REFIID              riid,
                         LCID                lcid,
                         WORD                wFlags,
                         DISPPARAMS *        pdispparams,
                         VARIANT *           pvarResult,
                         EXCEPINFO *         pexcepinfo,
                         UINT *              puArgErr,
                         IServiceProvider *  pSrvProvider,
                         RETCOLLECT_KIND     returnCollection = RETCOLLECT_ALL);

HRESULT
DispatchGetDispIDCollection(CBase *             pThis,
                            GetDispIDPROC       SuperGetDispIDFunction,
                            CCollectionCache *  pCollectionCache,
                            long                lCollectionIdx,
                            BSTR                bstrName,
                            DWORD               grfdex,
                            DISPID    *         pid);

HRESULT
DispatchGetNextDispIDCollection(CBase *             pThis,
                                GetNextDispIDPROC   SuperGetNextDispIDFunction,
                                CCollectionCache *  pCollectionCache,
                                long                lCollectionIdx,
                                DWORD               grfdex,
                                DISPID              id,
                                DISPID *            pid);

HRESULT
DispatchGetMemberNameCollection(CBase *                 pThis,
                                GetGetMemberNamePROC    SuperGetMemberNameFunction,
                                CCollectionCache *      pCollectionCache,
                                long                    lCollectionIdx,
                                DISPID                  id,
                                BSTR *                  pbstrName);

#pragma INCMSG("--- End 'collect.hxx'")
#else
#pragma INCMSG("*** Dup 'collect.hxx'")
#endif
