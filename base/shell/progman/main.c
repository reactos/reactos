/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * PROJECT:         ReactOS Program Manager
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            base/shell/progman/main.c
 * PURPOSE:         ProgMan entry point & MDI window
 * PROGRAMMERS:     Ulrich Schmid
 *                  Sylvain Petreolle
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "progman.h"

#include <shellapi.h>

#define WC_MDICLIENTA        "MDICLIENT"
#define WC_MDICLIENTW       L"MDICLIENT"

#ifdef UNICODE
#define WC_MDICLIENT        WC_MDICLIENTW
#else
#define WC_MDICLIENT        WC_MDICLIENTA
#endif

GLOBALS Globals;

static VOID MAIN_LoadGroups(VOID);
static VOID MAIN_MenuCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
static ATOM MAIN_RegisterMainWinClass(VOID);
static VOID MAIN_CreateMainWindow(VOID);
static VOID MAIN_CreateMDIWindow(VOID);
static VOID MAIN_AutoStart(VOID);


#define BUFFER_SIZE 1024



/*
 * Memory management functions
 */
PVOID
Alloc(IN DWORD  dwFlags,
      IN SIZE_T dwBytes)
{
    return HeapAlloc(GetProcessHeap(), dwFlags, dwBytes);
}

BOOL
Free(IN PVOID lpMem)
{
    return HeapFree(GetProcessHeap(), 0, lpMem);
}

PVOID
ReAlloc(IN DWORD  dwFlags,
        IN PVOID  lpMem,
        IN SIZE_T dwBytes)
{
    return HeapReAlloc(GetProcessHeap(), dwFlags, lpMem, dwBytes);
}

PVOID
AppendToBuffer(IN PVOID   pBuffer,
               IN PSIZE_T pdwBufferSize,
               IN PVOID   pData,
               IN SIZE_T  dwDataSize)
{
    PVOID  pTmp;
    SIZE_T dwBufferSize;

    dwBufferSize = dwDataSize + *pdwBufferSize;

    if (pBuffer)
        pTmp = ReAlloc(0, pBuffer, dwBufferSize);
    else
        pTmp = Alloc(0, dwBufferSize);

    if (!pTmp)
        return NULL;

    memcpy((PVOID)((ULONG_PTR)pTmp + *pdwBufferSize), pData, dwDataSize);
    *pdwBufferSize = dwBufferSize;

    return pTmp;
}



/*
 * Debugging helpers
 */
VOID
PrintStringV(IN LPCWSTR szStr,
             IN va_list args)
{
    WCHAR Buffer[4096];

    _vsnwprintf(Buffer, ARRAYSIZE(Buffer), szStr, args);
    MessageBoxW(Globals.hMainWnd, Buffer, L"Information", MB_OK);
}

VOID
PrintString(IN LPCWSTR szStr, ...)
{
    va_list args;

    va_start(args, szStr);
    PrintStringV(szStr, args);
    va_end(args);
}

VOID
PrintResourceString(IN UINT uID, ...)
{
    WCHAR Buffer[4096];
    va_list args;

    va_start(args, uID);
    LoadStringW(Globals.hInstance, uID, Buffer, ARRAYSIZE(Buffer));
    PrintStringV(Buffer, args);
    va_end(args);
}

VOID
PrintWin32Error(IN LPWSTR Message, IN DWORD ErrorCode)
{
    LPWSTR lpMsgBuf;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&lpMsgBuf, 0, NULL);

    PrintString(L"%s: %s\n", Message, lpMsgBuf);
    LocalFree(lpMsgBuf);
}

int ShowLastWin32Error(VOID)
{
    DWORD dwError;
    LPWSTR lpMsgBuf = NULL;
    WCHAR Buffer[4096];

    dwError = GetLastError();

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&lpMsgBuf, 0, NULL);
    _snwprintf(Buffer, ARRAYSIZE(Buffer), L"Error %d: %s\n", dwError, lpMsgBuf);
    LocalFree(lpMsgBuf);
    return MessageBoxW(Globals.hMainWnd, Buffer, L"Error", MB_OK);
}







