/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         System setup
 * FILE:            dll/win32/syssetup/wizard.c
 * PURPOSE:         GUI controls
 * PROGRAMMERS:     Eric Kohl
 *                  Pierre Schweitzer <heis_spiter@hotmail.com>
 */

/* INCLUDES *****************************************************************/

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string.h>
#include <setupapi.h>
#include <pseh/pseh2.h>
#include <shlobj.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <syssetup/syssetup.h>

#define NDEBUG
#include <debug.h>

#include "globals.h"
#include "resource.h"

#define VMWINST

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

/* GLOBALS ******************************************************************/

SETUPDATA SetupData;


/* FUNCTIONS ****************************************************************/
BOOL
GetRosInstallCD(WCHAR * szPath, DWORD dwPathLength);

#ifdef VMWINST
static BOOL
RunVMWInstall(HWND hWnd)
{
  PROCESS_INFORMATION ProcInfo;
  MSG msg;
  DWORD ret;
  STARTUPINFOW si = {0};
  WCHAR InstallName[] = L"vmwinst.exe";

  si.cb = sizeof(STARTUPINFO);

  if(CreateProcessW(NULL, InstallName, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                   NULL, NULL, &si, &ProcInfo))
  {
    EnableWindow(hWnd, FALSE);
    for (;;)
    {
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        if (msg.message == WM_QUIT)
          goto done;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      ret = MsgWaitForMultipleObjects(1, &ProcInfo.hProcess, FALSE, INFINITE, QS_ALLEVENTS | QS_ALLINPUT);
      if (ret == WAIT_OBJECT_0)
        break;
    }
done:
    EnableWindow(hWnd, TRUE);

    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    return TRUE;
  }
  return FALSE;
}
#endif

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
  NONCLIENTMETRICSW ncm;
  LOGFONTW LogFont;
  HDC hdc;
  INT FontSize;
  HFONT hFont;

  ncm.cbSize = sizeof(NONCLIENTMETRICSW);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  LogFont = ncm.lfMessageFont;
  LogFont.lfWeight = FW_BOLD;
  wcscpy(LogFont.lfFaceName, L"MS Shell Dlg");

  hdc = GetDC(NULL);
  FontSize = 12;
  LogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
  hFont = CreateFontIndirectW(&LogFont);
  ReleaseDC(NULL, hdc);

  return hFont;
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
  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          PSETUPDATA SetupData;
          HWND hwndControl;
          DWORD dwStyle;

          /* Get pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;

          hwndControl = GetParent(hwndDlg);

          /* Center the wizard window */
          CenterWindow (hwndControl);

          /* Hide the system menu */
          dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
          SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

          /* Hide and disable the 'Cancel' button */
          hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
          ShowWindow (hwndControl, SW_HIDE);
          EnableWindow (hwndControl, FALSE);

          /* Set title font */
          SendDlgItemMessage(hwndDlg,
                             IDC_WELCOMETITLE,
                             WM_SETFONT,
                             (WPARAM)SetupData->hTitleFont,
                             (LPARAM)TRUE);
        }
        break;


      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Next button */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                if (SetupData.UnattendSetup)
                  {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_ACKPAGE);
                    return TRUE;
                  }
                break;

              case PSN_WIZBACK:
                SetupData.UnattendSetup = FALSE;
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

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          Projects = NULL;
          ProjectsSize = 256;
          do
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
          while (1);
          CurrentProject = Projects;
          while (L'\0' != *CurrentProject)
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
                if (SetupData.UnattendSetup)
                  {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_OWNERPAGE);
                    return TRUE;
                  }
                break;

              case PSN_WIZBACK:
                SetupData.UnattendSetup = FALSE;
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

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, EM_LIMITTEXT, 50, 0);
          SendDlgItemMessage(hwndDlg, IDC_OWNERORGANIZATION, EM_LIMITTEXT, 50, 0);

          /* Set focus to owner name */
          SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
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
                if (SetupData.UnattendSetup)
                  {
                    SendMessage(GetDlgItem(hwndDlg, IDC_OWNERNAME), WM_SETTEXT, 0, (LPARAM)SetupData.OwnerName);
                    SendMessage(GetDlgItem(hwndDlg, IDC_OWNERORGANIZATION), WM_SETTEXT, 0, (LPARAM)SetupData.OwnerOrganization);
                    if (WriteOwnerSettings(SetupData.OwnerName, SetupData.OwnerOrganization))
                      {
                        SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_COMPUTERPAGE);
                        return TRUE;
                      }
                  }
                break;

              case PSN_WIZNEXT:
                OwnerName[0] = 0;
                if (GetDlgItemTextW(hwndDlg, IDC_OWNERNAME, OwnerName, 50) == 0)
                {
                  if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
                  {
                    wcscpy(Title, L"ReactOS Setup");
                  }
                  if (0 == LoadStringW(hDllInstance, IDS_WZD_NAME, ErrorName, sizeof(ErrorName) / sizeof(ErrorName[0])))
                  {
                    wcscpy(ErrorName, L"Setup cannot continue until you enter your name.");
                  }
                  MessageBoxW(hwndDlg, ErrorName, Title, MB_ICONERROR | MB_OK);

                  SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);

                  return TRUE;
                }

                OwnerOrganization[0] = 0;
                GetDlgItemTextW(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                if (!WriteOwnerSettings(OwnerName, OwnerOrganization))
                  {
                    SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                    return TRUE;
                  }

              case PSN_WIZBACK:
                SetupData.UnattendSetup = FALSE;
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
      if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
      {
        wcscpy(Title, L"ReactOS Setup");
      }
      if (0 == LoadStringW(hDllInstance, IDS_WZD_SETCOMPUTERNAME, ErrorComputerName,
                           sizeof(ErrorComputerName) / sizeof(ErrorComputerName[0])))
      {
        wcscpy(ErrorComputerName, L"Setup failed to set the computer name.");
      }
      MessageBoxW(hwndDlg, ErrorComputerName, Title, MB_ICONERROR | MB_OK);

      return FALSE;
    }

  /* Try to also set DNS hostname */
  SetComputerNameExW(ComputerNamePhysicalDnsHostname, ComputerName);

  return TRUE;
}

