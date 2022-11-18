/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/shutdown.c
 * PURPOSE:         Shutdown Dialog Box (GUI only)
 * PROGRAMMERS:     Lee Schroeder (spaceseel at gmail dot com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "msgina.h"
#include <powrprof.h>
#include <wingdi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>

/* Shutdown state flags */
#define WLX_SHUTDOWN_STATE_LOGOFF       0x01
#define WLX_SHUTDOWN_STATE_POWER_OFF    0x02
#define WLX_SHUTDOWN_STATE_REBOOT       0x04
// 0x08
#define WLX_SHUTDOWN_STATE_SLEEP        0x10
// 0x20
#define WLX_SHUTDOWN_STATE_HIBERNATE    0x40
// 0x80

/* Macros for fancy shut down dialog */
#define FONT_POINT_SIZE                 13

#define DARK_GREY_COLOR                 RGB(244, 244, 244)
#define LIGHT_GREY_COLOR                RGB(38, 38, 38)

/* Bitmap's size for buttons */
#define CX_BITMAP                       33
#define CY_BITMAP                       33

#define NUMBER_OF_BUTTONS               4
#define NUMBER_OF_BUTTON_STATES         10

/* For HTHEME and HBITMAP */
#define DLG_BITMAP_CUSTOMIZABLE         3
#define THEME_COMPATIBLE_CONTROLS       (DLG_BITMAP_CUSTOMIZABLE + 2)

#define DLG_BACKGROUND                  0
#define DLG_FLAG                        1
#define DLG_BUTTON_BITMAP               2
#define DLG_STATIC_TITLE                3
#define DLG_STATIC_BUTTONS              4

/* Static Texts */
#define NUMBER_OF_STATIC_TEXTS          (NUMBER_OF_BUTTONS + 1)

#define STATIC_TURNOFF                  0
#define STATIC_SHUTDOWN                 1
#define STATIC_RESTART                  2
#define STATIC_SLEEP                    3
#define STATIC_HIBERNATE                4

/* After determining the button as well as its state paint the image strip bitmap using these predefined positions */
#define BUTTON_SHUTDOWN                 0
#define BUTTON_SHUTDOWN_PRESSED         cy
#define BUTTON_SHUTDOWN_FOCUSED         (cy * 2)
#define BUTTON_REBOOT                   (cy * 3)
#define BUTTON_REBOOT_PRESSED           (cy * 4)
#define BUTTON_REBOOT_FOCUSED           (cy * 5)
#define BUTTON_SLEEP                    (cy * 6)
#define BUTTON_SLEEP_PRESSED            (cy * 7)
#define BUTTON_SLEEP_FOCUSED            (cy * 8)
#define BUTTON_SLEEP_DISABLED           (cy * 9)

/* For bIsButtonHot */
#define SHUTDOWN_BUTTON_HOT             0
#define REBOOT_BUTTON_HOT               1
#define SLEEP_BUTTON_HOT                2
#define HIBERNATE_BUTTON_HOT            3

typedef HTHEME (WINAPI *MSGINAOPENTHEMEDATA)(HWND, LPCWSTR);
typedef HRESULT (WINAPI *MSGINACLOSETHEMEDATA)(HTHEME);
typedef HRESULT (WINAPI *MSGINADRAWTHEMEBACKGROUND)(HTHEME, HDC, int, int, LPCRECT, LPCRECT);
typedef HRESULT (WINAPI *MSGINADRAWTHEMETEXT)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, DWORD, LPCRECT);
typedef HRESULT (WINAPI *MSGINAGETTHEMEPARTSIZE)(HTHEME, HDC, int, int, LPCRECT, THEMESIZE, LPSIZE);

typedef struct _SHUTDOWN_DLG_CONTEXT
{
    PGINA_CONTEXT pgContext;
    BOOL bCloseDlg;
    BOOL bFriendlyUI;
    BOOL bIsButtonHot[NUMBER_OF_BUTTONS];
    BOOL bIsSleepButtonReplaced;
    BOOL bReasonUI;
    BOOL bTimer;
    HBITMAP hBitmap[DLG_BITMAP_CUSTOMIZABLE];
    DWORD ShutdownDialogId;
    DWORD ShutdownOptions;
    HFONT hfFont;
    HMODULE hUxtheme;
    HTHEME hTheme[THEME_COMPATIBLE_CONTROLS]; // Number of customizable bitmaps + customizable static controls
    UINT_PTR iTimer;
    WCHAR StaticTexts[NUMBER_OF_STATIC_TEXTS][128];
    WNDPROC OldButtonProc;
} SHUTDOWN_DLG_CONTEXT, *PSHUTDOWN_DLG_CONTEXT;

static
BOOL
GetShutdownReasonUI(VOID)
{
    OSVERSIONINFOEX VersionInfo;
    DWORD dwValue, dwSize;
    HKEY hKey;
    LONG lRet;

    /* Query the policy value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"Software\\Policies\\Microsoft\\Windows NT\\Reliability",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        RegQueryValueExW(hKey,
                         L"ShutdownReasonUI",
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize);
        RegCloseKey(hKey);

        return (dwValue != 0) ? TRUE : FALSE;
    }

    /* Query the machine value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Reliability",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        RegQueryValueExW(hKey,
                         L"ShutdownReasonUI",
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize);
        RegCloseKey(hKey);

        return (dwValue != 0) ? TRUE : FALSE;
    }

    /* Return the default value */
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    if (!GetVersionEx((POSVERSIONINFO)&VersionInfo))
        return FALSE;

    return FALSE;
//    return (VersionInfo.wProductType == VER_NT_WORKSTATION) ? FALSE : TRUE;
}

