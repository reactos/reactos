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

#include "progman.h"

/***********************************************************************
 *
 *           PROGRAM_ProgramWndProc
 */

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

/***********************************************************************
 *
 *           PROGRAM_RegisterProgramWinClass
 */

ATOM PROGRAM_RegisterProgramWinClass(void)
{
  WNDCLASSW class;

  class.style         = CS_HREDRAW | CS_VREDRAW;
  class.lpfnWndProc   = PROGRAM_ProgramWndProc;
  class.cbClsExtra    = 0;
  class.cbWndExtra    = sizeof(LONG_PTR);
  class.hInstance     = Globals.hInstance;
  class.hIcon         = 0;
  class.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
  class.hbrBackground = GetStockObject (WHITE_BRUSH);
  class.lpszMenuName  = 0;
  class.lpszClassName = STRING_PROGRAM_WIN_CLASS_NAME;

  return RegisterClassW(&class);
}

/***********************************************************************
 *
 *           PROGRAM_NewProgram
 */

VOID PROGRAM_NewProgram(HLOCAL hGroup)
{
  INT  nCmdShow = SW_SHOWNORMAL;
  INT  nHotKey = 0;
  INT  nIconIndex = 0;
  CHAR szName[MAX_PATHNAME_LEN] = "";
  CHAR szCmdLine[MAX_PATHNAME_LEN] = "";
  CHAR szIconFile[MAX_PATHNAME_LEN] = "";
  CHAR szWorkDir[MAX_PATHNAME_LEN] = "";
  HICON hIcon = 0;

  if (!DIALOG_ProgramAttributes(szName, szCmdLine, szWorkDir, szIconFile,
				&hIcon, &nIconIndex, &nHotKey,
				&nCmdShow, MAX_PATHNAME_LEN))
    return;

  if (!hIcon) hIcon = LoadIconW(0, (LPWSTR)IDI_WINLOGO);


  if (!PROGRAM_AddProgram(hGroup, hIcon, szName, 0, 0, szCmdLine, szIconFile,
			  nIconIndex, szWorkDir, nHotKey, nCmdShow))
    return;

  GRPFILE_WriteGroupFile(hGroup);
}

/***********************************************************************
 *
 *           PROGRAM_ModifyProgram
 */

VOID PROGRAM_ModifyProgram(HLOCAL hProgram)
{
  PROGRAM *program = LocalLock(hProgram);
  CHAR szName[MAX_PATHNAME_LEN];
  CHAR szCmdLine[MAX_PATHNAME_LEN];
  CHAR szIconFile[MAX_PATHNAME_LEN];
  CHAR szWorkDir[MAX_PATHNAME_LEN];

  lstrcpynA(szName, LocalLock(program->hName), MAX_PATHNAME_LEN);
  lstrcpynA(szCmdLine, LocalLock(program->hCmdLine), MAX_PATHNAME_LEN);
  lstrcpynA(szIconFile, LocalLock(program->hIconFile), MAX_PATHNAME_LEN);
  lstrcpynA(szWorkDir, LocalLock(program->hWorkDir), MAX_PATHNAME_LEN);

  if (!DIALOG_ProgramAttributes(szName, szCmdLine, szWorkDir, szIconFile,
				&program->hIcon, &program->nIconIndex,
				&program->nHotKey, &program->nCmdShow,
				MAX_PATHNAME_LEN))
    return;

  MAIN_ReplaceString(&program->hName, szName);
  MAIN_ReplaceString(&program->hCmdLine, szCmdLine);
  MAIN_ReplaceString(&program->hIconFile, szIconFile);
  MAIN_ReplaceString(&program->hWorkDir, szWorkDir);

  SetWindowTextA(program->hWnd, szName);
  UpdateWindow(program->hWnd);

  GRPFILE_WriteGroupFile(program->hGroup);

  return;
}

/***********************************************************************
 *
 *           PROGRAM_AddProgram
 */

HLOCAL PROGRAM_AddProgram(HLOCAL hGroup, HICON hIcon, LPCSTR lpszName,
			  INT x, INT y, LPCSTR lpszCmdLine,
			  LPCSTR lpszIconFile, INT nIconIndex,
			  LPCSTR lpszWorkDir, INT nHotKey, INT nCmdShow)
{
  PROGGROUP *group = LocalLock(hGroup);
  PROGRAM *program;
  HLOCAL hPrior, *p;
  HLOCAL hProgram  = LocalAlloc(LMEM_FIXED, sizeof(PROGRAM));
  HLOCAL hName     = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszName));
  HLOCAL hCmdLine  = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszCmdLine));
  HLOCAL hIconFile = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszIconFile));
  HLOCAL hWorkDir  = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszWorkDir));
  if (!hProgram || !hName || !hCmdLine || !hIconFile || !hWorkDir)
    {
      MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
      if (hProgram)  LocalFree(hProgram);
      if (hName)     LocalFree(hName);
      if (hCmdLine)  LocalFree(hCmdLine);
      if (hIconFile) LocalFree(hIconFile);
      if (hWorkDir)  LocalFree(hWorkDir);
      return(0);
    }
  memcpy(LocalLock(hName),     lpszName,     1 + strlen(lpszName));
  memcpy(LocalLock(hCmdLine),  lpszCmdLine,  1 + strlen(lpszCmdLine));
  memcpy(LocalLock(hIconFile), lpszIconFile, 1 + strlen(lpszIconFile));
  memcpy(LocalLock(hWorkDir),  lpszWorkDir,  1 + strlen(lpszWorkDir));

  group->hActiveProgram  = hProgram;

  hPrior = 0;
  p = &group->hPrograms;
  while (*p)
    {
      hPrior = *p;
      p = &((PROGRAM*)LocalLock(hPrior))->hNext;
    }
  *p = hProgram;

  program = LocalLock(hProgram);
  program->hGroup     = hGroup;
  program->hPrior     = hPrior;
  program->hNext      = 0;
  program->hName      = hName;
  program->hCmdLine   = hCmdLine;
  program->hIconFile  = hIconFile;
  program->nIconIndex = nIconIndex;
  program->hWorkDir   = hWorkDir;
  program->hIcon      = hIcon;
  program->nCmdShow   = nCmdShow;
  program->nHotKey    = nHotKey;

  program->hWnd =
    CreateWindowW(STRING_PROGRAM_WIN_CLASS_NAME, NULL,
		  WS_CHILD | WS_CAPTION,
		  x, y, CW_USEDEFAULT, CW_USEDEFAULT,
		  group->hWnd, 0, Globals.hInstance, 0);

  SetWindowTextA(program->hWnd, lpszName);
  SetWindowLongPtrW(program->hWnd, 0, (LONG_PTR) hProgram);

  ShowWindow (program->hWnd, SW_SHOWMINIMIZED);
  SetWindowPos (program->hWnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
  UpdateWindow (program->hWnd);

  return hProgram;
}

