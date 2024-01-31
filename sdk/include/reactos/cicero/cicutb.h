/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for msutb.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

DEFINE_GUID(GUID_COMPARTMENT_SPEECH_OPENCLOSE, 0x544D6A63, 0xE2E8, 0x4752, 0xBB, 0xD1, 0x00, 0x09, 0x60, 0xBC, 0xA0, 0x83);

EXTERN_C LPVOID WINAPI GetLibTls(VOID);
EXTERN_C BOOL WINAPI GetPopupTipbar(HWND hWnd, BOOL fWinLogon);
EXTERN_C HRESULT WINAPI SetRegisterLangBand(BOOL bRegister);
EXTERN_C VOID WINAPI ClosePopupTipbar(VOID);
