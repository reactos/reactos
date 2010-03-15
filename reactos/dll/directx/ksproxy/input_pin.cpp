/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/input_cpp.cpp
 * PURPOSE:         InputPin of Proxy Filter
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IKsPinEx = {0x7bb38260L, 0xd19c, 0x11d2, {0xb3, 0x8a, 0x00, 0xa0, 0xc9, 0x5e, 0xc2, 0x2e}};

#ifndef _MSC_VER
const GUID KSPROPSETID_Connection = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
#endif

#ifndef _MSC_VER
KSPIN_INTERFACE StandardPinInterface = 
{
    {STATIC_KSINTERFACESETID_Standard},
    KSINTERFACE_STANDARD_STREAMING,
    0
};

KSPIN_MEDIUM StandardPinMedium =
{
    {STATIC_KSMEDIUMSETID_Standard},
    KSMEDIUM_TYPE_ANYINSTANCE,
    0
};

#else

KSPIN_INTERFACE StandardPinInterface = 
{
    STATIC_KSINTERFACESETID_Standard,
    KSINTERFACE_STANDARD_STREAMING,
    0
};

KSPIN_MEDIUM StandardPinMedium =
{
    STATIC_KSMEDIUMSETID_Standard,
    KSMEDIUM_TYPE_ANYINSTANCE,
    0
};

#endif

class CInputPin : public IPin,
                  public IKsPropertySet,
                  public IKsControl,
                  public IKsObject,
                  public IKsPinEx,
                  public IMemInputPin,
                  public ISpecifyPropertyPages
