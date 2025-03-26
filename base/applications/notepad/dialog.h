/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 */

#pragma once

VOID DIALOG_FileNew(VOID);
VOID DIALOG_FileNewWindow(VOID);
VOID DIALOG_FileOpen(VOID);
BOOL DIALOG_FileSave(VOID);
BOOL DIALOG_FileSaveAs(VOID);
VOID DIALOG_FilePrint(VOID);
VOID DIALOG_FilePageSetup(VOID);
VOID DIALOG_FileExit(VOID);

VOID DIALOG_EditUndo(VOID);
VOID DIALOG_EditCut(VOID);
VOID DIALOG_EditCopy(VOID);
VOID DIALOG_EditPaste(VOID);
VOID DIALOG_EditDelete(VOID);
VOID DIALOG_EditSelectAll(VOID);
VOID DIALOG_EditTimeDate(VOID);
VOID DIALOG_EditWrap(VOID);

VOID DIALOG_Search(VOID);
VOID DIALOG_SearchNext(BOOL bDown);
VOID DIALOG_Replace(VOID);
VOID DIALOG_GoTo(VOID);

VOID DIALOG_SelectFont(VOID);

VOID DIALOG_ViewStatusBar(VOID);
VOID DIALOG_StatusBarAlignParts(VOID);
VOID DIALOG_StatusBarUpdateCaretPos(VOID);

VOID DIALOG_HelpContents(VOID);
VOID DIALOG_HelpSearch(VOID);
VOID DIALOG_HelpLicense(VOID);
VOID DIALOG_HelpNoWarranty(VOID);
VOID DIALOG_HelpAboutNotepad(VOID);

VOID DIALOG_TimeDate(VOID);

int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCTSTR szString, DWORD dwFlags);

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/* utility functions */
VOID ShowLastError(VOID);
BOOL FileExists(LPCTSTR szFilename);
BOOL HasFileExtension(LPCTSTR szFilename);
BOOL DoCloseFile(VOID);
VOID DoOpenFile(LPCTSTR szFileName);
VOID DoShowHideStatusBar(VOID);
VOID DoCreateEditWindow(VOID);
void UpdateWindowCaption(BOOL clearModifyAlert);
VOID WaitCursor(BOOL bBegin);
