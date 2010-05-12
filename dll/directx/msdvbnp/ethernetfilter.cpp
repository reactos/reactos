/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msdvbnp/ethernetfilter.cpp
 * PURPOSE:         IBDA_EthernetFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CEthernetFilter : public IBDA_EthernetFilter
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IBDA_EthernetFilter
    HRESULT STDMETHODCALLTYPE GetMulticastListSize(ULONG *pulcbAddresses);
    HRESULT STDMETHODCALLTYPE PutMulticastList(ULONG ulcbAddresses, BYTE * pAddressList);
    HRESULT STDMETHODCALLTYPE GetMulticastList(ULONG *pulcbAddresses, BYTE *pAddressList);
    HRESULT STDMETHODCALLTYPE PutMulticastMode(ULONG ulModeMask);
    HRESULT STDMETHODCALLTYPE GetMulticastMode(ULONG *pulModeMask);


    CEthernetFilter(IBDA_NetworkProvider * pNetworkProvider);
    virtual ~CEthernetFilter();

protected:
    IBDA_NetworkProvider * m_pNetworkProvider;
    ULONG m_ulcbAddresses;
    BYTE * m_pAddressList;
    ULONG m_ulModeMask;
};

CEthernetFilter::CEthernetFilter(
    IBDA_NetworkProvider * pNetworkProvider) : m_pNetworkProvider(pNetworkProvider),
                                               m_ulcbAddresses(0),
                                               m_pAddressList(0),
                                               m_ulModeMask(0)
{
}

CEthernetFilter::~CEthernetFilter()
{
    if (m_pAddressList)
        CoTaskMemFree(m_pAddressList);
}

HRESULT
STDMETHODCALLTYPE
CEthernetFilter::QueryInterface(
    REFIID InterfaceId,
    PVOID* Interface)
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->QueryInterface(InterfaceId, Interface);
}

ULONG
STDMETHODCALLTYPE
CEthernetFilter::AddRef()
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->AddRef();
}

ULONG
STDMETHODCALLTYPE
CEthernetFilter::Release()
{
    assert(m_pNetworkProvider);
    return m_pNetworkProvider->Release();
}

//-------------------------------------------------------------------
//IBDA_EthernetFilter
//

HRESULT
STDMETHODCALLTYPE
CEthernetFilter::PutMulticastList(
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
CEthernetFilter::GetMulticastList(
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
CEthernetFilter::GetMulticastListSize(
    ULONG *pulcbAddresses)
{
    if (!pulcbAddresses)
        return E_POINTER;

    *pulcbAddresses = m_ulcbAddresses;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CEthernetFilter::PutMulticastMode(
    ULONG ulModeMask)
{
    m_ulModeMask = ulModeMask;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CEthernetFilter::GetMulticastMode(
    ULONG *pulModeMask)
{
    *pulModeMask = m_ulModeMask;
    return NOERROR;
}

HRESULT
WINAPI
CEthernetFilter_fnConstructor(
    IBDA_NetworkProvider * pNetworkProvider,
    REFIID riid,
    LPVOID * ppv)
{
    CEthernetFilter * filter = new CEthernetFilter(pNetworkProvider);

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
