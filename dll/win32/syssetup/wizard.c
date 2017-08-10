/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         System setup
 * FILE:            dll/win32/syssetup/wizard.c
 * PURPOSE:         GUI controls
 * PROGRAMMERS:     Eric Kohl
 *                  Pierre Schweitzer <heis_spiter@hotmail.com>
 *                  Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <stdlib.h>
#include <time.h>
#include <winnls.h>
#include <windowsx.h>
#include <wincon.h>
#include <shlobj.h>

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


/* FUNCTIONS ****************************************************************/

extern void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

BOOL
GetRosInstallCD(WCHAR *pwszPath, DWORD cchPathMax);


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

    LogFont.lfWeight = FW_HEAVY;
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
    tmpFont.lfWeight = FW_HEAVY;
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_ACKPAGE);
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_LOCALEPAGE);
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
                            SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_COMPUTERPAGE);
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);

                        return TRUE;
                    }

                    OwnerOrganization[0] = 0;
                    GetDlgItemTextW(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                    if (!WriteOwnerSettings(OwnerName, OwnerOrganization))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
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

    /* Try to also set DNS hostname */
    SetComputerNameExW(ComputerNamePhysicalDnsHostname, ComputerName);

    /* Set the accounts domain name */
    SetAccountsDomainSid(NULL, ComputerName);

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
                           L"DefaultDomain",
                           0,
                           REG_SZ,
                           (LPBYTE)Domain,
                           (wcslen(Domain)+ 1) * sizeof(WCHAR));
    if (lError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueEx(\"DefaultDomain\") failed!\n");
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_THEMEPAGE);
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
                        return TRUE;
                    }

                    /* No need to check computer name for invalid characters,
                     * SetComputerName() will do it for us */

                    if (!WriteComputerSettings(ComputerName, hwndDlg))
                    {
                        SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
                        return TRUE;
                    }

#if 0
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
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
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
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
                            SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
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
SetKeyboardLayoutName(HWND hwnd)
{
#if 0
    TCHAR szLayoutPath[256];
    TCHAR szLocaleName[32];
    DWORD dwLocaleSize;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SYSTEM\\CurrentControlSet\\Control\\NLS\\Locale"),
                     0,
                     KEY_ALL_ACCESS,
                     &hKey))
        return;

    dwValueSize = 16 * sizeof(TCHAR);
    if (RegQueryValueEx(hKey,
                        NULL,
                        NULL,
                        NULL,
                        szLocaleName,
                        &dwLocaleSize))
    {
        RegCloseKey(hKey);
        return;
    }

    _tcscpy(szLayoutPath,
            _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\"));
    _tcscat(szLayoutPath,
            szLocaleName);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szLayoutPath,
                     0,
                     KEY_ALL_ACCESS,
                     &hKey))
        return;

    dwValueSize = 32 * sizeof(TCHAR);
    if (RegQueryValueEx(hKey,
                        _T("Layout Text"),
                        NULL,
                        NULL,
                        szLocaleName,
                        &dwLocaleSize))
    {
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);
#endif
}


static BOOL
RunControlPanelApplet(HWND hwnd, PCWSTR pwszCPLParameters)
{
    MSG msg;
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

    while ((MsgWaitForMultipleObjects(1, &ProcessInformation.hProcess, FALSE, INFINITE, QS_ALLINPUT|QS_ALLPOSTMESSAGE )) != WAIT_OBJECT_0)
    { 
       while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
       {
           TranslateMessage(&msg);
           DispatchMessageW(&msg);
       }
    }
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);
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
    SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);
            WriteUserLocale();

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
                        /* FIXME: Update input locale name */
                        break;

                    case IDC_CUSTOMLAYOUT:
                        RunControlPanelApplet(hwndDlg, L"input.dll,@1");
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
                        WCHAR wszPath[MAX_PATH];
                        if (GetRosInstallCD(wszPath, _countof(wszPath)))
                        {
                            WCHAR wszParams[1024];
                            swprintf(wszParams, L"intl.cpl,,/f:\"%sreactos\\unattend.inf\"", wszPath);
                            RunControlPanelApplet(hwndDlg, wszParams);
                        }
                        else
                        {
                            RunControlPanelApplet(hwndDlg, L"intl.cpl,,/f:\"unattend.inf\"");
                        }

                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_OWNERPAGE);
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


