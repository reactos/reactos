
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       idata.cxx
//
//  Contents:   IDataObject implementation for path
//
//  History:    27-Oct-95 BruceFo stolen from old sharing tool
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <shlobj.h>
#include "idata.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::CDataObject
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------

CDataObject::CDataObject(
    VOID
    )
    :
    m_refs(1),
    m_pszItem(NULL)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::~CDataObject
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------

CDataObject::~CDataObject()
{
    delete[] m_pszItem;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::InitInstance
//
//  Synopsis:   2nd phase constructor
//
//----------------------------------------------------------------------------

HRESULT
CDataObject::InitInstance(
    IN PWSTR pszItem
    )
{
    DWORD dwLen = wcslen(pszItem);
    m_pszItem = new WCHAR[dwLen + 1];
    if (NULL == m_pszItem)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        wcscpy(m_pszItem, pszItem);
    }

    return S_OK;
}


//
// IUnknown Methods
//

//+---------------------------------------------------------------
//
//  Member:     CDataObject::QueryInterface
//
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryInterface(
    IN REFIID riid,
    OUT VOID ** ppv
    )
{
    *ppv = NULL;

    IUnknown*    pUnkTemp = NULL;
    HRESULT      hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        appDebugOut((DEB_ITRACE, "CDataObject::IUnknown queried\n"));
        pUnkTemp = (IUnknown *) this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        appDebugOut((DEB_ITRACE, "CDataObject::IDataObject queried\n"));
        pUnkTemp = (IDataObject *) this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (pUnkTemp != NULL)
    {
        pUnkTemp->AddRef();
    }

    *ppv = pUnkTemp;

    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::AddRef
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::AddRef(VOID)
{
    return ++m_refs;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::Release
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::Release(VOID)
{
    ULONG refs = --m_refs;
    if (refs == 0)
    {
        delete this;
    }

    return refs;
}


//
// IDataObject methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::GetData
//
//  Synopsis:   IDataObject method of Death
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetData(
    IN LPFORMATETC pFormatEtc,
    OUT LPSTGMEDIUM pStgMedium
    )
{
    if (pFormatEtc->cfFormat != CF_HDROP)
    {
        //
        // Sorry, we don't recognize this format.
        //

        return DATA_E_FORMATETC;
    }

    DWORD size = sizeof(DROPFILES) + (wcslen(m_pszItem) + 1 + 1) * sizeof(WCHAR);

    HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, size);
    if (hGlobal == NULL)
    {
         return E_OUTOFMEMORY;
    }
    ZeroMemory((LPVOID)hGlobal, size);

    LPDROPFILES pdrop = (LPDROPFILES)hGlobal;
    pdrop->pFiles = sizeof(DROPFILES);
    pdrop->fWide  = TRUE;
    wcscpy((LPWSTR) ((LPBYTE)pdrop + sizeof(DROPFILES)), m_pszItem);

    // return the data
    pStgMedium->tymed          = TYMED_HGLOBAL;
    pStgMedium->hGlobal        = hGlobal;
    pStgMedium->pUnkForRelease = NULL;

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::GetDataHere
//
// BUGBUG: is GetDataHere implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::GetDataHere(
    IN LPFORMATETC pformatetc,
    IN OUT LPSTGMEDIUM pStgMedium
    )
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::QueryGetData
//
// BUGBUG: is QueryGetData implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryGetData(
    IN LPFORMATETC pformatetc
    )
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::GetCanonicalFormatEtc
//
// BUGBUG: is GetCanonicalFormatEtc implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::GetCanonicalFormatEtc(
    IN  LPFORMATETC pformatetc,
    OUT LPFORMATETC pformatetcOut
    )
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::SetData
//
// BUGBUG: is SetData implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::SetData(
    IN LPFORMATETC pFormatEtc,
    IN STGMEDIUM * pStgMedium,
    IN BOOL fRelease
    )
{
    return E_NOTIMPL;
}




//+---------------------------------------------------------------
//
//  Member:     CDataObject::EnumFormatEtc
//
// BUGBUG: is EnumFormatEtc implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumFormatEtc(
    IN DWORD dwDirection,
    OUT LPENUMFORMATETC * ppEnum
    )
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::DAdvise
//
// BUGBUG: is DAdvise implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::DAdvise(
    IN FORMATETC *  pFormatetc,
    IN DWORD        advf,
    IN LPADVISESINK pAdvSink,
    OUT DWORD *     pdwConnection
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::DUnadvise
//
// BUGBUG: is DUnadvise implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::DUnadvise(
    IN DWORD dwConnection
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}


//+---------------------------------------------------------------
//
//  Member:     CDataObject::EnumDAdvise
//
// BUGBUG: is EnumDAdvise implemented?
//---------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumDAdvise(
    OUT LPENUMSTATDATA * ppenumAdvise
    )
{
    return OLE_E_ADVISENOTSUPPORTED;
}
