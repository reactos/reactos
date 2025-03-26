/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/networkprovider.cpp
 * PURPOSE:         IBDA_NetworkProvider interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

#define DEVICE_FILTER_MASK (0x80000000)

class CNetworkProvider : public IBaseFilter,
                         public IAMovieSetup,
                         public IBDA_NetworkProvider
{
public:
    typedef std::vector<IUnknown*>DeviceFilterStack;

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

    //IBDA_NetworkProvider methods
    HRESULT STDMETHODCALLTYPE PutSignalSource(ULONG ulSignalSource);
    HRESULT STDMETHODCALLTYPE GetSignalSource(ULONG *pulSignalSource);
    HRESULT STDMETHODCALLTYPE GetNetworkType(GUID *pguidNetworkType);
    HRESULT STDMETHODCALLTYPE PutTuningSpace(REFGUID guidTuningSpace);
    HRESULT STDMETHODCALLTYPE GetTuningSpace(GUID *pguidTuingSpace);
    HRESULT STDMETHODCALLTYPE RegisterDeviceFilter(IUnknown *pUnkFilterControl, ULONG *ppvRegistrationContext);
    HRESULT STDMETHODCALLTYPE UnRegisterDeviceFilter(ULONG pvRegistrationContext);

    CNetworkProvider(LPCGUID ClassID);
    virtual ~CNetworkProvider(){};

protected:
    LONG m_Ref;
    IFilterGraph *m_pGraph;
    IReferenceClock * m_ReferenceClock;
    FILTER_STATE m_FilterState;
    IPin * m_Pins[1];
    GUID m_ClassID;
    DeviceFilterStack m_DeviceFilters;
    IScanningTuner * m_Tuner;
    IBDA_IPV6Filter * m_IPV6Filter;
    IBDA_IPV4Filter * m_IPV4Filter;
    IBDA_EthernetFilter * m_EthernetFilter;
};

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    ULONG Index;
    HRESULT hr;

    *Output = NULL;

    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IBaseFilter))
    {
        *Output = (IBaseFilter*)(this);
        reinterpret_cast<IBaseFilter*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_ITuner) ||
        IsEqualGUID(refiid, IID_IScanningTuner))
    {
        // construct scanning tuner
        if (!m_Tuner)
        {
            HRESULT hr = CScanningTunner_fnConstructor(m_DeviceFilters, refiid, (void**)&m_Tuner);
            if (FAILED(hr))
                return hr;
        }
        m_Tuner->AddRef();
        *Output = (IUnknown*)m_Tuner;

        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_IPV6Filter))
    {
        // construct scanning tuner
        if (!m_IPV6Filter)
        {
            HRESULT hr = CIPV6Filter_fnConstructor((IBDA_NetworkProvider*)this, refiid, (void**)&m_IPV6Filter);
            if (FAILED(hr))
                return hr;
        }
        m_IPV6Filter->AddRef();
        *Output = (IUnknown*)m_IPV6Filter;

        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_IPV4Filter))
    {
        // construct scanning tuner
        if (!m_IPV4Filter)
        {
            HRESULT hr = CIPV4Filter_fnConstructor((IBDA_NetworkProvider*)this, refiid, (void**)&m_IPV4Filter);
            if (FAILED(hr))
                return hr;
        }
        m_IPV4Filter->AddRef();
        *Output = (IUnknown*)m_IPV4Filter;

        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_EthernetFilter))
    {
        // construct scanning tuner
        if (!m_EthernetFilter)
        {
            HRESULT hr = CIPV4Filter_fnConstructor((IBDA_NetworkProvider*)this, refiid, (void**)&m_EthernetFilter);
            if (FAILED(hr))
                return hr;
        }
        m_EthernetFilter->AddRef();
        *Output = (IUnknown*)m_EthernetFilter;

        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_NetworkProvider))
    {
        *Output = (IBDA_NetworkProvider*)(this);
        reinterpret_cast<IBDA_NetworkProvider*>(*Output)->AddRef();
        return NOERROR;
    }

    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get device filter
        IUnknown *pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        // query for requested interface
        hr =  pFilter->QueryInterface(refiid, Output);
        if (SUCCEEDED(hr))
        {
#ifdef MSDVBNP_TRACE
            WCHAR Buffer[MAX_PATH];
            LPOLESTR lpstr;
            StringFromCLSID(refiid, &lpstr);
            swprintf(Buffer, L"CNetworkProvider::QueryInterface: DeviceFilter %lu supports %s !!!\n", Index, lpstr);
            OutputDebugStringW(Buffer);
            CoTaskMemFree(lpstr);
#endif
            return hr;
        }
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CNetworkProvider::QueryInterface: NoInterface for %s !!!\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

CNetworkProvider::CNetworkProvider(LPCGUID ClassID) : m_Ref(0),
                                                      m_pGraph(0),
                                                      m_ReferenceClock(0),
                                                      m_FilterState(State_Stopped),
                                                      m_DeviceFilters(),
                                                      m_Tuner(0),
                                                      m_IPV6Filter(0),
                                                      m_IPV4Filter(0),
                                                      m_EthernetFilter(0)
{
    m_Pins[0] = 0;

    CopyMemory(&m_ClassID, ClassID, sizeof(GUID));
};

