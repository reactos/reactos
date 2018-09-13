/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    internat.c

Abstract:

    This module contains the application that displays an indicator in
    the Notification area of the Tray.

Revision History:

--*/



//
//  Include Files.
//

#include "internat.h"



//   we have two attachthreadinput() usages in internat.exe. 
//   Internat_SetInputHooks() has one, which is to get mouse 
//   notifications from the tray window, the other is for window 
//   activation. Activation itself in fact should not need 
//   AttachThreadInput(), but this was added at the very
//   beginning of this indicator applet while win95 development,
//   we believe it was to suppress any possible flickering or
//   maybe internat.exe got confused if activation wasn't serialized.
//   Now we install CBT hook to keep track of every focus change, so
//   it is theoritically unnecessary to get this serialization code.
//   let's try ripping out these attachthreadinput() for activation,
//   and see what happens. 2/22/99 yutakan
//
#define NO_SERIALIZED_ACTIVATE

//
//  Constant Declarations.
//
#define  NLS_RESOURCE_LOCALE_KEY   TEXT("Control Panel\\desktop\\ResourceLocale")





//
//  Global Variables.
//

int Internat_OnTaskbarCreated(HWND hwnd, WPARAM wParam, LPARAM lparam);
void Internat_SetInputHooks(HWND hwnd);
BOOL Internat_SetIndicHooks(HWND hwnd);
BOOL FindTrayEtc();

UINT g_wmTaskbarCreated;
HKL                  hklCurrent;
#define B3_RESTART      2   // 3rd state: we're active, but in restart state
BOOL                 bInternatActive      = FALSE;  // T/F/B3_RESTART
HIMAGELIST           himIndicators        = NULL;
HDPA                 hdpaInfoList         = NULL;
HWND                 hwndTray             = NULL;
HWND                 hwndNotify           = NULL;
HINSTANCE            hinst                = NULL;
HINSTANCE            hInstLib             = NULL;
UINT                 cxSmIcon             = 0;
UINT                 cySmIcon             = 0;
UINT                 uTaskBarEdge         = ABE_BOTTOM;
LONG                 lIconFontHeight      = 0;
LONG                 lIconFontWidth       = 0;

// default IMC - restored when being destroyed
HIMC                 hDefIMC;

int                  meEto                = 0;

int                  iImeCurStat;

REGHOOKPROC          fpRegHookWindow      = NULL;
PROC                 fpStartShell         = NULL;
PROC                 fpStopShell          = NULL;
FPGETIMESTAT         fpGetIMEStat         = NULL;
FPGETIMEMENU         fpGetIMEMenu         = NULL;
FPGETLAYOUT          fpGetLayout          = NULL;
FPBUILDIMEMENU       fpBuildIMEMenu       = NULL;
FPGETIMEMENUITEMID   fpGetIMEMenuItemID   = NULL;
FPDESTROYIMEMENU     fpDestroyIMEMenu     = NULL;
FPSETNOTIFYWND       fpSetNotifyWnd       = NULL;
FPGETLASTACTIVE      fpGetLastActive      = NULL;
FPGETLASTFOCUS       fpGetLastFocus       = NULL;
FPSETIMEMENUITEMDATA fpSetIMEMenuItemData = NULL;
FPGETCONSOLEIMEWND   fpGetConsoleImeWnd   = NULL;

FPGETDEFAULTIMEMENUITEM   fpGetDefaultImeMenuItem    = NULL;

BOOL                 bInLangMenu          = FALSE;

DWORD                fsShell;

TCHAR                szAppName[]          = INDICATOR_CLASS;
TCHAR                szHelpFile[]         = TEXT("windows.hlp");
TCHAR                szPropHwnd[]         = TEXT("hwndIMC");
TCHAR                szPropImeStat[]      = TEXT("ImeStat");
#ifdef WINNT
TCHAR                szConsoleWndClass[]  = TEXT("ConsoleWindowClass");
TCHAR                szProgManWndClass[]  =TEXT("Progman");
#endif
BOOL                 g_bIMEIndicator      = FALSE;
int                  nIMEIconIndex[8];             // eight states for now


//
//  For Keyboard Layout info.
//
typedef struct
{
    DWORD dwID;                     // numeric id
    ATOM atmLayoutText;             // layout text
    UINT iSpecialID;                // i.e. 0xf001 for dvorak etc

} LAYOUT, *LPLAYOUT;


static HANDLE      hLayout;
static UINT        nLayoutBuffSize;
static UINT        iLayoutBuff = 0;
static LPLAYOUT    lpLayout;

static TCHAR       szLayoutPath[]  = TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
static TCHAR       szLayoutText[]  = TEXT("layout text");
static TCHAR       szLayoutID[]    = TEXT("layout id");

////////////////////////////////////////////////////////////////////////////
//
//  Internat_SendEndMenuMessage
//
////////////////////////////////////////////////////////////////////////////

void Internat_SendEndMenuMessage(HWND hwnd, UINT uID, HWND hwndDefIME)
{
    int nCnt = 0;
    int nRet;
    
    fpSetIMEMenuItemData(uID);
    
    if (IsWindow(hwndDefIME))
    {
        // IMS_ENDIMEMENU on NT5 always returns 0L
        SendMessage(hwnd, WM_IME_SYSTEM, IMS_ENDIMEMENU,  (LPARAM)hwndDefIME);
    }
}

void Internat_HandleDblClick(HWND hwnd)
{
    HWND hwndImc;
    UINT  uID;
    int istat = Internat_GetIMEStatus(&hwndImc);
    
    // BUGBUG: should check if we're already in GetIMEMenu
    // and skip the following. Maybe an NT bug.
    Internat_GetIMEMenu(hwndImc, FALSE);
                
    if (!(uID = Internat_GetDefaultImeMenuItem()))
    {
        HKL dwhkl = Internat_GetKeyboardLayout(hwndImc);

        if (istat != IMESTAT_DISABLED &&
            ((ULONG_PTR)dwhkl & 0xf000ffffL) != 0xe0000412L)
        {
            Internat_SetIMEOpenStatus( hwnd,
                                   istat == IMESTAT_CLOSE,
                                   hwndImc );
            SetForegroundWindow(Internat_GetTopLevelWindow(hwndImc));
        }
    }
    else
    {
        HWND hwndToSend= ImmGetDefaultIMEWnd(hwndImc);
        Internat_SerializeActivate(hwnd, hwndImc);
        Internat_SendEndMenuMessage(hwndToSend, uID, hwndImc);
    }

    Internat_DestroyIMEMenu();
} 

