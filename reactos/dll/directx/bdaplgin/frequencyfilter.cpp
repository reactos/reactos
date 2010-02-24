/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/frequencyfilter.cpp
 * PURPOSE:         IBDA_FrequencyFilter interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_FrequencyFilter = {0x71985f47, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x00, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};

class CBDAFrequencyFilter : public IBDA_FrequencyFilter
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

    HRESULT STDMETHODCALLTYPE put_Autotune(ULONG ulTransponder);
    HRESULT STDMETHODCALLTYPE get_Autotune(ULONG *pulTransponder);
    HRESULT STDMETHODCALLTYPE put_Frequency(ULONG ulFrequency);
    HRESULT STDMETHODCALLTYPE get_Frequency(ULONG *pulFrequency);
    HRESULT STDMETHODCALLTYPE put_Polarity(Polarisation Polarity);
    HRESULT STDMETHODCALLTYPE get_Polarity(Polarisation *pPolarity);
    HRESULT STDMETHODCALLTYPE put_Range(ULONG ulRange);
    HRESULT STDMETHODCALLTYPE get_Range(ULONG *pulRange);
    HRESULT STDMETHODCALLTYPE put_Bandwidth(ULONG ulBandwidth);
    HRESULT STDMETHODCALLTYPE get_Bandwidth(ULONG *pulBandwidth);
    HRESULT STDMETHODCALLTYPE put_FrequencyMultiplier(ULONG ulMultiplier);
    HRESULT STDMETHODCALLTYPE get_FrequencyMultiplier(ULONG *pulMultiplier);

    CBDAFrequencyFilter(HANDLE hFile) : m_Ref(0), m_hFile(hFile){};
    virtual ~CBDAFrequencyFilter(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
};

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::QueryInterface(
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

    if (IsEqualGUID(refiid, IID_IBDA_FrequencyFilter))
    {
        *Output = (IBDA_FrequencyFilter*)(this);
        reinterpret_cast<IBDA_FrequencyFilter*>(*Output)->AddRef();
        return NOERROR;
    }

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CControlNode::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Autotune(ULONG ulTransponder)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_Autotune: NotImplemented\n");
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Autotune(ULONG *pulTransponder)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_Autotune\n");
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Frequency(ULONG ulFrequency)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_Frequency: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Frequency(ULONG *pulFrequency)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_Frequency\n");
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Polarity(Polarisation Polarity)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_Polarity: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Polarity(Polarisation *pPolarity)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_Polarity\n");
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Range(ULONG ulRange)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_Range: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Range(ULONG *pulRange)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_Range\n");
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_Bandwidth(ULONG ulBandwidth)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_Bandwidth: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_Bandwidth(ULONG *pulBandwidth)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_Bandwidth\n");
    return E_NOINTERFACE;
}
HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::put_FrequencyMultiplier(ULONG ulMultiplier)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::put_FrequencyMultiplier: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAFrequencyFilter::get_FrequencyMultiplier(ULONG *pulMultiplier)
{
    OutputDebugStringW(L"CBDAFrequencyFilter::get_FrequencyMultiplier\n");
    return E_NOINTERFACE;
}

HRESULT
WINAPI
CBDAFrequencyFilter_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    IKsObject *pObject = NULL;
    HANDLE hFile;

    // sanity check
    assert(pUnkOuter);

    // query for IKsObject
    hr = pUnkOuter->QueryInterface(IID_IKsObject, (void**)&pObject);

    // sanity check
    assert(hr == NOERROR);

    // another sanity check
    assert(pObject != NULL);

    // get file handle
    hFile = pObject->KsGetObjectHandle();

    // one more sanity check
    assert(hFile != NULL && hFile != INVALID_HANDLE_VALUE);

    // release IKsObject interface
    pObject->Release();

    // construct device control
    CBDAFrequencyFilter * handler = new CBDAFrequencyFilter(hFile);

    OutputDebugStringW(L"CBDAFrequencyFilter_fnConstructor\n");

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