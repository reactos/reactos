/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Providing <msctfmonitorapi.h> header
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define ILMCM_CHECKLAYOUTANDTIPENABLED 0x1
#define ILMCM_LANGUAGEBAROFF 0x2

EXTERN_C HRESULT WINAPI InitLocalMsCtfMonitor(DWORD dwFlags);
EXTERN_C HRESULT WINAPI UninitLocalMsCtfMonitor(VOID);
