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
#include <oleauto.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <wine/list.h>

// Cicero
#include <cicbase.h>

#include "compartmentmgr.h"
#include "msctf_internal.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////
// CCompartmentValue

CCompartmentValue::CCompartmentValue(
    _In_opt_ const GUID *guid,
    _In_ TfClientId owner,
    _In_opt_ ITfCompartment *compartment) : m_cRefs(1), m_owner(owner)
{
    if (guid)
        m_guid = *guid;
    else
        ZeroMemory(&m_guid, sizeof(m_guid));

    list_init(&m_entry);

    m_compartment = compartment;
    if (m_compartment)
        m_compartment->AddRef();
}

CCompartmentValue::~CCompartmentValue()
{
    if (m_compartment)
    {
        m_compartment->Release();
        m_compartment = NULL;
    }
}

HRESULT CCompartmentValue::Clone(CCompartmentValue **ppValue)
{
    if (!ppValue)
    {
        ERR("E_POINTER\n");
        return E_POINTER;
    }

    CCompartmentValue *pCloned = new(cicNoThrow) CCompartmentValue(&m_guid, m_owner, m_compartment);
    if (!pCloned)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    *ppValue = pCloned;
    return TRUE;
}

ULONG CCompartmentValue::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

ULONG CCompartmentValue::Release()
{
    ULONG ref = ::InterlockedDecrement(&m_cRefs);
    if (!ref)
        delete this;
    return ref;
}

////////////////////////////////////////////////////////////////////////////
// CEnumCompartment

CEnumCompartment::CEnumCompartment()
    : m_cRefs(1)
{
    list_init(&m_valueList);
    m_cursor = list_head(&m_valueList);
}

HRESULT
CEnumCompartment::Init(_In_ struct list *valueList, _In_opt_ struct list *current_cursor)
{
    // Duplicate values
    ULONG iItem = 0, iSelected = 0;
    for (struct list *cursor = list_head(valueList); cursor != valueList;
         cursor = list_next(valueList, cursor))
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        if (current_cursor && cursor == current_cursor)
            iSelected = iItem;

        CCompartmentValue *newValue;
        HRESULT hr = value->Clone(&newValue);
        if (FAILED(hr))
        {
            ERR("hr: 0x%lX\n", hr);
            return hr;
        }

        list_add_head(&m_valueList, &newValue->m_entry);
        ++iItem;
    }

    // Positioning
    return iSelected ? Skip(iSelected) : S_OK;
}

CEnumCompartment::~CEnumCompartment()
{
    struct list *cursor, *cursor2;
    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_valueList)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        list_remove(cursor);
        value->Release();
    }
}

