/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/proxy.cpp
 * PURPOSE:         IKsProxy interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IPersistPropertyBag = {0x37D84F60, 0x42CB, 0x11CE, {0x81, 0x35, 0x00, 0xAA, 0x00, 0x4B, 0xB8, 0x51}};
const GUID IID_ISpecifyPropertyPages = {0xB196B28B, 0xBAB4, 0x101A, {0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07}};
const GUID IID_IPersistStream = {0x00000109, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID IID_IPersist = {0x0000010c, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID IID_IBDA_DeviceControl = {0xFD0A5AF3, 0xB41D, 0x11d2, {0x9C, 0x95, 0x00, 0xC0, 0x4F, 0x79, 0x71, 0xE0}};
const GUID IID_IKsAggregateControl = {0x7F40EAC0, 0x3947, 0x11D2, {0x87, 0x4E, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsClockPropertySet = {0x5C5CBD84, 0xE755, 0x11D0, {0xAC, 0x18, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsTopology             = {0x28F54683, 0x06FD, 0x11D2, {0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsClock            = {0x877E4351, 0x6FEA, 0x11D0, {0xB8, 0x63, 0x00, 0xAA, 0x00, 0xA2, 0x16, 0xA1}};
/*
    Needs IKsClock, IKsNotifyEvent
*/

class CKsProxy : public IBaseFilter,
                 public IAMovieSetup,
                 public IPersistPropertyBag,
                 public IKsObject,
                 public IPersistStream,
                 public IAMDeviceRemoval,
                 public ISpecifyPropertyPages,
                 public IReferenceClock,
                 public IMediaSeeking,
                 public IKsPropertySet,
                 public IKsClock,
                 public IKsClockPropertySet,
                 public IAMFilterMiscFlags,
                 public IKsControl,
                 public IKsTopology,
                 public IKsAggregateControl

{
public:
    typedef std::vector<IUnknown *>ProxyPluginVector;
    typedef std::vector<IPin *> PinVector;

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

    // IBaseFilter methods
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
    HRESULT STDMETHODCALLTYPE Stop( void);
    HRESULT STDMETHODCALLTYPE Pause( void);
    HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
    HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
    HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
    HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock);
    HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins **ppEnum);
    HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, IPin **ppPin);
    HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO *pInfo);
    HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
    HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR *pVendorInfo);

    //IReferenceClock
    HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTime);
    HRESULT STDMETHODCALLTYPE AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent, DWORD_PTR *pdwAdviseCookie);
    HRESULT STDMETHODCALLTYPE AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HSEMAPHORE hSemaphore, DWORD_PTR *pdwAdviseCookie);
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD_PTR dwAdviseCookie);

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

    //IKsPropertySet
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

    //IAMFilterMiscFlags
    ULONG STDMETHODCALLTYPE GetMiscFlags( void);

    //IKsControl
    HRESULT STDMETHODCALLTYPE KsProperty(PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsMethod(PKSMETHOD Method, ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsEvent(PKSEVENT Event, ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned);

    //IKsTopolology
    HRESULT STDMETHODCALLTYPE CreateNodeInstance(ULONG NodeId, ULONG Flags, ACCESS_MASK DesiredAccess, IUnknown* UnkOuter, REFGUID InterfaceId, LPVOID* Interface);

    //IKsAggregateControl
    HRESULT STDMETHODCALLTYPE KsAddAggregate(IN REFGUID AggregateClass);
    HRESULT STDMETHODCALLTYPE KsRemoveAggregate(REFGUID AggregateClass);

    //IKsClockPropertySet
    HRESULT STDMETHODCALLTYPE KsGetTime(LONGLONG* Time);
    HRESULT STDMETHODCALLTYPE KsSetTime(LONGLONG Time);
    HRESULT STDMETHODCALLTYPE KsGetPhysicalTime(LONGLONG* Time);
    HRESULT STDMETHODCALLTYPE KsSetPhysicalTime(LONGLONG Time);
    HRESULT STDMETHODCALLTYPE KsGetCorrelatedTime(KSCORRELATED_TIME* CorrelatedTime);
    HRESULT STDMETHODCALLTYPE KsSetCorrelatedTime(KSCORRELATED_TIME* CorrelatedTime);
    HRESULT STDMETHODCALLTYPE KsGetCorrelatedPhysicalTime(KSCORRELATED_TIME* CorrelatedTime);
    HRESULT STDMETHODCALLTYPE KsSetCorrelatedPhysicalTime(KSCORRELATED_TIME* CorrelatedTime);
    HRESULT STDMETHODCALLTYPE KsGetResolution(KSRESOLUTION* Resolution);
    HRESULT STDMETHODCALLTYPE KsGetState(KSSTATE* State);


    //IAMovieSetup methods
    HRESULT STDMETHODCALLTYPE Register( void);
    HRESULT STDMETHODCALLTYPE Unregister( void);

    // IPersistPropertyBag methods
    HRESULT STDMETHODCALLTYPE InitNew( void);
    HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog);
    HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // IKsObject
    HANDLE STDMETHODCALLTYPE KsGetObjectHandle();

    // IKsClock
    HANDLE STDMETHODCALLTYPE KsGetClockHandle();

    //IAMDeviceRemoval
    HRESULT STDMETHODCALLTYPE DeviceInfo(CLSID *pclsidInterfaceClass, LPWSTR *pwszSymbolicLink);
    HRESULT STDMETHODCALLTYPE Reassociate(void);
    HRESULT STDMETHODCALLTYPE Disassociate( void);

    //IPersistStream
    HRESULT STDMETHODCALLTYPE IsDirty( void);
    HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // ISpecifyPropertyPages
    HRESULT STDMETHODCALLTYPE GetPages(CAUUID *pPages);


    CKsProxy();
    ~CKsProxy()
    {
        if (m_hDevice)
            CloseHandle(m_hDevice);
    };

    HRESULT STDMETHODCALLTYPE GetSupportedSets(LPGUID * pOutGuid, PULONG NumGuids);
    HRESULT STDMETHODCALLTYPE LoadProxyPlugins(LPGUID pGuids, ULONG NumGuids);
    HRESULT STDMETHODCALLTYPE GetNumberOfPins(PULONG NumPins);
    HRESULT STDMETHODCALLTYPE GetPinInstanceCount(ULONG PinId, PKSPIN_CINSTANCES Instances);
    HRESULT STDMETHODCALLTYPE GetPinDataflow(ULONG PinId, KSPIN_DATAFLOW * DataFlow);
    HRESULT STDMETHODCALLTYPE GetPinName(ULONG PinId, KSPIN_DATAFLOW DataFlow, ULONG PinCount, LPWSTR * OutPinName);
    HRESULT STDMETHODCALLTYPE GetPinCommunication(ULONG PinId, KSPIN_COMMUNICATION * Communication);
    HRESULT STDMETHODCALLTYPE CreatePins();
    HRESULT STDMETHODCALLTYPE GetMediaSeekingFormats(PKSMULTIPLE_ITEM *FormatList);
    HRESULT STDMETHODCALLTYPE CreateClockInstance();
    HRESULT STDMETHODCALLTYPE PerformClockProperty(ULONG PropertyId, ULONG PropertyFlags, PVOID OutputBuffer, ULONG OutputBufferSize);
    HRESULT STDMETHODCALLTYPE SetPinState(KSSTATE State);


protected:
    LONG m_Ref;
    IFilterGraph *m_pGraph;
    IReferenceClock * m_ReferenceClock;
    FILTER_STATE m_FilterState;
    HANDLE m_hDevice;
    ProxyPluginVector m_Plugins;
    PinVector m_Pins;
    LPWSTR m_DevicePath;
    CLSID m_DeviceInterfaceGUID;
    HANDLE m_hClock;
    CRITICAL_SECTION m_Lock;
};

