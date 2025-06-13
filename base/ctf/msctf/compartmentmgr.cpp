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
#include <msctf.h>
#include <msctf_undoc.h>
#include <wine/list.h>

// Cicero
#include <cicbase.h>
#include <cicreg.h>
#include <cicutb.h>

#include "compartmentmgr.h"
#include "msctf_internal.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////
// CCompartmentValue

CCompartmentValue::CCompartmentValue() : m_owner(0)
{
    list_init(&m_entry);
    m_compartment = NULL;
}

CCompartmentValue::~CCompartmentValue()
{
    if (m_compartment)
    {
        m_compartment->Release();
        m_compartment = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
// CEnumCompartment

CEnumCompartment::CEnumCompartment()
    : m_cRefs(1)
    , m_valuesHead(NULL)
    , m_cursor(NULL)
{
}

CEnumCompartment::~CEnumCompartment()
{
}

HRESULT CEnumCompartment::Init(struct list *values_head)
{
    if (!values_head)
        return E_INVALIDARG;
    m_valuesHead = values_head;
    m_cursor = list_head(m_valuesHead); // Start from the first element after the head
    return S_OK;
}

STDMETHODIMP CEnumCompartment::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IEnumGUID))
        *ppvOut = static_cast<IEnumGUID *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumCompartment::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CEnumCompartment::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CEnumCompartment::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    ULONG fetched = 0;

    if (!rgelt)
        return E_POINTER;

    for (; fetched < celt && m_cursor != m_valuesHead; m_cursor = m_cursor->next)
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
    for (ULONG i = 0; i < celt && m_cursor != m_valuesHead; ++i)
    {
        m_cursor = m_cursor->next;
    }
    return S_OK;
}

STDMETHODIMP CEnumCompartment::Reset()
{
    m_cursor = list_head(m_valuesHead); // Reset to the first element
    return S_OK;
}

STDMETHODIMP CEnumCompartment::Clone(IEnumGUID **ppenum)
{
    if (!ppenum)
        return E_POINTER;

    *ppenum = NULL;
    CEnumCompartment *newEnum = new(cicNoThrow) CEnumCompartment();
    if (!newEnum)
        return E_OUTOFMEMORY;

    HRESULT hr = newEnum->Init(m_valuesHead);
    if (FAILED(hr))
    {
        delete newEnum;
        return hr;
    }

    newEnum->m_cursor = m_cursor; // Clone the current cursor position
    *ppenum = newEnum;
    return hr;
}

////////////////////////////////////////////////////////////////////////////
// CCompartment

CCompartment::CCompartment()
    : m_cRefs(1)
    , m_valueData(NULL)
{
    ::VariantInit(&m_variant);
    list_init(&m_CompartmentEventSink);
}

CCompartment::~CCompartment()
{
    ::VariantClear(&m_variant);
    free_sinks(&m_CompartmentEventSink);
}

HRESULT CCompartment::Init(CCompartmentValue *valueData_in)
{
    if (!valueData_in)
        return E_INVALIDARG;
    m_valueData = valueData_in;
    return S_OK;
}

STDMETHODIMP CCompartment::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ITfCompartment))
        *ppvOut = static_cast<ITfCompartment *>(this);
    else if (IsEqualIID(iid, IID_ITfSource))
        *ppvOut = static_cast<ITfSource *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCompartment::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCompartment::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CCompartment::SetValue(TfClientId tid, const VARIANT *pvarValue)
{
    if (!pvarValue)
        return E_INVALIDARG;

    VARTYPE vt = V_VT(pvarValue);
    if (!(vt == VT_BSTR || vt == VT_I4 || vt == VT_UNKNOWN))
        return E_INVALIDARG;

    if (!m_valueData->m_owner)
        m_valueData->m_owner = tid;

    ::VariantCopy(&m_variant, (VARIANT *)pvarValue);

    struct list *cursor = m_CompartmentEventSink.next;
    while (cursor != &m_CompartmentEventSink)
        cursor = cursor->next;

    return S_OK;
}

STDMETHODIMP CCompartment::GetValue(VARIANT *pvarValue)
{
    if (!pvarValue)
        return E_INVALIDARG;

    ::VariantInit(pvarValue);
    if (V_VT(&m_variant) == VT_EMPTY)
        return S_FALSE;

    return ::VariantCopy(pvarValue, &m_variant);
}

