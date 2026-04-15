/*
 * PROJECT:     ReactOS User API Server DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Defining FakeDrag* functions and removing shell32 import
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

UINT FakeDragQueryFileA(HDROP hDrop, UINT lFile, LPSTR lpszFile, UINT lLength);
UINT FakeDragQueryFileW(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength);
VOID FakeDragAcceptFiles(HWND hWnd, BOOL fAccept);

#define FakeDragFinish(hDrop) GlobalFree(hDrop)
