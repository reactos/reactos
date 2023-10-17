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

typedef enum tagENCODING
{
    ENCODING_ANSI = 0,
    ENCODING_WIDE,
    ENCODING_UTF8,
    ENCODING_HLOCAL_ANSI,
    ENCODING_HLOCAL_WIDE,
} ENCODING;

typedef BOOL (CALLBACK *TEXTPROC)(LPVOID pvText, SIZE_T cbText, ENCODING encoding, BOOL bAlloc);
BOOL CALLBACK DoSetTextMode(LPVOID pvText, SIZE_T cbText, ENCODING encoding, BOOL bAlloc);
BOOL DoTextFromFormat(UINT uFormat, TEXTPROC fnCallback);
