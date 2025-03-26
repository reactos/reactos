/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/lnbinfo.cpp
 * PURPOSE:         IBDA_LNBInfo interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID IID_IBDA_LNBInfo = {0x992cf102, 0x49f9, 0x4719, {0xa6, 0x64,  0xc4, 0xf2, 0x3e, 0x24, 0x08, 0xf4}};
const GUID KSPROPSETID_BdaLNBInfo = {0x992cf102, 0x49f9, 0x4719, {0xa6, 0x64, 0xc4, 0xf2, 0x3e, 0x24, 0x8, 0xf4}};
#endif

class CBDALNBInfo : public IBDA_LNBInfo
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

    //IBDA_LNBInfo methods
    HRESULT STDMETHODCALLTYPE put_LocalOscillatorFrequencyLowBand(ULONG ulLOFLow);
    HRESULT STDMETHODCALLTYPE get_LocalOscillatorFrequencyLowBand(ULONG *pulLOFLow);
    HRESULT STDMETHODCALLTYPE put_LocalOscillatorFrequencyHighBand(ULONG ulLOFHigh);
    HRESULT STDMETHODCALLTYPE get_LocalOscillatorFrequencyHighBand(ULONG *pulLOFHigh);
    HRESULT STDMETHODCALLTYPE put_HighLowSwitchFrequency(ULONG ulSwitchFrequency);
    HRESULT STDMETHODCALLTYPE get_HighLowSwitchFrequency(ULONG *pulSwitchFrequency);

    CBDALNBInfo(IKsPropertySet *pProperty, ULONG NodeId) : m_Ref(0), m_pProperty(pProperty), m_NodeId(NodeId){};
    ~CBDALNBInfo(){};

protected:
    LONG m_Ref;
    IKsPropertySet * m_pProperty;
    ULONG m_NodeId;
};

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBDA_LNBInfo))
    {
        *Output = (IBDA_LNBInfo*)(this);
        reinterpret_cast<IBDA_LNBInfo*>(*Output)->AddRef();
        return NOERROR;
    }

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDALNBInfo::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);
#endif

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_LocalOscillatorFrequencyLowBand(ULONG ulLOFLow)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaLNBInfo, KSPROPERTY_BDA_LNB_LOF_LOW_BAND, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulLOFLow, sizeof(LONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_LocalOscillatorFrequencyLowBand: m_NodeId %lu ulLOFLow %lu hr %lx\n", m_NodeId, ulLOFLow, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscillatorFrequencyLowBand(ULONG *pulLOFLow)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_LocalOscillatorFrequencyHighBand(ULONG ulLOFHigh)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaLNBInfo, KSPROPERTY_BDA_LNB_LOF_HIGH_BAND, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulLOFHigh, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_LocalOscillatorFrequencyHighBand: m_NodeId %lu ulLOFHigh %lu hr %lx\n", m_NodeId, ulLOFHigh, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscillatorFrequencyHighBand(ULONG *pulLOFHigh)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_HighLowSwitchFrequency(ULONG ulSwitchFrequency)
{
    KSP_NODE Node;
    HRESULT hr;

    // setup request
    Node.NodeId = m_NodeId;
    Node.Reserved = 0;

    // perform request
    hr = m_pProperty->Set(KSPROPSETID_BdaLNBInfo, KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY, &Node.NodeId, sizeof(KSP_NODE)-sizeof(KSPROPERTY), &ulSwitchFrequency, sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_HighLowSwitchFrequency: m_NodeId %lu ulSwitchFrequency %lu hr %lx\n", m_NodeId, ulSwitchFrequency, hr);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_HighLowSwitchFrequency(ULONG *pulSwitchFrequency)
{
    return E_NOINTERFACE;
}

HRESULT
WINAPI
CBDALNBInfo_fnConstructor(
    IKsPropertySet *pProperty,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDALNBInfo * handler = new CBDALNBInfo(pProperty, NodeId);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDALNBInfo_fnConstructor\n");
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
