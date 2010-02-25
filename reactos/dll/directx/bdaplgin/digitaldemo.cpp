/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/digitaldemo.cpp
 * PURPOSE:         IBDA_DigitalDemodulator interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_DigitalDemodulator = {0xef30f379, 0x985b, 0x4d10, {0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x04, 0xe1, 0xe0}};

class CBDADigitalDemodulator : public IBDA_DigitalDemodulator
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
    //IBDA_DigitalDemodulator methods
    HRESULT STDMETHODCALLTYPE put_ModulationType(ModulationType *pModulationType);
    HRESULT STDMETHODCALLTYPE get_ModulationType(ModulationType *pModulationType);
    HRESULT STDMETHODCALLTYPE put_InnerFECMethod(FECMethod *pFECMethod);
    HRESULT STDMETHODCALLTYPE get_InnerFECMethod(FECMethod *pFECMethod);
    HRESULT STDMETHODCALLTYPE put_InnerFECRate(BinaryConvolutionCodeRate *pFECRate);
    HRESULT STDMETHODCALLTYPE get_InnerFECRate(BinaryConvolutionCodeRate *pFECRate);
    HRESULT STDMETHODCALLTYPE put_OuterFECMethod(FECMethod *pFECMethod);
    HRESULT STDMETHODCALLTYPE get_OuterFECMethod(FECMethod *pFECMethod);
    HRESULT STDMETHODCALLTYPE put_OuterFECRate(BinaryConvolutionCodeRate *pFECRate);
    HRESULT STDMETHODCALLTYPE get_OuterFECRate(BinaryConvolutionCodeRate *pFECRate);
    HRESULT STDMETHODCALLTYPE put_SymbolRate(ULONG *pSymbolRate);
    HRESULT STDMETHODCALLTYPE get_SymbolRate(ULONG *pSymbolRate);
    HRESULT STDMETHODCALLTYPE put_SpectralInversion(SpectralInversion *pSpectralInversion);
    HRESULT STDMETHODCALLTYPE get_SpectralInversion(SpectralInversion *pSpectralInversion);

    CBDADigitalDemodulator(HANDLE hFile) : m_Ref(0), m_hFile(hFile){};
    ~CBDADigitalDemodulator(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;

};

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBDA_DigitalDemodulator))
    {
        *Output = (IBDA_DigitalDemodulator*)(this);
        reinterpret_cast<IBDA_DigitalDemodulator*>(*Output)->AddRef();
        return NOERROR;
    }

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDADigitalDemodulator::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_ModulationType(ModulationType *pModulationType)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_ModulationType NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_ModulationType(ModulationType *pModulationType)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_ModulationType NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_InnerFECMethod(FECMethod *pFECMethod)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_InnerFECMethod NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_InnerFECMethod(FECMethod *pFECMethod)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_InnerFECMethod NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_InnerFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_InnerFECRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_InnerFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_InnerFECRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_OuterFECMethod(FECMethod *pFECMethod)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_OuterFECMethod NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE CBDADigitalDemodulator::get_OuterFECMethod(FECMethod *pFECMethod)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_OuterFECMethod NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_OuterFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_OuterFECRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_OuterFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_OuterFECRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_SymbolRate(ULONG *pSymbolRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_SymbolRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_SymbolRate(ULONG *pSymbolRate)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_SymbolRate NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_SpectralInversion(SpectralInversion *pSpectralInversion)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::put_SpectralInversion NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_SpectralInversion(SpectralInversion *pSpectralInversion)
{
    OutputDebugStringW(L"CBDADigitalDemodulator::get_SpectralInversion NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
WINAPI
CBDADigitalDemodulator_fnConstructor(
    HANDLE hFile,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDADigitalDemodulator * handler = new CBDADigitalDemodulator(hFile);

    OutputDebugStringW(L"CBDADigitalDemodulator_fnConstructor\n");

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
