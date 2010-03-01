/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/pin.cpp
 * PURPOSE:         IPin interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID KSDATAFORMAT_TYPE_BDA_ANTENNA = {0x71985f41, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID GUID_NULL                     = {0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

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
            //delete this;
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

    CPin(IBaseFilter * ParentFilter) : m_Ref(0), m_ParentFilter(ParentFilter){};
    virtual ~CPin(){};

    static LPCWSTR PIN_ID;

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
};


LPCWSTR CPin::PIN_ID = L"Antenna Out";

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
    OutputDebugStringW(L"CPin::Connect called\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CPin::ReceiveConnection called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::Disconnect( void)
{
    OutputDebugStringW(L"CPin::Disconnect called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::ConnectedTo(IPin **pPin)
{
    OutputDebugStringW(L"CPin::ConnectedTo called\n");
    return VFW_E_NOT_CONNECTED;
}
HRESULT
STDMETHODCALLTYPE
CPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CPin::ConnectionMediaType called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::QueryPinInfo(PIN_INFO *pInfo)
{
    wcscpy(pInfo->achName, PIN_ID);
    pInfo->dir = PINDIR_OUTPUT;
    pInfo->pFilter = m_ParentFilter;

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
    OutputDebugStringW(L"CPin::QueryAccept called\n");
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
    MediaType->subtype = GUID_NULL;
    MediaType->formattype = GUID_NULL;
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
    OutputDebugStringW(L"CPin::QueryInternalConnections called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::EndOfStream( void)
{
    OutputDebugStringW(L"CPin::EndOfStream called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::BeginFlush( void)
{
    OutputDebugStringW(L"CPin::BeginFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::EndFlush( void)
{
    OutputDebugStringW(L"CPin::EndFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    OutputDebugStringW(L"CPin::NewSegment called\n");
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