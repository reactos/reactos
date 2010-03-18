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

    COutputPin(IBaseFilter * ParentFilter, LPCWSTR PinName, ULONG PinId);
    virtual ~COutputPin();

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    LPCWSTR m_PinName;
    HANDLE m_hPin;
    ULONG m_PinId;
    IKsObject * m_KsObjectParent;
    IPin * m_Pin;
    IKsAllocatorEx * m_KsAllocatorEx;
    ULONG m_PipeAllocatorFlag;
    BOOL m_bPinBusCacheInitialized;
    GUID m_PinBusCache;
    LPWSTR m_FilterName;
    FRAMING_PROP m_FramingProp[4];
    PKSALLOCATOR_FRAMING_EX m_FramingEx[4];

    IMemAllocator * m_MemAllocator;
    LONG m_IoCount;
    KSPIN_COMMUNICATION m_Communication;
    KSPIN_INTERFACE m_Interface;
    KSPIN_MEDIUM m_Medium;
    IMediaSeeking * m_FilterMediaSeeking;
};

COutputPin::~COutputPin()
{
    if (m_KsObjectParent)
        m_KsObjectParent->Release();
}

COutputPin::COutputPin(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    ULONG PinId) : m_Ref(0),
                   m_ParentFilter(ParentFilter),
                   m_PinName(PinName),
                   m_hPin(INVALID_HANDLE_VALUE),
                   m_PinId(PinId),
                   m_KsObjectParent(0),
                   m_Pin(0),
                   m_KsAllocatorEx(0),
                   m_PipeAllocatorFlag(0),
                   m_bPinBusCacheInitialized(0),
                   m_FilterName(0),
                   m_MemAllocator(0),
                   m_IoCount(0),
                   m_Communication(KSPIN_COMMUNICATION_NONE),
                   m_FilterMediaSeeking(0)
{
    HRESULT hr;

    hr = m_ParentFilter->QueryInterface(IID_IKsObject, (LPVOID*)&m_KsObjectParent);
    assert(hr == S_OK);

    hr = m_ParentFilter->QueryInterface(IID_IMediaSeeking, (LPVOID*)&m_FilterMediaSeeking);
    assert(hr == S_OK);

    ZeroMemory(m_FramingProp, sizeof(m_FramingProp));
    ZeroMemory(m_FramingEx, sizeof(m_FramingEx));
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
        OutputDebugStringW(L"COutputPin::QueryInterface IID_IKsPropertySet\n");
        DebugBreak();
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
    swprintf(Buffer, L"COutputPin::QueryInterface: NoInterface for %s\n", lpstr);
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
    OutputDebugStringW(L"COutputPin::SuggestAllocatorProperties NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetAllocatorProperties(
    ALLOCATOR_PROPERTIES *pprop)
{
    OutputDebugStringW(L"COutputPin::GetAllocatorProperties NotImplemented\n");
    return E_NOTIMPL;
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
    OutputDebugStringW(L"COutputPin::NotifyRelease NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IMediaSeeking interface
//
HRESULT
STDMETHODCALLTYPE
COutputPin::GetCapabilities(
    DWORD *pCapabilities)
{
    return m_FilterMediaSeeking->GetCapabilities(pCapabilities);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::CheckCapabilities(
    DWORD *pCapabilities)
{
    return m_FilterMediaSeeking->CheckCapabilities(pCapabilities);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::IsFormatSupported(
    const GUID *pFormat)
{
    return m_FilterMediaSeeking->IsFormatSupported(pFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::QueryPreferredFormat(
    GUID *pFormat)
{
    return m_FilterMediaSeeking->QueryPreferredFormat(pFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetTimeFormat(
    GUID *pFormat)
{
    return m_FilterMediaSeeking->GetTimeFormat(pFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::IsUsingTimeFormat(
    const GUID *pFormat)
{
    return m_FilterMediaSeeking->IsUsingTimeFormat(pFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetTimeFormat(
    const GUID *pFormat)
{
    return m_FilterMediaSeeking->SetTimeFormat(pFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetDuration(
    LONGLONG *pDuration)
{
    return m_FilterMediaSeeking->GetDuration(pDuration);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetStopPosition(
    LONGLONG *pStop)
{
    return m_FilterMediaSeeking->GetStopPosition(pStop);
}


HRESULT
STDMETHODCALLTYPE
COutputPin::GetCurrentPosition(
    LONGLONG *pCurrent)
{
    return m_FilterMediaSeeking->GetCurrentPosition(pCurrent);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::ConvertTimeFormat(
    LONGLONG *pTarget,
    const GUID *pTargetFormat,
    LONGLONG Source,
    const GUID *pSourceFormat)
{
    return m_FilterMediaSeeking->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetPositions(
    LONGLONG *pCurrent,
    DWORD dwCurrentFlags,
    LONGLONG *pStop,
    DWORD dwStopFlags)
{
    return m_FilterMediaSeeking->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetPositions(
    LONGLONG *pCurrent,
    LONGLONG *pStop)
{
    return m_FilterMediaSeeking->GetPositions(pCurrent, pStop);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetAvailable(
    LONGLONG *pEarliest,
    LONGLONG *pLatest)
{
    return m_FilterMediaSeeking->GetAvailable(pEarliest, pLatest);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::SetRate(
    double dRate)
{
    return m_FilterMediaSeeking->SetRate(dRate);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetRate(
    double *pdRate)
{
    return m_FilterMediaSeeking->GetRate(pdRate);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::GetPreroll(
    LONGLONG *pllPreroll)
{
    return m_FilterMediaSeeking->GetPreroll(pllPreroll);
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
    HANDLE hFilter = m_KsObjectParent->KsGetObjectHandle();
    return KsGetMultiplePinFactoryItems(hFilter, m_PinId, KSPROPERTY_PIN_MEDIUMS, (PVOID*)MediumList);
}

HRESULT
STDMETHODCALLTYPE
COutputPin::KsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList)
{
    HANDLE hFilter = m_KsObjectParent->KsGetObjectHandle();

    return KsGetMultiplePinFactoryItems(hFilter, m_PinId, KSPROPERTY_PIN_INTERFACES, (PVOID*)InterfaceList);
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
    OutputDebugStringW(L"COutputPin::KsPropagateAcquire NotImplemented\n");
    return E_NOTIMPL;
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
    assert(m_hPin);
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
    assert(m_hPin != 0);
    OutputDebugStringW(L"COutputPin::KsProperty\n");
    return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_PROPERTY, (PVOID)Property, PropertyLength, (PVOID)PropertyData, DataLength, BytesReturned);
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
    assert(m_hPin != 0);
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
    assert(m_hPin != 0);

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

    OutputDebugStringW(L"COutputPin::Set\n");

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

    OutputDebugStringW(L"COutputPin::Get\n");

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
    AM_MEDIA_TYPE MediaType;
    HRESULT hr;
    HANDLE hFilter;

    OutputDebugStringW(L"COutputPin::Connect called\n");
    if (pmt)
    {
        hr = pReceivePin->QueryAccept(pmt);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        // get parent filter handle
        hFilter = m_KsObjectParent->KsGetObjectHandle();

        // get media type count
        ZeroMemory(&MediaType, sizeof(AM_MEDIA_TYPE));
        hr = KsGetMediaType(0, &MediaType, hFilter, m_PinId);
        if (FAILED(hr))
            return hr;

        // query accept
        hr = pReceivePin->QueryAccept(&MediaType);
        if (FAILED(hr))
            return hr;

         pmt = &MediaType;
    }

    //FIXME create pin handle

    // receive connection;
    hr = pReceivePin->ReceiveConnection((IPin*)this, pmt);
    if (SUCCEEDED(hr))
    {
        // increment reference count
        pReceivePin->AddRef();
        m_Pin = pReceivePin;
        OutputDebugStringW(L"COutputPin::Connect success\n");
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
COutputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"COutputPin::ReceiveConnection\n");
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
    OutputDebugStringW(L"COutputPin::QueryPinInfo\n");

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
    OutputDebugStringW(L"COutputPin::QueryDirection\n");

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
    OutputDebugStringW(L"COutputPin::QueryId\n");

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

    OutputDebugStringW(L"COutputPin::EnumMediaTypes called\n");

    if (!m_KsObjectParent)
    {
        // no interface
        return E_NOINTERFACE;
    }

    // get parent filter handle
    hFilter = m_KsObjectParent->KsGetObjectHandle();

    // query media type count
    hr = KsGetMediaTypeCount(hFilter, m_PinId, &MediaTypeCount);
    if (FAILED(hr) || !MediaTypeCount)
    {
        OutputDebugStringW(L"COutputPin::EnumMediaTypes failed1\n");
        return hr;
    }

    // allocate media types
    MediaTypes = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * MediaTypeCount);
    if (!MediaTypes)
    {
        // not enough memory
        OutputDebugStringW(L"COutputPin::EnumMediaTypes CoTaskMemAlloc\n");
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
            OutputDebugStringW(L"COutputPin::EnumMediaTypes failed\n");
            return hr;
        }
    }

    return CEnumMediaTypes_fnConstructor(MediaTypeCount, MediaTypes, IID_IEnumMediaTypes, (void**)ppEnum);
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
    ULONG PinId,
    REFIID riid,
    LPVOID * ppv)
{
    COutputPin * handler = new COutputPin(ParentFilter, PinName, PinId);

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
