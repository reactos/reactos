/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     The sinks of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

/***********************************************************************/

typedef struct CESMAP
{
    ITfCompartment* m_pComp;
    DWORD m_dwCookie;
} CESMAP, *PCESMAP;

typedef INT (CALLBACK *FN_EVENTSINK)(LPVOID, REFGUID);

class CCompartmentEventSink : public ITfCompartmentEventSink
{
    CicArray<CESMAP> m_array;
    LONG m_cRefs = 1;
    FN_EVENTSINK m_fnEventSink = NULL;
    LPVOID m_pUserData = NULL;

public:
    CCompartmentEventSink(FN_EVENTSINK fnEventSink, LPVOID pUserData);
    virtual ~CCompartmentEventSink() { }

    HRESULT _Advise(IUnknown *pUnknown, REFGUID rguid, BOOL bThread);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfCompartmentEventSink interface
    STDMETHODIMP OnChange(REFGUID rguid) override;
};

/***********************************************************************/

typedef INT (CALLBACK *FN_ENDEDIT)(INT, LPVOID, LPVOID);
typedef INT (CALLBACK *FN_LAYOUTCHANGE)(UINT nType, FN_ENDEDIT fnEndEdit, ITfContextView *pView);

class CTextEventSink : public ITfTextEditSink, ITfTextLayoutSink
{
protected:
    LONG m_cRefs = 1;
    IUnknown* m_pUnknown = NULL;
    DWORD m_dwEditSinkCookie = (DWORD)-1;
    DWORD m_dwLayoutSinkCookie = (DWORD)-1;
    union
    {
        FN_LAYOUTCHANGE m_fnLayoutChange = NULL;
        UINT m_uFlags;
    };
    FN_ENDEDIT m_fnEndEdit = NULL;
    LPVOID m_pCallbackPV = NULL;

public:
    CTextEventSink(FN_ENDEDIT fnEndEdit, LPVOID pCallbackPV);
    virtual ~CTextEventSink() { }

    HRESULT _Advise(IUnknown *pUnknown, UINT uFlags);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextEditSink interface
    STDMETHODIMP OnEndEdit(
        ITfContext *pic,
        TfEditCookie ecReadOnly,
        ITfEditRecord *pEditRecord) override;

    // ITfTextLayoutSink interface
    STDMETHODIMP
    OnLayoutChange(
        ITfContext *pContext,
        TfLayoutCode lcode,
        ITfContextView *pContextView) override;
};

/***********************************************************************/

typedef INT (CALLBACK *FN_INITDOCMGR)(UINT, ITfDocumentMgr *, ITfDocumentMgr *, LPVOID);
typedef INT (CALLBACK *FN_PUSHPOP)(UINT, ITfContext *, LPVOID);

class CThreadMgrEventSink : public ITfThreadMgrEventSink
{
protected:
    ITfThreadMgr* m_pThreadMgr = NULL;
    DWORD m_dwCookie = 0;
    FN_INITDOCMGR m_fnInit = NULL;
    FN_PUSHPOP m_fnPushPop = NULL;
    DWORD m_dw = 0;
    LPVOID m_pCallbackPV = NULL;
    LONG m_cRefs = 1;

public:
    CThreadMgrEventSink(
        _In_ FN_INITDOCMGR fnInit,
        _In_ FN_PUSHPOP fnPushPop = NULL,
        _Inout_ LPVOID pvCallbackPV = NULL);
    virtual ~CThreadMgrEventSink() { }

    void SetCallbackPV(_Inout_ LPVOID pv);
    HRESULT _Advise(ITfThreadMgr *pThreadMgr);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfThreadMgrEventSink interface
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *pdim) override;
    STDMETHODIMP OnSetFocus(ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus) override;
    STDMETHODIMP OnPushContext(ITfContext *pic) override;
    STDMETHODIMP OnPopContext(ITfContext *pic) override;

    static INT CALLBACK DIMCallback(
        UINT nCode,
        ITfDocumentMgr *pDocMgr1,
        ITfDocumentMgr *pDocMgr2,
        LPVOID pUserData);
};

/***********************************************************************/

class CActiveLanguageProfileNotifySink : public ITfActiveLanguageProfileNotifySink
{
protected:
    typedef INT (CALLBACK *FN_COMPARE)(REFGUID rguid1, REFGUID rguid2, BOOL fActivated,
                                       LPVOID pUserData);
    LONG m_cRefs = 1;
    ITfThreadMgr* m_pThreadMgr = NULL;
    DWORD m_dwConnection = 0;
    FN_COMPARE m_fnCompare = NULL;
    LPVOID m_pUserData = NULL;

public:
    CActiveLanguageProfileNotifySink(_In_ FN_COMPARE fnCompare, _Inout_opt_ void *pUserData);
    virtual ~CActiveLanguageProfileNotifySink() { }

    HRESULT _Advise(ITfThreadMgr *pThreadMgr);
    HRESULT _Unadvise();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfActiveLanguageProfileNotifySink interface
    STDMETHODIMP
    OnActivated(
        REFCLSID clsid,
        REFGUID guidProfile,
        BOOL fActivated) override;
};
