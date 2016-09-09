#ifndef _MAIN_H
#define _MAIN_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <devguid.h>
#include <shlobj.h>
#include <cpl.h>
#include <regstr.h>

#include "resource.h"

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

//typedef LONG (CALLBACK *APPLET_PROC)(VOID);

typedef struct _APPLET
{
  UINT idIcon;
  UINT idName;
  UINT idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

#define MAX_CPL_PAGES   32

/* keyboard.c */

LONG APIENTRY
KeyboardApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

/* main.c */

BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc);

BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam);

/* mouse.c */

LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

#endif /* _MAIN_H */