static
BOOL
IsFriendlyUIActive(VOID)
{
    DWORD dwType, dwValue, dwSize;
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SYSTEM\\CurrentControlSet\\Control\\Windows",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        return FALSE;

    /* CORE-17282 First check an optional ReactOS specific override, that Windows does not check.
       We use this to allow users pairing 'Server'-configuration with FriendlyShutdown.
       Otherwise users would have to change CSDVersion or LogonType (side-effects AppCompat) */
    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"EnforceFriendlyShutdown",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);

    if (lRet == ERROR_SUCCESS && dwType == REG_DWORD && dwValue == 0x1)
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    /* Check product version number */
    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"CSDVersion",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS || dwType != REG_DWORD || dwValue != 0x300)
    {
        /* Allow Friendly UI only on Workstation */
        return FALSE;
    }

    /* Check LogonType value */
    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        return FALSE;

    dwValue = 0;
    dwSize = sizeof(dwValue);
    lRet = RegQueryValueExW(hKey,
                            L"LogonType",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize);
    RegCloseKey(hKey);

    if (lRet != ERROR_SUCCESS || dwType != REG_DWORD)
        return FALSE;

    return (dwValue != 0);
}

static
BOOL
IsDomainMember(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

static
BOOL
IsNetwareActive(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

static
BOOL
IsShowHibernateButtonActive(VOID)
{
    INT_PTR lRet;
    HKEY hKey;
    DWORD dwValue, dwSize;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Policies\\Microsoft\\Windows\\System\\Shutdown",
                         0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);

        lRet = RegQueryValueExW(hKey,
                                L"ShowHibernateButton",
                                NULL, NULL,
                                (LPBYTE)&dwValue, &dwSize);
        RegCloseKey(hKey);
        if (lRet != ERROR_SUCCESS)
        {
            return FALSE;
        }
        return (dwValue != 0);
    }
    return FALSE;
}

static
BOOL
ForceFriendlyUI(VOID)
{
    DWORD dwType, dwValue, dwSize;
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        lRet = RegQueryValueExW(hKey,
                                L"ForceFriendlyUI",
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize);
        RegCloseKey(hKey);

        if (lRet == ERROR_SUCCESS && dwType == REG_DWORD)
            return (dwValue != 0);
    }

    lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwValue = 0;
        dwSize = sizeof(dwValue);
        lRet = RegQueryValueExW(hKey,
                                L"ForceFriendlyUI",
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize);

        RegCloseKey(hKey);

        if (lRet == ERROR_SUCCESS && dwType == REG_DWORD)
            return (dwValue != 0);
    }

    return FALSE;
}

static BOOL
DrawDialogBoxBgElements(
    PDRAWITEMSTRUCT pdis,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    MSGINADRAWTHEMEBACKGROUND msDrawThemeBackground = NULL;
    BITMAP bm;
    RECT rect;
    int Part;
    HDC hdcMem;
    HBITMAP hbmOld;

    GetClientRect(pdis->hwndItem, &rect);

    if (pContext->hUxtheme)
    {
        msDrawThemeBackground = (MSGINADRAWTHEMEBACKGROUND)GetProcAddress(pContext->hUxtheme, "DrawThemeBackground");
    }

    switch (pdis->CtlID)
    {
        case IDC_SHUTDOWN_DLG_BG:
            Part = DLG_BACKGROUND;
            break;

        case IDC_SHUTDOWN_DLG_FLAG:
            Part = DLG_FLAG;
            break;
    }

    if (pContext->hTheme[Part] && msDrawThemeBackground)
    {
        msDrawThemeBackground(pContext->hTheme[Part], pdis->hDC, 0, 0, &rect, NULL);
    }
    else
    {
        hdcMem = CreateCompatibleDC(pdis->hDC);
        hbmOld = SelectObject(hdcMem, pContext->hBitmap[Part]);

        GetObject(pContext->hBitmap[Part], sizeof(bm), &bm);

        if (Part == DLG_BACKGROUND)
        {
            StretchBlt(pdis->hDC, 0, 0,
                       rect.right - rect.left,
                       rect.bottom - rect.top,
                       hdcMem, 0, 0, bm.bmWidth,
                       bm.bmHeight, SRCCOPY);
        }
        else if (Part == DLG_FLAG)
        {
            BitBlt(pdis->hDC,
                   0, 0,
                   bm.bmWidth, bm.bmHeight, 
                   hdcMem, 0, 0, SRCCOPY);
        }
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
    }

    return TRUE;
}

static BOOL
DrawDialogBoxStaticTexts(
    PDRAWITEMSTRUCT pdis,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    MSGINADRAWTHEMETEXT msDrawThemeText = NULL;
    RECT rect;
    DWORD dwStyle = DT_CENTER;
    int Part = DLG_STATIC_BUTTONS;
    int StaticPart;

    if (pContext->hUxtheme)
    {
        msDrawThemeText = (MSGINADRAWTHEMETEXT)GetProcAddress(pContext->hUxtheme, "DrawThemeText");
    }
    
    GetClientRect(pdis->hwndItem, &rect);

    switch (pdis->CtlID)
    {
        case IDC_TURN_OFF_STATIC:
            dwStyle = DT_LEFT;
            Part = DLG_STATIC_TITLE;
            StaticPart = STATIC_TURNOFF;
            break;

        case IDC_SHUTDOWN_STATIC:
            StaticPart = STATIC_SHUTDOWN;
            break;

        case IDC_RESTART_STATIC:
            StaticPart = STATIC_RESTART;
            break;

        case IDC_SLEEP_STATIC:
            StaticPart = STATIC_SLEEP;
            break;

        case IDC_HIBERNATE_STATIC:
            StaticPart = STATIC_HIBERNATE;
            break;
    }
    
    if (pContext->hTheme[Part] && msDrawThemeText)
    {
        msDrawThemeText(pContext->hTheme[Part], pdis->hDC, 0, 0, pContext->StaticTexts[StaticPart], -1,  DT_SINGLELINE | dwStyle, 0, &rect);
    }
    else
    {
        DrawText(pdis->hDC, pContext->StaticTexts[StaticPart], -1, &rect, DT_SINGLELINE | dwStyle);
    }

    return TRUE;
}

