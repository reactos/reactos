/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/networkprovider.cpp
 * PURPOSE:         IBDA_NetworkProvider interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CNetworkProvider : public IBaseFilter,
                         public IAMovieSetup,
                         public IBDA_NetworkProvider
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
    HRESULT STDMETHODCALLTYPE RegisterDeviceFilter(IUnknown *pUnkFilterControl, ULONG *ppvRegisitrationContext);
    HRESULT STDMETHODCALLTYPE UnRegisterDeviceFilter(ULONG pvRegistrationContext);

    CNetworkProvider() : m_Ref(0), m_pGraph(0), m_ReferenceClock(0), m_FilterState(State_Stopped) {m_Pins[0] = 0;};
    virtual ~CNetworkProvider(){};

protected:
    LONG m_Ref;
    IFilterGraph *m_pGraph;
    IReferenceClock * m_ReferenceClock;
    FILTER_STATE m_FilterState;
    IPin * m_Pins[1];
};

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
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
        return CScanningTunner_fnConstructor(NULL, refiid, Output);
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CNetworkProvider::QueryInterface: NoInterface for %s !!!\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IBaseFilter interface
//

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::GetClassID(
    CLSID *pClassID)
{
    OutputDebugStringW(L"CNetworkProvider::GetClassID : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Stop()
{
    OutputDebugStringW(L"CNetworkProvider::Stop : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Pause()
{
    OutputDebugStringW(L"CNetworkProvider::Pause : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugStringW(L"CNetworkProvider::Run : NotImplemented\n");
    return E_NOTIMPL;
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
    OutputDebugStringW(L"CNetworkProvider::QueryVendorInfo : NotImplemented\n");
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
    ULONG *ppvRegisitrationContext)
{
    OutputDebugStringW(L"CNetworkProvider::RegisterDeviceFilter : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CNetworkProvider::UnRegisterDeviceFilter(ULONG pvRegistrationContext)
{
    OutputDebugStringW(L"CNetworkProvider::UnRegisterDeviceFilter : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetworkProvider_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    CNetworkProvider * handler = new CNetworkProvider();

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
