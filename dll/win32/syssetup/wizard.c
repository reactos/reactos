/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         System setup
 * FILE:            dll/win32/syssetup/wizard.c
 * PURPOSE:         GUI controls
 * PROGRAMMERS:     Eric Kohl
 *                  Pierre Schweitzer <heis_spiter@hotmail.com>
 *                  Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *                  Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <stdlib.h>
#include <time.h>
#include <winnls.h>
#include <windowsx.h>
#include <wincon.h>
#include <shlobj.h>
#include <tzlib.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

#define PM_REGISTRATION_NOTIFY (WM_APP + 1)
/* Private Message used to communicate progress from the background
   registration thread to the main thread.
   wParam = 0 Registration in progress
          = 1 Registration completed
   lParam = Pointer to a REGISTRATIONNOTIFY structure */

typedef struct _REGISTRATIONNOTIFY
{
    ULONG Progress;
    UINT ActivityID;
    LPCWSTR CurrentItem;
    LPCWSTR ErrorMessage;
} REGISTRATIONNOTIFY, *PREGISTRATIONNOTIFY;

typedef struct _REGISTRATIONDATA
{
    HWND hwndDlg;
    ULONG DllCount;
    ULONG Registered;
    PVOID DefaultContext;
} REGISTRATIONDATA, *PREGISTRATIONDATA;

typedef struct _TIMEZONE_ENTRY
{
    struct _TIMEZONE_ENTRY *Prev;
    struct _TIMEZONE_ENTRY *Next;
    WCHAR Description[128]; /* 'Display' */
    WCHAR StandardName[32]; /* 'Std' */
    WCHAR DaylightName[32]; /* 'Dlt' */
    REG_TZI_FORMAT TimezoneInfo; /* 'TZI' */
    ULONG Index;
} TIMEZONE_ENTRY, *PTIMEZONE_ENTRY;


/* FUNCTIONS ****************************************************************/

extern void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);


static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
                 HWND_TOP,
                 ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
                 ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0,
                 0,
                 SWP_NOSIZE);
}


static HFONT
CreateTitleFont(VOID)
{
    LOGFONTW LogFont = {0};
    HDC hdc;
    HFONT hFont;

    LogFont.lfWeight = FW_BOLD;
    wcscpy(LogFont.lfFaceName, L"MS Shell Dlg");

    hdc = GetDC(NULL);
    LogFont.lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    hFont = CreateFontIndirectW(&LogFont);

    ReleaseDC(NULL, hdc);

    return hFont;
}


static HFONT
CreateBoldFont(VOID)
{
    LOGFONTW tmpFont = {0};
    HFONT hBoldFont;
    HDC hDc;

    /* Grabs the Drawing Context */
    hDc = GetDC(NULL);

    tmpFont.lfHeight = -MulDiv(8, GetDeviceCaps(hDc, LOGPIXELSY), 72);
    tmpFont.lfWeight = FW_BOLD;
    wcscpy(tmpFont.lfFaceName, L"MS Shell Dlg");

    hBoldFont = CreateFontIndirectW(&tmpFont);

    ReleaseDC(NULL, hDc);

    return hBoldFont;
}

static INT_PTR CALLBACK
GplDlgProc(HWND hwndDlg,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    HRSRC GplTextResource;
    HGLOBAL GplTextMem;
    PVOID GplTextLocked;
    PCHAR GplText;
    DWORD Size;


    switch (uMsg)
    {
        case WM_INITDIALOG:
            GplTextResource = FindResourceW(hDllInstance, MAKEINTRESOURCE(IDR_GPL), L"RT_TEXT");
            if (NULL == GplTextResource)
            {
                break;
            }
            Size = SizeofResource(hDllInstance, GplTextResource);
            if (0 == Size)
            {
                break;
            }
            GplText = HeapAlloc(GetProcessHeap(), 0, Size + 1);
            if (NULL == GplText)
            {
                break;
            }
            GplTextMem = LoadResource(hDllInstance, GplTextResource);
            if (NULL == GplTextMem)
            {
                HeapFree(GetProcessHeap(), 0, GplText);
                break;
            }
            GplTextLocked = LockResource(GplTextMem);
            if (NULL == GplTextLocked)
            {
                HeapFree(GetProcessHeap(), 0, GplText);
                break;
            }
            memcpy(GplText, GplTextLocked, Size);
            GplText[Size] = '\0';
            SendMessageA(GetDlgItem(hwndDlg, IDC_GPL_TEXT), WM_SETTEXT, 0, (LPARAM) GplText);
            HeapFree(GetProcessHeap(), 0, GplText);
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            return FALSE;

        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED && IDOK == LOWORD(wParam))
            {
                EndDialog(hwndDlg, IDOK);
            }
            break;

        default:
            break;
    }

    return FALSE;
}


static INT_PTR CALLBACK
WelcomeDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PSETUPDATA pSetupData;

    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSetupData);

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow (hwndControl);

            /* Hide the system menu */
            dwStyle = GetWindowLongPtr(hwndControl, GWL_STYLE);
            SetWindowLongPtr(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

            /* Hide and disable the 'Cancel' button */
            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow (hwndControl, SW_HIDE);
            EnableWindow (hwndControl, FALSE);

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                               IDC_WELCOMETITLE,
                               WM_SETFONT,
                               (WPARAM)pSetupData->hTitleFont,
                               (LPARAM)TRUE);
        }
        break;


        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    LogItem(L"BEGIN", L"WelcomePage");
                    /* Enable the Next button */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ACKPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    LogItem(L"END", L"WelcomePage");
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}