static
BOOL
DrawIconOnOwnerDrawnButtons(
    PDRAWITEMSTRUCT pdis,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    MSGINADRAWTHEMEBACKGROUND msDrawThemeBackground = NULL;
    MSGINAGETTHEMEPARTSIZE msGetThemePartSize = NULL;
    BITMAP bm;
    RECT rect;
    SIZE size;
    BOOL bRet;
    int y = 0;
    int cx, cy = 0;
    HDC hdcMem;
    HBITMAP hbmOld;

    if (pContext->hUxtheme)
    {
        msDrawThemeBackground = (MSGINADRAWTHEMEBACKGROUND)GetProcAddress(pContext->hUxtheme, "DrawThemeBackground");
        msGetThemePartSize = (MSGINAGETTHEMEPARTSIZE)GetProcAddress(pContext->hUxtheme, "GetThemePartSize");
    }

    rect = pdis->rcItem;

    if (pContext->hTheme[DLG_BUTTON_BITMAP] && msGetThemePartSize)
    {
        msGetThemePartSize(pContext->hTheme[DLG_BUTTON_BITMAP], pdis->hDC, 0, 0, NULL, TS_TRUE, &size);
    }
    else
    {
        GetObject(pContext->hBitmap[DLG_BUTTON_BITMAP], sizeof(bm), &bm);
    }
    
    /* Check the button ID for revelant bitmap to be used */
    switch (pdis->CtlID)
    {
        case IDC_BUTTON_SHUTDOWN:
        {
            switch (pdis->itemAction)
            {
                case ODA_DRAWENTIRE:
                case ODA_FOCUS:
                case ODA_SELECT:
                {
                    y = BUTTON_SHUTDOWN;
                    if (pdis->itemState & ODS_SELECTED)
                    {
                        y = BUTTON_SHUTDOWN_PRESSED;
                    }
                    else if (pContext->bIsButtonHot[SHUTDOWN_BUTTON_HOT] || (pdis->itemState & ODS_FOCUS))
                    {
                        y = BUTTON_SHUTDOWN_FOCUSED;
                    }
                    break;
                }
            }
            break;
        }

        case IDC_BUTTON_REBOOT:
        {
            switch (pdis->itemAction)
            {
                case ODA_DRAWENTIRE:
                case ODA_FOCUS:
                case ODA_SELECT:
                {
                    y = BUTTON_REBOOT;
                    if (pdis->itemState & ODS_SELECTED)
                    {
                        y = BUTTON_REBOOT_PRESSED;
                    }
                    else if (pContext->bIsButtonHot[REBOOT_BUTTON_HOT] || (pdis->itemState & ODS_FOCUS))
                    {
                        y = BUTTON_REBOOT_FOCUSED;
                    }
                    break;
                }
            }
            break;
        }

        case IDC_BUTTON_HIBERNATE:
        case IDC_BUTTON_SLEEP:
        {
            switch (pdis->itemAction)
            {
                case ODA_DRAWENTIRE:
                case ODA_FOCUS:
                case ODA_SELECT:
                {
                    y = BUTTON_SLEEP;
                    if (pdis->itemState & ODS_DISABLED)
                    {
                        y = BUTTON_SLEEP_DISABLED;
                    }
                    else if (pdis->itemState & ODS_SELECTED)
                    {
                        y = BUTTON_SLEEP_PRESSED;
                    }
                    else if ((pdis->CtlID == IDC_BUTTON_SLEEP && pContext->bIsButtonHot[SLEEP_BUTTON_HOT]) ||
                             (pdis->CtlID == IDC_BUTTON_HIBERNATE && pContext->bIsButtonHot[HIBERNATE_BUTTON_HOT]) ||
                             (pdis->itemState & ODS_FOCUS))
                    {
                        y = BUTTON_SLEEP_FOCUSED;
                    }
                    break;
                }
            }
            break;
        }
    }

    if (pContext->hTheme[DLG_BUTTON_BITMAP] && msDrawThemeBackground)
    {
        rect.bottom = size.cy;
        OffsetRect(&rect, 0, -y);

        msDrawThemeBackground(pContext->hTheme[DLG_BUTTON_BITMAP], pdis->hDC, 0, 0, &rect, 0);
        bRet = TRUE;
    }
    else
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight / NUMBER_OF_BUTTON_STATES;

        hdcMem = CreateCompatibleDC(pdis->hDC);
        hbmOld = SelectObject(hdcMem, pContext->hBitmap[DLG_BUTTON_BITMAP]);

        /* Draw it on the required button */
        bRet = BitBlt(pdis->hDC,
                     (rect.right - rect.left - cx) / 2,
                     (rect.bottom - rect.top - cy) / 2,
                     cx, cy, hdcMem, 0, y, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
    }
    return bRet;
}

BOOL
WINAPI
ShellIsFriendlyUIActive(VOID)
{
    BOOL bActive;

    bActive = IsFriendlyUIActive();

    if ((IsDomainMember() || IsNetwareActive()) && !ForceFriendlyUI())
        return FALSE;

    return bActive;
}

DWORD
GetDefaultShutdownSelState(VOID)
{
    return WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
}

DWORD
LoadShutdownSelState(VOID)
{
    LONG lRet;
    HKEY hKeyCurrentUser, hKey;
    DWORD dwValue, dwTemp, dwSize;

    /* Default to shutdown */
    dwValue = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;

    /* Open the current user HKCU key */
    lRet = RegOpenCurrentUser(MAXIMUM_ALLOWED, &hKeyCurrentUser);
    if (lRet == ERROR_SUCCESS)
    {
        /* Open the subkey */
        lRet = RegOpenKeyExW(hKeyCurrentUser,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                             0, KEY_QUERY_VALUE, &hKey);
        RegCloseKey(hKeyCurrentUser);
    }
    if (lRet != ERROR_SUCCESS)
        return dwValue;

    /* Read the value */
    dwSize = sizeof(dwTemp);
    lRet = RegQueryValueExW(hKey,
                            L"Shutdown Setting",
                            NULL, NULL,
                            (LPBYTE)&dwTemp, &dwSize);
    RegCloseKey(hKey);

    if (lRet == ERROR_SUCCESS)
    {
        switch (dwTemp)
        {
            case WLX_SHUTDOWN_STATE_LOGOFF:
                dwValue = WLX_SAS_ACTION_LOGOFF;
                break;

            case WLX_SHUTDOWN_STATE_POWER_OFF:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
                break;

            case WLX_SHUTDOWN_STATE_REBOOT:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_REBOOT;
                break;

            // 0x08

            case WLX_SHUTDOWN_STATE_SLEEP:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_SLEEP;
                break;

            // 0x20

            case WLX_SHUTDOWN_STATE_HIBERNATE:
                dwValue = WLX_SAS_ACTION_SHUTDOWN_HIBERNATE;
                break;

            // 0x80
        }
    }

    return dwValue;
}