CKsProxy::CKsProxy() : m_Ref(0),
                       m_pGraph(0),
                       m_FilterState(State_Stopped),
                       m_hDevice(0),
                       m_Plugins(),
                       m_Pins(),
                       m_DevicePath(0),
                       m_hClock(0)
{
    m_ReferenceClock = this;
    InitializeCriticalSection(&m_Lock);
}


HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    *Output = NULL;

    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IBaseFilter))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IPersistPropertyBag))
    {
        *Output = (IPersistPropertyBag*)(this);
        reinterpret_cast<IPersistPropertyBag*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IAMDeviceRemoval))
    {
        *Output = (IAMDeviceRemoval*)(this);
        reinterpret_cast<IAMDeviceRemoval*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IPersistStream))
    {
        *Output = (IPersistStream*)(this);
        reinterpret_cast<IPersistStream*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IPersist))
    {
        *Output = (IPersistStream*)(this);
        reinterpret_cast<IPersist*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsObject))
    {
        *Output = (IKsObject*)(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsClock))
    {
        *Output = (IKsClock*)(this);
        reinterpret_cast<IKsClock*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IReferenceClock))
    {
        if (!m_hClock)
        {
            HRESULT hr = CreateClockInstance();
            if (FAILED(hr))
                return hr;
        }

        *Output = (IReferenceClock*)(this);
        reinterpret_cast<IReferenceClock*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IMediaSeeking))
    {
        *Output = (IMediaSeeking*)(this);
        reinterpret_cast<IMediaSeeking*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IAMFilterMiscFlags))
    {
        *Output = (IAMFilterMiscFlags*)(this);
        reinterpret_cast<IAMFilterMiscFlags*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsControl))
    {
        *Output = (IKsControl*)(this);
        reinterpret_cast<IKsControl*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsPropertySet))
    {
        *Output = (IKsPropertySet*)(this);
        reinterpret_cast<IKsPropertySet*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsTopology))
    {
        *Output = (IKsTopology*)(this);
        reinterpret_cast<IKsTopology*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsAggregateControl))
    {
        *Output = (IKsAggregateControl*)(this);
        reinterpret_cast<IKsAggregateControl*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_IKsClockPropertySet))
    {
        if (!m_hClock)
        {
            HRESULT hr = CreateClockInstance();
            if (FAILED(hr))
                return hr;
        }

        *Output = (IKsClockPropertySet*)(this);
        reinterpret_cast<IKsClockPropertySet*>(*Output)->AddRef();
        return NOERROR;
    }
    else if (IsEqualGUID(refiid, IID_ISpecifyPropertyPages))
    {
        *Output = (ISpecifyPropertyPages*)(this);
        reinterpret_cast<ISpecifyPropertyPages*>(*Output)->AddRef();
        return NOERROR;
    }

    for(ULONG Index = 0; Index < m_Plugins.size(); Index++)
    {
        if (m_Pins[Index])
        {
            HRESULT hr = m_Plugins[Index]->QueryInterface(refiid, Output);
            if (SUCCEEDED(hr))
            {
#ifdef KSPROXY_TRACE
                WCHAR Buffer[100];
                LPOLESTR lpstr;
                StringFromCLSID(refiid, &lpstr);
                swprintf(Buffer, L"CKsProxy::QueryInterface plugin %lu supports interface %s\n", Index, lpstr);
                OutputDebugStringW(Buffer);
                CoTaskMemFree(lpstr);
#endif
                return hr;
            }
        }
    }
#ifdef KSPROXY_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CKsProxy::QueryInterface: NoInterface for %s !!!\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);
#endif

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// ISpecifyPropertyPages
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPages(CAUUID *pPages)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetPages NotImplemented\n");
#endif

    if (!pPages)
        return E_POINTER;

    pPages->cElems = 0;
    pPages->pElems = NULL;

    return S_OK;
}

//-------------------------------------------------------------------
// IKsClockPropertySet interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::CreateClockInstance()
{
    HRESULT hr;
    HANDLE hPin = INVALID_HANDLE_VALUE;
    ULONG Index;
    PIN_DIRECTION PinDir;
    IKsObject *pObject;
    KSCLOCK_CREATE ClockCreate;

    // find output pin and handle
    for(Index = 0; Index < m_Pins.size(); Index++)
    {
        //get pin
        IPin * pin = m_Pins[Index];
        if (!pin)
            continue;

        // get direction
        hr = pin->QueryDirection(&PinDir);
        if (FAILED(hr))
            continue;

        // query IKsObject interface
        hr = pin->QueryInterface(IID_IKsObject, (void**)&pObject);
        if (FAILED(hr))
            continue;


        // get pin handle
        hPin = pObject->KsGetObjectHandle();

        //release IKsObject
        pObject->Release();

        if (hPin != INVALID_HANDLE_VALUE)
            break;
    }

    if (hPin == INVALID_HANDLE_VALUE)
    {
        // clock can only be instantiated on a pin handle
        return E_NOTIMPL;
    }

    if (m_hClock)
    {
        // release clock handle
        CloseHandle(m_hClock);
    }

    //setup clock create request
    ClockCreate.CreateFlags = 0;

    // setup clock create request
    hr = KsCreateClock(hPin, &ClockCreate, &m_hClock); // FIXME KsCreateClock returns NTSTATUS
    if (SUCCEEDED(hr))
    {
        // failed to create clock
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::PerformClockProperty(
    ULONG PropertyId,
    ULONG PropertyFlags,
    PVOID OutputBuffer,
    ULONG OutputBufferSize)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;

    if (!m_hClock)
    {
        // create clock
        hr = CreateClockInstance();
        if (FAILED(hr))
            return hr;
    }

    // setup request
    Property.Set = KSPROPSETID_Clock;
    Property.Id = PropertyId;
    Property.Flags = PropertyFlags;

    hr = KsSynchronousDeviceControl(m_hClock, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)OutputBuffer, OutputBufferSize, &BytesReturned);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetTime(
    LONGLONG* Time)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_TIME, KSPROPERTY_TYPE_GET, (PVOID)Time, sizeof(LONGLONG));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsSetTime(
    LONGLONG Time)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsSetTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_TIME, KSPROPERTY_TYPE_SET, (PVOID)&Time, sizeof(LONGLONG));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetPhysicalTime(
    LONGLONG* Time)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetPhysicalTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_PHYSICALTIME, KSPROPERTY_TYPE_GET, (PVOID)Time, sizeof(LONGLONG));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsSetPhysicalTime(
    LONGLONG Time)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsSetPhysicalTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_PHYSICALTIME, KSPROPERTY_TYPE_SET, (PVOID)&Time, sizeof(LONGLONG));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetCorrelatedTime(
    KSCORRELATED_TIME* CorrelatedTime)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetCorrelatedTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_CORRELATEDTIME, KSPROPERTY_TYPE_GET, (PVOID)CorrelatedTime, sizeof(KSCORRELATED_TIME));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsSetCorrelatedTime(
    KSCORRELATED_TIME* CorrelatedTime)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsSetCorrelatedTime\n");
#endif
    return PerformClockProperty(KSPROPERTY_CLOCK_CORRELATEDTIME, KSPROPERTY_TYPE_SET, (PVOID)CorrelatedTime, sizeof(KSCORRELATED_TIME));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetCorrelatedPhysicalTime(
    KSCORRELATED_TIME* CorrelatedTime)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetCorrelatedPhysicalTime\n");
#endif
    return PerformClockProperty(KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME, KSPROPERTY_TYPE_GET, (PVOID)CorrelatedTime, sizeof(KSCORRELATED_TIME));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsSetCorrelatedPhysicalTime(
    KSCORRELATED_TIME* CorrelatedTime)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsSetCorrelatedPhysicalTime\n");
#endif

    return PerformClockProperty(KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME, KSPROPERTY_TYPE_SET, (PVOID)CorrelatedTime, sizeof(KSCORRELATED_TIME));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetResolution(
    KSRESOLUTION* Resolution)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetResolution\n");
