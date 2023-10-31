/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);

BOOL WINAPI ImmLoadLayout(HKL hKL, PIMEINFOEX pImeInfoEx);
PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc);
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc);
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL);
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi);
HRESULT WINAPI CtfImmTIMActivate(HKL hKL);

HRESULT WINAPI CtfAImmActivate(_Out_opt_ HINSTANCE *phinstCtfIme);
HRESULT WINAPI CtfAImmDeactivate(_In_ BOOL bDestroy);

#ifdef __cplusplus
} // extern "C"
#endif
