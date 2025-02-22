/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/progman.h
 * PURPOSE:         ProgMan header
 * PROGRAMMERS:     Ulrich Schmid
 *                  Sylvain Petreolle
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef PROGMAN_H
#define PROGMAN_H

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>

#include <shellapi.h>

#include <commctrl.h>
#include <richedit.h>

#include <windowsx.h>
#include <strsafe.h>

#define MAX_STRING_LEN      255
#define MAX_PATHNAME_LEN    1024
#define MAX_LANGUAGE_NUMBER (PM_LAST_LANGUAGE - PM_FIRST_LANGUAGE)

#include "resource.h"

/* Fallback icon */
#define DEFAULTICON OIC_WINLOGO

#define DEF_GROUP_WIN_XPOS   100
#define DEF_GROUP_WIN_YPOS   100
#define DEF_GROUP_WIN_WIDTH  300
#define DEF_GROUP_WIN_HEIGHT 200


/*
 * windowsx.h extensions
 */
#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))





typedef struct _PROGRAM PROGRAM, *PPROGRAM;
typedef struct _PROGGROUP PROGGROUP, *PPROGGROUP;

struct _PROGRAM
{
    PROGGROUP* hGroup;
    PROGRAM*   hPrior;
    PROGRAM*   hNext;
    HWND       hWnd;

    INT        iItem;
    INT        x;
    INT        y;
    INT        nIconIndex;
    HICON      hIcon;
    LPWSTR     hName;
    LPWSTR     hCmdLine;
    LPWSTR     hIconFile;
    LPWSTR     hWorkDir;    /* Extension 0x8101 */
    INT        nHotKey;     /* Extension 0x8102 */
    INT        nCmdShow;    /* Extension 0x8103 */
    BOOL       bNewVDM;     /* Extension 0x8104 */

    SIZE_T     TagsSize;
    PVOID      Tags;
}; // PROGRAM, *PPROGRAM;

typedef enum _GROUPFORMAT
{
    Win_311    = 0x0,
    NT_Ansi    = 0x1, // 0x02
    NT_Unicode = 0x2, // 0x03
} GROUPFORMAT;

struct _PROGGROUP
{
    PROGGROUP* hPrior;
    PROGGROUP* hNext;
    HWND       hWnd;

    HWND       hListView;
    HIMAGELIST hListLarge;
    HIMAGELIST hDragImageList;
    HICON      hOldCursor;
    POINT      ptStart;
    BOOL       bDragging;

    GROUPFORMAT format;
    BOOL     bIsCommonGroup;
    // BOOL     bFileNameModified;
    BOOL     bOverwriteFileOk;
    LPWSTR   hGrpFile;
    INT      seqnum;
    INT      nCmdShow;
    INT      x;
    INT      y;
    INT      width;
    INT      height;
    INT      iconx;
    INT      icony;
    LPWSTR   hName;
    PROGRAM* hPrograms;
    PROGRAM* hActiveProgram;

    SIZE_T   TagsSize;
    PVOID    Tags;
}; // PROGGROUP, *PPROGGROUP;


typedef struct _GLOBALS
{
    HINSTANCE hInstance;
    HACCEL    hAccel;
    HWND      hMainWnd;
    HWND      hMDIWnd;
    HICON     hDefaultIcon;
    HICON     hMainIcon;
    // HICON     hGroupIcon;
    HICON     hPersonalGroupIcon;
    HICON     hCommonGroupIcon;
    HMENU     hMainMenu;
    HMENU     hFileMenu;
    HMENU     hOptionMenu;
    HMENU     hWindowsMenu;
    HMENU     hLanguageMenu;

    HKEY      hKeyProgMan;
    HKEY      hKeyPMSettings;
    HKEY      hKeyPMCommonGroups;
    HKEY      hKeyPMAnsiGroups;
    HKEY      hKeyPMUnicodeGroups;
    HKEY      hKeyAnsiGroups;
    HKEY      hKeyUnicodeGroups;
    HKEY      hKeyCommonGroups;

    BOOL      bAutoArrange;
    BOOL      bSaveSettings;
    BOOL      bMinOnRun;
    PROGGROUP* hGroups;
    PROGGROUP* hActiveGroup;
    // int field_74;
    // int field_78;
    // PROGGROUP* field_79;
} GLOBALS, *PGLOBALS;

