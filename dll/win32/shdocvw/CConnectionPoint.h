/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Connection Point
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus

typedef struct tagCONNECTION
{
    CComPtr<IDispatch> m_pDispatch; // Communicate with connection
} CONNECTION, *PCONNECTION;

/*************************************************************************
 *    CConnectionPoint class --- Connection point
 */
class CConnectionPoint : public IConnectionPoint
{
public:
    CConnectionPoint();
    virtual ~CConnectionPoint();

    HRESULT Init(_Inout_ IConnectionPointContainer *pContainer, _In_ const IID *pIID);
    HRESULT InvokeDispid(_In_ DISPID dispId);
    HRESULT UnadviseAll();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ LPVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    // *** IConnectionPoint methods ***
    STDMETHODIMP GetConnectionInterface(_Out_ IID *pIID) override;
    STDMETHODIMP GetConnectionPointContainer(_Out_ IConnectionPointContainer **ppCPC) override;
    STDMETHODIMP Advise(_In_ IUnknown *pUnkSink, _Out_ DWORD *pdwCookie) override;
    STDMETHODIMP Unadvise(_In_ DWORD dwCookie);
    STDMETHODIMP EnumConnections(_Out_ IEnumConnections **ppEnum) override;
    // *** CConnectionPoint methods ***
    STDMETHOD(DoInvokeIE4)(
        _In_ DWORD unknown1,
        _In_ DWORD unknown2,
        _In_ DISPID dispId,
        _Inout_ DISPPARAMS *dispParams);
    STDMETHOD(UnknownMethod0)(_In_ DWORD unknown1, _In_ DWORD unknown2);

protected:
    LONG m_nRefs; // The reference count
    PCONNECTION m_pSlots; // The slots that stores items in
    UINT m_nValidItems; // The number of valid connections
    UINT m_nSlots; // The capacity of m_pSlots
    CComPtr<IConnectionPointContainer> m_pContainer;
    const IID *m_pIID; // The interface ID
};

#endif /* def __cplusplus */
