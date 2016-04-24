/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/signalstatistics.cpp
 * PURPOSE:         IBDA_FrequencyFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID IID_IBDA_SignalStatistics = {0x1347d106, 0xcf3a, 0x428a, {0xa5, 0xcb, 0xac, 0x0d, 0x9a, 0x2a, 0x43, 0x38}};
const GUID KSPROPSETID_BdaSignalStats = {0x1347d106, 0xcf3a, 0x428a, {0xa5, 0xcb, 0xac, 0xd, 0x9a, 0x2a, 0x43, 0x38}};
#endif

class CBDASignalStatistics : public IBDA_SignalStatistics
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

    // IBDA_SignalStatistics methods
    HRESULT STDMETHODCALLTYPE put_SignalStrength(LONG lDbStrength);
    HRESULT STDMETHODCALLTYPE get_SignalStrength(LONG *plDbStrength);
    HRESULT STDMETHODCALLTYPE put_SignalQuality(LONG lPercentQuality);
    HRESULT STDMETHODCALLTYPE get_SignalQuality(LONG *plPercentQuality);
    HRESULT STDMETHODCALLTYPE put_SignalPresent(BOOLEAN fPresent);
    HRESULT STDMETHODCALLTYPE get_SignalPresent(BOOLEAN *pfPresent);
    HRESULT STDMETHODCALLTYPE put_SignalLocked(BOOLEAN fLocked);
    HRESULT STDMETHODCALLTYPE get_SignalLocked(BOOLEAN *pfLocked);
    HRESULT STDMETHODCALLTYPE put_SampleTime(LONG lmsSampleTime);
    HRESULT STDMETHODCALLTYPE get_SampleTime(LONG *plmsSampleTime);

    CBDASignalStatistics(IKsPropertySet * pProperty, ULONG NodeId) : m_Ref(0), m_pProperty(pProperty), m_NodeId(NodeId){};
    ~CBDASignalStatistics(){};

protected:
    LONG m_Ref;
    IKsPropertySet * m_pProperty;
    ULONG m_NodeId;
};

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBDA_SignalStatistics))
    {
        *Output = (IBDA_SignalStatistics*)(this);
        reinterpret_cast<IBDA_SignalStatistics*>(*Output)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalStrength(LONG lDbStrength)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalStrength(LONG *plDbStrength)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    assert(m_pProperty);

    hr = m_pProperty->Get(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_STRENGTH, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), plDbStrength, sizeof(LONG), &BytesReturned);


#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::get_SignalStrength: m_NodeId %lu hr %lx, BytesReturned %lu plDbStrength %ld\n", m_NodeId, hr, BytesReturned, *plDbStrength);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalQuality(LONG lPercentQuality)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalQuality(LONG *plPercentQuality)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Get(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_QUALITY, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), plPercentQuality, sizeof(LONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::get_SignalQuality: m_NodeId %lu hr %lx, BytesReturned %lu plPercentQuality %lu\n", m_NodeId, hr, BytesReturned, *plPercentQuality);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalPresent(BOOLEAN fPresent)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalPresent(BOOLEAN *pfPresent)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG Present;
    ULONG BytesReturned;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Get(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_PRESENT, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &Present, sizeof(ULONG), &BytesReturned);

    // store result
    *pfPresent = Present;

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::get_SignalPresent: m_NodeId %lu hr %lx, BytesReturned %lu Present %lu\n", m_NodeId, hr, BytesReturned, Present);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalLocked(BOOLEAN fLocked)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalLocked(BOOLEAN *pfLocked)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG Locked;
    ULONG BytesReturned;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Get(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SIGNAL_LOCKED, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &Locked, sizeof(ULONG), &BytesReturned);

    *pfLocked = Locked;

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::get_SignalLocked: m_NodeId %lu hr %lx, BytesReturned %lu Locked %lu\n", m_NodeId, hr, BytesReturned, Locked);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SampleTime(LONG lmsSampleTime)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SAMPLE_TIME, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &lmsSampleTime, sizeof(LONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::put_SampleTime: m_NodeId %lu hr %lx\n", m_NodeId, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SampleTime(LONG *plmsSampleTime)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.NodeId = (ULONG)-1;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Get(KSPROPSETID_BdaSignalStats, KSPROPERTY_BDA_SAMPLE_TIME, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), plmsSampleTime, sizeof(LONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDASignalStatistics::get_SampleTime: m_NodeId %lu hr %lx, BytesReturned %lu \n", m_NodeId, hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
WINAPI
CBDASignalStatistics_fnConstructor(
    IKsPropertySet * pProperty,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDASignalStatistics * handler = new CBDASignalStatistics(pProperty, NodeId);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDASignalStatistics_fnConstructor\n");
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
