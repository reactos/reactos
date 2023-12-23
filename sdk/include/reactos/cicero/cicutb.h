/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for msutb.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

EXTERN_C LPVOID WINAPI GetLibTls(VOID);
EXTERN_C BOOL WINAPI GetPopupTipbar(HWND hWnd, BOOL fWinLogon);
EXTERN_C HRESULT WINAPI SetRegisterLangBand(BOOL bRegister);
EXTERN_C VOID WINAPI ClosePopupTipbar(VOID);