STDMETHODIMP CCompartment::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    if (!punk || !pdwCookie)
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_ITfCompartmentEventSink))
        return advise_sink(&m_CompartmentEventSink, IID_ITfCompartmentEventSink,
                           COOKIE_MAGIC_COMPARTMENTSINK, punk, pdwCookie);

    return E_NOTIMPL;
}

STDMETHODIMP CCompartment::UnadviseSink(DWORD pdwCookie)
{
    if (get_Cookie_magic(pdwCookie) != COOKIE_MAGIC_COMPARTMENTSINK)
        return E_INVALIDARG;
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
        delete value;
    }
}

STDMETHODIMP CCompartmentMgr::QueryInterface(REFIID iid, LPVOID *ppvOut)
{
    if (m_pUnkOuter)
        return m_pUnkOuter->QueryInterface(iid, ppvOut);

    *ppvOut = NULL;

    if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_ITfCompartmentMgr))
        *ppvOut = static_cast<ITfCompartmentMgr *>(this);

    if (*ppvOut)
    {
        AddRef();
        return S_OK;
    }

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

    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CCompartmentMgr::GetCompartment(REFGUID rguid, ITfCompartment **ppcomp)
{
    if (!ppcomp)
        return E_POINTER;

    *ppcomp = NULL;

    // Search for existing compartment
    struct list *cursor;
    LIST_FOR_EACH(cursor, &m_values)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        if (!IsEqualGUID(rguid, value->m_guid))
            continue;

        *ppcomp = value->m_compartment;
        value->m_compartment->AddRef();
        return S_OK;
    }

    // Not found, create a new one
    CCompartmentValue *newValue = new (cicNoThrow) CCompartmentValue();
    if (!newValue)
        return E_OUTOFMEMORY;

    newValue->m_guid = rguid;
    newValue->m_owner = 0; // Will be set by CCompartment::SetValue

    CCompartment *newCompart = new (cicNoThrow) CCompartment();
    if (!newCompart)
    {
        delete newValue;
        return E_OUTOFMEMORY;
    }

    HRESULT hr = newCompart->Init(newValue);
    if (FAILED(hr))
    {
        delete newCompart;
        delete newValue;
        *ppcomp = NULL;
        return hr;
    }

    newValue->m_compartment = newCompart;

    list_add_head(&m_values, &newValue->m_entry);

    *ppcomp = newCompart;
    newCompart->AddRef();

    return hr;
}

STDMETHODIMP CCompartmentMgr::ClearCompartment(TfClientId tid, REFGUID rguid)
{
    struct list *cursor;
    LIST_FOR_EACH(cursor, &m_values)
    {
        CCompartmentValue *value = LIST_ENTRY(cursor, CCompartmentValue, m_entry);
        if (!IsEqualGUID(rguid, value->m_guid))
            continue;

        if (value->m_owner && tid != value->m_owner)
            return E_UNEXPECTED;

        list_remove(cursor);
        delete value;
        return S_OK;
    }

    return OLE_E_NOCONNECTION;
}

STDMETHODIMP CCompartmentMgr::EnumCompartments(IEnumGUID **ppEnum)
{
    if (!ppEnum)
        return E_INVALIDARG;
    *ppEnum = NULL;

    CEnumCompartment *newEnum = new (cicNoThrow) CEnumCompartment();
    if (!newEnum)
        return E_OUTOFMEMORY;

    HRESULT hr = newEnum->Init(&m_values);
    if (FAILED(hr))
    {
        delete newEnum;
        return hr;
    }

    *ppEnum = newEnum;
    return hr;
}

HRESULT CCompartmentMgr::CreateInstance(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut)
{
    if (!ppOut)
        return E_POINTER;

    *ppOut = NULL;

    if (pUnkOuter && !IsEqualIID(riid, IID_IUnknown))
        return CLASS_E_NOAGGREGATION;

    CCompartmentMgr *newMgr = new (cicNoThrow) CCompartmentMgr(pUnkOuter);
    if (!newMgr)
        return E_OUTOFMEMORY;

    if (pUnkOuter)
    {
        // Aggregated object: return the inner unknown (the ITfCompartmentMgr interface itself)
        *ppOut = static_cast<ITfCompartmentMgr *>(newMgr);
        newMgr->AddRef();
        return S_OK;
    }

    // Non-aggregated: QueryInterface for the requested IID
    HRESULT hr = newMgr->QueryInterface(riid, (void **)ppOut);
    if (FAILED(hr))
        delete newMgr;

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
    iface->Release();
    return S_OK;
}
