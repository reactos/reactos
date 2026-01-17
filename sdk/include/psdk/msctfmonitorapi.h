/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Definitions for the MS CTF monitor
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define ILMCM_CHECKLAYOUTANDTIPENABLED 0x1
#define ILMCM_LANGUAGEBAROFF 0x2

EXTERN_C HRESULT WINAPI InitLocalMsCtfMonitor(_In_ DWORD dwFlags);
EXTERN_C HRESULT WINAPI UninitLocalMsCtfMonitor(VOID);

#define DCM_FLAGS_TASKENG 0x1
#define DCM_FLAGS_CTFMON 0x2
#define DCM_FLAGS_LOCALTHREADTSF 0x4

EXTERN_C BOOL WINAPI DoMsCtfMonitor(_In_ DWORD dwFlags, _In_ HANDLE hEventForServiceStop);
