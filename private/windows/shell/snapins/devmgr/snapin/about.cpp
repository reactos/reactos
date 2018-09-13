/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    about.cpp

Abstract:

    This module implemets ISnapinAbout inteface(CDevMgrAbout class).

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "about.h"

//
// IUnknown interface
//
ULONG
CDevMgrAbout::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}
ULONG
CDevMgrAbout::Release()
{
    ::InterlockedDecrement((LONG*)&m_Ref);
    if (!m_Ref)
    {
    delete this;
    return 0;
    }
    return m_Ref;
}

STDMETHODIMP
CDevMgrAbout::QueryInterface(
    REFIID  riid,
    void**  ppv
    )
{
    if (!ppv)
    return E_INVALIDARG;
    HRESULT hr = S_OK;


    if (IsEqualIID(riid, IID_IUnknown))
    {
    *ppv = (IUnknown*)this;
    }
    else if (IsEqualIID(riid, IID_ISnapinAbout))
    {
    *ppv = (ISnapinAbout*)this;
    }
    else
    {
    *ppv = NULL;
    hr = E_NOINTERFACE;
    }
    if (SUCCEEDED(hr))
    {
    AddRef();
    }
    return hr;
}

// ISnapinAbout interface

STDMETHODIMP
CDevMgrAbout::GetSnapinDescription(
    LPOLESTR *ppDescription
    )
{
    return LoadResourceOleString(IDS_PROGRAM_ABOUT, ppDescription);
}
STDMETHODIMP
CDevMgrAbout::GetProvider(
    LPOLESTR *ppProvider
    )
{
    return LoadResourceOleString(IDS_PROGRAM_PROVIDER, ppProvider);
}
STDMETHODIMP
CDevMgrAbout::GetSnapinVersion(
    LPOLESTR *ppVersion
    )
{
    return LoadResourceOleString(IDS_PROGRAM_VERSION, ppVersion);
}

STDMETHODIMP
CDevMgrAbout::GetSnapinImage(
    HICON* phIcon
    )
{
    if (!phIcon)
    return E_INVALIDARG;

    *phIcon = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_DEVMGR));
    if (!*phIcon)
    return E_OUTOFMEMORY;
    return S_OK;
}

STDMETHODIMP
CDevMgrAbout::GetStaticFolderImage(
    HBITMAP* phSmall,
    HBITMAP* phSmallOpen,
    HBITMAP* phLarge,
    COLORREF* pcrMask
    )
{
    if (!phSmall || !phSmallOpen || !phLarge || !pcrMask)
    return E_INVALIDARG;

    *phSmall = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SYSDM16));
    *phSmallOpen = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SYSDM16));
    *phLarge =::LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_SYSDM32));
    *pcrMask = RGB(255, 0, 255);
    if (NULL == *phSmall || NULL == *phSmallOpen || NULL == *phLarge)
    {
    if (NULL != *phSmall)
    {
        ::DeleteObject(*phSmall);
        *phSmall = NULL;
    }
    if (NULL != *phSmallOpen)
    {
        ::DeleteObject(*phSmallOpen);
        *phSmallOpen = NULL;
    }
    if (NULL != *phLarge)
    {
        ::DeleteObject(*phLarge);
        *phLarge = NULL;
    }
    return E_OUTOFMEMORY;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// private routine to allocate ole task memory and load the given resource
// string(indicated by its string id) to the allocated memory.
// INPUT:
//  StringId -- the string resource id
//  LPOLESTR* -- place holder to hold the ole string pointer
// OUTPUT:
//  standard OLE HRESULT
HRESULT
CDevMgrAbout::LoadResourceOleString(
    int StringId,
    LPOLESTR* ppolestr
    )
{
    if (!ppolestr)
    return E_INVALIDARG;
    TCHAR Text[MAX_PATH];
    // get the string
    ::LoadString(g_hInstance, StringId, Text, ARRAYLEN(Text));
    try
    {
    *ppolestr = AllocOleTaskString(Text);
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    if (*ppolestr)
    {
        FreeOleTaskString(*ppolestr);
        *ppolestr = NULL;
    }
    }
    if (!*ppolestr)
    return E_OUTOFMEMORY;
    return S_OK;
}