static VOID
CreateTimeZoneList(PSETUPDATA SetupData)
{
    WCHAR szKeyName[256];
    DWORD dwIndex;
    DWORD dwNameSize;
    DWORD dwValueSize;
    LONG lError;
    HKEY hZonesKey;
    HKEY hZoneKey;

    PTIMEZONE_ENTRY Entry;
    PTIMEZONE_ENTRY Current;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                      0,
                      KEY_ALL_ACCESS,
                      &hZonesKey))
        return;

    dwIndex = 0;
    while (TRUE)
    {
        dwNameSize = 256 * sizeof(WCHAR);
        lError = RegEnumKeyExW(hZonesKey,
                               dwIndex,
                               szKeyName,
                               &dwNameSize,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (lError != ERROR_SUCCESS && lError != ERROR_MORE_DATA)
            break;

        if (RegOpenKeyExW(hZonesKey,
                          szKeyName,
                          0,
                          KEY_ALL_ACCESS,
                          &hZoneKey))
            break;

        Entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEZONE_ENTRY));
        if (Entry == NULL)
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 64 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Display",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->Description,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 32 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Std",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->StandardName,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = 32 * sizeof(WCHAR);
        if (RegQueryValueExW(hZoneKey,
                             L"Dlt",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->DaylightName,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = sizeof(DWORD);
        if (RegQueryValueExW(hZoneKey,
                             L"Index",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->Index,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        dwValueSize = sizeof(TZ_INFO);
        if (RegQueryValueExW(hZoneKey,
                             L"TZI",
                             NULL,
                             NULL,
                             (LPBYTE)&Entry->TimezoneInfo,
                             &dwValueSize))
        {
            RegCloseKey(hZoneKey);
            break;
        }

        RegCloseKey(hZoneKey);

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

        dwIndex++;
    }

    RegCloseKey(hZonesKey);
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

static BOOL
GetTimeZoneListIndex(LPDWORD lpIndex)
{
    WCHAR szLanguageIdString[9];
    HKEY hKey;
    DWORD dwValueSize;
    DWORD Length;
    LPWSTR Buffer;
    LPWSTR Ptr;
    LPWSTR End;
    BOOL bFound = FALSE;
    unsigned long iLanguageID;

    if (*lpIndex == -1)
    {
        *lpIndex = 85; /* fallback to GMT time zone */

        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
                          0,
                          KEY_ALL_ACCESS,
                          &hKey))
            return FALSE;

        dwValueSize = 9 * sizeof(WCHAR);
        if (RegQueryValueExW(hKey,
                             L"Default",
                             NULL,
                             NULL,
                             (LPBYTE)szLanguageIdString,
                             &dwValueSize))
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        iLanguageID = wcstoul(szLanguageIdString, NULL, 16);
        RegCloseKey(hKey);
    }
    else
    {
        iLanguageID = *lpIndex;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                      0,
                      KEY_ALL_ACCESS,
                      &hKey))
        return FALSE;

    dwValueSize = 0;
    if (RegQueryValueExW(hKey,
                         L"IndexMapping",
                         NULL,
                         NULL,
                         NULL,
                         &dwValueSize))
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueSize);
    if (Buffer == NULL)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if (RegQueryValueExW(hKey,
                         L"IndexMapping",
                         NULL,
                         NULL,
                         (LPBYTE)Buffer,
                         &dwValueSize))
    {
        HeapFree(GetProcessHeap(), 0, Buffer);
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    Ptr = Buffer;
    while (*Ptr != 0)
    {
        Length = wcslen(Ptr);
        if (wcstoul(Ptr, NULL, 16) == iLanguageID)
            bFound = TRUE;

        Ptr = Ptr + Length + 1;
        if (*Ptr == 0)
            break;

        if (bFound)
        {
            *lpIndex = wcstoul(Ptr, &End, 10);
            HeapFree(GetProcessHeap(), 0, Buffer);
            return TRUE;
        }

        Length = wcslen(Ptr);
        Ptr = Ptr + Length + 1;
    }

    HeapFree(GetProcessHeap(), 0, Buffer);

    return FALSE;
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


static VOID
SetAutoDaylightInfo(HWND hwnd)
{
    HKEY hKey;
    DWORD dwValue = 1;

    if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
                          0,
                          KEY_SET_VALUE,
                          &hKey))
            return;

        RegSetValueExW(hKey,
                       L"DisableAutoDaylightTimeSet",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwValue,
                       sizeof(DWORD));
        RegCloseKey(hKey);
    }
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
UpdateLocalSystemTime(HWND hwnd)
{
    SYSTEMTIME LocalTime;

    GetLocalTime(&LocalTime);
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

    SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
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
    SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);

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

        case WM_TIMER:
            UpdateLocalSystemTime(hwndDlg);
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (SetupData->UnattendSetup && WriteDateTimeSettings(hwndDlg, SetupData))
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, SetupData->uFirstNetworkWizardPage);
                        return TRUE;
                    }
                    SetTimer(hwndDlg, 1, 1000, NULL);
                    break;

                case PSN_KILLACTIVE:
                case DTN_DATETIMECHANGE:
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


