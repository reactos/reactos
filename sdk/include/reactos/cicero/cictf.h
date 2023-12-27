/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero Text Framework
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

DEFINE_GUID(IID_ITfSysHookSink, 0x495388DA, 0x21A5, 0x4852, 0x8B, 0xB1, 0xED, 0x2F, 0x29, 0xDA, 0x8D, 0x60);

struct ITfSysHookSink : IUnknown
{
    STDMETHOD(OnPreFocusDIM)(HWND hwnd) = 0;
    STDMETHOD(OnSysKeyboardProc)(UINT, LONG) = 0;
    STDMETHOD(OnSysShellProc)(INT, UINT, LONG) = 0;
};

DEFINE_GUID(IID_ITfThreadMgr_P, 0x7C6247A1, 0x2884, 0x4B7C, 0xAF, 0x24, 0xF1, 0x98, 0x04, 0x7A, 0xA7, 0x28);

struct ITfThreadMgr_P : ITfThreadMgr
{
    STDMETHOD(GetAssociated)(HWND hwnd, ITfDocumentMgr **ppDocMgr) = 0;
    STDMETHOD(SetSysHookSink)(ITfSysHookSink *pSysHookSink) = 0;
    STDMETHOD(RequestPostponedLock)(ITfContext *pContext) = 0;
    STDMETHOD(IsKeystrokeFeedEnabled)(int *) = 0;
    STDMETHOD(CallImm32HotkeyHandler)(UINT vKey, LPARAM lParam, HRESULT* phrResult) = 0;
    STDMETHOD(ActivateEx)(LPDWORD, DWORD) = 0;
};

DEFINE_GUID(IID_ITfKeystrokeMgr_P, 0x53FA1BEC, 0x5BE1, 0x458E, 0xAE, 0x70, 0xA9, 0xF1, 0xDC, 0x84, 0x3E, 0x81);