////////////////////////////////////////////////////////////////////////////
//
//  Internat_MainWndProc
//
//  Main window for processing messages.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Internat_MainWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndShell = (HWND)wParam;

    switch (uMsg)
    {
        //
        //  Discard any of the notifications to the IME because
        //  we don't want to see the IME UI on indicator.
        //
        case ( WM_IME_NOTIFY ) :
        {
            return (0L);
        }
        case ( WM_SETTINGCHANGE ) :
        {
            Internat_onSettingChange(hwnd, FALSE);
            break;
        }
        case ( WM_INPUTLANGCHANGEREQUEST ) :
        {
            return (0L);
        }
        case ( WM_QUERYENDSESSION ) :
        {
            DestroyWindow(hwnd);
            return (TRUE);
        }
        case ( WM_CREATE ) :
        {
            if (Internat_OnCreate(hwnd, wParam, lParam))
            {
                PostMessage(hwnd, WM_COMMAND, CMDINDIC_EXIT, 0L);
                return ((LRESULT)-1);
            }
            else
            {
                break;
            }
        }
        case ( WM_MYLANGUAGECHANGE ) :
        {
            //
            //  Don't handle LANGUAGE change for internat itself.
            //
            if (hwndShell == hwnd)
            {
                break;
            }
            else
            {
                return (Internat_HandleLanguageMsg(hwnd, (HKL)lParam));
            }
        }
        case ( WM_DESTROY ) :
        {
            Internat_Destroy(hwnd);
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            Internat_HandleLangMenuMeasure(hwnd, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            Internat_HandleLangMenuDraw(hwnd, (LPDRAWITEMSTRUCT)lParam);
            break;
        }
        case ( WM_LANGUAGE_INDICATOR ) :
        {
            //
            //  If the current focus window has gone already, there's
            //  nothing we can do other than to get an active window
            //  which comes with HSHELL_WINDOWACTIVATED.
            //
            if (lParam == WM_LBUTTONDOWN)
            {
                lParam = GetMessagePos();
                Internat_CreateLanguageMenu(hwnd, lParam);
                PostMessage(hwndNotify, WM_LBUTTONUP, wParam, lParam);
            }
            else if (lParam == WM_RBUTTONDOWN)
            {
                lParam = GetMessagePos();
                Internat_CreateOtherIndicatorMenu(hwnd, lParam);
            }
            break;
        }
        case ( WM_IME_INDICATOR2 ) :
            if (lParam == WM_LBUTTONDBLCLK)
            {
                Internat_HandleDblClick(hwnd);
            }
            else if (lParam == WM_LBUTTONDOWN)
            {
                Internat_CreateImeMenu(hwnd);
            }
            break;
        case ( WM_IME_INDICATOR ) :
        {
            // because shell sends us notification
            // using SendNotifyMessage(), we get
            // these two messages at the same
            // time. NT IMM unfortunately yield
            // control to system when it sendmsg
            // so we get WM_LBUTTONDBLCLK while we
            // process WM_LBUTTONDOWN.
            // we post them so we can handle them later
            // 
            if (lParam == WM_LBUTTONDOWN 
             || lParam == WM_LBUTTONDBLCLK)
            {    
                PostMessage(hwnd, WM_IME_INDICATOR2, wParam, lParam);
            }
            else if (lParam == WM_RBUTTONDOWN)
            {
                Internat_CreateRightImeMenu(hwnd);
            }
            
            break;
        }
        case ( WM_MYSETOPENSTATUS ) :
        {
            if (IsWindow((HWND)lParam))
            {
                //
                //  WM_IME_SYSTEM should go straight to the default IME
                //  window.
                //
                DWORD dwPI;
                HWND hwndDefIme = ImmGetDefaultIMEWnd((HWND)lParam);

                if (IsWindow(hwndDefIme))
                {
                    SendMessage( hwndDefIme,
                                 WM_IME_SYSTEM,
                                 IMS_SETOPENCLOSE,
                                 (LPARAM)wParam );
                }
            }
            break;
        }
        case ( WM_MYLANGUAGECHECK ) :
        {
            HKL hklLastFocus;
            HWND hwndFocus;

            hklLastFocus = Internat_GetLayout();

            if ((hklLastFocus == hklCurrent) &&
                (iImeCurStat == Internat_GetIMEStatus(&hwndFocus)))
            {
                break;
            }

            SetTimer(hwnd, TIMER_MYLANGUAGECHECK, 500, NULL);
            break;
        }
        case ( WM_TIMER ) :
        {
            if (wParam == TIMER_MYLANGUAGECHECK)
            {
                HKL hklLastFocus;
                HWND hwndFocus;

                KillTimer(hwnd, TIMER_MYLANGUAGECHECK);

                hklLastFocus = Internat_GetLayout();

                if ((hklLastFocus == hklCurrent) &&
                    (iImeCurStat == Internat_GetIMEStatus(&hwndFocus)))
                {
                    break;
                }

                SendMessage(hwnd, WM_MYLANGUAGECHANGE, 0, (LPARAM)hklLastFocus);
            }
            break;
        }
        case ( INDICM_SETIMEICON ) :
        case ( INDICM_SETIMETOOLTIPS ) :
        case INDICM_REMOVEDEFAULTMENUITEMS:
        {
            HWND       hwndImc;
            HKL        dwhkl;
            PMLNGINFO  pMlngInfo;
            BOOL       bFound = FALSE;
            int        nIndex;
            int        nCount;
            int        istat;
            BOOL       bSendIconNotify = TRUE;
#ifdef WINNT
            TCHAR      szClassName[64];
#endif

            //
            // We should not allow IME to use SendMessage() with INDICM_xxxx.
            // (Memphis compat)
            //
            if (InSendMessageEx(NULL) != ISMEX_NOSEND) {
                break;
            }

            nCount = DPA_GetPtrCount(hdpaInfoList);
            for (nIndex = 0; nIndex < nCount; nIndex++)
            {
                pMlngInfo = DPA_GetPtr(hdpaInfoList, nIndex);
                if (pMlngInfo->dwHkl == (HKL)lParam)
                {
                    bFound = TRUE;
                    break;
                }
            }

            //
            //  If we can't find it, take the first one in the list.
            //
            if (!bFound)
            {
                break;
            }

            if (uMsg == INDICM_SETIMEICON)
            {
                if (LOWORD(wParam) == 0xffff)
                {
                    pMlngInfo->dwIMEFlag &= ~MMIF_USE_ICON_INDEX;
                }
                else
                {
                    pMlngInfo->dwIMEFlag |= MMIF_USE_ICON_INDEX;
                    pMlngInfo->uIMEIconIndex = (UINT)wParam;
                }
            }
            else if (uMsg == INDICM_SETIMETOOLTIPS)
            {
                if (LOWORD(wParam) == 0xffff)
                {
                    pMlngInfo->dwIMEFlag &= ~MMIF_USE_ATOM_TIP;
                }
                else
                {
                    pMlngInfo->dwIMEFlag |= MMIF_USE_ATOM_TIP;
                    pMlngInfo->atomTip = (ATOM)wParam;
                }
            }
            else if (uMsg == INDICM_REMOVEDEFAULTMENUITEMS) {
                // No need to update the tray icon.
                bSendIconNotify = FALSE;

                if (wParam == 0) {
                    pMlngInfo->dwIMEFlag &= ~(MMIF_REMOVEDEFAULTMENUITEMS | MMIF_REMOVEDEFAULTRIGHTMENUITEMS);
                }
                else {
                    if (wParam & RDMI_LEFT)
                        pMlngInfo->dwIMEFlag |= MMIF_REMOVEDEFAULTMENUITEMS;
                    if (wParam & RDMI_RIGHT)
                        pMlngInfo->dwIMEFlag |= MMIF_REMOVEDEFAULTRIGHTMENUITEMS;
                }
            }

            istat = Internat_GetIMEStatus(&hwndImc);
            dwhkl = Internat_GetKeyboardLayout(hwndImc);

#ifdef WINNT
            GetClassName(hwndImc, szClassName, ARRAYSIZE(szClassName));

            if (!lstrcmpi(szClassName,szConsoleWndClass))
                dwhkl = (HKL) lParam;

#endif
            if (bSendIconNotify && dwhkl == (HKL)lParam)
            {
              Internat_SendIMEIndicatorMsg(hwnd, (HKL)lParam, NIM_MODIFY);
            }

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( CMDINDIC_REFRESHINDIC ) :
                {
                    //
                    //  Refresh, message comes straight from cpanel.
                    //
                    return (Internat_HandleLanguageMsg(hwnd, (HKL)lParam));
                }
                case ( CMDINDIC_EXIT ) :
                {
                    DestroyWindow(hwnd);
                    break;
                }
            }
            break;
        }
        case ( WM_SYSCOMMAND ) :
        {
            if (wParam == SC_CLOSE)
            {
                break;
            }

            // fall thru...
        }
        default :
        {
            if (uMsg == g_wmTaskbarCreated)
            {
                Internat_OnTaskbarCreated(hwnd, wParam, lParam);
            }
            return (DefWindowProc(hwnd, uMsg, wParam, lParam));
        }
    }

    return (0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_InitApplication
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_InitApplication(
    HINSTANCE hInstance)
{
    WNDCLASSEX wc;
    TCHAR sz[100];
    TCHAR szRcAppName[100];

    LoadString(hInstance, IDS_APPNAME, szRcAppName, ARRAYSIZE(szRcAppName));

    if (FindWindow(szAppName, NULL))
    {
        if (LoadString(hInstance, IDS_PREVIOUS, sz, 100))
        {
            MessageBox(NULL, sz, szRcAppName, MB_ICONINFORMATION | MB_OK);
        }
        return (FALSE);
    }

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)Internat_MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INTERNAT));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAppName;
    wc.hIconSm       = NULL;

    return (RegisterClassEx(&wc));
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_InitInstance
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_InitInstance(
    HINSTANCE hInstance,
    int nCmdShow)
{
    HWND hwnd;

    hinst = hInstance;

    hwnd = CreateWindowEx( WS_EX_TOOLWINDOW,
                           szAppName,
                           NULL,
                           WS_DISABLED | WS_POPUP,
                           0, 0, 0, 0,
                           NULL,
                           NULL,
                           hInstance,
                           NULL );
    if (!hwnd)
    {
        return (FALSE);
    }

    //
    //  We don't want to get activated.
    //
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_WinMain
//
//  Applications main entry point. Gets the whole show up and running.
//
////////////////////////////////////////////////////////////////////////////

int WINAPI Internat_WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpAnsiCmdLine,
    int nCmdShow)
{
    MSG msg;

    if (!Internat_InitApplication(hInstance))
    {
        return (FALSE);
    }

    if (!Internat_InitInstance(hInstance, nCmdShow))
    {
        return (FALSE);
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
    lpAnsiCmdLine;
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_TransNum
//
//  Converts a number string to a dword value.
//
////////////////////////////////////////////////////////////////////////////

DWORD Internat_TransNum(
    LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }

    return (dw);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetHKL
//
////////////////////////////////////////////////////////////////////////////

HKL Internat_GetHKL(
    DWORD dwLocale,
    DWORD dwLayout,
    LPTSTR pszLayout,
    HINF hIntlInf)
{
    TCHAR szData[MAX_PATH];
    INFCONTEXT Context;

    //
    //  Get the HKL based on the input locale value and the layout value.
    //
    if (dwLayout == 0)
    {
        //
        //  See if it's the default layout for the input locale or an IME.
        //
        if (HIWORD(dwLocale) == 0)
        {
            return ((HKL)MAKELONG(dwLocale, dwLocale));
        }
        else if ((HIWORD(dwLocale) & 0xf000) == 0xe000)
        {
            return ((HKL)dwLocale);
        }
    }
    else
    {
        //
        //  Use the layout to make the hkl.
        //
        if (HIWORD(dwLayout) != 0)
        {
            //
            //  We have a special id.  Need to find out what the layout id
            //  should be.
            //
            if ((SetupFindFirstLine( hIntlInf,
                                     TEXT("KbdLayoutIds"),
                                     pszLayout,
                                     &Context )) &&
                (SetupGetStringField(&Context, 1, szData, MAX_PATH, NULL)))
            {
                dwLayout = (DWORD)(LOWORD(Internat_TransNum(szData)) | 0xf000);
            }
        }

        //
        //  Return the hkl:
        //      loword = input locale id
        //      hiword = layout id
        //
        return ((HKL)MAKELONG(dwLocale, dwLayout));
    }

    //
    //  Return failure.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_ValidateRegistryPreload
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_ValidateRegistryPreload()
{
    HKEY hKeyPreload, hKeySubst;
    HINF hIntlInf;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szData2[MAX_PATH];
    DWORD cbData, cbData2;
    DWORD dwData;
    DWORD dwNum, dwNumSet;
    LONG rc;

    //
    //  Open the HKCU\Keyboard Layout\Preload key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      TEXT("Keyboard Layout\\Preload"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKeyPreload ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Open the HKCU\Keyboard Layout\Substitutes key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      TEXT("Keyboard Layout\\Substitutes"),
                      0,
                      KEY_ALL_ACCESS,
                      &hKeySubst ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyPreload);
        return (FALSE);
    }

    //
    //  Open the Inf file.
    //
    hIntlInf = SetupOpenInfFile(TEXT("intl.inf"), NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf == INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (FALSE);
    }
    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        RegCloseKey(hKeyPreload);
        RegCloseKey(hKeySubst);
        return (FALSE);
    }

    //
    //  Go through the values in the Preload key.
    //
    dwNum = dwNumSet = 2;
    wsprintf(szValue, TEXT("%d"), dwNum);
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegQueryValueEx( hKeyPreload,
                          szValue,
                          NULL,
                          NULL,
                          (LPBYTE)szData,
                          &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  Look to see if the input locale is installed and if the layout
        //  is valid.
        //
        dwData = Internat_TransNum(szData);
        cbData2 = sizeof(szData2);
        szData2[0] = TEXT('\0');
        if (IsValidLocale((LCID)(LOWORD(dwData)), LCID_INSTALLED))
        {
            //
            //  Make sure we didn't delete one before this.  If so, we
            //  need to add this one with the new preload value.
            //
            if (dwNum != dwNumSet)
            {
                RegDeleteValue(hKeyPreload, szValue);
                wsprintf(szValue, TEXT("%d"), dwNumSet);
                RegSetValueEx( hKeyPreload,
                               szValue,
                               0,
                               REG_SZ,
                               (LPBYTE)szData,
                               (DWORD)(lstrlen(szData) + 1) * sizeof(TCHAR) );
            }
            dwNumSet++;
        }
        else
        {
            if (RegQueryValueEx( hKeySubst,
                                 szData,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szData2,
                                 &cbData2 ) == ERROR_SUCCESS)
            {
                UnloadKeyboardLayout(Internat_GetHKL( dwData,
                                                      Internat_TransNum(szData2),
                                                      szData2,
                                                      hIntlInf ));
                RegDeleteValue(hKeySubst, szData);
                RegDeleteValue(hKeyPreload, szValue);
            }
            else
            {
                UnloadKeyboardLayout(Internat_GetHKL(dwData, 0, NULL, hIntlInf));
                RegDeleteValue(hKeyPreload, szValue);
            }
        }

        //
        //  Query the next value.
        //
        dwNum++;
        wsprintf(szValue, TEXT("%d"), dwNum);
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegQueryValueEx( hKeyPreload,
                              szValue,
                              NULL,
                              NULL,
                              (LPBYTE)szData,
                              &cbData );
    }

    //
    //  Close the registry keys.
    //
    RegFlushKey(hKeyPreload);
    RegFlushKey(hKeySubst);
    RegCloseKey(hKeyPreload);
    RegCloseKey(hKeySubst);

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(hIntlInf);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_LoadKeyboardLayouts
//
//  Loads the layouts from the registry.
//
////////////////////////////////////////////////////////////////////////////

#define ALLOCBLOCK  3        // # items added to block for alloc/realloc

BOOL Internat_LoadKeyboardLayouts(void)
{
    HKEY hKey;
    HKEY hkey1;
    DWORD cb;
    DWORD dwIndex;
    LONG dwRetVal;
    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name


    //
    //  Now read all the locales from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szLayoutPath, &hKey) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    dwIndex = 0;
    dwRetVal = RegEnumKey( hKey,
                           dwIndex,
                           szValue,
                           ARRAYSIZE(szValue) );

    if (dwRetVal != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (FALSE);
    }

    hLayout = GlobalAlloc(GHND, ALLOCBLOCK * sizeof(LAYOUT));
    nLayoutBuffSize = ALLOCBLOCK;
    iLayoutBuff = 0;
    lpLayout = GlobalLock(hLayout);

    if (!hLayout)
    {
        RegCloseKey(hKey);
        return (FALSE);
    }

    do
    {
        //
        //  New language - get the language name and the language id.
        //
        if (iLayoutBuff + 1 == nLayoutBuffSize)
        {
            HANDLE hTemp;

            GlobalUnlock(hLayout);

            nLayoutBuffSize += ALLOCBLOCK;
            hTemp = GlobalReAlloc( hLayout,
                                   nLayoutBuffSize * sizeof(LAYOUT),
                                   GHND );
            if (hTemp == NULL)
            {
                break;
            }

            hLayout = hTemp;
            lpLayout = GlobalLock(hLayout);
        }

        lpLayout[iLayoutBuff].dwID = Internat_TransNum(szValue);

        lstrcpy(szData, szLayoutPath);
        lstrcat(szData, TEXT("\\"));
        lstrcat(szData, szValue);

        if (RegOpenKey(HKEY_LOCAL_MACHINE, szData, &hkey1) == ERROR_SUCCESS)
        {
            //
            //  Get the layout name.
            //
            szValue[0] = TEXT('\0');
            cb = sizeof(szValue);
            if ((RegQueryValueEx( hkey1,
                                  szLayoutText,
                                  NULL,
                                  NULL,
                                  (LPBYTE)szValue,
                                  &cb ) == ERROR_SUCCESS) &&
                (cb > sizeof(TCHAR)))
            {
                lpLayout[iLayoutBuff].atmLayoutText = AddAtom(szValue);

                szValue[0] = TEXT('\0');
                cb = sizeof(szValue);
                lpLayout[iLayoutBuff].iSpecialID = 0;
                if (RegQueryValueEx( hkey1,
                                     szLayoutID,
                                     NULL,
                                     NULL,
                                     (LPBYTE)szValue,
                                     &cb ) == ERROR_SUCCESS)
                {
                    //
                    //  This may not exist!
                    //
                    lpLayout[iLayoutBuff].iSpecialID = (UINT)Internat_TransNum(szValue);
                }
                iLayoutBuff++;
            }
            RegCloseKey(hkey1);
        }

        dwIndex++;
        szValue[0] = TEXT('\0');
        dwRetVal = RegEnumKey( hKey,
                               dwIndex,
                               szValue,
                               ARRAYSIZE(szValue) );

    } while (dwRetVal == ERROR_SUCCESS);

    RegCloseKey(hKey);

    return (iLayoutBuff);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetKbdLayoutName
//
//  Gets the name of the given layout.
//
////////////////////////////////////////////////////////////////////////////

void Internat_GetKbdLayoutName(
    DWORD dwLayout,
    LPTSTR pBuffer,
    int nBufSize)
{
    UINT ctr, id;
    WORD wLayout = HIWORD(dwLayout);
    BOOL bIsIME = ((HIWORD(dwLayout) & 0xf000) == 0xe000) ? TRUE : FALSE;

    //
    //  Find the layout in the global structure.
    //
    if ((wLayout & 0xf000) == 0xf000)
    {
        //
        //  Layout is special, need to search for the ID number.
        //
        id = wLayout & 0x0fff;
        for (ctr = 0; ctr < iLayoutBuff; ctr++)
        {
            if (id == lpLayout[ctr].iSpecialID)
            {
                break;
            }
        }
    }
    else
    {
        for (ctr = 0; ctr < iLayoutBuff; ctr++)
        {
            //
            //  If it's an IME, we need to do a DWORD comparison.
            //
            if (bIsIME && (dwLayout == lpLayout[ctr].dwID))
            {
                break;
            }
            else if (wLayout == LOWORD(lpLayout[ctr].dwID))
            {
                break;
            }
        }
    }

    //
    //  Make sure there is a match.  If not, then simply return without
    //  copying anything.
    //
    if (ctr < iLayoutBuff)
    {
        //
        //  Separate the Input Locale name and the Layout name with " - ".
        //
        pBuffer[0] = TEXT(' ');
        pBuffer[1] = TEXT('-');
        pBuffer[2] = TEXT(' ');

        GetAtomName(lpLayout[ctr].atmLayoutText, pBuffer + 3, nBufSize - 3);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_Destroy
//
////////////////////////////////////////////////////////////////////////////

void Internat_Destroy(
    HWND hwnd)
{
    UINT ctr;

    KillTimer(hwnd, TIMER_MYLANGUAGECHECK);

    ImmAssociateContext(hwnd, hDefIMC);

    Internat_ManageMlngInfo(hwnd, DESTROY_MLNGINFO);

    for (ctr = 0; ctr < iLayoutBuff; ctr++)
    {
        if (lpLayout[ctr].atmLayoutText)
        {
            DeleteAtom(lpLayout[ctr].atmLayoutText);
        }
    }
    iLayoutBuff = 0;
    GlobalUnlock(hLayout);
    GlobalFree(hLayout);

    if (fpStopShell)
    {
        fpStopShell();
    }
    FreeLibrary(hInstLib);
    PostQuitMessage(0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_onSettingChange
//
////////////////////////////////////////////////////////////////////////////

void Internat_onSettingChange(
    HWND hwnd, BOOL fForce)
{
    UINT cx, cy;
    LOGFONT lf;
    APPBARDATA abd;

    cx = GetSystemMetrics(SM_CXSMICON);
    cy = GetSystemMetrics(SM_CYSMICON);

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
    {
        if (lf.lfHeight != lIconFontHeight || lf.lfWidth != lIconFontWidth)
        {
            lIconFontHeight = lf.lfHeight;
            lIconFontWidth = lf.lfWidth;
            fForce = TRUE;
        }
    }

    if (fForce || (cxSmIcon != cx) || (cySmIcon != cy))
    {
        HKL dwHkl;

        cxSmIcon = cx;
        cySmIcon = cy;
        Internat_ManageMlngInfo(hwnd, UPDATE_MLNGINFO);

        dwHkl = Internat_GetKeyboardLayout(Internat_GetCurrentFocusWnd());
        hklCurrent = dwHkl - 1; // make it bogus
        Internat_SendLangIndicatorMsg(hwnd, dwHkl, NIM_MODIFY);
    }

    abd.cbSize = sizeof(APPBARDATA);
    if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd))
    {
        if (abd.rc.top == abd.rc.left)
            uTaskBarEdge = (abd.rc.bottom > abd.rc.right)? ABE_LEFT: ABE_TOP;
        else
            uTaskBarEdge = (abd.rc.top < abd.rc.left)? ABE_RIGHT: ABE_BOTTOM;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_EnumChildWndProc
//
//  Look at the class names using GetClassName to see if we can find the
//  Tray notification Window.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Internat_EnumChildWndProc(
    HWND hwnd,
    LPARAM lParam)
{
    TCHAR szString[50];

    GetClassName(hwnd, szString, ARRAYSIZE(szString));
    if (lstrcmp(szString, szNotifyWindow) == 0)
    {
        hwndNotify = hwnd;
        return (FALSE);
    }

    return (TRUE);
}


//***   FindTrayEtc -- find tray and tray notify windows
// ENTRY/EXIT
//  fRet        (return) TRUE on success, o.w. FALSE
//  hwndTray    (side-effect) tray window
//  hwndNotify  (side-effect) tray notify window
BOOL FindTrayEtc()
{
    hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), NULL);

    if (!hwndTray)
    {
        return FALSE;
    }

    EnumChildWindows(hwndTray, (WNDENUMPROC)Internat_EnumChildWndProc, 0);

    if (!hwndNotify)
    {
        return FALSE;
    }

    g_wmTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_OnCreate
//
//  Let's put all the Create stuff in a neat function of its own.
//
//  We find the Tray Window and enumarate its windows to locate the
//  Notify Window.
//
//  Get all the Multilingual information about the system and then load the
//  dll and start the shell Hook proc.
//
////////////////////////////////////////////////////////////////////////////

int Internat_OnCreate(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    HDC hdc;
    CHARSETINFO cs;

    if (GetSystemMetrics(SM_MIDEASTENABLED))
    {
        TCHAR sz[10];
        long cb = sizeof(sz);

        //
        //  Check the resource locale.
        //
        sz[0] = TEXT('\0');
        if (RegQueryValue( HKEY_CURRENT_USER,
                           NLS_RESOURCE_LOCALE_KEY,
                           sz,
                           &cb ) == ERROR_SUCCESS)
        {
            if ( (cb == 9) &&
                 (sz[6] == TEXT('0')) &&
                 ((sz[7] == TEXT('1')) || (sz[7] == TEXT('d')) ||
                  (sz[7] == TEXT('D'))) )
            {
                meEto = ETO_RTLREADING;
            }
        }
    }

    if (!FindTrayEtc())
        return (-1);

    hInstLib = LoadLibrary(szDllName);
    if (hInstLib == NULL)
    {
        return (-1);
    }

    fpRegHookWindow      = (REGHOOKPROC)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_REGISTERHOOK));
    fpStartShell         = (PROC)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_STARTSHELL));
    fpStopShell          = (PROC)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_STOPSHELL));

    fpGetLastFocus       = (FPGETLASTFOCUS)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETLASTFOCUS));
    fpGetLastActive      = (FPGETLASTACTIVE)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETLASTACTIVE));
    fpSetNotifyWnd       = (FPSETNOTIFYWND)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_SETNOTIFYWND));

    fpGetIMEStat         = (FPGETIMESTAT)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETIMESTAT));
    fpGetIMEMenu         = (FPGETIMEMENU)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETIMEMENU));
    fpGetLayout          = (FPGETLAYOUT)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETLAYOUT));
    fpBuildIMEMenu       = (FPBUILDIMEMENU)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_BUILDIMEMENU));
    fpGetIMEMenuItemID   = (FPGETIMEMENUITEMID)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETIMEMENUITEMID));
    fpDestroyIMEMenu     = (FPDESTROYIMEMENU)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_DESTROYIMEMENU));
    fpSetIMEMenuItemData = (FPSETIMEMENUITEMDATA)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_SETIMEMENUITEMDATA));
    fpGetConsoleImeWnd   = (FPGETCONSOLEIMEWND)GetProcAddress(hInstLib,
                                           MAKEINTRESOURCEA(ORD_GETCONSOLEIMEWND));

    fpGetDefaultImeMenuItem  = (FPGETDEFAULTIMEMENUITEM)GetProcAddress(hInstLib, 
                                       MAKEINTRESOURCEA(ORD_GETDEFAULTIMEMENUITEM));

    if (!fpRegHookWindow    || !fpStartShell     || !fpStopShell    ||
        !fpGetLastFocus     || !fpGetLastActive  || !fpGetIMEStat   ||
        !fpGetIMEMenu       || !fpGetLayout      || !fpBuildIMEMenu ||
        !fpGetIMEMenuItemID || !fpDestroyIMEMenu || !fpSetIMEMenuItemData ||
        !fpGetConsoleImeWnd || !fpGetDefaultImeMenuItem)
    {
        FreeLibrary(hInstLib);

        fpRegHookWindow = NULL;
        fpStartShell = NULL;
        fpStopShell = NULL;

        return (-1);
    }

    hdc = GetDC(hwnd);
    TranslateCharsetInfo( (LPVOID)(LONG)GetTextCharsetInfo(hdc, NULL, 0),
                          &cs,
                          TCI_SRCCHARSET );
    fsShell = cs.fs.fsCsb[0];
    ReleaseDC(hwnd, hdc);

    cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    cySmIcon = GetSystemMetrics(SM_CYSMICON);

    Internat_ValidateRegistryPreload();

    Internat_SetInputHooks(hwnd);

    Internat_LoadKeyboardLayouts();

    Internat_ManageMlngInfo(hwnd, CREATE_MLNGINFO);
    if ((hdpaInfoList == NULL) || (himIndicators == NULL))
    {
        return (-1);
    }

    if (!Internat_SetIndicHooks(hwnd))
    {
        return (-1);
    }

    PostMessage( HWND_BROADCAST,
                 WM_IME_SYSTEM,
                 (WPARAM)IMS_SETOPENSTATUS,
                 (LPARAM)0 );

    //
    //  Does not want Internat's own IME displayed.
    //
    hDefIMC = ImmAssociateContext(hwnd, (HIMC)NULL);

    return (0);
}

