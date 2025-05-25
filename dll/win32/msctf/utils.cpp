/*
 * PROJECT:     ReactOS msctf.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Text Framework Services
 * COPYRIGHT:   Copyright 2023-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define COBJMACROS
#define INITGUID
#define _EXTYPES_H

#include <windows.h>
#include <sddl.h>
#include <imm.h>
#include <cguid.h>
#include <tchar.h>
#include <msctf.h>
#include <msctf_undoc.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicarray.h>
#include <cicreg.h>
#include <cicmutex.h>
#include <cicfmap.h>

#include "mlng.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

BOOL gf_CRT_INIT = FALSE;
BOOL g_fDllProcessDetached = FALSE;
CRITICAL_SECTION g_cs;
CRITICAL_SECTION g_csInDllMain;
CRITICAL_SECTION g_csDelayLoad;
HINSTANCE g_hInst = NULL;
BOOL g_bOnWow64 = FALSE;
UINT g_uACP = CP_ACP; // Active Code Page
DWORD g_dwOSInfo = 0; // See cicGetOSInfo
HKL g_hklDefault = NULL;
DWORD g_dwTLSIndex = (DWORD)-1;
BOOL gfSharedMemory = FALSE;
LONG g_cRefDll = -1;
BOOL g_fCUAS = FALSE;
TCHAR g_szCUASImeFile[16] = { 0 };
DWORD g_dwAppCompatibility = 0;
BOOL g_fNoITextStoreAnchor = TRUE;

// Messages
UINT g_msgPrivate = 0;
UINT g_msgSetFocus = 0;
UINT g_msgThreadTerminate = 0;
UINT g_msgThreadItemChange = 0;
UINT g_msgLBarModal = 0;
UINT g_msgRpcSendReceive = 0;
UINT g_msgThreadMarshal = 0;
UINT g_msgCheckThreadInputIdel = 0;
UINT g_msgStubCleanUp = 0;
UINT g_msgShowFloating = 0;
UINT g_msgLBUpdate = 0;
UINT g_msgNuiMgrDirtyUpdate = 0;

// Unique names
BOOL g_fUserSidString = FALSE;
TCHAR g_szUserSidString[MAX_PATH] = { 0 };
TCHAR g_szUserUnique[MAX_PATH] = { 0 };
TCHAR g_szAsmListCache[MAX_PATH] = { 0 };
TCHAR g_szTimListCache[MAX_PATH] = { 0 };
TCHAR g_szLayoutsCache[MAX_PATH] = { 0 };

// Mutexes
CicMutex g_mutexLBES;
CicMutex g_mutexCompart;
CicMutex g_mutexAsm;
CicMutex g_mutexLayouts;
CicMutex g_mutexTMD;

// File mapping
CicFileMappingStatic g_SharedMemory;

// Hot-Keys
UINT g_uLangHotKeyModifiers = 0;
UINT g_uLangHotKeyVKey = 0;
UINT g_uLangHotKeyVKey2 = 0;
UINT g_uKeyTipHotKeyModifiers = 0;
UINT g_uKeyTipHotKeyVKey = 0;
UINT g_uKeyTipHotKeyVKey2 = 0;

/**
 * @implemented
 */
LPTSTR GetUserSIDString(void)
{
    HANDLE hToken = NULL;
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    if (!hToken)
        return NULL;

    DWORD dwLengthNeeded = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLengthNeeded);
    PTOKEN_USER pTokenUser = (PTOKEN_USER)cicMemAllocClear(dwLengthNeeded);
    if (!pTokenUser)
    {
        CloseHandle(hToken);
        return NULL;
    }

    LPTSTR StringSid = NULL;
    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwLengthNeeded, &dwLengthNeeded) ||
        !ConvertSidToStringSid(pTokenUser->User.Sid, &StringSid))
    {
        if (StringSid)
        {
            LocalFree(StringSid);
            StringSid = NULL;
        }
    }

    cicMemFree(pTokenUser);
    CloseHandle(hToken);
    return StringSid;
}

/**
 * @implemented
 */
BOOL InitUserSidString(void)
{
    if (g_fUserSidString)
        return TRUE;

    LPTSTR pszUserSID = GetUserSIDString();
    if (!pszUserSID)
        return FALSE;

    StringCchCopy(g_szUserSidString, _countof(g_szUserSidString), pszUserSID);
    g_fUserSidString = TRUE;
    LocalFree(pszUserSID);
    return TRUE;
}

