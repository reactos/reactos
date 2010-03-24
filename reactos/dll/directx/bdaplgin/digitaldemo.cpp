/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/digitaldemo.cpp
 * PURPOSE:         IBDA_DigitalDemodulator interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

#ifndef _MSC_VER
const GUID IID_IBDA_DigitalDemodulator = {0xef30f379, 0x985b, 0x4d10, {0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x04, 0xe1, 0xe0}};
const GUID KSPROPSETID_BdaDigitalDemodulator = {0xef30f379, 0x985b, 0x4d10, {0xb6, 0x40, 0xa7, 0x9d, 0x5e, 0x4, 0xe1, 0xe0}};
#endif

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

    CBDADigitalDemodulator(HANDLE hFile, ULONG NodeId) : m_Ref(0), m_hFile(hFile), m_NodeId(NodeId){};
    ~CBDADigitalDemodulator(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
    ULONG m_NodeId;
};

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
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

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDADigitalDemodulator::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);
#endif

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_ModulationType(ModulationType *pModulationType)
{
    KSP_NODE Node;
    HRESULT hr;

    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_MODULATION_TYPE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pModulationType, sizeof(ModulationType), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_ModulationType: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_ModulationType(ModulationType *pModulationType)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_InnerFECMethod(FECMethod *pFECMethod)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_INNER_FEC_TYPE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pFECMethod, sizeof(FECMethod), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_InnerFECMethod: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_InnerFECMethod(FECMethod *pFECMethod)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_InnerFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    KSP_NODE Node;
    HRESULT hr;

    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_INNER_FEC_RATE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pFECRate, sizeof(BinaryConvolutionCodeRate), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_InnerFECRate: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_InnerFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_OuterFECMethod(FECMethod *pFECMethod)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_OUTER_FEC_TYPE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pFECMethod, sizeof(FECMethod), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_OuterFECMethod: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}


HRESULT
STDMETHODCALLTYPE CBDADigitalDemodulator::get_OuterFECMethod(FECMethod *pFECMethod)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_OuterFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    KSP_NODE Node;
    HRESULT hr;

    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_OUTER_FEC_RATE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pFECRate, sizeof(BinaryConvolutionCodeRate), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_OuterFECRate: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_OuterFECRate(BinaryConvolutionCodeRate *pFECRate)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_SymbolRate(ULONG *pSymbolRate)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_SYMBOL_RATE;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pSymbolRate, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_SymbolRate: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_SymbolRate(ULONG *pSymbolRate)
{
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::put_SpectralInversion(SpectralInversion *pSpectralInversion)
{
    KSP_NODE Node;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Node.Property.Set = KSPROPSETID_BdaDigitalDemodulator;
    Node.Property.Id = KSPROPERTY_BDA_SPECTRAL_INVERSION;
    Node.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | KSPROPERTY_TYPE_SET;
    Node.NodeId = m_NodeId;

    // perform request
    hr = KsSynchronousDeviceControl(m_hFile, IOCTL_KS_PROPERTY, (PVOID)&Node, sizeof(KSP_NODE), pSpectralInversion, sizeof(SpectralInversion), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADigitalDemodulator::put_SpectralInversion: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADigitalDemodulator::get_SpectralInversion(SpectralInversion *pSpectralInversion)
{
    return E_NOINTERFACE;
}


HRESULT
WINAPI
CBDADigitalDemodulator_fnConstructor(
    HANDLE hFile,
    ULONG NodeId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CBDADigitalDemodulator * handler = new CBDADigitalDemodulator(hFile, NodeId);

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADigitalDemodulator_fnConstructor\n");
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
