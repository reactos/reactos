/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msvidctl/enumtuningspaces.cpp
 * PURPOSE:         ITuningSpace interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CEnumTuningSpaces : public IEnumTuningSpaces
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

    // IEnumTuningSpaces methods
    HRESULT STDMETHODCALLTYPE Next(ULONG celt, ITuningSpace **rgelt, ULONG *pceltFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumTuningSpaces **ppEnum);

    CEnumTuningSpaces() : m_Ref(0){};

    virtual ~CEnumTuningSpaces(){};

protected:
    LONG m_Ref;
};

HRESULT
STDMETHODCALLTYPE
CEnumTuningSpaces::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IEnumTuningSpaces))
    {
        *Output = (IEnumTuningSpaces*)this;
        reinterpret_cast<IEnumTuningSpaces*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CEnumTuningSpaces::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IEnumTuningSpaces
//
HRESULT
STDMETHODCALLTYPE
CEnumTuningSpaces::Next(ULONG celt, ITuningSpace **rgelt, ULONG *pceltFetched)
{
    OutputDebugStringW(L"CEnumTuningSpaces::Next : stub\n");
    return CTuningSpace_fnConstructor(NULL, IID_ITuningSpace, (void**)rgelt);

}

HRESULT
STDMETHODCALLTYPE
CEnumTuningSpaces::Skip(ULONG celt)
{
    OutputDebugStringW(L"CEnumTuningSpaces::Skip : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CEnumTuningSpaces::Reset()
{
    OutputDebugStringW(L"CEnumTuningSpaces::Reset : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CEnumTuningSpaces::Clone(IEnumTuningSpaces **ppEnum)
{
    OutputDebugStringW(L"CEnumTuningSpaces::Clone : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CEnumTuningSpaces_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CEnumTuningSpaces * tuningspaces = new CEnumTuningSpaces();

#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CEnumTuningSpaces_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
#endif

    if (!tuningspaces)
        return E_OUTOFMEMORY;

    if (FAILED(tuningspaces->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete tuningspaces;
        return E_NOINTERFACE;
    }

    return NOERROR;
}