/**
 * @implemented
 */
BOOL InitUniqueString(void)
{
    g_szUserUnique[0] = TEXT('\0');

    DWORD dwThreadId = GetCurrentThreadId();
    HDESK hDesk = GetThreadDesktop(dwThreadId);

    DWORD nLengthNeeded;
    TCHAR szName[MAX_PATH];
    if (hDesk && GetUserObjectInformation(hDesk, UOI_NAME, szName, _countof(szName), &nLengthNeeded))
        StringCchCat(g_szUserUnique, _countof(g_szUserUnique), szName);

    if (!InitUserSidString())
        return FALSE;

    StringCchCat(g_szUserUnique, _countof(g_szUserUnique), g_szUserSidString);
    return TRUE;
}

void
GetDesktopUniqueName(
    _In_ LPCTSTR pszName,
    _Out_ LPTSTR pszBuff,
    _In_ UINT cchBuff)
{
    StringCchCopy(pszBuff, cchBuff, pszName);
    StringCchCat(pszBuff, cchBuff, g_szUserUnique);
}

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
    TCHAR szCommandLine[2 * MAX_PATH];

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

static inline BOOL
RunCPLSetting(LPCTSTR pszCmdLine)
{
    if (!pszCmdLine)
        return FALSE;

    return FullPathExec(TEXT("rundll32.exe"), pszCmdLine, SW_SHOWMINNOACTIVE, FALSE);
}

/***********************************************************************
 *      TF_GetThreadFlags (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_GetThreadFlags(
    _In_ DWORD dwThreadId,
    _Out_ LPDWORD pdwFlags1,
    _Out_ LPDWORD pdwFlags2,
    _Out_ LPDWORD pdwFlags3)
{
    FIXME("(%lu, %p, %p, %p)\n", dwThreadId, pdwFlags1, pdwFlags2, pdwFlags3);
    *pdwFlags1 = *pdwFlags2 = *pdwFlags3 = 0;
    return FALSE;
}

/***********************************************************************
 *      TF_CreateCategoryMgr (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_CreateCategoryMgr(_Out_ ITfCategoryMgr **ppcat)
{
    FIXME("(%p)\n", ppcat);
    if (ppcat)
        *ppcat = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_CreateCicLoadMutex (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HANDLE WINAPI
TF_CreateCicLoadMutex(_Out_ LPBOOL pfWinLogon)
{
    FIXME("(%p)\n", pfWinLogon);
    if (pfWinLogon == NULL)
        return NULL;
    *pfWinLogon = FALSE;
    return NULL;
}

/***********************************************************************
 *      TF_CreateDisplayAttributeMgr (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_CreateDisplayAttributeMgr(_Out_ ITfDisplayAttributeMgr **ppdam)
{
    FIXME("(%p)\n", ppdam);
    *ppdam = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_DllDetachInOther (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_DllDetachInOther(VOID)
{
    FIXME("()\n");
    return TRUE;
}

/***********************************************************************
 *      TF_GetGlobalCompartment (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_GetGlobalCompartment(_Out_ ITfCompartmentMgr **ppCompMgr)
{
    FIXME("(%p)\n", ppCompMgr);
    *ppCompMgr = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_GetLangIcon (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HICON WINAPI
TF_GetLangIcon(_In_ LANGID LangID, _Out_ PWSTR pszText, _In_ INT cchText)
{
    FIXME("(0x%X, %p, %d)\n", LangID, pszText, cchText);
    return NULL;
}

/***********************************************************************
 *      TF_IsFullScreenWindowAcitvated (MSCTF.@)
 *
 * Yes, this function name is misspelled by MS.
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_IsFullScreenWindowAcitvated(VOID)
{
    FIXME("()\n");
    return FALSE;
}

/***********************************************************************
 *      TF_GetInputScope (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_GetInputScope(_In_opt_ HWND hWnd, _Out_ ITfInputScope **ppInputScope)
{
    FIXME("(%p, %p)\n", hWnd, ppInputScope);
    *ppInputScope = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *      SetInputScopeXML (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
SetInputScopeXML(_In_opt_ HWND hwnd, _In_opt_ PCWSTR pszXML)
{
    FIXME("(%p, %p)\n", hwnd, pszXML);
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_CUASAppFix (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
TF_CUASAppFix(_In_ LPCSTR pszName)
{
    if (!pszName || lstrcmpiA(pszName, "DelayFirstActivateKeyboardLayout") != 0)
        return E_INVALIDARG;
    g_dwAppCompatibility |= CTF_COMPAT_DELAY_FIRST_ACTIVATE;
    return S_OK;
}

/***********************************************************************
 *      TF_CheckThreadInputIdle (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C LONG WINAPI
TF_CheckThreadInputIdle(_In_ DWORD dwThreadId, _In_ DWORD dwMilliseconds)
{
    FIXME("(%lu, %lu)\n", dwThreadId, dwMilliseconds);
    return -1;
}

/***********************************************************************
 *      TF_ClearLangBarAddIns (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_ClearLangBarAddIns(_In_ REFGUID guid)
{
    FIXME("(%p)\n", guid);
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_InvalidAssemblyListCache (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_InvalidAssemblyListCache(VOID)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_IsInMarshaling (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_IsInMarshaling(_In_ DWORD dwThreadId)
{
    FIXME("(%lu)\n", dwThreadId);
    return FALSE;
}

/***********************************************************************
 *      TF_PostAllThreadMsg (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
TF_PostAllThreadMsg(_In_opt_ WPARAM wParam, _In_ DWORD dwFlags)
{
    FIXME("(%p, 0x%X)\n", wParam, dwFlags);
    return E_NOTIMPL;
}

/***********************************************************************
 *      TF_InitSystem (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_InitSystem(VOID)
{
    FIXME("()\n");
    return FALSE;
}

/***********************************************************************
 *      TF_UninitSystem (MSCTF.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
TF_UninitSystem(VOID)
{
    FIXME("()\n");
    return FALSE;
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

/***********************************************************************
 *      TF_IsCtfmonRunning (MSCTF.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
TF_IsCtfmonRunning(VOID)
{
    TCHAR szName[MAX_PATH];
    GetDesktopUniqueName(TEXT("CtfmonInstMutex"), szName, _countof(szName));

    HANDLE hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, szName);
    if (!hMutex)
        return FALSE;

    ::CloseHandle(hMutex);
    return TRUE;
}

/**
 * @implemented
 */