static INT_PTR CALLBACK
AckPageDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    LPNMHDR lpnm;
    PWCHAR Projects;
    PWCHAR End, CurrentProject;
    INT ProjectsSize, ProjectsCount;
    PSETUPDATA pSetupData;

    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSetupData);

            Projects = NULL;
            ProjectsSize = 256;
            while (TRUE)
            {
                Projects = HeapAlloc(GetProcessHeap(), 0, ProjectsSize * sizeof(WCHAR));
                if (NULL == Projects)
                {
                    return FALSE;
                }
                ProjectsCount =  LoadStringW(hDllInstance, IDS_ACKPROJECTS, Projects, ProjectsSize);
                if (0 == ProjectsCount)
                {
                    HeapFree(GetProcessHeap(), 0, Projects);
                    return FALSE;
                }
                if (ProjectsCount < ProjectsSize - 1)
                {
                    break;
                }
                HeapFree(GetProcessHeap(), 0, Projects);
                ProjectsSize *= 2;
            }

            CurrentProject = Projects;
            while (*CurrentProject != L'\0')
            {
                End = wcschr(CurrentProject, L'\n');
                if (NULL != End)
                {
                    *End = L'\0';
                }
                (void)ListBox_AddString(GetDlgItem(hwndDlg, IDC_PROJECTS), CurrentProject);
                if (NULL != End)
                {
                    CurrentProject = End + 1;
                }
                else
                {
                    CurrentProject += wcslen(CurrentProject);
                }
            }
            HeapFree(GetProcessHeap(), 0, Projects);
        }
        break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED && IDC_VIEWGPL == LOWORD(wParam))
            {
                DialogBox(hDllInstance, MAKEINTRESOURCE(IDD_GPL), NULL, GplDlgProc);
                SetForegroundWindow(GetParent(hwndDlg));
            }
            break;

        case WM_NOTIFY:
        {
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PRODUCT);
                        return TRUE;
                    }
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static const WCHAR s_szProductOptions[] = L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions";
static const WCHAR s_szRosVersion[] = L"SYSTEM\\CurrentControlSet\\Control\\ReactOS\\Settings\\Version";
static const WCHAR s_szControlWindows[] = L"SYSTEM\\CurrentControlSet\\Control\\Windows";
static const WCHAR s_szWinlogon[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static const WCHAR s_szDefaultSoundEvents[] = L"AppEvents\\Schemes\\Apps\\.Default";
static const WCHAR s_szExplorerSoundEvents[] = L"AppEvents\\Schemes\\Apps\\Explorer";

typedef struct _PRODUCT_OPTION_DATA
{
    LPCWSTR ProductSuite;
    LPCWSTR ProductType;
    DWORD ReportAsWorkstation;
    DWORD CSDVersion;
    DWORD LogonType;
} PRODUCT_OPTION_DATA;

static const PRODUCT_OPTION_DATA s_ProductOptionData[] =
{
    { L"Terminal Server\0", L"ServerNT", 0, 0x200, 0 },
    { L"\0", L"WinNT", 1, 0x300, 1 }
};

static const WCHAR* s_DefaultSoundEvents[][2] = 
{
    { L".Default", L"%SystemRoot%\\Media\\ReactOS_Default.wav" },
    { L"AppGPFault", L"" },
    { L"Close", L"" },
    { L"CriticalBatteryAlarm", L"%SystemRoot%\\Media\\ReactOS_Battery_Critical.wav" },
    { L"DeviceConnect",  L"%SystemRoot%\\Media\\ReactOS_Hardware_Insert.wav" },
    { L"DeviceDisconnect", L"%SystemRoot%\\Media\\ReactOS_Hardware_Remove.wav" },
    { L"DeviceFail", L"%SystemRoot%\\Media\\ReactOS_Hardware_Fail.wav" },
    { L"LowBatteryAlarm", L"%SystemRoot%\\Media\\ReactOS_Battery_Low.wav" },
    { L"MailBeep", L"%SystemRoot%\\Media\\ReactOS_Notify.wav" },
    { L"Maximize", L"%SystemRoot%\\Media\\ReactOS_Restore.wav" },
    { L"MenuCommand", L"%SystemRoot%\\Media\\ReactOS_Menu_Command.wav" },
    { L"MenuPopup", L"" },
    { L"Minimize", L"%SystemRoot%\\Media\\ReactOS_Minimize.wav" },
    { L"Open", L"" },
    { L"PrintComplete", L"%SystemRoot%\\Media\\ReactOS_Print_Complete.wav" },
    { L"RestoreDown", L"" },
    { L"RestoreUp", L"" },
    { L"SystemAsterisk", L"%SystemRoot%\\Media\\ReactOS_Ding.wav" },
    { L"SystemExclamation", L"%SystemRoot%\\Media\\ReactOS_Exclamation.wav" },
    { L"SystemExit", L"%SystemRoot%\\Media\\ReactOS_Shutdown.wav" },
    { L"SystemHand", L"%SystemRoot%\\Media\\ReactOS_Critical_Stop.wav" },
    { L"SystemNotification", L"%SystemRoot%\\Media\\ReactOS_Balloon.wav" },
    { L"SystemQuestion", L"%SystemRoot%\\Media\\ReactOS_Ding.wav" },
    { L"SystemStart", L"%SystemRoot%\\Media\\ReactOS_Startup.wav" },
    { L"WindowsLogoff", L"%SystemRoot%\\Media\\ReactOS_LogOff.wav" }
/* Logon sound is already set by default for both Server and Workstation */
};

static const WCHAR* s_ExplorerSoundEvents[][2] = 
{
    { L"EmptyRecycleBin", L"%SystemRoot%\\Media\\ReactOS_Recycle.wav" },
    { L"Navigating", L"%SystemRoot%\\Media\\ReactOS_Start.wav" }
};

static BOOL
DoWriteSoundEvents(HKEY hKey,
                   LPCWSTR lpSubkey,
                   LPCWSTR lpEventsArray[][2],
                   DWORD dwSize)
{
    HKEY hRootKey, hEventKey, hDefaultKey;
    LONG error;
    ULONG i;
    WCHAR szDest[MAX_PATH];
    DWORD dwAttribs;
    DWORD cbData;

    /* Open the sound events key */
    error = RegOpenKeyExW(hKey, lpSubkey, 0, KEY_READ, &hRootKey);
    if (error)
    {
        DPRINT1("RegOpenKeyExW failed\n");
        goto Error;
    }

    /* Set each sound event */
    for (i = 0; i < dwSize; i++)
    {
        /*
         * Verify that the sound file exists and is an actual file.
         */

        /* Expand the sound file path */
        if (!ExpandEnvironmentStringsW(lpEventsArray[i][1], szDest, _countof(szDest)))
        {
            /* Failed to expand, continue with the next sound event */
            continue;
        }

        /* Check if the sound file exists and isn't a directory */
        dwAttribs = GetFileAttributesW(szDest);
        if ((dwAttribs == INVALID_FILE_ATTRIBUTES) ||
            (dwAttribs & FILE_ATTRIBUTE_DIRECTORY))
        {
            /* It does not, just continue with the next sound event */
            continue;
        }

        /*
         * Create the sound event entry.
         */

        /* Open the sound event subkey */
        error = RegOpenKeyExW(hRootKey, lpEventsArray[i][0], 0, KEY_READ, &hEventKey);
        if (error)
        {
            /* Failed to open, continue with next sound event */
            continue;
        }

        /* Open .Default subkey */
        error = RegOpenKeyExW(hEventKey, L".Default", 0, KEY_WRITE, &hDefaultKey);
        RegCloseKey(hEventKey);
        if (error)
        {
            /* Failed to open, continue with next sound event */
            continue;
        }

        /* Associate the sound file to this sound event */
        cbData = (lstrlenW(lpEventsArray[i][1]) + 1) * sizeof(WCHAR);
        error = RegSetValueExW(hDefaultKey, NULL, 0, REG_EXPAND_SZ, (const BYTE *)lpEventsArray[i][1], cbData);
        RegCloseKey(hDefaultKey);
        if (error)
        {
            /* Failed to set the value, continue with next sound event */
            continue;
        }
    }

Error:
    if (hRootKey)
        RegCloseKey(hRootKey);

    return error == ERROR_SUCCESS;
}

static BOOL
DoWriteProductOption(PRODUCT_OPTION nOption)
{
    HKEY hKey;
    LONG error;
    LPCWSTR pszData;
    DWORD dwValue, cbData;
    const PRODUCT_OPTION_DATA *pData = &s_ProductOptionData[nOption];
    ASSERT(0 <= nOption && nOption < _countof(s_ProductOptionData));

    /* open ProductOptions key */
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_szProductOptions, 0, KEY_WRITE, &hKey);
    if (error)
    {
        DPRINT1("RegOpenKeyExW failed\n");
        goto Error;
    }

    /* write ProductSuite */
    pszData = pData->ProductSuite;
    cbData = (lstrlenW(pszData) + 2) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, L"ProductSuite", 0, REG_MULTI_SZ, (const BYTE *)pszData, cbData);
    if (error)
    {
        DPRINT1("RegSetValueExW failed\n");
        goto Error;
    }

    /* write ProductType */
    pszData = pData->ProductType;
    cbData = (lstrlenW(pszData) + 1) * sizeof(WCHAR);
    error = RegSetValueExW(hKey, L"ProductType", 0, REG_SZ, (const BYTE *)pszData, cbData);
    if (error)
    {
        DPRINT1("RegSetValueExW failed\n");
        goto Error;
    }

    RegCloseKey(hKey);

    /* open ReactOS version key */
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_szRosVersion, 0, KEY_WRITE, &hKey);
    if (error)
    {
        DPRINT1("RegOpenKeyExW failed\n");
        goto Error;
    }

    /* write ReportAsWorkstation */
    dwValue = pData->ReportAsWorkstation;
    cbData = sizeof(dwValue);
    error = RegSetValueExW(hKey, L"ReportAsWorkstation", 0, REG_DWORD, (const BYTE *)&dwValue, cbData);
    if (error)
    {
        DPRINT1("RegSetValueExW failed\n");
        goto Error;
    }

    RegCloseKey(hKey);

    /* open Control Windows key */
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_szControlWindows, 0, KEY_WRITE, &hKey);
    if (error)
    {
        DPRINT1("RegOpenKeyExW failed\n");
        goto Error;
    }

    /* write Control Windows CSDVersion */
    dwValue = pData->CSDVersion;
    cbData = sizeof(dwValue);
    error = RegSetValueExW(hKey, L"CSDVersion", 0, REG_DWORD, (const BYTE *)&dwValue, cbData);
    if (error)
    {
        DPRINT1("RegSetValueExW failed\n");
        goto Error;
    }

    RegCloseKey(hKey);

    /* open Winlogon key */
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_szWinlogon, 0, KEY_WRITE, &hKey);
    if (error)
    {
        DPRINT1("RegOpenKeyExW failed\n");
        goto Error;
    }

    /* write LogonType */
    dwValue = pData->LogonType;
    cbData = sizeof(dwValue);
    error = RegSetValueExW(hKey, L"LogonType", 0, REG_DWORD, (const BYTE *)&dwValue, cbData);
    if (error)
    {
        DPRINT1("RegSetValueExW failed\n");
        goto Error;
    }

    if (nOption == PRODUCT_OPTION_WORKSTATION)
    {
        /* Write system sound events values for Workstation */
        DoWriteSoundEvents(HKEY_CURRENT_USER, s_szDefaultSoundEvents, s_DefaultSoundEvents, _countof(s_DefaultSoundEvents));
        DoWriteSoundEvents(HKEY_CURRENT_USER, s_szExplorerSoundEvents, s_ExplorerSoundEvents, _countof(s_ExplorerSoundEvents));
    }

Error:
    if (hKey)
        RegCloseKey(hKey);

    return error == ERROR_SUCCESS;
}

static void
OnChooseOption(HWND hwndDlg, PRODUCT_OPTION nOption)
{
    WCHAR szText[256];
    ASSERT(0 <= nOption && nOption < _countof(s_ProductOptionData));

    switch (nOption)
    {
        case PRODUCT_OPTION_SERVER:
            LoadStringW(hDllInstance, IDS_PRODUCTSERVERINFO, szText, _countof(szText));
            break;

        case PRODUCT_OPTION_WORKSTATION:
            LoadStringW(hDllInstance, IDS_PRODUCTWORKSTATIONINFO, szText, _countof(szText));
            break;

        default:
            return;
    }

    SetDlgItemTextW(hwndDlg, IDC_PRODUCT_DESCRIPTION, szText);
}

