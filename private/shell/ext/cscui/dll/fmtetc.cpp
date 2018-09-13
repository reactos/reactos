//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       fmtetc.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "fmtetc.h"

CEnumFormatEtc::CEnumFormatEtc(
    UINT cFormats, 
    LPFORMATETC prgFormats
    ) : m_cRef(0),
        m_cFormats(0),
        m_iCurrent(0),
        m_prgFormats(NULL),
        m_hrCtor(NOERROR)
{
    m_hrCtor = AddFormats(cFormats, prgFormats);
}


CEnumFormatEtc::CEnumFormatEtc(
    const CEnumFormatEtc& ef
    ) : m_cRef(0),
        m_cFormats(0),
        m_iCurrent(0),
        m_prgFormats(NULL),
        m_hrCtor(NOERROR)
{
    m_hrCtor = AddFormats(ef.m_cFormats, ef.m_prgFormats);
}



CEnumFormatEtc::~CEnumFormatEtc(
    VOID
    )
{
    delete[] m_prgFormats;
}


STDMETHODIMP 
CEnumFormatEtc::QueryInterface(
    REFIID riid, 
    LPVOID *ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(CEnumFormatEtc, IEnumFORMATETC),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) 
CEnumFormatEtc::AddRef(
    VOID
    )
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) 
CEnumFormatEtc::Release(
    VOID
    )
{
    if (0 != InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP
CEnumFormatEtc::Next(
    DWORD cFormats,
    LPFORMATETC pFormats,
    LPDWORD pcReturned
    )
{
    HRESULT hr = S_OK;
    DWORD iFormats = 0;
    if (NULL == pFormats)
        return E_INVALIDARG;

    while(cFormats-- > 0)
    {
        if (m_iCurrent < m_cFormats)
        {
            *(pFormats + iFormats++) = m_prgFormats[m_iCurrent++];
        }
        else
        {
            hr = S_FALSE;
            break;
        }
    }

    if (NULL != pcReturned)
        *pcReturned = iFormats;

    return hr;
}


STDMETHODIMP
CEnumFormatEtc::Skip(
    DWORD cFormats
    )
{
    while((cFormats-- > 0) && (m_iCurrent < m_cFormats))
        m_iCurrent++;

    return cFormats == 0 ? S_OK : S_FALSE;
}


STDMETHODIMP 
CEnumFormatEtc::Reset(
    VOID
    )
{
    m_iCurrent = 0;
    return S_OK;
}


STDMETHODIMP 
CEnumFormatEtc::Clone(
    IEnumFORMATETC **ppvOut
    )
{
    HRESULT hr = NO_ERROR;
    CEnumFormatEtc *pNew = new CEnumFormatEtc(*this);
    if (NULL != pNew)
    {
        hr = pNew->QueryInterface(IID_IEnumFORMATETC, (LPVOID *)ppvOut);
    }
    return hr;
}


HRESULT
CEnumFormatEtc::AddFormats(
    UINT cFormats,
    LPFORMATETC pFormats
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    LPFORMATETC pNew  = new FORMATETC[m_cFormats + cFormats];
    if (NULL != pNew)
    {
        FORMATETC *pSrc  = m_prgFormats;
        FORMATETC *pDest = pNew;
        int i;

        for (i = 0; i < m_cFormats; i++)
        {
            *pDest++ = *pSrc++;  // Copy original formats.
        }
        pSrc = pFormats;
        for (i = 0; i < int(cFormats); i++)
        {
            *pDest++ = *pSrc++;  // Add new formats.
        }
        delete[] m_prgFormats;
        m_cFormats += cFormats;
        m_prgFormats = pNew;
        hr = NOERROR;
    }
    return hr;
}