/* Copied and adapted from dll/win32/userenv/environment.c!GetUserAndDomainName */
static
BOOL
GetUserAndDomainName(OUT LPWSTR* UserName,
                     OUT LPWSTR* DomainName)
{
    BOOL bRet = TRUE;
    HANDLE hToken;
    DWORD cbTokenBuffer = 0;
    PTOKEN_USER pUserToken;

    LPWSTR lpUserName   = NULL;
    LPWSTR lpDomainName = NULL;
    DWORD  cbUserName   = 0;
    DWORD  cbDomainName = 0;

    SID_NAME_USE SidNameUse;

    /* Get the process token */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    /* Retrieve token's information */
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    pUserToken = Alloc(HEAP_ZERO_MEMORY, cbTokenBuffer);
    if (!pUserToken)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenUser, pUserToken, cbTokenBuffer, &cbTokenBuffer))
    {
        Free(pUserToken);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    /* Retrieve the domain and user name */
    if (!LookupAccountSidW(NULL,
                           pUserToken->User.Sid,
                           NULL,
                           &cbUserName,
                           NULL,
                           &cbDomainName,
                           &SidNameUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            bRet = FALSE;
            goto done;
        }
    }

    lpUserName = Alloc(HEAP_ZERO_MEMORY, cbUserName * sizeof(WCHAR));
    if (lpUserName == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    lpDomainName = Alloc(HEAP_ZERO_MEMORY, cbDomainName * sizeof(WCHAR));
    if (lpDomainName == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    if (!LookupAccountSidW(NULL,
                           pUserToken->User.Sid,
                           lpUserName,
                           &cbUserName,
                           lpDomainName,
                           &cbDomainName,
                           &SidNameUse))
    {
        bRet = FALSE;
        goto done;
    }

    *UserName   = lpUserName;
    *DomainName = lpDomainName;

done:
    if (bRet == FALSE)
    {
        if (lpUserName != NULL)
            Free(lpUserName);

        if (lpDomainName != NULL)
            Free(lpDomainName);
    }

    Free(pUserToken);

    return bRet;
}






static
VOID
MAIN_SetMainWindowTitle(VOID)
{
    LPWSTR caption;
    SIZE_T size;

    LPWSTR lpDomainName = NULL;
    LPWSTR lpUserName   = NULL;

    if (GetUserAndDomainName(&lpUserName, &lpDomainName) && lpUserName && lpDomainName)
    {
        size = (256 + 3 + wcslen(lpDomainName) + wcslen(lpUserName) + 1) * sizeof(WCHAR);
        caption = Alloc(HEAP_ZERO_MEMORY, size);
        if (caption)
        {
            StringCbPrintfW(caption, size, L"%s - %s\\%s", szTitle, lpDomainName, lpUserName);
            SetWindowTextW(Globals.hMainWnd, caption);
            Free(caption);
        }
        else
        {
            SetWindowTextW(Globals.hMainWnd, szTitle);
        }
    }
    else
    {
        SetWindowTextW(Globals.hMainWnd, szTitle);
    }

    if (lpUserName)   Free(lpUserName);
    if (lpDomainName) Free(lpDomainName);
}




static
BOOL
MAIN_LoadSettings(VOID)
{
    LPWSTR lpszTmp;
    LPWSTR lpszSection;
    LONG lRet;
    WCHAR dummy[2];
    LPWSTR lpszKeyValue;
    const LPCWSTR lpszIniFile = L"progman.ini";
    WCHAR szWinDir[MAX_PATH];
    LPWSTR lpszKey;
    DWORD Value;
    HKEY hKey;
    BOOL bIsIniMigrated;
    DWORD dwSize;
    LPWSTR lpszSections;
    LPWSTR lpszData;
    DWORD dwRet;
    DWORD dwType;
    LPWSTR lpszValue;

    bIsIniMigrated = FALSE;
    lpszSections = NULL;
    lpszData = NULL;

    /* Try to create/open the Program Manager user key */
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &Globals.hKeyProgMan,
                        NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    /*
     * TODO: Add the explanation for the migration...
     */
    dwSize = sizeof(Value);
    lRet = RegQueryValueExW(Globals.hKeyProgMan, L"IniMigrated", NULL, &dwType, (LPBYTE)&Value, &dwSize);
    if (lRet != ERROR_SUCCESS || dwType != REG_DWORD)
        Value = 0;
    bIsIniMigrated = !!Value;

    if (bIsIniMigrated)
    {
        /* The migration was already done, just load the settings */
        goto LoadSettings;
    }

    /* Perform the migration */

    bIsIniMigrated = TRUE;
    dwSize = ARRAYSIZE(dummy);
    SetLastError(0);
    GetPrivateProfileSectionW(L"Settings", dummy, dwSize, lpszIniFile);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
        goto MigrationDone;

    SetLastError(0);
    GetPrivateProfileSectionW(L"Groups", dummy, dwSize, lpszIniFile);
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
        goto MigrationDone;

    GetWindowsDirectoryW(szWinDir, ARRAYSIZE(szWinDir));
    // NOTE: GCC complains we cannot use the "\u2022" (UNICODE Code Point) notation for specifying the bullet character,
    // because it's only available in C++ or in C99. On the contrary MSVC is fine with it.
    // Instead we use a hex specification for the character: "\x2022".
    // Note also that the character "\x07" gives also a bullet, but a larger one.
    PrintString(
        L"The Program Manager has detected the presence of a legacy settings file PROGMAN.INI in the directory '%s' "
        L"and is going to migrate its contents into the current-user Program Manager settings registry key:\n"
        L"HKCU\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager"
        L"\n\n"
        L"\x2022 The migration operation will potentially overwrite all the existing current-user Program Manager settings in the registry by those stored in the PROGMAN.INI file.\n"
        L"\n"
        L"\x2022 The migration is done once, so that, at the next launch of the Program Manager, the new migrated settings are directly used.\n"
        L"\n"
        L"\x2022 It is possible to trigger later the migration by manually deleting the registry value \"IniMigrated\" under the current-user Program Manager settings registry key (specified above).\n"
        L"\n"
        L"Would you like to migrate its contents into the registry?",
        szWinDir);

    for (dwSize = BUFFER_SIZE; ; dwSize += BUFFER_SIZE)
    {
        lpszSections = Alloc(0, dwSize * sizeof(WCHAR));
        dwRet = GetPrivateProfileSectionNamesW(lpszSections, dwSize, lpszIniFile);
        if (dwRet < dwSize - 2)
            break;
        Free(lpszSections);
    }
    lpszSection = lpszSections;
    while (*lpszSection)
    {
        lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                                lpszSection,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &hKey,
                                NULL);
        if (lRet == ERROR_SUCCESS)
        {
            for (dwSize = BUFFER_SIZE; ; dwSize += BUFFER_SIZE)
            {
                lpszData = Alloc(0, dwSize * sizeof(WCHAR));
                dwRet = GetPrivateProfileSectionW(lpszSection, lpszData, dwSize, lpszIniFile);
                if (dwRet < dwSize - 2)
                    break;
                Free(lpszData);
            }
            lpszKeyValue = lpszData;
            while (*lpszKeyValue)
            {
                lpszKey = lpszKeyValue;
                lpszValue = wcschr(lpszKeyValue, L'=');
                lpszKeyValue += (wcslen(lpszKeyValue) + 1);
                if (lpszValue)
                {
                    *lpszValue = '\0';
                    ++lpszValue;
                    Value = wcstoul(lpszValue, &lpszTmp, 0);
                    if (lpszTmp - lpszValue >= wcslen(lpszValue))
                    {
                        lpszValue = (LPWSTR)&Value;
                        dwSize = sizeof(Value);
                        dwType = REG_DWORD;
                    }
                    else
                    {
                        dwSize = wcslen(lpszValue) * sizeof(WCHAR);
                        dwType = REG_SZ;
                    }
                }
                else
                {
                    dwSize = 0;
                    dwType = REG_DWORD;
                }
                lRet = RegSetValueExW(hKey, lpszKey, 0, dwType, (LPBYTE)lpszValue, dwSize);
            }
            Free(lpszData);
            RegCloseKey(hKey);
            lpszSection += (wcslen(lpszSection) + 1);
        }
    }
    Free(lpszSections);

MigrationDone:
    Value = TRUE;
    RegSetValueExW(Globals.hKeyProgMan, L"IniMigrated", 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));


LoadSettings:
    /* Create the necessary registry keys for the Program Manager and load its settings from the registry */

    lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                           L"Settings",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyPMSettings,
                           NULL);

    lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                           L"Common Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyPMCommonGroups,
                           NULL);

    lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                           L"Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyPMAnsiGroups,
                           NULL);

    lRet = RegCreateKeyExW(Globals.hKeyProgMan,
                           L"UNICODE Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyPMUnicodeGroups,
                           NULL);

    lRet = RegCreateKeyExW(HKEY_CURRENT_USER,
                           L"Program Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyAnsiGroups,
                           NULL);

    lRet = RegCreateKeyExW(HKEY_CURRENT_USER,
                           L"UNICODE Program Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyUnicodeGroups,
                           NULL);

    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Program Groups",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &Globals.hKeyCommonGroups,
                           NULL);

    dwSize = sizeof(Globals.bAutoArrange);
    RegQueryValueExW(Globals.hKeyPMSettings, L"AutoArrange", NULL, &dwType, (LPBYTE)&Globals.bAutoArrange, &dwSize);

    dwSize = sizeof(Globals.bMinOnRun);
    RegQueryValueExW(Globals.hKeyPMSettings, L"MinOnRun", NULL, &dwType, (LPBYTE)&Globals.bMinOnRun, &dwSize);

    dwSize = sizeof(Globals.bSaveSettings);
    RegQueryValueExW(Globals.hKeyPMSettings, L"SaveSettings", NULL, &dwType, (LPBYTE)&Globals.bSaveSettings, &dwSize);

    return TRUE;
}