#endif
    return PerformClockProperty(KSPROPERTY_CLOCK_RESOLUTION, KSPROPERTY_TYPE_GET, (PVOID)Resolution, sizeof(KSRESOLUTION));
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsGetState(
    KSSTATE* State)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetState\n");
#endif
    return PerformClockProperty(KSPROPERTY_CLOCK_STATE, KSPROPERTY_TYPE_GET, (PVOID)State, sizeof(KSSTATE));
}

//-------------------------------------------------------------------
// IReferenceClock interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::GetTime(
    REFERENCE_TIME *pTime)
{
    HRESULT hr;
    KSPROPERTY Property;
    ULONG BytesReturned;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetTime\n");
#endif

    if (!pTime)
        return E_POINTER;

    //
    //FIXME locks
    //

    if (!m_hClock)
    {
        // create clock
        hr = CreateClockInstance();
        if (FAILED(hr))
            return hr;
    }

    // setup request
    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_TIME;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_hClock, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pTime, sizeof(REFERENCE_TIME), &BytesReturned);

    // TODO
    // increment value
    //

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::AdviseTime(
    REFERENCE_TIME baseTime,
    REFERENCE_TIME streamTime,
    HEVENT hEvent,
    DWORD_PTR *pdwAdviseCookie)
{
    HRESULT hr;
    KSEVENT Property;
    ULONG BytesReturned;
    PKSEVENT_TIME_MARK Event;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::AdviseTime\n");
#endif

    //
    //FIXME locks
    //

    if (!pdwAdviseCookie)
        return E_POINTER;

    if (!m_hClock)
    {
        // create clock
        hr = CreateClockInstance();
        if (FAILED(hr))
            return hr;
    }

    // allocate event entry
    Event = (PKSEVENT_TIME_MARK)CoTaskMemAlloc(sizeof(KSEVENT_TIME_MARK));
    if (Event)
    {
        // setup request
        Property.Set = KSEVENTSETID_Clock;
        Property.Id = KSEVENT_CLOCK_POSITION_MARK;
        Property.Flags = KSEVENT_TYPE_ENABLE;

        Event->EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
        Event->EventData.EventHandle.Event = (HANDLE)hEvent;
        Event->EventData.Alignment.Alignment[0] = 0;
        Event->EventData.Alignment.Alignment[1] = 0;
        Event->MarkTime = baseTime + streamTime;

        // perform request
        hr = KsSynchronousDeviceControl(m_hClock, IOCTL_KS_ENABLE_EVENT, (PVOID)&Property, sizeof(KSEVENT), (PVOID)Event, sizeof(KSEVENT_TIME_MARK), &BytesReturned);
        if (SUCCEEDED(hr))
        {
            // store event handle
            *pdwAdviseCookie = (DWORD_PTR)Event;
        }
        else
        {
            // failed to enable event
            CoTaskMemFree(Event);
        }
    }
    else
    {
         hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::AdvisePeriodic(
    REFERENCE_TIME startTime,
    REFERENCE_TIME periodTime,
    HSEMAPHORE hSemaphore,
    DWORD_PTR *pdwAdviseCookie)
{
    HRESULT hr;
    KSEVENT Property;
    ULONG BytesReturned;
    PKSEVENT_TIME_INTERVAL Event;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::AdvisePeriodic\n");
#endif

    //
    //FIXME locks
    //

    if (!pdwAdviseCookie)
        return E_POINTER;

    if (!m_hClock)
    {
        // create clock
        hr = CreateClockInstance();
        if (FAILED(hr))
            return hr;
    }

    // allocate event entry
    Event = (PKSEVENT_TIME_INTERVAL)CoTaskMemAlloc(sizeof(KSEVENT_TIME_INTERVAL));
    if (Event)
    {
        // setup request
        Property.Set = KSEVENTSETID_Clock;
        Property.Id = KSEVENT_CLOCK_INTERVAL_MARK;
        Property.Flags = KSEVENT_TYPE_ENABLE;

        Event->EventData.NotificationType = KSEVENTF_SEMAPHORE_HANDLE;
        Event->EventData.SemaphoreHandle.Semaphore = (HANDLE)hSemaphore;
        Event->EventData.SemaphoreHandle.Reserved = 0;
        Event->EventData.SemaphoreHandle.Adjustment = 1;
        Event->TimeBase = startTime;
        Event->Interval = periodTime;

        // perform request
        hr = KsSynchronousDeviceControl(m_hClock, IOCTL_KS_ENABLE_EVENT, (PVOID)&Property, sizeof(KSEVENT), (PVOID)Event, sizeof(KSEVENT_TIME_INTERVAL), &BytesReturned);
        if (SUCCEEDED(hr))
        {
            // store event handle
            *pdwAdviseCookie = (DWORD_PTR)Event;
        }
        else
        {
            // failed to enable event
            CoTaskMemFree(Event);
        }
    }
    else
    {
         hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Unadvise(
    DWORD_PTR dwAdviseCookie)
{
    HRESULT hr;
    ULONG BytesReturned;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Unadvise\n");
#endif

    if (m_hClock)
    {
        //lets disable the event
        hr = KsSynchronousDeviceControl(m_hClock, IOCTL_KS_DISABLE_EVENT, (PVOID)dwAdviseCookie, sizeof(KSEVENTDATA), 0, 0, &BytesReturned);
        if (SUCCEEDED(hr))
        {
            // lets free event data
            CoTaskMemFree((LPVOID)dwAdviseCookie);
        }
    }
    else
    {
        // no clock available
        hr = E_FAIL;
    }

    return hr;
}

//-------------------------------------------------------------------
// IMediaSeeking interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::GetCapabilities(
    DWORD *pCapabilities)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr = S_OK;
    DWORD TempCaps;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_CAPABILITIES;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetCapabilities\n");
#endif


    if (!pCapabilities)
        return E_POINTER;


    *pCapabilities = (KS_SEEKING_CanSeekAbsolute | KS_SEEKING_CanSeekForwards | KS_SEEKING_CanSeekBackwards | KS_SEEKING_CanGetCurrentPos |
                      KS_SEEKING_CanGetStopPos | KS_SEEKING_CanGetDuration | KS_SEEKING_CanPlayBackwards);

    KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&pCapabilities, sizeof(KS_SEEKING_CAPABILITIES), &BytesReturned);
    // check if plugins support it
    for(Index = 0; Index < m_Plugins.size(); Index++)
    {
        // get plugin
        IUnknown * Plugin = m_Plugins[Index];

        if (!Plugin)
           continue;

        // query for IMediaSeeking interface
        IMediaSeeking *pSeek = NULL;
        hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
        if (FAILED(hr))
        {
            *pCapabilities = 0;
            return hr;
        }

        TempCaps = 0;
        // set time format
        hr = pSeek->GetCapabilities(&TempCaps);
        if (SUCCEEDED(hr))
        {
            // and with supported flags
            *pCapabilities = (*pCapabilities & TempCaps);
        }
        // release IMediaSeeking interface
        pSeek->Release();
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::CheckCapabilities(
    DWORD *pCapabilities)
{
    DWORD Capabilities;
    HRESULT hr;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::CheckCapabilities\n");
#endif

    if (!pCapabilities)
        return E_POINTER;

    if (!*pCapabilities)
        return E_FAIL;

    hr = GetCapabilities(&Capabilities);
    if (SUCCEEDED(hr))
    {
        if ((Capabilities | *pCapabilities) == Capabilities)
        {
            // all present
            return S_OK;
        }

        Capabilities = (Capabilities & *pCapabilities);
        if (Capabilities)
        {
            // not all present
            *pCapabilities = Capabilities;
            return S_FALSE;
        }
        // no capabilities are present
        return E_FAIL;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetMediaSeekingFormats(
    PKSMULTIPLE_ITEM *FormatList)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_FORMATS;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // query for format size list
    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_MORE_DATA))
    {
        // allocate format list
        *FormatList = (PKSMULTIPLE_ITEM)CoTaskMemAlloc(BytesReturned);
        if (!*FormatList)
        {
            // not enough memory
            return E_OUTOFMEMORY;
        }

        // get format list
        hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)*FormatList, BytesReturned, &BytesReturned);
        if (FAILED(hr))
        {
            // failed to query format list
            CoTaskMemFree(FormatList);
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::IsFormatSupported(
    const GUID *pFormat)
{
    PKSMULTIPLE_ITEM FormatList;
    LPGUID pGuid;
    ULONG Index;
    HRESULT hr = S_FALSE;

#ifdef KSPROXY_TRACE
    WCHAR Buffer[100];
    LPOLESTR pstr;
    StringFromCLSID(*pFormat, &pstr);
    swprintf(Buffer, L"CKsProxy::IsFormatSupported %s\n",pstr);
    OutputDebugStringW(Buffer);
#endif

    if (!pFormat)
        return E_POINTER;

    // get media formats
    hr = GetMediaSeekingFormats(&FormatList);
    if (SUCCEEDED(hr))
    {
#ifdef KSPROXY_TRACE
        swprintf(Buffer, L"CKsProxy::IsFormatSupported NumFormat %lu\n",FormatList->Count);
        OutputDebugStringW(Buffer);
#endif

        //iterate through format list
        pGuid = (LPGUID)(FormatList + 1);
        for(Index = 0; Index < FormatList->Count; Index++)
        {
            if (IsEqualGUID(*pGuid, *pFormat))
            {
                CoTaskMemFree(FormatList);
                return S_OK;
            }
            pGuid++;
        }
        // free format list
        CoTaskMemFree(FormatList);
    }

    // check if all plugins support it
    for(Index = 0; Index < m_Plugins.size(); Index++)
    {
        // get plugin
        IUnknown * Plugin = m_Plugins[Index];

        if (!Plugin)
            continue;

        // query for IMediaSeeking interface
        IMediaSeeking *pSeek = NULL;
        hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
        if (FAILED(hr))
        {
            // plugin does not support interface
            hr = S_FALSE;
#ifdef KSPROXY_TRACE
            OutputDebugStringW(L"CKsProxy::IsFormatSupported plugin does not support IMediaSeeking interface\n");
#endif
            break;
        }

        // query if it is supported
        hr = pSeek->IsFormatSupported(pFormat);
        // release interface
        pSeek->Release();

        if (FAILED(hr) || hr == S_FALSE)
            break;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryPreferredFormat(
    GUID *pFormat)
{
    PKSMULTIPLE_ITEM FormatList;
    HRESULT hr;
    ULONG Index;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::QueryPreferredFormat\n");
#endif

    if (!pFormat)
        return E_POINTER;

    hr = GetMediaSeekingFormats(&FormatList);
    if (SUCCEEDED(hr))
    {
        if (FormatList->Count)
        {
            CopyMemory(pFormat, (FormatList + 1), sizeof(GUID));
            CoTaskMemFree(FormatList);
            return S_OK;
        }
        CoTaskMemFree(FormatList);
    }
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // get preferred time format
                hr = pSeek->QueryPreferredFormat(pFormat);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE)
                    return hr;
            }
        }
        hr = S_FALSE;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetTimeFormat(
    GUID *pFormat)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_TIMEFORMAT;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetTimeFormat\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pFormat, sizeof(GUID), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // set time format
                hr = pSeek->GetTimeFormat(pFormat);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE)
                    break;
            }
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::IsUsingTimeFormat(
    const GUID *pFormat)
{
    GUID Format;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::IsUsingTimeFormat\n");
#endif

    if (FAILED(QueryPreferredFormat(&Format)))
        return S_FALSE;

    if (IsEqualGUID(Format, *pFormat))
        return S_OK;
    else
        return S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetTimeFormat(
    const GUID *pFormat)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_TIMEFORMAT;
    Property.Flags = KSPROPERTY_TYPE_SET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::SetTimeFormat\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pFormat, sizeof(GUID), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (FAILED(hr))
            {
                //not supported
                break;
            }
            // set time format
            hr = pSeek->SetTimeFormat(pFormat);
            // release IMediaSeeking interface
            pSeek->Release();

            if (FAILED(hr))
                break;
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetDuration(
    LONGLONG *pDuration)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_DURATION;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetDuration\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pDuration, sizeof(LONGLONG), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // get duration
                hr = pSeek->GetStopPosition(pDuration);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetStopPosition(
    LONGLONG *pStop)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_STOPPOSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetStopPosition\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pStop, sizeof(LONGLONG), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // get stop position
                hr = pSeek->GetStopPosition(pStop);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetCurrentPosition(
    LONGLONG *pCurrent)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_POSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetCurrentPosition\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pCurrent, sizeof(LONGLONG), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // get current position
                hr = pSeek->GetCurrentPosition(pCurrent);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::ConvertTimeFormat(
    LONGLONG *pTarget,
    const GUID *pTargetFormat,
    LONGLONG Source,
    const GUID *pSourceFormat)
{
    KSP_TIMEFORMAT Property;
    ULONG BytesReturned, Index;
    GUID SourceFormat, TargetFormat;
    HRESULT hr;

    Property.Property.Set = KSPROPSETID_MediaSeeking;
    Property.Property.Id = KSPROPERTY_MEDIASEEKING_CONVERTTIMEFORMAT;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::ConvertTimeFormat\n");
#endif

    if (!pTargetFormat)
    {
        // get current format
        hr = GetTimeFormat(&TargetFormat);
        if (FAILED(hr))
            return hr;

        pTargetFormat = &TargetFormat;
    }

    if (!pSourceFormat)
    {
        // get current format
        hr = GetTimeFormat(&SourceFormat);
        if (FAILED(hr))
            return hr;

        pSourceFormat = &SourceFormat;
    }

    Property.SourceFormat = *pSourceFormat;
    Property.TargetFormat = *pTargetFormat;
    Property.Time = Source;


    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_TIMEFORMAT), (PVOID)pTarget, sizeof(LONGLONG), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        //default error
        hr = E_NOTIMPL;

        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // convert time format
                hr = pSeek->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetPositions(
    LONGLONG *pCurrent,
    DWORD dwCurrentFlags,
    LONGLONG *pStop,
    DWORD dwStopFlags)
{
    KSPROPERTY Property;
    KSPROPERTY_POSITIONS Positions;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_POSITIONS;
    Property.Flags = KSPROPERTY_TYPE_SET;

    Positions.Current = *pCurrent;
    Positions.CurrentFlags = (KS_SEEKING_FLAGS)dwCurrentFlags;
    Positions.Stop = *pStop;
    Positions.StopFlags = (KS_SEEKING_FLAGS)dwStopFlags;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::SetPositions\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&Positions, sizeof(KSPROPERTY_POSITIONS), &BytesReturned);
    if (SUCCEEDED(hr))
    {
        if (dwCurrentFlags & AM_SEEKING_ReturnTime)
        {
            // retrieve current position
            hr = GetCurrentPosition(pCurrent);
        }

        if (SUCCEEDED(hr))
        {
            if (dwStopFlags & AM_SEEKING_ReturnTime)
            {
                // retrieve current position
                hr = GetStopPosition(pStop);
            }
        }
        return hr;
    }
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        hr = E_NOTIMPL;

        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // set positions
                hr = pSeek->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
                // release IMediaSeeking interface
                pSeek->Release();

                if (FAILED(hr))
                    break;
            }
        }
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPositions(
    LONGLONG *pCurrent,
    LONGLONG *pStop)
{
    HRESULT hr;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetPositions\n");
#endif

    hr = GetCurrentPosition(pCurrent);
    if (SUCCEEDED(hr))
        hr = GetStopPosition(pStop);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetAvailable(
    LONGLONG *pEarliest,
    LONGLONG *pLatest)
{
    KSPROPERTY Property;
    KSPROPERTY_MEDIAAVAILABLE Media;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_AVAILABLE;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetAvailable\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&Media, sizeof(KSPROPERTY_MEDIAAVAILABLE), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            hr = E_NOTIMPL;
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // delegate call
                hr = pSeek->GetAvailable(pEarliest, pLatest);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
    }
    else if (SUCCEEDED(hr))
    {
        *pEarliest = Media.Earliest;
        *pLatest = Media.Latest;
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetRate(
    double dRate)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::SetRate\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetRate(
    double *pdRate)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetRate\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPreroll(
    LONGLONG *pllPreroll)
{
    KSPROPERTY Property;
    ULONG BytesReturned, Index;
    HRESULT hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_PREROLL;
    Property.Flags = KSPROPERTY_TYPE_GET;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetPreroll\n");
#endif

    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pllPreroll, sizeof(LONGLONG), &BytesReturned);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
    {
        // check if all plugins support it
        for(Index = 0; Index < m_Plugins.size(); Index++)
        {
            // get plugin
            IUnknown * Plugin = m_Plugins[Index];

            if (!Plugin)
               continue;

            // query for IMediaSeeking interface
            IMediaSeeking *pSeek = NULL;
            hr = Plugin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek);
            if (SUCCEEDED(hr))
            {
                // get preroll
                hr = pSeek->GetPreroll(pllPreroll);
                // release IMediaSeeking interface
                pSeek->Release();

                if (hr != S_FALSE) // plugin implements it
                     break;
            }
        }
        hr = E_NOTIMPL;
    }
    return hr;
}

