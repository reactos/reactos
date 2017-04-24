#ifndef CONSOLE_H__
#define CONSOLE_H__

#include <stdio.h>
#include <wchar.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <commctrl.h>
#include <cpl.h>

#include <strsafe.h>

#include "resource.h"

#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

/* Shared header with the GUI Terminal Front-End from consrv.dll */
#include "settings.h" // in /winsrv/concfg/

typedef enum _TEXT_TYPE
{
    Screen,
    Popup
} TEXT_TYPE;

/* Globals */
extern PCONSOLE_STATE_INFO ConInfo;

VOID ApplyConsoleInfo(HWND hwndDlg);
BYTE CodePageToCharSet(UINT CodePage);
VOID PaintConsole(LPDRAWITEMSTRUCT drawItem, PCONSOLE_STATE_INFO pConInfo);
BOOL PaintText(LPDRAWITEMSTRUCT drawItem, PCONSOLE_STATE_INFO pConInfo, TEXT_TYPE TextMode);

#endif /* CONSOLE_H__ */
