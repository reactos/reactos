#ifndef I_DOMCOLL_HXX_
#define I_DOMCOLL_HXX_
#pragma INCMSG("--- Beg 'domcoll.hxx'")


#define _hxx_
#include "domcoll.hdl"

MtExtern(CAttrCollectionator)
MtExtern(CAttrCollectionator_aryAttributes_pv)
MtExtern(CDOMChildrenCollection)

DECLARE_CPtrAry(CAryAttributes, IDispatch *, Mt(Mem), Mt(CAttrCollectionator_aryAttributes_pv))


class CDOMChildrenCollection : public CCollectionBase 
{
    DECLARE_CLASS_TYPES(CDOMChildrenCollection, CCollectionBase)
private:
    CBase *_pOwner;
    BOOL  _fIsElement;
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDOMChildrenCollection))

    CDOMChildrenCollection( CBase *pOwner, BOOL fElement ){ 
        _pOwner = pOwner; _pOwner->PrivateAddRef(); _fIsElement = fElement; }
    ~CDOMChildrenCollection(){ 
        _pOwner->FindAAIndexAndDelete ( DISPID_INTERNAL_CDOMCHILDRENPTRCACHE, 
            CAttrValue::AA_Internal );
        _pOwner->PrivateRelease(); 
    };

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CDOMChildrenCollection);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    //STDMETHOD(QueryInterface)(REFIID riid, void **ppv) { return PrivateQueryInterface(riid,ppv); }
    //STDMETHOD_(ULONG,AddRef)(void) { return _pOwner->AddRef(); }
    //STDMETHOD_(ULONG,Release)(void) { return _pOwner->Release(); }

    #define _CDOMChildrenCollection_
    #include "domcoll.hdl"

    // Helpers
    HRESULT IsValidIndex ( long lIndex );
    long GetLength ( void );

    // CGenericOMCollection over-rides
    long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE ) { return -1; }
    LPCTSTR GetName(long lIdx) {return NULL;} // Don't support names - use index as name
    HRESULT GetItem (long lIndex, VARIANT *pvar);


protected:
    DECLARE_CLASSDESC_MEMBERS;
};


//+------------------------------------------------------------
//
//  Class : CAttrCollectionator
//
//-------------------------------------------------------------

class CAttrCollectionator : public CCollectionBase
{
public:
    DECLARE_CLASS_TYPES(CAttrCollectionator, CCollectionBase)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAttrCollectionator))

    CAttrCollectionator(CElement *pElemColl) { _pElemColl = pElemColl; pElemColl->AddRef(); }
    ~CAttrCollectionator();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CAttrCollectionator)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    HRESULT EnsureCollection();

    #define _CAttrCollectionator_
    #include "domcoll.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    virtual long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE );
    virtual LPCTSTR GetName(long lIdx);
    virtual HRESULT GetItem(long lIndex, VARIANT *pvar);

    CAryAttributes  _aryAttributes;
    CElement       *_pElemColl;

};

#pragma INCMSG("--- End 'domcoll.hxx'")
#else
#pragma INCMSG("*** Dup 'domcoll.hxx'")
#endif
