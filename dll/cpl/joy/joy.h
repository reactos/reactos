#pragma once

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct
{
    WCHAR szTarget[MAX_PATH];
    WCHAR szWorkingDirectory[MAX_PATH];
    WCHAR szDescription[MAX_PATH];
    WCHAR szLinkName[MAX_PATH];
} CREATE_LINK_CONTEXT, *PCREATE_LINK_CONTEXT;


extern HINSTANCE hApplet;

void ShowLastWin32Error(HWND hWndOwner);

/* EOF */