static INT_PTR CALLBACK
ProductPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnm;
    PSETUPDATA pSetupData;
    INT iItem;
    WCHAR szText[64], szDefault[64];
    HICON hIcon;

    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSetupData);

            LoadStringW(hDllInstance, IDS_DEFAULT, szDefault, _countof(szDefault));

            LoadStringW(hDllInstance, IDS_PRODUCTSERVERNAME, szText, _countof(szText));
            if (PRODUCT_OPTION_DEFAULT == PRODUCT_OPTION_SERVER)
            {
                StringCchCatW(szText, _countof(szText), L" ");
                StringCchCatW(szText, _countof(szText), szDefault);
            }
            SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_OPTIONS, CB_ADDSTRING, 0, (LPARAM)szText);

            LoadStringW(hDllInstance, IDS_PRODUCTWORKSTATIONNAME, szText, _countof(szText));
            if (PRODUCT_OPTION_DEFAULT == PRODUCT_OPTION_WORKSTATION)
            {
                StringCchCatW(szText, _countof(szText), L" ");
                StringCchCatW(szText, _countof(szText), szDefault);
            }
            SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_OPTIONS, CB_ADDSTRING, 0, (LPARAM)szText);

            SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_OPTIONS, CB_SETCURSEL, PRODUCT_OPTION_DEFAULT, 0);
            OnChooseOption(hwndDlg, PRODUCT_OPTION_DEFAULT);

            hIcon = LoadIcon(NULL, IDI_WINLOGO);
            SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            return TRUE;
        }

        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE && IDC_PRODUCT_OPTIONS == LOWORD(wParam))
            {
                iItem = SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_OPTIONS, CB_GETCURSEL, 0, 0);
                OnChooseOption(hwndDlg, (PRODUCT_OPTION)iItem);
            }
            break;

        case WM_NOTIFY:
        {
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        OnChooseOption(hwndDlg, pSetupData->ProductOption);
                        DoWriteProductOption(pSetupData->ProductOption);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_LOCALEPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    iItem = SendDlgItemMessageW(hwndDlg, IDC_PRODUCT_OPTIONS, CB_GETCURSEL, 0, 0);
                    pSetupData->ProductOption = (PRODUCT_OPTION)iItem;
                    DoWriteProductOption(pSetupData->ProductOption);
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static
BOOL
WriteOwnerSettings(WCHAR * OwnerName,
                   WCHAR * OwnerOrganization)
{
    HKEY hKey;
    LONG res;

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);

    if (res != ERROR_SUCCESS)
    {
        return FALSE;
    }

    res = RegSetValueExW(hKey,
                         L"RegisteredOwner",
                         0,
                         REG_SZ,
                         (LPBYTE)OwnerName,
                         (wcslen(OwnerName) + 1) * sizeof(WCHAR));

    if (res != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    res = RegSetValueExW(hKey,
                         L"RegisteredOrganization",
                         0,
                         REG_SZ,
                         (LPBYTE)OwnerOrganization,
                         (wcslen(OwnerOrganization) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);
    return (res == ERROR_SUCCESS);
}

static INT_PTR CALLBACK
OwnerPageDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    WCHAR OwnerName[51];
    WCHAR OwnerOrganization[51];
    WCHAR Title[64];
    WCHAR ErrorName[256];
    LPNMHDR lpnm;
    PSETUPDATA pSetupData;

    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSetupData);

            /* set a localized ('Owner') placeholder string as default */
            if (LoadStringW(hDllInstance, IDS_MACHINE_OWNER_NAME, OwnerName, _countof(OwnerName)))
            {
                SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, WM_SETTEXT, 0, (LPARAM)OwnerName);
            }

            SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, EM_LIMITTEXT, 50, 0);
            SendDlgItemMessage(hwndDlg, IDC_OWNERORGANIZATION, EM_LIMITTEXT, 50, 0);

            /* Set focus to owner name */
            SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));

            /* Select the default text to quickly overwrite it by typing */
            SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, EM_SETSEL, 0, -1);
        }
        break;


        case WM_NOTIFY:
        {
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SendMessage(GetDlgItem(hwndDlg, IDC_OWNERNAME), WM_SETTEXT, 0, (LPARAM)pSetupData->OwnerName);
                        SendMessage(GetDlgItem(hwndDlg, IDC_OWNERORGANIZATION), WM_SETTEXT, 0, (LPARAM)pSetupData->OwnerOrganization);
                        if (WriteOwnerSettings(pSetupData->OwnerName, pSetupData->OwnerOrganization))
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_COMPUTERPAGE);
                            return TRUE;
                        }
                    }
                    break;

                case PSN_WIZNEXT:
                    OwnerName[0] = 0;
                    if (GetDlgItemTextW(hwndDlg, IDC_OWNERNAME, OwnerName, 50) == 0)
                    {
                        if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, ARRAYSIZE(Title)))
                        {
                            wcscpy(Title, L"ReactOS Setup");
                        }
                        if (0 == LoadStringW(hDllInstance, IDS_WZD_NAME, ErrorName, ARRAYSIZE(ErrorName)))
                        {
                            wcscpy(ErrorName, L"Setup cannot continue until you enter your name.");
                        }
                        MessageBoxW(hwndDlg, ErrorName, Title, MB_ICONERROR | MB_OK);

                        SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                        return TRUE;
                    }

                    OwnerOrganization[0] = 0;
                    GetDlgItemTextW(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                    if (!WriteOwnerSettings(OwnerName, OwnerOrganization))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static
BOOL
WriteComputerSettings(WCHAR * ComputerName, HWND hwndDlg)
{
    WCHAR Title[64];
    WCHAR ErrorComputerName[256];
    LONG lError;
    HKEY hKey = NULL;

    if (!SetComputerNameW(ComputerName))
    {
        if (hwndDlg != NULL)
        {
            if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, ARRAYSIZE(Title)))
            {
                wcscpy(Title, L"ReactOS Setup");
            }
            if (0 == LoadStringW(hDllInstance, IDS_WZD_SETCOMPUTERNAME, ErrorComputerName,
                                 ARRAYSIZE(ErrorComputerName)))
            {
                wcscpy(ErrorComputerName, L"Setup failed to set the computer name.");
            }
            MessageBoxW(hwndDlg, ErrorComputerName, Title, MB_ICONERROR | MB_OK);
        }

        return FALSE;
    }

    /* Set the physical DNS domain */
    SetComputerNameExW(ComputerNamePhysicalDnsDomain, L"");

    /* Set the physical DNS hostname */
    SetComputerNameExW(ComputerNamePhysicalDnsHostname, ComputerName);

    /* Set the accounts domain name */
    SetAccountsDomainSid(NULL, ComputerName);

    /* Now we need to set the Hostname */
    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                           0,
                           KEY_SET_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW for Tcpip\\Parameters failed (%08lX)\n", lError);
        return TRUE;
    }

    lError = RegSetValueEx(hKey,
                           L"Hostname",
                           0,
                           REG_SZ,
                           (LPBYTE)ComputerName,
                           (wcslen(ComputerName) + 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx(\"Hostname\") failed (%08lX)\n", lError);
    }

    RegCloseKey(hKey);

    return TRUE;
}


static
BOOL
WriteDefaultLogonData(LPWSTR Domain)
{
    WCHAR szAdministratorName[256];
    HKEY hKey = NULL;
    LONG lError;

    if (LoadStringW(hDllInstance,
                    IDS_ADMINISTRATOR_NAME,
                    szAdministratorName,
                    ARRAYSIZE(szAdministratorName)) == 0)
    {
        wcscpy(szAdministratorName, L"Administrator");
    }

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                           0,
                           KEY_SET_VALUE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    lError = RegSetValueEx(hKey,
                           L"DefaultDomainName",
                           0,
                           REG_SZ,
                           (LPBYTE)Domain,
                           (wcslen(Domain)+ 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx(\"DefaultDomainName\") failed!\n");
    }

    lError = RegSetValueEx(hKey,
                           L"DefaultUserName",
                           0,
                           REG_SZ,
                           (LPBYTE)szAdministratorName,
                           (wcslen(szAdministratorName)+ 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx(\"DefaultUserName\") failed!\n");
    }

    RegCloseKey(hKey);

    return TRUE;
}


/* lpBuffer will be filled with a 15-char string (plus the null terminator) */
static void
GenerateComputerName(LPWSTR lpBuffer)
{
    static const WCHAR Chars[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const unsigned cChars = sizeof(Chars) / sizeof(WCHAR) - 1;
    unsigned i;

    wcscpy(lpBuffer, L"REACTOS-");

    srand(GetTickCount());

    /* fill in 7 characters */
    for (i = 8; i < 15; i++)
        lpBuffer[i] = Chars[rand() % cChars];

    lpBuffer[15] = UNICODE_NULL; /* NULL-terminate */
}

static INT_PTR CALLBACK
ComputerPageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR Password1[128];
    WCHAR Password2[128];
    PWCHAR Password;
    WCHAR Title[64];
    WCHAR EmptyComputerName[256], NotMatchPassword[256], WrongPassword[256];
    LPNMHDR lpnm;
    PSETUPDATA pSetupData;

    pSetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, ARRAYSIZE(Title)))
    {
        wcscpy(Title, L"ReactOS Setup");
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pSetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSetupData);

            /* Generate a new pseudo-random computer name */
            GenerateComputerName(ComputerName);

            /* Display current computer name */
            SetDlgItemTextW(hwndDlg, IDC_COMPUTERNAME, ComputerName);

            /* Set text limits */
            SendDlgItemMessage(hwndDlg, IDC_COMPUTERNAME, EM_LIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);
            SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD1, EM_LIMITTEXT, 127, 0);
            SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD2, EM_LIMITTEXT, 127, 0);

            /* Set focus to computer name */
            SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
            if (pSetupData->UnattendSetup)
            {
                SendMessage(GetDlgItem(hwndDlg, IDC_COMPUTERNAME), WM_SETTEXT, 0, (LPARAM)pSetupData->ComputerName);
                SendMessage(GetDlgItem(hwndDlg, IDC_ADMINPASSWORD1), WM_SETTEXT, 0, (LPARAM)pSetupData->AdminPassword);
                SendMessage(GetDlgItem(hwndDlg, IDC_ADMINPASSWORD2), WM_SETTEXT, 0, (LPARAM)pSetupData->AdminPassword);
                WriteComputerSettings(pSetupData->ComputerName, NULL);
                SetAdministratorPassword(pSetupData->AdminPassword);
            }

            /* Store the administrator account name as the default user name */
            WriteDefaultLogonData(pSetupData->ComputerName);
            break;


        case WM_NOTIFY:
        {
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup && WriteComputerSettings(pSetupData->ComputerName, hwndDlg))
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_THEMEPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    if (0 == GetDlgItemTextW(hwndDlg, IDC_COMPUTERNAME, ComputerName, MAX_COMPUTERNAME_LENGTH + 1))
                    {
                        if (0 == LoadStringW(hDllInstance, IDS_WZD_COMPUTERNAME, EmptyComputerName,
                                             ARRAYSIZE(EmptyComputerName)))
                        {
                            wcscpy(EmptyComputerName, L"Setup cannot continue until you enter the name of your computer.");
                        }
                        MessageBoxW(hwndDlg, EmptyComputerName, Title, MB_ICONERROR | MB_OK);
                        SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    /* No need to check computer name for invalid characters,
                     * SetComputerName() will do it for us */

                    if (!WriteComputerSettings(ComputerName, hwndDlg))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

#ifdef PASSWORDS_MANDATORY
                    /* Check if admin passwords have been entered */
                    if ((GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD1, Password1, 128) == 0) ||
                        (GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD2, Password2, 128) == 0))
                    {
                        if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDEMPTY, EmptyPassword,
                                             ARRAYSIZE(EmptyPassword)))
                        {
                            wcscpy(EmptyPassword, L"You must enter a password !");
                        }
                        MessageBoxW(hwndDlg, EmptyPassword, Title, MB_ICONERROR | MB_OK);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
#else
                    GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD1, Password1, 128);
                    GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD2, Password2, 128);
#endif
                    /* Check if passwords match */
                    if (wcscmp(Password1, Password2))
                    {
                        if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDMATCH, NotMatchPassword,
                                             ARRAYSIZE(NotMatchPassword)))
                        {
                            wcscpy(NotMatchPassword, L"The passwords you entered do not match. Please enter the desired password again.");
                        }
                        MessageBoxW(hwndDlg, NotMatchPassword, Title, MB_ICONERROR | MB_OK);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    /* Check password for invalid characters */
                    Password = (PWCHAR)Password1;
                    while (*Password)
                    {
                        if (!isprint(*Password))
                        {
                            if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDCHAR, WrongPassword,
                                                 ARRAYSIZE(WrongPassword)))
                            {
                                wcscpy(WrongPassword, L"The password you entered contains invalid characters. Please enter a cleaned password.");
                            }
                            MessageBoxW(hwndDlg, WrongPassword, Title, MB_ICONERROR | MB_OK);
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }
                        Password++;
                    }

                    /* Set admin password */
                    SetAdministratorPassword(Password1);
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}


