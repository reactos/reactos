/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/basicaudio.cpp
 * PURPOSE:         IBasicAudio interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IBasicAudio = {0x56a868b3, 0x0ad4, 0x11ce, {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};

class CKsBasicAudio : public IBasicAudio,
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

    // IDistributorNotify methods
    HRESULT STDMETHODCALLTYPE Stop();
    HRESULT STDMETHODCALLTYPE Pause();
    HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
    HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock);
    HRESULT STDMETHODCALLTYPE NotifyGraphChange();

    // IDispatch methods
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);


    // IBasicAudio methods
    HRESULT STDMETHODCALLTYPE put_Volume(long lVolume);
    HRESULT STDMETHODCALLTYPE get_Volume(long *plVolume);
    HRESULT STDMETHODCALLTYPE put_Balance(long lBalance);
    HRESULT STDMETHODCALLTYPE get_Balance(long *plBalance);


    CKsBasicAudio() : m_Ref(0){}
    virtual ~CKsBasicAudio(){}

protected:
    LONG m_Ref;
};

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBasicAudio))
    {
        *Output = (IBasicAudio*)(this);
        reinterpret_cast<IBasicAudio*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDistributorNotify interface
//


HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::Stop()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::Pause()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::Run(
    REFERENCE_TIME tStart)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::SetSyncSource(
    IReferenceClock *pClock)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::NotifyGraphChange()
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IDispatch interface
//

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::GetTypeInfoCount(
    UINT *pctinfo)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::GetTypeInfo(
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IBasicAudio interface
//

HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::put_Volume(
    long lVolume)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::get_Volume(
    long *plVolume)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::put_Balance(
    long lBalance)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CKsBasicAudio::get_Balance(
    long *plBalance)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CKsBasicAudio_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    CKsBasicAudio * handler = new CKsBasicAudio();

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
