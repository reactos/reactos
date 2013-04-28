/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msdvbnp/ipv6.cpp
 * PURPOSE:         IBDA_IPV6Filter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CIPV6Filter : public IBDA_IPV6Filter
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IBDA_IPV6Filter
    HRESULT STDMETHODCALLTYPE GetMulticastListSize(ULONG *pulcbAddresses);
    HRESULT STDMETHODCALLTYPE PutMulticastList(ULONG ulcbAddresses, BYTE * pAddressList);
    HRESULT STDMETHODCALLTYPE GetMulticastList(ULONG *pulcbAddresses, BYTE *pAddressList);
    HRESULT STDMETHODCALLTYPE PutMulticastMode(ULONG ulModeMask);
    HRESULT STDMETHODCALLTYPE GetMulticastMode(ULONG *pulModeMask);

    CIPV6Filter(IBDA_NetworkProvider * pNetworkProvider);
    virtual ~CIPV6Filter();

protected:
    IBDA_NetworkProvider * m_pNetworkProvider;
    ULONG m_ulcbAddresses;
    BYTE * m_pAddressList;
    ULONG m_ulModeMask;
};

CIPV6Filter::CIPV6Filter(
    IBDA_NetworkProvider * pNetworkProvider) : m_pNetworkProvider(pNetworkProvider),
                                               m_ulcbAddresses(0),
                                               m_pAddressList(0),
                                               m_ulModeMask(0)
{
}

CIPV6Filter::~CIPV6Filter()
{
    if (m_pAddressList)
        CoTaskMemFree(m_pAddressList);
}

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::QueryInterface(
    REFIID InterfaceId,
    PVOID* Interface)
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->QueryInterface(InterfaceId, Interface);
}

ULONG
STDMETHODCALLTYPE
CIPV6Filter::AddRef()
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->AddRef();
}

ULONG
STDMETHODCALLTYPE
CIPV6Filter::Release()
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->Release();
}

//-------------------------------------------------------------------
//IBDA_IPV6Filter
//

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::GetMulticastListSize(
    ULONG *pulcbAddresses)
{
    if (!pulcbAddresses)
        return E_POINTER;

    *pulcbAddresses = 0;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::GetMulticastList(
    ULONG *pulcbAddresses,
    BYTE *pAddressList)
{
    if (!pulcbAddresses || !pAddressList)
        return E_POINTER;

    if (*pulcbAddresses < m_ulcbAddresses)
        return E_OUTOFMEMORY;

    CopyMemory(pAddressList, m_pAddressList, m_ulcbAddresses);
    *pulcbAddresses = m_ulcbAddresses;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::PutMulticastList(
    ULONG ulcbAddresses,
    BYTE * pAddressList)
{
    if (!ulcbAddresses || !pAddressList)
        return E_POINTER;

    if (m_pAddressList)
        CoTaskMemFree(m_pAddressList);

    m_pAddressList = (BYTE*)CoTaskMemAlloc(ulcbAddresses);
    if (!m_pAddressList)
        return E_OUTOFMEMORY;

    CopyMemory(m_pAddressList, pAddressList, ulcbAddresses);
    m_ulcbAddresses = ulcbAddresses;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::PutMulticastMode(
    ULONG ulModeMask)
{
    m_ulModeMask = ulModeMask;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CIPV6Filter::GetMulticastMode(
    ULONG *pulModeMask)
{
    *pulModeMask = m_ulModeMask;
    return NOERROR;
}

HRESULT
WINAPI
CIPV6Filter_fnConstructor(
    IBDA_NetworkProvider * pNetworkProvider,
    REFIID riid,
    LPVOID * ppv)
{
    CIPV6Filter * filter = new CIPV6Filter(pNetworkProvider);

    if (!filter)
        return E_OUTOFMEMORY;

    if (FAILED(filter->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete filter;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
