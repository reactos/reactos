//=================================================================
//
//   File:      filtcol.hxx
//
//  Contents:   CFilterCollection class
//
//  Classes:    CFilterArray
//
//=================================================================

#ifndef I_FILTCOL_HXX_
#define I_FILTCOL_HXX_
#pragma INCMSG("--- Beg 'filtcol.hxx'")

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#pragma INCMSG("--- Beg <mshtmhst.h>")
#include <mshtmhst.h>
#pragma INCMSG("--- End <mshtmhst.h>")
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#define _hxx_
#include "filter.hdl"

MtExtern(CCSSFilterSite)
MtExtern(CFilterArray)
MtExtern(CFilterArray_aryFilters_pv)

class CPropertyBag;

//+----------------------------------------------------------------------------
//
//  Class:      CCSSFilterSite
//
//   Note:      CSS Extension object site which hangs off element 
//
//-----------------------------------------------------------------------------
class CCSSFilterSite : public CBase,
                       public ICSSFilterSite, 
                       public IServiceProvider
{
    friend class CElement;
    DECLARE_CLASS_TYPES(CCSSFilterSite, CBase)
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCSSFilterSite))

    CCSSFilterSite( CElement * pElem, LPCTSTR pName );
    ~CCSSFilterSite();

    #define _CCSSFilterSite_
    #include "filter.hdl"

    DECLARE_PLAIN_IUNKNOWN(CCSSFilterSite);
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    DECLARE_TEAROFF_TABLE(IBindHost)

    // helper functions
    ICSSFilter * GetExtension() 
        {return _pFilter; } 
    LPCTSTR      GetFilterName() 
        {return _cstrFilterName; }
    HRESULT      Connect( LPTSTR pNameValues );
    BOOL         SafeToScript() {return _fSafeToScript;}

    // ICSSFilterSite declarations
    STDMETHODIMP GetElement( IHTMLElement **ppElement );
    STDMETHODIMP FireOnFilterChangeEvent( );

    // IServiceProvider interface
    STDMETHODIMP QueryService(REFGUID, REFIID, void **ppv);

    // IDispatchEx methods
    STDMETHODIMP InvokeEx(DISPID dispidMember,
                             LCID lcid,
                             WORD wFlags,
                             DISPPARAMS * pdispparams,
                             VARIANT * pvarResult,
                             EXCEPINFO * pexcepinfo,
                             IServiceProvider *pSrvProvider);

    //
    // IBindHost Methods
    //

    NV_DECLARE_TEAROFF_METHOD(CreateMoniker, createmoniker,
        (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD(MonikerBindToStorage, monikerbindtostorage, 
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));
    NV_DECLARE_TEAROFF_METHOD(MonikerBindToObject, monikerbindtoobject,
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));

protected:
    DECLARE_CLASSDESC_MEMBERS;

    HRESULT ParseFilterNameValuePair(LPTSTR pchNameValue, CPropertyBag **ppPropBag);

private:
    // members
    CElement *     _pElem;
    ICSSFilter   * _pFilter;
    CStr           _cstrFilterName;
    BOOL           _fSafeToScript;
};



//+------------------------------------------------------------
//
//  Class : CFilterArray
//
//-------------------------------------------------------------

class CFilterArray :public CCollectionBase
{
    DECLARE_CLASS_TYPES(CFilterArray, CCollectionBase);

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFilterArray))

    CFilterArray(CElement * pOwner, LPCTSTR pText=NULL);
    ~CFilterArray();

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CFilterArray);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    void Passivate();

    // Necessary to support expandos on a collection.
    CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        { return _pElemOwner->GetAtomTable(pfExpando); }


    #define _CFilterArray_
    #include "filter.hdl"

    // helper methods
    long    AddFilterSite(CCSSFilterSite * pFilter);
    HRESULT RemoveFilterSite(long lIndex);

    LPCTSTR GetFullText() 
        {return _strFullText; };
    HRESULT SetFullText(LPCTSTR pText) 
        { RRETURN ( _strFullText.Set(pText) );  };

    BOOL IsFilterDispID (DISPID dispidMember);
    HRESULT ParseFilterProperty(LPCTSTR pcszFilter, CTreeNode * pNodeOwner);
    HRESULT OnAmbientPropertyChange ( DISPID dspID );
    HRESULT OnCommand ( COnCommandExecParams * pParm );
    void EnsureCollection ( void );


protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    virtual long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE );
    virtual LPCTSTR GetName(long lIdx);
    virtual HRESULT GetItem( long lIndex, VARIANT *pvar );

    DECLARE_CPtrAry(CAryFilters, CCSSFilterSite *, Mt(Mem), Mt(CFilterArray_aryFilters_pv))
    CAryFilters                 _aryFilters;
    CElement                   *_pElemOwner;
    CStr                        _strFullText;
};

#pragma INCMSG("--- End 'filtcol.hxx'")
#else
#pragma INCMSG("*** Dup 'filtcol.hxx'")
#endif