static VOID
SetUserLocaleName(HWND hwnd)
{
    WCHAR CurLocale[256] = L"";
    WCHAR CurGeo[256] = L"";
    WCHAR ResText[256] = L"";
    WCHAR LocaleText[256 * 2];

    GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLANGUAGE, CurLocale, ARRAYSIZE(CurLocale));
    GetGeoInfoW(GetUserGeoID(GEOCLASS_NATION), GEO_FRIENDLYNAME, CurGeo, ARRAYSIZE(CurGeo), GetThreadLocale());

    LoadStringW(hDllInstance, IDS_LOCALETEXT, ResText, ARRAYSIZE(ResText));
    StringCchPrintfW(LocaleText, ARRAYSIZE(LocaleText), ResText, CurLocale, CurGeo);

    SetWindowTextW(hwnd, LocaleText);
}

static VOID
SetKeyboardLayoutName(HWND hwnd)
{
    HKL hkl;
    BOOL LayoutSpecial = FALSE;
    WCHAR LayoutPath[256];
    WCHAR LocaleName[32];
    WCHAR SpecialId[5] = L"";
    WCHAR ResText[256] = L"";
    DWORD dwValueSize;
    HKEY hKey;
    UINT i;

    /* Get the default input language and method */
    if (!SystemParametersInfoW(SPI_GETDEFAULTINPUTLANG, 0, (LPDWORD)&hkl, 0))
    {
        hkl = GetKeyboardLayout(0);
    }

    if ((HIWORD(hkl) & 0xF000) == 0xF000)
    {
        /* Process keyboard layout with special id */
        StringCchPrintfW(SpecialId, ARRAYSIZE(SpecialId), L"%04x", (HIWORD(hkl) & 0x0FFF));
        LayoutSpecial = TRUE;
    }

#define MAX_LAYOUTS_PER_LANGID 0x10000
    for (i = 0; i < (LayoutSpecial ? MAX_LAYOUTS_PER_LANGID : 1); i++)
    {
        /* Generate a hexadecimal identifier for keyboard layout registry key */
        StringCchPrintfW(LocaleName, ARRAYSIZE(LocaleName), L"%08lx", (i << 16) | LOWORD(hkl));

        StringCchCopyW(LayoutPath, ARRAYSIZE(LayoutPath), L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\");
        StringCchCatW(LayoutPath, ARRAYSIZE(LayoutPath), LocaleName);
        *LocaleName = UNICODE_NULL;

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          LayoutPath,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey) == ERROR_SUCCESS)
        {
            /* Make sure the keyboard layout key we opened is the one we need.
             * If the layout has no special id, just pass this check. */
            dwValueSize = sizeof(LocaleName);
            if (!LayoutSpecial ||
                ((RegQueryValueExW(hKey,
                                   L"Layout Id",
                                   NULL,
                                   NULL,
                                   (PVOID)&LocaleName,
                                   &dwValueSize) == ERROR_SUCCESS) &&
                (wcscmp(LocaleName, SpecialId) == 0)))
            {
                *LocaleName = UNICODE_NULL;
                dwValueSize = sizeof(LocaleName);
                RegQueryValueExW(hKey,
                                 L"Layout Text",
                                 NULL,
                                 NULL,
                                 (PVOID)&LocaleName,
                                 &dwValueSize);
                /* Let the loop know where to stop */
                i = MAX_LAYOUTS_PER_LANGID;
            }
            RegCloseKey(hKey);
        }
        else
        {
            /* Keyboard layout registry keys are expected to go in order without gaps */
            break;
        }
    }
#undef MAX_LAYOUTS_PER_LANGID

    LoadStringW(hDllInstance, IDS_LAYOUTTEXT, ResText, ARRAYSIZE(ResText));
    StringCchPrintfW(LayoutPath, ARRAYSIZE(LayoutPath), ResText, LocaleName);

    SetWindowTextW(hwnd, LayoutPath);
}


static BOOL
RunControlPanelApplet(HWND hwnd, PCWSTR pwszCPLParameters)
{
    MSG msg;
    HWND MainWindow = GetParent(hwnd);
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    WCHAR CmdLine[MAX_PATH] = L"rundll32.exe shell32.dll,Control_RunDLL ";

    if (!pwszCPLParameters)
    {
        MessageBoxW(hwnd, L"Error: Failed to launch the Control Panel Applet.", NULL, MB_ICONERROR);
        return FALSE;
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

    ASSERT(_countof(CmdLine) > wcslen(CmdLine) + wcslen(pwszCPLParameters));
    wcscat(CmdLine, pwszCPLParameters);

    if (!CreateProcessW(NULL,
                        CmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation))
    {
        MessageBoxW(hwnd, L"Error: Failed to launch the Control Panel Applet.", NULL, MB_ICONERROR);
        return FALSE;
    }

    /* Disable the Back and Next buttons and the main window
     * while we're interacting with the control panel applet */
    PropSheet_SetWizButtons(MainWindow, 0);
    EnableWindow(MainWindow, FALSE);

    while ((MsgWaitForMultipleObjects(1, &ProcessInformation.hProcess, FALSE, INFINITE, QS_ALLINPUT|QS_ALLPOSTMESSAGE )) != WAIT_OBJECT_0)
    {
       /* We still need to process main window messages to avoid freeze */
       while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
       {
           TranslateMessage(&msg);
           DispatchMessageW(&msg);
       }
    }
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    /* Enable the Back and Next buttons and the main window again */
    PropSheet_SetWizButtons(MainWindow, PSWIZB_BACK | PSWIZB_NEXT);
    EnableWindow(MainWindow, TRUE);

    return TRUE;
}

static VOID
WriteUserLocale(VOID)
{
    HKEY hKey;
    LCID lcid;
    WCHAR Locale[12];

    lcid = GetSystemDefaultLCID();

    if (GetLocaleInfoW(MAKELCID(lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Locale, ARRAYSIZE(Locale)) != 0)
    {
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\International",
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, L"Locale", 0, REG_SZ, (LPBYTE)Locale, (wcslen(Locale) + 1) * sizeof(WCHAR));
            RegCloseKey(hKey);
        }
    }
}

static INT_PTR CALLBACK
LocalePageDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PSETUPDATA SetupData;

    /* Retrieve pointer to the global setup data */
    SetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)SetupData);
            WriteUserLocale();

            SetUserLocaleName(GetDlgItem(hwndDlg, IDC_LOCALETEXT));
            SetKeyboardLayoutName(GetDlgItem(hwndDlg, IDC_LAYOUTTEXT));
        }
        break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_CUSTOMLOCALE:
                        RunControlPanelApplet(hwndDlg, L"intl.cpl,,5");
                        SetUserLocaleName(GetDlgItem(hwndDlg, IDC_LOCALETEXT));
                        break;

                    case IDC_CUSTOMLAYOUT:
                        RunControlPanelApplet(hwndDlg, L"input.dll,@1");
                        SetKeyboardLayoutName(GetDlgItem(hwndDlg, IDC_LAYOUTTEXT));
                        break;
                }
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (SetupData->UnattendSetup)
                    {
                        // if (!*SetupData->SourcePath)
                        {
                            RunControlPanelApplet(hwndDlg, L"intl.cpl,,/f:\"$winnt$.inf\""); // Should be in System32
                        }

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OWNERPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    break;

                case PSN_WIZBACK:
                    SetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}


static PTIMEZONE_ENTRY
GetLargerTimeZoneEntry(PSETUPDATA SetupData, DWORD Index)
{
    PTIMEZONE_ENTRY Entry;

    Entry = SetupData->TimeZoneListHead;
    while (Entry != NULL)
    {
        if (Entry->Index >= Index)
            return Entry;

        Entry = Entry->Next;
    }

    return NULL;
}

static LONG
RetrieveTimeZone(
    IN HKEY hZoneKey,
    IN PVOID Context)
{
    LONG lError;
    PSETUPDATA SetupData = (PSETUPDATA)Context;
    PTIMEZONE_ENTRY Entry;
    PTIMEZONE_ENTRY Current;
    ULONG DescriptionSize;
    ULONG StandardNameSize;
    ULONG DaylightNameSize;

    Entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEZONE_ENTRY));
    if (Entry == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DescriptionSize  = sizeof(Entry->Description);
    StandardNameSize = sizeof(Entry->StandardName);
    DaylightNameSize = sizeof(Entry->DaylightName);

    lError = QueryTimeZoneData(hZoneKey,
                               &Entry->Index,
                               &Entry->TimezoneInfo,
                               Entry->Description,
                               &DescriptionSize,
                               Entry->StandardName,
                               &StandardNameSize,
                               Entry->DaylightName,
                               &DaylightNameSize);
    if (lError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Entry);
        return lError;
    }

    if (SetupData->TimeZoneListHead == NULL &&
        SetupData->TimeZoneListTail == NULL)
    {
        Entry->Prev = NULL;
        Entry->Next = NULL;
        SetupData->TimeZoneListHead = Entry;
        SetupData->TimeZoneListTail = Entry;
    }
    else
    {
        Current = GetLargerTimeZoneEntry(SetupData, Entry->Index);
        if (Current != NULL)
        {
            if (Current == SetupData->TimeZoneListHead)
            {
                /* Prepend to head */
                Entry->Prev = NULL;
                Entry->Next = SetupData->TimeZoneListHead;
                SetupData->TimeZoneListHead->Prev = Entry;
                SetupData->TimeZoneListHead = Entry;
            }
            else
            {
                /* Insert before current */
                Entry->Prev = Current->Prev;
                Entry->Next = Current;
                Current->Prev->Next = Entry;
                Current->Prev = Entry;
            }
        }
        else
        {
            /* Append to tail */
            Entry->Prev = SetupData->TimeZoneListTail;
            Entry->Next = NULL;
            SetupData->TimeZoneListTail->Next = Entry;
            SetupData->TimeZoneListTail = Entry;
        }
    }

    return ERROR_SUCCESS;
}