static
BOOL
MAIN_SaveSettings(VOID)
{
    WINDOWPLACEMENT WndPl;
    DWORD dwSize;
    WCHAR buffer[100];

    WndPl.length = sizeof(WndPl);
    GetWindowPlacement(Globals.hMainWnd, &WndPl);
    StringCbPrintfW(buffer, sizeof(buffer),
                    L"%d %d %d %d %d",
                    WndPl.rcNormalPosition.left,
                    WndPl.rcNormalPosition.top,
                    WndPl.rcNormalPosition.right,
                    WndPl.rcNormalPosition.bottom,
                    WndPl.showCmd);

    dwSize = wcslen(buffer) * sizeof(WCHAR);
    RegSetValueExW(Globals.hKeyPMSettings, L"Window", 0, REG_SZ, (LPBYTE)buffer, dwSize);

    return TRUE;
}


/***********************************************************************
 *
 *           WinMain
 */

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    MSG msg;
    INITCOMMONCONTROLSEX icex;

    /*
     * Set our shutdown parameters: we want to shutdown the very last,
     * but before any TaskMgr instance (which has a shutdown level of 1).
     */
    SetProcessShutdownParameters(2, 0);

    Globals.hInstance    = hInstance;
    Globals.hGroups      = NULL;
    Globals.hActiveGroup = NULL;

    /* Load Program Manager's settings */
    MAIN_LoadSettings();

    /* Load the default icons */
    Globals.hDefaultIcon       = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_WINLOGO));
    Globals.hMainIcon          = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    Globals.hPersonalGroupIcon = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_GROUP_PERSONAL_ICON));
    Globals.hCommonGroupIcon   = LoadIconW(Globals.hInstance, MAKEINTRESOURCEW(IDI_GROUP_COMMON_ICON));

    /* Initialize the common controls */
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_HOTKEY_CLASS | ICC_LISTVIEW_CLASSES; // | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    /* Register the window classes */
    if (!hPrevInstance) // FIXME: Unused on Win32!
    {
        if (!MAIN_RegisterMainWinClass())   goto Quit;
        if (!GROUP_RegisterGroupWinClass()) goto Quit;
    }

    /* Set up the strings, the main window, the accelerators, the menu, and the MDI child window */
    STRING_LoadStrings();
    MAIN_CreateMainWindow();
    Globals.hAccel = LoadAcceleratorsW(Globals.hInstance, MAKEINTRESOURCEW(IDA_ACCEL));
    STRING_LoadMenus();
    MAIN_CreateMDIWindow();

    /* Load all the groups */
    // MAIN_CreateGroups();
    MAIN_LoadGroups();

    /* Load the Startup group: start the initial applications */
    MAIN_AutoStart();

    /* Message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!TranslateMDISysAccel(Globals.hMDIWnd, &msg) &&
            !TranslateAcceleratorW(Globals.hMainWnd, Globals.hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

Quit:

    /* Save the settings, close the registry keys and quit */

    // MAIN_SaveSettings();
    RegCloseKey(Globals.hKeyCommonGroups);
    RegCloseKey(Globals.hKeyUnicodeGroups);
    RegCloseKey(Globals.hKeyAnsiGroups);
    RegCloseKey(Globals.hKeyPMUnicodeGroups);
    RegCloseKey(Globals.hKeyPMAnsiGroups);
    RegCloseKey(Globals.hKeyPMCommonGroups);
    RegCloseKey(Globals.hKeyPMSettings);
    RegCloseKey(Globals.hKeyProgMan);

    return 0;
}

