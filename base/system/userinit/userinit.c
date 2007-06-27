/*
 *  ReactOS applications
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Userinit Logon Application
 * FILE:        subsys/system/userinit/userinit.c
 * PROGRAMMERS: Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *              Hervé Poussineau (hpoussin@reactos.org)
 */
#include <windows.h>
#include <cfgmgr32.h>
#include <regstr.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "resource.h"

#define CMP_MAGIC  0x01234567

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

static LONG
ReadRegSzKey(
	IN HKEY hKey,
	IN LPCWSTR pszKey,
	OUT LPWSTR* pValue)
{
	LONG rc;
	DWORD dwType;
	DWORD cbData = 0;
	LPWSTR Value;

	rc = RegQueryValueExW(hKey, pszKey, NULL, &dwType, NULL, &cbData);
	if (rc != ERROR_SUCCESS)
		return rc;
	if (dwType != REG_SZ)
		return ERROR_FILE_NOT_FOUND;
	Value = (WCHAR*) HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
	if (!Value)
		return ERROR_NOT_ENOUGH_MEMORY;
	rc = RegQueryValueExW(hKey, pszKey, NULL, NULL, (LPBYTE)Value, &cbData);
	if (rc != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, Value);
		return rc;
	}
	/* NULL-terminate the string */
	Value[cbData / sizeof(WCHAR)] = '\0';

	*pValue = Value;
	return ERROR_SUCCESS;
}

static
BOOL IsConsoleShell(void)
{
	HKEY ControlKey = NULL;
	LPWSTR SystemStartOptions = NULL;
	LPWSTR CurrentOption, NextOption; /* Pointers into SystemStartOptions */
	LONG rc;
	BOOL ret = FALSE;

	rc = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REGSTR_PATH_CURRENT_CONTROL_SET,
		0,
		KEY_QUERY_VALUE,
		&ControlKey);

	rc = ReadRegSzKey(ControlKey, L"SystemStartOptions", &SystemStartOptions);
	if (rc != ERROR_SUCCESS)
		goto cleanup;

	/* Check for CMDCONS in SystemStartOptions */
	CurrentOption = SystemStartOptions;
	while (CurrentOption)
	{
		NextOption = wcschr(CurrentOption, L' ');
		if (NextOption)
			*NextOption = L'\0';
		if (wcsicmp(CurrentOption, L"CMDCONS") == 0)
		{
			ret = TRUE;
			goto cleanup;
		}
		CurrentOption = NextOption ? NextOption + 1 : NULL;
	}

cleanup:
	if (ControlKey != NULL)
		RegCloseKey(ControlKey);
	HeapFree(GetProcessHeap(), 0, SystemStartOptions);
	return ret;
}

static
BOOL GetShell(WCHAR *CommandLine, HKEY hRootKey)
{
  HKEY hKey;
  DWORD Type, Size;
  WCHAR Shell[MAX_PATH];
  BOOL Ret = FALSE;
  BOOL ConsoleShell = IsConsoleShell();

  if(RegOpenKeyEx(hRootKey,
                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", /* FIXME: should be REGSTR_PATH_WINLOGON */
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    Size = MAX_PATH * sizeof(WCHAR);
    if(RegQueryValueEx(hKey,
                       ConsoleShell ? L"ConsoleShell" : L"Shell",
                       NULL,
                       &Type,
                       (LPBYTE)Shell,
                       &Size) == ERROR_SUCCESS)
    {
      if((Type == REG_SZ) || (Type == REG_EXPAND_SZ))
      {
        wcscpy(CommandLine, Shell);
        Ret = TRUE;
      }
    }
    RegCloseKey(hKey);
  }

  return Ret;
}

static VOID
StartAutoApplications(int clsid)
{
    WCHAR szPath[MAX_PATH] = {0};
    HRESULT hResult;
    HANDLE hFind;
    WIN32_FIND_DATAW findData;
    SHELLEXECUTEINFOW ExecInfo;
    size_t len;

    hResult = SHGetFolderPathW(NULL, clsid, NULL, SHGFP_TYPE_CURRENT, szPath);
    len = wcslen(szPath);
    if (!SUCCEEDED(hResult) || len == 0)
    {
      return;
    }

    wcscat(szPath, L"\\*");
    hFind = FindFirstFileW(szPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
      return;
    }
    szPath[len] = L'\0';

    do
    {
      if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (findData.nFileSizeHigh || findData.nFileSizeLow))
      {
        memset(&ExecInfo, 0x0, sizeof(SHELLEXECUTEINFOW));
        ExecInfo.cbSize = sizeof(ExecInfo);
        ExecInfo.lpVerb = L"open";
        ExecInfo.lpFile = findData.cFileName;
        ExecInfo.lpDirectory = szPath;
        ShellExecuteExW(&ExecInfo);
      }
    }while(FindNextFileW(hFind, &findData));
    FindClose(hFind);
}