static VOID
CreateTimeZoneList(PSETUPDATA SetupData)
{
    EnumerateTimeZoneList(RetrieveTimeZone, SetupData);
}

static VOID
DestroyTimeZoneList(PSETUPDATA SetupData)
{
    PTIMEZONE_ENTRY Entry;

    while (SetupData->TimeZoneListHead != NULL)
    {
        Entry = SetupData->TimeZoneListHead;

        SetupData->TimeZoneListHead = Entry->Next;
        if (SetupData->TimeZoneListHead != NULL)
        {
            SetupData->TimeZoneListHead->Prev = NULL;
        }

        HeapFree(GetProcessHeap(), 0, Entry);
    }

    SetupData->TimeZoneListTail = NULL;
}


static VOID
ShowTimeZoneList(HWND hwnd, PSETUPDATA SetupData, DWORD dwEntryIndex)
{
    PTIMEZONE_ENTRY Entry;
    DWORD dwIndex = 0;
    DWORD dwCount;

    GetTimeZoneListIndex(&dwEntryIndex);

    Entry = SetupData->TimeZoneListHead;
    while (Entry != NULL)
    {
        dwCount = SendMessage(hwnd,
                              CB_ADDSTRING,
                              0,
                              (LPARAM)Entry->Description);

        if (dwEntryIndex != 0 && dwEntryIndex == Entry->Index)
            dwIndex = dwCount;

        Entry = Entry->Next;
    }

    SendMessage(hwnd,
                CB_SETCURSEL,
                (WPARAM)dwIndex,
                0);
}


static VOID
SetLocalTimeZone(HWND hwnd, PSETUPDATA SetupData)
{
    TIME_ZONE_INFORMATION TimeZoneInformation;
    PTIMEZONE_ENTRY Entry;
    DWORD dwIndex;
    DWORD i;

    dwIndex = SendMessage(hwnd,
                          CB_GETCURSEL,
                          0,
                          0);

    i = 0;
    Entry = SetupData->TimeZoneListHead;
    while (i < dwIndex)
    {
        if (Entry == NULL)
            return;

        i++;
        Entry = Entry->Next;
    }

    wcscpy(TimeZoneInformation.StandardName,
           Entry->StandardName);
    wcscpy(TimeZoneInformation.DaylightName,
           Entry->DaylightName);

    TimeZoneInformation.Bias = Entry->TimezoneInfo.Bias;
    TimeZoneInformation.StandardBias = Entry->TimezoneInfo.StandardBias;
    TimeZoneInformation.DaylightBias = Entry->TimezoneInfo.DaylightBias;

    memcpy(&TimeZoneInformation.StandardDate,
           &Entry->TimezoneInfo.StandardDate,
           sizeof(SYSTEMTIME));
    memcpy(&TimeZoneInformation.DaylightDate,
           &Entry->TimezoneInfo.DaylightDate,
           sizeof(SYSTEMTIME));

    /* Set time zone information */
    SetTimeZoneInformation(&TimeZoneInformation);
}


static BOOL
GetLocalSystemTime(HWND hwnd, PSETUPDATA SetupData)
{
    SYSTEMTIME Date;
    SYSTEMTIME Time;

    if (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATEPICKER), &Date) != GDT_VALID)
    {
        return FALSE;
    }

    if (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_TIMEPICKER), &Time) != GDT_VALID)
    {
        return FALSE;
    }

    SetupData->SystemTime.wYear = Date.wYear;
    SetupData->SystemTime.wMonth = Date.wMonth;
    SetupData->SystemTime.wDayOfWeek = Date.wDayOfWeek;
    SetupData->SystemTime.wDay = Date.wDay;
    SetupData->SystemTime.wHour = Time.wHour;
    SetupData->SystemTime.wMinute = Time.wMinute;
    SetupData->SystemTime.wSecond = Time.wSecond;
    SetupData->SystemTime.wMilliseconds = Time.wMilliseconds;

    return TRUE;
}


static BOOL
SetSystemLocalTime(HWND hwnd, PSETUPDATA SetupData)
{
    BOOL Ret = FALSE;

    /*
     * Call SetLocalTime twice to ensure correct results
     */
    Ret = SetLocalTime(&SetupData->SystemTime) &&
          SetLocalTime(&SetupData->SystemTime);

    return Ret;
}


static VOID
UpdateLocalSystemTime(HWND hwnd, SYSTEMTIME LocalTime)
{
    DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_DATEPICKER), GDT_VALID, &LocalTime);
    DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_TIMEPICKER), GDT_VALID, &LocalTime);
}


static BOOL
WriteDateTimeSettings(HWND hwndDlg, PSETUPDATA SetupData)
{
    WCHAR Title[64];
    WCHAR ErrorLocalTime[256];

    GetLocalSystemTime(hwndDlg, SetupData);
    SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                     SetupData);

    SetAutoDaylight(SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT,
                                       BM_GETCHECK, 0, 0) != BST_UNCHECKED);
    if (!SetSystemLocalTime(hwndDlg, SetupData))
    {
        if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, ARRAYSIZE(Title)))
        {
            wcscpy(Title, L"ReactOS Setup");
        }
        if (0 == LoadStringW(hDllInstance, IDS_WZD_LOCALTIME, ErrorLocalTime,
                             ARRAYSIZE(ErrorLocalTime)))
        {
            wcscpy(ErrorLocalTime, L"Setup was unable to set the local time.");
        }
        MessageBoxW(hwndDlg, ErrorLocalTime, Title, MB_ICONWARNING | MB_OK);
        return FALSE;
    }

    return TRUE;
}


static INT_PTR CALLBACK
DateTimePageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PSETUPDATA SetupData;

    /* Retrieve pointer to the global setup data */
    SetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)SetupData);

            CreateTimeZoneList(SetupData);

            if (SetupData->UnattendSetup)
            {
                ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                                 SetupData, SetupData->TimeZoneIndex);

                if (!SetupData->DisableAutoDaylightTimeSet)
                {
                    SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                }
            }
            else
            {
                ShowTimeZoneList(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                                 SetupData, -1);

                SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
            }
            break;
        }

        case WM_TIMER:
        {
            SYSTEMTIME LocalTime;

            GetLocalTime(&LocalTime);
            UpdateLocalSystemTime(hwndDlg, LocalTime);

            // Reset timeout.
            SetTimer(hwndDlg, 1, 1000 - LocalTime.wMilliseconds, NULL);
            break;
        }

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    SYSTEMTIME LocalTime;

                    GetLocalTime(&LocalTime);
                    UpdateLocalSystemTime(hwndDlg, LocalTime);

                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    if (SetupData->UnattendSetup && WriteDateTimeSettings(hwndDlg, SetupData))
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, SetupData->uFirstNetworkWizardPage);
                        return TRUE;
                    }

                    SetTimer(hwndDlg, 1, 1000 - LocalTime.wMilliseconds, NULL);
                    break;
                }

                case PSN_KILLACTIVE:
                case DTN_DATETIMECHANGE:
                    // NB: Not re-set until changing page (PSN_SETACTIVE).
                    KillTimer(hwndDlg, 1);
                    break;

                case PSN_WIZNEXT:
                    WriteDateTimeSettings(hwndDlg, SetupData);
                    break;

                case PSN_WIZBACK:
                    SetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
            break;

        case WM_DESTROY:
            DestroyTimeZoneList(SetupData);
            break;

        default:
            break;
    }

    return FALSE;
}

static struct ThemeInfo
{
    LPCWSTR PreviewBitmap;
    UINT DisplayName;
    LPCWSTR ThemeFile;

} Themes[] = {
    { MAKEINTRESOURCE(IDB_CLASSIC), IDS_CLASSIC, NULL },
    { MAKEINTRESOURCE(IDB_LAUTUS), IDS_LAUTUS, L"themes\\lautus\\lautus.msstyles" },
    { MAKEINTRESOURCE(IDB_LUNAR), IDS_LUNAR, L"themes\\lunar\\lunar.msstyles" },
    { MAKEINTRESOURCE(IDB_MIZU), IDS_MIZU, L"themes\\mizu\\mizu.msstyles"},
};

static INT_PTR CALLBACK
ThemePageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PSETUPDATA SetupData;
    LPNMLISTVIEW pnmv;

    /* Retrieve pointer to the global setup data */
    SetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hListView;
            HIMAGELIST himl;
            DWORD n;
            LVITEM lvi = {0};

            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)SetupData);

            hListView = GetDlgItem(hwndDlg, IDC_THEMEPICKER);

            /* Common */
            himl = ImageList_Create(180, 163, ILC_COLOR32 | ILC_MASK, ARRAYSIZE(Themes), 1);
            lvi.mask = LVIF_TEXT | LVIF_IMAGE |LVIF_STATE;

            for (n = 0; n < ARRAYSIZE(Themes); ++n)
            {
                WCHAR DisplayName[100] = {0};
                /* Load the bitmap */
                HANDLE image = LoadImageW(hDllInstance, Themes[n].PreviewBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
                ImageList_AddMasked(himl, image, RGB(255,0,255));

                /* Load the string */
                LoadStringW(hDllInstance, Themes[n].DisplayName, DisplayName, ARRAYSIZE(DisplayName));
                DisplayName[ARRAYSIZE(DisplayName)-1] = UNICODE_NULL;

                /* Add the listview item */
                lvi.iItem  = n;
                lvi.iImage = n;
                lvi.pszText = DisplayName;
                ListView_InsertItem(hListView, &lvi);
            }

            /* Register the imagelist */
            ListView_SetImageList(hListView, himl, LVSIL_NORMAL);
            /* Transparant background */
            ListView_SetBkColor(hListView, CLR_NONE);
            ListView_SetTextBkColor(hListView, CLR_NONE);
            /* Reduce the size between the items */
            ListView_SetIconSpacing(hListView, 190, 173);
            break;
        }
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                //case LVN_ITEMCHANGING:
                case LVN_ITEMCHANGED:
                    pnmv = (LPNMLISTVIEW)lParam;
                    if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED))
                    {
                        int iTheme = pnmv->iItem;
                        DPRINT1("Selected theme: %u\n", Themes[iTheme].DisplayName);

                        if (Themes[iTheme].ThemeFile)
                        {
                            WCHAR wszParams[1024];
                            WCHAR wszTheme[MAX_PATH];
                            WCHAR* format = L"desk.cpl desk,@Appearance /Action:ActivateMSTheme /file:\"%s\"";

                            SHGetFolderPathAndSubDirW(0, CSIDL_RESOURCES, NULL, SHGFP_TYPE_DEFAULT, Themes[iTheme].ThemeFile, wszTheme);
                            swprintf(wszParams, format, wszTheme);
                            RunControlPanelApplet(hwndDlg, wszParams);
                        }
                        else
                        {
                            RunControlPanelApplet(hwndDlg, L"desk.cpl desk,@Appearance /Action:ActivateMSTheme");
                        }
                    }
                    break;
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (SetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, SetupData->uFirstNetworkWizardPage);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    break;

                case PSN_WIZBACK:
                    SetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static UINT CALLBACK
