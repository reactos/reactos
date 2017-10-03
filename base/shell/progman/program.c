/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
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
 * FILE:            base/shell/progman/program.c
 * PURPOSE:         Program items helper functions
 * PROGRAMMERS:     Ulrich Schmid
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "progman.h"

#if 0

static LRESULT CALLBACK PROGRAM_ProgramWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_NCLBUTTONDOWN:
      {
	HLOCAL  hProgram = (HLOCAL) GetWindowLongPtrW(hWnd, 0);
	PROGRAM *program = LocalLock(hProgram);
	PROGGROUP   *group   = LocalLock(program->hGroup);
	group->hActiveProgram = hProgram;
	EnableMenuItem(Globals.hFileMenu, PM_MOVE , MF_ENABLED);
	EnableMenuItem(Globals.hFileMenu, PM_COPY , MF_ENABLED);
	break;
      }
    case WM_NCLBUTTONDBLCLK:
      {
	PROGRAM_ExecuteProgram((HLOCAL) GetWindowLongPtrW(hWnd, 0));
	return(0);
      }

    case WM_PAINTICON:
    case WM_NCPAINT:
      {
	PROGRAM *program;
	PAINTSTRUCT      ps;
	HDC              hdc;
	hdc     = BeginPaint(hWnd,&ps);
	program = LocalLock((HLOCAL) GetWindowLongPtrW(hWnd, 0));
	if (program->hIcon)
	  DrawIcon(hdc, 0, 0, program->hIcon);
	EndPaint(hWnd,&ps);
	break;
      }
    }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

#endif



/***********************************************************************
 *
 *           PROGRAM_NewProgram
 */

VOID PROGRAM_NewProgram(PROGGROUP* hGroup)
{
    HICON hIcon      = NULL;
    INT   nIconIndex = 0;
    INT   nHotKey    = 0;
    INT   nCmdShow   = SW_SHOWNORMAL;
    BOOL  bNewVDM    = FALSE;
    WCHAR szTitle[MAX_PATHNAME_LEN]    = L"";
    WCHAR szCmdLine[MAX_PATHNAME_LEN]  = L"";
    WCHAR szIconFile[MAX_PATHNAME_LEN] = L"";
    WCHAR szWorkDir[MAX_PATHNAME_LEN]  = L"";

    if (!DIALOG_ProgramAttributes(szTitle, szCmdLine, szWorkDir, szIconFile,
                                  &hIcon, &nIconIndex, &nHotKey, &nCmdShow, &bNewVDM,
                                  MAX_PATHNAME_LEN))
    {
        return;
    }

    if (!hIcon)
        hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_WINLOGO));

    if (!PROGRAM_AddProgram(hGroup, hIcon, szTitle, -1, -1, szCmdLine, szIconFile,
                            nIconIndex, szWorkDir, nHotKey, nCmdShow, bNewVDM))
    {
        return;
    }

    GRPFILE_WriteGroupFile(hGroup);
}

/***********************************************************************
 *
 *           PROGRAM_ModifyProgram
 */

VOID PROGRAM_ModifyProgram(PROGRAM* hProgram)
{
    LVITEMW lvItem;
    WCHAR szName[MAX_PATHNAME_LEN];
    WCHAR szWorkDir[MAX_PATHNAME_LEN];
    WCHAR szCmdLine[MAX_PATHNAME_LEN];
    WCHAR szIconFile[MAX_PATHNAME_LEN];

    lstrcpynW(szName    , hProgram->hName    , ARRAYSIZE(szName));
    lstrcpynW(szCmdLine , hProgram->hCmdLine , ARRAYSIZE(szCmdLine));
    lstrcpynW(szIconFile, hProgram->hIconFile, ARRAYSIZE(szIconFile));
    lstrcpynW(szWorkDir , hProgram->hWorkDir , ARRAYSIZE(szWorkDir));

    if (!DIALOG_ProgramAttributes(szName, szCmdLine, szWorkDir, szIconFile,
                                  &hProgram->hIcon, &hProgram->nIconIndex,
                                  &hProgram->nHotKey, &hProgram->nCmdShow,
                                  &hProgram->bNewVDM, MAX_PATHNAME_LEN))
    {
        return;
    }

    MAIN_ReplaceString(&hProgram->hName    , szName);
    MAIN_ReplaceString(&hProgram->hCmdLine , szCmdLine);
    MAIN_ReplaceString(&hProgram->hIconFile, szIconFile);
    MAIN_ReplaceString(&hProgram->hWorkDir , szWorkDir);

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask     = LVIF_TEXT;
    lvItem.iSubItem = 0;
    lvItem.pszText  = szName;
    SendMessageW(hProgram->hGroup->hListView, LVM_SETITEMTEXTW, hProgram->iItem, (LPARAM)&lvItem);

    GRPFILE_WriteGroupFile(hProgram->hGroup);
}

