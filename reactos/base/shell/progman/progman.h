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

#ifndef PROGMAN_H
#define PROGMAN_H

#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windef.h>
#include <commdlg.h>
#include <shellapi.h>


#define MAX_STRING_LEN      255
#define MAX_PATHNAME_LEN    1024
#define MAX_LANGUAGE_NUMBER (PM_LAST_LANGUAGE - PM_FIRST_LANGUAGE)


#include "resource.h"

/* Fallback icon */
#define DEFAULTICON OIC_WINLOGO

/* Icon index in MS Windows' progman.exe  */
#define PROGMAN_ICON_INDEX 0
#define GROUP_ICON_INDEX   6
#define DEFAULT_ICON_INDEX 7

#define DEF_GROUP_WIN_XPOS   100
#define DEF_GROUP_WIN_YPOS   100
#define DEF_GROUP_WIN_WIDTH  300
#define DEF_GROUP_WIN_HEIGHT 200

typedef struct
{
  HLOCAL   hGroup;
  HLOCAL   hPrior;
  HLOCAL   hNext;
  HWND     hWnd;
  /**/              /* Numbers are byte indexes in *.grp */

  /**/                       /* Program entry */
  INT      x, y;               /*  0 -  3 */
  INT      nIconIndex;         /*  4 -  5 */
  HICON    hIcon;
  /* icon flags ??? */         /*  6 -  7 */
  /* iconANDsize */            /*  8 -  9 */
  /* iconXORsize */            /* 10 - 11 */
  /* pointer to IconInfo    */ /* 12 - 13 */
  /* pointer to iconXORbits */ /* 14 - 15 */ /* sometimes iconANDbits ?! */
  /* pointer to iconANDbits */ /* 16 - 17 */ /* sometimes iconXORbits ?! */
  HLOCAL   hName;              /* 18 - 19 */
  HLOCAL   hCmdLine;           /* 20 - 21 */
  HLOCAL   hIconFile;          /* 22 - 23 */
  HLOCAL   hWorkDir;           /* Extension 0x8101 */
  INT      nHotKey;            /* Extension 0x8102 */
  /* Modifier: bit 8... */
  INT      nCmdShow;           /* Extension 0x8103 */

  /**/                         /* IconInfo */
  /* HotSpot x   ??? */        /*  0 -  1 */
  /* HotSpot y   ??? */        /*  2 -  3 */
  /* Width           */        /*  4 -  5 */
  /* Height          */        /*  6 -  7 */
  /* WidthBytes  ??? */        /*  8 -  9 */
  /* Planes          */        /* 10 - 10 */
  /* BitsPerPixel    */        /* 11 - 11 */
} PROGRAM;

typedef struct
{
  HLOCAL   hPrior;
  HLOCAL   hNext;
  HWND     hWnd;
  HLOCAL   hGrpFile;
  HLOCAL   hActiveProgram;
  BOOL     bFileNameModified;
  BOOL     bOverwriteFileOk;
  INT      seqnum;

  /**/                         /* Absolute */
  /* magic `PMCC'  */          /*  0 -  3 */
  /* checksum      */          /*  4 -  5 */
  /* Extension ptr */          /*  6 -  7 */
  INT      nCmdShow;           /*  8 -  9 */
  INT      x, y;               /* 10 - 13 */
  INT      width, height;      /* 14 - 17 */
  INT      iconx, icony;       /* 18 - 21 */
  HLOCAL   hName;              /* 22 - 23 */
  /* unknown */                /* 24 - 31 */
  /* number of programs */     /* 32 - 33 */
  HLOCAL   hPrograms;          /* 34 ...  */

  /**/                        /* Extensions */
  /* Extension type */         /*  0 -  1 */
  /* Program number */         /*  2 -  3 */
  /* Size of entry  */         /*  4 -  5 */
  /* Data           */         /*  6 ...  */

  /* magic `PMCC' */           /* Extension 0x8000 */
  /* End of Extensions */      /* Extension 0xffff */
} PROGGROUP;

