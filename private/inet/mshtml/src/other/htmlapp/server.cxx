//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       server.cxx
//
//  Contents:   implementation of server object
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SERVER_HXX_
#define X_SERVER_HXX_
#include "server.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#include "misc.hxx"
#endif

CServerObject::CServerObject(CHTMLApp *pApp)
    : _pApp(pApp), _ulRefs(0)
{
}

HRESULT CServerObject::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IPersistMoniker)
    {
        *ppv = (IPersistMoniker *)this;
    }
    else if (riid == IID_IOleObject)
    {
        *ppv = (IOleObject *)this;
    }

    if (*ppv)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT CServerObject::GetClassID(CLSID *pClsid)
{
    return CLSIDFromString(SZ_SERVER_CLSID, pClsid);
}

HRESULT CServerObject::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT CServerObject::Load(BOOL fFullyAvailable, IMoniker * pmk, LPBC pbc, DWORD grfMode)
{
    HRESULT hr = THR(_pApp->RunHTMLApplication(pmk));
    RRETURN(hr);
}

HRESULT CServerObject::Save(IMoniker * pmk, LPBC pbc, BOOL fRemember)
{
    return E_NOTIMPL;
}

HRESULT CServerObject::SaveCompleted(IMoniker * pmk, LPBC pbc)
{
    return E_NOTIMPL;
}

HRESULT CServerObject::GetCurMoniker(IMoniker ** ppmk)
{
    return E_NOTIMPL;
}


// IOleObject methods

HRESULT CServerObject::SetClientSite(IOleClientSite *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetClientSite(IOleClientSite **)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::SetHostNames(LPCOLESTR, LPCOLESTR)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::Close(DWORD)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::SetMoniker(DWORD , IMoniker *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetMoniker(DWORD, DWORD, IMoniker **)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::InitFromData(IDataObject *, BOOL, DWORD)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetClipboardData(DWORD, IDataObject **)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::DoVerb(LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCOLERECT)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::EnumVerbs(IEnumOLEVERB **)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::Update()
{
    return E_NOTIMPL;
}


HRESULT CServerObject::IsUpToDate()
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetUserClassID(CLSID *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetUserType(DWORD, LPOLESTR *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::SetExtent(DWORD, SIZEL *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetExtent(DWORD, SIZEL *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::Advise(IAdviseSink *, DWORD *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::Unadvise(DWORD)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::EnumAdvise(IEnumSTATDATA **)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::GetMiscStatus(DWORD, DWORD *)
{
    return E_NOTIMPL;
}


HRESULT CServerObject::SetColorScheme(LOGPALETTE  *pLogpal)
{
    return E_NOTIMPL;
}



