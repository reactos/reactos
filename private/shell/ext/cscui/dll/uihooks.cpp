//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       uihooks.cpp
//
//--------------------------------------------------------------------------

// uihooks.cpp 
// Hook Function extensions for CSC UI

#include "pch.h"
#include "uihooks.h"

#ifdef CSCUI_HOOKS

// This is the registry key where we put the DLL name
TCHAR const c_szUIHookKey[]     = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
TCHAR const c_szUIHookValue[]   = TEXT("CSCUI Hook DLL");
char  const c_szUIHookProc[]    = "CSCUIHook";

typedef void (WINAPI *PFNCSCHOOK)(DWORD, LPCWSTR);

static HINSTANCE g_hHookDll = NULL;
static PFNCSCHOOK g_pfnCscHook = NULL;
static BOOL g_bHooksInitialized = FALSE;

// Checks the reg key, and sets up function pointers if necessary
void CSCUIExt_InitializeHooks()
{
    HKEY hRegKey;

    g_bHooksInitialized = TRUE;

    // Lets check if the reg key is there
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szUIHookKey, 0, KEY_READ, &hRegKey) == ERROR_SUCCESS)
    {
        TCHAR szHookDll[MAX_PATH];
        DWORD dwSize = sizeof(szHookDll);
        if(RegQueryValueEx(hRegKey, c_szUIHookValue, 0, NULL, (LPBYTE)szHookDll, &dwSize) == ERROR_SUCCESS)
        {
            g_hHookDll = LoadLibrary(szHookDll);
            if(g_hHookDll != NULL)
                g_pfnCscHook = (PFNCSCHOOK)GetProcAddress(g_hHookDll, c_szUIHookProc);
        }
        RegCloseKey(hRegKey);
    }
}

void CSCUIExt_CleanupHooks()
{
    if(g_hHookDll)
    {
        FreeLibrary(g_hHookDll);
        g_hHookDll = NULL;
        g_pfnCscHook = NULL;
    }
}

void CSCUIExt_NotifyHook(DWORD csch, LPCTSTR psz, ...)
{
    USES_CONVERSION;

    if(!g_bHooksInitialized)
        CSCUIExt_InitializeHooks();
    if(g_pfnCscHook)
    {
        LPTSTR pszMsg = NULL;
        va_list va;
        va_start(va, psz);
        vFormatString(&pszMsg, psz, &va);
        (*g_pfnCscHook)(csch, T2CW(pszMsg));
        LocalFreeString(&pszMsg);
        va_end(va);
    }
}

#endif  // CSCUI_HOOKS
