/******************************************************************************
**
** Module:          RNAUI.DLL
** File:            scripter.c
** Descriptions:    Remote Network Access Scripting dialog
** Contains:        Scripting tools dialog boxes
**
** Copyright (c) 1992-1993, Microsoft Corporation, all rights reserved
**
** History:         Thu 19-Aug-1993 -by-  Viroon  Touranachun [viroont]
**                   Created
**
**                  Tue 08-Nov-1994 -by-  Viroon  Touranachun [viroont]
**                   Moved from sessmgr.c
**
******************************************************************************/

#include "rnaui.h"
#include "scripter.h"
#include "rnahelp.h"

//**************************************************************************
//  Macros
//**************************************************************************

#define ISMINIMIZED(cmd) (cmd==SW_SHOWMINNOACTIVE || cmd==SW_SHOWMINIMIZED)

//**************************************************************************
//  Global Parameters
//**************************************************************************

#pragma data_seg(DATASEG_READONLY)
char const g_szProfile[] = REGSTR_KEY_PROF;
char const g_szScript[]  = REGSTR_VAL_SCRIPT;
char const g_szMode[]    = REGSTR_VAL_MODE;
char const g_szPlacement[]=REGSTR_VAL_TERM;

char const cszHelpFile[]    = "winhelp.hlp>proc4";
#pragma data_seg()

//****************************************************************************
// BOOL CALLBACK _export ScriptAppletDlgProc (HWND, UINT, WPARAM, LPARAM)
//
// This function handles the modal connection info setting dialog box.
//
// History:
//  Fri 05-May-1995 08:58:47  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

BOOL CALLBACK ScriptAppletDlgProc (HWND    hwnd,
                                   UINT    message,
                                   WPARAM  wParam,
                                   LPARAM  lParam)
{
  PCONNENTDLG pConnEntDlg;

  switch (message)
  {
    case WM_INITDIALOG:
      pConnEntDlg = (PCONNENTDLG)(((LPPROPSHEETPAGE)lParam)->lParam);
      SetWindowLong(hwnd, DWL_USER, (LONG)pConnEntDlg);

      // Initilialize the script page
      //
      InitScriptDlg(hwnd);
      break;

    case WM_DESTROY:
      DeInitScriptDlg (hwnd);
      break;

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaScripter, message, wParam, lParam);
      break;

    case WM_NOTIFY:
      switch(((NMHDR FAR *)lParam)->code)
      {
        case PSN_KILLACTIVE:
          //
          // Update the selected SMM settings
          //
          SetWindowLong(hwnd, DWL_MSGRESULT,
                        (LONG)(CheckScriptDlgData(hwnd) != ERROR_SUCCESS));
          return TRUE;

        case PSN_APPLY:
          //
          // The property sheet information is permanently applied
          //
          SaveScriptDlgData(hwnd);
          return FALSE;

        default:
          break;
      };
      break;

    case WM_COMMAND:

      // Determine the end-user action
      //
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
        case IDC_SCRIPT_NAME:
          //
          // Adjust the dialog appearance
          //
          if (GET_WM_COMMAND_CMD(wParam, lParam)==EN_CHANGE)
          {
            BOOL bEnable = (0 < Edit_GetTextLength(GET_WM_COMMAND_HWND(wParam, lParam)));

            EnableWindow(GetDlgItem(hwnd, IDC_DEBUG), bEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT), bEnable);
          };
          break;

        case IDC_EDIT:
          //
          // Edit script file
          //
          EditScriptFile(hwnd);
          break;

        case IDC_SCRIPT_BROWSE:
          //
          // Browse an existing file
          //
          BrowseScriptFile(hwnd);
          break;

#ifdef SCRPT_HELP_ENABLED
        case IDC_SCRIPT_HELP:
          WinHelp(hwnd, cszHelpFile, HELP_CONTEXT, CREATE_SCRIPT_MAIN);
          break;
#endif // SCRPT_HELP_ENABLED
      };
      break;

    default:
      break;
  }
  return FALSE;
}

