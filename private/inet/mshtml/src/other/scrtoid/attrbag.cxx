#include <headers.hxx>

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_ATTRBAG_HXX_
#define X_ATTRBAG_HXX_
#include "attrbag.hxx"
#endif

#pragma warning(disable:4100)

typedef int (__stdcall *PFNSTRCMP)(LPCTSTR, LPCTSTR);


CAttrBag::CAttrBag() :
 _cRef( 1 ),
 _pAllArgs( NULL ),
 _pAllNames( NULL ),
 _uNumArgs( 0 )
{
}

CAttrBag::~CAttrBag()
{
    if ( _pAllArgs )
    {
        delete [] _pAllArgs;
        _pAllArgs = NULL;
    }

    if ( _pAllNames )
    {
        ULONG i;
        for ( i = 0 ; i < _uNumArgs ; ++i )
            CoTaskMemFree( _pAllNames[i] );
        delete [] _pAllNames;
        _pAllNames = NULL;
    }
}

STDMETHODIMP CAttrBag::QueryInterface(REFIID riid, void** ppv)
{
    Assert( ppv );

    *ppv = NULL;

    if ( riid==IID_IUnknown ||
         riid==IID_IDispatch ||
         riid == IID_IDispatchEx )
    {
        *ppv = (IDispatchEx*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAttrBag::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CAttrBag::Release() 
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }
    
    return _cRef;
}

STDMETHODIMP CAttrBag::GetTypeInfoCount(UINT * /* pctinfo */)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAttrBag::GetTypeInfo(UINT /* iTInfo */, 
                                   LCID /* lcid */,
                                   ITypeInfo ** /* ppTInfo */)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP CAttrBag::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                     UINT cNames, LCID lcid,
                                     DISPID *rgDispId)
{
    HRESULT hr;
    
    Assert(cNames == 1);

    if ( !rgszNames || !rgDispId )
        return E_POINTER;

    BSTR bstrName = SysAllocString( rgszNames[0] );
    if ( bstrName )
    {
        hr = GetDispID( bstrName, 0, rgDispId );
        SysFreeString( bstrName );
    }
    else 
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
    
STDMETHODIMP CAttrBag::Invoke( 
                     /* [in] */ DISPID dispIdMember,
                     /* [in] */ REFIID riid,
                     /* [in] */ LCID lcid,
                     /* [in] */ WORD wFlags,
                     /* [out][in] */ DISPPARAMS *pDispParams,
                     /* [out] */ VARIANT * pVarResult,
                     /* [out] */ EXCEPINFO * pExcepInfo,
                     /* [out] */ UINT * /* puArgErr */)
{
    return InvokeEx( dispIdMember, lcid, wFlags,
                     pDispParams, pVarResult, pExcepInfo, NULL );
}

STDMETHODIMP
CAttrBag::GetDispID( BSTR bstrName, DWORD grfdex, DISPID *pid )
{
    ULONG i;
    STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    if ( !pid )
        return E_POINTER;

    for ( i=0 ; i < _uNumArgs ; ++i )
    {
        if ( !pfnStrCmp( _pAllNames[i], bstrName ) )
        {
            *pid = i+1;
            return S_OK;
        }
    }

    *pid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}
        
STDMETHODIMP
CAttrBag::InvokeEx( DISPID id, LCID lcid, WORD wFlags,
                    DISPPARAMS *pdp, VARIANT *pvarRes,
                    EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HRESULT hr = S_OK;

    // BUGBUG: need to test invoke flags
    if ( id == DISPID_UNKNOWN )
    {
        pvarRes->vt = VT_NULL;
        return S_OK;
    }
    else if ( id > 0 && id <= (DISPID)_uNumArgs )
    {
        if ( pvarRes && wFlags & DISPATCH_PROPERTYGET)
        {
            VariantInit( pvarRes );
            hr = VariantCopy( pvarRes, _pAllArgs+(id-1) );
            return hr;
        }
    }

    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP
CAttrBag::DeleteMemberByName( /* [in] */ BSTR bstr,
                              /* [in] */ DWORD grfdex)
{
    return S_OK;
}
        
STDMETHODIMP
CAttrBag::DeleteMemberByDispID( /* [in] */ DISPID id)
{
    return S_OK;
}
        
STDMETHODIMP
CAttrBag::GetMemberProperties( /* [in] */ DISPID id,
                                 /* [in] */ DWORD grfdexFetch,
                                 /* [out] */ DWORD *pgrfdex )
{
    HRESULT hr;

    if ( !pgrfdex )
        return E_POINTER;

    if ( id > 0 && id <= (DISPID)_uNumArgs )
    {
        *pgrfdex = fdexPropCanGet |
                   fdexPropCannotPut |
                   fdexPropCannotPutRef |
                   fdexPropNoSideEffects;
        hr = S_OK;
    }
    else
    {
        *pgrfdex = 0;
        hr = DISP_E_MEMBERNOTFOUND;
    }

    *pgrfdex &= grfdexFetch;

    return hr;

/*
BUGBUG: Need to handle the following.

fdexPropCanGet
fdexPropCannotGet
fdexPropCanPut
fdexPropCannotPut
fdexPropCanPutRef
fdexPropCannotPutRef
fdexPropNoSideEffects
fdexPropDynamicType
fdexPropCanCall
fdexPropCannotCall
fdexPropCanConstruct
fdexPropCannotConstruct
fdexPropCanSourceEvents
fdexPropCannotSourceEvents
*/
}
        
STDMETHODIMP
CAttrBag::GetMemberName( /* [in] */ DISPID id,
                         /* [out] */ BSTR *pbstrName)
{
    // Caller will free pbstrName
    HRESULT hr;
    
    if ( !pbstrName )
        return E_POINTER;
        
    if ( id > 0 && id <= (DISPID)_uNumArgs )
    {
        *pbstrName = SysAllocString( _pAllNames[(id-1)] );
        hr = S_OK;
    }
    else
    {
        *pbstrName = NULL;
        hr = DISP_E_MEMBERNOTFOUND;
    }
    
    return hr;
}
        
STDMETHODIMP
CAttrBag::GetNextDispID( /* [in] */ DWORD grfdex,
                                  /* [in] */ DISPID id,
                                  /* [out] */ DISPID *pid)
{
    if ( !pid )
        return E_POINTER;

    if ( id == DISPID_STARTENUM || id < 0 )
        *pid = 0;
    else
        *pid = id;

    (*pid)++;

    if ( (ULONG)(*pid) <= _uNumArgs )
        return S_OK;

    return S_FALSE;
}
        
STDMETHODIMP
CAttrBag::GetNameSpaceParent( /* [out] */ IUnknown **ppunk)
{
    if ( !ppunk )
        return E_POINTER;

    *ppunk = NULL;
    return S_OK;
}

STDMETHODIMP
CAttrBag::Load( IPropertyBag *pBag, IPropertyBag2 *pBag2 )
{
    HRESULT hr = S_OK;

    ULONG uProps, uGot, i;
    PROPBAG2 *pBagInfo = NULL;

    hr = pBag2->CountProperties( &uProps );
    if ( hr )
        goto Error;
    
    pBagInfo = new PROPBAG2[uProps];
    if ( !pBagInfo )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = pBag2->GetPropertyInfo ( 0, uProps, pBagInfo, &uGot );
    if ( hr )
        goto Error;

    _uNumArgs = uProps;
    _pAllArgs = new VARIANT[uProps];
    if ( !_pAllArgs )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // Init all our variants (Some property bag impls require this)
    for (i=0 ; i < uProps ; ++i)
    {
        VariantInit(_pAllArgs+i);
    }

    _pAllNames = new LPTSTR[uProps];
    if ( !_pAllNames )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    for ( i=0 ; i < uProps ; ++i )
    {
        pBag->Read( pBagInfo[i].pstrName, _pAllArgs+i, NULL);
        _pAllNames[i] = pBagInfo[i].pstrName;
    }

Cleanup:
    if ( pBagInfo )
        delete [] pBagInfo;

    return hr;

Error:
    if ( _pAllArgs ) {
        delete [] _pAllArgs;
        _pAllArgs = NULL;
    }
    if ( _pAllNames ) {
        delete [] _pAllNames;
        _pAllNames = NULL;
    }
    goto Cleanup;
}
