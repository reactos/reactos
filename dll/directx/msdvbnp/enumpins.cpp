/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/enumpins.cpp
 * PURPOSE:         IEnumPins interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CEnumPins : public IEnumPins
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


    HRESULT STDMETHODCALLTYPE Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG cPins);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumPins **ppEnum);

    CEnumPins(ULONG NumPins, IPin ** pins) : m_Ref(0), m_NumPins(NumPins), m_Pins(pins), m_Index(0){};
    virtual ~CEnumPins(){};

protected:
    LONG m_Ref;
    ULONG m_NumPins;
    IPin ** m_Pins;
    ULONG m_Index;
};

HRESULT
STDMETHODCALLTYPE
CEnumPins::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IEnumPins))
    {
        *Output = (IEnumPins*)(this);
        reinterpret_cast<IEnumPins*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CEnumPins::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CEnumPins::Next(
    ULONG cPins,
    IPin **ppPins,
    ULONG *pcFetched)
{
    ULONG i = 0;

    if (!ppPins)
        return E_POINTER;

    if (cPins > 1 && !pcFetched)
        return E_INVALIDARG;

    while(i < cPins)
    {
        if (m_Index + i >= m_NumPins)
            break;

        ppPins[i] = m_Pins[m_Index + i];
        m_Pins[m_Index + i]->AddRef();

        i++;
    }

    if (pcFetched)
    {
        *pcFetched = i;
    }

    m_Index += i;

    if (i < cPins)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumPins::Skip(
    ULONG cPins)
{
    if (cPins + m_Index >= m_NumPins)
    {
        return S_FALSE;
    }

    m_Index += cPins;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumPins::Reset()
{
    m_Index = 0;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumPins::Clone(
    IEnumPins **ppEnum)
{
    OutputDebugStringW(L"CEnumPins::Clone : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CEnumPins_fnConstructor(
    IUnknown *pUnknown,
    ULONG NumPins,
    IPin ** pins,
    REFIID riid,
    LPVOID * ppv)
{
    CEnumPins * handler = new CEnumPins(NumPins, pins);

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
