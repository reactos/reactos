#ifndef _USRMGR_H
#define _USRMGR_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

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

/* groupprops.c */
BOOL
GroupProperties(HWND hwndDlg);

/* misc.c */
VOID
DebugPrintf(LPTSTR szFormat, ...);

BOOL
CheckAccountName(HWND hwndDlg,
                 INT nIdDlgItem,
                 LPTSTR lpAccountName);

/* userprops.c */
BOOL
UserProperties(HWND hwndDlg);

#endif /* _USRMGR_H */
