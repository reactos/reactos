/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell Desktop Windows List
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "CConnectionPoint.h"

#ifdef __cplusplus

/*************************************************************************
 *    CSDWindows class --- Shell Desktop Windows
 */
class CSDWindows
    : public CComCoClass<CSDWindows, &CLSID_ShellWindows>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IShellWindows
    , public IConnectionPointContainer
{
protected:
    HDPA m_hDpa1;
    HDPA m_hDpa2;
    DWORD m_dwUnknown;
    HWND m_hwndWorker;
    DWORD m_dwThreadId;
    CConnectionPoint m_ConnectionPoint;

public:
    CSDWindows();
    virtual ~CSDWindows() { }

    HRESULT Init(CComPtr<CSDWindows> pSDWindows, const IID *pIID);

    DECLARE_REGISTRY_RESOURCEID(IDR_SHELLWINDOWS)
    DECLARE_NOT_AGGREGATABLE(CSDWindows)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSDWindows)
        COM_INTERFACE_ENTRY_IID(IID_IShellWindows, IShellWindows)
        COM_INTERFACE_ENTRY2_IID(IID_IDispatch, IDispatch, IShellWindows)
        COM_INTERFACE_ENTRY2_IID(IID_IUnknown, IUnknown, IShellWindows)
        COM_INTERFACE_ENTRY_IID(IID_IConnectionPointContainer, IConnectionPointContainer)
    END_COM_MAP()

    // IUnknown methods are populated by CComPtr and ShellObjectCreator

    // *** IDispatch methods ***
    STDMETHODIMP GetTypeInfoCount(_Out_ UINT *pctinfo) override { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(
        _In_ UINT iTInfo,
        _In_ LCID lcid,
        _Out_ ITypeInfo **ppTInfo) override { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(
        _In_ REFIID riid,
        _In_ LPOLESTR *rgszNames,
        _In_ UINT cNames,
        _In_ LCID lcid,
        _Out_ DISPID *rgDispId) override { return E_NOTIMPL; }
    STDMETHODIMP Invoke(
        _In_ DISPID dispIdMember,
        _In_ REFIID riid,
        _In_ LCID lcid,
        _In_ WORD wFlags,
        _In_ DISPPARAMS *pDispParams,
        _Out_ VARIANT *pVarResult,
        _Out_ EXCEPINFO *pExcepInfo,
        _Out_ UINT *puArgErr) override { return E_NOTIMPL; }

    // *** IShellWindows methods ***
    STDMETHODIMP get_Count(_Out_ LONG *Count) override;
    STDMETHODIMP Item(_In_ VARIANT index, _Out_ IDispatch **Folder) override { return E_NOTIMPL; }
    STDMETHODIMP _NewEnum(_Out_ IUnknown **ppunk) override { return E_NOTIMPL; }
    STDMETHODIMP Register(
        _Inout_ IDispatch *pid,
        _In_ LONG hWnd,
        _In_ int swClass,
        _Out_ LONG *plCookie) override { return E_NOTIMPL; }
    STDMETHODIMP RegisterPending(
        _In_ LONG lThreadId,
        _In_ VARIANT *pvarloc,
        _In_ VARIANT *pvarlocRoot,
        _In_ int swClass,
        _Out_ LONG *plCookie) override { return E_NOTIMPL; }
    STDMETHODIMP Revoke(_In_ LONG lCookie) override { return E_NOTIMPL; }
    STDMETHODIMP OnNavigate(_In_ LONG lCookie, _In_ VARIANT *pvarLoc) override { return E_NOTIMPL; }
    STDMETHODIMP OnActivated(_In_ LONG lCookie, _In_ VARIANT_BOOL fActive) override { return E_NOTIMPL; }
    STDMETHODIMP FindWindowSW(
        _In_ VARIANT *pvarLoc,
        _In_ VARIANT *pvarLocRoot,
        _In_ INT swClass,
        _Out_opt_ LONG *phwnd,
        _In_ INT swfwOptions,
        _Out_opt_ IDispatch **ppdispOut) override { return E_NOTIMPL; }
    STDMETHODIMP OnCreated(_In_ LONG lCookie, _Inout_opt_ IUnknown *punk) override { return E_NOTIMPL; }
    STDMETHODIMP ProcessAttachDetach(_In_ VARIANT_BOOL fAttach) override { return E_NOTIMPL; }

    // *** IConnectionPointContainer methods ***
    STDMETHODIMP EnumConnectionPoints(
        _Out_ IEnumConnectionPoints **ppEnum) override { return E_NOTIMPL; }
    STDMETHODIMP FindConnectionPoint(
        _In_ REFIID riid,
        _Out_ IConnectionPoint **ppCP) override { return E_NOTIMPL; }
};

EXTERN_C HRESULT CSDWindows_CreateInstance(_Out_ IShellWindows **ppShellWindows);

#endif // def __cplusplus
