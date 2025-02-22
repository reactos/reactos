/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/frequencyfilter.cpp
 * PURPOSE:         IBDA_FrequencyFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID IID_IBDA_FrequencyFilter = {0x71985f47, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSPROPSETID_BdaFrequencyFilter = {0x71985f47, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
#endif

class CBDAFrequencyFilter : public IBDA_FrequencyFilter
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);
        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    HRESULT STDMETHODCALLTYPE put_Autotune(ULONG ulTransponder);
    HRESULT STDMETHODCALLTYPE get_Autotune(ULONG *pulTransponder);
    HRESULT STDMETHODCALLTYPE put_Frequency(ULONG ulFrequency);
    HRESULT STDMETHODCALLTYPE get_Frequency(ULONG *pulFrequency);
    HRESULT STDMETHODCALLTYPE put_Polarity(Polarisation Polarity);
    HRESULT STDMETHODCALLTYPE get_Polarity(Polarisation *pPolarity);
    HRESULT STDMETHODCALLTYPE put_Range(ULONG ulRange);
    HRESULT STDMETHODCALLTYPE get_Range(ULONG *pulRange);
    HRESULT STDMETHODCALLTYPE put_Bandwidth(ULONG ulBandwidth);
    HRESULT STDMETHODCALLTYPE get_Bandwidth(ULONG *pulBandwidth);
    HRESULT STDMETHODCALLTYPE put_FrequencyMultiplier(ULONG ulMultiplier);
    HRESULT STDMETHODCALLTYPE get_FrequencyMultiplier(ULONG *pulMultiplier);

    CBDAFrequencyFilter(IKsPropertySet * pProperty, ULONG NodeId) : m_Ref(0), m_pProperty(pProperty), m_NodeId(NodeId){};
    virtual ~CBDAFrequencyFilter(){};

protected:
    LONG m_Ref;
    IKsPropertySet * m_pProperty;
    ULONG m_NodeId;
};

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    *Output = NULL;

    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_FrequencyFilter))
    {
        *Output = (IBDA_FrequencyFilter*)(this);
        reinterpret_cast<IBDA_FrequencyFilter*>(*Output)->AddRef();
        return NOERROR;
    }

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CControlNode::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);
DebugBreak();
#endif

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Autotune(ULONG ulTransponder)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Autotune(ULONG *pulTransponder)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Frequency(ULONG ulFrequency)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaFrequencyFilter, KSPROPERTY_BDA_RF_TUNER_FREQUENCY, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulFrequency, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Frequency: m_NodeId %lu ulFrequency %lu hr %lx\n", m_NodeId, ulFrequency, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Frequency(ULONG *pulFrequency)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Polarity(Polarisation Polarity)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaFrequencyFilter, KSPROPERTY_BDA_RF_TUNER_POLARITY, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &Polarity, sizeof(Polarisation));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Polarity: m_NodeId %lu Polarity %lu hr %lx\n", m_NodeId, Polarity, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Polarity(Polarisation *pPolarity)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Range(ULONG ulRange)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaFrequencyFilter, KSPROPERTY_BDA_RF_TUNER_RANGE, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulRange, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Range: m_NodeId %lu ulRange %lu hr %lx\n", m_NodeId, ulRange, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Range(ULONG *pulRange)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Bandwidth(ULONG ulBandwidth)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaFrequencyFilter, KSPROPERTY_BDA_RF_TUNER_BANDWIDTH, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulBandwidth, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Bandwidth: m_NodeId %lu ulBandwidth %lu hr %lx\n", m_NodeId, ulBandwidth, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Bandwidth(ULONG *pulBandwidth)
{
    return E_NOINTERFACE;
}
HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_FrequencyMultiplier(ULONG ulMultiplier)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaFrequencyFilter, KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulMultiplier, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_FrequencyMultiplier: m_NodeId %lu ulMultiplier %lu hr %lx\n", m_NodeId, ulMultiplier, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_FrequencyMultiplier(ULONG *pulMultiplier)
{
    return E_NOINTERFACE;
}

HRESULT
WINAPI
CBDAFrequencyFilter_fnConstructor(
    IKsPropertySet* pProperty,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDAFrequencyFilter * handler = new CBDAFrequencyFilter(pProperty, NodeId);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAFrequencyFilter_fnConstructor\n");
#endif

    if (!handler)
        return E_OUTOFMEMORY;

    if (FAILED(handler->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete handler;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
