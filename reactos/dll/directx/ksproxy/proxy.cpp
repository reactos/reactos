/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/proxy.cpp
 * PURPOSE:         IKsProxy interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IPersistPropertyBag = {0x37D84F60, 0x42CB, 0x11CE, {0x81, 0x35, 0x00, 0xAA, 0x00, 0x4B, 0xB8, 0x51}};
const GUID GUID_NULL                     = {0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/*
    Needs IKsClock, IKsNotifyEvent
*/

class CKsProxy : public IBaseFilter,
                 public IAMovieSetup,
                 public IPersistPropertyBag,
                 public IKsObject
/*
                 public IPersistStream,
                 public ISpecifyPropertyPages,
                 public IReferenceClock,
                 public IMediaSeeking,
                 public IKsObject,
                 public IKsPropertySet,
                 public IKsClockPropertySet,
                 public IAMFilterMiscFlags,
                 public IKsControl,
                 public IKsTopology,
                 public IKsAggregateControl,
                 public IAMDeviceRemoval
*/
{
public:
    typedef std::vector<IUnknown *>ProxyPluginVector;

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

    //IAMovieSetup methods
    HRESULT STDMETHODCALLTYPE Register( void);
    HRESULT STDMETHODCALLTYPE Unregister( void);

    // IPersistPropertyBag methods
    HRESULT STDMETHODCALLTYPE InitNew( void);
    HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog);
    HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // IKsObject
    HANDLE STDMETHODCALLTYPE KsGetObjectHandle();

    CKsProxy() : m_Ref(0), m_pGraph(0), m_ReferenceClock(0), m_FilterState(State_Stopped), m_hDevice(0), m_Plugins(0) {};
    virtual ~CKsProxy()
    {
        if (m_hDevice)
            CloseHandle(m_hDevice);
    };

    HRESULT STDMETHODCALLTYPE GetSupportedSets(LPGUID * pOutGuid, PULONG NumGuids);
    HRESULT STDMETHODCALLTYPE LoadProxyPlugins(LPGUID pGuids, ULONG NumGuids);

protected:
    LONG m_Ref;
    IFilterGraph *m_pGraph;
    IReferenceClock * m_ReferenceClock;
    FILTER_STATE m_FilterState;
    HANDLE m_hDevice;
    ProxyPluginVector m_Plugins;
};

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
    if (IsEqualGUID(refiid, IID_IKsObject))
    {
        *Output = (IKsObject*)(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CKsProxy::QueryInterface: NoInterface for %s !!!\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IKsObject interface
//

HANDLE
STDMETHODCALLTYPE
CKsProxy::KsGetObjectHandle()
{
    return m_hDevice;
}

//-------------------------------------------------------------------
// IPersistPropertyBag interface
//
HRESULT
STDMETHODCALLTYPE
CKsProxy::InitNew( void)
{
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
CKsProxy::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    HRESULT hr;
    WCHAR Buffer[100];
    VARIANT varName;
    LPGUID pGuid;
    ULONG NumGuids = 0;

    // read device path
    varName.vt = VT_BSTR;
    hr = pPropBag->Read(L"DevicePath", &varName, pErrorLog);

    if (FAILED(hr))
    {
        swprintf(Buffer, L"CKsProxy::Load Read %lx\n", hr);
        OutputDebugStringW(Buffer);
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    // open device
    m_hDevice = CreateFileW(varName.bstrVal, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    if (m_hDevice == INVALID_HANDLE_VALUE)
    {
        // failed to open device
        swprintf(Buffer, L"CKsProxy:: failed to open device with %lx\n", GetLastError());
        OutputDebugStringW(Buffer);

        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

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

    CloseHandle(m_hDevice);
    m_hDevice = NULL;


    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
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
    OutputDebugStringW(L"CKsProxy::GetClassID : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Stop()
{
    OutputDebugStringW(L"CKsProxy::Stop : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Pause()
{
    OutputDebugStringW(L"CKsProxy::Pause : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugStringW(L"CKsProxy::Run : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetState(
    DWORD dwMilliSecsTimeout,
    FILTER_STATE *State)
{
    *State = m_FilterState;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::SetSyncSource(
    IReferenceClock *pClock)
{
    if (pClock)
    {
        pClock->AddRef();
    }

    if (m_ReferenceClock)
    {
        m_ReferenceClock->Release();
    }

    m_ReferenceClock = pClock;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::GetSyncSource(
    IReferenceClock **pClock)
{
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
    OutputDebugStringW(L"CKsProxy::EnumPins : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::FindPin(
    LPCWSTR Id, IPin **ppPin)
{
    OutputDebugStringW(L"CKsProxy::FindPin : NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryFilterInfo(
    FILTER_INFO *pInfo)
{
    if (!pInfo)
        return E_POINTER;

    pInfo->achName[0] = L'\0';
    pInfo->pGraph = m_pGraph;

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::JoinFilterGraph(
    IFilterGraph *pGraph,
    LPCWSTR pName)
{
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

    OutputDebugStringW(L"CKsProxy::JoinFilterGraph\n");
    return S_OK;
}


HRESULT
STDMETHODCALLTYPE
CKsProxy::QueryVendorInfo(
    LPWSTR *pVendorInfo)
{
    OutputDebugStringW(L"CKsProxy::QueryVendorInfo : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IAMovieSetup interface
//

HRESULT
STDMETHODCALLTYPE
CKsProxy::Register()
{
    OutputDebugStringW(L"CKsProxy::Register : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsProxy::Unregister()
{
    OutputDebugStringW(L"CKsProxy::Unregister : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CKsProxy_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    WCHAR Buffer[100];
    LPOLESTR pstr;
    StringFromCLSID(riid, &pstr);
    swprintf(Buffer, L"CKsProxy_Constructor pUnkOuter %p riid %s\n", pUnkOuter, pstr);
    OutputDebugStringW(Buffer);

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
