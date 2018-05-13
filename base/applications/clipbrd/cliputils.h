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

SIZE_T
GetLineExtentW(
    IN LPCWSTR lpText,
    OUT LPCWSTR* lpNextLine);

SIZE_T
GetLineExtentA(
    IN LPCSTR lpText,
    OUT LPCSTR* lpNextLine);

BOOL GetClipboardDataDimensions(UINT uFormat, PRECT pRc);