//-------------------------------------------------------------------
// IAMFilterMiscFlags interface
//

ULONG
STDMETHODCALLTYPE
CKsProxy::GetMiscFlags()
{
    ULONG Index;
    ULONG Flags = 0;
    HRESULT hr;
    PIN_DIRECTION PinDirection;
    KSPIN_COMMUNICATION Communication;


    for(Index = 0; Index < m_Pins.size(); Index++)
    {
        // get current pin
        IPin * pin = m_Pins[Index];
        // query direction
        hr = pin->QueryDirection(&PinDirection);
        if (SUCCEEDED(hr))
        {
            if (PinDirection == PINDIR_INPUT)
            {
                if (SUCCEEDED(GetPinCommunication(Index, //FIXME verify PinId
                                        &Communication)))
                {
                    if (Communication != KSPIN_COMMUNICATION_NONE && Communication != KSPIN_COMMUNICATION_BRIDGE)
                    {
                        Flags |= AM_FILTER_MISC_FLAGS_IS_SOURCE;
                    }
                }
            }
        }
    }

#ifdef KSPROXY_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CKsProxy::GetMiscFlags stub Flags %x\n", Flags);
    OutputDebugStringW(Buffer);
#endif

    return Flags;
}

//-------------------------------------------------------------------
// IKsControl
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::KsProperty(
    PKSPROPERTY Property,
    ULONG PropertyLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsProperty\n");
#endif

    assert(m_hDevice != 0);
    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)Property, PropertyLength, (PVOID)PropertyData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsMethod(
    PKSMETHOD Method,
    ULONG MethodLength,
    LPVOID MethodData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsMethod\n");
#endif

    assert(m_hDevice != 0);
    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_METHOD, (PVOID)Method, MethodLength, (PVOID)MethodData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsEvent(
    PKSEVENT Event,
    ULONG EventLength,
    LPVOID EventData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsEvent\n");
#endif

    assert(m_hDevice != 0);
    if (EventLength)
        return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_ENABLE_EVENT, (PVOID)Event, EventLength, (PVOID)EventData, DataLength, BytesReturned);
    else
        return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_DISABLE_EVENT, (PVOID)Event, EventLength, NULL, 0, BytesReturned);
}