STDMETHODIMP CEnumCompartment::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&iid), ppvOut);

    *ppvOut = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumGUID)
        *ppvOut = static_cast<IEnumGUID *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

    ERR("E_NOINTERFACE: %s\n", wine_dbgstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumCompartment::AddRef()
{
    TRACE("%p -> ()\n", this);
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumCompartment::Release()
{
    TRACE("%p -> ()\n", this);
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CEnumCompartment::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    TRACE("%p -> (%lu, %s, %p)\n", this, celt, wine_dbgstr_guid(rgelt), pceltFetched);

    if (!rgelt)
    {
        ERR("!rgelt\n");
        return E_POINTER;
    }

    ULONG fetched;
    for (fetched = 0; fetched < celt && m_cursor != &m_valueList;
         m_cursor = list_next(&m_valueList, m_cursor))
    {
        CCompartmentValue *value = LIST_ENTRY(m_cursor, CCompartmentValue, m_entry);
        if (!value)
            break;

        *rgelt = value->m_guid;
        ++fetched;
        ++rgelt;
    }

    if (pceltFetched)
        *pceltFetched = fetched;

    return (fetched == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumCompartment::Skip(ULONG celt)
{
    TRACE("%p -> (%lu)\n", this, celt);
    ULONG i;
    for (i = 0; i < celt && m_cursor != &m_valueList; ++i)
        m_cursor = list_next(&m_valueList, m_cursor);
    return (i == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP CEnumCompartment::Reset()
{
    TRACE("%p -> ()\n", this);
    m_cursor = list_head(&m_valueList); // Reset to the first element
    return S_OK;
}

STDMETHODIMP CEnumCompartment::Clone(IEnumGUID **ppenum)
{
    TRACE("%p -> (%p)\n", this, ppenum);

    if (!ppenum)
    {
        ERR("!ppenum\n");
        return E_POINTER;
    }

    *ppenum = NULL;

    CEnumCompartment *pCloned = new(cicNoThrow) CEnumCompartment();
    if (!pCloned)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pCloned->Init(&m_valueList, m_cursor);
    if (FAILED(hr))
    {
        ERR("hr: 0x%lX\n", hr);
        pCloned->Release();
        return hr;
    }

    *ppenum = pCloned;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// CCompartment

CCompartment::CCompartment()
    : m_cRefs(1)
    , m_value(NULL)
{
    ::VariantInit(&m_variant);
    list_init(&m_compartmentEventSink);
}

CCompartment::~CCompartment()
{
    ::VariantClear(&m_variant);
    free_sinks(&m_compartmentEventSink);

    if (m_value)
    {
        m_value->Release();
        m_value = NULL;
    }
}

HRESULT CCompartment::Init(_In_ CCompartmentValue *value)
{
    if (!value)
    {
        ERR("!value\n");
        return E_INVALIDARG;
    }
    m_value = value;
    m_value->AddRef();
    return S_OK;
}

STDMETHODIMP CCompartment::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&iid), ppvOut);

    *ppvOut = NULL;

    if (iid == IID_IUnknown || iid == IID_ITfCompartment)
        *ppvOut = static_cast<ITfCompartment *>(this);
    else if (iid == IID_ITfSource)
        *ppvOut = static_cast<ITfSource *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

    ERR("E_NOINTERFACE: %s\n", wine_dbgstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartment::AddRef()
{
    TRACE("%p -> ()\n", this);
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartment::Release()
{
    TRACE("%p -> ()\n", this);
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CCompartment::SetValue(TfClientId tid, const VARIANT *pvarValue)
{
    TRACE("%p -> (%d, %p)\n", this, tid, pvarValue);

    if (!pvarValue)
    {
        ERR("!pvarValue\n");
        return E_INVALIDARG;
    }

    VARTYPE vt = V_VT(pvarValue);
    if (vt != VT_BSTR && vt != VT_I4 && vt != VT_UNKNOWN)
    {
        ERR("vt: %d\n", vt);
        return E_INVALIDARG;
    }

    if (!m_value->m_owner)
        m_value->m_owner = tid;

    ::VariantCopy(&m_variant, (LPVARIANT)pvarValue);

    struct list *cursor;
    ITfCompartmentEventSink *sink;
    SINK_FOR_EACH(cursor, &m_compartmentEventSink, ITfCompartmentEventSink, sink)
    {
        sink->OnChange(m_value->m_guid);
    }

    return S_OK;
}

STDMETHODIMP CCompartment::GetValue(VARIANT *pvarValue)
{
    TRACE("%p -> (%p)\n", this, pvarValue);

    if (!pvarValue)
    {
        ERR("!pvarValue\n");
        return E_INVALIDARG;
    }

    ::VariantInit(pvarValue);
    if (V_VT(&m_variant) == VT_EMPTY)
        return S_FALSE;

    return ::VariantCopy(pvarValue, &m_variant);
}

STDMETHODIMP CCompartment::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    TRACE("%p -> (%s, %p, %p)\n", this, wine_dbgstr_guid(&riid), punk, pdwCookie);

    if (!punk || !pdwCookie)
    {
        ERR("%p, %p\n", punk, pdwCookie);
        return E_INVALIDARG;
    }

    if (riid == IID_ITfCompartmentEventSink)
        return advise_sink(&m_compartmentEventSink, IID_ITfCompartmentEventSink,
                           COOKIE_MAGIC_COMPARTMENTSINK, punk, pdwCookie);

    ERR("E_NOTIMPL\n");
    return E_NOTIMPL;
}

STDMETHODIMP CCompartment::UnadviseSink(DWORD pdwCookie)
{
    TRACE("%p -> (%p)\n", this, pdwCookie);
    if (get_Cookie_magic(pdwCookie) != COOKIE_MAGIC_COMPARTMENTSINK)
    {
        ERR("Invalid cookie\n");
        return E_INVALIDARG;
    }
    return unadvise_sink(pdwCookie);
}

////////////////////////////////////////////////////////////////////////////
// CCompartmentMgr

CCompartmentMgr::CCompartmentMgr(IUnknown *pUnkOuter_in)
    : m_cRefs(1)
    , m_pUnkOuter(pUnkOuter_in)
{
    list_init(&m_values);
}

CCompartmentMgr::~CCompartmentMgr()
{
    struct list *cursor, *cursor2;
    LIST_FOR_EACH_SAFE(cursor, cursor2, &m_values)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        list_remove(cursor);
        value->Release();
    }
}

STDMETHODIMP CCompartmentMgr::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&iid), ppvOut);

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

    ERR("E_NOINTERFACE: %s\n", wine_dbgstr_guid(&iid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartmentMgr::AddRef()
{
    TRACE("%p -> ()\n", this);

    if (m_pUnkOuter)
        return m_pUnkOuter->AddRef();

    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartmentMgr::Release()
{
    TRACE("%p -> ()\n", this);

    if (m_pUnkOuter)
        return m_pUnkOuter->Release();

    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CCompartmentMgr::GetCompartment(REFGUID rguid, ITfCompartment **ppcomp)
{
    TRACE("%p -> (%s, %p)\n", this, wine_dbgstr_guid(&rguid), ppcomp);

    if (!ppcomp)
    {
        ERR("!ppcomp\n");
        return E_POINTER;
    }
    *ppcomp = NULL;

    // Search for existing compartment
    struct list *cursor;
    LIST_FOR_EACH(cursor, &m_values)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        if (rguid != value->m_guid)
            continue;

        *ppcomp = value->m_compartment;
        (*ppcomp)->AddRef();
        return S_OK; // Found
    }

    // Not found, create a new one
    CCompartmentValue *value = new (cicNoThrow) CCompartmentValue(&rguid);
    if (!value)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    CCompartment *compartment = new (cicNoThrow) CCompartment();
    if (!compartment)
    {
        ERR("E_OUTOFMEMORY\n");
        value->Release();
        return E_OUTOFMEMORY;
    }

    HRESULT hr = compartment->Init(value);
    if (FAILED(hr))
    {
        compartment->Release();
        value->Release();
        return hr;
    }

    value->m_compartment = compartment;
    compartment->AddRef();

    list_add_head(&m_values, &value->m_entry);

    *ppcomp = compartment;
    return hr;
}

STDMETHODIMP CCompartmentMgr::ClearCompartment(TfClientId tid, REFGUID rguid)
{
    TRACE("%p -> (%d, %s)\n", this, tid, wine_dbgstr_guid(&rguid));

    struct list *cursor;
    LIST_FOR_EACH(cursor, &m_values)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        if (rguid != value->m_guid)
            continue;

        if (value->m_owner && tid != value->m_owner)
        {
            ERR("E_UNEXPECTED\n");
            return E_UNEXPECTED;
        }

        list_remove(cursor);
        value->Release();
        return S_OK;
    }

    ERR("OLE_E_NOCONNECTION\n");
    return OLE_E_NOCONNECTION;
}

STDMETHODIMP CCompartmentMgr::EnumCompartments(IEnumGUID **ppEnum)
{
    TRACE("%p -> (%p)\n", this, ppEnum);

    if (!ppEnum)
    {
        ERR("!ppEnum");
        return E_INVALIDARG;
    }
    *ppEnum = NULL;

    CEnumCompartment *pEnum = new (cicNoThrow) CEnumCompartment();
    if (!pEnum)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pEnum->Init(&m_values);
    if (FAILED(hr))
    {
        ERR("hr: 0x%lX\n", hr);
        pEnum->Release();
        return hr;
    }

    *ppEnum = pEnum;
    return hr;
}

HRESULT CCompartmentMgr::CreateInstance(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut)
{
    TRACE("(%p, %s, %p)\n", pUnkOuter, wine_dbgstr_guid(&riid), ppOut);

    if (!ppOut)
    {
        ERR("!ppOut\n");
        return E_POINTER;
    }

    *ppOut = NULL;

    if (pUnkOuter && riid != IID_IUnknown)
    {
        ERR("CLASS_E_NOAGGREGATION\n");
        return CLASS_E_NOAGGREGATION;
    }

    CCompartmentMgr *pManager = new (cicNoThrow) CCompartmentMgr(pUnkOuter);
    if (!pManager)
    {
        ERR("E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    if (pUnkOuter) // Aggregated object?
    {
        *ppOut = static_cast<ITfCompartmentMgr *>(pManager);
        pManager->AddRef();
        return S_OK;
    }

    // Non-aggregated object
    HRESULT hr = pManager->QueryInterface(riid, (void **)ppOut);
    if (FAILED(hr))
    {
        ERR("hr: 0x%lX\n", hr);
        pManager->Release();
    }

    return hr;
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
    if (iface)
        iface->Release();
    return S_OK;
}
