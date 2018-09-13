/*
 * @(#)provideclassinfo.cxx 1.0 9/2/98
 * 
 * Implementation of IProvideClassInfo tearoff
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "provideclassinfo.hxx"

///////////////////////////////////
// IProvideClassInfo functions
///////////////////////////////////

ProvideClassInfo::ProvideClassInfo(
    IUnknown *punk, 
    REFIID libid, 
    REFIID coclass) : _libid(libid), _co_class(coclass)
{
    Assert (punk && "Must pass in an outer unknown");
    _punk = punk;
    punk->AddRef();
    _cRef = 1;
}

HRESULT 
STDMETHODCALLTYPE 
ProvideClassInfo::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    if (NULL == ppvObject)
        return E_POINTER;
    
    if (IsEqualIID(riid, IID_IProvideClassInfo) || IsEqualIID(riid, IID_IUnknown))
        *ppvObject = (LPVOID *)(IProvideClassInfo *) this;
    else
        return _punk->QueryInterface(riid, ppvObject);
    
    AddRef();
    return S_OK;
}

ULONG 
STDMETHODCALLTYPE 
ProvideClassInfo::AddRef()
{
    InterlockedIncrement((LPLONG)&_cRef);
    return _punk->AddRef();
}

ULONG 
STDMETHODCALLTYPE 
ProvideClassInfo::Release()
{
    Assert (_cRef > 0 && "Trying to release when refcount is already 0");
    InterlockedDecrement((LPLONG)&_cRef);
    
    if (0 == _cRef)
    {
        ULONG cTemp = _punk->Release();
        delete this;
        return cTemp;
    }
    
    return _punk->Release();
    
}

HRESULT 
STDMETHODCALLTYPE 
ProvideClassInfo::GetClassInfo(ITypeInfo **ppTI)
{
    if (NULL == ppTI)
        return E_POINTER;
    else
        return ::GetTypeInfo(_libid, 1, LANG_NEUTRAL, _co_class, ppTI);
}




