/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/output_pin.cpp
 * PURPOSE:         OutputPin of Proxy Filter
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class COutputPin : public IPin,
                   public IKsObject,
                   public IKsPropertySet,
                   public IStreamBuilder,
                   public IKsPinFactory,
                   public ISpecifyPropertyPages,
                   public IKsPinEx,
                   public IKsPinPipe,
                   public IKsControl,
                   public IKsAggregateControl,
                   public IQualityControl,
                   public IMediaSeeking,
                   public IAMBufferNegotiation,
                   public IAMStreamConfig,
                   public IMemAllocatorNotifyCallbackTemp

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

    //IKsPin
    HRESULT STDMETHODCALLTYPE KsQueryMediums(PKSMULTIPLE_ITEM* MediumList);
    HRESULT STDMETHODCALLTYPE KsQueryInterfaces(PKSMULTIPLE_ITEM* InterfaceList);
    HRESULT STDMETHODCALLTYPE KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium);
    HRESULT STDMETHODCALLTYPE KsGetCurrentCommunication(KSPIN_COMMUNICATION *Communication, KSPIN_INTERFACE *Interface, KSPIN_MEDIUM *Medium);
    HRESULT STDMETHODCALLTYPE KsPropagateAcquire();
    HRESULT STDMETHODCALLTYPE KsDeliver(IMediaSample* Sample, ULONG Flags);
    HRESULT STDMETHODCALLTYPE KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment);
    IMemAllocator * STDMETHODCALLTYPE KsPeekAllocator(KSPEEKOPERATION Operation);
    HRESULT STDMETHODCALLTYPE KsReceiveAllocator(IMemAllocator *MemAllocator);
    HRESULT STDMETHODCALLTYPE KsRenegotiateAllocator();
    LONG STDMETHODCALLTYPE KsIncrementPendingIoCount();
    LONG STDMETHODCALLTYPE KsDecrementPendingIoCount();
    HRESULT STDMETHODCALLTYPE KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta);
    // IKsPinEx
    VOID STDMETHODCALLTYPE KsNotifyError(IMediaSample* Sample, HRESULT hr);

    //IKsPinPipe
    HRESULT STDMETHODCALLTYPE KsGetPinFramingCache(PKSALLOCATOR_FRAMING_EX *FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    HRESULT STDMETHODCALLTYPE KsSetPinFramingCache(PKSALLOCATOR_FRAMING_EX FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    IPin* STDMETHODCALLTYPE KsGetConnectedPin();
    IKsAllocatorEx* STDMETHODCALLTYPE KsGetPipe(KSPEEKOPERATION Operation);
    HRESULT STDMETHODCALLTYPE KsSetPipe(IKsAllocatorEx *KsAllocator);
    ULONG STDMETHODCALLTYPE KsGetPipeAllocatorFlag();
    HRESULT STDMETHODCALLTYPE KsSetPipeAllocatorFlag(ULONG Flag);
    GUID STDMETHODCALLTYPE KsGetPinBusCache();
    HRESULT STDMETHODCALLTYPE KsSetPinBusCache(GUID Bus);
    PWCHAR STDMETHODCALLTYPE KsGetPinName();
    PWCHAR STDMETHODCALLTYPE KsGetFilterName();

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

    // ISpecifyPropertyPages
    HRESULT STDMETHODCALLTYPE GetPages(CAUUID *pPages);

    //IKsObject methods
    HANDLE STDMETHODCALLTYPE KsGetObjectHandle();

    //IKsPropertySet
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

    //IKsControl
    HRESULT STDMETHODCALLTYPE KsProperty(PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsMethod(PKSMETHOD Method, ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsEvent(PKSEVENT Event, ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned);

    //IStreamBuilder
    HRESULT STDMETHODCALLTYPE Render(IPin *ppinOut, IGraphBuilder *pGraph);
    HRESULT STDMETHODCALLTYPE Backout(IPin *ppinOut, IGraphBuilder *pGraph);

    //IKsPinFactory
    HRESULT STDMETHODCALLTYPE KsPinFactory(ULONG* PinFactory);

    //IKsAggregateControl
    HRESULT STDMETHODCALLTYPE KsAddAggregate(IN REFGUID AggregateClass);
    HRESULT STDMETHODCALLTYPE KsRemoveAggregate(REFGUID AggregateClass);

    //IQualityControl
    HRESULT STDMETHODCALLTYPE Notify(IBaseFilter *pSelf, Quality q);
    HRESULT STDMETHODCALLTYPE SetSink(IQualityControl *piqc);

    //IMediaSeeking
    HRESULT STDMETHODCALLTYPE GetCapabilities(DWORD *pCapabilities);
    HRESULT STDMETHODCALLTYPE CheckCapabilities(DWORD *pCapabilities);
    HRESULT STDMETHODCALLTYPE IsFormatSupported(const GUID *pFormat);
    HRESULT STDMETHODCALLTYPE QueryPreferredFormat(GUID *pFormat);
    HRESULT STDMETHODCALLTYPE GetTimeFormat(GUID *pFormat);
    HRESULT STDMETHODCALLTYPE IsUsingTimeFormat(const GUID *pFormat);
    HRESULT STDMETHODCALLTYPE SetTimeFormat(const GUID *pFormat);
    HRESULT STDMETHODCALLTYPE GetDuration(LONGLONG *pDuration);
    HRESULT STDMETHODCALLTYPE GetStopPosition(LONGLONG *pStop);
    HRESULT STDMETHODCALLTYPE GetCurrentPosition(LONGLONG *pCurrent);
    HRESULT STDMETHODCALLTYPE ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat);
    HRESULT STDMETHODCALLTYPE SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags);
    HRESULT STDMETHODCALLTYPE GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
    HRESULT STDMETHODCALLTYPE GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
    HRESULT STDMETHODCALLTYPE SetRate(double dRate);
    HRESULT STDMETHODCALLTYPE GetRate(double *pdRate);
    HRESULT STDMETHODCALLTYPE GetPreroll(LONGLONG *pllPreroll);

    //IAMBufferNegotiation
    HRESULT STDMETHODCALLTYPE SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *pprop);
    HRESULT STDMETHODCALLTYPE GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop);

    //IAMStreamConfig
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);

    //IMemAllocatorNotifyCallbackTemp
    HRESULT STDMETHODCALLTYPE NotifyRelease();

    //---------------------------------------------------------------
    COutputPin(IBaseFilter * ParentFilter, LPCWSTR PinName, ULONG PinId, KSPIN_COMMUNICATION Communication);
    virtual ~COutputPin();
    HRESULT STDMETHODCALLTYPE CheckFormat(const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE CreatePin(const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE CreatePinHandle(PKSPIN_MEDIUM Medium, PKSPIN_INTERFACE Interface, const AM_MEDIA_TYPE *pmt);
    HRESULT WINAPI IoProcessRoutine();
    HRESULT WINAPI InitializeIOThread();

    friend DWORD WINAPI COutputPin_IoThreadStartup(LPVOID lpParameter);
    friend HRESULT STDMETHODCALLTYPE COutputPin_SetState(IPin * Pin, KSSTATE State);

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    LPCWSTR m_PinName;
    HANDLE m_hPin;
    ULONG m_PinId;
    IPin * m_Pin;
    IKsAllocatorEx * m_KsAllocatorEx;
    ULONG m_PipeAllocatorFlag;
    BOOL m_bPinBusCacheInitialized;
    GUID m_PinBusCache;
    LPWSTR m_FilterName;
    FRAMING_PROP m_FramingProp[4];
    PKSALLOCATOR_FRAMING_EX m_FramingEx[4];

    IMemAllocator * m_MemAllocator;
    IMemInputPin * m_MemInputPin;
    LONG m_IoCount;
    KSPIN_COMMUNICATION m_Communication;
    KSPIN_INTERFACE m_Interface;
    KSPIN_MEDIUM m_Medium;
    AM_MEDIA_TYPE m_MediaFormat;
    ALLOCATOR_PROPERTIES m_Properties;
    IKsInterfaceHandler * m_InterfaceHandler;

    HANDLE m_hStartEvent;
    HANDLE m_hBufferAvailable;
    HANDLE m_hStopEvent;
    BOOL m_StopInProgress;
    BOOL m_IoThreadStarted;

    KSSTATE m_State;
    CRITICAL_SECTION m_Lock;
};

COutputPin::~COutputPin()
{
}

COutputPin::COutputPin(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    ULONG PinId,
    KSPIN_COMMUNICATION Communication) : m_Ref(0),
                                         m_ParentFilter(ParentFilter),
                                         m_PinName(PinName),
                                         m_hPin(INVALID_HANDLE_VALUE),
                                         m_PinId(PinId),
                                         m_Pin(0),
                                         m_KsAllocatorEx(0),
                                         m_PipeAllocatorFlag(0),
                                         m_bPinBusCacheInitialized(0),
                                         m_FilterName(0),
                                         m_MemAllocator(0),
                                         m_MemInputPin(0),
                                         m_IoCount(0),
                                         m_Communication(Communication),
                                         m_InterfaceHandler(0),
                                         m_hStartEvent(0),
                                         m_hBufferAvailable(0),
                                         m_hStopEvent(0),
                                         m_StopInProgress(0),
                                         m_IoThreadStarted(0),
                                         m_State(KSSTATE_STOP)
{
    HRESULT hr;
    IKsObject * KsObjectParent;

    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    assert(hr == S_OK);

    ZeroMemory(m_FramingProp, sizeof(m_FramingProp));
    ZeroMemory(m_FramingEx, sizeof(m_FramingEx));

    hr = KsGetMediaType(0, &m_MediaFormat, KsObjectParent->KsGetObjectHandle(), m_PinId);
    assert(hr == S_OK);

    InitializeCriticalSection(&m_Lock);

    KsObjectParent->Release();
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
        OutputDebugStringW(L"COutputPin::QueryInterface IID_IPin\n");
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsObject))
    {
        if (m_hPin == INVALID_HANDLE_VALUE)
        {
            HRESULT hr = CreatePin(&m_MediaFormat);
            if (FAILED(hr))
                return hr;
        }

        OutputDebugStringW(L"COutputPin::QueryInterface IID_IKsObject\n");
        *Output = (IKsObject*)(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPin) || IsEqualGUID(refiid, IID_IKsPinEx))
    {
        *Output = (IKsPinEx*)(this);
        reinterpret_cast<IKsPinEx*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPinPipe))
    {
        *Output = (IKsPinPipe*)(this);
        reinterpret_cast<IKsPinPipe*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsAggregateControl))
    {
        *Output = (IKsAggregateControl*)(this);
        reinterpret_cast<IKsAggregateControl*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IQualityControl))
    {
        *Output = (IQualityControl*)(this);
        reinterpret_cast<IQualityControl*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPropertySet))
    {
        if (m_hPin == INVALID_HANDLE_VALUE)
        {
            HRESULT hr = CreatePin(&m_MediaFormat);
            if (FAILED(hr))
                return hr;
        }
        OutputDebugStringW(L"COutputPin::QueryInterface IID_IKsPropertySet\n");
        *Output = (IKsPropertySet*)(this);
        reinterpret_cast<IKsPropertySet*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsControl))
    {
        OutputDebugStringW(L"COutputPin::QueryInterface IID_IKsControl\n");
        *Output = (IKsControl*)(this);
        reinterpret_cast<IKsControl*>(*Output)->AddRef();
        return NOERROR;
    }
