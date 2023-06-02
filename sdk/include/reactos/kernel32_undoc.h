/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Private header for kernel32.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

BOOL
WINAPI
DECLSPEC_HOTPATCH
RegisterConsoleIME(
    _In_ HWND hWnd,
    _Out_opt_ LPDWORD pdwAttachToThreadId);

BOOL
WINAPI
DECLSPEC_HOTPATCH
UnregisterConsoleIME(VOID);
