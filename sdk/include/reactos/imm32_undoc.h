/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Private header for imm32.dll
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define IS_IME_KBDLAYOUT(hKL) ((HIWORD(hKL) & 0xF000) == 0xE000)

BOOL WINAPI
ImmGetImeInfoEx(PIMEINFOEX pImeInfoEx, IMEINFOEXCLASS SearchType, PVOID pvSearchKey);