//-------------------------------------------------------------------
// IBaseFilter interface
//

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetClassID(
    CLSID *pClassID)
{
    OutputDebugStringW(L"CNetworkProvider::GetClassID\n");
    CopyMemory(&pClassID, &m_ClassID, sizeof(GUID));

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Stop()
{
    OutputDebugStringW(L"CNetworkProvider::Stop\n");
    m_FilterState = State_Stopped;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Pause()
{
    OutputDebugStringW(L"CNetworkProvider::Pause\n");

    m_FilterState = State_Paused;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugStringW(L"CNetworkProvider::Run\n");

    m_FilterState = State_Running;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetState(
    DWORD dwMilliSecsTimeout,
    FILTER_STATE *State)
{
    *State = m_FilterState;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::SetSyncSource(
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
CNetworkProvider::GetSyncSource(
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
CNetworkProvider::EnumPins(
    IEnumPins **ppEnum)
{
    if (m_Pins[0] == 0)
    {
        HRESULT hr = CPin_fnConstructor(NULL, (IBaseFilter*)this, IID_IUnknown, (void**)&m_Pins[0]);
        if (FAILED(hr))
            return hr;
    }

    return CEnumPins_fnConstructor(NULL, 1, m_Pins, IID_IEnumPins, (void**)ppEnum);
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::FindPin(
    LPCWSTR Id, IPin **ppPin)
{
    OutputDebugStringW(L"CNetworkProvider::FindPin : NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CNetworkProvider::QueryFilterInfo(
    FILTER_INFO *pInfo)
{
    if (!pInfo)
        return E_POINTER;

    pInfo->achName[0] = L'\0';
    pInfo->pGraph = m_pGraph;

    if (m_pGraph)
        m_pGraph->AddRef();

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::JoinFilterGraph(
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

    OutputDebugStringW(L"CNetworkProvider::JoinFilterGraph\n");
    return S_OK;
}


HRESULT
STDMETHODCALLTYPE
CNetworkProvider::QueryVendorInfo(
    LPWSTR *pVendorInfo)
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IAMovieSetup interface
//

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Register()
{
    OutputDebugStringW(L"CNetworkProvider::Register : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Unregister()
{
    OutputDebugStringW(L"CNetworkProvider::Unregister : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IBDA_NetworkProvider interface
//

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::PutSignalSource(
    ULONG ulSignalSource)
{
    OutputDebugStringW(L"CNetworkProvider::PutSignalSource : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetSignalSource(
    ULONG *pulSignalSource)
{
    OutputDebugStringW(L"CNetworkProvider::GetSignalSource : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetNetworkType(
    GUID *pguidNetworkType)
{
    OutputDebugStringW(L"CNetworkProvider::GetNetworkType : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::PutTuningSpace(
    REFGUID guidTuningSpace)
{
    OutputDebugStringW(L"CNetworkProvider::PutTuningSpace : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetTuningSpace(
    GUID *pguidTuingSpace)
{
    OutputDebugStringW(L"CNetworkProvider::GetTuningSpace : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::RegisterDeviceFilter(
    IUnknown *pUnkFilterControl,
    ULONG *ppvRegistrationContext)
{
    HRESULT hr;
    IBDA_DeviceControl * pDeviceControl = NULL;
    IBDA_Topology *pTopology = NULL;

    OutputDebugStringW(L"CNetworkProvider::RegisterDeviceFilter\n");

    if (!pUnkFilterControl || !ppvRegistrationContext)
    {
        //invalid argument
        return E_POINTER;
    }

    // the filter must support IBDA_DeviceControl and IBDA_Topology
    hr = pUnkFilterControl->QueryInterface(IID_IBDA_DeviceControl, (void**)&pDeviceControl);
    if (FAILED(hr))
    {
        OutputDebugStringW(L"CNetworkProvider::RegisterDeviceFilter Filter does not support IBDA_DeviceControl\n");
        return hr;
    }

    hr = pUnkFilterControl->QueryInterface(IID_IBDA_Topology, (void**)&pTopology);
    if (FAILED(hr))
    {
        pDeviceControl->Release();
        OutputDebugStringW(L"CNetworkProvider::RegisterDeviceFilter Filter does not support IID_IBDA_Topology\n");
        return hr;
    }

    //TODO
    // analyize device filter

    // increment reference
    pUnkFilterControl->AddRef();

    // release IBDA_DeviceControl interface
    pDeviceControl->Release();

    // release IBDA_Topology interface
    pTopology->Release();

    // store registration ctx
    *ppvRegistrationContext = (m_DeviceFilters.size() | DEVICE_FILTER_MASK);

    // store filter
    m_DeviceFilters.push_back(pUnkFilterControl);

    OutputDebugStringW(L"CNetworkProvider::RegisterDeviceFilter complete\n");

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::UnRegisterDeviceFilter(ULONG pvRegistrationContext)
{
    ULONG Index;
    IUnknown * pUnknown;

    OutputDebugStringW(L"CNetworkProvider::UnRegisterDeviceFilter\n");

    if (!(pvRegistrationContext & DEVICE_FILTER_MASK))
    {
        // invalid argument
        return E_INVALIDARG;
    }

    // get real index
    Index = pvRegistrationContext & ~DEVICE_FILTER_MASK;

    if (Index >= m_DeviceFilters.size())
    {
        // invalid argument
        return E_INVALIDARG;
    }

    pUnknown = m_DeviceFilters[Index];
    if (!pUnknown)
    {
        // filter was already de-registered
        return E_INVALIDARG;
    }

    // remove from vector
    m_DeviceFilters[Index] = NULL;

    // release extra reference
    pUnknown->Release();

    return NOERROR;
}

HRESULT
WINAPI
CNetworkProvider_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    CNetworkProvider * handler = new CNetworkProvider(&CLSID_DVBTNetworkProvider);

#ifdef MSDVBNP_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CNetworkProvider_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
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
