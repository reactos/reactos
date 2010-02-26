/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/frequencyfilter.cpp
 * PURPOSE:         IBDA_FrequencyFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_FrequencyFilter = {0x71985f47, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSPROPSETID_BdaFrequencyFilter = {0x71985f47, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};

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

    CBDAFrequencyFilter(HANDLE hFile, ULONG NodeId) : m_Ref(0), m_hFile(hFile), m_NodeId(NodeId){};
    virtual ~CBDAFrequencyFilter(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
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

    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaFrequencyFilter;
    Node.Property.Id = KSPROPERTY_BDA_RF_TUNER_FREQUENCY;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulFrequency, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Frequency: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaFrequencyFilter;
    Node.Property.Id = KSPROPERTY_BDA_RF_TUNER_POLARITY;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &Polarity, sizeof(Polarisation), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Polarity: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaFrequencyFilter;
    Node.Property.Id = KSPROPERTY_BDA_RF_TUNER_RANGE;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulRange, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Polarity: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaFrequencyFilter;
    Node.Property.Id = KSPROPERTY_BDA_RF_TUNER_BANDWIDTH;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulBandwidth, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_Bandwidth: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaFrequencyFilter;
    Node.Property.Id = KSPROPERTY_BDA_RF_TUNER_FREQUENCY_MULTIPLIER;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulMultiplier, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAFrequencyFilter::put_FrequencyMultiplier: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    HANDLE hFile,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDAFrequencyFilter * handler = new CBDAFrequencyFilter(hFile, NodeId);

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