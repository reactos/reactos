/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/pin.cpp
 * PURPOSE:         IPin interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

#ifndef _MSC_VER
const GUID KSDATAFORMAT_TYPE_BDA_ANTENNA = {0x71985f41, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID GUID_NULL                     = {0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif

class CPin : public IPin
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

    //IPin methods
    HRESULT STDMETHODCALLTYPE Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE Disconnect();
    HRESULT STDMETHODCALLTYPE ConnectedTo(IPin **pPin);
    HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO *pInfo);
    HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION *pPinDir);
    HRESULT STDMETHODCALLTYPE QueryId(LPWSTR *Id);
    HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes **ppEnum);
    HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin **apPin, ULONG *nPin);
    HRESULT STDMETHODCALLTYPE EndOfStream();
    HRESULT STDMETHODCALLTYPE BeginFlush();
    HRESULT STDMETHODCALLTYPE EndFlush();
    HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    CPin(IBaseFilter * ParentFilter);
    virtual ~CPin(){};

    static LPCWSTR PIN_ID;

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    AM_MEDIA_TYPE m_MediaType;
    IPin * m_Pin;
};


LPCWSTR CPin::PIN_ID = L"Antenna Out";


CPin::CPin(
    IBaseFilter * ParentFilter) : m_Ref(0),
                                  m_ParentFilter(ParentFilter),
                                  m_Pin(0)
{
    m_MediaType.majortype = KSDATAFORMAT_TYPE_BDA_ANTENNA;
    m_MediaType.subtype = MEDIASUBTYPE_None;
    m_MediaType.formattype = FORMAT_None;
    m_MediaType.bFixedSizeSamples = true;
    m_MediaType.bTemporalCompression = false;
    m_MediaType.lSampleSize = sizeof(CHAR);
    m_MediaType.pUnk = NULL;
    m_MediaType.cbFormat = 0;
    m_MediaType.pbFormat = NULL;
}


HRESULT
STDMETHODCALLTYPE
CPin::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IPin))
    {
        *Output = (IPin*)(this);
        reinterpret_cast<IPin*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CPin::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IPin interface
//
HRESULT
STDMETHODCALLTYPE
CPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    OutputDebugStringW(L"CPin::Connect called\n");

    if (pmt)
    {
        hr = pReceivePin->QueryAccept(pmt);
        if (FAILED(hr))
        {
            OutputDebugStringW(L"CPin::Connect QueryAccept failed\n");
            return hr;
        }
    }
    else
    {
        // query accept
        hr = pReceivePin->QueryAccept(&m_MediaType);
        if (FAILED(hr))
        {
            OutputDebugStringW(L"CPin::Connect QueryAccept pmt default failed\n");
            return hr;
        }

         pmt = &m_MediaType;
    }

    // receive connection;
    hr = pReceivePin->ReceiveConnection((IPin*)this, pmt);
    if (SUCCEEDED(hr))
    {
        // increment reference count
        pReceivePin->AddRef();
        m_Pin = pReceivePin;
        OutputDebugStringW(L"CPin::Connect success\n");
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    return E_UNEXPECTED;
}

HRESULT
STDMETHODCALLTYPE
CPin::Disconnect( void)
{
#ifdef MSDVBNP_TRACE
   OutputDebugStringW(L"CPin::Disconnect\n");
#endif

    if (!m_Pin)
    {
        // pin was not connected
        return S_FALSE;
    }

    m_Pin->Release();
    m_Pin = NULL;

    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CPin::ConnectedTo(IPin **pPin)
{
#ifdef MSDVBNP_TRACE
   OutputDebugStringW(L"CPin::ConnectedTo\n");
#endif

    if (!pPin)
        return E_POINTER;

    if (m_Pin)
    {
        // increment reference count
        m_Pin->AddRef();
        *pPin = m_Pin;
        return S_OK;
    }

    *pPin = NULL;
    return VFW_E_NOT_CONNECTED;
}
HRESULT
STDMETHODCALLTYPE
CPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CPin::ConnectionMediaType NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryPinInfo(PIN_INFO *pInfo)
{
    wcscpy(pInfo->achName, PIN_ID);
    pInfo->dir = PINDIR_OUTPUT;
    pInfo->pFilter = m_ParentFilter;
    m_ParentFilter->AddRef();

    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryDirection(PIN_DIRECTION *pPinDir)
{
    if (pPinDir)
    {
        *pPinDir = PINDIR_OUTPUT;
        return S_OK;
    }

    return E_POINTER;
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryId(LPWSTR *Id)
{
    *Id = (LPWSTR)CoTaskMemAlloc(sizeof(PIN_ID));
    if (!*Id)
        return E_OUTOFMEMORY;

    wcscpy(*Id, PIN_ID);
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CPin::QueryAccept NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    AM_MEDIA_TYPE *MediaType = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));

    if (!MediaType)
    {
        return E_OUTOFMEMORY;
    }

    MediaType->majortype = KSDATAFORMAT_TYPE_BDA_ANTENNA;
    MediaType->subtype = MEDIASUBTYPE_None;
    MediaType->formattype = FORMAT_None;
    MediaType->bFixedSizeSamples = true;
    MediaType->bTemporalCompression = false;
    MediaType->lSampleSize = sizeof(CHAR);
    MediaType->pUnk = NULL;
    MediaType->cbFormat = 0;
    MediaType->pbFormat = NULL;

    return CEnumMediaTypes_fnConstructor(NULL, 1, MediaType, IID_IEnumMediaTypes, (void**)ppEnum);
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    OutputDebugStringW(L"CPin::QueryInternalConnections NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::EndOfStream( void)
{
    OutputDebugStringW(L"CPin::EndOfStream NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::BeginFlush( void)
{
    OutputDebugStringW(L"CPin::BeginFlush NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::EndFlush( void)
{
    OutputDebugStringW(L"CPin::EndFlush NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    OutputDebugStringW(L"CPin::NewSegment NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CPin_fnConstructor(
    IUnknown *pUnknown,
    IBaseFilter * ParentFilter,
    REFIID riid,
    LPVOID * ppv)
{
    CPin * handler = new CPin(ParentFilter);

#ifdef MSDVBNP_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CPin_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
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