/***********************************************************************
 *
 *           PROGRAM_AddProgram
 */

PROGRAM*
PROGRAM_AddProgram(PROGGROUP* hGroup, HICON hIcon, LPCWSTR lpszName,
                   INT x, INT y, LPCWSTR lpszCmdLine, LPCWSTR lpszIconFile, INT nIconIndex,
                   LPCWSTR lpszWorkDir, INT nHotKey, INT nCmdShow, BOOL bNewVDM)
{
    PROGRAM* hProgram;
    PROGRAM* hPrior;
    PROGRAM** p;
    LPWSTR hCmdLine;
    LPWSTR hIconFile;
    LPWSTR hName;
    LPWSTR hWorkDir;
    LVITEMW lvItem;
    INT iItem;

    hProgram  = Alloc(HEAP_ZERO_MEMORY, sizeof(*hProgram));
    hName     = Alloc(HEAP_ZERO_MEMORY, (wcslen(lpszName)     + 1) * sizeof(WCHAR));
    hCmdLine  = Alloc(HEAP_ZERO_MEMORY, (wcslen(lpszCmdLine)  + 1) * sizeof(WCHAR));
    hIconFile = Alloc(HEAP_ZERO_MEMORY, (wcslen(lpszIconFile) + 1) * sizeof(WCHAR));
    hWorkDir  = Alloc(HEAP_ZERO_MEMORY, (wcslen(lpszWorkDir)  + 1) * sizeof(WCHAR));
    if (!hProgram || !hName || !hCmdLine || !hIconFile || !hWorkDir)
    {
        MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
        if (hProgram)  Free(hProgram);
        if (hName)     Free(hName);
        if (hCmdLine)  Free(hCmdLine);
        if (hIconFile) Free(hIconFile);
        if (hWorkDir)  Free(hWorkDir);
        return NULL;
    }
    memcpy(hName    , lpszName    , (wcslen(lpszName)     + 1) * sizeof(WCHAR));
    memcpy(hCmdLine , lpszCmdLine , (wcslen(lpszCmdLine)  + 1) * sizeof(WCHAR));
    memcpy(hIconFile, lpszIconFile, (wcslen(lpszIconFile) + 1) * sizeof(WCHAR));
    memcpy(hWorkDir , lpszWorkDir , (wcslen(lpszWorkDir)  + 1) * sizeof(WCHAR));

    hGroup->hActiveProgram = hProgram;

    hPrior = NULL;
    for (p = &hGroup->hPrograms; *p; p = &hPrior->hNext)
        hPrior = *p;
    *p = hProgram;

    hProgram->hGroup     = hGroup;
    hProgram->hPrior     = hPrior;
    hProgram->hNext      = NULL;
    hProgram->hName      = hName;
    hProgram->hCmdLine   = hCmdLine;
    hProgram->hIconFile  = hIconFile;
    hProgram->nIconIndex = nIconIndex;
    hProgram->hWorkDir   = hWorkDir;
    hProgram->hIcon      = hIcon;
    hProgram->nCmdShow   = nCmdShow;
    hProgram->nHotKey    = nHotKey;
    hProgram->bNewVDM    = bNewVDM;
    hProgram->TagsSize   = 0;
    hProgram->Tags       = NULL;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask    = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    lvItem.pszText = (LPWSTR)lpszName;
    lvItem.lParam  = (LPARAM)hProgram;
    lvItem.iImage  = ImageList_ReplaceIcon(hGroup->hListLarge, -1, hIcon);
    DestroyIcon(hIcon);

    lvItem.iItem = SendMessageA(hGroup->hListView, LVM_GETITEMCOUNT, 0, 0);
    iItem = SendMessageW(hGroup->hListView, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
    hProgram->iItem = iItem;
    if (x != -1 && y != -1)
        SendMessageA(hGroup->hListView, LVM_SETITEMPOSITION, lvItem.iItem, MAKELPARAM(x, y));

    return hProgram;
}



/***********************************************************************
 *
 *           PROGRAM_CopyMoveProgram
 */

VOID PROGRAM_CopyMoveProgram(PROGRAM* hProgram, BOOL bMove)
{
    PROGGROUP* hGroup;

    hGroup = DIALOG_CopyMove(hProgram, bMove);
    if (!hGroup)
        return;

    /* FIXME: shouldn't be necessary */
    OpenIcon(hGroup->hWnd);

    if (!PROGRAM_AddProgram(hGroup,
                            hProgram->hIcon,
                            hProgram->hName,
                            hProgram->x,
                            hProgram->y,
                            hProgram->hCmdLine,
                            hProgram->hIconFile,
                            hProgram->nIconIndex,
                            hProgram->hWorkDir,
                            hProgram->nHotKey,
                            hProgram->nCmdShow,
                            hProgram->bNewVDM))
    {
        return;
    }

    GRPFILE_WriteGroupFile(hGroup);

    if (bMove)
        PROGRAM_DeleteProgram(hProgram, TRUE);
}

/***********************************************************************
 *
 *           PROGRAM_ExecuteProgram
 */

VOID PROGRAM_ExecuteProgram(PROGRAM* hProgram)
{
    // TODO: Use a (private?) shell API with which one can use hProgram->bNewVDM

    ShellExecuteW(NULL, NULL, hProgram->hCmdLine, NULL, hProgram->hWorkDir, hProgram->nCmdShow);

    if (Globals.bMinOnRun)
        CloseWindow(Globals.hMainWnd);
}

/***********************************************************************
 *
 *           PROGRAM_DeleteProgram
 */

VOID PROGRAM_DeleteProgram(PROGRAM* hProgram, BOOL bUpdateGrpFile)
{
    PROGGROUP* group;

    group = hProgram->hGroup;
    if (hProgram->hGroup->hActiveProgram == hProgram)
        group->hActiveProgram = NULL;

    SendMessageA(group->hListView, LVM_DELETEITEM, hProgram->iItem, 0);

    if (hProgram->hPrior)
        hProgram->hPrior->hNext = hProgram->hNext;
    else
        hProgram->hGroup->hPrograms = hProgram->hNext;

    if (hProgram->hNext)
        hProgram->hNext->hPrior = hProgram->hPrior;

    if (bUpdateGrpFile)
        GRPFILE_WriteGroupFile(hProgram->hGroup);

#if 0
    DestroyWindow(program->hWnd);
    if (program->hIcon)
        DestroyIcon(program->hIcon);
#endif

    if (hProgram->Tags)
        Free(hProgram->Tags);
    Free(hProgram->hName);
    Free(hProgram->hCmdLine);
    Free(hProgram->hIconFile);
    Free(hProgram->hWorkDir);
    Free(hProgram);
}


/***********************************************************************
 *
 *           PROGRAM_ActiveProgram
 */

PROGRAM* PROGRAM_ActiveProgram(PROGGROUP* hGroup)
{
    if (!hGroup) return NULL;
    if (IsIconic(hGroup->hWnd)) return NULL;
    return hGroup->hActiveProgram;
}