//***   Internat_OnTaskbarCreated -- re-init things when shell restarts
// NOTES
//  we should figure out how to share this fully w/ Internat_OnCreate.
// for now i'm only sharing the helpers, not the ordering (i'm afraid
// to rearrange things lest there be non-obvious order dependencies
// in when the hwnd is created, when we register, etc.).
int Internat_OnTaskbarCreated(HWND hwnd, WPARAM wParam, LPARAM lparam)
{
    if (bInternatActive)
    {
        // go into restart state so we'll re-add Shell_NotifyIcon
        bInternatActive = B3_RESTART;
    }

    //
    // all our hwnd's changed, gotta find, re-register, re-hook, etc.
    //

    FindTrayEtc();

    Internat_SetInputHooks(hwnd);
    if (!Internat_SetIndicHooks(hwnd))
        return -1;

    Internat_onSettingChange(hwnd, TRUE);

    // do we need an IMS_SETOPENSTATUS here (like in OnCreate)?

    return 0;
}


//***   Internat_SetInputHooks --
//
void Internat_SetInputHooks(HWND hwnd)
{
    if (!AttachThreadInput( GetWindowThreadProcessId(hwnd, NULL),
                            GetWindowThreadProcessId(hwndTray, NULL),
                            TRUE ))
    {
        MessageBeep(MB_OK);
    }
    return;
}

