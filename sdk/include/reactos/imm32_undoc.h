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

PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc);
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc);
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL);
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi);
HRESULT APIENTRY CtfImmTIMCreateInputContext(HIMC hIMC);
HRESULT APIENTRY CtfImmTIMDestroyInputContext(HIMC hIMC);
HRESULT WINAPI CtfImmTIMActivate(HKL hKL);

#ifdef __cplusplus
} // extern "C"
#endif
