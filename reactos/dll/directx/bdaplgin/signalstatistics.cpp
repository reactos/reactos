/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/frequencyfilter.cpp
 * PURPOSE:         IBDA_FrequencyFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_SignalStatistics = {0x1347d106, 0xcf3a, 0x428a, {0xa5, 0xcb, 0xac, 0x0d, 0x9a, 0x2a, 0x43, 0x38}};

class CBDASignalStatistics : public IBDA_SignalStatistics
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

    // IBDA_SignalStatistics methods
    HRESULT STDMETHODCALLTYPE put_SignalStrength(LONG lDbStrength);
    HRESULT STDMETHODCALLTYPE get_SignalStrength(LONG *plDbStrength);
    HRESULT STDMETHODCALLTYPE put_SignalQuality(LONG lPercentQuality);
    HRESULT STDMETHODCALLTYPE get_SignalQuality(LONG *plPercentQuality);
    HRESULT STDMETHODCALLTYPE put_SignalPresent(BOOLEAN fPresent);
    HRESULT STDMETHODCALLTYPE get_SignalPresent(BOOLEAN *pfPresent);
    HRESULT STDMETHODCALLTYPE put_SignalLocked(BOOLEAN fLocked);
    HRESULT STDMETHODCALLTYPE get_SignalLocked(BOOLEAN *pfLocked);
    HRESULT STDMETHODCALLTYPE put_SampleTime(LONG lmsSampleTime);
    HRESULT STDMETHODCALLTYPE get_SampleTime(LONG *plmsSampleTime);

    CBDASignalStatistics(HANDLE hFile) : m_Ref(0), m_hFile(hFile){};
    ~CBDASignalStatistics(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
};

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;

    *Output = NULL;

    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_SignalStatistics))
    {
        *Output = (IBDA_SignalStatistics*)(this);
        reinterpret_cast<IBDA_SignalStatistics*>(*Output)->AddRef();
        return NOERROR;
    }

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDASignalStatistics::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalStrength(LONG lDbStrength)
{
    OutputDebugStringW(L"CBDASignalStatistics::put_SignalStrength NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalStrength(LONG *plDbStrength)
{
    OutputDebugStringW(L"CBDASignalStatistics::get_SignalStrength NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalQuality(LONG lPercentQuality)
{
    OutputDebugStringW(L"CBDASignalStatistics::put_SignalQuality NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalQuality(LONG *plPercentQuality)
{
    OutputDebugStringW(L"CBDASignalStatistics::get_SignalQuality NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalPresent(BOOLEAN fPresent)
{
    OutputDebugStringW(L"CBDASignalStatistics::put_SignalPresent NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalPresent(BOOLEAN *pfPresent)
{
    OutputDebugStringW(L"CBDASignalStatistics::get_SignalPresent NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SignalLocked(BOOLEAN fLocked)
{
    OutputDebugStringW(L"CBDASignalStatistics::put_SignalLocked NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SignalLocked(BOOLEAN *pfLocked)
{
    OutputDebugStringW(L"CBDASignalStatistics::get_SignalLocked NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::put_SampleTime(LONG lmsSampleTime)
{
    OutputDebugStringW(L"CBDASignalStatistics::put_SampleTime NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDASignalStatistics::get_SampleTime(LONG *plmsSampleTime)
{
    OutputDebugStringW(L"CBDASignalStatistics::get_SampleTime NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CBDASignalStatistics_fnConstructor(
    HANDLE hFile,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDASignalStatistics * handler = new CBDASignalStatistics(hFile);

    OutputDebugStringW(L"CBDASignalStatistics_fnConstructor\n");

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
