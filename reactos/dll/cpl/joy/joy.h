#ifndef __CPL_JOY_H
#define __CPL_JOY_H

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

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
}CREATE_LINK_CONTEXT, *PCREATE_LINK_CONTEXT;


extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_JOY_H */

/* EOF */
