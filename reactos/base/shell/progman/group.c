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
 *           GROUP_GroupWndProc
 */

static LRESULT CALLBACK GROUP_GroupWndProc(HWND hWnd, UINT msg,
				   WPARAM wParam, LPARAM lParam)
{
  switch (msg)
    {
    case WM_SYSCOMMAND:
      if (wParam == SC_CLOSE) wParam = SC_MINIMIZE;
      break;

    case WM_CHILDACTIVATE:
    case WM_NCLBUTTONDOWN:
      Globals.hActiveGroup = (HLOCAL)GetWindowLongPtrW(hWnd, 0);
      EnableMenuItem(Globals.hFileMenu, PM_MOVE , MF_GRAYED);
      EnableMenuItem(Globals.hFileMenu, PM_COPY , MF_GRAYED);
      break;
    }
  return DefMDIChildProcW(hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           GROUP_RegisterGroupWinClass
 */

ATOM GROUP_RegisterGroupWinClass(void)
{
  WNDCLASSW class;

  class.style         = CS_HREDRAW | CS_VREDRAW;
  class.lpfnWndProc   = GROUP_GroupWndProc;
  class.cbClsExtra    = 0;
  class.cbWndExtra    = sizeof(LONG_PTR);
  class.hInstance     = Globals.hInstance;
  class.hIcon         = LoadIconW(0, (LPWSTR)IDI_WINLOGO);
  class.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
  class.hbrBackground = GetStockObject (WHITE_BRUSH);
  class.lpszMenuName  = 0;
  class.lpszClassName = STRING_GROUP_WIN_CLASS_NAME;

  return RegisterClassW(&class);
}

/***********************************************************************
 *
 *           GROUP_NewGroup
 */

VOID GROUP_NewGroup(void)
{
  CHAR szName[MAX_PATHNAME_LEN] = "";
  CHAR szFile[MAX_PATHNAME_LEN] = "";
  OFSTRUCT dummy;

  if (!DIALOG_GroupAttributes(szName, szFile, MAX_PATHNAME_LEN)) return;

  if (OpenFile(szFile, &dummy, OF_EXIST) == HFILE_ERROR)
    {
      /* File doesn't exist */
      HLOCAL hGroup =
	GROUP_AddGroup(szName, szFile, SW_SHOWNORMAL,
		       DEF_GROUP_WIN_XPOS, DEF_GROUP_WIN_YPOS,
		       DEF_GROUP_WIN_WIDTH, DEF_GROUP_WIN_HEIGHT, 0, 0,
		       FALSE, FALSE, FALSE);
      if (!hGroup) return;
      GRPFILE_WriteGroupFile(hGroup);
    }
  else /* File exist */
    GRPFILE_ReadGroupFile(szFile);

  /* FIXME Update progman.ini */
}

/***********************************************************************
 *
 *           GROUP_AddGroup
 */

HLOCAL GROUP_AddGroup(LPCSTR lpszName, LPCSTR lpszGrpFile, INT nCmdShow,
		      INT x, INT y, INT width, INT height,
		      INT iconx, INT icony,
		      BOOL bFileNameModified, BOOL bOverwriteFileOk,
		      /* FIXME shouldn't be necessary */
		      BOOL bSuppressShowWindow)
{
  PROGGROUP *group, *prior;
  MDICREATESTRUCTW cs;
  INT    seqnum;
  HLOCAL hPrior, *p;
  HLOCAL hGroup   = LocalAlloc(LMEM_FIXED, sizeof(PROGGROUP));
  HLOCAL hName    = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszName));
  HLOCAL hGrpFile = LocalAlloc(LMEM_FIXED, 1 + strlen(lpszGrpFile));
  if (!hGroup || !hName || !hGrpFile)
    {
      MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
      if (hGroup)   LocalFree(hGroup);
      if (hName)    LocalFree(hName);
      if (hGrpFile) LocalFree(hGrpFile);
      return(0);
    }
  memcpy(LocalLock(hName), lpszName, 1 + strlen(lpszName));
  memcpy(LocalLock(hGrpFile), lpszGrpFile, 1 + strlen(lpszGrpFile));

  Globals.hActiveGroup   = hGroup;

  seqnum = 1;
  hPrior = 0;
  p = &Globals.hGroups;
  while (*p)
    {
      hPrior = *p;
      prior  = LocalLock(hPrior);
      p      = &prior->hNext;
      if (prior->seqnum >= seqnum)
	seqnum = prior->seqnum + 1;
    }
  *p = hGroup;

  group = LocalLock(hGroup);
  group->hPrior    = hPrior;
  group->hNext     = 0;
  group->hName     = hName;
  group->hGrpFile  = hGrpFile;
  group->bFileNameModified = bFileNameModified;
  group->bOverwriteFileOk  = bOverwriteFileOk;
  group->seqnum    = seqnum;
  group->nCmdShow  = nCmdShow;
  group->x         = x;
  group->y         = y;
  group->width     = width;
  group->height    = height;
  group->iconx     = iconx;
  group->icony     = icony;
  group->hPrograms = 0;
  group->hActiveProgram = 0;

  cs.szClass = STRING_GROUP_WIN_CLASS_NAME;
  cs.szTitle = NULL;
  cs.hOwner  = 0;
  cs.x       = x;
  cs.y       = y;
  cs.cx      = width;
  cs.cy      = height;
  cs.style   = 0;
  cs.lParam  = 0;

  group->hWnd = (HWND)SendMessageW(Globals.hMDIWnd, WM_MDICREATE, 0, (LPARAM)&cs);
  SetWindowTextA( group->hWnd, lpszName );
  SetWindowLongPtrW(group->hWnd, 0, (LONG_PTR) hGroup);

#if 1
  if (!bSuppressShowWindow) /* FIXME shouldn't be necessary */
#endif
    {
      ShowWindow (group->hWnd, nCmdShow);
      UpdateWindow (group->hWnd);
    }

  return(hGroup);
}

