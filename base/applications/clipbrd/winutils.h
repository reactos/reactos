/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Display helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#pragma once

void ShowLastWin32Error(HWND hwndParent);
void BringWindowToFront(HWND hWnd);
int MessageBoxRes(HWND hWnd, HINSTANCE hInstance, UINT uText, UINT uCaption, UINT uType);
void DrawTextFromResource(HINSTANCE hInstance, UINT uID, HDC hDC, LPRECT lpRect, UINT uFormat);
void DrawTextFromClipboard(UINT uFormat, PAINTSTRUCT ps, SCROLLSTATE state);
void BitBltFromClipboard(PAINTSTRUCT ps, SCROLLSTATE state, DWORD dwRop);
void SetDIBitsToDeviceFromClipboard(UINT uFormat, PAINTSTRUCT ps, SCROLLSTATE state, UINT fuColorUse);
void PlayMetaFileFromClipboard(HDC hdc, const RECT *lpRect);
void PlayEnhMetaFileFromClipboard(HDC hdc, const RECT *lpRect);
BOOL RealizeClipboardPalette(HDC hdc);
