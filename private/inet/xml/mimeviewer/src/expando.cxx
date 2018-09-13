/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <dispex.h>
#include "utils.hxx"
#include "expando.hxx"

HRESULT 
AddDOCExpandoProperty(
    BSTR bstrAttribName,
    IHTMLDocument2 *pDoc,
    IDispatch *pDispToAdd)
{
    HRESULT hr;
    VARIANT_BOOL vbExpando;
    VARIANT var;     
    DISPID dispid, putid;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    IDispatchEx *pIEx = NULL;

    hr = pDoc->get_expando(&vbExpando);
    CHECKHR(hr);

    // On change the state of the expando property if necessary
    if (VARIANT_TRUE != vbExpando)
    {
        hr = pDoc->put_expando(VARIANT_TRUE);
        CHECKHR(hr);
    }

    hr = pDoc->QueryInterface(IID_IDispatchEx, (void **)&pIEx);
    CHECKHR(hr);

    // Create new dispid in object

    hr = pIEx->GetDispID(bstrAttribName, fdexNameEnsure, &dispid);
    CHECKHR(hr);
      
    // now mark it as a dispatch and put the value
    // trident wants PUT, not PUTREF or it craps out (??)
    putid = DISPID_PROPERTYPUT;
    var.vt = VT_DISPATCH;
    var.pdispVal = pDispToAdd;
    dispparams.rgvarg = &var;
    dispparams.rgdispidNamedArgs = &putid;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;
    hr = pIEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    CHECKHR(hr);

    // Reset the state of the expando property
    if (VARIANT_TRUE != vbExpando)
    {
        hr = pDoc->put_expando(VARIANT_FALSE);
        CHECKHR(hr);
    }

CleanUp:
    SafeRelease(pIEx);
    return hr;
}  // AddExpandoProperty


/////////////////////////////////////////////////////////////////////////////////////////

DISPATCHINFO ExpandoDocument::s_dispatchinfoEXP = 
{
    NULL, &IID_IXMLDOMDocument, &LIBID_MSXML, ORD_MSXML, NULL, 0, NULL, 0, NULL
};


ExpandoDocument::ExpandoDocument(BSTR bURL)
{
    _bURL = ::SysAllocString(bURL);
    _refcount = 1;
    ::IncrementComponents();
}

ExpandoDocument::~ExpandoDocument()
{
    ::SysFreeString(_bURL);
    ::DecrementComponents();
}

//////////////////////////////////////////////
////  IUnknown methods

HRESULT STDMETHODCALLTYPE 
ExpandoDocument::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IDispatch || riid == IID_IXMLDOMDocument || riid == IID_IXMLDOMDocument)
    {
        *ppv = this; // assumes vtable layout is definition order
    }
    else if (riid == IID_ISupportErrorInfo)
    {
        *ppv = static_cast<ISupportErrorInfo *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE 
ExpandoDocument::AddRef()
{
    return InterlockedIncrement(&_refcount);
}

ULONG STDMETHODCALLTYPE 
ExpandoDocument::Release()
{
    if (InterlockedDecrement(&_refcount) == 0)
    {
        delete this;
        return 0;
    }
    return _refcount;
}

//////////////////////////////////////////////
//// IDispatch methods

HRESULT STDMETHODCALLTYPE 
ExpandoDocument::GetTypeInfoCount( 
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    return _dispatchImpl::GetTypeInfoCount(&s_dispatchinfoEXP, pctinfo);
}
    
HRESULT STDMETHODCALLTYPE 
ExpandoDocument::GetTypeInfo( 
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return _dispatchImpl::GetTypeInfo(&s_dispatchinfoEXP, iTInfo, lcid, ppTInfo); 
}
    
HRESULT STDMETHODCALLTYPE 
ExpandoDocument::GetIDsOfNames( 
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    return _dispatchImpl::GetIDsOfNames(&s_dispatchinfoEXP, riid, rgszNames, cNames, lcid, rgDispId);
}
    
HRESULT STDMETHODCALLTYPE 
ExpandoDocument::Invoke( 
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    return _dispatchImpl::Invoke(&s_dispatchinfoEXP, this,
                                      dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

//////////////////////////////////////////////
//// ISupportErrorInfo

HRESULT STDMETHODCALLTYPE 
ExpandoDocument::InterfaceSupportsErrorInfo(REFIID riid)
{
    if (riid == IID_IXMLDOMDocument)
        return S_OK;
    return S_FALSE;    

}       

void 
ExpandoDocument::setErrorInfo(WCHAR * szDescription)
{
    _dispatchImpl::setErrorInfo(szDescription);
}
    

HRESULT STDMETHODCALLTYPE 
ExpandoDocument::get_url( 
    /* [out][retval] */ BSTR __RPC_FAR *urlString)
{
    Assert(_bURL);
    *urlString = ::SysAllocString(_bURL);
    return (*urlString ? S_OK : E_OUTOFMEMORY);
}
                
 
