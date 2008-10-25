#ifndef __CPL_TELEPHON_H
#define __CPL_TELEPHON_H

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


extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

#endif /* __CPL_TELEPHON_H */

/* EOF */
