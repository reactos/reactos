//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      evtsink.cpp
//
//  Contents:  CInputEventSink implementation
//
//  History:   16-Jul-96 WayneSc    Created
//             16-Jan-97 EricB      Adapted from MMC code.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#include "EvtSink.h"
#include <mshtmdid.h>
#pragma hdrstop

CInputEventSink::CInputEventSink(LPSITE pSite) :
    m_refs(0)
{
    ASSERT(pSite != NULL);
    m_pSite = pSite;

#ifdef _DEBUG
    strcpy(szClass, "CInputEventSink");
#endif
}

CInputEventSink::~CInputEventSink()
{
    ASSERT(m_refs == 0);
}

STDMETHODIMP
CInputEventSink::QueryInterface(REFIID riid, void** ppv)
{
    ASSERT(m_pSite != NULL);
    return m_pSite->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG)
CInputEventSink::AddRef(void)
{
    ASSERT(m_pSite != NULL);
    ++m_refs;
    return m_pSite->AddRef();
}

STDMETHODIMP_(ULONG)
CInputEventSink::Release(void)
{
    ASSERT(m_pSite != NULL);
    ASSERT(m_refs > 0);
    --m_refs;
    return m_pSite->Release();
}

STDMETHODIMP
CInputEventSink::GetTypeInfoCount(UINT* pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CInputEventSink::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CInputEventSink::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
                               LCID lcid, DISPID* rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CInputEventSink::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS * pdispparams, VARIANT * pvarResult,
                        EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    TRACE(TEXT("CInputEventSink::Invoke called with dispid %d\n"), dispid);

    HRESULT hr = S_OK;

    if (dispid == DISPID_HTMLTEXTCONTAINEREVENTS_ONCHANGE ||
        //dispid == DISPID_IHTMLINPUTTEXT_ONCHANGE ||
        dispid == DISPID_HTMLSELECTELEMENTEVENTS_ONCHANGE ||
        dispid == DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE)
    {
        //
        // A change event has occurred. Enable the Apply button.
        //
        TRACE(TEXT("CInputEventSink::Invoke: OnChange received!/n"));
    }

    return hr;
}
