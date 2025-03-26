/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/cvpvbiconfig.cpp
 * PURPOSE:         CVPVBIConfig interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CVPVBIConfig : public IVPVBIConfig,
                     public IDistributorNotify
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

    // IDistributorNotify interface
    HRESULT STDMETHODCALLTYPE Stop();
    HRESULT STDMETHODCALLTYPE Pause();
    HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
    HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
    HRESULT STDMETHODCALLTYPE NotifyGraphChange();

    // IVPBaseConfig
    HRESULT STDMETHODCALLTYPE GetConnectInfo(LPDWORD pdwNumConnectInfo, IN OUT LPDDVIDEOPORTCONNECT pddVPConnectInfo);
    HRESULT STDMETHODCALLTYPE SetConnectInfo(DWORD dwChosenEntry);
    HRESULT STDMETHODCALLTYPE GetVPDataInfo(LPAMVPDATAINFO pamvpDataInfo);
    HRESULT STDMETHODCALLTYPE GetMaxPixelRate(LPAMVPSIZE pamvpSize, OUT LPDWORD pdwMaxPixelsPerSecond);
    HRESULT STDMETHODCALLTYPE InformVPInputFormats(DWORD dwNumFormats, IN LPDDPIXELFORMAT pDDPixelFormats);
    HRESULT STDMETHODCALLTYPE GetVideoFormats(LPDWORD pdwNumFormats, IN OUT LPDDPIXELFORMAT pddPixelFormats);
    HRESULT STDMETHODCALLTYPE SetVideoFormat(DWORD dwChosenEntry);
    HRESULT STDMETHODCALLTYPE SetInvertPolarity();
    HRESULT STDMETHODCALLTYPE GetOverlaySurface(LPDIRECTDRAWSURFACE* ppddOverlaySurface);
    HRESULT STDMETHODCALLTYPE SetDirectDrawKernelHandle(ULONG_PTR dwDDKernelHandle);
    HRESULT STDMETHODCALLTYPE SetVideoPortID(IN DWORD dwVideoPortID);
    HRESULT STDMETHODCALLTYPE SetDDSurfaceKernelHandles(DWORD cHandles, IN ULONG_PTR *rgDDKernelHandles);
    HRESULT STDMETHODCALLTYPE SetSurfaceParameters(DWORD dwPitch, IN DWORD dwXOrigin, IN DWORD dwYOrigin);

    CVPVBIConfig() : m_Ref(0){}
    virtual ~CVPVBIConfig(){}

protected:
    LONG m_Ref;
};

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IDistributorNotify))
    {
        *Output = (IDistributorNotify*)(this);
        reinterpret_cast<IDistributorNotify*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IVPVBIConfig))
    {
        *Output = (IVPConfig*)(this);
        reinterpret_cast<IVPConfig*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDistributorNotify interface
//


HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::Stop()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::Pause()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::Run(
    REFERENCE_TIME tStart)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetSyncSource(
    IReferenceClock *pClock)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::NotifyGraphChange()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IVPBaseConfig
//
HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::GetConnectInfo(
    LPDWORD pdwNumConnectInfo,
    IN OUT LPDDVIDEOPORTCONNECT pddVPConnectInfo)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetConnectInfo(
    DWORD dwChosenEntry)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::GetVPDataInfo(
    LPAMVPDATAINFO pamvpDataInfo)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::GetMaxPixelRate(
    LPAMVPSIZE pamvpSize,
    OUT LPDWORD pdwMaxPixelsPerSecond)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::InformVPInputFormats(
    DWORD dwNumFormats,
    IN LPDDPIXELFORMAT pDDPixelFormats)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::GetVideoFormats(
    LPDWORD pdwNumFormats,
    IN OUT LPDDPIXELFORMAT pddPixelFormats)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetVideoFormat(
    DWORD dwChosenEntry)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetInvertPolarity()
{
 #ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::GetOverlaySurface(
    LPDIRECTDRAWSURFACE* ppddOverlaySurface)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetDirectDrawKernelHandle(
    ULONG_PTR dwDDKernelHandle)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetVideoPortID(
    IN DWORD dwVideoPortID)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetDDSurfaceKernelHandles(
    DWORD cHandles,
    IN ULONG_PTR *rgDDKernelHandles)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CVPVBIConfig::SetSurfaceParameters(
    DWORD dwPitch,
    IN DWORD dwXOrigin,
    IN DWORD dwYOrigin)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}


HRESULT
WINAPI
CVPVBIConfig_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CVPVBIConfig_Constructor\n");
#endif

    CVPVBIConfig * handler = new CVPVBIConfig();

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