static INT_PTR CALLBACK
ComputerPageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
  WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
  WCHAR Password1[15];
  WCHAR Password2[15];
  PWCHAR Password;
  WCHAR Title[64];
  WCHAR EmptyComputerName[256], NotMatchPassword[256], WrongPassword[256];
  DWORD Length;
  LPNMHDR lpnm;

  if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
  {
    wcscpy(Title, L"ReactOS Setup");
  }

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Retrieve current computer name */
          Length = MAX_COMPUTERNAME_LENGTH + 1;
          GetComputerNameW(ComputerName, &Length);

          /* Display current computer name */
          SetDlgItemTextW(hwndDlg, IDC_COMPUTERNAME, ComputerName);

          /* Set text limits */
          SendDlgItemMessage(hwndDlg, IDC_COMPUTERNAME, EM_LIMITTEXT, 64, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD1, EM_LIMITTEXT, 14, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD2, EM_LIMITTEXT, 14, 0);

          /* Set focus to computer name */
          SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
          if (SetupData.UnattendSetup)
            {
              SendMessage(GetDlgItem(hwndDlg, IDC_COMPUTERNAME), WM_SETTEXT, 0, (LPARAM)SetupData.ComputerName);
              SendMessage(GetDlgItem(hwndDlg, IDC_ADMINPASSWORD1), WM_SETTEXT, 0, (LPARAM)SetupData.AdminPassword);
              SendMessage(GetDlgItem(hwndDlg, IDC_ADMINPASSWORD2), WM_SETTEXT, 0, (LPARAM)SetupData.AdminPassword);
            }

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
                if (SetupData.UnattendSetup && WriteComputerSettings(SetupData.ComputerName, hwndDlg))
                  {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_LOCALEPAGE);
                    return TRUE;
                  }
                break;

              case PSN_WIZNEXT:
                if (GetDlgItemTextW(hwndDlg, IDC_COMPUTERNAME, ComputerName, 64) == 0)
                {
                  if (0 == LoadStringW(hDllInstance, IDS_WZD_COMPUTERNAME, EmptyComputerName,
                                       sizeof(EmptyComputerName) / sizeof(EmptyComputerName[0])))
                  {
                    wcscpy(EmptyComputerName, L"Setup cannot continue until you enter the name of your computer.");
                  }
                  MessageBoxW(hwndDlg, EmptyComputerName, Title, MB_ICONERROR | MB_OK);
                  SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* No need to check computer name for invalid characters,
                 * SetComputerName() will do it for us */

                if (!WriteComputerSettings(ComputerName, hwndDlg))
                  {
                    SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                    return TRUE;
                  }

#if 0
                /* Check if admin passwords have been entered */
                if ((GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD1, Password1, 15) == 0) ||
                    (GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD2, Password2, 15) == 0))
                {
                  if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDEMPTY, EmptyPassword,
                                       sizeof(EmptyPassword) / sizeof(EmptyPassword[0])))
                  {
                    wcscpy(EmptyPassword, L"You must enter a password !");
                  }
                  MessageBoxW(hwndDlg, EmptyPassword, Title, MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }
