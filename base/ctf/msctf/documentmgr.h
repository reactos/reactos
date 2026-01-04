/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implementation of ITfDocumentMgr and IEnumTfContexts
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

////////////////////////////////////////////////////////////////////////////
// CDocumentMgr

class CDocumentMgr
    : public ITfDocumentMgr
    , public ITfSource
{
public:
    CDocumentMgr(ITfThreadMgrEventSink *threadMgrSink);
    virtual ~CDocumentMgr();

    static HRESULT
    CreateInstance(
        _In_ ITfThreadMgrEventSink *pThreadMgrSink,
        _Out_ ITfDocumentMgr **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfDocumentMgr methods **
    STDMETHODIMP CreateContext(
        TfClientId tidOwner,
        DWORD dwFlags,
        IUnknown *punk,
        ITfContext **ppic,
        TfEditCookie *pecTextStore) override;
    STDMETHODIMP Push(ITfContext *pic) override;
    STDMETHODIMP Pop(DWORD dwFlags) override;
    STDMETHODIMP GetTop(ITfContext **ppic) override;
    STDMETHODIMP GetBase(ITfContext **ppic) override;
    STDMETHODIMP EnumContexts(IEnumTfContexts **ppEnum) override;

    // ** ITfSource methods **
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(DWORD pdwCookie) override;

    friend class CEnumTfContext;

protected:
    LONG m_cRefs;
    ITfCompartmentMgr *m_pCompartmentMgr;
    ITfContext *m_initialContext;
    ITfContext *m_contextStack[2]; // limit of 2 contexts
    ITfThreadMgrEventSink *m_pThreadMgrSink;
    struct list m_transitoryExtensionSink;
};

EXTERN_C
HRESULT EnumTfContext_Constructor(CDocumentMgr *mgr, IEnumTfContexts **ppOut);

////////////////////////////////////////////////////////////////////////////
// CEnumTfContext

class CEnumTfContext
    : public IEnumTfContexts
{
public:
    CEnumTfContext(_In_opt_ CDocumentMgr *mgr);
    virtual ~CEnumTfContext();

    static HRESULT CreateInstance(_In_opt_ CDocumentMgr *mgr, _Out_ IEnumTfContexts **ppOut);

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IEnumTfContexts methods **
    STDMETHODIMP Next(ULONG ulCount, ITfContext **rgContext, ULONG *pcFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumTfContexts **ppenum) override;

protected:
    LONG m_cRefs;
    DWORD m_index;
    CDocumentMgr *m_pDocMgr;
};
