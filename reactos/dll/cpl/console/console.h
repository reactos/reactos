#ifndef CONSOLE_H__
#define CONSOLE_H__

#define WIN32_NO_STATUS
#include <limits.h> // just for UINT_MAX in layout.c
#include <tchar.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"

/* Shared header with the GUI Terminal Front-End from consrv.dll */
#include "consolecpl.h"

typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

BOOL ApplyConsoleInfo(HWND hwndDlg, PCONSOLE_PROPS pConInfo);
VOID PaintConsole(LPDRAWITEMSTRUCT drawItem, PCONSOLE_PROPS pConInfo);
VOID PaintText(LPDRAWITEMSTRUCT drawItem, PCONSOLE_PROPS pConInfo);

// Globals
extern HINSTANCE hApplet;

#endif /* CONSOLE_H__ */
