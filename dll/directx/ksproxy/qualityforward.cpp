/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/clockforward.cpp
 * PURPOSE:         IKsClockForwarder interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID KSCATEGORY_QUALITY = {0x97EBAACB, 0x95BD, 0x11D0, {0xA3, 0xEA, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

class CKsQualityForwarder : public IKsQualityForwarder
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

    // IKsObject interface
    HANDLE STDMETHODCALLTYPE KsGetObjectHandle();

    // IKsQualityForwarder
    VOID STDMETHODCALLTYPE KsFlushClient(IN IKsPin  *Pin); 

    CKsQualityForwarder(HANDLE handle) : m_Ref(0), m_Handle(handle){}
    virtual ~CKsQualityForwarder(){ if (m_Handle) CloseHandle(m_Handle);}

protected:
    LONG m_Ref;
    HANDLE m_Handle;


};

HRESULT
STDMETHODCALLTYPE
CKsQualityForwarder::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsQualityForwarder))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IKsObject interface
//

HANDLE
STDMETHODCALLTYPE
CKsQualityForwarder::KsGetObjectHandle()
{
    return m_Handle;
}

//-------------------------------------------------------------------
// IKsQualityForwarder interface
//
VOID
STDMETHODCALLTYPE
CKsQualityForwarder::KsFlushClient(
    IN IKsPin  *Pin)
{
    OutputDebugString("UNIMPLEMENTED\n");
}



HRESULT
WINAPI
CKsQualityForwarder_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    HANDLE handle;

    // open default clock
    hr = KsOpenDefaultDevice(KSCATEGORY_QUALITY, GENERIC_READ | GENERIC_WRITE, &handle);

    if (hr != NOERROR)
    {
         OutputDebugString("CKsClockForwarder_Constructor failed to open device\n");
         return hr;
    }

    CKsQualityForwarder * quality = new CKsQualityForwarder(handle);

    if (!quality)
    {
        // free clock handle
        CloseHandle(handle);
        return E_OUTOFMEMORY;
    }

    if (FAILED(quality->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete quality;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