/***********************************************************************
 *
 *           MAIN_CreateGroups
 */

#if 0
static VOID MAIN_CreateGroups(VOID)
{
  CHAR buffer[BUFFER_SIZE];
  CHAR szPath[MAX_PATHNAME_LEN];
  CHAR key[20], *ptr;

  /* Initialize groups according the `Order' entry of `progman.ini' */
  GetPrivateProfileStringA("Settings", "Order", "", buffer, sizeof(buffer), Globals.lpszIniFile);
  ptr = buffer;
  while (ptr < buffer + sizeof(buffer))
    {
      int num, skip, ret;
      ret = sscanf(ptr, "%d%n", &num, &skip);
      if (ret == 0)
	MAIN_MessageBoxIDS_s(IDS_FILE_READ_ERROR_s, Globals.lpszIniFile, IDS_ERROR, MB_OK);
      if (ret != 1) break;

      sprintf(key, "Group%d", num);
      GetPrivateProfileStringA("Groups", key, "", szPath,
			      sizeof(szPath), Globals.lpszIniFile);
      if (!szPath[0]) continue;

      GRPFILE_ReadGroupFile(szPath);

      ptr += skip;
    }
  /* FIXME initialize other groups, not enumerated by `Order' */
}
#endif

static VOID MAIN_LoadGroups(VOID)
{
}

