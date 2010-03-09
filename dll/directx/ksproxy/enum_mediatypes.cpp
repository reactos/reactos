/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/enum_mediatypes.cpp
 * PURPOSE:         IEnumMediaTypes interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CEnumMediaTypes : public IEnumMediaTypes
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

    HRESULT STDMETHODCALLTYPE Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG cMediaTypes);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes **ppEnum);


    CEnumMediaTypes(ULONG MediaTypeCount, AM_MEDIA_TYPE * MediaTypes) : m_Ref(0), m_MediaTypeCount(MediaTypeCount), m_MediaTypes(MediaTypes), m_Index(0){};
    virtual ~CEnumMediaTypes(){};

protected:
    LONG m_Ref;
    ULONG m_MediaTypeCount;
    AM_MEDIA_TYPE * m_MediaTypes;
    ULONG m_Index;
};

HRESULT
STDMETHODCALLTYPE
CEnumMediaTypes::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IEnumMediaTypes))
    {
        *Output = (IEnumMediaTypes*)(this);
        reinterpret_cast<IEnumMediaTypes*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CEnumMediaTypes::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IEnumMediaTypes
//

HRESULT
STDMETHODCALLTYPE
CEnumMediaTypes::Next(
    ULONG cMediaTypes,
    AM_MEDIA_TYPE **ppMediaTypes,
    ULONG *pcFetched)
{
    ULONG i = 0;
    AM_MEDIA_TYPE * MediaType;

    if (!ppMediaTypes)
        return E_POINTER;

    if (cMediaTypes > 1 && !pcFetched)
        return E_INVALIDARG;

    while(i < cMediaTypes)
    {
        if (m_Index + i >= m_MediaTypeCount)
            break;

        MediaType = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if (!MediaType)
            break;

        CopyMemory(MediaType, &m_MediaTypes[m_Index + i], sizeof(AM_MEDIA_TYPE));
        ppMediaTypes[i] = MediaType;
        i++;
    }

    if (pcFetched)
    {
        *pcFetched = i;
    }

    m_Index += i;

    if (i < cMediaTypes)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumMediaTypes::Skip(
    ULONG cMediaTypes)
{
    if (cMediaTypes + m_Index >= m_MediaTypeCount)
    {
        return S_FALSE;
    }

    m_Index += cMediaTypes;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumMediaTypes::Reset()
{
    m_Index = 0;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CEnumMediaTypes::Clone(
    IEnumMediaTypes **ppEnum)
{
    OutputDebugStringW(L"CEnumMediaTypes::Clone : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CEnumMediaTypes_fnConstructor(
    ULONG MediaTypeCount,
    AM_MEDIA_TYPE * MediaTypes,
    REFIID riid,
    LPVOID * ppv)
{
    CEnumMediaTypes * handler = new CEnumMediaTypes(MediaTypeCount, MediaTypes);

#ifdef KSPROXY_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CEnumMediaTypes_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
#endif

    if (!handler)
    {
        CoTaskMemFree(MediaTypes);
        return E_OUTOFMEMORY;
    }

    if (FAILED(handler->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete handler;
        return E_NOINTERFACE;
    }

    return NOERROR;
}

