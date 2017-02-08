/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/cliputils.h
 * PURPOSE:         Clipboard helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

void
RetrieveClipboardFormatName(HINSTANCE hInstance,
                            UINT uFormat,
                            BOOL Unicode,
                            PVOID lpszFormat,
                            UINT cch);

void DeleteClipboardContent(void);
UINT GetAutomaticClipboardFormat(void);
BOOL IsClipboardFormatSupported(UINT uFormat);