#else
                GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD1, Password1, 15);
                GetDlgItemTextW(hwndDlg, IDC_ADMINPASSWORD2, Password2, 15);
#endif
                /* Check if passwords match */
                if (wcscmp(Password1, Password2))
                {
                  if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDMATCH, NotMatchPassword,
                                       sizeof(NotMatchPassword) / sizeof(NotMatchPassword[0])))
                  {
                    wcscpy(NotMatchPassword, L"The passwords you entered do not match. Please enter the desired password again.");
                  }
                  MessageBoxW(hwndDlg, NotMatchPassword, Title, MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* Check password for invalid characters */
                Password = (PWCHAR)Password1;
                while (*Password)
                {
                  if (!isprint(*Password))
                  {
                    if (0 == LoadStringW(hDllInstance, IDS_WZD_PASSWORDCHAR, WrongPassword,
                                         sizeof(WrongPassword) / sizeof(WrongPassword[0])))
                    {
                      wcscpy(WrongPassword, L"The password you entered contains invalid characters. Please enter a cleaned password.");
                    }
                    MessageBoxW(hwndDlg, WrongPassword, Title, MB_ICONERROR | MB_OK);
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                    return TRUE;
                    break;
                  }
                  Password++;
                }

                /* FIXME: Set admin password */
                break;

              case PSN_WIZBACK:
                SetupData.UnattendSetup = FALSE;
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
	  _T("SYSTEM\\CurrentControlSet\\Control\\KeyboardLayouts\\"));
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
RunControlPanelApplet(HWND hwnd, WCHAR *lpCommandLine)
{
  STARTUPINFOW StartupInfo;
  PROCESS_INFORMATION ProcessInformation;

  ZeroMemory(&StartupInfo, sizeof(STARTUPINFOW));
  StartupInfo.cb = sizeof(STARTUPINFOW);

  if (!CreateProcessW(NULL,
                       lpCommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       NULL,
                       &StartupInfo,
                       &ProcessInformation))
    {
      MessageBoxW(hwnd, L"Error: failed to launch rundll32", NULL, MB_ICONERROR);
      return FALSE;
    }

  WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
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

  if (GetLocaleInfoW(MAKELCID(lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Locale, sizeof(Locale) / sizeof(Locale[0])) != 0)
  {
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\International",
                       0, NULL, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
       RegSetValueExW(hKey, L"Locale", 0, REG_SZ, (LPBYTE)Locale, (wcslen(Locale)+1) * sizeof(WCHAR));
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
  WCHAR szBuffer[1024];

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
          {
            wcscpy(szBuffer, L"rundll32.exe shell32.dll,Control_RunDLL intl.cpl,,5");
            RunControlPanelApplet(hwndDlg, szBuffer);
            /* FIXME: Update input locale name */
          }
          break;

        case IDC_CUSTOMLAYOUT:
          {
            wcscpy(szBuffer, L"rundll32.exe shell32.dll,Control_RunDLL input.dll,@1");
            RunControlPanelApplet(hwndDlg, szBuffer);
          }
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
                    WCHAR szPath[MAX_PATH];
                    if (GetRosInstallCD(szPath, MAX_PATH))
                      {
                        swprintf(szBuffer, L"rundll32.exe shell32.dll,Control_RunDLL intl.cpl,,/f:\"%S\\reactos\\unattend.inf\"", szPath);
                      }
                    else
                      {
                        wcscpy(szBuffer, L"rundll32.exe shell32.dll,Control_RunDLL intl.cpl,,/f:\"unattend.inf\"");
                      }

                    RunControlPanelApplet(hwndDlg, szBuffer);
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DATETIMEPAGE);
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

      dwValueSize = 64 * sizeof(TCHAR);
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

      dwValueSize = 32 * sizeof(TCHAR);
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

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		   L"SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
		   0,
		   KEY_ALL_ACCESS,
		   &hKey))
    return FALSE;

  dwValueSize = 9 * sizeof(TCHAR);
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
  HANDLE hToken;
  DWORD PrevSize;
  TOKEN_PRIVILEGES priv, previouspriv;
  BOOL Ret = FALSE;

  /*
   * enable the SeSystemtimePrivilege privilege
   */

  if(OpenProcessToken(GetCurrentProcess(),
                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                      &hToken))
  {
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if(LookupPrivilegeValue(NULL,
                            SE_SYSTEMTIME_NAME,
                            &priv.Privileges[0].Luid))
    {
      if(AdjustTokenPrivileges(hToken,
                               FALSE,
                               &priv,
                               sizeof(previouspriv),
                               &previouspriv,
                               &PrevSize) &&
         GetLastError() == ERROR_SUCCESS)
      {
        /*
         * We successfully enabled it, we're permitted to change the system time
         * Call SetLocalTime twice to ensure correct results
         */
        Ret = SetLocalTime(&SetupData->SystemTime) &&
              SetLocalTime(&SetupData->SystemTime);

        /*
         * for the sake of security, restore the previous status again
         */
        if(previouspriv.PrivilegeCount > 0)
        {
          AdjustTokenPrivileges(hToken,
                                FALSE,
                                &previouspriv,
                                0,
                                NULL,
                                0);
        }
      }
    }
    CloseHandle(hToken);
  }

  return Ret;
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
  if(!SetSystemLocalTime(hwndDlg, SetupData))
    {
      if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
      {
        wcscpy(Title, L"ReactOS Setup");
      }
      if (0 == LoadStringW(hDllInstance, IDS_WZD_LOCALTIME, ErrorLocalTime,
                           sizeof(ErrorLocalTime) / sizeof(ErrorLocalTime[0])))
      {
        wcscpy(ErrorLocalTime, L"Setup failed to set the computer name.");
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
  WCHAR Title[64];
  WCHAR ErrorLocalTime[256];

  /* Retrieve pointer to the global setup data */
  SetupData = (PSETUPDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
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
                               SetupData, 85 /* GMT time zone */);

              SendDlgItemMessage(hwndDlg, IDC_AUTODAYLIGHT, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
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
                if (SetupData->UnattendSetup && WriteDateTimeSettings(hwndDlg, SetupData))
                  {
                    SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_PROCESSPAGE);
                    return TRUE;
                  }
                break;

              case PSN_WIZNEXT:
                {
                  GetLocalSystemTime(hwndDlg, SetupData);
                  SetLocalTimeZone(GetDlgItem(hwndDlg, IDC_TIMEZONELIST),
                                   SetupData);
                  SetAutoDaylightInfo(GetDlgItem(hwndDlg, IDC_AUTODAYLIGHT));
                  if(!SetSystemLocalTime(hwndDlg, SetupData))
                  {
                    if (0 == LoadStringW(hDllInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
                    {
                      wcscpy(Title, L"ReactOS Setup");
                    }
                    if (0 == LoadStringW(hDllInstance, IDS_WZD_LOCALTIME, ErrorLocalTime,
                                         sizeof(ErrorLocalTime) / sizeof(ErrorLocalTime[0])))
                    {
                      wcscpy(ErrorLocalTime, L"Setup failed to set the computer name.");
                    }
                    MessageBoxW(hwndDlg, ErrorLocalTime, Title, MB_ICONWARNING | MB_OK);
                  }
                }
                break;

              case PSN_WIZBACK:
                SetupData->UnattendSetup = FALSE;
                break;

              default:
                break;
            }
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
                                   sizeof(ErrorMessage) /
                                   sizeof(ErrorMessage[0])))
                {
                  ErrorMessage[0] = L'\0';
                }
              if (SPREG_TIMEOUT != StatusInfo->FailureCode)
                {
                  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                                 StatusInfo->Win32Error, 0,
                                 ErrorMessage + wcslen(ErrorMessage),
                                 sizeof(ErrorMessage) / sizeof(ErrorMessage[0]) -
                                 wcslen(ErrorMessage), NULL);
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
                               sizeof(UnknownError) / sizeof(UnknownError[0] -
                               20)))
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
  if (! SetupFindFirstLineW(hSysSetupInf, L"RegistrationPhase2",
                            L"RegisterDlls", &Context))
    {
      DPRINT1("No RegistrationPhase2 section found\n");
      return FALSE;
    }
  if (! SetupGetStringFieldW(&Context, 1, SectionName,
                             sizeof(SectionName) / sizeof(SectionName[0]),
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
  if (NULL != RegistrationData)
    {
      RegistrationData->hwndDlg = hwndDlg;
      RegistrationData->DllCount = DllCount;
      RegistrationThread = CreateThread(NULL, 0, RegistrationProc,
                                        (LPVOID) RegistrationData, 0, NULL);
      if (NULL != RegistrationThread)
        {
          CloseHandle(RegistrationThread);
        }
      else
        {
          DPRINT1("CreateThread failed, error %u\n", GetLastError());
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
                               sizeof(Activity) / sizeof(Activity[0])))
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
                                   Title, sizeof(Title) / sizeof(Title[0])))
                {
                  wcscpy(Title, L"ReactOS Setup");
                }
              MessageBoxW(hwndDlg, RegistrationNotify->ErrorMessage,
                          Title, MB_ICONERROR | MB_OK);

            }

          if (wParam)
            {
#ifdef VMWINST
              if(!SetupData->UnattendSetup && !SetupData->DisableVmwInst)
                RunVMWInstall(GetParent(hwndDlg));
#endif

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
SetupIsActive( DWORD dw )
{
  HKEY hKey = 0;
  if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_WRITE, &hKey ) == ERROR_SUCCESS) {
    RegSetValueExW( hKey, L"SystemSetupInProgress", 0, REG_DWORD, (CONST BYTE *)&dw, sizeof(dw) );
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
          PSETUPDATA SetupData;

          /* Get pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;

          /* Set title font */
          SendDlgItemMessage(hwndDlg,
                             IDC_FINISHTITLE,
                             WM_SETFONT,
                             (WPARAM)SetupData->hTitleFont,
                             (LPARAM)TRUE);
          if (SetupData->UnattendSetup)
          {
            KillTimer(hwndDlg, 1);
            SetupIsActive(0);
            PostQuitMessage(0);
          }
        }
        break;

      case WM_DESTROY:
         {
           SetupIsActive(0);
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
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);

                SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0,
                                   MAKELPARAM(0, 300));
                SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                SetTimer(hwndDlg, 1, 50, NULL);
                break;

              case PSN_WIZBACK:
                /* Handle a Back button click, if necessary */
                KillTimer(hwndDlg, 1);

                /* Skip the progress page */
                SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DATETIMEPAGE);
                SetupData.UnattendSetup = FALSE;
                return TRUE;

              case PSN_WIZFINISH:
                /* Handle a Finish button click, if necessary */
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
BOOL
ProcessUnattendInf(HINF hUnattendedInf)
{
  INFCONTEXT InfContext;
  WCHAR szName[256];
  WCHAR szValue[MAX_PATH];
  DWORD LineLength;
  HKEY hKey;

  if (!SetupFindFirstLineW(hUnattendedInf,
              L"Unattend",
              L"UnattendSetupEnabled",
              &InfContext))
    {
      DPRINT1("Error: Cant find UnattendSetupEnabled Key! %d\n", GetLastError());
      return FALSE;
    }

   if (!SetupGetStringFieldW(&InfContext,
               1,
               szValue,
               sizeof(szValue) / sizeof(WCHAR),
               &LineLength))
    {
      DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
      return FALSE;
    }

  if (wcscmp(szValue, L"yes") != 0)
    {
      DPRINT("Unattend setup was disabled by UnattendSetupEnabled key.\n");
      return FALSE;
    }

  if (!SetupFindFirstLineW(hUnattendedInf,
              L"Unattend",
              NULL,
              &InfContext))
    {
      DPRINT1("Error: SetupFindFirstLine failed %d\n", GetLastError());
      return FALSE;
    }


  do
  {
    if (!SetupGetStringFieldW(&InfContext,
                 0,
                 szName,
                 sizeof(szName) / sizeof(WCHAR),
                 &LineLength))
      {
        DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
        return FALSE;
      }

    if (!SetupGetStringFieldW(&InfContext,
                 1,
                 szValue,
                 sizeof(szValue) / sizeof(WCHAR),
                 &LineLength))
      {
        DPRINT1("Error: SetupGetStringField failed with %d\n", GetLastError());
        return FALSE;
      }
    DPRINT1("Name %S Value %S\n",szName, szValue);
    if (!wcscmp(szName, L"FullName"))
      {
        if ((sizeof(SetupData.OwnerName) / sizeof(TCHAR)) > LineLength)
          {
            wcscpy(SetupData.OwnerName, szValue);
          }
      }
    else if (!wcscmp(szName, L"OrgName"))
      {
        if ((sizeof(SetupData.OwnerOrganization) / sizeof(WCHAR)) > LineLength)
          {
            wcscpy(SetupData.OwnerOrganization, szValue);
          }
      }
    else if (!wcscmp(szName, L"ComputerName"))
      {
        if ((sizeof(SetupData.ComputerName) / sizeof(WCHAR)) > LineLength)
        {
          wcscpy(SetupData.ComputerName, szValue);
        }
    }
    else if (!wcscmp(szName, L"AdminPassword"))
      {
        if ((sizeof(SetupData.AdminPassword) / sizeof(WCHAR)) > LineLength)
          {
            wcscpy(SetupData.AdminPassword, szValue);
          }
      }
    else if (!wcscmp(szName, L"TimeZoneIndex"))
      {
        SetupData.TimeZoneIndex = _wtoi(szValue);
      }
    else if (!wcscmp(szName, L"DisableAutoDaylightTimeSet"))
      {
        SetupData.DisableAutoDaylightTimeSet = _wtoi(szValue);
      }
    else if (!wcscmp(szName, L"DisableVmwInst"))
      {
        if(!wcscmp(szValue, L"yes"))
            SetupData.DisableVmwInst = 1;
        else
            SetupData.DisableVmwInst = 0;
      }
    else if (!wcscmp(szName, L"BootCDRegTestActive"))
      {
        SetupData.BootCDRegtestActive = _wtoi(szValue);
      }

  }
  while (SetupFindNextLine(&InfContext, &InfContext));

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                        0,
                        KEY_SET_VALUE,
                        &hKey) != ERROR_SUCCESS)
    {
      DPRINT1("Error: failed to open HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\n");
      return TRUE;
    }


  if (SetupFindFirstLineW(hUnattendedInf,
                         L"GuiRunOnce",
                         NULL,
                         &InfContext))
    {

      int i = 0;
      do
      {
        if(SetupGetStringFieldW(&InfContext,
                               0,
                               szValue,
                               sizeof(szValue) / sizeof(WCHAR),
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
                                 (wcslen(szPath)+1) * sizeof(WCHAR)) == ERROR_SUCCESS)
                 {
                   i++;
                 }
             }
          }
      }while(SetupFindNextLine(&InfContext, &InfContext));
   }

  if (SetupData.BootCDRegtestActive)
    {
      char szPath[MAX_PATH];
      FILE * file;
      WIN32_FIND_DATAA ffd;
      HANDLE hFind = INVALID_HANDLE_VALUE;
#if 0
      if (!SHGetSpecialFolderPathA(0, szPath, CSIDL_DESKTOP, FALSE))
        {
          /* failed to get desktop path */
            strcpy(szPath, "C:");
        }
      strcat(szPath, "\\sysregtest.bat");
#else
      strcpy(szPath, "C:\\sysregtest.bat");
#endif
      file = fopen(szPath, "w+");
      if (!file)
        {
          DPRINT1("Error: failed create sysregtest.bat");
          RegCloseKey(hKey);
          return TRUE;
        }

      RegSetValueExA(hKey,
                    "BootCDRegtestActive",
                    0,
                    REG_SZ,
                    (const BYTE*)szPath,
                     (strlen(szPath)+1) * sizeof(char));


      /* winetests */
      hFind = FindFirstFileA("c:\\reactos\\bin\\*.exe", &ffd); /* %windir% isn't working on ros */
      if (hFind != INVALID_HANDLE_VALUE)
      {
        do
        {
          if (ffd.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY)
            fprintf(file, "%s%s\n", "dbgprint --winetest %windir%\\bin\\", ffd.cFileName);
        }
        while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
      }

      fprintf(file, "%s\n", "dbgprint SYSREG_CHECKPOINT:THIRDBOOT_COMPLETE");
      fprintf(file, "%s\n", "shutdown -s");
      fclose(file);
    }
    RegCloseKey(hKey);
    return TRUE;
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
GetRosInstallCD(WCHAR * szPath, DWORD dwPathLength)
{
  WCHAR szDrives[512];
  WCHAR szDrive[] = L"D:\\";
  DWORD dwLength, dwIndex;
  WCHAR * pDrive;
  dwLength = GetLogicalDriveStringsW(sizeof(szDrives) / sizeof(WCHAR), szDrives);

  if (dwLength > (sizeof(szDrives) / sizeof(WCHAR)) || dwLength == 0)
  {
    /* buffer too small or failure */
     LogItem(SYSSETUP_SEVERITY_INFORMATION, L"GetLogicalDriveStringsW failed");
    return FALSE;
  }

  pDrive = szDrives;
  for (dwIndex = 0; dwIndex < dwLength; dwIndex++)
  {
    szDrive[0] = pDrive[dwIndex];
    if (GetDriveTypeW(szDrive) == DRIVE_CDROM)
    {
        WCHAR szBuffer[MAX_PATH];
        wcscpy(szBuffer, szDrive);
        wcscat(szBuffer, L"reactos\\ntoskrnl.exe");
        LogItem(SYSSETUP_SEVERITY_INFORMATION, szBuffer);
        if (FileExists(szBuffer, NULL))
        {
            wcsncpy(szPath, szDrive, dwPathLength);
            return TRUE;
        }
    }
  }
  return FALSE;
}

