/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

/* unconfirmed */
typedef struct tagCLIENTIMC
{
    HIMC hImc;
    LONG cLockObj;
    DWORD dwFlags;
    DWORD unknown;
    RTL_CRITICAL_SECTION cs;
    DWORD unknown2;
    DWORD unknown3;
    BOOL bUnknown4;
} CLIENTIMC, *PCLIENTIMC;

/* flags for CLIENTIMC */
#define CLIENTIMC_WIDE 0x1
#define CLIENTIMC_UNKNOWN1 0x40
#define CLIENTIMC_UNKNOWN2 0x100

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);

PCLIENTIMC WINAPI ImmLockClientImc(HIMC hImc);
VOID WINAPI ImmUnlockClientImc(PCLIENTIMC pClientImc);
PIMEDPI WINAPI ImmLockImeDpi(HKL hKL);
VOID WINAPI ImmUnlockImeDpi(PIMEDPI pImeDpi);

#ifdef __cplusplus
} // extern "C"
#endif