/*
                  public IQualityControl,
                  public IKsPinPipe,
                  public IStreamBuilder,
                  public IKsPinFactory,
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

    //IMemInputPin
    HRESULT STDMETHODCALLTYPE GetAllocator(IMemAllocator **ppAllocator);
    HRESULT STDMETHODCALLTYPE NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
    HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
    HRESULT STDMETHODCALLTYPE Receive(IMediaSample *pSample);
    HRESULT STDMETHODCALLTYPE ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
    HRESULT STDMETHODCALLTYPE ReceiveCanBlock( void);

    //---------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE CheckFormat(const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE CreatePin(const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE CreatePinHandle(PKSPIN_MEDIUM Medium, PKSPIN_INTERFACE Interface, const AM_MEDIA_TYPE *pmt);
    CInputPin(IBaseFilter * ParentFilter, LPCWSTR PinName, HANDLE hFilter, ULONG PinId, KSPIN_COMMUNICATION Communication) : m_Ref(0), m_ParentFilter(ParentFilter), m_PinName(PinName), m_hFilter(hFilter), m_hPin(INVALID_HANDLE_VALUE), m_PinId(PinId), m_MemAllocator(0), m_IoCount(0), m_Communication(Communication), m_Pin(0), m_ReadOnly(0){};
    virtual ~CInputPin(){};

protected:
    LONG m_Ref;
    IBaseFilter * m_ParentFilter;
    LPCWSTR m_PinName;
    HANDLE m_hFilter;
    HANDLE m_hPin;
    ULONG m_PinId;
    IMemAllocator * m_MemAllocator;
    LONG m_IoCount;
    KSPIN_COMMUNICATION m_Communication;
    KSPIN_INTERFACE m_Interface;
    KSPIN_MEDIUM m_Medium;
    IPin * m_Pin;
    BOOL m_ReadOnly;
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
    else if (IsEqualGUID(refiid, IID_IMemInputPin))
    {
        *Output = (IMemInputPin*)(this);
        reinterpret_cast<IMemInputPin*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsObject))
    {
        *Output = (IKsObject*)(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPropertySet))
    {
        *Output = (IKsPropertySet*)(this);
        reinterpret_cast<IKsPropertySet*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsControl))
    {
        *Output = (IKsControl*)(this);
        reinterpret_cast<IKsControl*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPin) ||
             IsEqualGUID(refiid, IID_IKsPinEx))
    {
        *Output = (IKsPinEx*)(this);
        reinterpret_cast<IKsPinEx*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_ISpecifyPropertyPages))
    {
        *Output = (ISpecifyPropertyPages*)(this);
        reinterpret_cast<ISpecifyPropertyPages*>(*Output)->AddRef();
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
// ISpecifyPropertyPages
//

HRESULT
STDMETHODCALLTYPE
CInputPin::GetPages(CAUUID *pPages)
{
    if (!pPages)
        return E_POINTER;

    pPages->cElems = 0;
    pPages->pElems = NULL;

    return S_OK;
}

//-------------------------------------------------------------------
// IMemInputPin
//


HRESULT
STDMETHODCALLTYPE
CInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    OutputDebugStringW(L"CInputPin::GetAllocator\n");
    return VFW_E_NO_ALLOCATOR;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
    if (pAllocator)
    {
        pAllocator->AddRef();
    }

    if (m_MemAllocator)
    {
        m_MemAllocator->Release();
    }

    m_MemAllocator = pAllocator;
    m_ReadOnly = bReadOnly;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
    KSALLOCATOR_FRAMING Framing;
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_SET;

    hr = KsProperty(&Property, sizeof(KSPROPERTY), (PVOID)&Framing, sizeof(KSALLOCATOR_FRAMING), &BytesReturned);
    if (SUCCEEDED(hr))
    {
        pProps->cBuffers = Framing.Frames;
        pProps->cbBuffer = Framing.FrameSize;
        pProps->cbAlign = Framing.FileAlignment;
        pProps->cbPrefix = 0;
        return hr;
    }
    else
        return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::Receive(IMediaSample *pSample)
{
    OutputDebugStringW(L"CInputPin::Receive NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
    OutputDebugStringW(L"CInputPin::ReceiveMultiple NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::ReceiveCanBlock( void)
{
    OutputDebugStringW(L"CInputPin::ReceiveCanBlock NotImplemented\n");
    return S_FALSE;
}


//-------------------------------------------------------------------
// IKsPin
//

HRESULT
STDMETHODCALLTYPE
CInputPin::KsQueryMediums(
    PKSMULTIPLE_ITEM* MediumList)
{
    return KsGetMultiplePinFactoryItems(m_hFilter, m_PinId, KSPROPERTY_PIN_MEDIUMS, (PVOID*)MediumList);
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList)
{
    return KsGetMultiplePinFactoryItems(m_hFilter, m_PinId, KSPROPERTY_PIN_INTERFACES, (PVOID*)InterfaceList);
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsCreateSinkPinHandle(
    KSPIN_INTERFACE& Interface,
    KSPIN_MEDIUM& Medium)
{
    OutputDebugStringW(L"CInputPin::KsCreateSinkPinHandle NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsGetCurrentCommunication(
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
CInputPin::KsPropagateAcquire()
{
    OutputDebugStringW(L"CInputPin::KsPropagateAcquire NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsDeliver(
    IMediaSample* Sample,
    ULONG Flags)
{
    return E_FAIL;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment)
{
    return NOERROR;
}

IMemAllocator * 
STDMETHODCALLTYPE
CInputPin::KsPeekAllocator(KSPEEKOPERATION Operation)
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
CInputPin::KsReceiveAllocator(IMemAllocator *MemAllocator)
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
CInputPin::KsRenegotiateAllocator()
{
    return E_FAIL;
}

LONG
STDMETHODCALLTYPE
CInputPin::KsIncrementPendingIoCount()
{
    return InterlockedIncrement((volatile LONG*)&m_IoCount);
}

LONG
STDMETHODCALLTYPE
CInputPin::KsDecrementPendingIoCount()
{
    return InterlockedDecrement((volatile LONG*)&m_IoCount);
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsQualityNotify(
    ULONG Proportion,
    REFERENCE_TIME TimeDelta)
{
    OutputDebugStringW(L"CInputPin::KsQualityNotify NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IKsPinEx
//

VOID
STDMETHODCALLTYPE
CInputPin::KsNotifyError(
    IMediaSample* Sample,
    HRESULT hr)
{
    OutputDebugStringW(L"CInputPin::KsNotifyError NotImplemented\n");
}


//-------------------------------------------------------------------
// IKsControl
//
HRESULT
STDMETHODCALLTYPE
CInputPin::KsProperty(
    PKSPROPERTY Property,
    ULONG PropertyLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_hPin != 0);
    return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_PROPERTY, (PVOID)Property, PropertyLength, (PVOID)PropertyData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsMethod(
    PKSMETHOD Method,
    ULONG MethodLength,
    LPVOID MethodData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_hPin != 0);
    return KsSynchronousDeviceControl(m_hPin, IOCTL_KS_METHOD, (PVOID)Method, MethodLength, (PVOID)MethodData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CInputPin::KsEvent(
    PKSEVENT Event,
    ULONG EventLength,
    LPVOID EventData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_hPin != 0);

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
CInputPin::Set(
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
CInputPin::Get(
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
CInputPin::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    Property.Set = guidPropSet;
    Property.Id = dwPropID;
    Property.Flags = KSPROPERTY_TYPE_SETSUPPORT;

    return KsProperty(&Property, sizeof(KSPROPERTY), pTypeSupport, sizeof(DWORD), &BytesReturned);
}


//-------------------------------------------------------------------
// IKsObject
//
HANDLE
STDMETHODCALLTYPE
CInputPin::KsGetObjectHandle()
{
    assert(m_hPin);
    return m_hPin;
}

//-------------------------------------------------------------------
// IPin interface
//
HRESULT
STDMETHODCALLTYPE
CInputPin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    OutputDebugStringW(L"CInputPin::Connect NotImplemented\n");
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    if (m_Pin)
    {
        return VFW_E_ALREADY_CONNECTED;
    }

    // first check format
    hr = CheckFormat(pmt);
    if (FAILED(hr))
        return hr;

    if (FAILED(CheckFormat(pmt)))
        return hr;

    hr = CreatePin(pmt);
    if (FAILED(hr))
    {
        return hr;
    }

    //FIXME create pin
   m_Pin = pConnector;
   m_Pin->AddRef();

    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::Disconnect( void)
{
    if (!m_Pin)
    {
        // pin was not connected
        return S_FALSE;
    }

    //FIXME
    //check if filter is active

    m_Pin->Release();
    m_Pin = NULL;

    OutputDebugStringW(L"CInputPin::Disconnect\n");
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::ConnectedTo(IPin **pPin)
{
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
CInputPin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    if (!m_Pin)
        return VFW_E_NOT_CONNECTED;

    OutputDebugStringW(L"CInputPin::ConnectionMediaType NotImplemented\n");
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
CInputPin::QueryAccept(
    const AM_MEDIA_TYPE *pmt)
{
    return CheckFormat(pmt);
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    HRESULT hr;
    ULONG MediaTypeCount = 0, Index;
    AM_MEDIA_TYPE * MediaTypes;

    // query media type count
    hr = KsGetMediaTypeCount(m_hFilter, m_PinId, &MediaTypeCount);
    if (FAILED(hr) || !MediaTypeCount)
        return hr;

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
        hr = KsGetMediaType(Index, &MediaTypes[Index], m_hFilter, m_PinId);
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
CInputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    OutputDebugStringW(L"CInputPin::QueryInternalConnections NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EndOfStream( void)
{
    OutputDebugStringW(L"CInputPin::EndOfStream NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::BeginFlush( void)
{
    OutputDebugStringW(L"CInputPin::BeginFlush NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::EndFlush( void)
{
    OutputDebugStringW(L"CInputPin::EndFlush NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    OutputDebugStringW(L"CInputPin::NewSegment NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
CInputPin::CheckFormat(
    const AM_MEDIA_TYPE *pmt)
{
    KSP_PIN Property;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATAFORMAT DataFormat;
    ULONG BytesReturned;
    HRESULT hr;

    // prepare request
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_DATARANGES;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = m_PinId;
    Property.Reserved = 0;

    if (!pmt)
        return E_POINTER;

    // query for size of dataranges
    hr = KsSynchronousDeviceControl(m_hFilter, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_MORE_DATA))
    {
        // allocate dataranges buffer
        MultipleItem = (PKSMULTIPLE_ITEM)CoTaskMemAlloc(BytesReturned);

        if (!MultipleItem)
            return E_OUTOFMEMORY;

        // query dataranges
        hr = KsSynchronousDeviceControl(m_hFilter, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

        if (FAILED(hr))
        {
            // failed to query data ranges
            CoTaskMemFree(MultipleItem);
            return hr;
        }

        DataFormat = (PKSDATAFORMAT)(MultipleItem + 1);
        for(ULONG Index = 0; Index < MultipleItem->Count; Index++)
        {
            if (IsEqualGUID(pmt->majortype, DataFormat->MajorFormat) &&
                IsEqualGUID(pmt->subtype, DataFormat->SubFormat) &&
                IsEqualGUID(pmt->formattype, DataFormat->Specifier))
            {
                // format is supported
                CoTaskMemFree(MultipleItem);
                OutputDebugStringW(L"CInputPin::CheckFormat format OK\n");
                return S_OK;
            }
            DataFormat = (PKSDATAFORMAT)((ULONG_PTR)DataFormat + DataFormat->FormatSize);
        }
        //format is not supported
        CoTaskMemFree(MultipleItem);
    }
    return S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::CreatePin(
    const AM_MEDIA_TYPE *pmt)
{
    PKSMULTIPLE_ITEM MediumList;
    PKSMULTIPLE_ITEM InterfaceList;
    PKSPIN_MEDIUM Medium;
    PKSPIN_INTERFACE Interface;
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

    // now create pin
    hr = CreatePinHandle(Medium, Interface, pmt);

    // free medium / interface / dataformat
    CoTaskMemFree(MediumList);
    CoTaskMemFree(InterfaceList);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CInputPin::CreatePinHandle(
    PKSPIN_MEDIUM Medium,
    PKSPIN_INTERFACE Interface,
    const AM_MEDIA_TYPE *pmt)
{
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT DataFormat;
    ULONG Length;
    HRESULT hr;

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

    // create pin
    hr = KsCreatePin(m_hFilter, PinConnect, GENERIC_WRITE, &m_hPin);

    // free pin connect
     CoTaskMemFree(PinConnect);

    return hr;
}

HRESULT
WINAPI
CInputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    HANDLE hFilter,
    ULONG PinId,
    KSPIN_COMMUNICATION Communication,
    REFIID riid,
    LPVOID * ppv)
{
    CInputPin * handler = new CInputPin(ParentFilter, PinName, hFilter, PinId, Communication);

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
