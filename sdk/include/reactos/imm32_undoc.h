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
typedef struct tagCLIENTIMC {
    HIMC   hImc;
    HANDLE hInputContext;
    BOOL   bWide;
    DWORD  dwFlags;
    RTL_CRITICAL_SECTION cs;
} CLIENTIMC, *PCLIENTIMC;

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);

#ifdef __cplusplus
} // extern "C"
#endif