RegistrationNotificationProc(PVOID Context,
                             UINT Notification,
                             UINT_PTR Param1,
                             UINT_PTR Param2)
{
    PREGISTRATIONDATA RegistrationData;
    REGISTRATIONNOTIFY RegistrationNotify;
    PSP_REGISTER_CONTROL_STATUSW StatusInfo;
    UINT MessageID;
    WCHAR ErrorMessage[128];

    RegistrationData = (PREGISTRATIONDATA) Context;

    if (SPFILENOTIFY_STARTREGISTRATION == Notification ||
            SPFILENOTIFY_ENDREGISTRATION == Notification)
    {
        StatusInfo = (PSP_REGISTER_CONTROL_STATUSW) Param1;
        RegistrationNotify.CurrentItem = wcsrchr(StatusInfo->FileName, L'\\');
        if (NULL == RegistrationNotify.CurrentItem)
        {
            RegistrationNotify.CurrentItem = StatusInfo->FileName;
        }
        else
        {
            RegistrationNotify.CurrentItem++;
        }

        if (SPFILENOTIFY_STARTREGISTRATION == Notification)
        {
            DPRINT("Received SPFILENOTIFY_STARTREGISTRATION notification for %S\n",
                   StatusInfo->FileName);
            RegistrationNotify.ErrorMessage = NULL;
            RegistrationNotify.Progress = RegistrationData->Registered;
        }
        else
        {
            DPRINT("Received SPFILENOTIFY_ENDREGISTRATION notification for %S\n",
                   StatusInfo->FileName);
            DPRINT("Win32Error %u FailureCode %u\n", StatusInfo->Win32Error,
                   StatusInfo->FailureCode);
            if (SPREG_SUCCESS != StatusInfo->FailureCode)
            {
                switch(StatusInfo->FailureCode)
                {
                    case SPREG_LOADLIBRARY:
                        MessageID = IDS_LOADLIBRARY_FAILED;
                        break;
                    case SPREG_GETPROCADDR:
                        MessageID = IDS_GETPROCADDR_FAILED;
                        break;
                    case SPREG_REGSVR:
                        MessageID = IDS_REGSVR_FAILED;
                        break;
                    case SPREG_DLLINSTALL:
                        MessageID = IDS_DLLINSTALL_FAILED;
                        break;
                    case SPREG_TIMEOUT:
                        MessageID = IDS_TIMEOUT;
                        break;
                    default:
                        MessageID = IDS_REASON_UNKNOWN;
                        break;
                }
                if (0 == LoadStringW(hDllInstance, MessageID,
                                     ErrorMessage,
                                     ARRAYSIZE(ErrorMessage)))
                {
                    ErrorMessage[0] = L'\0';
                }
                if (SPREG_TIMEOUT != StatusInfo->FailureCode)
                {
                    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                                   StatusInfo->Win32Error, 0,
                                   ErrorMessage + wcslen(ErrorMessage),
                                   ARRAYSIZE(ErrorMessage) - wcslen(ErrorMessage),
                                   NULL);
                }
                RegistrationNotify.ErrorMessage = ErrorMessage;
            }
            else
            {
                RegistrationNotify.ErrorMessage = NULL;
            }
            if (RegistrationData->Registered < RegistrationData->DllCount)
            {
                RegistrationData->Registered++;
            }
        }

        RegistrationNotify.Progress = RegistrationData->Registered;
        RegistrationNotify.ActivityID = IDS_REGISTERING_COMPONENTS;
        SendMessage(RegistrationData->hwndDlg, PM_REGISTRATION_NOTIFY,
                    0, (LPARAM) &RegistrationNotify);

        return FILEOP_DOIT;
    }
    else
    {
        DPRINT1("Received unexpected notification %u\n", Notification);
        return SetupDefaultQueueCallback(RegistrationData->DefaultContext,
                                         Notification, Param1, Param2);
    }
}


static DWORD CALLBACK
RegistrationProc(LPVOID Parameter)
{
    PREGISTRATIONDATA RegistrationData;
    REGISTRATIONNOTIFY RegistrationNotify;
    DWORD LastError = NO_ERROR;
    WCHAR UnknownError[84];

    RegistrationData = (PREGISTRATIONDATA) Parameter;
    RegistrationData->Registered = 0;
    RegistrationData->DefaultContext = SetupInitDefaultQueueCallback(RegistrationData->hwndDlg);

    _SEH2_TRY
    {
        if (!SetupInstallFromInfSectionW(GetParent(RegistrationData->hwndDlg),
        hSysSetupInf,
        L"RegistrationPhase2",
        SPINST_REGISTRY |
        SPINST_REGISTERCALLBACKAWARE  |
        SPINST_REGSVR,
        0,
        NULL,
        0,
        RegistrationNotificationProc,
        RegistrationData,
        NULL,
        NULL))
        {
            LastError = GetLastError();
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT("Catching exception\n");
        LastError = RtlNtStatusToDosError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (NO_ERROR == LastError)
    {
        RegistrationNotify.ErrorMessage = NULL;
    }
    else
    {
        DPRINT1("SetupInstallFromInfSection failed with error %u\n",
                LastError);
        if (0 == FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM, NULL, LastError, 0,
                                (LPWSTR) &RegistrationNotify.ErrorMessage, 0,
                                NULL))
        {
            if (0 == LoadStringW(hDllInstance, IDS_UNKNOWN_ERROR,
                                 UnknownError,
                                 ARRAYSIZE(UnknownError) - 20))
            {
                wcscpy(UnknownError, L"Unknown error");
            }
            wcscat(UnknownError, L" ");
            _ultow(LastError, UnknownError + wcslen(UnknownError), 10);
            RegistrationNotify.ErrorMessage = UnknownError;
        }
    }

    RegistrationNotify.Progress = RegistrationData->DllCount;
    RegistrationNotify.ActivityID = IDS_REGISTERING_COMPONENTS;
    RegistrationNotify.CurrentItem = NULL;

    RegisterTypeLibraries(hSysSetupInf, L"TypeLibraries");

    // FIXME: Move this call to a separate cleanup page!
    RtlCreateBootStatusDataFile();

    SendMessage(RegistrationData->hwndDlg, PM_REGISTRATION_NOTIFY,
                1, (LPARAM) &RegistrationNotify);
    if (NULL != RegistrationNotify.ErrorMessage &&
            UnknownError != RegistrationNotify.ErrorMessage)
    {
        LocalFree((PVOID) RegistrationNotify.ErrorMessage);
    }

    SetupTermDefaultQueueCallback(RegistrationData->DefaultContext);
    HeapFree(GetProcessHeap(), 0, RegistrationData);

    return 0;
}


static BOOL
StartComponentRegistration(HWND hwndDlg, PULONG MaxProgress)
{
    HANDLE RegistrationThread;
    LONG DllCount;
    INFCONTEXT Context;
    WCHAR SectionName[512];
    PREGISTRATIONDATA RegistrationData;

    DllCount = -1;
    if (!SetupFindFirstLineW(hSysSetupInf, L"RegistrationPhase2",
                             L"RegisterDlls", &Context))
    {
        DPRINT1("No RegistrationPhase2 section found\n");
        return FALSE;
    }
    if (!SetupGetStringFieldW(&Context, 1, SectionName,
                              ARRAYSIZE(SectionName),
                              NULL))
    {
        DPRINT1("Unable to retrieve section name\n");
        return FALSE;
    }
    DllCount = SetupGetLineCountW(hSysSetupInf, SectionName);
    DPRINT("SectionName %S DllCount %ld\n", SectionName, DllCount);
    if (DllCount < 0)
    {
        SetLastError(STATUS_NOT_FOUND);
        return FALSE;
    }

    *MaxProgress = (ULONG) DllCount;

    /*
     * Create a background thread to do the actual registrations, so the
     * main thread can just run its message loop.
     */
    RegistrationThread = NULL;
    RegistrationData = HeapAlloc(GetProcessHeap(), 0,
                                 sizeof(REGISTRATIONDATA));
    if (RegistrationData != NULL)
    {
        RegistrationData->hwndDlg = hwndDlg;
        RegistrationData->DllCount = DllCount;
        RegistrationThread = CreateThread(NULL, 0, RegistrationProc,
                                          RegistrationData, 0, NULL);
        if (RegistrationThread != NULL)
        {
            CloseHandle(RegistrationThread);
        }
        else
        {
            DPRINT1("CreateThread failed, error %u\n", GetLastError());
            HeapFree(GetProcessHeap(), 0, RegistrationData);
            return FALSE;
        }
    }
    else
    {
        DPRINT1("HeapAlloc() failed, error %u\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}


static INT_PTR CALLBACK
ProcessPageDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PSETUPDATA SetupData;
    PREGISTRATIONNOTIFY RegistrationNotify;
    static UINT oldActivityID = -1;
    WCHAR Title[64];

    /* Retrieve pointer to the global setup data */
    SetupData = (PSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)SetupData);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            ULONG MaxProgress = 0;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Disable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

                    StartComponentRegistration(hwndDlg, &MaxProgress);

                    SendDlgItemMessage(hwndDlg, IDC_PROCESSPROGRESS, PBM_SETRANGE,
                                       0, MAKELPARAM(0, MaxProgress));
                    SendDlgItemMessage(hwndDlg, IDC_PROCESSPROGRESS, PBM_SETPOS,
                                       0, 0);
                    break;

                case PSN_WIZNEXT:
                    break;

                case PSN_WIZBACK:
                    SetupData->UnattendSetup = FALSE;
                    break;

                default:
                    break;
            }
        }
        break;

        case PM_REGISTRATION_NOTIFY:
        {
            WCHAR Activity[64];
            RegistrationNotify = (PREGISTRATIONNOTIFY) lParam;
            // update if necessary only
            if (oldActivityID != RegistrationNotify->ActivityID)
            {
                if (0 != LoadStringW(hDllInstance, RegistrationNotify->ActivityID,
                                     Activity,
                                     ARRAYSIZE(Activity)))
                {
                    SendDlgItemMessageW(hwndDlg, IDC_ACTIVITY, WM_SETTEXT,
                                        0, (LPARAM) Activity);
                }
                oldActivityID = RegistrationNotify->ActivityID;
            }
            SendDlgItemMessageW(hwndDlg, IDC_ITEM, WM_SETTEXT, 0,
                                (LPARAM)(NULL == RegistrationNotify->CurrentItem ?
                                         L"" : RegistrationNotify->CurrentItem));
            SendDlgItemMessage(hwndDlg, IDC_PROCESSPROGRESS, PBM_SETPOS,
                               RegistrationNotify->Progress, 0);
            if (NULL != RegistrationNotify->ErrorMessage)
            {
                if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP,
                                     Title, ARRAYSIZE(Title)))
                {
                    wcscpy(Title, L"ReactOS Setup");
                }
                MessageBoxW(hwndDlg, RegistrationNotify->ErrorMessage,
                            Title, MB_ICONERROR | MB_OK);

            }

            if (wParam)
            {
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
            }
        }
        return TRUE;

        default:
            break;
    }

    return FALSE;
}