/***********************************************************************
 *
 *           MAIN_AutoStart
 */

static VOID MAIN_AutoStart(VOID)
{
    LONG lRet;
    DWORD dwSize;
    DWORD dwType;

    PROGGROUP* hGroup;
    PROGRAM* hProgram;

    WCHAR buffer[BUFFER_SIZE];

    dwSize = sizeof(buffer);
    lRet = RegQueryValueExW(Globals.hKeyPMSettings, L"Startup", NULL, &dwType, (LPBYTE)buffer, &dwSize);
    if (lRet != ERROR_SUCCESS || dwType != REG_SZ)
        return;

    for (hGroup = Globals.hGroups; hGroup; hGroup = hGroup->hNext)
    {
        if (_wcsicmp(buffer, hGroup->hName) == 0)
        {
            for (hProgram = hGroup->hPrograms; hProgram; hProgram = hProgram->hNext)
                PROGRAM_ExecuteProgram(hProgram);
        }
    }
}

/***********************************************************************
 *
 *           MAIN_MainWndProc
 */

static LRESULT CALLBACK MAIN_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITMENU:
        {
            PROGGROUP* hActiveGroup = GROUP_ActiveGroup();
            if (hActiveGroup)
            {
                if (PROGRAM_ActiveProgram(hActiveGroup))
                {
                    EnableMenuItem(Globals.hFileMenu, PM_OPEN, MF_ENABLED);
                    EnableMenuItem(Globals.hFileMenu, PM_MOVE, MF_ENABLED);
                    EnableMenuItem(Globals.hFileMenu, PM_COPY, MF_ENABLED);
                    EnableMenuItem(Globals.hFileMenu, PM_DELETE    , MF_ENABLED);
                    EnableMenuItem(Globals.hFileMenu, PM_ATTRIBUTES, MF_ENABLED);
                }
                else
                {
                    if (!hActiveGroup->hWnd || IsIconic(hActiveGroup->hWnd))
                        EnableMenuItem(Globals.hFileMenu, PM_OPEN, MF_ENABLED);
                    else
                        EnableMenuItem(Globals.hFileMenu, PM_OPEN, MF_GRAYED);

                    EnableMenuItem(Globals.hFileMenu, PM_MOVE, MF_GRAYED);
                    EnableMenuItem(Globals.hFileMenu, PM_COPY, MF_GRAYED);
                    EnableMenuItem(Globals.hFileMenu, PM_DELETE    , MF_ENABLED);
                    EnableMenuItem(Globals.hFileMenu, PM_ATTRIBUTES, MF_ENABLED);
                }
            }
            else
            {
                EnableMenuItem(Globals.hFileMenu, PM_OPEN, MF_GRAYED);
                EnableMenuItem(Globals.hFileMenu, PM_MOVE, MF_GRAYED);
                EnableMenuItem(Globals.hFileMenu, PM_COPY, MF_GRAYED);
                EnableMenuItem(Globals.hFileMenu, PM_DELETE    , MF_GRAYED);
                EnableMenuItem(Globals.hFileMenu, PM_ATTRIBUTES, MF_GRAYED);
            }

            CheckMenuItem(Globals.hOptionMenu, PM_AUTO_ARRANGE,
                          MF_BYCOMMAND | (Globals.bAutoArrange  ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(Globals.hOptionMenu, PM_MIN_ON_RUN,
                          MF_BYCOMMAND | (Globals.bMinOnRun     ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(Globals.hOptionMenu, PM_SAVE_SETTINGS,
                          MF_BYCOMMAND | (Globals.bSaveSettings ? MF_CHECKED : MF_UNCHECKED));
            break;
        }

        case WM_DESTROY:
            if (Globals.bSaveSettings)
                MAIN_SaveSettings();
            PostQuitMessage(0);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) < PM_FIRST_CHILD)
                MAIN_MenuCommand(hWnd, LOWORD(wParam), lParam);
            break;
    }

    return DefFrameProcW(hWnd, Globals.hMDIWnd, uMsg, wParam, lParam);
}


