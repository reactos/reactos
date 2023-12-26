/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Text Framework Services
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define COBJMACROS
#define INITGUID
#define _EXTYPES_H

#include <windows.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <cguid.h>
#include <tchar.h>
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicero/cicreg.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

DWORD g_dwOSInfo = 0; // See cicGetOSInfo

BOOL StringFromGUID2A(REFGUID rguid, LPSTR pszGUID, INT cchGUID)
{
    pszGUID[0] = ANSI_NULL;

    WCHAR szWide[40];
    szWide[0] = UNICODE_NULL;
    BOOL ret = StringFromGUID2(rguid, szWide, _countof(szWide));
    ::WideCharToMultiByte(CP_ACP, 0, szWide, -1, pszGUID, cchGUID, NULL, NULL);
    return ret;
}

#ifdef UNICODE
    #define StringFromGUID2T StringFromGUID2
    #define debugstr_t debugstr_w
#else
    #define StringFromGUID2T StringFromGUID2A
    #define debugstr_t debugstr_a
#endif

BOOL FullPathExec(LPCTSTR pszExeFile, LPCTSTR pszCmdLine, UINT nCmdShow, BOOL bSysWinDir)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    CicSystemModulePath ModPath;
    TCHAR szCommandLine[MAX_PATH];

    ModPath.Init(pszExeFile, bSysWinDir);
    if (!ModPath.m_cchPath)
    {
        ERR("%s\n", debugstr_t(pszExeFile));
        return FALSE;
    }

    StringCchCopy(szCommandLine, _countof(szCommandLine), pszCmdLine);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = nCmdShow;
    si.dwFlags = STARTF_USESHOWWINDOW;
    if (!CreateProcess(ModPath.m_szPath, szCommandLine, NULL, NULL, FALSE,
                       NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        ERR("%s, %s\n", debugstr_t(ModPath.m_szPath), debugstr_t(szCommandLine));
        return FALSE;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

BOOL RunCPLSetting(LPTSTR pszCmdLine)
{
    TCHAR szExeFile[16];
    StringCchCopy(szExeFile, _countof(szExeFile), TEXT("rundll32.exe"));
    if (!pszCmdLine)
        return FALSE;
    return FullPathExec(szExeFile, pszCmdLine, SW_SHOWMINNOACTIVE, FALSE);
}

/***********************************************************************
 *      TF_RegisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_RegisterLangBarAddIn(
    _In_ REFGUID rguid,
    _In_ LPCWSTR pszFilePath,
    _In_ DWORD dwFlags)
{
    TRACE("(%s, %s, 0x%lX)\n", debugstr_guid(&rguid), debugstr_w(pszFilePath), dwFlags);

    if (!pszFilePath || IsEqualGUID(rguid, GUID_NULL))
    {
        ERR("E_INVALIDARG\n");
        return E_INVALIDARG;
    }

    TCHAR szBuff[MAX_PATH], szGUID[40];
    StringCchCopy(szBuff, _countof(szBuff), TEXT("SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\"));
    StringFromGUID2T(rguid, szGUID, _countof(szGUID));
    StringCchCat(szBuff, _countof(szBuff), szGUID);

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Create(hBaseKey, szBuff);
    if (error == ERROR_SUCCESS)
    {
        error = regKey.SetSzW(L"FilePath", pszFilePath);
        if (error == ERROR_SUCCESS)
            error = regKey.SetDword(TEXT("Enable"), !!(dwFlags & 4));
    }

    return ((error == ERROR_SUCCESS) ? S_OK : E_FAIL);
}

/***********************************************************************
 *      TF_UnregisterLangBarAddIn (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_UnregisterLangBarAddIn(
    _In_ REFGUID rguid,
    _In_ DWORD dwFlags)
{
    TRACE("(%s, 0x%lX)\n", debugstr_guid(&rguid), dwFlags);

    if (IsEqualGUID(rguid, GUID_NULL))
    {
        ERR("E_INVALIDARG\n");
        return E_INVALIDARG;
    }

    TCHAR szSubKey[MAX_PATH];
    StringCchCopy(szSubKey, _countof(szSubKey), TEXT("SOFTWARE\\Microsoft\\CTF\\LangBarAddIn\\"));

    CicRegKey regKey;
    HKEY hBaseKey = ((dwFlags & 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    LSTATUS error = regKey.Open(hBaseKey, szSubKey, KEY_ALL_ACCESS);
    HRESULT hr = E_FAIL;
    if (error == ERROR_SUCCESS)
    {
        TCHAR szGUID[40];
        StringFromGUID2T(rguid, szGUID, _countof(szGUID));
        regKey.RecurseDeleteKey(szGUID);
        hr = S_OK;
    }

    return hr;
}

/***********************************************************************
 *      TF_RunInputCPL (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_RunInputCPL(VOID)
{
    CicSystemModulePath ModPath;
    TCHAR szCmdLine[2 * MAX_PATH];

    TRACE("()\n");

    // NOTE: We don't support Win95/98/Me
    if (g_dwOSInfo & CIC_OSINFO_XPPLUS)
        ModPath.Init(TEXT("input.dll"), FALSE);
    else
        ModPath.Init(TEXT("input.cpl"), FALSE);

    if (!ModPath.m_cchPath)
        return E_FAIL;

    StringCchPrintf(szCmdLine, _countof(szCmdLine),
                    TEXT("rundll32.exe shell32.dll,Control_RunDLL %s"), ModPath.m_szPath);
    if (!RunCPLSetting(szCmdLine))
        return E_FAIL;

    return S_OK;
}
