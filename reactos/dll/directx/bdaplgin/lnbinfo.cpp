/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/lnbinfo.cpp
 * PURPOSE:         IBDA_LNBInfo interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_LNBInfo = {0x992cf102, 0x49f9, 0x4719, {0xa6, 0x64,  0xc4, 0xf2, 0x3e, 0x24, 0x08, 0xf4}};
const GUID KSPROPSETID_BdaLNBInfo = {0x992cf102, 0x49f9, 0x4719, {0xa6, 0x64, 0xc4, 0xf2, 0x3e, 0x24, 0x8, 0xf4}};

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
    HRESULT STDMETHODCALLTYPE put_LocalOscilatorFrequencyLowBand(ULONG ulLOFLow);
    HRESULT STDMETHODCALLTYPE get_LocalOscilatorFrequencyLowBand(ULONG *pulLOFLow);
    HRESULT STDMETHODCALLTYPE put_LocalOscilatorFrequencyHighBand(ULONG ulLOFHigh);
    HRESULT STDMETHODCALLTYPE get_LocalOscilatorFrequencyHighBand(ULONG *pulLOFHigh);
    HRESULT STDMETHODCALLTYPE put_HighLowSwitchFrequency(ULONG ulSwitchFrequency);
    HRESULT STDMETHODCALLTYPE get_HighLowSwitchFrequency(ULONG *pulSwitchFrequency);

    CBDALNBInfo(HANDLE hFile, ULONG NodeId) : m_Ref(0), m_hFile(hFile), m_NodeId(NodeId){};
    ~CBDALNBInfo(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
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
CBDALNBInfo::put_LocalOscilatorFrequencyLowBand(ULONG ulLOFLow)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaLNBInfo;
    Node.Property.Id = KSPROPERTY_BDA_LNB_LOF_LOW_BAND;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulLOFLow, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_LocalOscilatorFrequencyLowBand: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscilatorFrequencyLowBand(ULONG *pulLOFLow)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_LocalOscilatorFrequencyHighBand(ULONG ulLOFHigh)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaLNBInfo;
    Node.Property.Id = KSPROPERTY_BDA_LNB_LOF_HIGH_BAND;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulLOFHigh, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_LocalOscilatorFrequencyHighBand: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscilatorFrequencyHighBand(ULONG *pulLOFHigh)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_HighLowSwitchFrequency(ULONG ulSwitchFrequency)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaLNBInfo;
    Node.Property.Id = KSPROPERTY_BDA_LNB_SWITCH_FREQUENCY;
    Node.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), &ulSwitchFrequency, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDALNBInfo::put_HighLowSwitchFrequency: m_NodeId %lu hr %lx, BytesReturned %lu\n", m_NodeId, hr, BytesReturned);
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
    HANDLE hFile,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDALNBInfo * handler = new CBDALNBInfo(hFile, NodeId);

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