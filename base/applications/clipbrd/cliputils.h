/*
 * PROJECT:     ReactOS Clipboard Viewer
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Clipboard helper functions.
 * COPYRIGHT:   Copyright 2015-2018 Ricardo Hanke
 *              Copyright 2015-2018 Hermes Belusca-Maito
 */

#pragma once

LRESULT
SendClipboardOwnerMessage(
    IN BOOL bUnicode,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);

void
RetrieveClipboardFormatName(HINSTANCE hInstance,
                            UINT uFormat,
                            BOOL Unicode,
                            PVOID lpszFormat,
                            UINT cch);

void DeleteClipboardContent(void);
UINT GetAutomaticClipboardFormat(void);
BOOL IsClipboardFormatSupported(UINT uFormat);
BOOL GetClipboardDataDimensions(UINT uFormat, PRECT pRc);
LPWSTR GetTextFromClipboard(UINT uFormat, BOOL bOpen);

typedef BOOL (CALLBACK *DO_TEXT_PROC)(UINT uFormat, LPCVOID text, SIZE_T cch, BOOL bAnsi,
                                      HDC hDC, LPRECT lpRect);
BOOL DoText(UINT uFormat, DO_TEXT_PROC fnCallback, HDC hDC, LPRECT lpRect);
LPCWSTR FindNewLineW(LPCWSTR pszText, SIZE_T cch);
LPCSTR FindNewLineA(LPCSTR pszText, SIZE_T cch);
void CacheClear(void);