VOID
ProcessUnattendSetup()
{
  WCHAR szPath[MAX_PATH];
  HINF hUnattendedInf;
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

  hUnattendedInf = SetupOpenInfFileW(szPath,
                           NULL,
                           INF_STYLE_OLDNT,
                           NULL);

  if (hUnattendedInf != INVALID_HANDLE_VALUE)
  {
    SetupData.UnattendSetup = ProcessUnattendInf(hUnattendedInf);
    SetupCloseInfFile(hUnattendedInf);
  }
}


VOID
InstallWizard(VOID)
{
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE ahpsp[8];
  PROPSHEETPAGE psp = {0};
  UINT nPages = 0;

  /* Clear setup data */
  ZeroMemory(&SetupData, sizeof(SETUPDATA));

  ProcessUnattendSetup();


  /* Create the Welcome page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.hInstance = hDllInstance;
  psp.lParam = (LPARAM)&SetupData;
  psp.pfnDlgProc = WelcomeDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create the Acknowledgements page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ACKTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ACKSUBTITLE);
  psp.pszTemplate = MAKEINTRESOURCE(IDD_ACKPAGE);
  psp.pfnDlgProc = AckPageDlgProc;
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create the Owner page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_OWNERTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_OWNERSUBTITLE);
  psp.pszTemplate = MAKEINTRESOURCE(IDD_OWNERPAGE);
  psp.pfnDlgProc = OwnerPageDlgProc;
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create the Computer page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_COMPUTERTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_COMPUTERSUBTITLE);
  psp.pfnDlgProc = ComputerPageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_COMPUTERPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);


  /* Create the Locale page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LOCALETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LOCALESUBTITLE);
  psp.pfnDlgProc = LocalePageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_LOCALEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);


  /* Create the DateTime page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DATETIMETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DATETIMESUBTITLE);
  psp.pfnDlgProc = DateTimePageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_DATETIMEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);


  /* Create the Process page */
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
  psp.pfnDlgProc = ProcessPageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);


  /* Create the Finish page */
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.pfnDlgProc = FinishDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create the property sheet */
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
  psh.hInstance = hDllInstance;
  psh.hwndParent = NULL;
  psh.nPages = nPages;
  psh.nStartPage = 0;
  psh.phpage = ahpsp;
  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  /* Create title font */
  SetupData.hTitleFont = CreateTitleFont();

  /* Display the wizard */
  PropertySheet(&psh);

  DeleteObject(SetupData.hTitleFont);
}

/* EOF */
