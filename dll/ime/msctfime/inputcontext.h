/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Input Context of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "sink.h"

class CInputContextOwnerCallBack;
class CInputContextOwner;

HRESULT
Inquire(
    _Out_ LPIMEINFO lpIMEInfo,
    _Out_ LPWSTR lpszWndClass,
    _In_ DWORD dwSystemInfoFlags,
    _In_ HKL hKL);

/***********************************************************************
 *      CicInputContext
 *
 * The msctfime.ime's input context.
 */
class CicInputContext
    : public ITfCleanupContextSink
    , public ITfContextOwnerCompositionSink
    , public ITfCompositionSink
{
public:
    LONG m_cRefs;
    HIMC m_hIMC;
    ITfDocumentMgr *m_pDocumentMgr;
    ITfContext *m_pContext;
    ITfContextOwnerServices *m_pContextOwnerServices;
    CInputContextOwnerCallBack *m_pICOwnerCallback;
    CTextEventSink *m_pTextEventSink;
    CCompartmentEventSink *m_pCompEventSink1;
    CCompartmentEventSink *m_pCompEventSink2;
    CInputContextOwner *m_pInputContextOwner;
    DWORD m_dwUnknown3[3];
    DWORD m_dwUnknown4[2];
    DWORD m_dwQueryPos;
    DWORD m_dwUnknown5;
    GUID m_guid;
    DWORD m_dwUnknown6[11];
    BOOL m_bSelecting;
    DWORD m_dwUnknown6_5;
    LONG m_cCompLocks;
    DWORD m_dwUnknown7[5];
    WORD m_cGuidAtoms;
    WORD m_padding;
    DWORD m_adwGuidAtoms[256];
    DWORD m_dwUnknown8[17];
    TfClientId m_clientId;
    DWORD m_dwUnknown9;

public:
    CicInputContext(
        _In_ TfClientId cliendId,
        _Inout_ PCIC_LIBTHREAD pLibThread,
        _In_ HIMC hIMC);
    virtual ~CicInputContext() { }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfCleanupContextSink interface
    STDMETHODIMP OnCleanupContext(_In_ TfEditCookie ecWrite, _Inout_ ITfContext *pic) override;

    // ITfContextOwnerCompositionSink interface
    STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk) override;
    STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew) override;
    STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition) override;

    // ITfCompositionSink interface
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition) override;

    HRESULT
    GetGuidAtom(
        _Inout_ CicIMCLock& imcLock,
        _In_ BYTE iAtom,
        _Out_opt_ LPDWORD pdwGuidAtom);

    HRESULT CreateInputContext(_Inout_ ITfThreadMgr *pThreadMgr, _Inout_ CicIMCLock& imcLock);
    HRESULT DestroyInputContext();
};
