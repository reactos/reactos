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

/* Registered clipboard formats */
extern UINT g_uCFSTR_FILECONTENTS;
extern UINT g_uCFSTR_FILEDESCRIPTOR;
extern UINT g_uCFSTR_FILENAMEA;
extern UINT g_uCFSTR_FILENAMEW;
extern UINT g_uCFSTR_FILENAMEMAP;
extern UINT g_uCFSTR_MOUNTEDVOLUME;
extern UINT g_uCFSTR_SHELLIDLIST;
extern UINT g_uCFSTR_SHELLIDLISTOFFSET;
extern UINT g_uCFSTR_NETRESOURCES;
extern UINT g_uCFSTR_PRINTERGROUP;
extern UINT g_uCFSTR_SHELLURL;
extern UINT g_uCFSTR_INDRAGLOOP;
extern UINT g_uCFSTR_LOGICALPERFORMEDDROPEFFECT;
extern UINT g_uCFSTR_PASTESUCCEEDED;
extern UINT g_uCFSTR_PERFORMEDDROPEFFECT;
extern UINT g_uCFSTR_PREFERREDDROPEFFECT;
extern UINT g_uCFSTR_TARGETCLSID;
