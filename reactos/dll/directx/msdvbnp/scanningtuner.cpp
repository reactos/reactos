/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/networkprovider.cpp
 * PURPOSE:         IScanningTunner interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CScanningTunner : public IScanningTuner
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

    //ITuner methods
    HRESULT STDMETHODCALLTYPE get_TuningSpace(ITuningSpace **TuningSpace);
    HRESULT STDMETHODCALLTYPE put_TuningSpace(ITuningSpace *TuningSpace);
    HRESULT STDMETHODCALLTYPE EnumTuningSpaces(IEnumTuningSpaces **ppEnum);
    HRESULT STDMETHODCALLTYPE get_TuneRequest(ITuneRequest **TuneRequest);
    HRESULT STDMETHODCALLTYPE put_TuneRequest(ITuneRequest *TuneRequest);
    HRESULT STDMETHODCALLTYPE Validate(ITuneRequest *TuneRequest);
    HRESULT STDMETHODCALLTYPE get_PreferredComponentTypes(IComponentTypes **ComponentTypes);
    HRESULT STDMETHODCALLTYPE put_PreferredComponentTypes(IComponentTypes *ComponentTypes);
    HRESULT STDMETHODCALLTYPE get_SignalStrength(long *Strength);
    HRESULT STDMETHODCALLTYPE TriggerSignalEvents(long Interval);

    //IScanningTuner methods
    HRESULT STDMETHODCALLTYPE SeekUp();
    HRESULT STDMETHODCALLTYPE SeekDown();
    HRESULT STDMETHODCALLTYPE ScanUp(long MillisecondsPause);
    HRESULT STDMETHODCALLTYPE ScanDown(long MillisecondsPause);
    HRESULT STDMETHODCALLTYPE AutoProgram();

    CScanningTunner() : m_Ref(0), m_TuningSpace(0){};
    virtual ~CScanningTunner(){};

protected:
    LONG m_Ref;
    ITuningSpace * m_TuningSpace;
};

HRESULT
STDMETHODCALLTYPE
CScanningTunner::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_ITuner))
    {
        *Output = (ITuner*)(this);
        reinterpret_cast<ITuner*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IScanningTuner))
    {
        *Output = (IScanningTuner*)(this);
        reinterpret_cast<IScanningTuner*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CScanningTunner::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
//ITuner
//
HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_TuningSpace(
    ITuningSpace **TuningSpace)
{
    OutputDebugStringW(L"CScanningTunner::get_TuningSpace\n");

    *TuningSpace = m_TuningSpace;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_TuningSpace(
    ITuningSpace *TuningSpace)
{
    OutputDebugStringW(L"CScanningTunner::put_TuningSpace\n");
    m_TuningSpace = TuningSpace;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::EnumTuningSpaces(
    IEnumTuningSpaces **ppEnum)
{
    OutputDebugStringW(L"CScanningTunner::EnumTuningSpaces : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_TuneRequest(
    ITuneRequest **TuneRequest)
{
    OutputDebugStringW(L"CScanningTunner::get_TuneRequest : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_TuneRequest(
    ITuneRequest *TuneRequest)
{
    OutputDebugStringW(L"CScanningTunner::put_TuneRequest : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::Validate(
    ITuneRequest *TuneRequest)
{
    OutputDebugStringW(L"CScanningTunner::Validate : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_PreferredComponentTypes(
    IComponentTypes **ComponentTypes)
{
    OutputDebugStringW(L"CScanningTunner::get_PreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_PreferredComponentTypes(
    IComponentTypes *ComponentTypes)
{
    OutputDebugStringW(L"CScanningTunner::put_PreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_SignalStrength(
    long *Strength)
{
    OutputDebugStringW(L"CScanningTunner::get_SignalStrength : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::TriggerSignalEvents(
    long Interval)
{
    OutputDebugStringW(L"CScanningTunner::TriggerSignalEvents : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
//IScanningTuner
HRESULT
STDMETHODCALLTYPE
CScanningTunner::SeekUp()
{
    OutputDebugStringW(L"CScanningTunner::SeekUp : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::SeekDown()
{
    OutputDebugStringW(L"CScanningTunner::SeekDown : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::ScanUp(
    long MillisecondsPause)
{
    OutputDebugStringW(L"CScanningTunner::ScanUp : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::ScanDown(
    long MillisecondsPause)
{
    OutputDebugStringW(L"CScanningTunner::ScanDown : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::AutoProgram()
{
    OutputDebugStringW(L"CScanningTunner::AutoProgram : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CScanningTunner_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    CScanningTunner * handler = new CScanningTunner();

#ifdef MSDVBNP_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CScanningTunner_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
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