static BOOL
TryToStartShell(LPCWSTR Shell)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  WCHAR ExpandedShell[MAX_PATH];

  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  ExpandEnvironmentStrings(Shell, ExpandedShell, MAX_PATH);

  if(!CreateProcess(NULL,
                   ExpandedShell,
                   NULL,
                   NULL,
                   FALSE,
                   NORMAL_PRIORITY_CLASS,
                   NULL,
                   NULL,
                   &si,
                   &pi))
    return FALSE;

  StartAutoApplications(CSIDL_STARTUP);
  StartAutoApplications(CSIDL_COMMON_STARTUP);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return TRUE;
}

static
void StartShell(void)
{
  WCHAR Shell[MAX_PATH];
  TCHAR szMsg[RC_STRING_MAX_SIZE];

  /* Try to run shell in user key */
  if (GetShell(Shell, HKEY_CURRENT_USER) && TryToStartShell(Shell))
    return;

  /* Try to run shell in local machine key */
  if (GetShell(Shell, HKEY_LOCAL_MACHINE) && TryToStartShell(Shell))
    return;

  /* Try default shell */
  if (IsConsoleShell())
    {
      if(GetSystemDirectory(Shell, MAX_PATH - 8))
        wcscat(Shell, L"\\cmd.exe");
      else
        wcscpy(Shell, L"cmd.exe");
    }
    else
    {
      if(GetWindowsDirectory(Shell, MAX_PATH - 13))
        wcscat(Shell, L"\\explorer.exe");
      else
        wcscpy(Shell, L"explorer.exe");
    }
  if (!TryToStartShell(Shell))
  {
    LoadString( GetModuleHandle(NULL), STRING_USERINIT_FAIL, szMsg, sizeof(szMsg) / sizeof(szMsg[0]));
    MessageBox(0, szMsg, NULL, 0);
  }
}