typedef struct
{
  HANDLE  hInstance;
  HANDLE  hAccel;
  HWND    hMainWnd;
  HWND    hMDIWnd;
  HICON   hMainIcon;
  HICON   hGroupIcon;
  HICON   hDefaultIcon;
  HMENU   hMainMenu;
  HMENU   hFileMenu;
  HMENU   hOptionMenu;
  HMENU   hWindowsMenu;
  HMENU   hLanguageMenu;
  LPCSTR  lpszIniFile;
  BOOL    bAutoArrange;
  BOOL    bSaveSettings;
  BOOL    bMinOnRun;
  HLOCAL  hGroups;
  HLOCAL  hActiveGroup;
} GLOBALS;

extern GLOBALS Globals;

INT  MAIN_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type);
INT  MAIN_MessageBoxIDS_s(UINT ids_text_s, LPCSTR str, UINT ids_title, WORD type);
VOID MAIN_ReplaceString(HLOCAL *handle, LPSTR replacestring);

HLOCAL GRPFILE_ReadGroupFile(const char* path);
BOOL   GRPFILE_WriteGroupFile(HLOCAL hGroup);

ATOM   GROUP_RegisterGroupWinClass(void);
HLOCAL GROUP_AddGroup(LPCSTR lpszName, LPCSTR lpszGrpFile, INT nCmdShow,
		      INT x, INT y, INT width, INT height,
		      INT iconx, INT icony,
		      BOOL bFileNameModified, BOOL bOverwriteFileOk,
		      /* FIXME shouldn't be necessary */
		      BOOL bSuppressShowWindow);
VOID   GROUP_NewGroup(void);
VOID   GROUP_ModifyGroup(HLOCAL hGroup);
VOID   GROUP_DeleteGroup(HLOCAL hGroup);
/* FIXME shouldn't be necessary */
VOID   GROUP_ShowGroupWindow(HLOCAL hGroup);
HLOCAL GROUP_FirstGroup(void);
HLOCAL GROUP_NextGroup(HLOCAL hGroup);
HLOCAL GROUP_ActiveGroup(void);
HWND   GROUP_GroupWnd(HLOCAL hGroup);
LPCSTR GROUP_GroupName(HLOCAL hGroup);

ATOM   PROGRAM_RegisterProgramWinClass(void);
HLOCAL PROGRAM_AddProgram(HLOCAL hGroup, HICON hIcon, LPCSTR lpszName,
			  INT x, INT y, LPCSTR lpszCmdLine,
			  LPCSTR lpszIconFile, INT nIconIndex,
			  LPCSTR lpszWorkDir, INT nHotKey, INT nCmdShow);
VOID   PROGRAM_NewProgram(HLOCAL hGroup);
VOID   PROGRAM_ModifyProgram(HLOCAL hProgram);
VOID   PROGRAM_CopyMoveProgram(HLOCAL hProgram, BOOL bMove);
VOID   PROGRAM_DeleteProgram(HLOCAL hProgram, BOOL BUpdateGrpFile);
HLOCAL PROGRAM_FirstProgram(HLOCAL hGroup);
HLOCAL PROGRAM_NextProgram(HLOCAL hProgram);
HLOCAL PROGRAM_ActiveProgram(HLOCAL hGroup);
LPCSTR PROGRAM_ProgramName(HLOCAL hProgram);
VOID   PROGRAM_ExecuteProgram(HLOCAL hLocal);

INT    DIALOG_New(INT nDefault);
HLOCAL DIALOG_CopyMove(LPCSTR lpszProgramName, LPCSTR lpszGroupName, BOOL bMove);
BOOL   DIALOG_Delete(UINT ids_format_s, LPCSTR lpszName);
BOOL   DIALOG_GroupAttributes(LPSTR lpszTitle, LPSTR lpszPath, INT nSize);
BOOL   DIALOG_ProgramAttributes(LPSTR lpszTitle, LPSTR lpszCmdLine,
				LPSTR lpszWorkDir, LPSTR lpszIconFile,
				HICON *lphIcon, INT *nIconIndex,
				INT *lpnHotKey, INT *lpnCmdShow, INT nSize);
VOID   DIALOG_Execute(void);

VOID   STRING_LoadMenus(VOID);

/* Class names */
extern WCHAR STRING_MAIN_WIN_CLASS_NAME[];
extern WCHAR STRING_MDI_WIN_CLASS_NAME[];
extern WCHAR STRING_GROUP_WIN_CLASS_NAME[];
extern WCHAR STRING_PROGRAM_WIN_CLASS_NAME[];

#endif /* PROGMAN_H */
