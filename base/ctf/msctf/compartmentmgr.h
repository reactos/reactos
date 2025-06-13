/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfCompartmentMgr implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

////////////////////////////////////////////////////////////////////////////
// CCompartmentValue

struct CCompartmentValue
{
    LONG m_cRefs;
    TfClientId m_owner;
    ITfCompartment *m_compartment;
    GUID m_guid;
    struct list m_entry;

    CCompartmentValue(
        _In_opt_ const GUID *guid = NULL,
        _In_ TfClientId owner = 0,
        _In_opt_ ITfCompartment *compartment = NULL);
    virtual ~CCompartmentValue();

    HRESULT Clone(CCompartmentValue **ppValue);
    ULONG AddRef();
    ULONG Release();
};

////////////////////////////////////////////////////////////////////////////
// CEnumCompartment

class CEnumCompartment
    : public IEnumGUID
{
public:
    CEnumCompartment();
    virtual ~CEnumCompartment();

    HRESULT Init(_In_ struct list *valueList, _In_opt_ struct list *current_cursor = NULL);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumGUID methods **
    STDMETHODIMP Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumGUID **ppenum) override;

private:
    LONG m_cRefs;
    struct list m_valueList;
    struct list *m_cursor;
};

////////////////////////////////////////////////////////////////////////////
// CCompartment

class CCompartment
    : public ITfCompartment
    , public ITfSource
{
public:
    CCompartment();
    virtual ~CCompartment();

    HRESULT Init(_In_ CCompartmentValue *value);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfCompartment methods **
    STDMETHODIMP SetValue(TfClientId tid, const VARIANT *pvarValue) override;
    STDMETHODIMP GetValue(VARIANT *pvarValue) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(DWORD pdwCookie) override;

private:
    LONG m_cRefs;
    VARIANT m_variant; // Must be in VT_BSTR, VT_I4, or VT_UNKNOWN
    CCompartmentValue *m_value;
    struct list m_compartmentEventSink; // Placeholder for sink management
};

////////////////////////////////////////////////////////////////////////////
// CCompartmentMgr

class CCompartmentMgr
    : public ITfCompartmentMgr
{
public:
    CCompartmentMgr(IUnknown *pUnkOuter = NULL);
    virtual ~CCompartmentMgr();

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfCompartmentMgr methods **
    STDMETHODIMP GetCompartment(REFGUID rguid, ITfCompartment **ppcomp) override;
    STDMETHODIMP ClearCompartment(TfClientId tid, REFGUID rguid) override;
    STDMETHODIMP EnumCompartments(IEnumGUID **ppEnum) override;

private:
    LONG m_cRefs;
    IUnknown *m_pUnkOuter;
    struct list m_values; // List of CCompartmentValue objects
};