static INT_PTR CALLBACK
ThemePageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PSETUPDATA SetupData;

    /* Retrieve pointer to the global setup data */
    SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            BUTTON_IMAGELIST imldata = {0, {0,10,0,10}, BUTTON_IMAGELIST_ALIGN_TOP};
            
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);
            
            imldata.himl = ImageList_LoadImage(hDllInstance, MAKEINTRESOURCE(IDB_CLASSIC), 0, 0, CLR_NONE , IMAGE_BITMAP, LR_CREATEDIBSECTION);
            SendDlgItemMessage(hwndDlg, IDC_CLASSICSTYLE, BCM_SETIMAGELIST, 0, (LPARAM)&imldata);

            imldata.himl = ImageList_LoadImage(hDllInstance, MAKEINTRESOURCE(IDB_LAUTUS), 0, 0, CLR_NONE , IMAGE_BITMAP, LR_CREATEDIBSECTION);
            SendDlgItemMessage(hwndDlg, IDC_THEMEDSTYLE, BCM_SETIMAGELIST, 0, (LPARAM)&imldata);
            
            SendDlgItemMessage(hwndDlg, IDC_CLASSICSTYLE, BM_SETCHECK, BST_CHECKED, 0);
            break;
        }
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_THEMEDSTYLE:
                    {
                        WCHAR wszParams[1024];
                        WCHAR wszTheme[MAX_PATH];
                        WCHAR* format = L"desk.cpl desk,@Appearance /Action:ActivateMSTheme /file:\"%s\"";

                        SHGetFolderPathAndSubDirW(0, CSIDL_RESOURCES, NULL, SHGFP_TYPE_DEFAULT, L"themes\\lautus\\lautus.msstyles", wszTheme);
                        swprintf(wszParams, format, wszTheme);
                        RunControlPanelApplet(hwndDlg, wszParams);
                        break;
                    }
                    case IDC_CLASSICSTYLE:
                        RunControlPanelApplet(hwndDlg, L"desk.cpl desk,@Appearance /Action:ActivateMSTheme");
                        break;
                }
            }
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (SetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, SetupData->uFirstNetworkWizardPage);
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
    SendMessage(RegistrationData->hwndDlg, PM_REGISTRATION_NOTIFY,
                1, (LPARAM) &RegistrationNotify);
    if (NULL != RegistrationNotify.ErrorMessage &&
            UnknownError != RegistrationNotify.ErrorMessage)
    {
        LocalFree((PVOID) RegistrationNotify.ErrorMessage);
    }

    SetupTermDefaultQueueCallback(RegistrationData->DefaultContext);
    HeapFree(GetProcessHeap(), 0, RegistrationData);

    RegisterTypeLibraries(hSysSetupInf, L"TypeLibraries");

    // FIXME: Move this call to a separate cleanup page!
    RtlCreateBootStatusDataFile();

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
    SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Save pointer to the global setup data */
            SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)SetupData);
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