VOID
ChangeRequiredButton(
    HWND hDlg,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    int destID = IDC_BUTTON_SLEEP;
    int targetedID = IDC_BUTTON_HIBERNATE;
    HWND hwndDest, hwndTarget;
    RECT rect;

    /* If the sleep button has been already replaced earlier, bring sleep button back to its original position */
    if (pContext->bIsSleepButtonReplaced)
    {
        destID = IDC_BUTTON_HIBERNATE;
        targetedID = IDC_BUTTON_SLEEP;
    }

    hwndDest = GetDlgItem(hDlg, destID);
    hwndTarget = GetDlgItem(hDlg, targetedID);

    /* Get the position of the destination button */
    GetWindowRect(hwndDest, &rect);

    /* Get the corrected translated coordinates which is relative to the client window */  
    MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    /* Set the position of targeted button and hide the destination button */
    SetWindowPos(hwndTarget,
                 HWND_TOP,
                 rect.left, rect.top,
                 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    EnableWindow(hwndDest, FALSE);
    ShowWindow(hwndDest, SW_HIDE);
    EnableWindow(hwndTarget, TRUE);
    ShowWindow(hwndTarget, SW_SHOW);
    SetFocus(hwndTarget);

    GetDlgItemTextW(hDlg, targetedID, pContext->StaticTexts[STATIC_SLEEP], _countof(pContext->StaticTexts[STATIC_SLEEP]));
    InvalidateRect(hDlg, NULL, FALSE);
}

VOID
CreateToolTipForButtons(
    int controlID,
    int detailID,
    HWND hDlg,
    int titleID,
    HINSTANCE hInst)
{
    HWND hwndTool, hwndTip;
    WCHAR szBuffer[256];
    TTTOOLINFOW tool;

    hwndTool = GetDlgItem(hDlg, controlID);

    tool.cbSize = sizeof(tool);
    tool.hwnd = hDlg;
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool.uId = (UINT_PTR)hwndTool;

    /* Create the tooltip */
    hwndTip = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL,
                             WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hDlg, NULL, hInst, NULL);

    /* Associate the tooltip with the tool. */
    LoadStringW(hInst, detailID, szBuffer, _countof(szBuffer));
    tool.lpszText = szBuffer;
    SendMessageW(hwndTip, TTM_ADDTOOLW, 0, (LPARAM)&tool);
    LoadStringW(hInst, titleID, szBuffer, _countof(szBuffer));
    SendMessageW(hwndTip, TTM_SETTITLEW, TTI_NONE, (LPARAM)szBuffer);
    SendMessageW(hwndTip, TTM_SETMAXTIPWIDTH, 0, 250);
}

VOID
EndFriendlyDialog(
    HWND hDlg,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    MSGINACLOSETHEMEDATA msCloseThemeData = NULL;
    int i;

    if (pContext->bTimer)
    {
        KillTimer(hDlg, pContext->iTimer);
    }

    if (pContext->hUxtheme)
    {
        msCloseThemeData = (MSGINACLOSETHEMEDATA)GetProcAddress(pContext->hUxtheme, "CloseThemeData");
        if (!msCloseThemeData)
            return;
    }

    for (i = 0; i < DLG_BITMAP_CUSTOMIZABLE; i++)
    {
        if (pContext->hTheme[i])
        {
            msCloseThemeData(pContext->hTheme[i]);
        }
        else
        {
            DeleteObject(pContext->hBitmap[i]);
        }
    }

    if (pContext->hTheme[DLG_STATIC_TITLE])
    {
        msCloseThemeData(pContext->hTheme[DLG_STATIC_TITLE]);
    }
    else
    {
        DeleteObject(pContext->hfFont);
    }

    if (pContext->hTheme[DLG_STATIC_BUTTONS])
    {
        msCloseThemeData(pContext->hTheme[DLG_STATIC_BUTTONS]);
    }

    /* Remove the subclass from the buttons */
    for (i = 0; i < NUMBER_OF_BUTTONS; i++)
    {
        SetWindowLongPtrW(GetDlgItem(hDlg, IDC_BUTTON_SHUTDOWN + i), GWLP_WNDPROC, (LONG_PTR)pContext->OldButtonProc);
    }

    /* Unload uxtheme module */
    FreeLibrary(pContext->hUxtheme);
}

VOID OnTimer(
    HWND hDlg,
    PSHUTDOWN_DLG_CONTEXT pContext)
{
    BOOL ReplaceButton = !!(GetKeyState(VK_SHIFT) & 0x8000);

    if (ReplaceButton && !pContext->bIsSleepButtonReplaced)
    {
        ChangeRequiredButton(hDlg, pContext);
        pContext->bIsSleepButtonReplaced = TRUE;
    }
    else if (!ReplaceButton && pContext->bIsSleepButtonReplaced)
    {
        ChangeRequiredButton(hDlg, pContext);
        pContext->bIsSleepButtonReplaced = FALSE;
    }
}

VOID
SaveShutdownSelState(
    IN DWORD ShutdownCode)
{
    LONG lRet;
    HKEY hKeyCurrentUser, hKey;
    DWORD dwValue = 0;

    /* Open the current user HKCU key */
    lRet = RegOpenCurrentUser(MAXIMUM_ALLOWED, &hKeyCurrentUser);
    if (lRet == ERROR_SUCCESS)
    {
        /* Create the subkey */
        lRet = RegCreateKeyExW(hKeyCurrentUser,
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                               0, NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_SET_VALUE,
                               NULL, &hKey, NULL);
        RegCloseKey(hKeyCurrentUser);
    }
    if (lRet != ERROR_SUCCESS)
        return;

    switch (ShutdownCode)
    {
        case WLX_SAS_ACTION_LOGOFF:
            dwValue = WLX_SHUTDOWN_STATE_LOGOFF;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            dwValue = WLX_SHUTDOWN_STATE_POWER_OFF;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
            dwValue = WLX_SHUTDOWN_STATE_REBOOT;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
            dwValue = WLX_SHUTDOWN_STATE_SLEEP;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
            dwValue = WLX_SHUTDOWN_STATE_HIBERNATE;
            break;
    }

    RegSetValueExW(hKey,
                   L"Shutdown Setting",
                   0, REG_DWORD,
                   (LPBYTE)&dwValue, sizeof(dwValue));
    RegCloseKey(hKey);
}