/***********************************************************************
 *
 *           GROUP_ModifyGroup
 */

VOID GROUP_ModifyGroup(HLOCAL hGroup)
{
  PROGGROUP *group = LocalLock(hGroup);
  CHAR szName[MAX_PATHNAME_LEN];
  CHAR szFile[MAX_PATHNAME_LEN];
  lstrcpynA(szName, LocalLock(group->hName), MAX_PATHNAME_LEN);
  lstrcpynA(szFile, LocalLock(group->hGrpFile), MAX_PATHNAME_LEN);

  if (!DIALOG_GroupAttributes(szName, szFile, MAX_PATHNAME_LEN)) return;

  if (strcmp(szFile, LocalLock(group->hGrpFile)))
    group->bOverwriteFileOk = FALSE;

  MAIN_ReplaceString(&group->hName,    szName);
  MAIN_ReplaceString(&group->hGrpFile, szFile);

  GRPFILE_WriteGroupFile(hGroup);

  /* FIXME Delete old GrpFile if GrpFile changed */

  /* FIXME Update progman.ini */

  SetWindowTextA(group->hWnd, szName);
}

/***********************************************************************
 *
 *           GROUP_ShowGroupWindow
 */

/* FIXME shouldn't be necessary */
VOID GROUP_ShowGroupWindow(HLOCAL hGroup)
{
  PROGGROUP *group = LocalLock(hGroup);
  ShowWindow (group->hWnd, group->nCmdShow);
  UpdateWindow (group->hWnd);
}

/***********************************************************************
 *
 *           GROUP_DeleteGroup
 */

VOID GROUP_DeleteGroup(HLOCAL hGroup)
{
  PROGGROUP *group = LocalLock(hGroup);

  Globals.hActiveGroup = 0;

  if (group->hPrior)
    ((PROGGROUP*)LocalLock(group->hPrior))->hNext = group->hNext;
  else Globals.hGroups = group->hNext;

  if (group->hNext)
    ((PROGGROUP*)LocalLock(group->hNext))->hPrior = group->hPrior;

  while (group->hPrograms)
    PROGRAM_DeleteProgram(group->hPrograms, FALSE);

  /* FIXME Update progman.ini */

  SendMessageW(Globals.hMDIWnd, WM_MDIDESTROY, (WPARAM)group->hWnd, 0);

  LocalFree(group->hName);
  LocalFree(group->hGrpFile);
  LocalFree(hGroup);
}

/***********************************************************************
 *
 *           GROUP_FirstGroup
 */

HLOCAL GROUP_FirstGroup(void)
{
  return(Globals.hGroups);
}

/***********************************************************************
 *
 *           GROUP_NextGroup
 */

HLOCAL GROUP_NextGroup(HLOCAL hGroup)
{
  PROGGROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  return(group->hNext);
}

/***********************************************************************
 *
 *           GROUP_ActiveGroup
 */

HLOCAL GROUP_ActiveGroup(void)
{
  return(Globals.hActiveGroup);
}

/***********************************************************************
 *
 *           GROUP_GroupWnd
 */

HWND GROUP_GroupWnd(HLOCAL hGroup)
{
  PROGGROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  return(group->hWnd);
}

/***********************************************************************
 *
 *           GROUP_GroupName
 */

LPCSTR GROUP_GroupName(HLOCAL hGroup)
{
  PROGGROUP *group;
  if (!hGroup) return(0);
  group = LocalLock(hGroup);
  return(LocalLock(group->hName));
}
