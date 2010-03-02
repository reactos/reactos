/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/input_cpp.cpp
 * PURPOSE:         InputPin of Proxy Filter
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class COutputPin : public IPin
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
                  public IMediaSeeking,
                  public IAMStreamConfig,
                  public IMemAllocatorNotifyCallbackTemp
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

    COutputPin(IBaseFilter * ParentFilter, LPCWSTR PinName) : m_Ref(0), m_ParentFilter(ParentFilter), m_PinName(PinName){};
    virtual ~COutputPin(){};

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    LPCWSTR m_PinName;
};

HRESULT
STDMETHODCALLTYPE
COutputPin::QueryInterface(
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
    swprintf(Buffer, L"COutputPin::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IPin interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::Connect called\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::ReceiveConnection called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::Disconnect( void)
{
    OutputDebugStringW(L"COutputPin::Disconnect called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::ConnectedTo(IPin **pPin)
{
    OutputDebugStringW(L"COutputPin::ConnectedTo called\n");
    return VFW_E_NOT_CONNECTED;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::ConnectionMediaType called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::QueryPinInfo(PIN_INFO *pInfo)
{
    wcscpy(pInfo->achName, m_PinName);
    pInfo->dir = PINDIR_OUTPUT;
    pInfo->pFilter = m_ParentFilter;
    m_ParentFilter->AddRef();

    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::QueryDirection(PIN_DIRECTION *pPinDir)
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
COutputPin::QueryId(LPWSTR *Id)
{
    *Id = (LPWSTR)CoTaskMemAlloc((wcslen(m_PinName)+1)*sizeof(WCHAR));
    if (!*Id)
        return E_OUTOFMEMORY;

    wcscpy(*Id, m_PinName);
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::QueryAccept called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    OutputDebugStringW(L"COutputPin::EnumMediaTypes called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    OutputDebugStringW(L"COutputPin::QueryInternalConnections called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::EndOfStream( void)
{
    OutputDebugStringW(L"COutputPin::EndOfStream called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::BeginFlush( void)
{
    OutputDebugStringW(L"COutputPin::BeginFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::EndFlush( void)
{
    OutputDebugStringW(L"COutputPin::EndFlush called\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    OutputDebugStringW(L"COutputPin::NewSegment called\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
COutputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    REFIID riid,
    LPVOID * ppv)
{
    COutputPin * handler = new COutputPin(ParentFilter, PinName);

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