BOOL InitLangChangeHotKey(VOID)
{
    CicRegKey regKey;
    TCHAR szLanguage[2], szLayout[2];
    LSTATUS error;

    szLanguage[0] = szLayout[0] = TEXT('3');
    szLanguage[1] = szLayout[1] = TEXT('\0');

    error = regKey.Open(HKEY_CURRENT_USER, TEXT("Keyboard Layout\\Toggle"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey.QuerySz(TEXT("Language Hotkey"), szLanguage, _countof(szLanguage));
        if (error != ERROR_SUCCESS)
        {
            if (g_dwOSInfo & CIC_OSINFO_NT)
            {
                error = regKey.QuerySz(TEXT("Hotkey"), szLanguage, _countof(szLanguage));
                if (error != ERROR_SUCCESS)
                    szLanguage[0] = TEXT('1');
            }
            else
            {
                error = regKey.QuerySz(NULL, szLanguage, _countof(szLanguage));
                if (error != ERROR_SUCCESS)
                    szLanguage[0] = TEXT('1');
            }

            if (PRIMARYLANGID(GetSystemDefaultLCID()) == LANG_CHINESE)
                szLanguage[0] = TEXT('1');
        }

        error = regKey.QuerySz(TEXT("Layout Hotkey"), szLayout, _countof(szLayout));
        if (error != ERROR_SUCCESS)
        {
            szLayout[0] = TEXT('1');
            if (szLanguage[0] != TEXT('2'))
                szLayout[0] = TEXT('2');
            if (GetSystemMetrics(SM_MIDEASTENABLED))
                szLayout[0] = TEXT('3');
        }

        szLanguage[1] = TEXT('\0');
        szLayout[1] = TEXT('\0');
    }

    if (szLanguage[0] == szLayout[0])
    {
        if (szLanguage[0] == TEXT('1'))
            szLayout[0] = TEXT('2');
        else if (szLanguage[0] == TEXT('2'))
            szLayout[0] = TEXT('1');
        else
            szLayout[0] = TEXT('3');
    }

    ::EnterCriticalSection(&g_csInDllMain);

    switch (szLanguage[0])
    {
        case TEXT('2'):
            g_uLangHotKeyModifiers = MOD_SHIFT | MOD_CONTROL;
            g_uLangHotKeyVKey2 = VK_CONTROL;
            g_uLangHotKeyVKey = VK_SHIFT;
            break;

        case TEXT('3'):
            g_uLangHotKeyVKey = 0;
            g_uLangHotKeyModifiers = 0;
            g_uLangHotKeyVKey2 = 0;
            break;

        case TEXT('4'):
            g_uLangHotKeyVKey = VK_NUMPAD0;
            g_uLangHotKeyModifiers = 0;
            g_uLangHotKeyVKey2 = 0;
            break;

        case TEXT('1'):
        default:
            g_uLangHotKeyModifiers = MOD_SHIFT | MOD_ALT;
            g_uLangHotKeyVKey2 = VK_MENU;
            g_uLangHotKeyVKey = VK_SHIFT;
            break;
    }

    switch (szLayout[0])
    {
        case TEXT('2'):
            g_uKeyTipHotKeyModifiers = MOD_SHIFT | MOD_CONTROL;
            g_uKeyTipHotKeyVKey = VK_SHIFT;
            g_uKeyTipHotKeyVKey2 = VK_CONTROL;
            break;

        case TEXT('3'):
            g_uKeyTipHotKeyModifiers = 0;
            g_uKeyTipHotKeyVKey = 0;
            g_uKeyTipHotKeyVKey2 = 0;
            break;

        case TEXT('4'):
            g_uKeyTipHotKeyModifiers = 0;
            g_uKeyTipHotKeyVKey = VK_OEM_3;
            g_uKeyTipHotKeyVKey2 = 0;
            break;

        case TEXT('1'):
        default:
            g_uKeyTipHotKeyModifiers = 0x40 | MOD_SHIFT;
            g_uKeyTipHotKeyVKey = VK_SHIFT;
            g_uKeyTipHotKeyVKey2 = VK_MENU;
            break;
    }

    ::LeaveCriticalSection(&g_csInDllMain);

    TRACE("HotKey: %c, %c\n", szLanguage[0], szLayout[0]);
    return TRUE;
}

/**
 * @implemented
 */
VOID CheckAnchorStores(VOID)
{
    HKEY hKey;
    LSTATUS error;
    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\CTF"), 0, KEY_READ, &hKey);
    if (error != ERROR_SUCCESS)
        return;

    DWORD dwData = 0, cbData = sizeof(dwData);
    error = RegQueryValueEx(hKey, TEXT("EnableAnchorContext"), NULL, NULL, (PBYTE)&dwData, &cbData);
    if (error == ERROR_SUCCESS && cbData == sizeof(DWORD) && dwData == 1)
        g_fNoITextStoreAnchor = FALSE;

    RegCloseKey(hKey);
}

VOID InitCUASFlag(VOID)
{
    CicRegKey regKey1;
    LSTATUS error;

    error = regKey1.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\CTF\\SystemShared\\"));
    if (error == ERROR_SUCCESS)
    {
        DWORD dwValue;
        error = regKey1.QueryDword(TEXT("CUAS"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_fCUAS = !!dwValue;
    }

    g_szCUASImeFile[0] = TEXT('\0');

    if (!g_fCUAS)
        return;

    TCHAR szImeFile[16];
    CicRegKey regKey2;
    error = regKey2.Open(HKEY_LOCAL_MACHINE,
                         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey2.QuerySz(TEXT("IME File"), szImeFile, _countof(szImeFile));
        if (error == ERROR_SUCCESS)
        {
            g_szCUASImeFile[_countof(g_szCUASImeFile) - 1] = TEXT('\0'); // Avoid buffer overrun
            StringCchCopy(g_szCUASImeFile, _countof(g_szCUASImeFile), szImeFile);
        }
    }
}

EXTERN_C VOID TFUninitLib(VOID)
{
    // Do nothing
}

/**
 * @unimplemented
 */
BOOL ProcessAttach(HINSTANCE hinstDLL) // FIXME: Call me from DllMain
{
    gf_CRT_INIT = TRUE;

    ::InitializeCriticalSectionAndSpinCount(&g_cs, 0);
    ::InitializeCriticalSectionAndSpinCount(&g_csInDllMain, 0);
    ::InitializeCriticalSectionAndSpinCount(&g_csDelayLoad, 0);

    g_bOnWow64 = cicIsWow64();
    g_hInst = hinstDLL;
    g_hklDefault = ::GetKeyboardLayout(0);
    g_dwTLSIndex = ::TlsAlloc();
    if (g_dwTLSIndex == (DWORD)-1)
        return FALSE;

    g_msgPrivate = ::RegisterWindowMessageA("MSUIM.Msg.Private");
    g_msgSetFocus = ::RegisterWindowMessageA("MSUIM.Msg.SetFocus");
    g_msgThreadTerminate = ::RegisterWindowMessageA("MSUIM.Msg.ThreadTerminate");
    g_msgThreadItemChange = ::RegisterWindowMessageA("MSUIM.Msg.ThreadItemChange");
    g_msgLBarModal = ::RegisterWindowMessageA("MSUIM.Msg.LangBarModal");
    g_msgRpcSendReceive = ::RegisterWindowMessageA("MSUIM.Msg.RpcSendReceive");
    g_msgThreadMarshal = ::RegisterWindowMessageA("MSUIM.Msg.ThreadMarshal");
    g_msgCheckThreadInputIdel = ::RegisterWindowMessageA("MSUIM.Msg.CheckThreadInputIdel");
    g_msgStubCleanUp = ::RegisterWindowMessageA("MSUIM.Msg.StubCleanUp");
    g_msgShowFloating = ::RegisterWindowMessageA("MSUIM.Msg.ShowFloating");
    g_msgLBUpdate = ::RegisterWindowMessageA("MSUIM.Msg.LBUpdate");
    g_msgNuiMgrDirtyUpdate = ::RegisterWindowMessageA("MSUIM.Msg.MuiMgrDirtyUpdate");
    if (!g_msgPrivate ||
        !g_msgSetFocus ||
        !g_msgThreadTerminate ||
        !g_msgThreadItemChange ||
        !g_msgLBarModal ||
        !g_msgRpcSendReceive ||
        !g_msgThreadMarshal ||
        !g_msgCheckThreadInputIdel ||
        !g_msgStubCleanUp ||
        !g_msgShowFloating ||
        !g_msgLBUpdate ||
        !g_msgNuiMgrDirtyUpdate)
    {
        return FALSE;
    }

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);
    TRACE("cicGetOSInfo: %u, 0x%lX\n", g_uACP, g_dwOSInfo);

    InitUniqueString();

    //FIXME

    gfSharedMemory = TRUE;

    //FIXME

    InitCUASFlag();

    //FIXME

    GetDesktopUniqueName(TEXT("CTF.AsmListCache.FMP"), g_szAsmListCache, _countof(g_szAsmListCache));
    GetDesktopUniqueName(TEXT("CTF.TimListCache.FMP"), g_szTimListCache, _countof(g_szTimListCache));
    GetDesktopUniqueName(TEXT("CTF.LayoutsCache.FMP"), g_szLayoutsCache, _countof(g_szLayoutsCache));

    //FIXME

    InitLangChangeHotKey();

    //FIXME

    CheckAnchorStores();

    return TRUE;
}

/**
 * @unimplemented
 */
VOID ProcessDetach(HINSTANCE hinstDLL) // FIXME: Call me from DllMain
{
    if (!gf_CRT_INIT)
    {
        g_fDllProcessDetached = TRUE;
        return;
    }

    if (gfSharedMemory)
    {
        if (g_cRefDll != -1 )
            TFUninitLib();
        //FIXME
    }

    UninitINAT();

    //FIXME

    //TF_UninitThreadSystem();

    //FIXME

    if (g_dwTLSIndex != (DWORD)-1)
    {
        ::TlsFree(g_dwTLSIndex);
        g_dwTLSIndex = (DWORD)-1;
    }

    //FIXME

    if (gfSharedMemory)
    {
        g_mutexLBES.Uninit();
        g_mutexCompart.Uninit();
        g_mutexAsm.Uninit();
        //FIXME
        g_mutexLayouts.Uninit();
        g_mutexTMD.Uninit();
        //FIXME
        g_SharedMemory.Close();
    }

    g_SharedMemory.Finalize();

    ::DeleteCriticalSection(&g_cs);
    ::DeleteCriticalSection(&g_csInDllMain);
    ::DeleteCriticalSection(&g_csDelayLoad);

    g_fDllProcessDetached = TRUE;
}