#if 0
    else if (IsEqualGUID(refiid, IID_IStreamBuilder))
    {
        *Output = (IStreamBuilder*)(this);
        reinterpret_cast<IStreamBuilder*>(*Output)->AddRef();
        return NOERROR;
    }
#endif
    else if (IsEqualGUID(refiid, IID_IKsPinFactory))
    {
        OutputDebugStringW(L"COutputPin::QueryInterface IID_IKsPinFactory\n");
        *Output = (IKsPinFactory*)(this);
        reinterpret_cast<IKsPinFactory*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_ISpecifyPropertyPages))
    {
        OutputDebugStringW(L"COutputPin::QueryInterface IID_ISpecifyPropertyPages\n");
        *Output = (ISpecifyPropertyPages*)(this);
        reinterpret_cast<ISpecifyPropertyPages*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IMediaSeeking))
    {
        *Output = (IMediaSeeking*)(this);
        reinterpret_cast<IMediaSeeking*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IAMBufferNegotiation))
    {
        *Output = (IAMBufferNegotiation*)(this);
        reinterpret_cast<IAMBufferNegotiation*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IAMStreamConfig))
    {
        *Output = (IAMStreamConfig*)(this);
        reinterpret_cast<IAMStreamConfig*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IMemAllocatorNotifyCallbackTemp))
    {
        *Output = (IMemAllocatorNotifyCallbackTemp*)(this);
        reinterpret_cast<IMemAllocatorNotifyCallbackTemp*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"COutputPin::QueryInterface: NoInterface for %s PinId %u PinName %s\n", lpstr, m_PinId, m_PinName);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IAMBufferNegotiation interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::SuggestAllocatorProperties(
    const ALLOCATOR_PROPERTIES *pprop)
{
    OutputDebugStringW(L"COutputPin::SuggestAllocatorProperties\n");

    if (m_Pin)
    {
        // pin is already connected
        return VFW_E_ALREADY_CONNECTED;
    }

    CopyMemory(&m_Properties, pprop, sizeof(ALLOCATOR_PROPERTIES));
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetAllocatorProperties(
    ALLOCATOR_PROPERTIES *pprop)
{
    OutputDebugStringW(L"COutputPin::GetAllocatorProperties\n");

    if (!m_Pin)
    {
        // you should call this method AFTER you connected
        return E_UNEXPECTED;
    }

    if (!m_KsAllocatorEx)
    {
        // something went wrong while creating the allocator
        return E_FAIL;
    }

    CopyMemory(pprop, &m_Properties, sizeof(ALLOCATOR_PROPERTIES));
    return NOERROR;
}

//-------------------------------------------------------------------
// IAMStreamConfig interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::SetFormat(
    AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::SetFormat NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    OutputDebugStringW(L"COutputPin::GetFormat NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetNumberOfCapabilities(
    int *piCount,
    int *piSize)
{
    OutputDebugStringW(L"COutputPin::GetNumberOfCapabilities NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetStreamCaps(
    int iIndex,
    AM_MEDIA_TYPE **ppmt,
    BYTE *pSCC)
{
    OutputDebugStringW(L"COutputPin::GetStreamCaps NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IMemAllocatorNotifyCallbackTemp interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::NotifyRelease()
{
    OutputDebugStringW(L"COutputPin::NotifyRelease\n");

    // notify thread of new available sample
    SetEvent(m_hBufferAvailable);

    return NOERROR;
}

//-------------------------------------------------------------------
// IMediaSeeking interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::GetCapabilities(
    DWORD *pCapabilities)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetCapabilities(pCapabilities);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::CheckCapabilities(
    DWORD *pCapabilities)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->CheckCapabilities(pCapabilities);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::IsFormatSupported(
    const GUID *pFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->IsFormatSupported(pFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::QueryPreferredFormat(
    GUID *pFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->QueryPreferredFormat(pFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetTimeFormat(
    GUID *pFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetTimeFormat(pFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::IsUsingTimeFormat(
    const GUID *pFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->IsUsingTimeFormat(pFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetTimeFormat(
    const GUID *pFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->SetTimeFormat(pFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetDuration(
    LONGLONG *pDuration)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetDuration(pDuration);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetStopPosition(
    LONGLONG *pStop)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetStopPosition(pStop);

    FilterMediaSeeking->Release();
    return hr;
}


HRESULT
STDMETHODCALLTYPE
COutputPin::GetCurrentPosition(
    LONGLONG *pCurrent)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetCurrentPosition(pCurrent);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::ConvertTimeFormat(
    LONGLONG *pTarget,
    const GUID *pTargetFormat,
    LONGLONG Source,
    const GUID *pSourceFormat)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetPositions(
    LONGLONG *pCurrent,
    DWORD dwCurrentFlags,
    LONGLONG *pStop,
    DWORD dwStopFlags)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetPositions(
    LONGLONG *pCurrent,
    LONGLONG *pStop)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetPositions(pCurrent, pStop);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetAvailable(
    LONGLONG *pEarliest,
    LONGLONG *pLatest)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetAvailable(pEarliest, pLatest);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetRate(
    double dRate)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->SetRate(dRate);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetRate(
    double *pdRate)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetRate(pdRate);

    FilterMediaSeeking->Release();
    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetPreroll(
    LONGLONG *pllPreroll)
{
    IMediaSeeking * FilterMediaSeeking;
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&FilterMediaSeeking);
    if (FAILED(hr))
        return hr;

    hr = FilterMediaSeeking->GetPreroll(pllPreroll);

    FilterMediaSeeking->Release();
    return hr;
}

//-------------------------------------------------------------------
// IQualityControl interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::Notify(
    IBaseFilter *pSelf,
    Quality q)
{
    OutputDebugStringW(L"COutputPin::Notify NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetSink(
    IQualityControl *piqc)
{
    OutputDebugStringW(L"COutputPin::SetSink NotImplemented\n");
    return E_NOTIMPL;
}


//-------------------------------------------------------------------
// IKsAggregateControl interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::KsAddAggregate(
    IN REFGUID AggregateClass)
{
    OutputDebugStringW(L"COutputPin::KsAddAggregate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsRemoveAggregate(
    REFGUID AggregateClass)
{
    OutputDebugStringW(L"COutputPin::KsRemoveAggregate NotImplemented\n");
    return E_NOTIMPL;
}


//-------------------------------------------------------------------
// IKsPin
//

HRESULT
STDMETHODCALLTYPE
COutputPin::KsQueryMediums(
    PKSMULTIPLE_ITEM* MediumList)
{
    HRESULT hr;
    HANDLE hFilter;
    IKsObject * KsObjectParent;

    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    if (FAILED(hr))
        return E_NOINTERFACE;

    hFilter = KsObjectParent->KsGetObjectHandle();

    if (hFilter)
        hr = KsGetMultiplePinFactoryItems(hFilter, m_PinId, KSPROPERTY_PIN_MEDIUMS, (PVOID*)MediumList);
    else
        hr = E_HANDLE;

    KsObjectParent->Release();

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList)
{
    HRESULT hr;
    HANDLE hFilter;
    IKsObject * KsObjectParent;

    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    if (FAILED(hr))
        return hr;

    hFilter = KsObjectParent->KsGetObjectHandle();

    if (hFilter)
        hr = KsGetMultiplePinFactoryItems(hFilter, m_PinId, KSPROPERTY_PIN_INTERFACES, (PVOID*)InterfaceList);
    else
        hr = E_HANDLE;

    KsObjectParent->Release();

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsCreateSinkPinHandle(
    KSPIN_INTERFACE& Interface,
    KSPIN_MEDIUM& Medium)
{
    OutputDebugStringW(L"COutputPin::KsCreateSinkPinHandle NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsGetCurrentCommunication(
    KSPIN_COMMUNICATION *Communication,
    KSPIN_INTERFACE *Interface,
    KSPIN_MEDIUM *Medium)
{
    if (Communication)
    {
        *Communication = m_Communication;
    }

    if (Interface)
    {
        if (!m_hPin)
            return VFW_E_NOT_CONNECTED;

        CopyMemory(Interface, &m_Interface, sizeof(KSPIN_INTERFACE));
    }

    if (Medium)
    {
        if (!m_hPin)
            return VFW_E_NOT_CONNECTED;

        CopyMemory(Medium, &m_Medium, sizeof(KSPIN_MEDIUM));
    }
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsPropagateAcquire()
{
    KSPROPERTY Property;
    KSSTATE State;
    ULONG BytesReturned;
    HRESULT hr;

    OutputDebugStringW(L"COutputPin::KsPropagateAcquire\n");

    assert(m_hPin != INVALID_HANDLE_VALUE);

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    State = KSSTATE_ACQUIRE;

    hr = KsProperty(&Property, sizeof(KSPROPERTY), (LPVOID)&State, sizeof(KSSTATE), &BytesReturned);

    //TODO
    //propagate to connected pin on the pipe

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsDeliver(
    IMediaSample* Sample,
    ULONG Flags)
{
    return E_FAIL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment)
{
    return NOERROR;
}

IMemAllocator * 
STDMETHODCALLTYPE
COutputPin::KsPeekAllocator(KSPEEKOPERATION Operation)
{
    if (Operation == KsPeekOperation_AddRef)
    {
        // add reference on allocator
        m_MemAllocator->AddRef();
    }

    return m_MemAllocator;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsReceiveAllocator(IMemAllocator *MemAllocator)
{
    if (MemAllocator)
    {
        MemAllocator->AddRef();
    }

    if (m_MemAllocator)
    {
        m_MemAllocator->Release();
    }

    m_MemAllocator = MemAllocator;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsRenegotiateAllocator()
{
    return E_FAIL;
}

LONG
STDMETHODCALLTYPE
COutputPin::KsIncrementPendingIoCount()
{
    return InterlockedIncrement((volatile LONG*)&m_IoCount);
}

LONG
STDMETHODCALLTYPE
COutputPin::KsDecrementPendingIoCount()
{
    return InterlockedDecrement((volatile LONG*)&m_IoCount);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsQualityNotify(
    ULONG Proportion,
    REFERENCE_TIME TimeDelta)
{
    OutputDebugStringW(L"COutputPin::KsQualityNotify NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IKsPinEx
//

VOID
STDMETHODCALLTYPE
COutputPin::KsNotifyError(
    IMediaSample* Sample,
    HRESULT hr)
{
    OutputDebugStringW(L"COutputPin::KsNotifyError NotImplemented\n");
}


//-------------------------------------------------------------------
// IKsPinPipe
//

HRESULT
STDMETHODCALLTYPE
COutputPin::KsGetPinFramingCache(
    PKSALLOCATOR_FRAMING_EX *FramingEx,
    PFRAMING_PROP FramingProp,
    FRAMING_CACHE_OPS Option)
{
    if (Option > Framing_Cache_Write || Option < Framing_Cache_ReadLast)
    {
        // invalid argument
        return E_INVALIDARG;
    }

    // get framing properties
    *FramingProp = m_FramingProp[Option];
    *FramingEx = m_FramingEx[Option];

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsSetPinFramingCache(
    PKSALLOCATOR_FRAMING_EX FramingEx,
    PFRAMING_PROP FramingProp,
    FRAMING_CACHE_OPS Option)
{
    ULONG Index;
    ULONG RefCount = 0;

    if (m_FramingEx[Option])
    {
        for(Index = 1; Index < 4; Index++)
        {
            if (m_FramingEx[Index] == m_FramingEx[Option])
                RefCount++;
        }

        if (RefCount == 1)
        {
            // existing framing is only used once
            CoTaskMemFree(m_FramingEx[Option]);
        }
    }

    // store framing
    m_FramingEx[Option] = FramingEx;
    m_FramingProp[Option] = *FramingProp;

    return S_OK;
}

IPin*
STDMETHODCALLTYPE
COutputPin::KsGetConnectedPin()
{
    return m_Pin;
}

IKsAllocatorEx*
STDMETHODCALLTYPE
COutputPin::KsGetPipe(
    KSPEEKOPERATION Operation)
{
    if (Operation == KsPeekOperation_AddRef)
    {
        if (m_KsAllocatorEx)
            m_KsAllocatorEx->AddRef();
    }
    return m_KsAllocatorEx;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsSetPipe(
    IKsAllocatorEx *KsAllocator)
{
    if (KsAllocator)
        KsAllocator->AddRef();

    if (m_KsAllocatorEx)
        m_KsAllocatorEx->Release();

    m_KsAllocatorEx = KsAllocator;
    return NOERROR;
}

ULONG
STDMETHODCALLTYPE
COutputPin::KsGetPipeAllocatorFlag()
{
    return m_PipeAllocatorFlag;
}


HRESULT
STDMETHODCALLTYPE
COutputPin::KsSetPipeAllocatorFlag(
    ULONG Flag)
{
    m_PipeAllocatorFlag = Flag;
    return NOERROR;
}

GUID
STDMETHODCALLTYPE
COutputPin::KsGetPinBusCache()
{
    if (!m_bPinBusCacheInitialized)
    {
        CopyMemory(&m_PinBusCache, &m_Medium.Set, sizeof(GUID));
        m_bPinBusCacheInitialized = TRUE;
    }

    return m_PinBusCache;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsSetPinBusCache(
    GUID Bus)
{
    CopyMemory(&m_PinBusCache, &Bus, sizeof(GUID));
    return NOERROR;
}

PWCHAR
STDMETHODCALLTYPE
COutputPin::KsGetPinName()
{
    return (PWCHAR)m_PinName;
}


PWCHAR
STDMETHODCALLTYPE
COutputPin::KsGetFilterName()
{
    return m_FilterName;
}

//-------------------------------------------------------------------
// ISpecifyPropertyPages
//

HRESULT
STDMETHODCALLTYPE
COutputPin::GetPages(CAUUID *pPages)
{
    OutputDebugStringW(L"COutputPin::GetPages NotImplemented\n");

    if (!pPages)
        return E_POINTER;

    pPages->cElems = 0;
    pPages->pElems = NULL;

    return S_OK;
}

//-------------------------------------------------------------------
// IKsPinFactory
//

HRESULT
STDMETHODCALLTYPE
COutputPin::KsPinFactory(
    ULONG* PinFactory)
{
    OutputDebugStringW(L"COutputPin::KsPinFactory\n");
    *PinFactory = m_PinId;
    return S_OK;
}


//-------------------------------------------------------------------
// IStreamBuilder
//

HRESULT
STDMETHODCALLTYPE
COutputPin::Render(
    IPin *ppinOut,
    IGraphBuilder *pGraph)
{
    OutputDebugStringW(L"COutputPin::Render\n");
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::Backout(
    IPin *ppinOut, 
    IGraphBuilder *pGraph)
{
    OutputDebugStringW(L"COutputPin::Backout\n");
    return S_OK;
}
//-------------------------------------------------------------------
// IKsObject
//
HANDLE
STDMETHODCALLTYPE
COutputPin::KsGetObjectHandle()
{
    OutputDebugStringW(L"COutputPin::KsGetObjectHandle\n");
    assert(m_hPin != INVALID_HANDLE_VALUE);
    return m_hPin;
}

//-------------------------------------------------------------------
// IKsControl
//
HRESULT
STDMETHODCALLTYPE
COutputPin::KsProperty(
    PKSPROPERTY Property,
    ULONG PropertyLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    HRESULT hr;
    WCHAR Buffer[100];
    LPOLESTR pstr;

    assert(m_hPin != INVALID_HANDLE_VALUE);

    hr = KsSynchronousDeviceControl(m_hPin, IOCTL_KS_PROPERTY, (PVOID)Property, PropertyLength, (PVOID)PropertyData, DataLength, BytesReturned);

    StringFromCLSID(Property->Set, &pstr);
    swprintf(Buffer, L"COutputPin::KsProperty Set %s Id %lu Flags %x hr %x\n", pstr, Property->Id, Property->Flags, hr);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsMethod(
    PKSMETHOD Method,
    ULONG MethodLength,
    LPVOID MethodData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_hPin != INVALID_HANDLE_VALUE);
    OutputDebugStringW(L"COutputPin::KsMethod\n");
    return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_METHOD, (PVOID)Method, MethodLength, (PVOID)MethodData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsEvent(
    PKSEVENT Event,
    ULONG EventLength,
    LPVOID EventData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_hPin != INVALID_HANDLE_VALUE);

    OutputDebugStringW(L"COutputPin::KsEvent\n");

    if (EventLength)
        return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_ENABLE_EVENT, (PVOID)Event, EventLength, (PVOID)EventData, DataLength, BytesReturned);
    else
        return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_DISABLE_EVENT, (PVOID)Event, EventLength, NULL, 0, BytesReturned);
}


//-------------------------------------------------------------------
// IKsPropertySet
//
HRESULT
STDMETHODCALLTYPE
COutputPin::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData)
{
    ULONG BytesReturned;

    if (cbInstanceData)
    {
        PKSPROPERTY Property = (PKSPROPERTY)CoTaskMemAlloc(sizeof(KSPROPERTY) + cbInstanceData);
        if (!Property)
            return E_OUTOFMEMORY;

        Property->Set = guidPropSet;
        Property->Id = dwPropID;
        Property->Flags = KSPROPERTY_TYPE_SET;

        CopyMemory((Property+1), pInstanceData, cbInstanceData);

        HRESULT hr = KsProperty(Property, sizeof(KSPROPERTY) + cbInstanceData, pPropData, cbPropData, &BytesReturned);
        CoTaskMemFree(Property);
        return hr;
    }
    else
    {
        KSPROPERTY Property;

        Property.Set = guidPropSet;
        Property.Id = dwPropID;
        Property.Flags = KSPROPERTY_TYPE_SET;

        HRESULT hr = KsProperty(&Property, sizeof(KSPROPERTY), pPropData, cbPropData, &BytesReturned);
        return hr;
    }
}

HRESULT
STDMETHODCALLTYPE
COutputPin::Get(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned)
{
    ULONG BytesReturned;

    if (cbInstanceData)
    {
        PKSPROPERTY Property = (PKSPROPERTY)CoTaskMemAlloc(sizeof(KSPROPERTY) + cbInstanceData);
        if (!Property)
            return E_OUTOFMEMORY;

        Property->Set = guidPropSet;
        Property->Id = dwPropID;
        Property->Flags = KSPROPERTY_TYPE_GET;

        CopyMemory((Property+1), pInstanceData, cbInstanceData);

        HRESULT hr = KsProperty(Property, sizeof(KSPROPERTY) + cbInstanceData, pPropData, cbPropData, &BytesReturned);
        CoTaskMemFree(Property);
        return hr;
    }
    else
    {
        KSPROPERTY Property;

        Property.Set = guidPropSet;
        Property.Id = dwPropID;
        Property.Flags = KSPROPERTY_TYPE_GET;

        HRESULT hr = KsProperty(&Property, sizeof(KSPROPERTY), pPropData, cbPropData, &BytesReturned);
        return hr;
    }
}

HRESULT
STDMETHODCALLTYPE
COutputPin::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    OutputDebugStringW(L"COutputPin::QuerySupported\n");

    Property.Set = guidPropSet;
    Property.Id = dwPropID;
    Property.Flags = KSPROPERTY_TYPE_SETSUPPORT;

    return KsProperty(&Property, sizeof(KSPROPERTY), pTypeSupport, sizeof(DWORD), &BytesReturned);
}


//-------------------------------------------------------------------
// IPin interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    ALLOCATOR_PROPERTIES Properties;
    IMemAllocatorCallbackTemp *pMemCallback;
    WCHAR Buffer[200];

    OutputDebugStringW(L"COutputPin::Connect called\n");
    if (pmt)
    {
        hr = pReceivePin->QueryAccept(pmt);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        // query accept
        hr = pReceivePin->QueryAccept(&m_MediaFormat);
        if (FAILED(hr))
            return hr;

         pmt = &m_MediaFormat;
    }

    // query for IMemInput interface
    hr = pReceivePin->QueryInterface(IID_IMemInputPin, (void**)&m_MemInputPin);
    if (FAILED(hr))
    {
        OutputDebugStringW(L"COutputPin::Connect no IMemInputPin interface\n");
        DebugBreak();
        return hr;
    }

    // get input pin allocator properties
    ZeroMemory(&Properties, sizeof(ALLOCATOR_PROPERTIES));
    m_MemInputPin->GetAllocatorRequirements(&Properties);

    //FIXME determine allocator properties
    Properties.cBuffers = 32;
    Properties.cbBuffer = 2048 * 188; //2048 frames * MPEG2 TS Payload size
    Properties.cbAlign = 4;

    // get input pin allocator
#if 0
    hr = m_MemInputPin->GetAllocator(&m_MemAllocator);
    if (SUCCEEDED(hr))
    {
        // set allocator properties
        hr = m_MemAllocator->SetProperties(&Properties, &m_Properties);
        if (FAILED(hr))
            m_MemAllocator->Release();
    }
#endif

    if (1)
    {
        hr = CKsAllocator_Constructor(NULL, IID_IMemAllocator, (void**)&m_MemAllocator);
        if (FAILED(hr))
            return hr;

        // set allocator properties
        hr = m_MemAllocator->SetProperties(&Properties, &m_Properties);
        if (FAILED(hr))
        {
            swprintf(Buffer, L"COutputPin::Connect IMemAllocator::SetProperties failed with hr %lx\n", hr);
            OutputDebugStringW(Buffer);
            m_MemAllocator->Release();
            m_MemInputPin->Release();
            return hr;
        }
    }

    // commit property changes
    hr = m_MemAllocator->Commit();
    if (FAILED(hr))
    {
        swprintf(Buffer, L"COutputPin::Connect IMemAllocator::Commit failed with hr %lx\n", hr);
        OutputDebugStringW(Buffer);
        m_MemAllocator->Release();
        m_MemInputPin->Release();
        return hr;
    }

    // get callback interface
    hr = m_MemAllocator->QueryInterface(IID_IMemAllocatorCallbackTemp, (void**)&pMemCallback);
    if (FAILED(hr))
    {
        swprintf(Buffer, L"COutputPin::Connect No IMemAllocatorCallbackTemp interface hr %lx\n", hr);
        OutputDebugStringW(Buffer);
        m_MemAllocator->Release();
        m_MemInputPin->Release();
        return hr;
    }

    // set notification routine
    hr = pMemCallback->SetNotify((IMemAllocatorNotifyCallbackTemp*)this);

    // release IMemAllocatorNotifyCallbackTemp interface
    pMemCallback->Release();

    if (FAILED(hr))
    {
        swprintf(Buffer, L"COutputPin::Connect IMemAllocatorNotifyCallbackTemp::SetNotify failed hr %lx\n", hr);
        OutputDebugStringW(Buffer);
        m_MemAllocator->Release();
        m_MemInputPin->Release();
        return hr;
    }

    // now set allocator
    hr = m_MemInputPin->NotifyAllocator(m_MemAllocator, TRUE);
    if (FAILED(hr))
    {
        swprintf(Buffer, L"COutputPin::Connect IMemInputPin::NotifyAllocator failed with hr %lx\n", hr);
        OutputDebugStringW(Buffer);
        m_MemAllocator->Release();
        m_MemInputPin->Release();
        return hr;
    }

    if (!m_hPin)
    {
        //FIXME create pin handle
        assert(0);
    }

    // receive connection;
    hr = pReceivePin->ReceiveConnection((IPin*)this, pmt);
    if (SUCCEEDED(hr))
    {
        // increment reference count
        pReceivePin->AddRef();
        m_Pin = pReceivePin;
        OutputDebugStringW(L"COutputPin::Connect success\n");
    }
    else
    {
        m_MemInputPin->Release();
        m_MemAllocator->Release();
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    return E_UNEXPECTED;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::Disconnect( void)
{
   OutputDebugStringW(L"COutputPin::Disconnect\n");

    if (!m_Pin)
    {
        // pin was not connected
        return S_FALSE;
    }

    //FIXME
    //check if filter is active

    m_Pin->Release();
    m_Pin = NULL;
    m_MemInputPin->Release();
    m_MemAllocator->Release();

    CloseHandle(m_hPin);
    m_hPin = INVALID_HANDLE_VALUE;

    OutputDebugStringW(L"COutputPin::Disconnect\n");
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::ConnectedTo(IPin **pPin)
{
   OutputDebugStringW(L"COutputPin::ConnectedTo\n");

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
    HRESULT hr;
    ULONG MediaTypeCount = 0, Index;
    AM_MEDIA_TYPE * MediaTypes;
    HANDLE hFilter;
    IKsObject * KsObjectParent;

    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    if (FAILED(hr))
        return hr;

    // get parent filter handle
    hFilter = KsObjectParent->KsGetObjectHandle();

    // release IKsObject
    KsObjectParent->Release();

    // query media type count
    hr = KsGetMediaTypeCount(hFilter, m_PinId, &MediaTypeCount);
    if (FAILED(hr) || !MediaTypeCount)
    {
        return hr;
    }

    // allocate media types
    MediaTypes = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * MediaTypeCount);
    if (!MediaTypes)
    {
        // not enough memory
        return E_OUTOFMEMORY;
    }

    // zero media types
    ZeroMemory(MediaTypes, sizeof(AM_MEDIA_TYPE) * MediaTypeCount);

    for(Index = 0; Index < MediaTypeCount; Index++)
    {
        // get media type
        hr = KsGetMediaType(Index, &MediaTypes[Index], hFilter, m_PinId);
        if (FAILED(hr))
        {
            // failed
            CoTaskMemFree(MediaTypes);
            return hr;
        }
    }

    return CEnumMediaTypes_fnConstructor(MediaTypeCount, MediaTypes, IID_IEnumMediaTypes, (void**)ppEnum);
}
HRESULT
STDMETHODCALLTYPE
COutputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::EndOfStream( void)
{
    /* should be called only on input pins */
    return E_UNEXPECTED;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::BeginFlush( void)
{
    /* should be called only on input pins */
    return E_UNEXPECTED;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::EndFlush( void)
{
    /* should be called only on input pins */
    return E_UNEXPECTED;
}
HRESULT
STDMETHODCALLTYPE
COutputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    if (!m_Pin)
    {
        // we are not connected
        return VFW_E_NOT_CONNECTED;
    }

    return m_Pin->NewSegment(tStart, tStop, dRate);
}

//-------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
COutputPin::CheckFormat(
    const AM_MEDIA_TYPE *pmt)
{
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATAFORMAT DataFormat;
    HRESULT hr;
    IKsObject * KsObjectParent;
    HANDLE hFilter;

    if (!pmt)
        return E_POINTER;

    // get IKsObject interface
    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    if (FAILED(hr))
        return hr;

    // get parent filter handle
    hFilter = KsObjectParent->KsGetObjectHandle();

    // release IKsObject
    KsObjectParent->Release();

    if (!hFilter)
        return E_HANDLE;


    hr = KsGetMultiplePinFactoryItems(hFilter, m_PinId, KSPROPERTY_PIN_DATARANGES, (PVOID*)&MultipleItem);
    if (FAILED(hr))
        return S_FALSE;

    DataFormat = (PKSDATAFORMAT)(MultipleItem + 1);
    for(ULONG Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUID(pmt->majortype, DataFormat->MajorFormat) &&
            IsEqualGUID(pmt->subtype, DataFormat->SubFormat) &&
            IsEqualGUID(pmt->formattype, DataFormat->Specifier))
        {
            // format is supported
            CoTaskMemFree(MultipleItem);
            return S_OK;
        }
        DataFormat = (PKSDATAFORMAT)((ULONG_PTR)DataFormat + DataFormat->FormatSize);
    }
    //format is not supported
    CoTaskMemFree(MultipleItem);
    return S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::CreatePin(
    const AM_MEDIA_TYPE *pmt)
{
    PKSMULTIPLE_ITEM MediumList;
    PKSMULTIPLE_ITEM InterfaceList;
    PKSPIN_MEDIUM Medium;
    PKSPIN_INTERFACE Interface;
    IKsInterfaceHandler * InterfaceHandler;
    HRESULT hr;

    // query for pin medium
    hr = KsQueryMediums(&MediumList);
    if (FAILED(hr))
        return hr;

    // query for pin interface
    hr = KsQueryInterfaces(&InterfaceList);
    if (FAILED(hr))
    {
        // failed
        CoTaskMemFree(MediumList);
        return hr;
    }

    if (MediumList->Count)
    {
        //use first available medium
        Medium = (PKSPIN_MEDIUM)(MediumList + 1);
    }
    else
    {
        // default to standard medium
        Medium = &StandardPinMedium;
    }

    if (InterfaceList->Count)
    {
        //use first available interface
        Interface = (PKSPIN_INTERFACE)(InterfaceList + 1);
    }
    else
    {
        // default to standard interface
        Interface = &StandardPinInterface;
    }

    if (m_Communication != KSPIN_COMMUNICATION_BRIDGE && m_Communication != KSPIN_COMMUNICATION_NONE)
    {
        // now create pin
        hr = CreatePinHandle(Medium, Interface, pmt);
        if (FAILED(hr))
        {
            m_InterfaceHandler->Release();
            m_InterfaceHandler = InterfaceHandler;
        }

        if (!m_InterfaceHandler)
        {
            // now load the IKsInterfaceHandler plugin
            hr = CoCreateInstance(Interface->Set, NULL, CLSCTX_INPROC_SERVER, IID_IKsInterfaceHandler, (void**)&InterfaceHandler);
            if (FAILED(hr))
            {
                // failed to load interface handler plugin
                CoTaskMemFree(MediumList);
                CoTaskMemFree(InterfaceList);

                return hr;
            }

            // now set the pin
            hr = InterfaceHandler->KsSetPin((IKsPin*)this);
            if (FAILED(hr))
            {
                // failed to load interface handler plugin
                InterfaceHandler->Release();
                CoTaskMemFree(MediumList);
                CoTaskMemFree(InterfaceList);
                return hr;
            }

            // store interface handler
            m_InterfaceHandler = InterfaceHandler;
        }
    }
    else
    {
        WCHAR Buffer[100];
        swprintf(Buffer, L"COutputPin::CreatePin unexpected communication %u %s\n", m_Communication, m_PinName);
        OutputDebugStringW(Buffer);
        DebugBreak();
        hr = E_FAIL;
    }

    // free medium / interface / dataformat
    CoTaskMemFree(MediumList);
    CoTaskMemFree(InterfaceList);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::CreatePinHandle(
    PKSPIN_MEDIUM Medium,
    PKSPIN_INTERFACE Interface,
    const AM_MEDIA_TYPE *pmt)
{
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT DataFormat;
    ULONG Length;
    HRESULT hr;
    HANDLE hFilter;
    IKsObject * KsObjectParent;

    //KSALLOCATOR_FRAMING Framing;
    //KSPROPERTY Property;
    //ULONG BytesReturned;

    if (m_hPin != INVALID_HANDLE_VALUE)
    {
        // pin already exists
        //CloseHandle(m_hPin);
        //m_hPin = INVALID_HANDLE_VALUE;
        return S_OK;
    }


    // calc format size
    Length = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT) + pmt->cbFormat;

    // allocate pin connect
    PinConnect = (PKSPIN_CONNECT)CoTaskMemAlloc(Length);
    if (!PinConnect)
    {
        // failed
        return E_OUTOFMEMORY;
    }

    // setup request
    CopyMemory(&PinConnect->Interface, Interface, sizeof(KSPIN_INTERFACE));
    CopyMemory(&PinConnect->Medium, Medium, sizeof(KSPIN_MEDIUM));
    PinConnect->PinId = m_PinId;
    PinConnect->PinToHandle = NULL;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = KSPRIORITY_NORMAL;

    // get dataformat offset
    DataFormat = (PKSDATAFORMAT)(PinConnect + 1);

    // copy data format
    DataFormat->FormatSize = sizeof(KSDATAFORMAT) + pmt->cbFormat;
    DataFormat->Flags = 0;
    DataFormat->SampleSize = pmt->lSampleSize;
    DataFormat->Reserved = 0;
    CopyMemory(&DataFormat->MajorFormat, &pmt->majortype, sizeof(GUID));
    CopyMemory(&DataFormat->SubFormat,  &pmt->subtype, sizeof(GUID));
    CopyMemory(&DataFormat->Specifier, &pmt->formattype, sizeof(GUID));

    if (pmt->cbFormat)
    {
        // copy extended format
        CopyMemory((DataFormat + 1), pmt->pbFormat, pmt->cbFormat);
    }

    // get IKsObject interface
    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&KsObjectParent);
    if (FAILED(hr))
        return hr;

    // get parent filter handle
    hFilter = KsObjectParent->KsGetObjectHandle();

    // release IKsObject
    KsObjectParent->Release();

    if (!hFilter)
        return E_HANDLE;

    // create pin
    hr = KsCreatePin(hFilter, PinConnect, GENERIC_READ, &m_hPin);

    if (SUCCEEDED(hr))
    {
        // store current interface / medium
        CopyMemory(&m_Medium, Medium, sizeof(KSPIN_MEDIUM));
        CopyMemory(&m_Interface, Interface, sizeof(KSPIN_INTERFACE));
        CopyMemory(&m_MediaFormat, pmt, sizeof(AM_MEDIA_TYPE));

        LPOLESTR pMajor, pSub, pFormat;
        StringFromIID(m_MediaFormat.majortype, &pMajor);
        StringFromIID(m_MediaFormat.subtype , &pSub);
        StringFromIID(m_MediaFormat.formattype, &pFormat);
        WCHAR Buffer[200];
        swprintf(Buffer, L"COutputPin::CreatePinHandle Major %s SubType %s Format %s pbFormat %p cbFormat %u\n", pMajor, pSub, pFormat, pmt->pbFormat, pmt->cbFormat);
        CoTaskMemFree(pMajor);
        CoTaskMemFree(pSub);
        CoTaskMemFree(pFormat);
        OutputDebugStringW(Buffer);

        if (pmt->cbFormat)
        {
            m_MediaFormat.pbFormat = (BYTE*)CoTaskMemAlloc(pmt->cbFormat);
            if (!m_MediaFormat.pbFormat)
            {
                CoTaskMemFree(PinConnect);
                m_MediaFormat.pbFormat = NULL;
                m_MediaFormat.cbFormat = 0;
                return E_OUTOFMEMORY;
            }
            CopyMemory(m_MediaFormat.pbFormat, pmt->pbFormat, pmt->cbFormat);
        }
#if 0
        Property.Set = KSPROPSETID_Connection;
        Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
        Property.Flags = KSPROPERTY_TYPE_GET;

        ZeroMemory(&Framing, sizeof(KSALLOCATOR_FRAMING));
        hr = KsProperty(&Property, sizeof(KSPROPERTY), (PVOID)&Framing, sizeof(KSALLOCATOR_FRAMING), &BytesReturned);
        if (SUCCEEDED(hr))
        {
            m_Properties.cbAlign = (Framing.FileAlignment + 1);
            m_Properties.cbBuffer = Framing.FrameSize;
            m_Properties.cbPrefix = 0; //FIXME
            m_Properties.cBuffers = Framing.Frames;
        }
        hr = S_OK;
#endif

        if (FAILED(InitializeIOThread()))
        {
            OutputDebugStringW(L"COutputPin::CreatePinHandle failed to initialize i/o thread\n");
            DebugBreak();
        }

        //TODO
        // connect pin pipes

    }
    // free pin connect
     CoTaskMemFree(PinConnect);

    return hr;
}

HRESULT
WINAPI
COutputPin::IoProcessRoutine()
{
    IMediaSample *Sample;
    LONG SampleCount;
    HRESULT hr;
    PKSSTREAM_SEGMENT StreamSegment;
    HANDLE hEvent;
    WCHAR Buffer[200];
    IMediaSample * Samples[1];

    // first wait for the start event to signal
    WaitForSingleObject(m_hStartEvent, INFINITE);

    assert(m_InterfaceHandler);
    do
    {
        if (m_StopInProgress)
        {
            // stop io thread
            break;
        }

        assert(m_State == KSSTATE_RUN);
        assert(m_MemAllocator);

        // get buffer
        hr = m_MemAllocator->GetBuffer(&Sample, NULL, NULL, AM_GBF_NOWAIT);

        if (FAILED(hr))
        {
            WaitForSingleObject(m_hBufferAvailable, INFINITE);
            // now retry again
            continue;
        }

        // fill buffer
        SampleCount = 1;
        Samples[0] = Sample;

        Sample->SetTime(NULL, NULL);
        hr = m_InterfaceHandler->KsProcessMediaSamples(NULL, /* FIXME */
                                                       Samples,
                                                       &SampleCount,
                                                       KsIoOperation_Read,
                                                       &StreamSegment);
        if (FAILED(hr) || !StreamSegment)
        {
            swprintf(Buffer, L"COutputPin::IoProcessRoutine KsProcessMediaSamples FAILED PinName %s hr %lx\n", m_PinName, hr);
            OutputDebugStringW(Buffer);
            break;
        }

        // get completion event
        hEvent = StreamSegment->CompletionEvent;

        // wait for i/o completion
        WaitForSingleObject(hEvent, INFINITE);

        // perform completion
        m_InterfaceHandler->KsCompleteIo(StreamSegment);

        // close completion event
        CloseHandle(hEvent);

        if (SUCCEEDED(hr))
        {
            assert(m_MemInputPin);

            // now deliver the sample
            hr = m_MemInputPin->Receive(Sample);

            swprintf(Buffer, L"COutputPin::IoProcessRoutine PinName %s IMemInputPin::Receive hr %lx Sample %p m_MemAllocator %p\n", m_PinName, hr, Sample, m_MemAllocator);
            OutputDebugStringW(Buffer);
             if (FAILED(hr))
				 DebugBreak();
            Sample = NULL;
        }
    }while(TRUE);

    // signal end of i/o thread
    SetEvent(m_hStopEvent);

    m_IoThreadStarted = false;

    return NOERROR;
}

DWORD
WINAPI
COutputPin_IoThreadStartup(
    LPVOID lpParameter)
{
    COutputPin * Pin = (COutputPin*)lpParameter;
    assert(Pin);

    return Pin->IoProcessRoutine();
}


HRESULT
WINAPI
COutputPin::InitializeIOThread()
{
    HANDLE hThread;

    if (m_IoThreadStarted)
        return NOERROR;

    if (!m_hStartEvent)
        m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!m_hStartEvent)
        return E_OUTOFMEMORY;

    if (!m_hStopEvent)
        m_hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!m_hStopEvent)
        return E_OUTOFMEMORY;

    if (!m_hBufferAvailable)
        m_hBufferAvailable = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!m_hBufferAvailable)
        return E_OUTOFMEMORY;

    m_StopInProgress = false;
    m_IoThreadStarted = true;

    // now create the startup thread
    hThread = CreateThread(NULL, 0, COutputPin_IoThreadStartup, (LPVOID)this, 0, NULL);
    if (!hThread)
        return E_OUTOFMEMORY;


    // close thread handle
    CloseHandle(hThread);
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
COutputPin_SetState(
    IPin * Pin,
    KSSTATE State)
{
    HRESULT hr = S_OK;
    KSPROPERTY Property;
    KSSTATE CurState;
    WCHAR Buffer[200];
    ULONG BytesReturned;

    COutputPin * pPin = (COutputPin*)Pin;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    EnterCriticalSection(&pPin->m_Lock);

    if (pPin->m_State <= State)
    {
        if (pPin->m_State == KSSTATE_STOP)
        {
            hr = pPin->InitializeIOThread();
            if (FAILED(hr))
            {
                // failed to initialize I/O thread
                OutputDebugStringW(L"Failed to initialize I/O Thread\n");
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }
            CurState = KSSTATE_ACQUIRE;
            hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

            swprintf(Buffer, L"COutputPin_SetState Setting State CurState: KSSTATE_STOP KSSTATE_ACQUIRE PinName %s hr %lx\n", pPin->m_PinName, hr);
            OutputDebugStringW(Buffer);
            if (FAILED(hr))
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }

            pPin->m_State = CurState;

            if (pPin->m_State == State)
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }
        }
        if (pPin->m_State == KSSTATE_ACQUIRE)
        {
            CurState = KSSTATE_PAUSE;
            hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

            swprintf(Buffer, L"COutputPin_SetState Setting State CurState KSSTATE_ACQUIRE KSSTATE_PAUSE PinName %s hr %lx\n", pPin->m_PinName, hr);
            OutputDebugStringW(Buffer);
            if (FAILED(hr))
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }

            pPin->m_State = CurState;

            if (pPin->m_State == State)
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }
        }
        if (State == KSSTATE_RUN && pPin->m_State == KSSTATE_PAUSE)
        {
            CurState = KSSTATE_RUN;
            hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

            swprintf(Buffer, L"COutputPin_SetState Setting State CurState: KSSTATE_PAUSE KSSTATE_RUN PinName %s hr %lx\n", pPin->m_PinName, hr);
            OutputDebugStringW(Buffer);

            if (SUCCEEDED(hr))
            {
                pPin->m_State = CurState;
                // signal start event
                SetEvent(pPin->m_hStartEvent);
            }
        }

        LeaveCriticalSection(&pPin->m_Lock);
        return hr;
    }
    else
    {
        if (pPin->m_State == KSSTATE_RUN)
        {
            // setting pending stop flag
            pPin->m_StopInProgress = true;

            // release any waiting threads
            SetEvent(pPin->m_hBufferAvailable);

            // wait until i/o thread is done
            WaitForSingleObject(pPin->m_hStopEvent, INFINITE);

            CurState = KSSTATE_PAUSE;
            hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

            swprintf(Buffer, L"COutputPin_SetState Setting State CurState: KSSTATE_RUN KSSTATE_PAUSE PinName %s hr %lx\n", pPin->m_PinName, hr);
            OutputDebugStringW(Buffer);

            if (FAILED(hr))
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }

            pPin->m_State = CurState;

            if (FAILED(hr))
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }
        }
        if (pPin->m_State == KSSTATE_PAUSE)
        {
            CurState = KSSTATE_ACQUIRE;
            hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

            swprintf(Buffer, L"COutputPin_SetState Setting State CurState: KSSTATE_PAUSE KSSTATE_ACQUIRE PinName %s hr %lx\n", pPin->m_PinName, hr);
            OutputDebugStringW(Buffer);

            if (FAILED(hr))
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }

            pPin->m_State = CurState;

            if (pPin->m_State == State)
            {
                LeaveCriticalSection(&pPin->m_Lock);
                return hr;
            }
        }

        CloseHandle(pPin->m_hStopEvent);
        CloseHandle(pPin->m_hStartEvent);
        CloseHandle(pPin->m_hBufferAvailable);

        /* close event handles */
        pPin->m_hStopEvent = NULL;
        pPin->m_hStartEvent = NULL;
        pPin->m_hBufferAvailable = NULL;

        CurState = KSSTATE_STOP;
        hr = pPin->KsProperty(&Property, sizeof(KSPROPERTY), &CurState, sizeof(KSSTATE), &BytesReturned);

        swprintf(Buffer, L"COutputPin_SetState Setting State CurState: KSSTATE_ACQUIRE KSSTATE_STOP PinName %s hr %lx\n", pPin->m_PinName, hr);
        OutputDebugStringW(Buffer);

        if (SUCCEEDED(hr))
        {
            // store state
            pPin->m_State = CurState;
        }

        // unset pending stop flag
        pPin->m_StopInProgress = false;

        LeaveCriticalSection(&pPin->m_Lock);
        return hr;
    }
}

HRESULT
WINAPI
COutputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    ULONG PinId,
    KSPIN_COMMUNICATION Communication,
    REFIID riid,
    LPVOID * ppv)
{
    COutputPin * handler = new COutputPin(ParentFilter, PinName, PinId, Communication);

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