/***********************************************************************
 *
 *           MAIN_MenuCommand
 */

static VOID MAIN_MenuCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#if 0
  HLOCAL hActiveGroup    = GROUP_ActiveGroup();
  HLOCAL hActiveProgram  = PROGRAM_ActiveProgram(hActiveGroup);
  HWND   hActiveGroupWnd = GROUP_GroupWnd(hActiveGroup);

  switch(wParam)
    {
      /* Menu File */
    case PM_NEW:
      switch (DIALOG_New((hActiveGroupWnd && !IsIconic(hActiveGroupWnd)) ?
			 PM_NEW_PROGRAM : PM_NEW_GROUP))
      {
      case PM_NEW_PROGRAM:
	if (hActiveGroup) PROGRAM_NewProgram(hActiveGroup);
	break;

      case PM_NEW_GROUP:
	GROUP_NewGroup();
	break;
      }
      break;


    case PM_DELETE:
      if (hActiveProgram)
	{
	if (DIALOG_Delete(IDS_DELETE_PROGRAM_s, PROGRAM_ProgramName(hActiveProgram)))
	  PROGRAM_DeleteProgram(hActiveProgram, TRUE);
	}
      else if (hActiveGroup)
	{
	if (DIALOG_Delete(IDS_DELETE_GROUP_s, GROUP_GroupName(hActiveGroup)))
	  GROUP_DeleteGroup(hActiveGroup);
	}
      break;



    case PM_SAVE_SETTINGS:
      Globals.bSaveSettings = !Globals.bSaveSettings;
      CheckMenuItem(Globals.hOptionMenu, PM_SAVE_SETTINGS,
		    MF_BYCOMMAND | (Globals.bSaveSettings ?
				    MF_CHECKED : MF_UNCHECKED));
      WritePrivateProfileStringA("Settings", "SaveSettings",
				Globals.bSaveSettings ? "1" : "0",
				Globals.lpszIniFile);
      WritePrivateProfileStringA(NULL,NULL,NULL,Globals.lpszIniFile); /* flush it */
      break;


    case PM_ARRANGE:

      if (hActiveGroupWnd && !IsIconic(hActiveGroupWnd))
	ArrangeIconicWindows(hActiveGroupWnd);
      else
	SendMessageW(Globals.hMDIWnd, WM_MDIICONARRANGE, 0, 0);
      break;

    }