static VOID
SetInstallationCompleted(VOID)
{
    HKEY hKey = 0;
    DWORD InProgress = 0;
    DWORD InstallDate;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                       L"SYSTEM\\Setup",
                       0,
                       KEY_WRITE,
                       &hKey ) == ERROR_SUCCESS)
    {
        RegSetValueExW( hKey, L"SystemSetupInProgress", 0, REG_DWORD, (LPBYTE)&InProgress, sizeof(InProgress) );
        RegCloseKey( hKey );
    }

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                       L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                       0,
                       KEY_WRITE,
                       &hKey ) == ERROR_SUCCESS)
    {
        InstallDate = (DWORD)time(NULL);
        RegSetValueExW( hKey, L"InstallDate", 0, REG_DWORD, (LPBYTE)&InstallDate, sizeof(InstallDate) );
        RegCloseKey( hKey );
    }
}

static INT_PTR CALLBACK
FinishDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Get pointer to the global setup data */
            PSETUPDATA SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;

            if (!SetupData->UnattendSetup || !SetupData->DisableGeckoInst)
            {
                /* Run the Wine Gecko prompt */
                Control_RunDLLW(hwndDlg, 0, L"appwiz.cpl install_gecko", SW_SHOW);
            }

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                               IDC_FINISHTITLE,
                               WM_SETFONT,
                               (WPARAM)SetupData->hTitleFont,
                               (LPARAM)TRUE);
            if (SetupData->UnattendSetup)
            {
                KillTimer(hwndDlg, 1);
                SetInstallationCompleted();
                PostQuitMessage(0);
            }
        }
        break;

        case WM_DESTROY:
        {
            SetInstallationCompleted();
            PostQuitMessage(0);
            return TRUE;
        }

        case WM_TIMER:
        {
            INT Position;
            HWND hWndProgress;

            hWndProgress = GetDlgItem(hwndDlg, IDC_RESTART_PROGRESS);
            Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
            if (Position == 300)
            {
                KillTimer(hwndDlg, 1);
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
            else
            {
                SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
            }
        }
        return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the correct buttons on for the active page */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);

                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0,
                                       MAKELPARAM(0, 300));
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                    SetTimer(hwndDlg, 1, 50, NULL);
                    break;

                case PSN_WIZFINISH:
                    DestroyWindow(GetParent(hwndDlg));
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}


/*
 * GetInstallSourceWin32 retrieves the path to the ReactOS installation medium
 * in Win32 format, for later use by syssetup and storage in the registry.
 */
static BOOL
GetInstallSourceWin32(
    OUT PWSTR pwszPath,
    IN DWORD cchPathMax,
    IN PCWSTR pwszNTPath)
{
    WCHAR wszDrives[512];
    WCHAR wszNTPath[512]; // MAX_PATH ?
    DWORD cchDrives;
    PWCHAR pwszDrive;

    *pwszPath = UNICODE_NULL;

    cchDrives = GetLogicalDriveStringsW(_countof(wszDrives) - 1, wszDrives);
    if (cchDrives == 0 || cchDrives >= _countof(wszDrives))
    {
        /* Buffer too small or failure */
        LogItem(NULL, L"GetLogicalDriveStringsW failed");
        return FALSE;
    }

    for (pwszDrive = wszDrives; *pwszDrive; pwszDrive += wcslen(pwszDrive) + 1)
    {
        WCHAR wszBuf[MAX_PATH];

        /* Retrieve the NT path corresponding to the current Win32 DOS path */
        pwszDrive[2] = UNICODE_NULL; // Temporarily remove the backslash
        QueryDosDeviceW(pwszDrive, wszNTPath, _countof(wszNTPath));
        pwszDrive[2] = L'\\';        // Restore the backslash

        wcscat(wszNTPath, L"\\");    // Concat a backslash

        /* Logging */
        wsprintf(wszBuf, L"Testing '%s' --> '%s' %s a CD",
                 pwszDrive, wszNTPath,
                 (GetDriveTypeW(pwszDrive) == DRIVE_CDROM) ? L"is" : L"is not");
        LogItem(NULL, wszBuf);

        /* Check whether the NT path corresponds to the NT installation source path */
        if (!_wcsicmp(wszNTPath, pwszNTPath))
        {
            /* Found it! */
            wcscpy(pwszPath, pwszDrive); // cchPathMax

            /* Logging */
            wsprintf(wszBuf, L"GetInstallSourceWin32: %s", pwszPath);
            LogItem(NULL, wszBuf);
            wcscat(wszBuf, L"\n");
            OutputDebugStringW(wszBuf);

            return TRUE;
        }
    }

    return FALSE;
}

