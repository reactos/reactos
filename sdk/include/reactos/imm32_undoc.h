/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define KBDLAYOUT_MASK 0xF000
#define KBDLAYOUT_IME 0xE000
#define IS_IME_KBDLAYOUT(hKL) ((HIWORD(hKL) & KBDLAYOUT_MASK) == KBDLAYOUT_IME)

/* unconfirmed */
typedef struct tagCLIENTIMC
{
    HIMC hImc;                  // offset 0x0
    LONG cLockObj;              // offset 0x4
    DWORD dwFlags;              // offset 0x8
    DWORD unknown;              // offset 0xc
    RTL_CRITICAL_SECTION cs;    // offset 0x10
    DWORD unknown2[3];          // offset 0x28
} CLIENTIMC, *PCLIENTIMC;       // size 0x34

/* flags for CLIENTIMC */
#define CLIENTIMC_WIDE (1 << 0)
#define CLIENTIMC_DISABLED (1 << 6)
#define CLIENTIMC_UNKNOWN2 (1 << 8)

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);

#ifdef __cplusplus
} // extern "C"
#endif