/***********************************************************************
 *
 *           PROGRAM_CopyMoveProgram
 */

VOID PROGRAM_CopyMoveProgram(HLOCAL hProgram, BOOL bMove)
{
  PROGRAM *program = LocalLock(hProgram);
  PROGGROUP   *fromgroup = LocalLock(program->hGroup);
  HLOCAL hGroup = DIALOG_CopyMove(LocalLock(program->hName),
				  LocalLock(fromgroup->hName), bMove);
  if (!hGroup) return;

  /* FIXME shouldn't be necessary */
  OpenIcon(((PROGGROUP*)LocalLock(hGroup))->hWnd);

  if (!PROGRAM_AddProgram(hGroup,
#if 0
			  CopyIcon(program->hIcon),
#else
			  program->hIcon,
#endif
			  LocalLock(program->hName),
			  program->x, program->y,
			  LocalLock(program->hCmdLine),
			  LocalLock(program->hIconFile),
			  program->nIconIndex,
			  LocalLock(program->hWorkDir),
			  program->nHotKey, program->nCmdShow)) return;
  GRPFILE_WriteGroupFile(hGroup);

  if (bMove) PROGRAM_DeleteProgram(hProgram, TRUE);
}

/***********************************************************************
 *
 *           PROGRAM_ExecuteProgram
 */

VOID PROGRAM_ExecuteProgram(HLOCAL hProgram)
{
  PROGRAM *program = LocalLock(hProgram);
  LPSTR lpszCmdLine = LocalLock(program->hCmdLine);

  /* FIXME set working directory from program->hWorkDir */

  WinExec(lpszCmdLine, program->nCmdShow);
  if (Globals.bMinOnRun) CloseWindow(Globals.hMainWnd);
}

/***********************************************************************
 *
 *           PROGRAM_DeleteProgram
 */

VOID PROGRAM_DeleteProgram(HLOCAL hProgram, BOOL bUpdateGrpFile)
{
  PROGRAM *program = LocalLock(hProgram);
  PROGGROUP   *group   = LocalLock(program->hGroup);

  group->hActiveProgram = 0;

  if (program->hPrior)
    ((PROGRAM*)LocalLock(program->hPrior))->hNext = program->hNext;
  else
    ((PROGGROUP*)LocalLock(program->hGroup))->hPrograms = program->hNext;

  if (program->hNext)
    ((PROGRAM*)LocalLock(program->hNext))->hPrior = program->hPrior;

  if (bUpdateGrpFile)
    GRPFILE_WriteGroupFile(program->hGroup);

  DestroyWindow(program->hWnd);
#if 0
  if (program->hIcon)
    DestroyIcon(program->hIcon);
#endif
  LocalFree(program->hName);
  LocalFree(program->hCmdLine);
  LocalFree(program->hIconFile);
  LocalFree(program->hWorkDir);
  LocalFree(hProgram);
}

/***********************************************************************
 *
 *           PROGRAM_FirstProgram
 */

HLOCAL PROGRAM_FirstProgram(HLOCAL hGroup)
{
  PROGGROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  return(group->hPrograms);
}

/***********************************************************************
 *
 *           PROGRAM_NextProgram
 */

HLOCAL PROGRAM_NextProgram(HLOCAL hProgram)
{
  PROGRAM *program;
  if (!hProgram) return(0);
  program = LocalLock(hProgram);
  return(program->hNext);
}

/***********************************************************************
 *
 *           PROGRAM_ActiveProgram
 */

HLOCAL PROGRAM_ActiveProgram(HLOCAL hGroup)
{
  PROGGROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  if (IsIconic(group->hWnd)) return(0);

  return(group->hActiveProgram);
}

/***********************************************************************
 *
 *           PROGRAM_ProgramName
 */

LPCSTR PROGRAM_ProgramName(HLOCAL hProgram)
{
  PROGRAM *program;
  if (!hProgram) return(0);
  program = LocalLock(hProgram);
  return(LocalLock(program->hName));
}
