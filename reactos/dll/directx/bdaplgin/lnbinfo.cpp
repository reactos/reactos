/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/lnbinfo.cpp
 * PURPOSE:         IBDA_LNBInfo interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_LNBInfo = {0x992cf102, 0x49f9, 0x4719, {0xa6, 0x64,  0xc4, 0xf2, 0x3e, 0x24, 0x08, 0xf4}};

class CBDALNBInfo : public IBDA_LNBInfo
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

    //IBDA_LNBInfo methods
    HRESULT STDMETHODCALLTYPE put_LocalOscilatorFrequencyLowBand(ULONG ulLOFLow);
    HRESULT STDMETHODCALLTYPE get_LocalOscilatorFrequencyLowBand(ULONG *pulLOFLow);
    HRESULT STDMETHODCALLTYPE put_LocalOscilatorFrequencyHighBand(ULONG ulLOFHigh);
    HRESULT STDMETHODCALLTYPE get_LocalOscilatorFrequencyHighBand(ULONG *pulLOFHigh);
    HRESULT STDMETHODCALLTYPE put_HighLowSwitchFrequency(ULONG ulSwitchFrequency);
    HRESULT STDMETHODCALLTYPE get_HighLowSwitchFrequency(ULONG *pulSwitchFrequency);

    CBDALNBInfo(HANDLE hFile) : m_Ref(0), m_hFile(hFile){};
    ~CBDALNBInfo(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
};

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBDA_LNBInfo))
    {
        *Output = (IBDA_LNBInfo*)(this);
        reinterpret_cast<IBDA_LNBInfo*>(*Output)->AddRef();
        return NOERROR;
    }

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDALNBInfo::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_LocalOscilatorFrequencyLowBand(ULONG ulLOFLow)
{
    OutputDebugStringW(L"CBDALNBInfo::put_LocalOscilatorFrequencyLowBand NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscilatorFrequencyLowBand(ULONG *pulLOFLow)
{
    OutputDebugStringW(L"CBDALNBInfo::get_LocalOscilatorFrequencyLowBand NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_LocalOscilatorFrequencyHighBand(ULONG ulLOFHigh)
{
    OutputDebugStringW(L"CBDALNBInfo::put_LocalOscilatorFrequencyHighBand NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_LocalOscilatorFrequencyHighBand(ULONG *pulLOFHigh)
{
    OutputDebugStringW(L"CBDALNBInfo::get_LocalOscilatorFrequencyHighBand NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::put_HighLowSwitchFrequency(ULONG ulSwitchFrequency)
{
    OutputDebugStringW(L"CBDALNBInfo::put_HighLowSwitchFrequency NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDALNBInfo::get_HighLowSwitchFrequency(ULONG *pulSwitchFrequency)
{
    OutputDebugStringW(L"CBDALNBInfo::get_HighLowSwitchFrequency NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CBDALNBInfo_fnConstructor(
    HANDLE hFile,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDALNBInfo * handler = new CBDALNBInfo(hFile);

    OutputDebugStringW(L"CBDALNBInfo_fnConstructor\n");

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