#endif

    DWORD Value;

    PROGGROUP* hActiveGroup;
    PROGRAM* hActiveProgram;
    HWND hActiveGroupWnd;

    hActiveGroup = GROUP_ActiveGroup();
    hActiveProgram = PROGRAM_ActiveProgram(hActiveGroup);
    hActiveGroupWnd = (hActiveGroup ? hActiveGroup->hWnd : NULL);

    switch (wParam)
    {
        /* Menu File */

        case PM_NEW:
        {
            BOOL Success;
            INT nResult;

            if (!hActiveGroupWnd || IsIconic(hActiveGroupWnd))
                Success = DIALOG_New(PM_NEW_GROUP, &nResult);
            else
                Success = DIALOG_New(PM_NEW_PROGRAM, &nResult);
            if (!Success)
                break;

            if (nResult & 1)
            {
                GROUPFORMAT format;
                BOOL bIsCommonGroup;

                format = (nResult & 0xC) >> 2;
                bIsCommonGroup = (nResult & 2) != 0;
                GROUP_NewGroup(format, bIsCommonGroup);
            }
            else if (hActiveGroup)
            {
                PROGRAM_NewProgram(hActiveGroup);
            }

            break;
        }

        case PM_OPEN:
            if (hActiveProgram)
                PROGRAM_ExecuteProgram(hActiveProgram);
            else if (hActiveGroupWnd)
                OpenIcon(hActiveGroupWnd);
            break;

        case PM_MOVE:
        case PM_COPY:
            if (hActiveProgram)
                PROGRAM_CopyMoveProgram(hActiveProgram, wParam == PM_MOVE);
            break;

        case PM_DELETE:
        {
            if (hActiveProgram)
            {
                if (DIALOG_Delete(IDS_DELETE_PROGRAM_s, hActiveProgram->hName))
                    PROGRAM_DeleteProgram(hActiveProgram, TRUE);
            }
            else if (hActiveGroup && DIALOG_Delete(IDS_DELETE_GROUP_s, hActiveGroup->hName))
            {
                GROUP_DeleteGroup(hActiveGroup);
            }
            break;
        }

        case PM_ATTRIBUTES:
            if (hActiveProgram)
                PROGRAM_ModifyProgram(hActiveProgram);
            else if (hActiveGroup)
                GROUP_ModifyGroup(hActiveGroup);
            break;

        case PM_EXECUTE:
            DIALOG_Execute();
            break;

        case PM_EXIT:
            // MAIN_SaveSettings();
            PostQuitMessage(0);
            break;


        /* Menu Options */

        case PM_AUTO_ARRANGE:
            Globals.bAutoArrange = !Globals.bAutoArrange;
            CheckMenuItem(Globals.hOptionMenu, PM_AUTO_ARRANGE,
                          MF_BYCOMMAND | (Globals.bAutoArrange ? MF_CHECKED : MF_UNCHECKED));
            Value = Globals.bAutoArrange;
            RegSetValueExW(Globals.hKeyPMSettings, L"AutoArrange", 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
            break;

        case PM_MIN_ON_RUN:
            Globals.bMinOnRun = !Globals.bMinOnRun;
            CheckMenuItem(Globals.hOptionMenu, PM_MIN_ON_RUN,
                          MF_BYCOMMAND | (Globals.bMinOnRun ? MF_CHECKED : MF_UNCHECKED));
            Value = Globals.bMinOnRun;
            RegSetValueExW(Globals.hKeyPMSettings, L"MinOnRun", 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
            break;

        case PM_SAVE_SETTINGS:
            Globals.bSaveSettings = !Globals.bSaveSettings;
            CheckMenuItem(Globals.hOptionMenu, PM_SAVE_SETTINGS,
                          MF_BYCOMMAND | (Globals.bSaveSettings ? MF_CHECKED : MF_UNCHECKED));
            Value = Globals.bSaveSettings;
            RegSetValueExW(Globals.hKeyPMSettings, L"SaveSettings", 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value));
            break;

        case PM_SAVE_SETTINGS_NOW:
            MAIN_SaveSettings();
            break;


        /* Menu Windows */

        case PM_OVERLAP:
            SendMessageW(Globals.hMDIWnd, WM_MDICASCADE, 0, 0);
            break;

        case PM_SIDE_BY_SIDE:
            SendMessageW(Globals.hMDIWnd, WM_MDITILE, MDITILE_VERTICAL, 0);
            break;

        case PM_ARRANGE:
            if (!hActiveGroupWnd || IsIconic(hActiveGroupWnd))
                SendMessageW(Globals.hMDIWnd, WM_MDIICONARRANGE, 0, 0);
            else
                SendMessageA(hActiveGroup->hListView, LVM_ARRANGE, 0, 0);
            break;


        /* Menu Help */

        case PM_CONTENTS:
            if (!WinHelpW(Globals.hMainWnd, L"progman.hlp", HELP_CONTENTS, 0))
                MAIN_MessageBoxIDS(IDS_WINHELP_ERROR, IDS_ERROR, MB_OK);
            break;

        case PM_ABOUT:
            ShellAboutW(hWnd, szTitle, NULL, Globals.hMainIcon);
            break;

        default:
            MAIN_MessageBoxIDS(IDS_NOT_IMPLEMENTED, IDS_ERROR, MB_OK);
            break;
    }

}

/***********************************************************************
 *
 *           MAIN_RegisterMainWinClass
 */

static ATOM MAIN_RegisterMainWinClass(VOID)
{
    WNDCLASSW wndClass;

    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = MAIN_MainWndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = Globals.hInstance;
    wndClass.hIcon         = Globals.hMainIcon;
    wndClass.hCursor       = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = STRING_MAIN_WIN_CLASS_NAME;

    return RegisterClassW(&wndClass);
}

/***********************************************************************
 *
 *           MAIN_CreateMainWindow
 */