//-------------------------------------------------------------------
// IKsPropertySet
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData)
{
    ULONG BytesReturned;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Set\n");
#endif

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
CKsProxy::Get(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned)
{
    ULONG BytesReturned;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Get\n");
#endif

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
CKsProxy::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::QuerySupported\n");
#endif

    Property.Set = guidPropSet;
    Property.Id = dwPropID;
    Property.Flags = KSPROPERTY_TYPE_SETSUPPORT;

    return KsProperty(&Property, sizeof(KSPROPERTY), pTypeSupport, sizeof(DWORD), &BytesReturned);
}


//-------------------------------------------------------------------
// IKsTopology interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::CreateNodeInstance(
    ULONG NodeId,
    ULONG Flags,
    ACCESS_MASK DesiredAccess,
    IUnknown* UnkOuter,
    REFGUID InterfaceId,
    LPVOID* Interface)
{
    HRESULT hr;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::CreateNodeInstance\n");
#endif

    *Interface = NULL;

    if (IsEqualIID(IID_IUnknown, InterfaceId) || !UnkOuter)
    {
        hr = CKsNode_Constructor(UnkOuter, m_hDevice, NodeId, DesiredAccess, InterfaceId, Interface);
    }
    else
    {
        // interface not supported
        hr = E_NOINTERFACE;
    }

    return hr;
}

//-------------------------------------------------------------------
// IKsAggregateControl interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::KsAddAggregate(
    IN REFGUID AggregateClass)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsAddAggregate NotImplemented\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::KsRemoveAggregate(
    REFGUID AggregateClass)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsRemoveAggregate NotImplemented\n");
#endif

    return E_NOTIMPL;
}


//-------------------------------------------------------------------
// IPersistStream interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::IsDirty()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::IsDirty Notimplemented\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Load(
    IStream *pStm)
{
    HRESULT hr;
    WCHAR Buffer[1000];
    AM_MEDIA_TYPE MediaType;
    ULONG BytesReturned;
    LONG Length;

    ULONG PinId;
    LPOLESTR pMajor, pSub, pFormat;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Load\n");
#endif

#if 0
    ULONG Version = ReadInt(pStm, hr);
    if (Version != 1)
        return E_FAIL;
#endif

    hr = pStm->Read(&Length, sizeof(ULONG), &BytesReturned);
    swprintf(Buffer, L"Length hr %x hr length %lu\n", hr, Length);
    OutputDebugStringW(Buffer);

    do
    {
        hr = pStm->Read(&PinId, sizeof(ULONG), &BytesReturned);
        swprintf(Buffer, L"Read: hr %08x PinId %lx BytesReturned %lu\n", hr, PinId, BytesReturned);
        OutputDebugStringW(Buffer);

        if (FAILED(hr) || !BytesReturned)
            break;

        Length -= BytesReturned;

        hr = pStm->Read(&MediaType, sizeof(AM_MEDIA_TYPE), &BytesReturned);
        if (FAILED(hr) || BytesReturned != sizeof(AM_MEDIA_TYPE))
        {
            swprintf(Buffer, L"Read failed with %lx\n", hr);
            OutputDebugStringW(Buffer);
            break;
        }


        StringFromIID(MediaType.majortype, &pMajor);
        StringFromIID(MediaType.subtype , &pSub);
        StringFromIID(MediaType.formattype, &pFormat);

        swprintf(Buffer, L"BytesReturned %lu majortype %s subtype %s bFixedSizeSamples %u bTemporalCompression %u lSampleSize %u formattype %s, pUnk %p cbFormat %u pbFormat %p\n", BytesReturned, pMajor, pSub, MediaType.bFixedSizeSamples, MediaType.bTemporalCompression, MediaType.lSampleSize, pFormat, MediaType.pUnk, MediaType.cbFormat, MediaType.pbFormat);
        OutputDebugStringW(Buffer);

        Length -= BytesReturned;


        if (MediaType.cbFormat)
        {
            MediaType.pbFormat = (BYTE*)CoTaskMemAlloc(MediaType.cbFormat);
            if (!MediaType.pbFormat)
                return E_OUTOFMEMORY;

            hr = pStm->Read(&MediaType.pbFormat, sizeof(MediaType.cbFormat), &BytesReturned);
            if (FAILED(hr))
            {
                swprintf(Buffer, L"ReadFormat failed with %lx\n", hr);
                OutputDebugStringW(Buffer);
                break;
            }
            Length -= BytesReturned;
        }

    }while(Length > 0);

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Save(
    IStream *pStm,
    BOOL fClearDirty)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Save Notimplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetSizeMax(
    ULARGE_INTEGER *pcbSize)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetSizeMax Notimplemented\n");
#endif

    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IAMDeviceRemoval interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::DeviceInfo(CLSID *pclsidInterfaceClass, LPWSTR *pwszSymbolicLink)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::DeviceInfo\n");
#endif

    if (!m_DevicePath)
    {
        // object not initialized
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
    }

    // copy device interface guid
    CopyMemory(pclsidInterfaceClass, &m_DeviceInterfaceGUID, sizeof(GUID));

    if (pwszSymbolicLink)
    {
        *pwszSymbolicLink = (LPWSTR)CoTaskMemAlloc((wcslen(m_DevicePath)+1) * sizeof(WCHAR));
        if (!*pwszSymbolicLink)
            return E_OUTOFMEMORY;

        wcscpy(*pwszSymbolicLink, m_DevicePath);
    }
    return S_OK;
}
HRESULT
STDMETHODCALLTYPE
CKsProxy::Reassociate(void)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Reassociate\n");
#endif

    if (!m_DevicePath || m_hDevice)
    {
        // file path not available
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
    }

    m_hDevice = CreateFileW(m_DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (!m_hDevice)
    {
        // failed to open device
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    // success
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Disassociate(void)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Disassociate\n");
#endif

    if (!m_hDevice)
        return E_HANDLE;

    CloseHandle(m_hDevice);
    m_hDevice = NULL;
    return NOERROR;
}

//-------------------------------------------------------------------
// IKsClock interface
//

HANDLE
STDMETHODCALLTYPE
CKsProxy::KsGetClockHandle()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetClockHandle\n");
#endif

    return m_hClock;
}