//****************************************************************************
// DWORD NEAR PASCAL InitScriptDlg (HWND hwnd)
//
// This function initializes the scripting page.
//
// History:
//  Tue 08-Nov-1994 09:14:13  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL InitScriptDlg (HWND hwnd)
{
  PCONNENTDLG pConnEntDlg;
  WINDOWPLACEMENT wp;
  HKEY  hkey, hkeyEntry;
  DWORD cbSize, dwType;
  UINT  uMode;
  char  szFileName[MAX_PATH];

  // Get the currently selected connection name
  //
  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hwnd, DWL_USER);

  // Assume no assigned script
  //
  szFileName[0] = '\0';
  uMode       = NORMAL_MODE;
  wp.showCmd  = SW_SHOWMINNOACTIVE;

  // Look up the registry for the current script name
  //
  if (RegOpenKey(HKEY_CURRENT_USER, g_szProfile, &hkey) == ERROR_SUCCESS)
  {
    if (RegOpenKey(hkey, pConnEntDlg->pConnEntry->pszEntry, &hkeyEntry)
        == ERROR_SUCCESS)
    {
      cbSize = sizeof(szFileName);
      if (RegQueryValueEx(hkeyEntry, g_szScript, 0, &dwType, szFileName,
                          &cbSize) != ERROR_SUCCESS)
      {
        szFileName[0] = '\0';
      };

      cbSize = sizeof(uMode);
      if (RegQueryValueEx(hkeyEntry, g_szMode, 0, &dwType, (LPBYTE)&uMode,
                          &cbSize) != ERROR_SUCCESS)
      {
        uMode = NORMAL_MODE;
      };

      // Get the current window setting
      //
      cbSize = sizeof(wp);
      if (RegQueryValueEx(hkeyEntry, g_szPlacement, 0, &dwType, (LPBYTE)&wp,
                          &cbSize) != ERROR_SUCCESS)
      {
        // It is not there, signify the default values
        //
        wp.showCmd= SW_SHOWMINNOACTIVE;
      };

      RegCloseKey(hkeyEntry);
    };
    RegCloseKey(hkey);
  };

  // Set the name to the script name box
  //
  Edit_SetText(GetDlgItem(hwnd, IDC_SCRIPT_NAME), szFileName);

  // Set the minimized options
  //
  CheckDlgButton(hwnd, IDC_MINIMIZED,
                 ISMINIMIZED(wp.showCmd) ? BST_CHECKED : BST_UNCHECKED);

  // Set the test mode
  //
  CheckDlgButton(hwnd, IDC_DEBUG,
                 uMode == TEST_MODE ? BST_CHECKED : BST_UNCHECKED);
  EnableWindow(GetDlgItem(hwnd, IDC_DEBUG),
               (szFileName[0] == '\0') ? FALSE : TRUE);

  // Enable/disable edit button
  EnableWindow(GetDlgItem(hwnd, IDC_EDIT), 0 != szFileName[0]);

  // Prepare to browse from the scripts directory
  //
  if (GetWindowsDirectory(szFileName, sizeof(szFileName)))
  {
    LPSTR lpsz = szFileName;

    while((*lpsz != '\\') && (*lpsz != '\0'))
    {
      lpsz = CharNext(lpsz);
    };

    if (*lpsz == '\\')
    {
      // Try to use it as the current directory
      //
      if ((LoadString(ghInstance, IDS_INI_SCRIPT_DIR, lpsz,
                      MAX_PATH - (lpsz - szFileName)) == 0) ||
          (!SetCurrentDirectory(szFileName)))
      {
        // Try the short name
        //
        if (LoadString(ghInstance, IDS_INI_SCRIPT_SHORTDIR, lpsz,
                       MAX_PATH - (lpsz - szFileName)))
        {
          SetCurrentDirectory(szFileName);
        };
      };
    };
  };
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL DeInitScriptDlg (HWND)
//
// This function initializes the advanced device options box.
//
// History:
//  Mon 01-Mar-1993 13:51:30  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL DeInitScriptDlg (HWND hwnd)
{
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL EditScriptFile (HWND)
//
// This function brings up the associated editor for the script.
//
// History:
//  Tue 07-Jun-1995 08:02:00  -by-  Scott Hysom  [scotth]
// Created.
//****************************************************************************

DWORD NEAR PASCAL EditScriptFile(HWND hwnd)
{
  HWND          hCtrl;
  SHELLEXECUTEINFO sei;
  char          szFileName[MAX_PATH];
  DWORD         dwRet;

  hCtrl = GetDlgItem(hwnd, IDC_SCRIPT_NAME);
  Edit_GetText(hCtrl, szFileName, sizeof(szFileName));

  ZeroMemory(&sei, sizeof(sei));
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_DOENVSUBST;
  sei.hwnd = hwnd;
  sei.lpVerb = "Open";
  sei.lpFile = szFileName;
  sei.nShow = SW_NORMAL;
  sei.hInstApp = ghInstance;

  dwRet = ERROR_SUCCESS;      // assume success

  if (!ShellExecuteEx(&sei))
  {
    char szCommand[MAX_PATH];

    // We failed to open the file, default to Notepad

    lstrcpy(szCommand, "notepad");
    sei.lpFile = szCommand;
    sei.lpParameters = szFileName;
    sei.fMask &= ~SEE_MASK_FLAG_NO_UI;    // show any errors this time

    if (!ShellExecuteEx(&sei))
      dwRet = GetLastError();
  };

  return dwRet;
}

//****************************************************************************
// DWORD NEAR PASCAL BrowseScriptFile (HWND)
//
// This function adjusts the dialog layout.
//
// History:
//  Tue 08-Nov-1994 09:14:13  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL BrowseScriptFile(HWND hwnd)
{
  HWND          hCtrl;
  OPENFILENAME  ofn;
  LPSTR         pszFiles, szFileName, szFilter;
  DWORD         dwRet;

  // Allocate filename buffer
  //
  if ((pszFiles = (LPSTR)LocalAlloc(LPTR, 2*MAX_PATH)) == NULL)
    return ERROR_OUTOFMEMORY;
  szFileName = pszFiles;
  szFilter   = szFileName+MAX_PATH;

  // Start file browser dialog
  //
  LoadString(ghInstance, IDS_FILE_FILTER, szFilter, MAX_PATH);

  *szFileName     = '\0';
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner   = hwnd;
  ofn.hInstance   = ghInstance;
  ofn.lpstrFilter = szFilter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter    = 0;
  ofn.nFilterIndex      = 2;
  ofn.lpstrFile         = szFileName;
  ofn.nMaxFile          = MAX_PATH;
  ofn.lpstrFileTitle    = NULL;
  ofn.nMaxFileTitle     = 0;
  ofn.lpstrInitialDir   = NULL;
  ofn.lpstrTitle        = NULL;
  ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nFileOffset       = 0;
  ofn.nFileExtension    = 0;
  ofn.lpstrDefExt       = NULL;
  ofn.lCustData         = 0;
  ofn.lpfnHook          = NULL;
  ofn.lpTemplateName    = NULL;

  if (GetOpenFileName(&ofn))
  {
    // Set the filename to a new name
    //
    hCtrl = GetDlgItem(hwnd, IDC_SCRIPT_NAME);
    Edit_SetText(hCtrl, szFileName);
    Edit_SetSel(hCtrl, 0, -1);
    SetFocus(hCtrl);
    EnableWindow(GetDlgItem(hwnd, IDC_DEBUG),
                 (*szFileName == '\0') ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT), 0 != *szFileName);

    dwRet = ERROR_SUCCESS;
  }
  else
  {
    dwRet = ERROR_OPEN_FAILED;
  };

  LocalFree(pszFiles);
  return dwRet;
}

