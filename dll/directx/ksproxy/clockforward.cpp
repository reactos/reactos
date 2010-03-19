/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/clockforward.cpp
 * PURPOSE:         IKsClockForwarder interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

#ifndef _MSC_VER
const GUID KSCATEGORY_CLOCK       = {0x53172480, 0x4791, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
#endif

class CKsClockForwarder : public IDistributorNotify,
                          public IKsObject
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

    // IKsObject interface
    HANDLE STDMETHODCALLTYPE KsGetObjectHandle();

    CKsClockForwarder(HANDLE handle) : m_Ref(0), m_Handle(handle){}
    virtual ~CKsClockForwarder(){ if (m_Handle) CloseHandle(m_Handle);}

protected:
    LONG m_Ref;
    HANDLE m_Handle;
};

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IKsObject))
    {
        *Output = (IKsObject*)(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IDistributorNotify))
    {
        *Output = (IDistributorNotify*)(this);
        reinterpret_cast<IDistributorNotify*>(*Output)->AddRef();
        return NOERROR;
    }

#if 0
    if (IsEqualGUID(refiid, IID_IKsClockForwarder))
    {
        *Output = PVOID(this);
        reinterpret_cast<IKsObject*>(*Output)->AddRef();
        return NOERROR;
    }
#endif

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDistributorNotify interface
//


HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Stop()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Pause()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::SetSyncSource(
    IReferenceClock *pClock)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::NotifyGraphChange()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IKsObject interface
//

HANDLE
STDMETHODCALLTYPE
CKsClockForwarder::KsGetObjectHandle()
{
    return m_Handle;
}

HRESULT
WINAPI
CKsClockForwarder_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    HANDLE handle;

    OutputDebugStringW(L"CKsClockForwarder_Constructor\n");

    // open default clock
    hr = KsOpenDefaultDevice(KSCATEGORY_CLOCK, GENERIC_READ | GENERIC_WRITE, &handle);

    if (hr != NOERROR)
    {
         OutputDebugString("CKsClockForwarder_Constructor failed to open device\n");
         return hr;
    }

    CKsClockForwarder * clock = new CKsClockForwarder(handle);

    if (!clock)
    {
        // free clock handle
        CloseHandle(handle);
        return E_OUTOFMEMORY;
    }

    if (FAILED(clock->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete clock;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
