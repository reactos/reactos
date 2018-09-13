//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padole.cxx
//
//  Contents:   CPadDoc IOleObject implementation.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

HRESULT
CPadDoc::SetClientSite(IOleClientSite *)
{
    RRETURN(E_NOTIMPL);
}
       
HRESULT
CPadDoc::GetClientSite(IOleClientSite **ppClientSite)
{
    *ppClientSite = NULL;
    return S_OK;
}
      
HRESULT
CPadDoc::SetHostNames(LPCOLESTR, LPCOLESTR)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::Close(DWORD)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::SetMoniker(DWORD , IMoniker *)
{
    RRETURN(E_NOTIMPL);
}
  
HRESULT
CPadDoc::GetMoniker(DWORD, DWORD, IMoniker **)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::InitFromData(IDataObject *, BOOL, DWORD)
{
    RRETURN(E_NOTIMPL);
}
        
HRESULT
CPadDoc::GetClipboardData(DWORD, IDataObject **)
{
    RRETURN(E_NOTIMPL);
}
       
HRESULT
CPadDoc::DoVerb(LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCOLERECT)
{
    RRETURN(E_NOTIMPL);
}
    
HRESULT
CPadDoc::EnumVerbs(IEnumOLEVERB **)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::Update()
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::IsUpToDate()
{
    RRETURN(E_NOTIMPL);
}
     
HRESULT
CPadDoc::GetUserClassID(CLSID *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::GetUserType(DWORD, LPOLESTR *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::SetExtent(DWORD, SIZEL *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::GetExtent(DWORD, SIZEL *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::Advise(IAdviseSink *, DWORD *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::Unadvise(DWORD)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::EnumAdvise(IEnumSTATDATA **)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::GetMiscStatus(DWORD, DWORD *)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::SetColorScheme(LOGPALETTE  *pLogpal)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::ParseDisplayName(IBindCtx *, LPOLESTR, ULONG *, IMoniker **)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::EnumObjects(DWORD, IEnumUnknown **)
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CPadDoc::LockContainer(BOOL)
{
    return S_OK;
}