static VOID MAIN_CreateMainWindow(VOID)
{
    INT left, top, right, bottom;
    INT width, height;
    INT nCmdShow;
    WCHAR buffer[100];

    LONG lRet;
    DWORD dwSize;
    DWORD dwType;

    Globals.hMDIWnd   = NULL;
    Globals.hMainMenu = NULL;

    /* Get the geometry of the main window */
    dwSize = sizeof(buffer);
    lRet = RegQueryValueExW(Globals.hKeyPMSettings, L"Window", NULL, &dwType, (LPBYTE)buffer, &dwSize);
    if (lRet != ERROR_SUCCESS || dwType != REG_SZ)
        buffer[0] = '\0';

    if (swscanf(buffer, L"%d %d %d %d %d", &left, &top, &right, &bottom, &nCmdShow) == 5)
    {
        width  = right  - left;
        height = bottom - top;
    }
    else
    {
        left = top = width = height = CW_USEDEFAULT;
        nCmdShow = SW_SHOWNORMAL;
    }

    /* Create the main window */
    Globals.hMainWnd =
        CreateWindowW(STRING_MAIN_WIN_CLASS_NAME,
                      szTitle,
                      WS_OVERLAPPEDWINDOW, // /* | WS_CLIPSIBLINGS | WS_CLIPCHILDREN */
                      left, top, width, height,
                      NULL, NULL,
                      Globals.hInstance,
                      NULL);

    MAIN_SetMainWindowTitle();
    ShowWindow(Globals.hMainWnd, nCmdShow);
    UpdateWindow(Globals.hMainWnd);
}

/***********************************************************************
 *
 *           MAIN_CreateMDIWindow
 */

static VOID MAIN_CreateMDIWindow(VOID)
{
    CLIENTCREATESTRUCT ccs;
    RECT rect;

    /* Get the geometry of the MDI window */
    GetClientRect(Globals.hMainWnd, &rect);

    ccs.hWindowMenu  = Globals.hWindowsMenu;
    ccs.idFirstChild = PM_FIRST_CHILD;

    /* Create MDI Window */
    Globals.hMDIWnd =
        CreateWindowW(WC_MDICLIENT, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL, // WS_CHILDWINDOW | ...
                      rect.left, rect.top,
                      rect.right - rect.left, rect.bottom - rect.top,
                      Globals.hMainWnd, 0,
                      Globals.hInstance, &ccs);

    /* Reset the background of the MDI client window (default: COLOR_APPWORKSPACE + 1) */
    SetClassLongPtrW(Globals.hMDIWnd, GCLP_HBRBACKGROUND, (COLOR_WINDOW + 1));

    ShowWindow(Globals.hMDIWnd, SW_SHOW);
    UpdateWindow(Globals.hMDIWnd);
}

/**********************************************************************/
/***********************************************************************
 *
 *           MAIN_MessageBoxIDS
 */
INT MAIN_MessageBoxIDS(UINT ids_text, UINT ids_title, WORD type)
{
    WCHAR text[MAX_STRING_LEN];
    WCHAR title[MAX_STRING_LEN];

    LoadStringW(Globals.hInstance, ids_text , text , ARRAYSIZE(text));
    LoadStringW(Globals.hInstance, ids_title, title, ARRAYSIZE(title));

    return MessageBoxW(Globals.hMainWnd, text, title, type);
}

/***********************************************************************
 *
 *           MAIN_MessageBoxIDS_s
 */
INT MAIN_MessageBoxIDS_s(UINT ids_text, LPCWSTR str, UINT ids_title, WORD type)
{
    WCHAR text[MAX_STRING_LEN];
    WCHAR title[MAX_STRING_LEN];
    WCHAR newtext[MAX_STRING_LEN + MAX_PATHNAME_LEN];

    LoadStringW(Globals.hInstance, ids_text , text , ARRAYSIZE(text));
    LoadStringW(Globals.hInstance, ids_title, title, ARRAYSIZE(title));
    wsprintfW(newtext, text, str);

    return MessageBoxW(Globals.hMainWnd, newtext, title, type);
}

/***********************************************************************
 *
 *           MAIN_ReplaceString
 */

VOID MAIN_ReplaceString(LPWSTR* string, LPWSTR replace)
{
    LPWSTR newstring;

    newstring = Alloc(HEAP_ZERO_MEMORY, (wcslen(replace) + 1) * sizeof(WCHAR));
    if (newstring)
    {
        wcscpy(newstring, replace);
        *string = newstring;
    }
    else
    {
        MAIN_MessageBoxIDS(IDS_OUT_OF_MEMORY, IDS_ERROR, MB_OK);
    }
}
