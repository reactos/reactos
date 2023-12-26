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
#include <sddl.h>
#include <imm.h>
#include <ddk/immdev.h>
#include <cguid.h>
#include <tchar.h>
#include <msctf.h>
#include <ctffunc.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <cicero/cicreg.h>
#include <cicero/cicmutex.h>
#include <cicero/cicfmap.h>

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
BOOL WINAPI
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
 * @unimplemented
 */
VOID InitLangChangeHotKey(VOID)
{
    //FIXME
}

/**
 * @unimplemented
 */
VOID CheckAnchorStores(VOID)
{
    //FIXME
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
    InitUniqueString();

    //FIXME

    gfSharedMemory = TRUE;

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
