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

const GUID IID_IKsClockForwarder              = {0x877e4352, 0x6fea, 0x11d0, {0xb8, 0x63, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};

DWORD WINAPI CKsClockForwarder_ThreadStartup(LPVOID lpParameter);

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

    CKsClockForwarder(HANDLE handle);
    virtual ~CKsClockForwarder(){};
    HRESULT STDMETHODCALLTYPE SetClockState(KSSTATE State);
protected:
    LONG m_Ref;
    HANDLE m_Handle;
    IReferenceClock * m_Clock;
    HANDLE m_hEvent;
    HANDLE m_hThread;
    BOOL m_ThreadStarted;
    BOOL m_PendingStop;
    BOOL m_ForceStart;
    KSSTATE m_State;
    REFERENCE_TIME m_Time;

    friend DWORD WINAPI CKsClockForwarder_ThreadStartup(LPVOID lpParameter);
};

CKsClockForwarder::CKsClockForwarder(
    HANDLE handle) : m_Ref(0),
                     m_Handle(handle),
                     m_Clock(0),
                     m_hEvent(NULL),
                     m_hThread(NULL),
                     m_ThreadStarted(FALSE),
                     m_PendingStop(FALSE),
                     m_ForceStart(FALSE),
                     m_State(KSSTATE_STOP),
                     m_Time(0)
{
}

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
    if (IsEqualGUID(refiid, IID_IKsObject) ||
        IsEqualGUID(refiid, IID_IKsClockForwarder))
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

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDistributorNotify interface
//


HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Stop()
{
    WCHAR Buffer[200];

    swprintf(Buffer, L"CKsClockForwarder::Stop m_ThreadStarted %u m_PendingStop %u m_hThread %p m_hEvent %p m_Handle %p\n", m_ThreadStarted, m_PendingStop, m_hThread, m_hEvent, m_Handle);
    OutputDebugStringW(Buffer);

    m_Time = 0;
    if (m_ThreadStarted)
    {
        // signal pending stop
        m_PendingStop = true;

        assert(m_hThread);
        assert(m_hEvent);

        // set stop event
        SetEvent(m_hEvent);

        // wait untill the thread has finished
        WaitForSingleObject(m_hThread, INFINITE);

        // close thread handle
        CloseHandle(m_hThread);

        // zero handle
        m_hThread = NULL;
    }

    if (m_hEvent)
    {
        // close stop event
        CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }

    m_PendingStop = false;

    SetClockState(KSSTATE_STOP);
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Pause()
{
    OutputDebugString("CKsClockForwarder::Pause\n");

    if (!m_hEvent)
    {
        m_hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!m_hEvent)
            return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    if (m_State <= KSSTATE_PAUSE)
    {
        if (m_State == KSSTATE_STOP)
            SetClockState(KSSTATE_ACQUIRE);

        if (m_State == KSSTATE_ACQUIRE)
            SetClockState(KSSTATE_PAUSE);
    }
    else
    {
        if (!m_ForceStart)
        {
            SetClockState(KSSTATE_PAUSE);
        }
    }

    if (!m_hThread)
    {
        m_hThread = CreateThread(NULL, 0, CKsClockForwarder_ThreadStartup, (LPVOID)this, 0, NULL);
        if (!m_hThread)
            return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugString("CKsClockForwarder::Run\n");

    m_Time = tStart;

    if (!m_hEvent || !m_hThread)
    {
        m_ForceStart = TRUE;
        HRESULT hr = Pause();
        m_ForceStart = FALSE;

        if (FAILED(hr))
            return hr;
    }

    assert(m_hThread);

    SetClockState(KSSTATE_RUN);
    SetEvent(m_hEvent);

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::SetSyncSource(
    IReferenceClock *pClock)
{
    OutputDebugString("CKsClockForwarder::SetSyncSource\n");

    if (pClock)
        pClock->AddRef();

    if (m_Clock)
        m_Clock->Release();


    m_Clock = pClock;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::NotifyGraphChange()
{
    OutputDebugString("CKsClockForwarder::NotifyGraphChange\n");
    return NOERROR;
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

//-------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
CKsClockForwarder::SetClockState(KSSTATE State)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    HRESULT hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), &State, sizeof(KSSTATE), &BytesReturned);
    if (SUCCEEDED(hr))
        m_State = State;

    WCHAR Buffer[100];
    swprintf(Buffer, L"CKsClockForwarder::SetClockState m_State %u State %u hr %lx\n", m_State, State, hr);
    OutputDebugStringW(Buffer);

    return hr;
}

DWORD
WINAPI
CKsClockForwarder_ThreadStartup(LPVOID lpParameter)
{
    REFERENCE_TIME Time;
    ULONG BytesReturned;

    CKsClockForwarder * Fwd = (CKsClockForwarder*)lpParameter;

    Fwd->m_ThreadStarted = TRUE;

    do
    {
        if (Fwd->m_PendingStop)
            break;

        if (Fwd->m_State != KSSTATE_RUN)
            WaitForSingleObject(Fwd->m_hEvent, INFINITE);

        KSPROPERTY Property;
        Property.Set = KSPROPSETID_Clock;
        Property.Id = KSPROPERTY_CLOCK_TIME;
        Property.Flags = KSPROPERTY_TYPE_SET;

        Fwd->m_Clock->GetTime(&Time);
        Time -= Fwd->m_Time;

        KsSynchronousDeviceControl(Fwd->m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), &Time, sizeof(REFERENCE_TIME), &BytesReturned);
    }
    while(TRUE);

    Fwd->m_ThreadStarted = FALSE;
    return NOERROR;
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