extern GLOBALS Globals;
extern WCHAR szTitle[256];


/*
 * Memory management functions
 */
PVOID
Alloc(IN DWORD  dwFlags,
      IN SIZE_T dwBytes);

BOOL
Free(IN PVOID lpMem);

PVOID
ReAlloc(IN DWORD  dwFlags,
        IN PVOID  lpMem,
        IN SIZE_T dwBytes);

PVOID
AppendToBuffer(IN PVOID   pBuffer,
               IN PSIZE_T pdwBufferSize,
               IN PVOID   pData,
               IN SIZE_T  dwDataSize);


INT  MAIN_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type);
INT  MAIN_MessageBoxIDS_s(UINT ids_text, LPCWSTR str, UINT ids_title, WORD type);
VOID MAIN_ReplaceString(LPWSTR* string, LPWSTR replace);

DWORD GRPFILE_ReadGroupFile(LPCWSTR lpszPath, BOOL bIsCommonGroup);
BOOL GRPFILE_WriteGroupFile(PROGGROUP* hGroup);

ATOM GROUP_RegisterGroupWinClass(VOID);
PROGGROUP* GROUP_AddGroup(GROUPFORMAT format, BOOL bIsCommonGroup, LPCWSTR lpszName, LPCWSTR lpszGrpFile,
                          INT left, INT top, INT right, INT bottom, INT xMin, INT yMin, INT nCmdShow,
                          WORD cxIcon, WORD cyIcon, BOOL bOverwriteFileOk,
                          /* FIXME shouldn't be necessary */
                          BOOL bSuppressShowWindow);
VOID GROUP_NewGroup(GROUPFORMAT format, BOOL bIsCommonGroup);
VOID GROUP_ModifyGroup(PROGGROUP* hGroup);
VOID GROUP_DeleteGroup(PROGGROUP* hGroup);
/* FIXME shouldn't be necessary */
VOID GROUP_ShowGroupWindow(PROGGROUP* hGroup);
PROGGROUP* GROUP_ActiveGroup(VOID);

PROGRAM* PROGRAM_AddProgram(PROGGROUP* hGroup, HICON hIcon, LPCWSTR lpszName,
                            INT x, INT y, LPCWSTR lpszCmdLine, LPCWSTR lpszIconFile, INT nIconIndex,
                            LPCWSTR lpszWorkDir, INT nHotKey, INT nCmdShow, BOOL bNewVDM);
VOID PROGRAM_NewProgram(PROGGROUP* hGroup);
VOID PROGRAM_ModifyProgram(PROGRAM* hProgram);
VOID PROGRAM_CopyMoveProgram(PROGRAM* hProgram, BOOL bMove);
VOID PROGRAM_DeleteProgram(PROGRAM* hProgram, BOOL bUpdateGrpFile);
VOID PROGRAM_ExecuteProgram(PROGRAM* hProgram);
PROGRAM* PROGRAM_ActiveProgram(PROGGROUP* hGroup);

BOOL DIALOG_New(INT nDefault, PINT pnResult);
PROGGROUP* DIALOG_CopyMove(PROGRAM* hProgram, BOOL bMove);
BOOL DIALOG_Delete(UINT ids_text_s, LPCWSTR lpszName);
BOOL DIALOG_GroupAttributes(GROUPFORMAT format, LPWSTR lpszTitle, LPWSTR lpszGrpFile, INT nSize);
BOOL DIALOG_ProgramAttributes(LPWSTR lpszTitle, LPWSTR lpszCmdLine, LPWSTR lpszWorkDir, LPWSTR lpszIconFile,
                              HICON* lphIcon, INT* lpnIconIndex, INT* lpnHotKey, INT* lpnCmdShow, BOOL* lpbNewVDM, INT nSize);
VOID DIALOG_Execute(VOID);

VOID STRING_LoadStrings(VOID);
VOID STRING_LoadMenus(VOID);

/* Class names */
#define STRING_MAIN_WIN_CLASS_NAME  L"PMMain"
#define STRING_GROUP_WIN_CLASS_NAME L"PMGroup"

#endif /* PROGMAN_H */