WCHAR g_RegColorNames[][32] =
  {L"Scrollbar",            /* 00 = COLOR_SCROLLBAR */
  L"Background",            /* 01 = COLOR_DESKTOP */
  L"ActiveTitle",           /* 02 = COLOR_ACTIVECAPTION  */
  L"InactiveTitle",         /* 03 = COLOR_INACTIVECAPTION */
  L"Menu",                  /* 04 = COLOR_MENU */
  L"Window",                /* 05 = COLOR_WINDOW */
  L"WindowFrame",           /* 06 = COLOR_WINDOWFRAME */
  L"MenuText",              /* 07 = COLOR_MENUTEXT */
  L"WindowText",            /* 08 = COLOR_WINDOWTEXT */
  L"TitleText",             /* 09 = COLOR_CAPTIONTEXT */
  L"ActiveBorder",          /* 10 = COLOR_ACTIVEBORDER */
  L"InactiveBorder",        /* 11 = COLOR_INACTIVEBORDER */
  L"AppWorkSpace",          /* 12 = COLOR_APPWORKSPACE */
  L"Hilight",               /* 13 = COLOR_HIGHLIGHT */
  L"HilightText",           /* 14 = COLOR_HIGHLIGHTTEXT */
  L"ButtonFace",            /* 15 = COLOR_BTNFACE */
  L"ButtonShadow",          /* 16 = COLOR_BTNSHADOW */
  L"GrayText",              /* 17 = COLOR_GRAYTEXT */
  L"ButtonText",            /* 18 = COLOR_BTNTEXT */
  L"InactiveTitleText",     /* 19 = COLOR_INACTIVECAPTIONTEXT */
  L"ButtonHilight",         /* 20 = COLOR_BTNHIGHLIGHT */
  L"ButtonDkShadow",        /* 21 = COLOR_3DDKSHADOW */
  L"ButtonLight",           /* 22 = COLOR_3DLIGHT */
  L"InfoText",              /* 23 = COLOR_INFOTEXT */
  L"InfoWindow",            /* 24 = COLOR_INFOBK */
  L"ButtonAlternateFace",   /* 25 = COLOR_ALTERNATEBTNFACE */
  L"HotTrackingColor",      /* 26 = COLOR_HOTLIGHT */
  L"GradientActiveTitle",   /* 27 = COLOR_GRADIENTACTIVECAPTION */
  L"GradientInactiveTitle", /* 28 = COLOR_GRADIENTINACTIVECAPTION */
  L"MenuHilight",           /* 29 = COLOR_MENUHILIGHT */
  L"MenuBar"                /* 30 = COLOR_MENUBAR */
};
#define NUM_SYSCOLORS (sizeof(g_RegColorNames) / sizeof(g_RegColorNames[0]))

static
COLORREF StrToColorref(LPWSTR lpszCol)
{
  BYTE rgb[3];

  rgb[0] = StrToIntW(lpszCol);
  lpszCol = StrChrW(lpszCol, L' ') + 1;
  rgb[1] = StrToIntW(lpszCol);
  lpszCol = StrChrW(lpszCol, L' ') + 1;
  rgb[2] = StrToIntW(lpszCol);
  return RGB(rgb[0], rgb[1], rgb[2]);
}