//****************************************************************************
// DWORD NEAR PASCAL CheckScriptDlgData (HWND)
//
// This function checks the valid data..
//
// History:
//  Tue 08-Nov-1994 09:14:13  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL CheckScriptDlgData(HWND hwnd)
{
  HWND  hCtrl;
  OFSTRUCT of;
  DWORD dwRet = ERROR_SUCCESS;

  // Get the current script name
  //
  hCtrl = GetDlgItem(hwnd, IDC_SCRIPT_NAME);
  Edit_GetText(hCtrl, of.szPathName, sizeof(of.szPathName));

  // Check whether the file exist
  //
  if (of.szPathName[0] != '\0')
  {
    of.cBytes = sizeof(of);
    if (OpenFile(of.szPathName, &of, OF_EXIST) == HFILE_ERROR)
    {
      if (RuiUserMessage(hwnd,
                         IDS_ERR_FILE_NOT_EXIST,
                         MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
      {
        Edit_SetSel(hCtrl, 0, -1);
        SetFocus(hCtrl);
        dwRet = ERROR_FILE_NOT_FOUND;
      };
    };
  };

  return dwRet;
}

//****************************************************************************
// DWORD NEAR PASCAL SaveScriptDlgData (HWND)
//
// This function saves the data permanently.
//
// History:
//  Tue 08-Nov-1994 09:14:13  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD NEAR PASCAL SaveScriptDlgData(HWND hwnd)
{
  PCONNENTDLG pConnEntDlg;
  HWND  hCtrl;
  WINDOWPLACEMENT wp;
  UINT  uMode;
  HKEY  hkey, hkeyEntry;
  DWORD dwRet;
  char  szPathName[MAX_PATH];

  // Get the current script name
  //
  hCtrl = GetDlgItem(hwnd, IDC_SCRIPT_NAME);
  Edit_GetText(hCtrl, szPathName, sizeof(szPathName));

  // Get the connection entry
  //
  pConnEntDlg = (PCONNENTDLG)GetWindowLong(hwnd, DWL_USER);

  // Write the name to the registry key
  //
  if (RegCreateKey(HKEY_CURRENT_USER, g_szProfile, &hkey) == ERROR_SUCCESS)
  {
    if (RegCreateKey(hkey, pConnEntDlg->pConnEntry->pszEntry, &hkeyEntry)
        == ERROR_SUCCESS)
    {
      DWORD cbSize, dwType;

      // If the name exists, update it. Otherwise, remove it.
      //
      if (szPathName[0] != '\0')
      {
        RegSetValueEx(hkeyEntry, g_szScript, 0, REG_SZ, szPathName,
                      lstrlen(szPathName)+1);
      }
      else
      {
        RegDeleteValue(hkeyEntry, g_szScript);
      };

      // Get the current window setting
      //
      cbSize = sizeof(wp);
      if (RegQueryValueEx(hkeyEntry, g_szPlacement, 0, &dwType, (LPBYTE)&wp,
                          &cbSize) != ERROR_SUCCESS)
      {
        // The current setting is not there, use the default value
        //
        ZeroMemory(&wp, sizeof(wp));
        wp.showCmd = SW_SHOWMINNOACTIVE;
      };

      if (IsDlgButtonChecked(hwnd, IDC_MINIMIZED))
      {
        wp.showCmd = SW_SHOWMINNOACTIVE;
      }
      else
      {
        // If the user specifed not-minimized but it is minimized
        //
        if (ISMINIMIZED(wp.showCmd))
        {
          // Set to show normal
          //
          wp.showCmd = SW_SHOWNORMAL;
        };
      };

      RegSetValueEx(hkeyEntry, g_szPlacement, 0, REG_BINARY, (LPBYTE)&wp,
                    sizeof(wp));

      // Set the debug mode
      //
      uMode = (IsWindowEnabled(GetDlgItem(hwnd, IDC_DEBUG)) &&
               IsDlgButtonChecked(hwnd, IDC_DEBUG)) ? TEST_MODE : NORMAL_MODE;
      RegSetValueEx(hkeyEntry, g_szMode, 0, REG_BINARY, (LPBYTE)&uMode,
                    sizeof(uMode));

      RegCloseKey(hkeyEntry);

      dwRet = ERROR_SUCCESS;
    }
    else
    {
      dwRet = ERROR_CANNOT_OPEN_PHONEBOOK;
    };
    RegCloseKey(hkey);
  }
  else
  {
    dwRet = ERROR_CANNOT_OPEN_PHONEBOOK;
  };

  return dwRet;
}
