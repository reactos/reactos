/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/controlnode.cpp
 * PURPOSE:         ControlNode interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IKsPropertySet = {0x31efac30, 0x515c, 0x11d0, {0xa9,0xaa, 0x00,0xaa,0x00,0x61,0xbe,0x93}};

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

    CControlNode(HANDLE hFile, ULONG NodeType, ULONG PinId) : m_Ref(0), m_hFile(hFile), m_NodeType(NodeType), m_PinId(PinId){};
    virtual ~CControlNode(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
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
        return CBDAFrequencyFilter_fnConstructor(m_hFile, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_SignalStatistics))
    {
        return CBDASignalStatistics_fnConstructor(m_hFile, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_LNBInfo))
    {
        return CBDALNBInfo_fnConstructor(m_hFile, m_NodeType, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_DigitalDemodulator))
    {
        return CBDADigitalDemodulator_fnConstructor(m_hFile, m_NodeType, refiid, Output);
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
    HANDLE hFile,
    IBaseFilter * pFilter,
    ULONG NodeType,
    ULONG PinId,
    REFIID riid,
    LPVOID * ppv)
{
    WCHAR Buffer[100];
    HRESULT hr;
    IPin * pPin = NULL;
    IKsObject * pObject = NULL;

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

    // query IKsObject interface
    hr = pPin->QueryInterface(IID_IKsObject, (void**)&pObject);

#ifdef BDAPLGIN_TRACE
    swprintf(Buffer, L"CControlNode_fnConstructor get IID_IKsObject status %lx\n", hr);
    OutputDebugStringW(Buffer);
#endif

    if (SUCCEEDED(hr))
    {
        // get pin handle
        hFile = pObject->KsGetObjectHandle();
        // release IKsObject interface
        pObject->Release();
    }
    // release IPin interface
    pPin->Release();

    // construct device control
    CControlNode * handler = new CControlNode(hFile, NodeType, PinId);

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