DWORD
GetDefaultShutdownOptions(VOID)
{
    return WLX_SHUTDOWN_STATE_POWER_OFF | WLX_SHUTDOWN_STATE_REBOOT;
}

DWORD
GetAllowedShutdownOptions(VOID)
{
    DWORD Options = 0;

    // FIXME: Compute those options accordings to current user's rights!
    Options |= WLX_SHUTDOWN_STATE_LOGOFF | WLX_SHUTDOWN_STATE_POWER_OFF | WLX_SHUTDOWN_STATE_REBOOT;

    if (IsPwrSuspendAllowed())
        Options |= WLX_SHUTDOWN_STATE_SLEEP;

    if (IsPwrHibernateAllowed())
        Options |= WLX_SHUTDOWN_STATE_HIBERNATE;

    return Options;
}

static VOID
UpdateShutdownDesc(
    IN HWND hDlg,
    IN PSHUTDOWN_DLG_CONTEXT pContext) // HINSTANCE hInstance
{
    UINT DescId = 0;
    DWORD ShutdownCode;
    WCHAR szBuffer[256];

    ShutdownCode = SendDlgItemMessageW(hDlg, IDC_SHUTDOWN_ACTION, CB_GETCURSEL, 0, 0);
    if (ShutdownCode == CB_ERR) // Invalid selection
        return;

    ShutdownCode = SendDlgItemMessageW(hDlg, IDC_SHUTDOWN_ACTION, CB_GETITEMDATA, ShutdownCode, 0);

    switch (ShutdownCode)
    {
        case WLX_SAS_ACTION_LOGOFF:
            DescId = IDS_SHUTDOWN_LOGOFF_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            DescId = IDS_SHUTDOWN_SHUTDOWN_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
            DescId = IDS_SHUTDOWN_RESTART_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
            DescId = IDS_SHUTDOWN_SLEEP_DESC;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
            DescId = IDS_SHUTDOWN_HIBERNATE_DESC;
            break;

        default:
            break;
    }

    LoadStringW(pContext->pgContext->hDllInstance, DescId, szBuffer, _countof(szBuffer));
    SetDlgItemTextW(hDlg, IDC_SHUTDOWN_DESCRIPTION, szBuffer);

    if (pContext->bReasonUI)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_PLANNED), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_LIST), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
        EnableWindow(GetDlgItem(hDlg, IDC_REASON_COMMENT), (ShutdownCode != WLX_SAS_ACTION_LOGOFF));
    }
}

static INT_PTR
CALLBACK
OwnerDrawButtonSubclass(
    HWND hButton,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSHUTDOWN_DLG_CONTEXT pContext;
    pContext = (PSHUTDOWN_DLG_CONTEXT)GetWindowLongPtrW(GetParent(hButton), GWLP_USERDATA);
    int buttonID = GetDlgCtrlID(hButton);

    switch (uMsg)
    {
        case WM_MOUSEMOVE:
        {
            HWND hwndTarget;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

            if (GetCapture() != hButton)
            {
                SetCapture(hButton);

                switch (buttonID)
                {
                    case IDC_BUTTON_SHUTDOWN:
                    {
                        pContext->bIsButtonHot[SHUTDOWN_BUTTON_HOT] = TRUE;
                        break;
                    }
                    case IDC_BUTTON_REBOOT:
                    {
                        pContext->bIsButtonHot[REBOOT_BUTTON_HOT] = TRUE;
                        break;
                    }
                    case IDC_BUTTON_SLEEP:
                    {
                        pContext->bIsButtonHot[SLEEP_BUTTON_HOT] = TRUE;
                        break;
                    }
                    case IDC_BUTTON_HIBERNATE:
                    {
                        pContext->bIsButtonHot[HIBERNATE_BUTTON_HOT] = TRUE;
                        break;
                    }
                }
                SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_HAND)));
            }

            ClientToScreen(hButton, &pt);
            hwndTarget = WindowFromPoint(pt);

            if (hwndTarget != hButton)
            {
                ReleaseCapture();

                switch (buttonID)
                {
                    case IDC_BUTTON_SHUTDOWN:
                    {
                        pContext->bIsButtonHot[SHUTDOWN_BUTTON_HOT] = FALSE;
                        break;
                    }
                    case IDC_BUTTON_REBOOT:
                    {
                        pContext->bIsButtonHot[REBOOT_BUTTON_HOT] = FALSE;
                        break;
                    }
                    case IDC_BUTTON_SLEEP:
                    {
                        pContext->bIsButtonHot[SLEEP_BUTTON_HOT] = FALSE;
                        break;
                    }
                    case IDC_BUTTON_HIBERNATE:
                    {
                        pContext->bIsButtonHot[HIBERNATE_BUTTON_HOT] = FALSE;
                        break;
                    }
                }
            }
            InvalidateRect(hButton, NULL, FALSE);
            break;
        }

        /* Whenever one of the buttons gets the keyboard focus, set it as default button */
        case WM_SETFOCUS:
        {
            SendMessageW(GetParent(hButton), DM_SETDEFID, buttonID, 0);
            break;
        }

        /* Otherwise, set IDCANCEL as default button */
        case WM_KILLFOCUS:
        {
            SendMessageW(GetParent(hButton), DM_SETDEFID, IDCANCEL, 0);
            break;
        }
    }
    return CallWindowProcW(pContext->OldButtonProc, hButton, uMsg, wParam, lParam);
}

