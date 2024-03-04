/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for msutb.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

DEFINE_GUID(GUID_COMPARTMENT_SPEECH_OPENCLOSE, 0x544D6A63, 0xE2E8, 0x4752, 0xBB, 0xD1, 0x00, 0x09, 0x60, 0xBC, 0xA0, 0x83);
DEFINE_GUID(GUID_LBI_TRAYMAIN,                 0xE0B724E9, 0x6F76, 0x45F7, 0xB4, 0xC1, 0xB1, 0xC0, 0xFA, 0xBC, 0xE2, 0x3E);
DEFINE_GUID(GUID_LBI_INATITEM,                 0xCDBC683A, 0x55CE, 0x4717, 0xBA, 0xC0, 0x50, 0xBF, 0x44, 0xA3, 0x27, 0x0C);
DEFINE_GUID(GUID_LBI_CTRL,                     0x58C99D96, 0x2F9B, 0x42CE, 0x91, 0xBE, 0x37, 0xEF, 0x18, 0x60, 0xF8, 0x82);
DEFINE_GUID(GUID_TFCAT_TIP_KEYBOARD,           0x34745C63, 0xB2F0, 0x4784, 0x8B, 0x67, 0x5E, 0x12, 0xC8, 0x70, 0x1A, 0x31);
DEFINE_GUID(CLSID_SYSTEMLANGBARITEM,           0xBEBACC94, 0x5CD3, 0x4662, 0xA1, 0xE0, 0xF3, 0x31, 0x99, 0x49, 0x36, 0x69);
DEFINE_GUID(IID_ITfLangBarMgr_P,               0xD72C0FA9, 0xADD5, 0x4AF0, 0x87, 0x06, 0x4F, 0xA9, 0xAE, 0x3E, 0x2E, 0xFF);
DEFINE_GUID(IID_ITfLangBarEventSink_P,         0x7A460360, 0xDA21, 0x4B09, 0xA8, 0xA0, 0x8A, 0x69, 0xE7, 0x28, 0xD8, 0x93);
DEFINE_GUID(CLSID_MSUTBDeskBand,               0x540D8A8B, 0x1C3F, 0x4E32, 0x81, 0x32, 0x53, 0x0F, 0x6A, 0x50, 0x20, 0x90);
DEFINE_GUID(CATID_DeskBand,                    0x00021492, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
DEFINE_GUID(
    GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY,       0xB95F181B, 0xEA4C, 0x4AF1, 0x80, 0x56, 0x7C, 0x32, 0x1A, 0xBB, 0xB0, 0x91);

typedef struct CIC_LIBTHREAD
{
    ITfCategoryMgr *m_pCategoryMgr;
    ITfDisplayAttributeMgr *m_pDisplayAttrMgr;
} CIC_LIBTHREAD, *PCIC_LIBTHREAD;

EXTERN_C PCIC_LIBTHREAD WINAPI GetLibTls(VOID);
EXTERN_C BOOL WINAPI GetPopupTipbar(HWND hWnd, BOOL fWinLogon);
EXTERN_C HRESULT WINAPI SetRegisterLangBand(BOOL bRegister);
EXTERN_C VOID WINAPI ClosePopupTipbar(VOID);

struct ITfLangBarMgr_P : ITfLangBarMgr
{
    STDMETHOD(GetPrevShowFloatingStatus)(DWORD*) = 0;
};

struct ITfLangBarEventSink_P : IUnknown
{
    STDMETHOD(OnLangBarUpdate)(TfLBIClick click, BOOL bFlag) = 0;
};

inline void TFUninitLib_Thread(PCIC_LIBTHREAD pLibThread)
{
    if (!pLibThread)
        return;

    if (pLibThread->m_pCategoryMgr)
    {
        pLibThread->m_pCategoryMgr->Release();
        pLibThread->m_pCategoryMgr = NULL;
    }
    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }
}
