#ifndef CONSOLE_H__
#define CONSOLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>

#include <wincon.h>
#include <wingdi.h>
#include <winnls.h>
#include <winreg.h>

#include <winuser.h>
#include <commctrl.h>
#include <cpl.h>

#include <strsafe.h>

#include "resource.h"

#define EnableDlgItem(hDlg, nID, bEnable)   \
    EnableWindow(GetDlgItem((hDlg), (nID)), (bEnable))

/* Shared header with the GUI Terminal Front-End from consrv.dll */
#include "concfg.h" // in /winsrv/concfg/

typedef enum _TEXT_TYPE
{
    Screen,
    Popup
} TEXT_TYPE;

typedef struct _FONT_PREVIEW
{
    HFONT hFont;
    UINT  CharWidth;
    UINT  CharHeight;
} FONT_PREVIEW;

/* Globals */
extern HINSTANCE hApplet;
extern PCONSOLE_STATE_INFO ConInfo;
extern FONT_PREVIEW FontPreview;

VOID ApplyConsoleInfo(HWND hwndDlg);


VOID
RefreshFontPreview(
    IN FONT_PREVIEW* Preview,
    IN PCONSOLE_STATE_INFO pConInfo);

VOID
UpdateFontPreview(
    IN FONT_PREVIEW* Preview,
    IN HFONT hFont,
    IN UINT  CharWidth,
    IN UINT  CharHeight);

#define ResetFontPreview(Preview)   \
    UpdateFontPreview((Preview), NULL, 0, 0)


/* Preview Windows */
BOOL
RegisterWinPrevClass(
    IN HINSTANCE hInstance);

BOOL
UnRegisterWinPrevClass(
    IN HINSTANCE hInstance);


VOID
PaintText(
    IN LPDRAWITEMSTRUCT drawItem,
    IN PCONSOLE_STATE_INFO pConInfo,
    IN TEXT_TYPE TextMode);


struct _LIST_CTL;

typedef INT (*PLIST_GETCOUNT)(IN struct _LIST_CTL* ListCtl);
typedef ULONG_PTR (*PLIST_GETDATA)(IN struct _LIST_CTL* ListCtl, IN INT Index);

typedef struct _LIST_CTL
{
    HWND hWndList;
    PLIST_GETCOUNT GetCount;
    PLIST_GETDATA  GetData;
} LIST_CTL, *PLIST_CTL;

UINT
BisectListSortedByValueEx(
    IN PLIST_CTL ListCtl,
    IN ULONG_PTR Value,
    IN UINT itemStart,
    IN UINT itemEnd,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft);

UINT
BisectListSortedByValue(
    IN PLIST_CTL ListCtl,
    IN ULONG_PTR Value,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft);

#endif /* CONSOLE_H__ */
