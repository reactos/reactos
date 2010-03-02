/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/input_cpp.cpp
 * PURPOSE:         InputPin of Proxy Filter
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CInputPin : public IPin
/*
                  public IQualityControl,
                  public IKsObject,
                  public IKsPinEx,
                  public IKsPinPipe,
                  public ISpecifyPropertyPages,
                  public IStreamBuilder,
                  public IKsPropertySet,
                  public IKsPinFactory,
                  public IKsControl,
                  public IKsAggregateControl
*/
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

    CInputPin(IBaseFilter * ParentFilter, LPCWSTR PinName) : m_Ref(0), m_ParentFilter(ParentFilter), m_PinName(PinName){};
    virtual ~CInputPin(){};

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    LPCWSTR m_PinName;
};

HRESULT
STDMETHODCALLTYPE
CInputPin::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    *Output = NULL;
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IPin))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CInputPin::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IPin interface
//
HRESULT
STDMETHODCALLTYPE
CInputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CInputPin::Connect called\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CInputPin::ReceiveConnection called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::Disconnect( void)
{
    OutputDebugStringW(L"CInputPin::Disconnect called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::ConnectedTo(IPin **pPin)
{
    OutputDebugStringW(L"CInputPin::ConnectedTo called\n");
    return VFW_E_NOT_CONNECTED;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CInputPin::ConnectionMediaType called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::QueryPinInfo(PIN_INFO *pInfo)
{
    wcscpy(pInfo->achName, m_PinName);
    pInfo->dir = PINDIR_INPUT;
    pInfo->pFilter = m_ParentFilter;
    m_ParentFilter->AddRef();

    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::QueryDirection(PIN_DIRECTION *pPinDir)
{
    if (pPinDir)
    {
        *pPinDir = PINDIR_INPUT;
        return S_OK;
    }

    return E_POINTER;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::QueryId(LPWSTR *Id)
{
    *Id = (LPWSTR)CoTaskMemAlloc((wcslen(m_PinName)+1)*sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    wcscpy(*Id, m_PinName);
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CInputPin::QueryAccept called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    OutputDebugStringW(L"CInputPin::EnumMediaTypes called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    OutputDebugStringW(L"CInputPin::QueryInternalConnections called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EndOfStream( void)
{
    OutputDebugStringW(L"CInputPin::EndOfStream called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::BeginFlush( void)
{
    OutputDebugStringW(L"CInputPin::BeginFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EndFlush( void)
{
    OutputDebugStringW(L"CInputPin::EndFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    OutputDebugStringW(L"CInputPin::NewSegment called\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CInputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    REFIID riid,
    LPVOID * ppv)
{
    CInputPin * handler = new CInputPin(ParentFilter, PinName);

    if (!handler)
        return E_OUTOFMEMORY;

    if (FAILED(handler->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete handler;
        return E_NOINTERFACE;
    }

    return S_OK;
}
