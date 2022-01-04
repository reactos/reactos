/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/cvpconfig.cpp
 * PURPOSE:         IVPConfig interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CVPConfig : public IVPConfig,
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
    // IVPConfig
    HRESULT STDMETHODCALLTYPE IsVPDecimationAllowed(LPBOOL pbIsDecimationAllowed);
    HRESULT STDMETHODCALLTYPE SetScalingFactors(LPAMVPSIZE pamvpSize);

    CVPConfig() : m_Ref(0){}
    virtual ~CVPConfig(){}

protected:
    LONG m_Ref;
};


HRESULT
STDMETHODCALLTYPE
CVPConfig::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IVPConfig))
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
CVPConfig::Stop()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::Pause()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::Run(
    REFERENCE_TIME tStart)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::SetSyncSource(
    IReferenceClock *pClock)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::NotifyGraphChange()
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
CVPConfig::GetConnectInfo(
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
CVPConfig::SetConnectInfo(
    DWORD dwChosenEntry)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::GetVPDataInfo(
    LPAMVPDATAINFO pamvpDataInfo)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::GetMaxPixelRate(
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
CVPConfig::InformVPInputFormats(
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
CVPConfig::GetVideoFormats(
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
CVPConfig::SetVideoFormat(
    DWORD dwChosenEntry)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::SetInvertPolarity()
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::GetOverlaySurface(
    LPDIRECTDRAWSURFACE* ppddOverlaySurface)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::SetDirectDrawKernelHandle(
    ULONG_PTR dwDDKernelHandle)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::SetVideoPortID(
    IN DWORD dwVideoPortID)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CVPConfig::SetDDSurfaceKernelHandles(
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
CVPConfig::SetSurfaceParameters(
    DWORD dwPitch,
    IN DWORD dwXOrigin,
    IN DWORD dwYOrigin)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IVPConfig
//

HRESULT
STDMETHODCALLTYPE
CVPConfig::IsVPDecimationAllowed(
    LPBOOL pbIsDecimationAllowed)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CVPConfig::SetScalingFactors(
    LPAMVPSIZE pamvpSize)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"UNIMPLEMENTED\n");
#endif
    return E_NOTIMPL;
}


HRESULT
WINAPI
CVPConfig_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CVPConfig_Constructor\n");
#endif

    CVPConfig * handler = new CVPConfig();

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