static
void SetUserSysColors(void)
{
  HKEY hKey;
  INT i;
  WCHAR szColor[20];
  DWORD Type, Size;
  COLORREF crColor;

  if(!RegOpenKeyEx(HKEY_CURRENT_USER,
                  L"Control Panel\\Colors",
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    return;
  }
  for(i = 0; i < NUM_SYSCOLORS; i++)
  {
    Size = sizeof(szColor);
    if(RegQueryValueEx(hKey, g_RegColorNames[i], NULL, &Type,
        (LPBYTE)szColor, &Size) == ERROR_SUCCESS && Type == REG_SZ)
    {
      crColor = StrToColorref(szColor);
      SetSysColors(1, &i, &crColor);
    }
  }
  RegCloseKey(hKey);
  return;
}

static
void LoadUserFontSetting(LPWSTR lpValueName, PLOGFONTW pFont)
{
  HKEY hKey;
  LOGFONTW lfTemp;
  DWORD Type, Size;
  INT error;

  Size = sizeof(LOGFONTW);
  if(!RegOpenKeyEx(HKEY_CURRENT_USER,
                  L"Control Panel\\Desktop\\WindowMetrics",
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    return;
  }
  error = RegQueryValueEx(hKey, lpValueName, NULL, &Type, (LPBYTE)&lfTemp, &Size);
  if ((error != ERROR_SUCCESS) || (Type != REG_BINARY))
  {
    return;
  }
  RegCloseKey(hKey);
  /* FIXME: Check if lfTemp is a valid font */
  *pFont = lfTemp;
  return;
}

static
void LoadUserMetricSetting(LPWSTR lpValueName, INT *pValue)
{
  HKEY hKey;
  DWORD Type, Size;
  INT ret;
  WCHAR strValue[8];

  Size = sizeof(strValue);
  if(!RegOpenKeyEx(HKEY_CURRENT_USER,
                  L"Control Panel\\Desktop\\WindowMetrics",
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    return;
  }
  ret = RegQueryValueEx(hKey, lpValueName, NULL, &Type, (LPBYTE)&strValue, &Size);
  if ((ret != ERROR_SUCCESS) || (Type != REG_SZ))
  {
    return;
  }
  RegCloseKey(hKey);
  *pValue = StrToInt(strValue);
  return;
}

static
void SetUserMetrics(void)
{
  NONCLIENTMETRICSW ncmetrics;
  MINIMIZEDMETRICS mmmetrics;

  ncmetrics.cbSize = sizeof(NONCLIENTMETRICSW);
  mmmetrics.cbSize = sizeof(MINIMIZEDMETRICS);
  SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncmetrics, 0);
  SystemParametersInfoW(SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &mmmetrics, 0);

  LoadUserFontSetting(L"CaptionFont", &ncmetrics.lfCaptionFont);
  LoadUserFontSetting(L"SmCaptionFont", &ncmetrics.lfSmCaptionFont);
  LoadUserFontSetting(L"MenuFont", &ncmetrics.lfMenuFont);
  LoadUserFontSetting(L"StatusFont", &ncmetrics.lfStatusFont);
  LoadUserFontSetting(L"MessageFont", &ncmetrics.lfMessageFont);
  /* FIXME: load icon font ? */

  LoadUserMetricSetting(L"BorderWidth", &ncmetrics.iBorderWidth);
  LoadUserMetricSetting(L"ScrollWidth", &ncmetrics.iScrollWidth);
  LoadUserMetricSetting(L"ScrollHeight", &ncmetrics.iScrollHeight);
  LoadUserMetricSetting(L"CaptionWidth", &ncmetrics.iCaptionWidth);
  LoadUserMetricSetting(L"CaptionHeight", &ncmetrics.iCaptionHeight);
  LoadUserMetricSetting(L"SmCaptionWidth", &ncmetrics.iSmCaptionWidth);
  LoadUserMetricSetting(L"SmCaptionHeight", &ncmetrics.iSmCaptionHeight);
  LoadUserMetricSetting(L"Menuwidth", &ncmetrics.iMenuWidth);
  LoadUserMetricSetting(L"MenuHeight", &ncmetrics.iMenuHeight);

  SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncmetrics, 0);

  return;
}

static
void SetUserWallpaper(void)
{
  HKEY hKey;
  DWORD Type, Size;
  WCHAR szWallpaper[MAX_PATH + 1];

  if(RegOpenKeyEx(HKEY_CURRENT_USER,
                  REGSTR_PATH_DESKTOP,
                  0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    Size = sizeof(szWallpaper);
    if(RegQueryValueEx(hKey,
                       L"Wallpaper",
                       NULL,
                       &Type,
                       (LPBYTE)szWallpaper,
                       &Size) == ERROR_SUCCESS
       && Type == REG_SZ)
    {
      /* Load and change the wallpaper */
      SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, szWallpaper, SPIF_SENDCHANGE);
    }
    else
    {
      /* remove the wallpaper */
      SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDCHANGE);
    }
    RegCloseKey(hKey);
  }
}

static
void SetUserSettings(void)
{
  SetUserSysColors();
  SetUserMetrics();
  SetUserWallpaper();
}

typedef DWORD (WINAPI *PCMP_REPORT_LOGON)(DWORD, DWORD);

static VOID
NotifyLogon(VOID)
{
    HINSTANCE hModule;
    PCMP_REPORT_LOGON CMP_Report_LogOn;

    hModule = LoadLibrary(L"setupapi.dll");
    if (hModule)
    {
        CMP_Report_LogOn = (PCMP_REPORT_LOGON)GetProcAddress(hModule, "CMP_Report_LogOn");
        if (CMP_Report_LogOn)
            CMP_Report_LogOn(CMP_MAGIC, GetCurrentProcessId());

        FreeLibrary(hModule);
    }
}



#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif /* _MSC_VER */

int WINAPI
WinMain(HINSTANCE hInst,
        HINSTANCE hPrevInstance,
        LPSTR lpszCmdLine,
        int nCmdShow)
{
  NotifyLogon();
  SetUserSettings();
  StartShell();
  return 0;
}

/* EOF */