//-------------------------------------------------------------------
// IKsObject interface
//

HANDLE
STDMETHODCALLTYPE
CKsProxy::KsGetObjectHandle()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::KsGetObjectHandle\n");
#endif

    return m_hDevice;
}

//-------------------------------------------------------------------
// IPersistPropertyBag interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::InitNew( void)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::InitNew\n");
#endif

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetSupportedSets(
    LPGUID * pOutGuid,
    PULONG NumGuids)
{
    KSPROPERTY Property;
    LPGUID pGuid;
    ULONG NumProperty = 0;
    ULONG NumMethods = 0;
    ULONG NumEvents = 0;
    ULONG Length;
    ULONG BytesReturned;
    HRESULT hr;

    Property.Set = GUID_NULL;
    Property.Id = 0;
    Property.Flags = KSPROPERTY_TYPE_SETSUPPORT;

    KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &NumProperty);
    KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_METHOD, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &NumMethods);
    KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_ENABLE_EVENT, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &NumEvents);

    Length = NumProperty + NumMethods + NumEvents;

    // allocate guid buffer
    pGuid = (LPGUID)CoTaskMemAlloc(Length);
    if (!pGuid)
    {
        // failed
        return E_OUTOFMEMORY;
    }

    NumProperty /= sizeof(GUID);
    NumMethods /= sizeof(GUID);
    NumEvents /= sizeof(GUID);

    // get all properties
    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)pGuid, Length, &BytesReturned);
    if (FAILED(hr))
    {
        CoTaskMemFree(pGuid);
        return E_FAIL;
    }
    Length -= BytesReturned;

    // get all methods
    if (Length)
    {
        hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_METHOD, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&pGuid[NumProperty], Length, &BytesReturned);
        if (FAILED(hr))
        {
            CoTaskMemFree(pGuid);
            return E_FAIL;
        }
        Length -= BytesReturned;
    }

    // get all events
    if (Length)
    {
        hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_ENABLE_EVENT, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&pGuid[NumProperty+NumMethods], Length, &BytesReturned);
        if (FAILED(hr))
        {
            CoTaskMemFree(pGuid);
            return E_FAIL;
        }
        Length -= BytesReturned;
    }

#ifdef KSPROXY_TRACE
    WCHAR Buffer[200];
    swprintf(Buffer, L"NumProperty %lu NumMethods %lu NumEvents %lu\n", NumProperty, NumMethods, NumEvents);
    OutputDebugStringW(Buffer);
