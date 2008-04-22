#ifndef __CPL_DESK_H__
#define __CPL_DESK_H__

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cpl.h>
#include <tchar.h>
#include <setupapi.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <stdio.h>

#include "resource.h"

typedef struct _APPLET
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;


INT_PTR CALLBACK UsersPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GroupsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ExtraPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif /* __CPL_DESK_H__ */