static VOID
ShutdownOnInit(
    IN HWND hDlg,
    IN PSHUTDOWN_DLG_CONTEXT pContext)
{
    MSGINAOPENTHEMEDATA msOpenThemeData = NULL;
    PGINA_CONTEXT pgContext = pContext->pgContext;
    HWND hwndList;
    INT idx, count, i;
    WCHAR szBuffer[256];
    WCHAR szBuffer2[256];
    HDC hdc;
    LONG lfHeight;

    if (pContext->bFriendlyUI)
    {
        LoadStringW(pgContext->hDllInstance,
                    IDS_SHUTDOWN_TURNOFF,
                    pContext->StaticTexts[STATIC_TURNOFF],
                    _countof(pContext->StaticTexts[STATIC_TURNOFF]));
        pContext->bIsSleepButtonReplaced = FALSE;
        pContext->bTimer = FALSE;

        /* Gather old button func */
        pContext->OldButtonProc = (WNDPROC)GetWindowLongPtrW(GetDlgItem(hDlg, IDC_BUTTON_SHUTDOWN), GWLP_WNDPROC);

        /* load uxtheme.dll */
        pContext->hUxtheme = LoadLibraryW(L"uxtheme.dll");
        if (pContext->hUxtheme)
        {
            /* Load revelant theme data if possible */
            msOpenThemeData = (MSGINAOPENTHEMEDATA)GetProcAddress(pContext->hUxtheme, "OpenThemeData");
            if (msOpenThemeData)
            {
                pContext->hTheme[DLG_BACKGROUND] = msOpenThemeData(hDlg, L"LogOffShutDownDialogBg");
                if (pContext->hTheme[DLG_BACKGROUND])
                {
                    SetWindowLong(hDlg, GWL_STYLE, GetWindowLong(hDlg, GWL_STYLE)&~WS_BORDER);
                }

                pContext->hTheme[DLG_FLAG] = msOpenThemeData(hDlg, L"LogOffShutDownDialogFlag");
                pContext->hTheme[DLG_BUTTON_BITMAP] = msOpenThemeData(hDlg, L"ShutDownDialogButtons");
                pContext->hTheme[DLG_STATIC_TITLE] = msOpenThemeData(hDlg, L"LogOffShutDownDialogStaticTitle");
                pContext->hTheme[DLG_STATIC_BUTTONS] = msOpenThemeData(hDlg, L"LogOffShutDownDialogStaticButtons");
            }
        }
        
        /* Gather revelant bitmaps and load font if the required theme data was not loaded */
        if (!pContext->hTheme[DLG_BACKGROUND])
        {
            pContext->hBitmap[DLG_BACKGROUND] = LoadBitmapW(pgContext->hDllInstance, MAKEINTRESOURCEW(IDB_SHUTDOWN_DLG_BG));
        }
        if (!pContext->hTheme[DLG_FLAG])
        {
            pContext->hBitmap[DLG_FLAG] = LoadBitmapW(pgContext->hDllInstance, MAKEINTRESOURCEW(IDB_ROSLOGO_FLAG));
        }
        if (!pContext->hTheme[DLG_BUTTON_BITMAP])
        {
            pContext->hBitmap[DLG_BUTTON_BITMAP] = LoadBitmapW(pgContext->hDllInstance, MAKEINTRESOURCEW(IDB_IMAGE_STRIP));
        }
        if (!pContext->hTheme[DLG_STATIC_TITLE])
        {
            hdc = GetDC(hDlg);
            lfHeight = -MulDiv(FONT_POINT_SIZE, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ReleaseDC(hDlg, hdc);
            pContext->hfFont = CreateFontW(lfHeight, 0, 0, 0, FW_MEDIUM, FALSE, 0, 0, 0, 0, 0, 0, 0, L"MS Shell Dlg");
            SendDlgItemMessageW(hDlg, IDC_TURN_OFF_STATIC, WM_SETFONT, (WPARAM)pContext->hfFont, TRUE);
        }

        for (i = 0; i < NUMBER_OF_BUTTONS; i++)
        {
            pContext->bIsButtonHot[i] = FALSE;
            GetDlgItemTextW(hDlg, IDC_BUTTON_SHUTDOWN + i, pContext->StaticTexts[i + 1], _countof(pContext->StaticTexts[i + 1]));
            SetWindowLongPtrW(GetDlgItem(hDlg, IDC_BUTTON_SHUTDOWN + i), GWLP_WNDPROC, (LONG_PTR)OwnerDrawButtonSubclass);
        }

        /* Disable hibernation and sleep button since they are not implemented yet */
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_HIBERNATE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SLEEP), FALSE);

        /* Create tool tips for the buttons of fancy log off dialog */
        CreateToolTipForButtons(IDC_BUTTON_HIBERNATE,
                                IDS_SHUTDOWN_HIBERNATE_DESC,
                                hDlg, IDS_SHUTDOWN_HIBERNATE,
                                pContext->pgContext->hDllInstance);
        CreateToolTipForButtons(IDC_BUTTON_SHUTDOWN,
                                IDS_SHUTDOWN_SHUTDOWN_DESC,
                                hDlg, IDS_SHUTDOWN_SHUTDOWN,
                                pContext->pgContext->hDllInstance);
        CreateToolTipForButtons(IDC_BUTTON_REBOOT,
                                IDS_SHUTDOWN_RESTART_DESC,
                                hDlg, IDS_SHUTDOWN_RESTART,
                                pContext->pgContext->hDllInstance);
        CreateToolTipForButtons(IDC_BUTTON_SLEEP,
                                IDS_SHUTDOWN_SLEEP_DESC,
                                hDlg, IDS_SHUTDOWN_SLEEP,
                                pContext->pgContext->hDllInstance);

        if (pContext->ShutdownDialogId == IDD_SHUTDOWN_FANCY && IsPwrHibernateAllowed())
        {
            pContext->iTimer = SetTimer(hDlg, 0, 50, NULL);
            pContext->bTimer = TRUE;
        }
    }

    else
    {
        hwndList = GetDlgItem(hDlg, IDC_SHUTDOWN_ACTION);

        /* Clear the content before it's used */
        SendMessageW(hwndList, CB_RESETCONTENT, 0, 0);

        /* Log off */
        if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_LOGOFF)
        {
            LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_LOGOFF, szBuffer, _countof(szBuffer));
            StringCchPrintfW(szBuffer2, _countof(szBuffer2), szBuffer, pgContext->UserName);
            idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer2);
            if (idx != CB_ERR)
                SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_LOGOFF);
        }

        /* Shut down - DEFAULT */
        if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_POWER_OFF)
        {
            LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_SHUTDOWN, szBuffer, _countof(szBuffer));
            idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
            if (idx != CB_ERR)
                SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
        }

        /* Restart */
        if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_REBOOT)
        {
            LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_RESTART, szBuffer, _countof(szBuffer));
            idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
            if (idx != CB_ERR)
                SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
        }

        // if (pContext->ShutdownOptions & 0x08) {}

        /* Sleep */
        if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_SLEEP)
        {
            LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_SLEEP, szBuffer, _countof(szBuffer));
            idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
            if (idx != CB_ERR)
                SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_SLEEP);
        }

        // if (pContext->ShutdownOptions & 0x20) {}

        /* Hibernate */
        if (pContext->ShutdownOptions & WLX_SHUTDOWN_STATE_HIBERNATE)
        {
            LoadStringW(pgContext->hDllInstance, IDS_SHUTDOWN_HIBERNATE, szBuffer, _countof(szBuffer));
            idx = SendMessageW(hwndList, CB_ADDSTRING, 0, (LPARAM)szBuffer);
            if (idx != CB_ERR)
                SendMessageW(hwndList, CB_SETITEMDATA, idx, WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
        }

        // if (pContext->ShutdownOptions & 0x80) {}

        /* Set the default shut down selection */
        count = SendMessageW(hwndList, CB_GETCOUNT, 0, 0);
        for (i = 0; i < count; i++)
        {
            if (SendMessageW(hwndList, CB_GETITEMDATA, i, 0) == pgContext->nShutdownAction)
            {
                SendMessageW(hwndList, CB_SETCURSEL, i, 0);
                break;
            }
        }
    
        /* Update the choice description based on the current selection */
        UpdateShutdownDesc(hDlg, pContext);
    }
}

