/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGFILE.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  File import and export user interface routines for the Registry Editor.
*
*******************************************************************************/

#ifndef _INC_REGFILE
#define _INC_REGFILE

VOID
PASCAL
RegEdit_ImportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    LPTSTR lpFileName
    );

VOID
PASCAL
RegEdit_OnDropFiles(
    HWND hWnd,
    HDROP hDrop
    );

VOID
PASCAL
RegEdit_OnCommandImportRegFile(
    HWND hWnd
    );

VOID
PASCAL
RegEdit_ExportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    BOOL fUseDownlevelFormat,
    LPTSTR lpFileName,
    LPTSTR lpSelectedPath
    );

VOID
PASCAL
RegEdit_OnCommandExportRegFile(
    HWND hWnd
    );

#endif // _INC_REGFILE
