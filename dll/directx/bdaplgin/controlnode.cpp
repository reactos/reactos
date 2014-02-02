/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/controlnode.cpp
 * PURPOSE:         ControlNode interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID IID_IKsPropertySet = {0x31efac30, 0x515c, 0x11d0, {0xa9,0xaa, 0x00,0xaa,0x00,0x61,0xbe,0x93}};
#endif

class CControlNode : public IUnknown
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

    CControlNode(IKsPropertySet * pProperty, ULONG NodeType, ULONG PinId) : m_Ref(0), m_pKsProperty(pProperty), m_NodeType(NodeType), m_PinId(PinId){};
    virtual ~CControlNode(){};

protected:
    LONG m_Ref;
    IKsPropertySet * m_pKsProperty;
    ULONG m_NodeType;
    ULONG m_PinId;
};

HRESULT
STDMETHODCALLTYPE
CControlNode::QueryInterface(
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
    else if(IsEqualGUID(refiid, IID_IBDA_FrequencyFilter))
    {
        return CBDAFrequencyFilter_fnConstructor(m_pKsProperty, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_SignalStatistics))
    {
        return CBDASignalStatistics_fnConstructor(m_pKsProperty, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_LNBInfo))
    {
        return CBDALNBInfo_fnConstructor(m_pKsProperty, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_DigitalDemodulator))
    {
        return CBDADigitalDemodulator_fnConstructor(m_pKsProperty, m_NodeType, refiid, Output);
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
WINAPI
CControlNode_fnConstructor(
    IBaseFilter * pFilter,
    ULONG NodeType,
    ULONG PinId,
    REFIID riid,
    LPVOID * ppv)
{
    WCHAR Buffer[100];
    HRESULT hr;
    IPin * pPin = NULL;
    IKsPropertySet * pProperty;

    // store pin id
    swprintf(Buffer, L"%u", PinId);

    // try find target pin
    hr = pFilter->FindPin(Buffer, &pPin);

    if (FAILED(hr))
    {
#ifdef BDAPLGIN_TRACE
        swprintf(Buffer, L"CControlNode_fnConstructor failed find pin %lu with %lx\n", PinId, hr);
        OutputDebugStringW(Buffer);
#endif
        return hr;
    }

    // query for IKsPropertySet interface
    hr = pPin->QueryInterface(IID_IKsPropertySet, (void**)&pProperty);
    if (FAILED(hr))
        return hr;

#ifdef BDAPLGIN_TRACE
    swprintf(Buffer, L"CControlNode_fnConstructor get IID_IKsObject status %lx\n", hr);
    OutputDebugStringW(Buffer);
#endif

    // release IPin interface
    pPin->Release();

    // construct device control
    CControlNode * handler = new CControlNode(pProperty, NodeType, PinId);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CControlNode_fnConstructor\n");
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