#endif

    *pOutGuid = pGuid;
    *NumGuids = NumProperty+NumEvents+NumMethods;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::LoadProxyPlugins(
    LPGUID pGuids,
    ULONG NumGuids)
{
    ULONG Index;
    LPOLESTR pStr;
    HKEY hKey, hSubKey;
    HRESULT hr;
    IUnknown * pUnknown;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\MediaInterfaces", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        OutputDebugStringW(L"CKsProxy::LoadProxyPlugins failed to open MediaInterfaces key\n");
        return E_FAIL;
    }

    // enumerate all sets
    for(Index = 0; Index < NumGuids; Index++)
    {
        // convert to string
        hr = StringFromCLSID(pGuids[Index], &pStr);
        if (FAILED(hr))
            return E_FAIL;

        // now try open class key
        if (RegOpenKeyExW(hKey, pStr, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
        {
            // no plugin for that set exists
            CoTaskMemFree(pStr);
            continue;
        }

        // try load plugin
        hr = CoCreateInstance(pGuids[Index], (IBaseFilter*)this, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnknown);
        if (SUCCEEDED(hr))
        {
            // store plugin
            m_Plugins.push_back(pUnknown);
        }
        // close key
        RegCloseKey(hSubKey);
    }

    // close media interfaces key
    RegCloseKey(hKey);
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetNumberOfPins(
    PULONG NumPins)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    // setup request
    Property.Set = KSPROPSETID_Pin;
    Property.Id = KSPROPERTY_PIN_CTYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;

    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)NumPins, sizeof(ULONG), &BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPinInstanceCount(
    ULONG PinId,
    PKSPIN_CINSTANCES Instances)
{
    KSP_PIN Property;
    ULONG BytesReturned;

    // setup request
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_CINSTANCES;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = PinId;
    Property.Reserved = 0;

    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)Instances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPinCommunication(
    ULONG PinId,
    KSPIN_COMMUNICATION * Communication)
{
    KSP_PIN Property;
    ULONG BytesReturned;

    // setup request
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = PinId;
    Property.Reserved = 0;

    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPinDataflow(
    ULONG PinId,
    KSPIN_DATAFLOW * DataFlow)
{
    KSP_PIN Property;
    ULONG BytesReturned;

    // setup request
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_DATAFLOW;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = PinId;
    Property.Reserved = 0;

    return KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), (PVOID)DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetPinName(
    ULONG PinId,
    KSPIN_DATAFLOW DataFlow,
    ULONG PinCount,
    LPWSTR * OutPinName)
{
    KSP_PIN Property;
    LPWSTR PinName;
    ULONG BytesReturned;
    HRESULT hr;
    WCHAR Buffer[100];

    // setup request
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_NAME;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.PinId = PinId;
    Property.Reserved = 0;

    // #1 try get it from pin directly
    hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_MORE_DATA))
    {
        // allocate pin name
        PinName = (LPWSTR)CoTaskMemAlloc(BytesReturned);
        if (!PinName)
            return E_OUTOFMEMORY;

        // retry with allocated buffer
        hr = KsSynchronousDeviceControl(m_hDevice, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_PIN), PinName, BytesReturned, &BytesReturned);
        if (SUCCEEDED(hr))
        {
            *OutPinName = PinName;
            return hr;
        }

        //free buffer
        CoTaskMemFree(PinName);
    }

    //
    // TODO: retrieve pin name from topology node
    //

    if (DataFlow == KSPIN_DATAFLOW_IN)
    {
        swprintf(Buffer, L"Input%lu", PinCount);
    }
    else
    {
        swprintf(Buffer, L"Output%lu", PinCount);
    }

    // allocate pin name
    PinName = (LPWSTR)CoTaskMemAlloc((wcslen(Buffer)+1) * sizeof(WCHAR));
    if (!PinName)
        return E_OUTOFMEMORY;

    // copy pin name
    wcscpy(PinName, Buffer);

    // store result
    *OutPinName = PinName;
    // done
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::CreatePins()
{
    ULONG NumPins, Index;
    KSPIN_CINSTANCES Instances;
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    HRESULT hr;
    LPWSTR PinName;
    IPin * pPin;
    ULONG InputPin = 0;
    ULONG OutputPin = 0;

    // get number of pins
    hr = GetNumberOfPins(&NumPins);
    if (FAILED(hr))
        return hr;

    for(Index = 0; Index < NumPins; Index++)
    {
        // query current instance count
        hr = GetPinInstanceCount(Index, &Instances);
        if (FAILED(hr))
        {
#ifdef KSPROXY_TRACE
            WCHAR Buffer[100];
            swprintf(Buffer, L"CKsProxy::CreatePins GetPinInstanceCount failed with %lx\n", hr);
            OutputDebugStringW(Buffer);
#endif
            continue;
        }


        // query pin communication;
        hr = GetPinCommunication(Index, &Communication);
        if (FAILED(hr))
        {
#ifdef KSPROXY_TRACE
            WCHAR Buffer[100];
            swprintf(Buffer, L"CKsProxy::CreatePins GetPinCommunication failed with %lx\n", hr);
            OutputDebugStringW(Buffer);
#endif
            continue;
        }

        if (Instances.CurrentCount == Instances.PossibleCount)
        {
            // already maximum reached for this pin
#ifdef KSPROXY_TRACE
            WCHAR Buffer[100];
            swprintf(Buffer, L"CKsProxy::CreatePins Instances.CurrentCount == Instances.PossibleCount\n");
            OutputDebugStringW(Buffer);
#endif
            continue;
        }

        // get direction of pin
        hr = GetPinDataflow(Index, &DataFlow);
        if (FAILED(hr))
        {
#ifdef KSPROXY_TRACE
            WCHAR Buffer[100];
            swprintf(Buffer, L"CKsProxy::CreatePins GetPinDataflow failed with %lx\n", hr);
            OutputDebugStringW(Buffer);
#endif
            continue;
        }

        if (DataFlow == KSPIN_DATAFLOW_IN)
            hr = GetPinName(Index, DataFlow, InputPin, &PinName);
        else
            hr = GetPinName(Index, DataFlow, OutputPin, &PinName);

        if (FAILED(hr))
        {
#ifdef KSPROXY_TRACE
            WCHAR Buffer[100];
            swprintf(Buffer, L"CKsProxy::CreatePins GetPinName failed with %lx\n", hr);
            OutputDebugStringW(Buffer);
#endif
            continue;
        }

        // construct the pins
        if (DataFlow == KSPIN_DATAFLOW_IN)
        {
            hr = CInputPin_Constructor((IBaseFilter*)this, PinName, m_hDevice, Index, Communication, IID_IPin, (void**)&pPin);
            if (FAILED(hr))
            {
#ifdef KSPROXY_TRACE
                WCHAR Buffer[100];
                swprintf(Buffer, L"CKsProxy::CreatePins CInputPin_Constructor failed with %lx\n", hr);
                OutputDebugStringW(Buffer);
#endif
                CoTaskMemFree(PinName);
                continue;
            }
            InputPin++;
        }
        else
        {
            hr = COutputPin_Constructor((IBaseFilter*)this, PinName, Index, Communication, IID_IPin, (void**)&pPin);
            if (FAILED(hr))
            {
#ifdef KSPROXY_TRACE
                WCHAR Buffer[100];
                swprintf(Buffer, L"CKsProxy::CreatePins COutputPin_Constructor failed with %lx\n", hr);
                OutputDebugStringW(Buffer);
#endif
                CoTaskMemFree(PinName);
                continue;
            }
            OutputPin++;
        }

        // store pins
        m_Pins.push_back(pPin);

#ifdef KSPROXY_TRACE
        WCHAR Buffer[100];
        swprintf(Buffer, L"Index %lu DataFlow %lu Name %s\n", Index, DataFlow, PinName);
        OutputDebugStringW(Buffer);
#endif

    }

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    HRESULT hr;
    VARIANT varName;
    LPGUID pGuid;
    ULONG NumGuids = 0;
    HDEVINFO hList;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;

#ifdef KSPROXY_TRACE
    WCHAR Buffer[100];
    OutputDebugStringW(L"CKsProxy::Load\n");
#endif

    // read device path
    varName.vt = VT_BSTR;
    hr = pPropBag->Read(L"DevicePath", &varName, pErrorLog);

    if (FAILED(hr))
    {
#ifdef KSPROXY_TRACE
        swprintf(Buffer, L"CKsProxy::Load Read %lx\n", hr);
        OutputDebugStringW(Buffer);
#endif
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"DevicePath: ");
    OutputDebugStringW(varName.bstrVal);
    OutputDebugStringW(L"\n");
#endif

    // create device list
    hList = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    if (hList == INVALID_HANDLE_VALUE)
    {
        // failed to create device list
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiOpenDeviceInterfaceW(hList, (PCWSTR)varName.bstrVal, 0, &DeviceInterfaceData))
    {
        // failed to open device interface
        SetupDiDestroyDeviceInfoList(hList);
    }

    // FIXME handle device interface links(aliases)
    CopyMemory(&m_DeviceInterfaceGUID, &DeviceInterfaceData.InterfaceClassGuid, sizeof(GUID));

    // close device info list
   SetupDiDestroyDeviceInfoList(hList);

    // open device
    m_hDevice = CreateFileW(varName.bstrVal, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    if (m_hDevice == INVALID_HANDLE_VALUE)
    {
        // failed to open device
#ifdef KSPROXY_TRACE
        swprintf(Buffer, L"CKsProxy:: failed to open device with %lx\n", GetLastError());
        OutputDebugStringW(Buffer);
#endif
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    // store device path
    m_DevicePath = varName.bstrVal;

    // get all supported sets
    hr = GetSupportedSets(&pGuid, &NumGuids);
    if (FAILED(hr))
    {
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
        return hr;
    }

    // load all proxy plugins
    hr = LoadProxyPlugins(pGuid, NumGuids);
    if (FAILED(hr))
    {
#if 0 //HACK
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
        return hr;
#endif
        OutputDebugStringW(L"CKsProxy::LoadProxyPlugins failed!\n");
    }

    // free sets
    CoTaskMemFree(pGuid);

    // now create the input / output pins
    hr = CreatePins();

#ifdef KSPROXY_TRACE
    swprintf(Buffer, L"CKsProxy::Load CreatePins %lx\n", hr);
    OutputDebugStringW(Buffer);
#endif

    //HACK
    hr = S_OK;

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Save\n");
#endif
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IBaseFilter interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetClassID(
    CLSID *pClassID)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetClassID\n");
#endif
    CopyMemory(pClassID, &CLSID_Proxy, sizeof(GUID));

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Stop()
{
    HRESULT hr;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Stop\n");
#endif

    EnterCriticalSection(&m_Lock);

    hr = SetPinState(KSSTATE_STOP);
    if (SUCCEEDED(hr))
        m_FilterState = State_Stopped;

    LeaveCriticalSection(&m_Lock);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Pause()
{
    HRESULT hr = S_OK;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Pause\n");
#endif

    EnterCriticalSection(&m_Lock);

    if (m_FilterState == State_Running)
    {
        hr = SetPinState(KSSTATE_STOP);
    }
    if (SUCCEEDED(hr))
    {
        if (m_FilterState == State_Stopped)
        {
            hr = SetPinState(KSSTATE_PAUSE);
        }
    }

    if (SUCCEEDED(hr))
        m_FilterState = State_Paused;

    LeaveCriticalSection(&m_Lock);
    return hr;

}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Run(
    REFERENCE_TIME tStart)
{
    HRESULT hr;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Run\n");
#endif

    EnterCriticalSection(&m_Lock);

    if (m_FilterState == State_Stopped)
    {
        LeaveCriticalSection(&m_Lock);
        // setting filter state to pause
        hr = Pause();
        if (FAILED(hr))
            return hr;

        EnterCriticalSection(&m_Lock);
        assert(m_FilterState == State_Paused);
    }

    hr = SetPinState(KSSTATE_RUN);

    if (SUCCEEDED(hr))
    {
        m_FilterState = State_Running;
    }

    LeaveCriticalSection(&m_Lock);
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetPinState(
    KSSTATE State)
{
    HRESULT hr = S_OK;
    ULONG Index;
    IKsObject *pObject;
    ULONG BytesReturned;
    KSPROPERTY Property;
    PIN_INFO PinInfo;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    // set all pins to running state
    for(Index = 0; Index < m_Pins.size(); Index++)
    {
        IPin * Pin = m_Pins[Index];
        if (!Pin)
            continue;

        //check if the pin is connected
        IPin * TempPin;
        hr = Pin->ConnectedTo(&TempPin);
        if (FAILED(hr))
        {
            // skip unconnected pins
            continue;
        }

        // release connected pin
        TempPin->Release();

        // query for the pin info
        hr = Pin->QueryPinInfo(&PinInfo);

        if (SUCCEEDED(hr))
        {
            if (PinInfo.pFilter)
                PinInfo.pFilter->Release();

            if (PinInfo.dir == PINDIR_OUTPUT)
            {
                hr = COutputPin_SetState(Pin, State);
                if (SUCCEEDED(hr))
                    continue;
            }
        }

        //query IKsObject interface
        hr = Pin->QueryInterface(IID_IKsObject, (void**)&pObject);

        // get pin handle
        HANDLE hPin = pObject->KsGetObjectHandle();

        // sanity check
        assert(hPin && hPin != INVALID_HANDLE_VALUE);

        // now set state
        hr = KsSynchronousDeviceControl(hPin, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesReturned);

#ifdef KSPROXY_TRACE
        WCHAR Buffer[100];
        swprintf(Buffer, L"CKsProxy::SetPinState Index %u State %u hr %lx\n", Index, State, hr);
        OutputDebugStringW(Buffer);
#endif

        if (FAILED(hr))
            return hr;
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetState(
    DWORD dwMilliSecsTimeout,
    FILTER_STATE *State)
{
    if (!State)
        return E_POINTER;

    *State = m_FilterState;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetSyncSource(
    IReferenceClock *pClock)
{
    HRESULT hr;
    IKsClock *pKsClock;
    HANDLE hClock, hPin;
    ULONG Index;
    IPin * pin;
    IKsObject * pObject;
    KSPROPERTY Property;
    ULONG BytesReturned;
    PIN_DIRECTION PinDir;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::SetSyncSource\n");
#endif

    // FIXME
    // need locks

    if (pClock)
    {
        hr = pClock->QueryInterface(IID_IKsClock, (void**)&pKsClock);
        if (FAILED(hr))
        {
            hr = m_ReferenceClock->QueryInterface(IID_IKsClock, (void**)&pKsClock);
            if (FAILED(hr))
                return hr;
        }

        // get clock handle
        hClock = pKsClock->KsGetClockHandle();

        // release IKsClock interface
        pKsClock->Release();
        m_hClock = hClock;
    }
    else
    {
        // no clock handle
        m_hClock = NULL;
    }


    // distribute clock to all pins
    for(Index = 0; Index < m_Pins.size(); Index++)
    {
        // get current pin
        pin = m_Pins[Index];
        if (!pin)
            continue;

        // get IKsObject interface
        hr = pin->QueryInterface(IID_IKsObject, (void **)&pObject);
        if (SUCCEEDED(hr))
        {
            // get pin handle
            hPin = pObject->KsGetObjectHandle();
            if (hPin != INVALID_HANDLE_VALUE && hPin)
            {
                // set clock
                Property.Set = KSPROPSETID_Stream;
                Property.Id = KSPROPERTY_STREAM_MASTERCLOCK;
                Property.Flags = KSPROPERTY_TYPE_SET;

                // set master clock
                hr = KsSynchronousDeviceControl(hPin, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&m_hClock, sizeof(HANDLE), &BytesReturned);

                if (FAILED(hr))
                {
                    if (hr != MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND) &&
                        hr != MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND))
                    {
                        // failed to set master clock
                        pObject->Release();
                        WCHAR Buffer[100];
                        swprintf(Buffer, L"CKsProxy::SetSyncSource KSPROPERTY_STREAM_MASTERCLOCK failed with %lx\n", hr);
                        OutputDebugStringW(Buffer);
                        return hr;
                    }
                }
            }
            // release IKsObject
            pObject->Release();
        }

        // now get the direction
        hr = pin->QueryDirection(&PinDir);
        if (SUCCEEDED(hr))
        {
            if (PinDir == PINDIR_OUTPUT)
            {
                // notify pin via
                //CBaseStreamControl::SetSyncSource(pClock)
            }
        }
    }

    if (pClock)
    {
        pClock->AddRef();
    }

    if (m_ReferenceClock)
    {
        m_ReferenceClock->Release();
    }

    m_ReferenceClock = pClock;
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::SetSyncSource done\n");
#endif
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetSyncSource(
    IReferenceClock **pClock)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::GetSyncSource\n");
#endif

    if (!pClock)
        return E_POINTER;

    if (m_ReferenceClock)
        m_ReferenceClock->AddRef();

    *pClock = m_ReferenceClock;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::EnumPins(
    IEnumPins **ppEnum)
{
    return CEnumPins_fnConstructor(m_Pins, IID_IEnumPins, (void**)ppEnum);
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::FindPin(
    LPCWSTR Id, IPin **ppPin)
{
    ULONG PinId;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::FindPin\n");
#endif

    if (!ppPin)
        return E_POINTER;

    // convert to pin
    int ret = swscanf(Id, L"%u", &PinId);

    if (!ret || ret == EOF)
    {
        // invalid id
        return VFW_E_NOT_FOUND;
    }

    if (PinId >= m_Pins.size() || m_Pins[PinId] == NULL)
    {
        // invalid id
        return VFW_E_NOT_FOUND;
    }

    // found pin
    *ppPin = m_Pins[PinId];
    m_Pins[PinId]->AddRef();

    return S_OK;
}


HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryFilterInfo(
    FILTER_INFO *pInfo)
{
    if (!pInfo)
        return E_POINTER;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::QueryFilterInfo\n");
#endif

    pInfo->achName[0] = L'\0';
    pInfo->pGraph = m_pGraph;

    if (m_pGraph)
        m_pGraph->AddRef();

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::JoinFilterGraph(
    IFilterGraph *pGraph,
    LPCWSTR pName)
{
#ifdef KSPROXY_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CKsProxy::JoinFilterGraph pName %s pGraph %p m_Ref %u\n", pName, pGraph, m_Ref);
    OutputDebugStringW(Buffer);
#endif

    if (pGraph)
    {
        // joining filter graph
        m_pGraph = pGraph;
    }
    else
    {
        // leaving graph
        m_pGraph = 0;
    }

    return S_OK;
}


HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryVendorInfo(
    LPWSTR *pVendorInfo)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::QueryVendorInfo\n");
#endif
    return StringFromCLSID(CLSID_Proxy, pVendorInfo);
}

//-------------------------------------------------------------------
// IAMovieSetup interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::Register()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Register : NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Unregister()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsProxy::Unregister : NotImplemented\n");
#endif
    return E_NOTIMPL;
}

HRESULT
WINAPI
CKsProxy_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    WCHAR Buffer[100];
    LPOLESTR pstr;
    StringFromCLSID(riid, &pstr);
    swprintf(Buffer, L"CKsProxy_Constructor pUnkOuter %p riid %s\n", pUnkOuter, pstr);
    OutputDebugStringW(Buffer);
#endif

    CKsProxy * handler = new CKsProxy();

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