static VOID
ShutdownOnOk(
    IN HWND hDlg,
    IN PGINA_CONTEXT pgContext)
{
    INT idx;

    idx = SendDlgItemMessageW(hDlg,
                              IDC_SHUTDOWN_ACTION,
                              CB_GETCURSEL,
                              0,
                              0);
    if (idx != CB_ERR)
    {
        pgContext->nShutdownAction =
            SendDlgItemMessageW(hDlg,
                                IDC_SHUTDOWN_ACTION,
                                CB_GETITEMDATA,
                                idx,
                                0);
    }
}

static INT_PTR
CALLBACK
ShutdownDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSHUTDOWN_DLG_CONTEXT pContext;

    pContext = (PSHUTDOWN_DLG_CONTEXT)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pContext = (PSHUTDOWN_DLG_CONTEXT)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)pContext);

            ShutdownOnInit(hDlg, pContext);
            return TRUE;
        }

        case WM_ACTIVATE:
        {
            /*
             * If the user deactivates the shutdown dialog (it loses its focus
             * while the dialog is not being closed), then destroy the dialog
             * and cancel shutdown.
             */
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                if (!pContext->bCloseDlg)
                {
                    pContext->bCloseDlg = TRUE;
                    EndDialog(hDlg, IDCANCEL);
                }
            }
            return FALSE;
        }

        case WM_DESTROY:
            if (pContext->bFriendlyUI)
            {
                EndFriendlyDialog(hDlg, pContext);
            }
            break;

        case WM_CLOSE:
            pContext->bCloseDlg = TRUE;
            EndDialog(hDlg, IDCANCEL);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_SHUTDOWN:
                    ExitWindowsEx(EWX_SHUTDOWN, SHTDN_REASON_MAJOR_OTHER);
                    break;

                case IDC_BUTTON_REBOOT:
                    ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER);
                    break;

                case IDC_BUTTON_SLEEP:
                    SetSuspendState(TRUE, TRUE, TRUE);
                    break;

                case IDOK:
                    ShutdownOnOk(hDlg, pContext->pgContext);

                /* Fall back */
                case IDCANCEL:
                case IDHELP:
                    pContext->bCloseDlg = TRUE;
                    EndDialog(hDlg, LOWORD(wParam));
                    break;

                case IDC_SHUTDOWN_ACTION:
                    UpdateShutdownDesc(hDlg, pContext);
                    break;
            }
            break;

        case WM_CTLCOLORSTATIC:
        {
            /* Make background transparent and fill the text with required colours*/
            HDC hdcStatic = (HDC)wParam;
            UINT StaticID = (UINT)GetWindowLongPtrW((HWND)lParam, GWL_ID);

            switch (StaticID)
            {
                case IDC_TURN_OFF_STATIC:
                   SetTextColor(hdcStatic, DARK_GREY_COLOR);
                   SetBkMode(hdcStatic, TRANSPARENT);
                   return (INT_PTR)GetStockObject(HOLLOW_BRUSH);

                case IDC_HIBERNATE_STATIC:
                case IDC_SHUTDOWN_STATIC:
                case IDC_SLEEP_STATIC:
                case IDC_RESTART_STATIC:
                    SetTextColor(hdcStatic, LIGHT_GREY_COLOR);
                    SetBkMode(hdcStatic, TRANSPARENT);
                    return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
            }
            return FALSE;
        }

        case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;
            switch (pdis->CtlID)
            {
                case IDC_BUTTON_SHUTDOWN:
                case IDC_BUTTON_REBOOT:
                case IDC_BUTTON_SLEEP:
                case IDC_BUTTON_HIBERNATE:
                    return DrawIconOnOwnerDrawnButtons(pdis, pContext);

                case IDC_SHUTDOWN_DLG_BG:
                case IDC_SHUTDOWN_DLG_FLAG:
                    return DrawDialogBoxBgElements(pdis, pContext);

                case IDC_TURN_OFF_STATIC:
                case IDC_HIBERNATE_STATIC:
                case IDC_SHUTDOWN_STATIC:
                case IDC_SLEEP_STATIC:
                case IDC_RESTART_STATIC:
                    return DrawDialogBoxStaticTexts(pdis, pContext);
            }
            return FALSE;
        }

        case WM_TIMER:
            OnTimer(hDlg, pContext);
            return TRUE;

        default:
            return FALSE;
    }
    return TRUE;
}

