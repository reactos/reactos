/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/pincontrol.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID KSPROPSETID_BdaPinControl = {0x0ded49d5, 0xa8b7, 0x4d5d, {0x97, 0xa1, 0x12, 0xb0, 0xc1, 0x95, 0x87, 0x4d}};
const GUID IID_IBDA_PinControl       = {0x0DED49D5, 0xA8B7, 0x4d5d, {0x97, 0xA1, 0x12, 0xB0, 0xC1, 0x95, 0x87, 0x4D}};
const GUID IID_IPin = {0x56a86891, 0x0ad4, 0x11ce, {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
#endif


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


    CBDAPinControl(HANDLE hFile, IBDA_NetworkProvider * pProvider, IPin * pConnectedPin, ULONG RegistrationCtx) : m_Ref(0), m_Handle(hFile), m_pProvider(pProvider), m_pConnectedPin(pConnectedPin), m_RegistrationCtx(RegistrationCtx){};
    virtual ~CBDAPinControl()
    {
        //m_pConnectedPin->Release();
        //m_pProvider->Release();
    };

protected:
    LONG m_Ref;
    HANDLE m_Handle;
    IBDA_NetworkProvider * m_pProvider;
    IPin * m_pConnectedPin;
    ULONG m_RegistrationCtx;
};

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::QueryInterface(
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
    if (IsEqualGUID(refiid, IID_IBDA_PinControl))
    {
        *Output = (IBDA_PinControl*)(this);
        reinterpret_cast<IBDA_PinControl*>(*Output)->AddRef();
        return NOERROR;
    }
DebugBreak();
    return E_NOINTERFACE;
}
//-------------------------------------------------------------------
// IBDA_PinControl methods
//
HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinID(ULONG *pulPinID)
{
    KSPROPERTY Property;
    ULONG BytesReturned;
    HRESULT hr;

    // setup request
    Property.Set = KSPROPSETID_BdaPinControl;
    Property.Id = KSPROPERTY_BDA_PIN_ID;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), pulPinID, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAPinControl::GetPinID: hr %lx pulPinID %lu BytesReturned %lx\n", hr, *pulPinID, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinType(ULONG *pulPinType)
{
    KSPROPERTY Property;
    ULONG BytesReturned;
    HRESULT hr;

    // setup request
    Property.Set = KSPROPSETID_BdaPinControl;
    Property.Id = KSPROPERTY_BDA_PIN_TYPE;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), pulPinType, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDAPinControl::GetPinType: hr %lx pulPinType %lu BytesReturned %lx\n", hr, *pulPinType, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::RegistrationContext(ULONG *pulRegistrationCtx)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl::RegistrationContext\n");
#endif

    if (!pulRegistrationCtx)
    {
        // invalid argument
        return E_POINTER;
    }

    if (m_RegistrationCtx)
    {
        // is registered
        *pulRegistrationCtx = m_RegistrationCtx;
        return NOERROR;
    }

    //pin not registered
    return E_FAIL;
}

//-------------------------------------------------------------------
HRESULT
GetNetworkProviderFromGraph(
    IFilterGraph * pGraph,
    IBDA_NetworkProvider ** pOutNetworkProvider)
{
    IEnumFilters *pEnumFilters = NULL;
    IBaseFilter * ppFilter[1];
    IBDA_NetworkProvider * pNetworkProvider = NULL;
    HRESULT hr;

    // get IEnumFilters interface
    hr = pGraph->EnumFilters(&pEnumFilters);

    if (FAILED(hr))
    {
        //clean up
        *pOutNetworkProvider = NULL;
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

    //store result
    *pOutNetworkProvider = pNetworkProvider;

    if (pNetworkProvider)
        return S_OK;
    else
        return E_FAIL;
}

HRESULT
CBDAPinControl_RealConstructor(
    HANDLE hPin,
    IBDA_NetworkProvider *pNetworkProvider,
    IPin * pConnectedPin,
    ULONG RegistrationCtx,
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    CBDAPinControl * handler = new CBDAPinControl(hPin, pNetworkProvider, pConnectedPin, RegistrationCtx);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDAPinControl_fnConstructor\n");
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

HRESULT
WINAPI
CBDAPinControl_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    IPin * pConnectedPin = NULL;
    IBDA_NetworkProvider * pNetworkProvider = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hr;
    IKsObject * pObject = NULL;
    IPin * pPin = NULL;
    IUnknown * pUnknown = NULL;
    PIN_INFO PinInfo;
    FILTER_INFO FilterInfo;
    ULONG RegistrationCtx = 0;

    if (!pUnkOuter)
        return E_POINTER;

    OutputDebugStringW(L"CBDAPinControl_fnConstructor\n");
    //DebugBreak();

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

    if (!PinInfo.pFilter)
    {
        //clean up
       pObject->Release();
       pPin->Release();
       return hr;
    }

    // sanity checks
    assert(PinInfo.pFilter != NULL);

    // query filter info
    hr = PinInfo.pFilter->QueryFilterInfo(&FilterInfo);

    // sanity check
    assert(FilterInfo.pGraph != NULL);

    // get network provider interface
    hr = GetNetworkProviderFromGraph(FilterInfo.pGraph, &pNetworkProvider);

    if (SUCCEEDED(hr))
    {
        if (PinInfo.dir == PINDIR_OUTPUT)
        {
            // get connected pin handle
            hr = pPin->ConnectedTo(&pConnectedPin);
            if (SUCCEEDED(hr))
            {
                // get file handle
                hFile = pObject->KsGetObjectHandle();
                if (hFile)
                {
                    hr = CBDAPinControl_RealConstructor(hFile, pNetworkProvider, pConnectedPin, 0, pUnkOuter, riid, ppv);
                    if (SUCCEEDED(hr))
                    {
                        // set to null to prevent releasing
                        pNetworkProvider = NULL;
                        pConnectedPin = NULL;
                    }
                }
                else
                {
                    // expected file handle
                    hr = E_UNEXPECTED;
                }
            }
        }
        else
        {
            // get IUnknown from base filter
            hr = PinInfo.pFilter->QueryInterface(IID_IUnknown, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                // register device filter
                OutputDebugStringW(L"CBDAPinControl_fnConstructor registering device filter with network provider\n");
                hr = pNetworkProvider->RegisterDeviceFilter(pUnknown, &RegistrationCtx);
                if (SUCCEEDED(hr))
                {
                    // get file handle
                    hFile = pObject->KsGetObjectHandle();
                    if (hFile)
                    {
                        hr = CBDAPinControl_RealConstructor(hFile, pNetworkProvider, NULL, RegistrationCtx, pUnkOuter, riid, ppv);
                        if (SUCCEEDED(hr))
                        {
                            // set to null to prevent releasing
                            pNetworkProvider = NULL;
                        }
                    }
                    else
                    {
                        // expected file handle
                        hr = E_UNEXPECTED;
                    }
                }
                else
                {
                    WCHAR Buffer[100];
                    swprintf(Buffer, L"CBDAPinControl_fnConstructor failed to register filter with %lx\n", hr);
                    OutputDebugStringW(Buffer);
                }
            }
        }
    }

    // release IFilterGraph interface
    FilterInfo.pGraph->Release();

    // release IBaseFilter interface
    PinInfo.pFilter->Release();

    // release IPin
    pPin->Release();

    // release IKsObject
    pObject->Release();


    if (pNetworkProvider)
    {
        // release network provider
        pNetworkProvider->Release();
    }

    if (pConnectedPin)
    {
        // release connected pin
        pConnectedPin->Release();
    }

    if (pUnknown)
    {
        // release filter
        pUnknown->Release();
    }

    return hr;
}
