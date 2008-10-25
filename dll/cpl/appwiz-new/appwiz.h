#ifndef __CPL_APPWIZ_H
#define __CPL_APPWIZ_H

#define COBJMACROS
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <commctrl.h>
#include <cpl.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>
#include <prsht.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <richedit.h>

#include "resource.h"

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct
{
   WCHAR szTarget[MAX_PATH];
   WCHAR szWorkingDirectory[MAX_PATH];
   WCHAR szDescription[MAX_PATH];
   WCHAR szLinkName[MAX_PATH];
} CREATE_LINK_CONTEXT, *PCREATE_LINK_CONTEXT;

typedef struct
{
    DWORD Size;
    DWORD Masks;
    ULONGLONG AppSize;
    FILETIME LastUsed;
    int TimesUsed;
    WCHAR ImagePath[MAX_PATH];
} APPARPINFO;

typedef struct
{
    DWORD Size;
    BOOL Maximized;
    INT Left;
    INT Top;
    INT Right;
    INT Bottom;
} APPWIZSETTINGS;

/* appwiz.c */
HINSTANCE hApplet; // Main applet instance
HWND hMainWnd,     // Main window
     hActList,     // Actions list
     hAppList,     // Programs list
     hSearch,      //Search line
     hRemoveBtn,   // Remove button
     hModifyBtn;   // Modify button

APPWIZSETTINGS AppWizSettings;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_APPWIZ_H */

/* EOF */
