#ifndef CONSOLE_H__
#define CONSOLE_H__

#include <stdio.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"

#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

/* Shared header with the GUI Terminal Front-End from consrv.dll */
#include "consolecpl.h"

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef enum _TEXT_TYPE
{
    Screen,
    Popup
} TEXT_TYPE;

BOOL ApplyConsoleInfo(HWND hwndDlg, PCONSOLE_PROPS pConInfo);
VOID PaintConsole(LPDRAWITEMSTRUCT drawItem, PCONSOLE_PROPS pConInfo);
BOOL PaintText(LPDRAWITEMSTRUCT drawItem, PCONSOLE_PROPS pConInfo, TEXT_TYPE TextMode);

// Globals
extern HINSTANCE hApplet;

#endif /* CONSOLE_H__ */
