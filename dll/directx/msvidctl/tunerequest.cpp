/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msvidctl/tunerequest.cpp
 * PURPOSE:         ITuningRequest interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CTuneRequest : public IDVBTuneRequest
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
            OutputDebugStringW(L"CTuneRequest::Release : delete\n");

            WCHAR Buffer[100];
            swprintf(Buffer, L"CTuneRequest::Release : m_TuningSpace %p delete\n", m_TuningSpace);
            OutputDebugStringW(Buffer);


            m_TuningSpace->Release();
            //delete this;
            return 0;
        }
        return m_Ref;
    }

    //IDispatch methods
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

     //ITuneRequest methods
    HRESULT STDMETHODCALLTYPE get_TuningSpace(ITuningSpace **TuningSpace);
    HRESULT STDMETHODCALLTYPE get_Components(IComponents **Components);
    HRESULT STDMETHODCALLTYPE Clone(ITuneRequest **NewTuneRequest);
    HRESULT STDMETHODCALLTYPE get_Locator(ILocator **Locator);
    HRESULT STDMETHODCALLTYPE put_Locator(ILocator *Locator);

    //IDVBTuneRequest methods
    HRESULT STDMETHODCALLTYPE get_ONID(long *ONID);
    HRESULT STDMETHODCALLTYPE put_ONID(long ONID);
    HRESULT STDMETHODCALLTYPE get_TSID(long *TSID);
    HRESULT STDMETHODCALLTYPE put_TSID(long TSID);
    HRESULT STDMETHODCALLTYPE get_SID(long *SID);
    HRESULT STDMETHODCALLTYPE put_SID(long SID);

    CTuneRequest(ITuningSpace * TuningSpace) : m_Ref(0), m_ONID(-1), m_TSID(-1), m_SID(-1), m_Locator(0), m_TuningSpace(TuningSpace)
    {
        m_TuningSpace->AddRef();
    };

    CTuneRequest(ITuningSpace * TuningSpace, LONG ONID, LONG TSID, LONG SID, ILocator * Locator) : m_Ref(1), m_ONID(ONID), m_TSID(TSID), m_SID(SID), m_Locator(Locator), m_TuningSpace(TuningSpace)
    {
        if (m_Locator)
            m_Locator->AddRef();

        m_TuningSpace->AddRef();
    };

    virtual ~CTuneRequest(){};

protected:
    LONG m_Ref;
    LONG m_ONID;
    LONG m_TSID;
    LONG m_SID;
    ILocator * m_Locator;
    ITuningSpace * m_TuningSpace;
};


HRESULT
STDMETHODCALLTYPE
CTuneRequest::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_ITuneRequest))
    {
        *Output = (ITuneRequest*)this;
        reinterpret_cast<ITuneRequest*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IDVBTuneRequest))
    {
        *Output = (IDVBTuneRequest*)this;
        reinterpret_cast<IDVBTuneRequest*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CTuneRequest::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDispatch methods
//
HRESULT
STDMETHODCALLTYPE
CTuneRequest::GetTypeInfoCount(UINT *pctinfo)
{
    OutputDebugStringW(L"CTuneRequest::GetTypeInfoCount : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    OutputDebugStringW(L"CTuneRequest::GetTypeInfo : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuneRequest::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    OutputDebugStringW(L"CTuneRequest::GetIDsOfNames : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuneRequest::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OutputDebugStringW(L"CTuneRequest::Invoke : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// ITuneRequest interface
//

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_TuningSpace(ITuningSpace **TuningSpace)
{
#ifdef MSVIDCTL_TRACE
    OutputDebugStringW(L"CTuneRequest::get_TuningSpace\n");
#endif

    *TuningSpace = m_TuningSpace;
    m_TuningSpace->AddRef();

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_Components(IComponents **Components)
{
    OutputDebugStringW(L"CTuneRequest::get_Components : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::Clone(ITuneRequest **NewTuneRequest)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CTuneRequest::Clone %p\n", NewTuneRequest);
    OutputDebugStringW(Buffer);
#endif

    *NewTuneRequest = new CTuneRequest(m_TuningSpace, m_ONID, m_TSID, m_SID, m_Locator);

    if (!*NewTuneRequest)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_Locator(ILocator **Locator)
{
    OutputDebugStringW(L"CTuneRequest::get_Locator : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::put_Locator(ILocator *Locator)
{
    OutputDebugStringW(L"CTuneRequest::put_Locator : stub\n");
    m_Locator = Locator;

    return S_OK;
}

//-------------------------------------------------------------------
// IDVBTuneRequest interface
//

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_ONID(long *ONID)
{
#ifdef MSVIDCTL_TRACE
    OutputDebugStringW(L"CTuneRequest::get_ONID\n");
#endif

    *ONID = m_ONID;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::put_ONID(long ONID)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CTuneRequest::put_ONID : %lu\n", ONID);
    OutputDebugStringW(Buffer);
#endif

    m_ONID = ONID;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_TSID(long *TSID)
{
#ifdef MSVIDCTL_TRACE
    OutputDebugStringW(L"CTuneRequest::get_TSID\n");
#endif

   *TSID = m_TSID;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::put_TSID(long TSID)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CTuneRequest::put_TSID : %lu\n", TSID);
    OutputDebugStringW(Buffer);
#endif

    m_TSID = TSID;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::get_SID(long *SID)
{
#ifdef MSVIDCTL_TRACE
    OutputDebugStringW(L"CTuneRequest::get_SID\n");
#endif

    *SID = m_SID;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuneRequest::put_SID(long SID)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CTuneRequest::put_SID : %lu\n", SID);
    OutputDebugStringW(Buffer);
#endif

    m_SID = SID;
    return S_OK;
}

HRESULT
WINAPI
CTuneRequest_fnConstructor(
    IUnknown *pUnknown,
    ITuningSpace * TuningSpace,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CTuneRequest * request = new CTuneRequest(TuningSpace);

#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CTuneRequest_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
#endif

    if (!request)
        return E_OUTOFMEMORY;

    if (FAILED(request->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete request;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
