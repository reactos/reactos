/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/ksproxy/enumpins.cpp
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

    CEnumPins(std::vector<IPin*> Pins) : m_Ref(0), m_Pins(Pins), m_Index(0){};
    virtual ~CEnumPins(){};

protected:
    LONG m_Ref;
    std::vector<IPin*> m_Pins;
    ULONG m_Index;
};

HRESULT
STDMETHODCALLTYPE
CEnumPins::QueryInterface(
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
    if (IsEqualGUID(refiid, IID_IEnumPins))
    {
        *Output = (IEnumPins*)(this);
        reinterpret_cast<IEnumPins*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[100];
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
        if (m_Index + i >= m_Pins.size())
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
    if (cPins + m_Index >= m_Pins.size())
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
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CEnumPins::Clone : NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
WINAPI
CEnumPins_fnConstructor(
    std::vector<IPin*> Pins,
    REFIID riid,
    LPVOID * ppv)
{
    CEnumPins * handler = new CEnumPins(Pins);

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