INT_PTR
ShutdownDialog(
    IN HWND hwndDlg,
    IN DWORD ShutdownOptions,
    IN PGINA_CONTEXT pgContext)
{
    INT_PTR ret;
    SHUTDOWN_DLG_CONTEXT Context;

#if 0
    DWORD ShutdownOptions;

    // FIXME: User impersonation!!
    pgContext->nShutdownAction = LoadShutdownSelState();
    ShutdownOptions = GetAllowedShutdownOptions();
#endif

    Context.pgContext = pgContext;
    Context.ShutdownOptions = ShutdownOptions;
    Context.ShutdownDialogId = IDD_SHUTDOWN;
    Context.bCloseDlg = FALSE;
    Context.bReasonUI = GetShutdownReasonUI();
    Context.bFriendlyUI = ShellIsFriendlyUIActive();

    if (pgContext->hWlx && pgContext->pWlxFuncs && !Context.bFriendlyUI)
    {
        ret = pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                                      pgContext->hDllInstance,
                                                      MAKEINTRESOURCEW(Context.bReasonUI ? IDD_SHUTDOWN_REASON : IDD_SHUTDOWN),
                                                      hwndDlg,
                                                      ShutdownDialogProc,
                                                      (LPARAM)&Context);
    }
    else
    {
        if (Context.bFriendlyUI)
        {
            if (IsShowHibernateButtonActive())
            {
                Context.ShutdownDialogId = IDD_SHUTDOWN_FANCY_LONG;
            }
            else
            {
                Context.ShutdownDialogId = IDD_SHUTDOWN_FANCY;
            }
        }

        ret = DialogBoxParamW(pgContext->hDllInstance,
                              MAKEINTRESOURCEW(Context.bReasonUI ? IDD_SHUTDOWN_REASON : Context.ShutdownDialogId),
                              hwndDlg,
                              ShutdownDialogProc,
                              (LPARAM)&Context);
    }

#if 0
    // FIXME: User impersonation!!
    if (ret == IDOK)
        SaveShutdownSelState(pgContext->nShutdownAction);
#endif

    return ret;
}


/*
 * NOTES:
 * - Based upon observations on the ShellShutdownDialog() function, the function doesn't actually
 *   do anything except show a dialog box and returning a value based upon the value chosen. That
 *   means that any code that calls the function has to execute the chosen action (shut down,
 *   restart, etc.).
 * - When this function is called in Windows XP, it shows the classic dialog box regardless if
 *   SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\LogonType is enabled or not.
 * - When the Help button is pushed, it sends the same return value as IDCANCEL (0x00), but
 *   at the same time, it calls the help file directly from the dialog box.
 * - When the dialog is created, it doesn't disable all other input from the other windows.
 *   This is done elsewhere. When running the function ShellShutdownDialog() from XP/2K3, if the user clicks
 *   out of the window, it automatically closes itself.
 * - The parameter, lpUsername never seems to be used when calling the function from Windows XP. Either
 *   it was a parameter that was never used in the final version before release, or it has a use that
 *   is currently not known.
 */
DWORD WINAPI
ShellShutdownDialog(
    HWND   hParent,
    LPWSTR lpUsername,
    BOOL   bHideLogoff)
{
    INT_PTR dlgValue;
    DWORD ShutdownOptions;

    /*
     * As we are called by the shell itself, don't use
     * the cached GINA context but use a local copy here.
     */
    GINA_CONTEXT gContext = { 0 };
    DWORD BufferSize;

    UNREFERENCED_PARAMETER(lpUsername);

    ShutdownOptions = GetAllowedShutdownOptions();
    if (bHideLogoff)
        ShutdownOptions &= ~WLX_SHUTDOWN_STATE_LOGOFF;

    /* Initialize our local GINA context */
    gContext.hDllInstance = hDllInstance;
    BufferSize = _countof(gContext.UserName);
    // NOTE: Only when this function is called, Win checks inside
    // HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
    // value "Logon User Name", and determines whether it will display
    // the user name.
    GetUserNameW(gContext.UserName, &BufferSize);
    gContext.nShutdownAction = LoadShutdownSelState();

    /* Load the shutdown dialog box */
    dlgValue = ShutdownDialog(hParent, ShutdownOptions, &gContext);

    /* Determine what to do based on user selection */
    if (dlgValue == IDOK)
    {
        SaveShutdownSelState(gContext.nShutdownAction);

        switch (gContext.nShutdownAction)
        {
            case WLX_SAS_ACTION_LOGOFF:
                return WLX_SHUTDOWN_STATE_LOGOFF;

            case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
                return WLX_SHUTDOWN_STATE_POWER_OFF;

            case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
                return WLX_SHUTDOWN_STATE_REBOOT;

            // 0x08

            case WLX_SAS_ACTION_SHUTDOWN_SLEEP:
                return WLX_SHUTDOWN_STATE_SLEEP;

            // 0x20

            case WLX_SAS_ACTION_SHUTDOWN_HIBERNATE:
                return WLX_SHUTDOWN_STATE_HIBERNATE;

            // 0x80
        }
    }
    /* Help file is called directly here */
    else if (dlgValue == IDHELP)
    {
        FIXME("Help is not implemented yet.\n");
        MessageBoxW(hParent, L"Help is not implemented yet.", L"Message", MB_OK | MB_ICONEXCLAMATION);
    }
    else if (dlgValue == -1)
    {
        ERR("Failed to create dialog\n");
    }

    return 0;
}

/*
 * NOTES:
 * - Undocumented, called from MS shell32.dll to show the turn off dialog.
 * - Seems to have the same purpose as ShellShutdownDialog.
 */
DWORD WINAPI
ShellTurnOffDialog(HWND hWnd)
{
    return ShellShutdownDialog(hWnd, NULL, FALSE);
}