VOID
ProcessUnattendSection(
    IN OUT PSETUPDATA pSetupData)
{
    INFCONTEXT InfContext;
    WCHAR szName[256];
    WCHAR szValue[MAX_PATH];
    DWORD LineLength;
    HKEY hKey;

    if (!SetupFindFirstLineW(pSetupData->hSetupInf,
                             L"Unattend",
                             L"UnattendSetupEnabled",
                             &InfContext))
    {
        DPRINT1("Error: Cannot find UnattendSetupEnabled Key! %d\n", GetLastError());
        return;
    }

    if (!SetupGetStringFieldW(&InfContext,
                              1,
                              szValue,
                              ARRAYSIZE(szValue),
                              &LineLength))
    {
        DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
        return;
    }

    if (_wcsicmp(szValue, L"yes") != 0)
    {
        DPRINT("Unattend setup was disabled by UnattendSetupEnabled key.\n");
        return;
    }

    pSetupData->UnattendSetup = TRUE;

    if (!SetupFindFirstLineW(pSetupData->hSetupInf,
                             L"Unattend",
                             NULL,
                             &InfContext))
    {
        DPRINT1("Error: SetupFindFirstLine failed %d\n", GetLastError());
        return;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szName,
                                  ARRAYSIZE(szName),
                                  &LineLength))
        {
            DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
            return;
        }

        if (!SetupGetStringFieldW(&InfContext,
                                  1,
                                  szValue,
                                  ARRAYSIZE(szValue),
                                  &LineLength))
        {
            DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
            return;
        }
        DPRINT1("Name %S Value %S\n", szName, szValue);
        if (!_wcsicmp(szName, L"FullName"))
        {
            if (ARRAYSIZE(pSetupData->OwnerName) > LineLength)
            {
                wcscpy(pSetupData->OwnerName, szValue);
            }
        }
        else if (!_wcsicmp(szName, L"OrgName"))
        {
            if (ARRAYSIZE(pSetupData->OwnerOrganization) > LineLength)
            {
                wcscpy(pSetupData->OwnerOrganization, szValue);
            }
        }
        else if (!_wcsicmp(szName, L"ComputerName"))
        {
            if (ARRAYSIZE(pSetupData->ComputerName) > LineLength)
            {
                wcscpy(pSetupData->ComputerName, szValue);
            }
        }
        else if (!_wcsicmp(szName, L"AdminPassword"))
        {
            if (ARRAYSIZE(pSetupData->AdminPassword) > LineLength)
            {
                wcscpy(pSetupData->AdminPassword, szValue);
            }
        }
        else if (!_wcsicmp(szName, L"TimeZoneIndex"))
        {
            pSetupData->TimeZoneIndex = _wtoi(szValue);
        }
        else if (!_wcsicmp(szName, L"DisableAutoDaylightTimeSet"))
        {
            pSetupData->DisableAutoDaylightTimeSet = _wtoi(szValue);
        }
        else if (!_wcsicmp(szName, L"DisableGeckoInst"))
        {
            if (!_wcsicmp(szValue, L"yes"))
                pSetupData->DisableGeckoInst = TRUE;
            else
                pSetupData->DisableGeckoInst = FALSE;
        }
        else if (!_wcsicmp(szName, L"ProductOption"))
        {
            pSetupData->ProductOption = (PRODUCT_OPTION)_wtoi(szValue);
        }
    } while (SetupFindNextLine(&InfContext, &InfContext));

    if (SetupFindFirstLineW(pSetupData->hSetupInf,
                            L"Display",
                            NULL,
                            &InfContext))
    {
        DEVMODEW dm = { { 0 } };
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm))
        {
            do
            {
                int iValue;
                if (!SetupGetStringFieldW(&InfContext,
                                          0,
                                          szName,
                                          ARRAYSIZE(szName),
                                          &LineLength))
                {
                    DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
                    return;
                }

                if (!SetupGetStringFieldW(&InfContext,
                                          1,
                                          szValue,
                                          ARRAYSIZE(szValue),
                                          &LineLength))
                {
                    DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
                    return;
                }
                iValue = _wtoi(szValue);
                DPRINT1("Name %S Value %i\n", szName, iValue);

                if (!iValue)
                    continue;

                if (!_wcsicmp(szName, L"BitsPerPel"))
                {
                    dm.dmFields |= DM_BITSPERPEL;
                    dm.dmBitsPerPel = iValue;
                }
                else if (!_wcsicmp(szName, L"XResolution"))
                {
                    dm.dmFields |= DM_PELSWIDTH;
                    dm.dmPelsWidth = iValue;
                }
                else if (!_wcsicmp(szName, L"YResolution"))
                {
                    dm.dmFields |= DM_PELSHEIGHT;
                    dm.dmPelsHeight = iValue;
                }
                else if (!_wcsicmp(szName, L"VRefresh"))
                {
                    dm.dmFields |= DM_DISPLAYFREQUENCY;
                    dm.dmDisplayFrequency = iValue;
                }
            } while (SetupFindNextLine(&InfContext, &InfContext));

            ChangeDisplaySettingsW(&dm, CDS_UPDATEREGISTRY);
        }
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                      0,
                      KEY_SET_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        DPRINT1("Error: failed to open HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\n");
        return;
    }

    if (SetupFindFirstLineW(pSetupData->hSetupInf,
                            L"GuiRunOnce",
                            NULL,
                            &InfContext))
    {
        int i = 0;
        do
        {
            if (SetupGetStringFieldW(&InfContext,
                                     0,
                                     szValue,
                                     ARRAYSIZE(szValue),
                                     NULL))
            {
                WCHAR szPath[MAX_PATH];
                swprintf(szName, L"%d", i);
                DPRINT("szName %S szValue %S\n", szName, szValue);

                if (ExpandEnvironmentStringsW(szValue, szPath, MAX_PATH))
                {
                    DPRINT("value %S\n", szPath);
                    if (RegSetValueExW(hKey,
                                       szName,
                                       0,
                                       REG_SZ,
                                       (const BYTE*)szPath,
                                       (wcslen(szPath) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS)
                    {
                        i++;
                    }
                }
            }
        } while (SetupFindNextLine(&InfContext, &InfContext));
    }

    RegCloseKey(hKey);
}

VOID
ProcessSetupInf(
    IN OUT PSETUPDATA pSetupData)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    INFCONTEXT InfContext;
    DWORD LineLength;
    HKEY hKey;
    LONG res;

    pSetupData->hSetupInf = INVALID_HANDLE_VALUE;

    /* Retrieve the path of the setup INF */
    GetSystemDirectoryW(szPath, _countof(szPath));
    wcscat(szPath, L"\\$winnt$.inf");

    /* Open the setup INF */
    pSetupData->hSetupInf = SetupOpenInfFileW(szPath,
                                              NULL,
                                              INF_STYLE_OLDNT,
                                              NULL);
    if (pSetupData->hSetupInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Error: Cannot open the setup information file %S with error %d\n", szPath, GetLastError());
        return;
    }


    /* Retrieve the NT source path from which the 1st-stage installer was run */
    if (!SetupFindFirstLineW(pSetupData->hSetupInf,
                             L"data",
                             L"sourcepath",
                             &InfContext))
    {
        DPRINT1("Error: Cannot find sourcepath Key! %d\n", GetLastError());
        return;
    }

    if (!SetupGetStringFieldW(&InfContext,
                              1,
                              szValue,
                              ARRAYSIZE(szValue),
                              &LineLength))
    {
        DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
        return;
    }

    *pSetupData->SourcePath = UNICODE_NULL;

    /* Close the setup INF as we are going to modify it manually */
    if (pSetupData->hSetupInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(pSetupData->hSetupInf);


    /* Find the installation source path in Win32 format */
    if (!GetInstallSourceWin32(pSetupData->SourcePath,
                               _countof(pSetupData->SourcePath),
                               szValue))
    {
        *pSetupData->SourcePath = UNICODE_NULL;
    }

    /* Save the path in Win32 format in the setup INF */
    swprintf(szValue, L"\"%s\"", pSetupData->SourcePath);
    WritePrivateProfileStringW(L"data", L"dospath", szValue, szPath);

    /*
     * Save it also in the registry, in the following keys:
     * - HKLM\Software\Microsoft\Windows\CurrentVersion\Setup ,
     *   values "SourcePath" and "ServicePackSourcePath" (REG_SZ);
     * - HKLM\Software\Microsoft\Windows NT\CurrentVersion ,
     *   value "SourcePath" (REG_SZ); set to the full path (e.g. D:\I386).
     */
#if 0
    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);

    if (res != ERROR_SUCCESS)
    {
        return FALSE;
    }
#endif

    res = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup",
                          0, NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, // KEY_WRITE
                          NULL,
                          &hKey,
                          NULL);
    if (res == ERROR_SUCCESS)
    {
        res = RegSetValueExW(hKey,
                             L"SourcePath",
                             0,
                             REG_SZ,
                             (LPBYTE)pSetupData->SourcePath,
                             (wcslen(pSetupData->SourcePath) + 1) * sizeof(WCHAR));

        res = RegSetValueExW(hKey,
                             L"ServicePackSourcePath",
                             0,
                             REG_SZ,
                             (LPBYTE)pSetupData->SourcePath,
                             (wcslen(pSetupData->SourcePath) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }


    /* Now, re-open the setup INF (this must succeed) */
    pSetupData->hSetupInf = SetupOpenInfFileW(szPath,
                                              NULL,
                                              INF_STYLE_OLDNT,
                                              NULL);
    if (pSetupData->hSetupInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Error: Cannot open the setup information file %S with error %d\n", szPath, GetLastError());
        return;
    }

    /* Process the unattended section of the setup file */
    ProcessUnattendSection(pSetupData);
}

typedef DWORD(WINAPI *PFNREQUESTWIZARDPAGES)(PDWORD, HPROPSHEETPAGE *, PSETUPDATA);

VOID
InstallWizard(VOID)
{
    PROPSHEETHEADER psh = {0};
    HPROPSHEETPAGE *phpage = NULL;
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;
    HWND hWnd;
    MSG msg;
    PSETUPDATA pSetupData = NULL;
    HMODULE hNetShell = NULL;
    PFNREQUESTWIZARDPAGES pfn = NULL;
    DWORD dwPageCount = 10, dwNetworkPageCount = 0;

    LogItem(L"BEGIN_SECTION", L"InstallWizard");

    /* Allocate setup data */
    pSetupData = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           sizeof(SETUPDATA));
    if (pSetupData == NULL)
    {
        LogItem(NULL, L"SetupData allocation failed!");
        MessageBoxW(NULL,
                    L"Setup failed to allocate global data!",
                    L"ReactOS Setup",
                    MB_ICONERROR | MB_OK);
        goto done;
    }
    pSetupData->ProductOption = PRODUCT_OPTION_DEFAULT;

    hNetShell = LoadLibraryW(L"netshell.dll");
    if (hNetShell != NULL)
    {
        DPRINT("Netshell.dll loaded!\n");

        pfn = (PFNREQUESTWIZARDPAGES)GetProcAddress(hNetShell,
                                                    "NetSetupRequestWizardPages");
        if (pfn != NULL)
        {
            pfn(&dwNetworkPageCount, NULL, NULL);
            dwPageCount += dwNetworkPageCount;
        }
    }

    DPRINT("PageCount: %lu\n", dwPageCount);

    phpage = HeapAlloc(GetProcessHeap(),
                       HEAP_ZERO_MEMORY,
                       dwPageCount * sizeof(HPROPSHEETPAGE));
    if (phpage == NULL)
    {
        LogItem(NULL, L"Page array allocation failed!");
        MessageBoxW(NULL,
                    L"Setup failed to allocate page array!",
                    L"ReactOS Setup",
                    MB_ICONERROR | MB_OK);
        goto done;
    }

    /* Process the $winnt$.inf setup file */
    ProcessSetupInf(pSetupData);

    /* Create the Welcome page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hDllInstance;
    psp.lParam = (LPARAM)pSetupData;
    psp.pfnDlgProc = WelcomeDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Acknowledgements page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ACKTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ACKSUBTITLE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ACKPAGE);
    psp.pfnDlgProc = AckPageDlgProc;
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Product page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PRODUCTTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PRODUCTSUBTITLE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PRODUCT);
    psp.pfnDlgProc = ProductPageDlgProc;
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Locale page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LOCALETITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LOCALESUBTITLE);
    psp.pfnDlgProc = LocalePageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_LOCALEPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Owner page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_OWNERTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_OWNERSUBTITLE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_OWNERPAGE);
    psp.pfnDlgProc = OwnerPageDlgProc;
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Computer page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_COMPUTERTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_COMPUTERSUBTITLE);
    psp.pfnDlgProc = ComputerPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_COMPUTERPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the DateTime page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DATETIMETITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DATETIMESUBTITLE);
    psp.pfnDlgProc = DateTimePageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DATETIMEPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the theme selection page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_THEMESELECTIONTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_THEMESELECTIONSUBTITLE);
    psp.pfnDlgProc = ThemePageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_THEMEPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    pSetupData->uFirstNetworkWizardPage = IDD_PROCESSPAGE;
    pSetupData->uPostNetworkWizardPage = IDD_PROCESSPAGE;

    if (pfn)
    {
        pfn(&dwNetworkPageCount, &phpage[nPages], pSetupData);
        nPages += dwNetworkPageCount;
    }

    /* Create the Process page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
    psp.pfnDlgProc = ProcessPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Finish page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.pfnDlgProc = FinishDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
    phpage[nPages++] = CreatePropertySheetPage(&psp);

    ASSERT(nPages == dwPageCount);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_MODELESS;
    psh.hInstance = hDllInstance;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = phpage;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Create title font */
    pSetupData->hTitleFont = CreateTitleFont();
    pSetupData->hBoldFont  = CreateBoldFont();

    /* Display the wizard */
    hWnd = (HWND)PropertySheet(&psh);
    ShowWindow(hWnd, SW_SHOW);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!IsDialogMessage(hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteObject(pSetupData->hBoldFont);
    DeleteObject(pSetupData->hTitleFont);

    if (pSetupData->hSetupInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(pSetupData->hSetupInf);

done:
    if (phpage != NULL)
        HeapFree(GetProcessHeap(), 0, phpage);

    if (hNetShell != NULL)
        FreeLibrary(hNetShell);

    if (pSetupData != NULL)
        HeapFree(GetProcessHeap(), 0, pSetupData);

    LogItem(L"END_SECTION", L"InstallWizard");
}

/* EOF */