VOID
ProcessUnattendInf(
    PSETUPDATA pSetupData)
{
    INFCONTEXT InfContext;
    WCHAR szName[256];
    WCHAR szValue[MAX_PATH];
    DWORD LineLength;
    HKEY hKey;

    if (!SetupFindFirstLineW(pSetupData->hUnattendedInf,
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

    if (!SetupFindFirstLineW(pSetupData->hUnattendedInf,
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

    } while (SetupFindNextLine(&InfContext, &InfContext));

    if (SetupFindFirstLineW(pSetupData->hUnattendedInf,
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

    if (SetupFindFirstLineW(pSetupData->hUnattendedInf,
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

/*
 * GetRosInstallCD should find the path to ros installation medium
 * BUG 1
 * If there are more than one CDDrive in it containing a ReactOS
 * installation cd, then it will pick the first one regardless if
 * it is really the installation cd
 *
 * The best way to implement this is to set the key
 * HKLM\Software\Microsoft\Windows NT\CurrentVersion\SourcePath (REG_SZ)
 */

BOOL
GetRosInstallCD(WCHAR *pwszPath, DWORD cchPathMax)
{
    WCHAR wszDrives[512];
    DWORD cchDrives;
    WCHAR *pwszDrive;

    cchDrives = GetLogicalDriveStringsW(_countof(wszDrives) - 1, wszDrives);
    if (cchDrives == 0 || cchDrives >= _countof(wszDrives))
    {
        /* buffer too small or failure */
        LogItem(NULL, L"GetLogicalDriveStringsW failed");
        return FALSE;
    }

    for (pwszDrive = wszDrives; pwszDrive[0]; pwszDrive += wcslen(pwszDrive) + 1)
    {
        if (GetDriveTypeW(pwszDrive) == DRIVE_CDROM)
        {
            WCHAR wszBuf[MAX_PATH];
            wsprintf(wszBuf, L"%sreactos\\system32\\ntoskrnl.exe", pwszDrive);
            LogItem(NULL, wszBuf);
            if (GetFileAttributesW(wszBuf) != INVALID_FILE_ATTRIBUTES)
            {
                /* the file exists, so this is the right drive */
                wcsncpy(pwszPath, pwszDrive, cchPathMax);
                OutputDebugStringW(L"GetRosInstallCD: ");OutputDebugStringW(pwszPath);OutputDebugStringW(L"\n");
                return TRUE;
            }
        }
    }
    return FALSE;
}


VOID
ProcessUnattendSetup(
    PSETUPDATA pSetupData)
{
    WCHAR szPath[MAX_PATH];
    DWORD dwLength;

    if (!GetRosInstallCD(szPath, MAX_PATH))
    {
        /* no cd drive found */
        return;
    }

    dwLength = wcslen(szPath);
    if (dwLength + 21 > MAX_PATH)
    {
        /* FIXME
         * allocate bigger buffer
         */
        return;
    }

    wcscat(szPath, L"reactos\\unattend.inf");

    pSetupData->hUnattendedInf = SetupOpenInfFileW(szPath,
                                                   NULL,
                                                   INF_STYLE_OLDNT,
                                                   NULL);
    if (pSetupData->hUnattendedInf != INVALID_HANDLE_VALUE)
    {
        ProcessUnattendInf(pSetupData);
    }
}

typedef DWORD(WINAPI *PFNREQUESTWIZARDPAGES)(PDWORD, HPROPSHEETPAGE *, PSETUPDATA);

BOOL ActivateComctl32v6ActCtx(ULONG_PTR *cookie, HANDLE* hActCtx)
{
    ACTCTXW ActCtx = {sizeof(ACTCTX), ACTCTX_FLAG_RESOURCE_NAME_VALID};
    WCHAR fileBuffer[MAX_PATH];

    *hActCtx = INVALID_HANDLE_VALUE;

    if (!GetModuleFileNameW(hDllInstance, fileBuffer, ARRAYSIZE(fileBuffer)))
        return FALSE;

    ActCtx.lpSource = fileBuffer;
    ActCtx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
    *hActCtx = CreateActCtx(&ActCtx);
    if (*hActCtx == INVALID_HANDLE_VALUE)
        return FALSE;

    return ActivateActCtx(*hActCtx, cookie);
}

VOID
InstallWizard(VOID)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE *phpage = NULL;
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;
    HWND hWnd;
    MSG msg;
    PSETUPDATA pSetupData = NULL;
    HMODULE hNetShell = NULL;
    PFNREQUESTWIZARDPAGES pfn = NULL;
    DWORD dwPageCount = 8, dwNetworkPageCount = 0;
    BOOL bActCtxActivated;
    ULONG_PTR cookie;
    HANDLE hActCtx;

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

    /* Load and activate the act ctx for comctl32v6 now manually. 
     *  Even if the exe of the process had a manifest, at the point of its launch
     *  the manifest of comctl32 wouldn't be installed so it wouldn't be loaded at all */
    bActCtxActivated = ActivateComctl32v6ActCtx(&cookie, &hActCtx);

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

    pSetupData->hUnattendedInf = INVALID_HANDLE_VALUE;
    ProcessUnattendSetup(pSetupData);

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

    if (pSetupData->hUnattendedInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(pSetupData->hUnattendedInf);

done:
    if (phpage != NULL)
        HeapFree(GetProcessHeap(), 0, phpage);

    if (hNetShell != NULL)
        FreeLibrary(hNetShell);

    if (bActCtxActivated)
    {
        DeactivateActCtx(0, cookie);
        ReleaseActCtx(hActCtx);
    }

    if (pSetupData != NULL)
    {
        DeleteObject(pSetupData->hBoldFont);
        DeleteObject(pSetupData->hTitleFont);
        HeapFree(GetProcessHeap(), 0, pSetupData);
    }

    LogItem(L"END_SECTION", L"InstallWizard");
}

/* EOF */