//***   Internat_SetIndicHooks -- (re)register the indicator DLL
//
BOOL Internat_SetIndicHooks(HWND hwnd)
{
    if (fpRegHookWindow)
        if (!(fpRegHookWindow)(hwnd, TRUE))
        {
            return FALSE;
        }

    if (fpStartShell)
        fpStartShell();

    //
    //  Send the notify window to the hook. This is to skip status update
    //  when the notify window is getting the focus.
    //
    if (fpSetNotifyWnd) {
        struct NotifyWindows nw = {
            sizeof nw,
            hwndNotify,
            hwndTray,
        };
        (fpSetNotifyWnd)(&nw);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_SendLangIndicatorMsg
//
//  Using an image list to store the icons g_hIconImageList,
//  we can get the icons by index calling ImageList_ExtractIcon.
//
////////////////////////////////////////////////////////////////////////////

void Internat_SendLangIndicatorMsg(
    HWND hwnd,
    HKL dwHkl,
    DWORD dwMessage)
{
    NOTIFYICONDATA tnd;
    PMLNGINFO pMlngInfo;
    BOOL bFound = FALSE;
    int nIndex, nCount;

    if (dwHkl == hklCurrent)
    {
        return;
    }

    tnd.uCallbackMessage = WM_LANGUAGE_INDICATOR;
    tnd.cbSize           = sizeof(NOTIFYICONDATA);
    tnd.hWnd             = hwnd;
    tnd.uID              = LANG_INDICATOR_ID;
    tnd.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;

    //
    //  Deleting is a special case.
    //
    if ((dwHkl == 0) && (dwMessage == NIM_DELETE))
    {
        tnd.hIcon = NULL;
        tnd.szTip[0] = TEXT('\0');
        Shell_NotifyIcon(dwMessage, &tnd);
        hklCurrent--;                            // make it bogus
        return;
    }

    if (hdpaInfoList == NULL)
    {
        return;
    }
    nCount = DPA_GetPtrCount(hdpaInfoList);

    // BugBug#246166 - Some of IME installing(PRC IME98) didn't notify
    // the Language updating information.
    if (GetKeyboardLayoutList(0, NULL) != nCount)
    {
        Internat_ManageMlngInfo(hwnd, UPDATE_MLNGINFO);
        nCount = DPA_GetPtrCount(hdpaInfoList);
    }

    for (nIndex = 0; nIndex < nCount; nIndex++)
    {
        pMlngInfo = DPA_GetPtr(hdpaInfoList, nIndex);
        if (pMlngInfo->dwHkl == dwHkl)
        {
            bFound = TRUE;
            break;
        }
    }

    //
    //  If we can't find it, take the first one in the list.
    //
    if (!bFound)
    {
        pMlngInfo = DPA_GetPtr(hdpaInfoList, 0);
    }

    if (!pMlngInfo)
    {
        return;
    }

    tnd.hIcon = ImageList_ExtractIcon( hinst,
                                       himIndicators,
                                       pMlngInfo->nIconIndex );
    lstrcpyn(tnd.szTip, pMlngInfo->szTip, ARRAYSIZE(tnd.szTip));
    hklCurrent = pMlngInfo->dwHkl;               // update current hkl
    Shell_NotifyIcon(dwMessage, &tnd);
    DestroyIcon(tnd.hIcon);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_LanguageIndicator
//
//  Only allows you to delete the indicator if it's present and
//  add the indicator if it's not present. Also does indicator start up.
//
////////////////////////////////////////////////////////////////////////////

void Internat_LanguageIndicator(
    HWND hwnd,
    DWORD dwFlag)
{
    HKL dwHkl = 0;

    if (bInternatActive)
    {
        if (dwFlag == NIM_DELETE)
        {
            Internat_SendLangIndicatorMsg(hwnd, dwHkl, dwFlag);
            bInternatActive = FALSE;
        }
        if (bInternatActive == B3_RESTART)
            goto Ladd;
    }
    else
    {
Ladd:
        if (dwFlag == NIM_ADD)
        {
            // Get current thread's keyboard layout
            dwHkl = GetKeyboardLayout(0);
            hklCurrent = dwHkl - 1; // make it bogus ?
            Internat_SendLangIndicatorMsg(hwnd, dwHkl, dwFlag);
            bInternatActive = TRUE;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetIconFromFile
//
//  Extracts an Icon from a file if possible.
//
////////////////////////////////////////////////////////////////////////////

HICON Internat_GetIconFromFile(
    HIMAGELIST himIndicators,
    LPTSTR lpszFileName,
    UINT uIconIndex)
{
    int cx, cy;
    HICON hicon;

    ImageList_GetIconSize(himIndicators, &cx, &cy);
    if (cx > GetSystemMetrics(SM_CXSMICON))
    {
        ExtractIconEx(lpszFileName, uIconIndex, &hicon, NULL, 1);
    }
    else
    {
        ExtractIconEx(lpszFileName, uIconIndex, NULL, &hicon, 1);
    }

    return (hicon);
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_CreateIcon
//
////////////////////////////////////////////////////////////////////////////

HICON Internat_CreateIcon(
    HWND hwnd,
    WORD langID)
{
    HBITMAP hbmColour;
    HBITMAP hbmMono;
    HBITMAP hbmOld;
    HICON hicon = NULL;
    ICONINFO ii;
    RECT rc;
    DWORD rgbText;
    DWORD rgbBk = 0;
    UINT i;
    HDC hdc;
    HDC hdcScreen;
    LOGFONT lf;
    HFONT hfont;
    HFONT hfontOld;
    TCHAR szData[20];


    //
    //  Get the indicator by using the first 2 characters of the
    //  abbreviated language name.
    //
    if (GetLocaleInfo( MAKELCID(langID, SORT_DEFAULT),
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szData,
                       ARRAYSIZE(szData) ))
    {
        //
        //  Only use the first two characters.
        //
        szData[2] = TEXT('\0');
    }
    else
    {
        //
        //  Id wasn't found.  Use question marks.
        //
        szData[0] = TEXT('?');
        szData[1] = TEXT('?');
        szData[2] = TEXT('\0');
    }

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
    {
        if ((hfont = CreateFontIndirect(&lf)))
        {
            hdcScreen = GetDC(NULL);
            hdc = CreateCompatibleDC(hdcScreen);
            hbmColour = CreateCompatibleBitmap(hdcScreen, cxSmIcon, cySmIcon);
            ReleaseDC(NULL, hdcScreen);
            if (hbmColour && hdc)
            {
                hbmMono = CreateBitmap(cxSmIcon, cySmIcon, 1, 1, NULL);
                if (hbmMono)
                {
                    hbmOld    = SelectObject(hdc, hbmColour);
                    rc.left   = 0;
                    rc.top    = 0;
                    rc.right  = cxSmIcon;
                    rc.bottom = cySmIcon;

                    rgbBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                    rgbText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

                    ExtTextOut( hdc,
                                rc.left,
                                rc.top,
                                ETO_OPAQUE,
                                &rc,
                                TEXT(""),
                                0,
                                NULL );
                    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
                    hfontOld = SelectObject(hdc, hfont);
                    DrawText( hdc,
                              szData,
                              2,
                              &rc,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE );
                    SelectObject(hdc, hbmMono);
                    PatBlt(hdc, 0, 0, cxSmIcon, cySmIcon, BLACKNESS);
                    SelectObject(hdc, hbmOld);

                    ii.fIcon    = TRUE;
                    ii.xHotspot = 0;
                    ii.yHotspot = 0;
                    ii.hbmColor = hbmColour;
                    ii.hbmMask  = hbmMono;
                    hicon       = CreateIconIndirect(&ii);

                    DeleteObject(hbmMono);
                    SelectObject(hdc, hfontOld);
                }
            }
            DeleteObject(hbmColour);
            DeleteDC(hdc);
            DeleteObject(hfont);
        }
    }

    return (hicon);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_ManageMlngInfo
//
//  Manages Shell Multilingual Info. Creates and maintains "database"
//  of Multilingual components in the system.
//  The Information is initially taken from the registry.
//
//  The "database" is a DPA of the languages in System and IMAGELIST of the
//  indicator Icons.
//
//  Function is called at different times for different functionality:
//      1. When application is started
//      2. When changes are made to registry information.
//         This is done through the HSHELL_LANGUAGE shell hook.
//      3. When Application closes.
//
//  wFlag is one of the following:
//
//    DESTROY_MLNGINFO
//      We're finished, so clean it all up.
//
//    CREATE_MLNGINFO
//      We've just started, interrogate the system and get the info we need.
//
//    UPDATE_MLNGINFO
//      Multilingual information has just changed, so rebuild it.
//      Update involves destroying the old DPA and ImageList stuff and
//      recreating it again.
//
//  NOTE: Even if the system has only one language we create the DPA
//        and image list. When adding extra languages to the system,
//        an UPDATE will just have to be done.
//
////////////////////////////////////////////////////////////////////////////

void Internat_ManageMlngInfo(
    HWND hwnd,
    WORD wFlag)
{
    HKL *pLanguages;
    DWORD dwRegValue;
    long cb;
    UINT uCount;
    UINT uLangs;
    int i;
    HICON hIcon;
    HKEY hkey;
    TCHAR szRegText[256];
    LPTSTR pszRT;
    PMLNGINFO pMlngInfo;
    PMLNGINFO *ppMlngInfo;
    BOOL fNeedInd = FALSE;

    uLangs = GetKeyboardLayoutList(0, NULL);

    //
    //  Make sure data structures are present.
    //
    if (((!hdpaInfoList) || (!himIndicators)) && (wFlag != CREATE_MLNGINFO))
    {
        return;
    }

    if (wFlag != CREATE_MLNGINFO)
    {
        if (wFlag == DESTROY_MLNGINFO)
        {
            if (g_bIMEIndicator)
            {
                Internat_SendIMEIndicatorMsg(hwnd, 0, NIM_DELETE);
                g_bIMEIndicator = FALSE;
            }
            Internat_LanguageIndicator(hwnd, NIM_DELETE);
        }

        ppMlngInfo = (PMLNGINFO *)DPA_GetPtrPtr(hdpaInfoList);

        if (!ppMlngInfo)
        {
            return;
        }

        for (i = DPA_GetPtrCount(hdpaInfoList); i > 0; --i, ++ppMlngInfo)
        {
            LocalFree(*ppMlngInfo);
            ImageList_Remove(himIndicators, 0);
        }

        DPA_Destroy(hdpaInfoList);
        ImageList_Destroy(himIndicators);
    }

    if (wFlag != DESTROY_MLNGINFO)
    {
        if (!(hdpaInfoList = DPA_Create(0)))
        {
            return;
        }

        himIndicators = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          TRUE,
                                          0,
                                          0 );
        if (!himIndicators)
        {
            DPA_Destroy(hdpaInfoList);
            hdpaInfoList = NULL;
            himIndicators = NULL;
            return;
        }

        pLanguages = (HKL *)LocalAlloc(LPTR, uLangs * sizeof(HKL));
        GetKeyboardLayoutList(uLangs, (HKL *)pLanguages);

        //
        //  pLanguages contains all the HKLs in the system.
        //  Put everything together in the DPA and Image List.
        //
        for (uCount = 0; uCount < uLangs; uCount++)
        {
            DWORD dwIMEDesc = 0;

            if ((HIWORD((ULONG_PTR)pLanguages[uCount]) & 0xf000) == 0xe000)
            {
                dwIMEDesc = ImmGetDescription(pLanguages[uCount], NULL, 0L);
            }

            //
            //  Get the Input Locale name.
            //
            dwRegValue = ((DWORD)(LOWORD((ULONG_PTR)pLanguages[uCount])));
            if (!GetLocaleInfo( dwRegValue,
                                LOCALE_SLANGUAGE,
                                szRegText,
                                ARRAYSIZE(szRegText) ))
            {
                LoadString(hinst, IDS_UNKNOWN, szRegText, ARRAYSIZE(szRegText));
            }

            //
            //  Attach the Layout name if it's not the default.
            //
            if (HIWORD((ULONG_PTR)pLanguages[uCount]) != LOWORD((ULONG_PTR)pLanguages[uCount]))
            {
                pszRT = szRegText + lstrlen(szRegText);
                Internat_GetKbdLayoutName((DWORD)(ULONG_PTR)pLanguages[uCount],
                                           pszRT,
                                           (int)(ARRAYSIZE(szRegText) - (pszRT - szRegText)));
            }

            if (dwIMEDesc)
            {
                cb = (long)((dwIMEDesc + 1) * sizeof(TCHAR));
                if (!(pMlngInfo = LocalAlloc(LPTR, sizeof(MLNGINFO) + cb)))
                {
                    goto MError1;
                }
                ImmGetDescription( pLanguages[uCount],
                                   pMlngInfo->szTip,
                                   cb / sizeof(TCHAR) );
            }
            else
            {
                cb = (lstrlen(szRegText) + 1) * sizeof(TCHAR);
                if (!(pMlngInfo = LocalAlloc(LPTR, sizeof(MLNGINFO) + cb)))
                {
                    goto MError1;
                }
                lstrcpy(pMlngInfo->szTip, szRegText);
            }
            pMlngInfo->dwHkl = pLanguages[uCount];

            if ((HIWORD((ULONG_PTR)pMlngInfo->dwHkl) & 0xf000) == 0xe000)
            {
                TCHAR szIMEFile[32];   // assume long filename up to 32 byte

                // Is this really a good way ?
                if (LOWORD((ULONG_PTR)pMlngInfo->dwHkl) == 0x0404 ||
                    LOWORD((ULONG_PTR)pMlngInfo->dwHkl) == 0x0411 ||
                    LOWORD((ULONG_PTR)pMlngInfo->dwHkl) == 0x0412 ||
                    LOWORD((ULONG_PTR)pMlngInfo->dwHkl) == 0x0804)
                {
                    fNeedInd = TRUE;
                    pMlngInfo->dwIMEFlag |= MMIF_IME;
                }

                if (ImmGetIMEFileName( pMlngInfo->dwHkl,
                                       szIMEFile,
                                       ARRAYSIZE(szIMEFile) ))
                {
                    //
                    //  First one of the file.
                    //
                    hIcon = Internat_GetIconFromFile(himIndicators, szIMEFile, 0);
                }
                else
                {
                    hIcon = Internat_CreateIcon(hwnd, LOWORD((ULONG_PTR)pLanguages[uCount]));
                }
            }
            else             // for non-ime layout
            {
                hIcon = Internat_CreateIcon(hwnd, LOWORD((ULONG_PTR)pLanguages[uCount]));
            }

            pMlngInfo->nIconIndex = ImageList_AddIcon(himIndicators, hIcon);
            DestroyIcon(hIcon);

            if ((i = DPA_InsertPtr(hdpaInfoList, 0x7fff, pMlngInfo)) == -1)
            {
MError1:
                //
                //  Cover our tracks.
                //
                LocalFree((HLOCAL)pLanguages);

                ppMlngInfo = (PMLNGINFO *)DPA_GetPtrPtr(hdpaInfoList);

                if (!ppMlngInfo)
                {
                    return;
                }

                for (i = DPA_GetPtrCount(hdpaInfoList) - 1;
                     i > 0;
                     --i, ++ppMlngInfo)
                {
                    LocalFree(*ppMlngInfo);
                    ImageList_Remove(himIndicators, 0);
                }

                //
                //  Destroy everything.
                //
                DPA_Destroy(hdpaInfoList);
                ImageList_Destroy(himIndicators);

                //
                //  We are DEAD.  Something major has gone wrong, so
                //  remove any form of the language indication.
                //
                hdpaInfoList = NULL;
                himIndicators = NULL;

                //
                //  No indicator.
                //
                Internat_LanguageIndicator(hwnd, NIM_DELETE);
                return;
            }
        }

        if (fNeedInd)
        {
            Internat_LoadIMEIndicatorIcon(hInstLib, nIMEIconIndex);
        }

        if (pLanguages && ((HIWORD(pLanguages[0]) & 0xf000) == 0xe000))
        {
            if (!g_bIMEIndicator)
            {
                Internat_SendIMEIndicatorMsg(hwnd, pLanguages[0], NIM_ADD);
                g_bIMEIndicator = TRUE;
            }
        }

        //
        //  Clean up, and add the Indicator.
        //
        LocalFree((HLOCAL)pLanguages);

        //
        //  If uLang == 1 and the only layout is the IME, then we just put
        //  the IME indicator.
        //
        if (uLangs < 2)
        {
            Internat_LanguageIndicator(hwnd, NIM_DELETE);
        }
        else
        {
            Internat_LanguageIndicator(hwnd, NIM_ADD);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_HandleLanguageMsg
//
//  Tell the indicator if there has been a language switch in the system.
//
////////////////////////////////////////////////////////////////////////////

int Internat_HandleLanguageMsg(
    HWND hwnd,
    HKL dwHkl)
{
    //
    //  If dwHkl == 0, then reread registry information.
    //  Otherwise, change indicator to this language.
    //
    if (dwHkl == 0)
    {
        Internat_ManageMlngInfo(hwnd, UPDATE_MLNGINFO);
    }
    else
    {
        Internat_SendLangIndicatorMsg(hwnd, dwHkl, NIM_MODIFY);
    }

    if (!dwHkl)
    {
        return (0);
    }

    if ((HIWORD((ULONG_PTR)dwHkl) & 0xf000) == 0xe000)
    {
        if (!g_bIMEIndicator)
        {
            Internat_SendIMEIndicatorMsg(hwnd, dwHkl, NIM_ADD);
            g_bIMEIndicator = TRUE;
        }
        else
        {
            Internat_SendIMEIndicatorMsg(hwnd, dwHkl, NIM_MODIFY);
        }
    }
    else if (g_bIMEIndicator)
    {
        Internat_SendIMEIndicatorMsg(hwnd, 0, NIM_DELETE);
        g_bIMEIndicator = FALSE;
    }

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_HandleLangMenuMeasure
//
//  Does the calculation for the language owner drawn menu item.
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_HandleLangMenuMeasure(
    HWND hwnd,
    LPMEASUREITEMSTRUCT lpmi)
{
    NONCLIENTMETRICS ncm;
    SIZE size;
    PMLNGINFO pMlng  = NULL;
    HFONT hMenuFont, hOldFont;
    HDC hDC;
    UINT uCount = 0;

    if (lpmi->CtlID != 0)
    {
        return (FALSE);
    }

    uCount = GetKeyboardLayoutList(0, NULL);

    pMlng = DPA_GetPtr(hdpaInfoList, lpmi->itemID - IDM_LANG_MENU_START);
    if (!pMlng)
    {
        return (FALSE);
    }

    //
    //  Get the Menu font.
    //
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);

    hMenuFont = CreateFontIndirect(&ncm.lfMenuFont);

    if (!hMenuFont)
    {
        return (FALSE);
    }

    hDC      = GetWindowDC(hwnd);
    hOldFont = SelectObject(hDC, hMenuFont);

    //
    //  Get the length of our string as it would appear in the menu.
    //
    GetTextExtentPoint(hDC, pMlng->szTip, lstrlen(pMlng->szTip), &size);

    //
    //  Total width is Icon width + 3 check marks + the text width.
    //
    lpmi->itemWidth = 3 * GetSystemMetrics(SM_CXMENUCHECK) +
                      GetSystemMetrics(SM_CYSMICON) + size.cx;

    SelectObject(hDC, hOldFont);
    ReleaseDC(hwnd, hDC);
    DeleteObject(hMenuFont);

    //
    //  Height is fairly straight forward, the larger of the two,
    //  text or Icon.
    //
    lpmi->itemHeight = ((size.cy > GetSystemMetrics(SM_CYSMICON))
                            ? size.cy
                            : GetSystemMetrics(SM_CYSMICON)) + 2;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_HandleLangMenuDraw
//
//  Draws the owner drawn menu item.
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_HandleLangMenuDraw(
    HWND hwnd,
    LPDRAWITEMSTRUCT lpdi)
{
    DWORD dwRop;
    int checkMarkSize;
    int nIndex, x, y;
    PMLNGINFO pMlng = NULL;

    if (lpdi->CtlID != 0)
    {
        return (FALSE);
    }

    nIndex = (int)(lpdi->itemID - IDM_LANG_MENU_START);
    pMlng  = DPA_GetPtr(hdpaInfoList, nIndex);

    if (!pMlng)
    {
        return (FALSE);
    }

    checkMarkSize = GetSystemMetrics(SM_CXMENUCHECK);

    if (lpdi->itemState & ODS_SELECTED)
    {
        SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
        dwRop = SRCSTENCIL;
    }
    else
    {
        SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
        SetBkColor(lpdi->hDC, GetSysColor(COLOR_MENU));
        dwRop = SRCAND;
    }

    //
    //  Write out the text.
    //
    ExtTextOut( lpdi->hDC,
                lpdi->rcItem.left + (2 * checkMarkSize) +
                    GetSystemMetrics(SM_CXSMICON),
                lpdi->rcItem.top + 3,
                ETO_OPAQUE | meEto,
                &lpdi->rcItem,
                (LPTSTR)pMlng->szTip,
                lstrlen((LPTSTR)pMlng->szTip),
                NULL );

    //
    //  Draw the Icon.
    //
    ImageList_Draw( himIndicators,
                    nIndex,
                    lpdi->hDC,
                    lpdi->rcItem.left + (checkMarkSize),
                    lpdi->rcItem.top + 1,
                    ILD_TRANSPARENT );

    if (lpdi->itemState & ODS_CHECKED)
    {
        //
        //  Can't use DrawFrameControl, DFC doesn't allow colour changing.
        //
        HBITMAP hBmp, hBmpSave;
        BITMAP bm;
        HDC hDCBmp;
        DWORD textColorSave;
        DWORD bkColorSave;

        hDCBmp = CreateCompatibleDC(lpdi->hDC);
        if (hDCBmp)
        {
            hBmp = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
            GetObject(hBmp, sizeof(bm), &bm);
            hBmpSave = SelectObject(hDCBmp, hBmp);
            x = lpdi->rcItem.left;      // + checkMarkSize;
            y = (lpdi->rcItem.bottom + lpdi->rcItem.top - bm.bmHeight) / 2;

            textColorSave = SetTextColor(lpdi->hDC, 0x00000000L);
            bkColorSave   = SetBkColor(lpdi->hDC, 0x00ffffffL);

            BitBlt( lpdi->hDC,
                    x,
                    y,
                    bm.bmWidth,
                    bm.bmHeight,
                    hDCBmp,
                    0,
                    0,
                    dwRop );

            SetTextColor(lpdi->hDC, textColorSave);
            SetBkColor(lpdi->hDC, bkColorSave);

            SelectObject(hDCBmp, hBmpSave);
            DeleteObject(hBmp);
            DeleteDC(hDCBmp);
        }
    }

    if (lpdi->itemState & ODS_FOCUS)
    {
        DrawFocusRect(lpdi->hDC, &lpdi->rcItem);
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_CreateLanguageMenu
//
//  This is the real Menu brought up by clicking on the language indicator
//  with the left mouse button. This will be owner drawn later. For now it
//  will allow you to select a language for your last active captioned app.
//
////////////////////////////////////////////////////////////////////////////

void Internat_CreateLanguageMenu(
    HWND hwnd,
    LPARAM lParam)
{
    HMENU hLangMenu;
    BOOL bCheckIt = FALSE;
    UINT uMenuId = IDM_LANG_MENU_START;
    int nIndex;
    int cmd;
    TCHAR szMenuOption[MENUSTRLEN];
    PMLNGINFO pMlngInfo;
    TPMPARAMS tpm;
    LOCALESIGNATURE ls;
    HWND hwndT, hwndP;
#ifndef NO_SERIALIZED_ACTIVATE
    DWORD dwThread, dwThreadLang;
#endif
    DWORD dwThreadActivate;
    BOOL bFontSig = 0;
    HWND hwndForActivate;
    HWND hwndForLang;
    int nXPos;

#ifdef USE_MIRRORING
    DWORD dwLayout;

    GetProcessDefaultLayout(&dwLayout);
#endif

    bInLangMenu = TRUE;
    hLangMenu = CreatePopupMenu();

    cmd = DPA_GetPtrCount(hdpaInfoList);

    // BugBug#246166 - Some of IME installing(PRC IME98) didn't notify
    // the Language updating information.
    if (GetKeyboardLayoutList(0, NULL) != cmd)
    {
        Internat_ManageMlngInfo(hwnd, UPDATE_MLNGINFO);
        cmd = DPA_GetPtrCount(hdpaInfoList);
    }

    if (cmd == -1)
    {
        bInLangMenu = FALSE;
        return;
    }

    for (nIndex = 0; nIndex < cmd; nIndex++)
    {
        pMlngInfo = DPA_FastGetPtr(hdpaInfoList, nIndex);
        lstrcpyn(szMenuOption, pMlngInfo->szTip, ARRAYSIZE(szMenuOption));

        if (pMlngInfo->dwHkl == hklCurrent)
        {
            bCheckIt = TRUE;
        }

        InsertMenu( hLangMenu,
                    (UINT)-1,
                    MF_BYPOSITION | MF_OWNERDRAW,
                    uMenuId,
                    (LPCTSTR)szMenuOption );
        if (bCheckIt)
        {
            CheckMenuItem(hLangMenu, uMenuId, MF_CHECKED);
            bCheckIt = FALSE;
        }
        uMenuId++;
    }

    SetForegroundWindow(hwnd);
    GetClientRect(hwndNotify, &tpm.rcExclude);
    MapWindowPoints(hwndNotify, NULL, (LPPOINT)&tpm.rcExclude, 2);

    if (uTaskBarEdge == ABE_LEFT)
        nXPos = tpm.rcExclude.right;
    else
    {
        nXPos = tpm.rcExclude.left;

#ifdef USE_MIRRORING
        if ((dwLayout & LAYOUT_RTL) && (uTaskBarEdge != ABE_RIGHT))
            nXPos = tpm.rcExclude.right;
#endif
    }

    cmd = TrackPopupMenuEx( hLangMenu,
                            TPM_VERTICAL | TPM_RETURNCMD,
                            nXPos,
                            tpm.rcExclude.top,
                            hwnd,
                            NULL );

    bInLangMenu = FALSE;
    hwndForLang = Internat_GetCurrentFocusWnd();

    if (cmd && (cmd != -1))
    {
        cmd -= IDM_LANG_MENU_START;
        pMlngInfo = DPA_FastGetPtr(hdpaInfoList, cmd);

        if (hwndForLang == NULL)
        {
            MessageBeep(MB_ICONEXCLAMATION);
            return;
        }

        //
        //  Send a WM_INPUTLANGCHANGEREQUEST msg to change the apps language.
        //  This also generates the HSHELL_LANGUAGE Hook that we
        //  monitor to provide the correct indicator.
        //
        hwndForActivate = hwndForLang;
        hwndForActivate = Internat_GetTopLevelWindow(hwndForActivate);

        //
        // BugBug#333120 - The serialization problem can be occured in case of
        // the different thread window. So set the activate window as the
        // current window.
        //
        dwThreadActivate = GetWindowThreadProcessId(hwndForActivate, NULL);
        if (hwndForLang && dwThreadActivate != GetWindowThreadProcessId(hwndForLang, NULL))
            hwndForActivate = hwndForLang;

        //
        //  If it's a dialog, use it.
        //
        hwndForActivate = GetLastActivePopup(hwndForActivate);

        if (GetLocaleInfo( (DWORD)(LOWORD((ULONG_PTR)pMlngInfo->dwHkl)),
                           LOCALE_FONTSIGNATURE,
                           (LPTSTR)&ls,
                           34 ))
        {
            if (fsShell & ls.lsCsbSupported[0])
            {
                bFontSig = 1;
            }
        }

#ifndef NO_SERIALIZED_ACTIVATE
        dwThread = GetWindowThreadProcessId(hwnd, NULL);
        dwThreadLang = GetWindowThreadProcessId(hwndForActivate, NULL);
        if (dwThread != dwThreadLang)
        {
            if (!AttachThreadInput(dwThread, dwThreadLang, TRUE))
            {
                MessageBeep(MB_OK);
            }
        }
#endif

        SetForegroundWindow(hwndForActivate);

        PostMessage( hwndForLang,
                     WM_INPUTLANGCHANGEREQUEST,
                     (WPARAM)bFontSig,
                     (LPARAM)pMlngInfo->dwHkl );
        Internat_SetInputHooks(hwnd);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_CreateOtherIndicatorMenu
//
//  Right Mouse button click menu on indicator.
//  Strings need to go into cabinet if necessary.
//
////////////////////////////////////////////////////////////////////////////

void Internat_CreateOtherIndicatorMenu(
    HWND hwnd,
    LPARAM lParam)
{
    HMENU hMenu;
    int cmd;
    TPMPARAMS tpm;
    TCHAR szTextString[MENUSTRLEN];

    hMenu = CreatePopupMenu();

    LoadString(hinst, IDS_WHATSTHIS, szTextString, ARRAYSIZE(szTextString));
    InsertMenu( hMenu,
                (UINT)-1,
                MF_STRING | MF_BYPOSITION,
                IDM_RMENU_WHATSTHIS,
                (LPCTSTR)szTextString );

    InsertMenu( hMenu,
                (UINT)-1,
                MF_BYPOSITION | MF_SEPARATOR,
                0,
                (LPCTSTR)NULL );

    LoadString(hinst, IDS_PROPERTIES, szTextString, ARRAYSIZE(szTextString));
    InsertMenu( hMenu,
                (UINT)-1,
                MF_STRING | MF_BYPOSITION,
                IDM_RMENU_PROPERTIES,
                (LPCTSTR)szTextString );

    GetClientRect(hwndNotify, &tpm.rcExclude);
    MapWindowPoints(hwndNotify, NULL, (LPPOINT)&tpm.rcExclude, 2);
    tpm.cbSize = sizeof(tpm);

    //
    //  The following line is necessary for FE to keep consistency.
    //  Without this, the icon can be messed up when no focus window
    //  exists.
    //
    bInLangMenu = TRUE;

    SetActiveWindow(hwnd);
    cmd = TrackPopupMenuEx( hMenu,
                            TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                            tpm.rcExclude.left,
                            tpm.rcExclude.top,
                            hwnd,
                            &tpm );

    bInLangMenu = FALSE;

    if (cmd && (cmd != -1))
    {
        switch (cmd)
        {
            case ( IDM_RMENU_PROPERTIES ) :
            {
                LoadStringA( hinst,
                             IDS_CPL_INTL,
                             (LPSTR)szTextString,
                             MENUSTRLEN );
                WinExec((LPSTR)szTextString, SW_SHOWNORMAL);
                break;
            }
            case ( IDM_EXIT ) :
            {
                DestroyWindow(hwnd);
                break;
            }
            case ( IDM_RMENU_WHATSTHIS ) :
            {
                WinHelp( hwnd,
                         szHelpFile,
                         HELP_CONTEXTPOPUP,
                         IDH_KEYB_INDICATOR_ON_TASKBAR );
                break;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetCurrentFocusWnd
//
////////////////////////////////////////////////////////////////////////////

HWND Internat_GetCurrentFocusWnd(void)
{
    HWND hwndFocus = (fpGetLastFocus());

    if (!IsWindow(hwndFocus))
    {
        hwndFocus = (fpGetLastActive());
#ifdef WINNT
        // BugBug#316479 - After close application, the last focus window can
        // be the same with the previous application window handle that is
        // already destroyed. This scenario happen when the focus is on the
        // TrayBar after closing application. We ignore all focus setting for
        // TrayBar from CBT Hook. So last focus can't be updated at this time.
        if (!IsWindow(hwndFocus))
        {
            hwndFocus = FindWindow(szProgManWndClass, NULL);
        }
#endif
    }

    return (hwndFocus);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_LoadIMEIndicatorIcon
//
////////////////////////////////////////////////////////////////////////////

void Internat_LoadIMEIndicatorIcon(
    HINSTANCE hInstLib,
    int *ImeIcon)
{
    HICON hIcon;
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(10));
    ImeIcon[0] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(11));
    ImeIcon[1] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(12));
    ImeIcon[2] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(13));
    ImeIcon[3] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(14));
    ImeIcon[4] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(15));
    ImeIcon[5] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
    hIcon = LoadIcon(hInstLib, MAKEINTRESOURCE(16));
    ImeIcon[6] = ImageList_AddIcon(himIndicators, hIcon);
    DestroyIcon(hIcon);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_SendIMEIndicatorMsg
//
////////////////////////////////////////////////////////////////////////////

void Internat_SendIMEIndicatorMsg(
    HWND hwnd,
    HKL dwHkl,
    DWORD dwMessage)
{
    NOTIFYICONDATA tnd;
    PMLNGINFO pMlngInfo;
    BOOL bFound = FALSE;
    int nIndex, nCount;
    int iStat;
    HWND hwndImc;

    tnd.uCallbackMessage = WM_IME_INDICATOR;
    tnd.cbSize           = sizeof(NOTIFYICONDATA);
    tnd.hWnd             = hwnd;
    tnd.uID              = IME_INDICATOR_ID;
    tnd.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;

    if ((dwHkl == 0) && (dwMessage == NIM_DELETE))
    {
        tnd.hIcon = NULL;
        tnd.szTip[0] = TEXT('\0');
        Shell_NotifyIcon(dwMessage, &tnd);
        return;
    }

    if (hdpaInfoList == NULL)
    {
        return;
    }
    nCount = DPA_GetPtrCount(hdpaInfoList);

    for (nIndex = 0; nIndex < nCount; nIndex++)
    {
        pMlngInfo = DPA_GetPtr(hdpaInfoList, nIndex);
        if (pMlngInfo->dwHkl == dwHkl)
        {
            bFound = TRUE;
            break;
        }
    }

    //
    //  If we can't find it, take the first one in the list.
    //
    if (!bFound)
    {
        pMlngInfo = DPA_GetPtr(hdpaInfoList, 0);
    }
    else if (!(pMlngInfo->dwIMEFlag & MMIF_IME))
    {
        return;
    }
    else
    {
        int nIcon;
        BOOL fLngIndicator = bInternatActive;

        //
        //  Delete LANG ICON first when adding the IME indicator.
        //
        if (dwMessage == NIM_ADD && fLngIndicator)
        {
            Internat_LanguageIndicator(hwnd, NIM_DELETE);
        }

        //
        //  Check IME status here.
        //
        iStat = Internat_GetIMEStatus(&hwndImc);

        if (pMlngInfo->dwIMEFlag & MMIF_USE_ICON_INDEX)
        {
            HICON hIcon = NULL;
            TCHAR szIMEFile[32];   // assume long filename up to 32 byte

            if (ImmGetIMEFileName(dwHkl, szIMEFile, sizeof(szIMEFile)))
            {
                hIcon = Internat_GetIconFromFile( himIndicators,
                                                  szIMEFile,
                                                  pMlngInfo->uIMEIconIndex );
            }
            if (!hIcon)
            {
                goto Internat_UseOriginalIcon;
            }

            tnd.hIcon = hIcon;
        }
        else
        {
Internat_UseOriginalIcon:
            if (((DWORD)(ULONG_PTR)dwHkl & 0xf000ffffL) == 0xe0000412L)
            {
                if (iStat == IMESTAT_DISABLED)
                {
                    nIcon = 2;
                }
                else if (iStat & IMESTAT_CLOSE)
                {
                    nIcon = 3;
                }
                else
                {
                    nIcon = (iStat & IMESTAT_NATIVE) ? 5 : 3;
                    if (iStat & IMESTAT_FULLSHAPE)
                    {
                        nIcon++;
                    }
                }
            }
            else
            {
                switch (iStat)
                {
                    case ( IMESTAT_DISABLED ) :
                    default :
                    {
                        //
                        //  Disable.
                        //
                        nIcon = 2;
                        break;
                    }
                    case ( IMESTAT_OPEN ) :
                    {
                        //
                        //  Open.
                        //
                        nIcon = 0;
                        break;
                    }
                    case ( IMESTAT_CLOSE ) :
                    {
                        //
                        //  Close.
                        //
                        nIcon = 1;
                        break;
                    }
                }
            }
            tnd.hIcon = ImageList_ExtractIcon( hinst,
                                               himIndicators,
                                               nIMEIconIndex[nIcon]);
        }

        if (pMlngInfo->dwIMEFlag & MMIF_USE_ATOM_TIP)
        {
            if (!GlobalGetAtomName( pMlngInfo->atomTip,
                                    tnd.szTip,
                                    ARRAYSIZE(tnd.szTip) ))
            {
                goto Internat_UseOriginalTips;
            }
        }
        else
        {
Internat_UseOriginalTips:
            lstrcpyn(tnd.szTip, pMlngInfo->szTip, ARRAYSIZE(tnd.szTip));
        }
        Shell_NotifyIcon(dwMessage, &tnd);
        DestroyIcon(tnd.hIcon);

        //
        //  Put LANG ICON again if already deleted it.
        //
        if (dwMessage == NIM_ADD && fLngIndicator)
        {
            Internat_SendLangIndicatorMsg(hwnd, pMlngInfo->dwHkl, NIM_ADD);
            bInternatActive = TRUE;
        }
    }

    iImeCurStat = iStat;
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_SerializeActivate
//
////////////////////////////////////////////////////////////////////////////

void Internat_SerializeActivate(
    HWND hwnd,
    HWND hwndIMC)
{
    DWORD dwThread, dwThreadIMC;
    HWND hwndForActivate;

#ifndef NO_SERIALIZED_ACTIVATE
    dwThread = GetWindowThreadProcessId(hwnd, NULL);
    dwThreadIMC = GetWindowThreadProcessId(hwndIMC, NULL);
    if (dwThread != dwThreadIMC)
    {
        if (!AttachThreadInput(dwThread, dwThreadIMC, TRUE))
        {
            MessageBeep(MB_OK);
        }
    }
#endif

    //
    //  If it's a dialog, use it.
    //
    hwndForActivate = GetLastActivePopup(Internat_GetTopLevelWindow(hwndIMC));
    SetForegroundWindow(hwndForActivate);

    Internat_SetInputHooks(hwnd);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_CreateRightImeMenu
//
////////////////////////////////////////////////////////////////////////////

BOOL PASCAL IsDefaultMenuItemRemoved(HKL dwHkl, BOOL fRight)
{
    int nCount, nIndex;
    BOOL bFound;
    PMLNGINFO      pMlngInfo;

    nCount = DPA_GetPtrCount(hdpaInfoList);

    for (nIndex = 0; nIndex < nCount; nIndex++) {
        pMlngInfo = DPA_GetPtr(hdpaInfoList, nIndex);
        if (pMlngInfo->dwHkl == dwHkl) {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
        return FALSE;

    if (fRight) {
        if (pMlngInfo->dwIMEFlag & MMIF_REMOVEDEFAULTRIGHTMENUITEMS)
            return TRUE;
    }
    else {
        if (pMlngInfo->dwIMEFlag & MMIF_REMOVEDEFAULTMENUITEMS)
            return TRUE;
    }

    return FALSE;
}

void Internat_CreateRightImeMenu(
    HWND hwnd)
{
    HMENU hMenu;
    int cmd;
    TPMPARAMS tpm;
    TCHAR szTextString[MENUSTRLEN];
    HKL dwhkl;
    DWORD dwThreadId;
    DWORD dwMF;
    HWND hwndIMC;
    BOOL fRemoveDefault;    // Have apps or IME specified to remove default menu ?

    if (Internat_GetIMEStatus(&hwndIMC) == IMESTAT_DISABLED)
    {
        goto Cleanup;
    }

    if (hwndIMC) {
        dwThreadId = GetWindowThreadProcessId(hwndIMC, 0);
    }
    else {
        goto Cleanup;
    }

    dwhkl = GetKeyboardLayout(dwThreadId);

    Internat_GetIMEMenu(hwndIMC, TRUE);

    hMenu = CreatePopupMenu();
    fRemoveDefault = IsDefaultMenuItemRemoved(dwhkl, TRUE);

    Internat_BuildIMEMenu(hMenu, TRUE, fRemoveDefault);

    if (!fRemoveDefault) {
        LoadString(hinst, IDS_IMEHELP, szTextString, ARRAYSIZE(szTextString));

        dwMF = MF_STRING | MF_BYPOSITION;
        if (!Internat_CallIMEHelp(hwndIMC, FALSE))
        {
            dwMF |= MF_GRAYED;
        }

        InsertMenu( hMenu,
                    (UINT)-1,
                    dwMF,
                    IDM_RMENU_IMEHELP,
                    (LPCTSTR)szTextString );

        InsertMenu( hMenu,
                    (UINT)-1,
                    MF_BYPOSITION | MF_SEPARATOR,
                    0,
                    (LPCTSTR)NULL );

        LoadString(hinst, IDS_CONFIGUREIME, szTextString, ARRAYSIZE(szTextString));
        InsertMenu( hMenu,
                    (UINT)-1,
                    MF_STRING | MF_BYPOSITION,
                    IDM_RMENU_PROPERTIES,
                    (LPCTSTR)szTextString );
    }

    GetClientRect(hwndNotify, &tpm.rcExclude);
    MapWindowPoints(hwndNotify, NULL, (LPPOINT)&tpm.rcExclude, 2);
    tpm.cbSize = sizeof(tpm);

    bInLangMenu = TRUE;
    SetActiveWindow(hwnd);
    cmd = TrackPopupMenuEx( hMenu,
                            TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                            tpm.rcExclude.left,
                            tpm.rcExclude.top,
                            hwnd,
                            &tpm );

    DestroyMenu(hMenu);
    bInLangMenu = FALSE;

    if (cmd && (cmd != -1))
    {
        switch (cmd)
        {
            case ( IDM_RMENU_PROPERTIES ) :
            {
                dwhkl = Internat_GetKeyboardLayout(hwndIMC);
                if ((HIWORD((ULONG_PTR)dwhkl) & 0xf000) == 0xe000)
                {
                    Internat_CallConfigureIME(hwndIMC, dwhkl);
                }

                break;
            }
            case ( IDM_RMENU_IMEHELP ) :
            {
                Internat_CallIMEHelp(hwndIMC, TRUE);
                break;
            }
            default:
            {
                if (cmd >= IDM_IME_MENUSTART)
                {
                    UINT uID = Internat_GetIMEMenuItemID(cmd);
                    HWND hwndDefIme = ImmGetDefaultIMEWnd(hwndIMC);

                    Internat_SerializeActivate(hwnd, hwndIMC);

                    fpSetIMEMenuItemData(uID);
                    if (IsWindow(hwndDefIme))
                    {
                        SendMessage( hwndDefIme,
                                     WM_IME_SYSTEM,
                                     IMS_ENDIMEMENU,
                                     (LPARAM)hwndIMC );
                    }
                }
                break;
            }
        }
    }

    Internat_DestroyIMEMenu();
Cleanup:
    return;
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetDefaultImeMenuItem
//
////////////////////////////////////////////////////////////////////////////

int Internat_GetDefaultImeMenuItem()
{
    if (!fpGetDefaultImeMenuItem)
        return 0;

    return (fpGetDefaultImeMenuItem)();
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_CreateImeMenu
//
////////////////////////////////////////////////////////////////////////////

void Internat_CreateImeMenu(
    HWND hwnd)
{
    int nIds, nIdsSoftKbd;
    int cmd;
    HMENU hMenu;
    TCHAR szText[32];
    TPMPARAMS tpm;
    BOOL fShow;
    int istat;
    HWND hwndIMC;
    HKL dwhkl;
    BOOL fRemoveDefault;    // Have apps or IME specified to remove default menu ?

    if ((istat = Internat_GetIMEStatus(&hwndIMC)) == IMESTAT_DISABLED)
    {
        goto Cleanup;
    }

    Internat_GetIMEMenu(hwndIMC, FALSE);

    bInLangMenu = TRUE;
    SetForegroundWindow(hwnd);
    hMenu = CreatePopupMenu();

    dwhkl = Internat_GetKeyboardLayout(hwndIMC);
    fRemoveDefault = IsDefaultMenuItemRemoved(dwhkl, FALSE);

    Internat_BuildIMEMenu(hMenu, FALSE, fRemoveDefault);

    if (!fRemoveDefault) {
        //
        //  In Korean case, don't show OPEN/CLOSE menu.
        //
        nIds = 0;
        if (((DWORD)(ULONG_PTR)dwhkl & 0xf000ffffL) != 0xe0000412L)
        {
            nIds = ((istat == IMESTAT_OPEN) ? IDS_IMECLOSE : IDS_IMEOPEN);
            LoadString(hinst, nIds, szText, ARRAYSIZE(szText));
            InsertMenu( hMenu,
                        (UINT)-1,
                        MF_BYPOSITION,
                        IDM_IME_OPENCLOSE,
                        (LPCTSTR)szText );
        }

        //
        //  Open or close the soft keyboard.
        //
        nIdsSoftKbd = 0;
        if (ImmGetProperty(dwhkl, IGP_CONVERSION) & IME_CMODE_SOFTKBD)
        {
            LRESULT fdwConversion;

            if (IsWindow(hwndIMC))
            {
                HWND hwndDefIme = ImmGetDefaultIMEWnd(hwndIMC);

                if (IsWindow(hwndDefIme))
                {
                    fdwConversion = SendMessage( hwndDefIme,
                                                 WM_IME_SYSTEM,
                                                 IMS_GETCONVERSIONMODE,
                                                 0 );

                    nIdsSoftKbd = ((fdwConversion & IME_CMODE_SOFTKBD)
                                      ? IDS_SOFTKBDOFF
                                      : IDS_SOFTKBDON);
                    LoadString(hinst, nIdsSoftKbd, szText, ARRAYSIZE(szText));
                    InsertMenu( hMenu,
                                (UINT)-1,
                                MF_BYPOSITION,
                                IDM_IME_SOFTKBDONOFF,
                                (LPCTSTR)szText );
                }
            }
        }

        if (nIds || nIdsSoftKbd)
        {
            InsertMenu(hMenu, (UINT)-1, MF_SEPARATOR, 0, 0);
        }
        LoadString(hinst, IDS_IMESHOWSTATUS, szText, ARRAYSIZE(szText));
        InsertMenu( hMenu,
                    (UINT)-1,
                    MF_BYPOSITION,
                    IDM_IME_SHOWSTATUS,
                    (LPCTSTR)szText );

        if ((fShow = Internat_GetIMEShowStatus()) == TRUE)
        {
            CheckMenuItem(hMenu, IDM_IME_SHOWSTATUS, MF_CHECKED);
        }
    }

    GetClientRect(hwndNotify, &tpm.rcExclude);
    MapWindowPoints(hwndNotify, NULL, (LPPOINT)&tpm.rcExclude, 2);
    tpm.cbSize = sizeof(tpm);

    cmd = TrackPopupMenuEx( hMenu,
                            TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                            tpm.rcExclude.left,
                            tpm.rcExclude.top,
                            hwnd,
                            &tpm );

    DestroyMenu(hMenu);
    bInLangMenu = FALSE;

    //
    //  Don't want to activate an app in case of canceling.
    //
    if (cmd && cmd != -1)
    {
        Internat_SerializeActivate(hwnd, hwndIMC);
    }

    switch (cmd)
    {
        case ( IDM_IME_OPENCLOSE ) :
        {
             //
             //  I assume IMC_SETOPENSTATUS will not be hooked by an app.
             //
             if (hwndIMC)
             {
                 BOOL fopen = (nIds == IDS_IMECLOSE) ? FALSE : TRUE;

                 Internat_SetIMEOpenStatus(hwnd, fopen, hwndIMC);
             }
             break;
        }
        case ( IDM_IME_SOFTKBDONOFF ) :
        {
             if (hwndIMC)
             {
                 BOOL fFlag = (nIdsSoftKbd == IDS_SOFTKBDOFF) ? FALSE: TRUE;
                 HWND hwndDefIme = ImmGetDefaultIMEWnd(hwndIMC);

                 if (IsWindow(hwndDefIme))
                 {
                     SendMessage( hwndDefIme,
                                  WM_IME_SYSTEM,
                                  IMS_SETSOFTKBDONOFF,
                                  (LPARAM)fFlag );
                 }
             }
             break;
        }
        case ( IDM_IME_SHOWSTATUS ) :
        {
             Internat_SetIMEShowStatus(hwndIMC, !fShow);
             break;
        }
        default:
        {
            if (cmd >= IDM_IME_MENUSTART)
            {
                UINT uID = Internat_GetIMEMenuItemID(cmd);
                HWND hwndDefIme = ImmGetDefaultIMEWnd(hwndIMC);

                fpSetIMEMenuItemData(uID);
                if (IsWindow(hwndDefIme))
                {
                    SendMessage( hwndDefIme,
                                 WM_IME_SYSTEM,
                                 IMS_ENDIMEMENU,
                                 (LPARAM)hwndIMC );
                }
            }
            break;
        }
    }

    //
    //  Don't move the following three lines from here.
    //
    hwndIMC = Internat_GetTopLevelWindow(hwndIMC);
    SetForegroundWindow(GetLastActivePopup(hwndIMC));
    bInLangMenu = FALSE;

    Internat_DestroyIMEMenu();
    
Cleanup: 
    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetIMEShowStatus
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_GetIMEShowStatus(void)
{
    static CONST TCHAR szInputMethod[] = TEXT("Control Panel\\Input Method");
    static CONST TCHAR szValueName[]   = TEXT("show status");
    TCHAR szValueText[16];
    int cb;
    HKEY hkey;
    BOOL fReturn = TRUE;

    if (RegOpenKey(HKEY_CURRENT_USER, szInputMethod, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(szValueText);
        RtlZeroMemory(szValueText, sizeof(szValueText));
        if (RegQueryValueEx( hkey,
                             szValueName,
                             NULL,
                             NULL,
                             (LPBYTE)szValueText,
                             &cb ) == ERROR_SUCCESS)
        {
            if ((szValueText[0] == TEXT('0')) && (szValueText[1] == 0))
            {
                fReturn = FALSE;
            }
        }
        RegCloseKey(hkey);
    }

    return (fReturn);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_SetIMEShowStatus
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_SetIMEShowStatus(
    HWND hwnd,
    BOOL fShow)
{
    static TCHAR szInputMethod[] = TEXT("Control Panel\\Input Method");
    static TCHAR szValueName[] = TEXT("show status");
    TCHAR szValueText[16];
    int cb;
    HKEY hkey;

    if (fShow)
    {
        szValueText[0] = TEXT('1');
    }
    else
    {
        szValueText[0] = TEXT('0');
    }
    szValueText[1] = 0;

    if (RegOpenKey(HKEY_CURRENT_USER, szInputMethod, &hkey) == ERROR_SUCCESS)
    {
        cb = (lstrlen(szValueText) + 1) * sizeof(TCHAR);
        if (RegSetValueEx( hkey,
                           szValueName,
                           0L,
                           REG_SZ,
                           (LPBYTE)szValueText,
                           cb ) == ERROR_SUCCESS)
        {
            HWND hwndDefIme = ImmGetDefaultIMEWnd(hwnd);

            if (IsWindow(hwndDefIme))
            {
                SendMessage( hwndDefIme,
                             WM_IME_SYSTEM,
                             IMS_CHANGE_SHOWSTAT,
                             (LPARAM)(DWORD)fShow );
            }
        }
        RegCloseKey(hkey);
        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetIMEStatus
//
////////////////////////////////////////////////////////////////////////////

int Internat_GetIMEStatus(
    HWND *phwndFocus)
{
    if (phwndFocus)
        *phwndFocus = Internat_GetCurrentFocusWnd();

    return ((fpGetIMEStat)());
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetLayout
//
////////////////////////////////////////////////////////////////////////////

HKL Internat_GetLayout(void)
{
    if (!fpGetLayout)
    {
        return (hklCurrent);
    }

    return ((fpGetLayout)());
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetKeyboardLayout
//
////////////////////////////////////////////////////////////////////////////

HKL Internat_GetKeyboardLayout(HWND hwnd)
{
    HKL   hkl;
    DWORD dwThreadId;

    if (!IsWindow(hwnd))
        return NULL;

    dwThreadId = GetWindowThreadProcessId(hwnd, 0);
    hkl = GetKeyboardLayout(dwThreadId);

    if (hkl == NULL) {
        /*
         * This is a console window
         */
        hwnd = (fpGetConsoleImeWnd)();
        dwThreadId = GetWindowThreadProcessId(hwnd, 0);
        hkl = GetKeyboardLayout(dwThreadId);
    }

    return hkl;
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_BuildIMEMenu
//
////////////////////////////////////////////////////////////////////////////

void Internat_BuildIMEMenu(
    HMENU hMenu,
    BOOL fRight,
    BOOL fRemoveDefault)
{
    BOOL fRet;

    if (fpBuildIMEMenu == NULL) {
        return;
    }
    fRet = (fpBuildIMEMenu)(hMenu, fRight);

    if (fRet && !fRemoveDefault) {
        InsertMenu(hMenu, (UINT)-1, MF_SEPARATOR, 0, 0);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_DestroyIMEMenu
//
////////////////////////////////////////////////////////////////////////////

void Internat_DestroyIMEMenu(void)
{
    if (fpDestroyIMEMenu)
    {
        (fpDestroyIMEMenu)();
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetIMEMenu
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_GetIMEMenu(
    HWND hwndIMC,
    BOOL bRight)
{
    //
    //  NT Only:
    //  NT supports inter-process ImmGetIMEMenu().
    //  Let IMM32 handle inter-process hbitmap copying through indicdll.
    //  On NT, we keep data locally, so we need to call our own default IME
    //  window.
    //
    return (fpGetIMEMenu(hwndIMC, bRight));
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetIMEMenuItemID
//
////////////////////////////////////////////////////////////////////////////

UINT Internat_GetIMEMenuItemID(
    int nMenuID)
{
    if (!fpGetIMEMenuItemID)
    {
        return (0);
    }

    return ((fpGetIMEMenuItemID)(nMenuID));
}

////////////////////////////////////////////////////////////////////////////
//
//  Internat_TimerProc
//
////////////////////////////////////////////////////////////////////////////

void CALLBACK Internat_TimerProc(
    HWND hwnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime)
{
    HWND hwndIMC = (HWND)GetProp(hwnd, szPropHwnd);
    BOOL fopen   = (GetProp(hwnd, szPropImeStat) != NULL);

    KillTimer(hwnd, 1);
    PostMessage(hwnd, WM_MYSETOPENSTATUS, (WPARAM)fopen, (LPARAM)hwndIMC);

    RemoveProp(hwnd, szPropHwnd);
    RemoveProp(hwnd, szPropImeStat);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_SetIMEOpenStatus
//
////////////////////////////////////////////////////////////////////////////

void Internat_SetIMEOpenStatus(
    HWND hwnd,
    BOOL fopen,
    HWND hwndIMC)
{
    SetProp(hwnd, szPropHwnd, hwndIMC);
    SetProp(hwnd, szPropImeStat, (HANDLE)fopen);
    SetTimer(hwnd, 1, 100, Internat_TimerProc);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_CallIMEHelp
//
////////////////////////////////////////////////////////////////////////////

BOOL Internat_CallIMEHelp(
    HWND hwnd,
    BOOL fCallWinHelp)
{
    if (hwnd)
    {
        HWND hwndDefIme = ImmGetDefaultIMEWnd(hwnd);

        // this is proposed by HiroYama because of NT5
        // specific window mgmt. here's a quote:
        // 
        // "On NT5, if the parent process is not foreground, 
        // the spawned application will not get to be foreground.
        // The code chunk below will devolve the foreground 
        // priviledge to a specified thread"
        // 
        if (fCallWinHelp) 
        {
            DWORD dwProcessId;
            GetWindowThreadProcessId(hwndDefIme, &dwProcessId);
            AllowSetForegroundWindow(dwProcessId);
        }
        return SendMessage( hwndDefIme,
                             WM_IME_SYSTEM,
                             IMS_IMEHELP,
                             (LPARAM)fCallWinHelp ) != 0;
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_CallConfigureIME
//
////////////////////////////////////////////////////////////////////////////

void Internat_CallConfigureIME(
    HWND hwnd,
    HKL dwhkl)
{
    if (IsWindow(hwnd))
    {
        HWND hwndDefIme = ImmGetDefaultIMEWnd(hwnd);
        if (IsWindow(hwndDefIme))
        {
            SendMessage( hwndDefIme,
                         WM_IME_SYSTEM,
                         IMS_CONFIGUREIME,
                         (LPARAM)dwhkl );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_GetTopLevelWindow
//
////////////////////////////////////////////////////////////////////////////

HWND Internat_GetTopLevelWindow(
    HWND hwnd)
{
    HWND hwndT, hwndRet;
    HWND hwndDesktop = GetDesktopWindow();

    hwndT = hwndRet = hwnd;

    while (hwndT && hwndT != hwndDesktop)
    {
        hwndRet = hwndT;
        hwndT = (GetWindowLong(hwndT,GWL_STYLE) & WS_CHILD)?
                 GetParent(hwndT):
                 GetWindow(hwndT, GW_OWNER);
    }

    return (hwndRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Internat_ModuleEntry
//
////////////////////////////////////////////////////////////////////////////

int _stdcall Internat_ModuleEntry(void)
{
    int i;
    STARTUPINFO si;

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = Internat_WinMain( GetModuleHandle(NULL),
                          NULL,
                          NULL,
                          si.dwFlags & STARTF_USESHOWWINDOW
                            ? si.wShowWindow
                            : SW_SHOWDEFAULT );

    ExitProcess(i);
    return (i);
}
