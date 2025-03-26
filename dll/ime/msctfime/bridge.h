/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     The bridge of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "sinks.h"
#include "tls.h"

class CicBridge : public ITfSysHookSink
{
protected:
    LONG m_cRefs;
    BOOL m_bImmxInited;
    BOOL m_bUnknown1;
    BOOL m_bDeactivating;
    DWORD m_cActivateLocks;
    ITfKeystrokeMgr *m_pKeystrokeMgr;
    ITfDocumentMgr *m_pDocMgr;
    CThreadMgrEventSink *m_pThreadMgrEventSink;
    TfClientId m_cliendId;
    CIC_LIBTHREAD m_LibThread;
    BOOL m_bUnknown2;

    static BOOL CALLBACK EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam);
    static BOOL CALLBACK EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam);

    LRESULT EscHanjaMode(TLS *pTLS, HIMC hIMC, LPVOID lpData);

public:
    CicBridge();
    virtual ~CicBridge();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfSysHookSink interface
    STDMETHODIMP OnPreFocusDIM(HWND hwnd) override;
    STDMETHODIMP OnSysKeyboardProc(UINT, LONG) override;
    STDMETHODIMP OnSysShellProc(INT, UINT, LONG) override;

    HRESULT InitIMMX(_Inout_ TLS *pTLS);
    BOOL UnInitIMMX(_Inout_ TLS *pTLS);
    HRESULT ActivateIMMX(_Inout_ TLS *pTLS, _Inout_ ITfThreadMgr_P *pThreadMgr);
    HRESULT DeactivateIMMX(_Inout_ TLS *pTLS, _Inout_ ITfThreadMgr_P *pThreadMgr);

    HRESULT CreateInputContext(TLS *pTLS, HIMC hIMC);
    HRESULT DestroyInputContext(TLS *pTLS, HIMC hIMC);
    ITfContext *GetInputContext(CicIMCCLock<CTFIMECONTEXT>& imeContext);

    HRESULT SelectEx(
        _Inout_ TLS *pTLS,
        _Inout_ ITfThreadMgr_P *pThreadMgr,
        _In_ HIMC hIMC,
        _In_ BOOL fSelect,
        _In_ HKL hKL);
    HRESULT OnSetOpenStatus(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        CicIMCLock& imcLock,
        CicInputContext *pCicIC);

    void PostTransMsg(_In_ HWND hWnd, _In_ INT cTransMsgs, _In_ const TRANSMSG *pTransMsgs);
    ITfDocumentMgr* GetDocumentManager(_Inout_ CicIMCCLock<CTFIMECONTEXT>& imeContext);

    HRESULT
    ConfigureGeneral(_Inout_ TLS* pTLS,
        _In_ ITfThreadMgr *pThreadMgr,
        _In_ HKL hKL,
        _In_ HWND hWnd);
    HRESULT ConfigureRegisterWord(
        _Inout_ TLS* pTLS,
        _In_ ITfThreadMgr *pThreadMgr,
        _In_ HKL hKL,
        _In_ HWND hWnd,
        _Inout_opt_ LPVOID lpData);

    HRESULT SetActiveContextAlways(TLS *pTLS, HIMC hIMC, BOOL fActive, HWND hWnd, HKL hKL);

    void SetAssociate(
        TLS *pTLS,
        HWND hWnd,
        HIMC hIMC,
        ITfThreadMgr_P *pThreadMgr,
        ITfDocumentMgr *pDocMgr);

    HRESULT Notify(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        HIMC hIMC,
        DWORD dwAction,
        DWORD dwIndex,
        DWORD_PTR dwValue);

    BOOL ProcessKey(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        HIMC hIMC,
        WPARAM wParam,
        LPARAM lParam,
        CONST LPBYTE lpbKeyState,
        INT *pnUnknown60);

    HRESULT ToAsciiEx(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        UINT uVirtKey,
        UINT uScanCode,
        CONST LPBYTE lpbKeyState,
        LPTRANSMSGLIST lpTransBuf,
        UINT fuState,
        HIMC hIMC,
        UINT *pResult);

    BOOL SetCompositionString(
        TLS *pTLS,
        ITfThreadMgr_P *pThreadMgr,
        HIMC hIMC,
        DWORD dwIndex,
        LPCVOID lpComp,
        DWORD dwCompLen,
        LPCVOID lpRead,
        DWORD dwReadLen);

    LRESULT EscapeKorean(
        TLS *pTLS,
        HIMC hIMC,
        UINT uSubFunc,
        LPVOID lpData);

    static BOOL IsOwnDim(ITfDocumentMgr *pDocMgr);

    BOOL
    DoOpenCandidateHanja(
        ITfThreadMgr_P *pThreadMgr,
        CicIMCLock& imcLock,
        CicInputContext *pCicIC);

    HRESULT
    OnSetConversionSentenceMode(
        ITfThreadMgr_P *pThreadMgr,
        CicIMCLock& imcLock,
        CicInputContext *pCicIC,
        DWORD dwValue,
        LANGID LangID);
};
