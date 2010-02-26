/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/classfactory.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_PinControl = {0x0DED49D5, 0xA8B7, 0x4d5d, {0x97, 0xA1, 0x12, 0xB0, 0xC1, 0x95, 0x87, 0x4D}};
const GUID IID_IPin = {0x56a86891, 0x0ad4, 0x11ce, {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};

class CBDAPinControl : public IBDA_PinControl
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
    // IBDA_PinControl methods
    HRESULT STDMETHODCALLTYPE GetPinID(ULONG *pulPinID);
    HRESULT STDMETHODCALLTYPE GetPinType(ULONG *pulPinType);
    HRESULT STDMETHODCALLTYPE RegistrationContext(ULONG *pulRegistrationCtx);


    CBDAPinControl(HANDLE hFile, IBDA_NetworkProvider * pProvider, IPin * pConnectedPin) : m_Ref(0), m_Handle(hFile), m_pProvider(pProvider), m_pConnectedPin(pConnectedPin){};
    virtual ~CBDAPinControl(){};

protected:
    LONG m_Ref;
    HANDLE m_Handle;
    IBDA_NetworkProvider * m_pProvider;
    IPin * m_pConnectedPin;
};

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IBDA_PinControl))
    {
        *Output = (IBDA_PinControl*)(this);
        reinterpret_cast<IBDA_PinControl*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}
//-------------------------------------------------------------------
// IBDA_PinControl methods
//
HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinID(ULONG *pulPinID)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl::GetPinID: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinType(ULONG *pulPinType)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl::GetPinType: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::RegistrationContext(ULONG *pulRegistrationCtx)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl::RegistrationContext: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
CBDAPinControl_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    IKsObject * pObject = NULL;
    IPin * pPin = NULL, * pConnectedPin = NULL;
    IEnumFilters *pEnumFilters = NULL;
    IBDA_NetworkProvider * pNetworkProvider = NULL;
    IBaseFilter * ppFilter[1];
    PIN_INFO PinInfo;
    FILTER_INFO FilterInfo;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    if (!pUnkOuter)
        return E_POINTER;

    // query for IKsObject interface
    hr = pUnkOuter->QueryInterface(IID_IKsObject, (void**)&pObject);

    if (FAILED(hr))
        return hr;

    // query for IPin interface
    hr = pObject->QueryInterface(IID_IPin, (void**)&pPin);

    if (FAILED(hr))
    {
        //clean up
       pObject->Release();
       return hr;
    }

    // get pin info
    hr = pPin->QueryPinInfo(&PinInfo);

    if (FAILED(hr))
    {
        //clean up
       pObject->Release();
       pPin->Release();
       return hr;
    }

    // sanity checks
    assert(PinInfo.dir == PINDIR_OUTPUT);
    assert(PinInfo.pFilter != NULL);

    // query filter info
    hr = PinInfo.pFilter->QueryFilterInfo(&FilterInfo);

    // sanity check
    assert(FilterInfo.pGraph != NULL);

    // get IEnumFilters interface
    hr = FilterInfo.pGraph->EnumFilters(&pEnumFilters);

    if (FAILED(hr))
    {
        //clean up
       FilterInfo.pGraph->Release();
       PinInfo.pFilter->Release();
       pObject->Release();
       pPin->Release();
       return hr;
    }

    while(pEnumFilters->Next(1, ppFilter, NULL) == S_OK)
    {
        // check if that filter supports the IBDA_NetworkProvider interface
        hr = ppFilter[0]->QueryInterface(IID_IBDA_NetworkProvider, (void**)&pNetworkProvider);

        // release IBaseFilter
        ppFilter[0]->Release();

        if (SUCCEEDED(hr))
            break;
    }

    // release IEnumFilters interface
    pEnumFilters->Release();

    // release IFilterGraph interface
    FilterInfo.pGraph->Release();

    // release IBaseFilter interface
    PinInfo.pFilter->Release();

    if (pNetworkProvider)
    {
        // get connected pin handle
        hr = pPin->ConnectedTo(&pConnectedPin);

        // get file handle
        hFile = pObject->KsGetObjectHandle();

        if (FAILED(hr) || hFile == INVALID_HANDLE_VALUE)
        {
            // pin not connected
            pNetworkProvider->Release();
            // set zero
            pNetworkProvider = NULL;
        }
    }

    // release IPin 
    pPin->Release();

    // release IKsObject
    pObject->Release();


    if (pNetworkProvider == NULL)
    {
        // no network provider interface in graph
        return E_NOINTERFACE;
    }

    CBDAPinControl * handler = new CBDAPinControl(hFile, pNetworkProvider, pConnectedPin);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl_fnConstructor");
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
