/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfCompartmentMgr implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <cguid.h>
#include <olectl.h>
#include <oleauto.h>
#include <msctf.h>
#include <msctf_undoc.h>

// Cicero
#include <cicbase.h>
#include <cicreg.h>
#include <cicutb.h>

#include "msctf_internal.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

typedef struct tagCompartmentValue
{
    struct list entry;
    GUID guid;
    TfClientId owner;
    ITfCompartment *compartment;
} CompartmentValue;

////////////////////////////////////////////////////////////////////////////

class CCompartmentMgr
    : public ITfCompartmentMgr
{
public:
    CCompartmentMgr(IUnknown *pUnkOuter = NULL);
    virtual ~CCompartmentMgr();

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfCompartmentMgr methods **
    STDMETHODIMP GetCompartment(_In_ REFGUID rguid, _Out_ ITfCompartment **ppcomp) override;
    STDMETHODIMP ClearCompartment(_In_ TfClientId tid, _In_ REFGUID rguid) override;
    STDMETHODIMP EnumCompartments(_Out_ IEnumGUID **ppEnum) override;

protected:
    LONG m_cRefs;
    IUnknown *m_pUnkOuter;
    struct list m_values;
};

////////////////////////////////////////////////////////////////////////////

class CCompartmentEnumGuid
    : public IEnumGUID
{
public:
    CCompartmentEnumGuid();
    virtual ~CCompartmentEnumGuid();

    static HRESULT CreateInstance(struct list *values, IEnumGUID **ppOut);
    static HRESULT CreateInstance(struct list *values, IEnumGUID **ppOut, struct list *cursor);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumGUID methods **
    STDMETHODIMP Next(
        _In_ ULONG celt,
        _Out_ GUID *rgelt,
        _Out_ ULONG *pceltFetched) override;
    STDMETHODIMP Skip(_In_ ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(_Out_ IEnumGUID **ppenum) override;

protected:
    LONG m_cRefs;
    struct list *m_values;
    struct list *m_cursor;
};

////////////////////////////////////////////////////////////////////////////

class CCompartment
    : public ITfCompartment
    , public ITfSource
{
public:
    CCompartment();
    virtual ~CCompartment();

    static HRESULT CreateInstance(CompartmentValue *valueData, ITfCompartment **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfCompartment methods **
    STDMETHODIMP SetValue(_In_ TfClientId tid, _In_ const VARIANT *pvarValue) override;
    STDMETHODIMP GetValue(_Out_ VARIANT *pvarValue) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(
        _In_ REFIID riid,
        _In_ IUnknown *punk,
        _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(_In_ DWORD dwCookie) override;

protected:
    LONG m_cRefs;
    VARIANT m_variant; // Only VT_I4, VT_UNKNOWN and VT_BSTR data types are allowed
    CompartmentValue *m_valueData;
    struct list m_CompartmentEventSink;
};

////////////////////////////////////////////////////////////////////////////

CCompartmentMgr::CCompartmentMgr(IUnknown *pUnkOuter)
    : m_cRefs(1)
    , m_pUnkOuter(pUnkOuter)
{
    list_init(&m_values);
}

CCompartmentMgr::~CCompartmentMgr()
{
    struct list *cursor, *cursor2;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_values)
    {
        CompartmentValue *value = LIST_ENTRY(cursor, CompartmentValue, entry);
        list_remove(cursor);
        value->compartment->Release();
        HeapFree(GetProcessHeap(), 0, value);
    }
}

HRESULT CCompartmentMgr::CreateInstance(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut)
{
    if (!ppOut)
        return E_POINTER;

    if (pUnkOuter && riid != IID_IUnknown)
        return CLASS_E_NOAGGREGATION;

    CCompartmentMgr *This = new(cicNoThrow) CCompartmentMgr(pUnkOuter);
    if (This == NULL)
        return E_OUTOFMEMORY;

    if (pUnkOuter)
    {
        *ppOut = static_cast<IUnknown *>(This);
        TRACE("returning %p\n", *ppOut);
        return S_OK;
    }
    else
    {
        HRESULT hr;
        hr = This->QueryInterface(riid, (void **)ppOut);
        if (FAILED(hr))
            delete This;
        return hr;
    }
}

STDMETHODIMP CCompartmentMgr::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    if (m_pUnkOuter)
        return m_pUnkOuter->QueryInterface(iid, ppvOut);

    *ppvOut = NULL;

    if (iid == IID_IUnknown || iid == IID_ITfCompartmentMgr)
        *ppvOut = static_cast<ITfCompartmentMgr *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartmentMgr::AddRef()
{
    if (m_pUnkOuter)
        return m_pUnkOuter->AddRef();
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartmentMgr::Release()
{
    if (m_pUnkOuter)
        return m_pUnkOuter->Release();
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

HRESULT CCompartmentMgr::GetCompartment(_In_ REFGUID rguid, _Out_ ITfCompartment **ppcomp)
{
    if (!ppcomp)
        return E_POINTER;
    CompartmentValue* value;
    struct list *cursor;
    HRESULT hr;

    TRACE("(%p) %s %p\n", this, debugstr_guid(&rguid), ppcomp);

    LIST_FOR_EACH(cursor, &m_values)
    {
        value = LIST_ENTRY(cursor, CompartmentValue, entry);
        if (rguid == value->guid)
        {
            value->compartment->AddRef();
            *ppcomp = value->compartment;
            return S_OK;
        }
    }

    value = (CompartmentValue *)HeapAlloc(GetProcessHeap(), 0, sizeof(CompartmentValue));
    if (!value)
        return E_OUTOFMEMORY;

    value->guid = rguid;
    value->owner = 0;
    hr = CCompartment::CreateInstance(value, &value->compartment);
    if (SUCCEEDED(hr))
    {
        list_add_head(&m_values, &value->entry);
        value->compartment->AddRef();
        *ppcomp = value->compartment;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, value);
        *ppcomp = NULL;
    }

    return hr;
}

HRESULT CCompartmentMgr::ClearCompartment(_In_ TfClientId tid, _In_ REFGUID rguid)
{
    struct list *cursor;

    TRACE("(%p) %i %s\n", this, tid, debugstr_guid(&rguid));

    LIST_FOR_EACH(cursor, &m_values)
    {
        CompartmentValue *value = LIST_ENTRY(cursor,CompartmentValue, entry);
        if (rguid == value->guid)
        {
            if (value->owner && tid != value->owner)
                return E_UNEXPECTED;
            list_remove(cursor);
            value->compartment->Release();
            HeapFree(GetProcessHeap(), 0, value);
            return S_OK;
        }
    }

    return CONNECT_E_NOCONNECTION;
}

HRESULT CCompartmentMgr::EnumCompartments(_Out_ IEnumGUID **ppEnum)
{
    TRACE("(%p) %p\n", this, ppEnum);
    if (!ppEnum)
        return E_INVALIDARG;

    return CCompartmentEnumGuid::CreateInstance(&m_values, ppEnum);
}

////////////////////////////////////////////////////////////////////////////

CCompartmentEnumGuid::CCompartmentEnumGuid()
    : m_cRefs(1)
    , m_values(NULL)
    , m_cursor(NULL)
{
}

CCompartmentEnumGuid::~CCompartmentEnumGuid()
{
    TRACE("destroying %p\n", this);
}

HRESULT
CCompartmentEnumGuid::CreateInstance(struct list *values, IEnumGUID **ppOut)
{
    CCompartmentEnumGuid *This = new(cicNoThrow) CCompartmentEnumGuid();
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->m_values = values;
    This->m_cursor = list_head(values);

    *ppOut = static_cast<IEnumGUID *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

HRESULT
CCompartmentEnumGuid::CreateInstance(struct list *values, IEnumGUID **ppOut, struct list *cursor)
{
    CCompartmentEnumGuid *This = new(cicNoThrow) CCompartmentEnumGuid();
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->m_values = values;
    This->m_cursor = cursor;

    *ppOut = static_cast<IEnumGUID *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

STDMETHODIMP CCompartmentEnumGuid::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumGUID)
    {
        *ppvObj = static_cast<IEnumGUID *>(this);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartmentEnumGuid::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartmentEnumGuid::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP CCompartmentEnumGuid::Next(
    _In_ ULONG celt,
    _Out_ GUID *rgelt,
    _Out_ ULONG *pceltFetched)
{
    ULONG fetched = 0;

    TRACE("(%p)\n", this);

    if (rgelt == NULL)
        return E_POINTER;

    while (fetched < celt && m_cursor)
    {
        CompartmentValue* value = LIST_ENTRY(m_cursor, CompartmentValue, entry);
        if (!value)
            break;

        m_cursor = list_next(m_values, m_cursor);
        *rgelt = value->guid;

        ++fetched;
        ++rgelt;
    }

    if (pceltFetched)
        *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

STDMETHODIMP CCompartmentEnumGuid::Skip(_In_ ULONG celt)
{
    TRACE("(%p)\n", this);
    m_cursor = list_next(m_values, m_cursor);
    return S_OK;
}

STDMETHODIMP CCompartmentEnumGuid::Reset()
{
    TRACE("(%p)\n", this);
    m_cursor = list_head(m_values);
    return S_OK;
}

STDMETHODIMP CCompartmentEnumGuid::Clone(_Out_ IEnumGUID **ppenum)
{
    TRACE("(%p)\n", this);

    if (ppenum == NULL)
        return E_POINTER;

    return CCompartmentEnumGuid::CreateInstance(m_values, ppenum, m_cursor);
}

////////////////////////////////////////////////////////////////////////////

CCompartment::CCompartment()
    : m_cRefs(1)
    , m_valueData(NULL)
{
    VariantInit(&m_variant);
}

CCompartment::~CCompartment()
{
    VariantClear(&m_variant);
    free_sinks(&m_CompartmentEventSink);
}

HRESULT CCompartment::CreateInstance(CompartmentValue *valueData, ITfCompartment **ppOut)
{
    CCompartment *This = new(cicNoThrow) CCompartment();
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->m_valueData = valueData;
    VariantInit(&This->m_variant);

    list_init(&This->m_CompartmentEventSink);

    *ppOut = static_cast<ITfCompartment *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}

STDMETHODIMP CCompartment::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (iid == IID_IUnknown || iid == IID_ITfCompartment)
        *ppvObj = static_cast<ITfCompartment *>(this);
    else if (iid == IID_ITfSource)
        *ppvObj = static_cast<ITfSource *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartment::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartment::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP CCompartment::SetValue(_In_ TfClientId tid, _In_ const VARIANT *pvarValue)
{
    ITfCompartmentEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %i %p\n", this, tid, pvarValue);

    if (!pvarValue)
        return E_INVALIDARG;

    if (!(V_VT(pvarValue) == VT_BSTR || V_VT(pvarValue) == VT_I4 ||
          V_VT(pvarValue) == VT_UNKNOWN))
        return E_INVALIDARG;

    if (!m_valueData->owner)
        m_valueData->owner = tid;

    ::VariantClear(&m_variant);

    /* Shallow copy of value and type */
    m_variant = *pvarValue;

    if (V_VT(pvarValue) == VT_BSTR)
    {
        V_BSTR(&m_variant) = ::SysAllocStringByteLen((char*)V_BSTR(pvarValue),
                ::SysStringByteLen(V_BSTR(pvarValue)));
    }
    else if (V_VT(pvarValue) == VT_UNKNOWN)
    {
        V_UNKNOWN(&m_variant)->AddRef();
    }

    SINK_FOR_EACH(cursor, &m_CompartmentEventSink, ITfCompartmentEventSink, sink)
    {
        sink->OnChange(m_valueData->guid);
    }

    return S_OK;
}

STDMETHODIMP CCompartment::GetValue(_Out_ VARIANT *pvarValue)
{
    TRACE("(%p) %p\n", this, pvarValue);

    if (!pvarValue)
        return E_INVALIDARG;

    ::VariantInit(pvarValue);
    if (V_VT(&m_variant) == VT_EMPTY)
        return S_FALSE;
    return ::VariantCopy(pvarValue, &m_variant);
}

STDMETHODIMP CCompartment::AdviseSink(
    _In_ REFIID riid,
    _In_ IUnknown *punk,
    _Out_ DWORD *pdwCookie)
{
    TRACE("(%p) %s %p %p\n", this, debugstr_guid(&riid), punk, pdwCookie);

    if (cicIsNullPtr(&riid) || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (riid == IID_ITfCompartmentEventSink)
    {
        return advise_sink(&m_CompartmentEventSink, IID_ITfCompartmentEventSink,
                           COOKIE_MAGIC_COMPARTMENTSINK, punk, pdwCookie);
    }

    FIXME("(%p) Unhandled Sink: %s\n", this, debugstr_guid(&riid));
    return E_NOTIMPL;
}

STDMETHODIMP CCompartment::UnadviseSink(_In_ DWORD dwCookie)
{
    TRACE("(%p) %x\n", this, dwCookie);

    if (get_Cookie_magic(dwCookie) != COOKIE_MAGIC_COMPARTMENTSINK)
        return E_INVALIDARG;

    return unadvise_sink(dwCookie);
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT CompartmentMgr_Constructor(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut)
{
    return CCompartmentMgr::CreateInstance(pUnkOuter, riid, ppOut);
}

EXTERN_C
HRESULT CompartmentMgr_Destructor(ITfCompartmentMgr *iface)
{
    iface->Release();
    return S_OK;
}
