/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero base
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "cicbase.h"
#include <shlwapi.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>

void* operator new(size_t size, const CicNoThrow&) noexcept
{
    return cicMemAllocClear(size);
}
void* operator new[](size_t size, const CicNoThrow&) noexcept
{
    return cicMemAllocClear(size);
}
void operator delete(void* ptr) noexcept
{
    cicMemFree(ptr);
}
void operator delete[](void* ptr) noexcept
{
    cicMemFree(ptr);
}
void operator delete(void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}
void operator delete[](void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}

// FIXME
typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessBreakOnTermination = 29
} PROCESSINFOCLASS;

// FIXME
typedef LONG NTSTATUS;

/* ntdll!NtQueryInformationProcess */
typedef NTSTATUS (WINAPI *FN_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

EXTERN_C
BOOL cicIsWow64(VOID)
{
    static FN_NtQueryInformationProcess s_fnNtQueryInformationProcess = NULL;
    ULONG_PTR Value;

    if (!s_fnNtQueryInformationProcess)
    {
        HMODULE hNTDLL = cicGetSystemModuleHandle(TEXT("ntdll.dll"), FALSE);
        if (!hNTDLL)
            return FALSE;

        s_fnNtQueryInformationProcess =
            (FN_NtQueryInformationProcess)GetProcAddress(hNTDLL, "NtQueryInformationProcess");
        if (!s_fnNtQueryInformationProcess)
            return FALSE;
    }

    Value = 0;
    s_fnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information,
                                  &Value, sizeof(Value), NULL);
    return !!Value;
}

EXTERN_C
void cicGetOSInfo(LPUINT puACP, LPDWORD pdwOSInfo)
{
    *pdwOSInfo = 0;

    /* Check OS version info */
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
    GetVersionEx(&VerInfo);
    if (VerInfo.dwPlatformId == DLLVER_PLATFORM_NT)
    {
        *pdwOSInfo |= CIC_OSINFO_NT;
        if (VerInfo.dwMajorVersion >= 5)
        {
            *pdwOSInfo |= CIC_OSINFO_2KPLUS;
            if (VerInfo.dwMinorVersion > 0)
                *pdwOSInfo |= CIC_OSINFO_XPPLUS;
        }
    }
    else
    {
        if (VerInfo.dwMinorVersion >= 10)
            *pdwOSInfo |= CIC_OSINFO_98PLUS;
        else
            *pdwOSInfo |= CIC_OSINFO_95;
    }

    /* Check codepage */
    *puACP = GetACP();
    switch (*puACP)
    {
        case 932: /* Japanese (Japan) */
        case 936: /* Chinese (PRC, Singapore) */
        case 949: /* Korean (Korea) */
        case 950: /* Chinese (Taiwan, Hong Kong) */
            *pdwOSInfo |= CIC_OSINFO_CJK;
            break;
    }

    if (GetSystemMetrics(SM_IMMENABLED))
        *pdwOSInfo |= CIC_OSINFO_IMM;

    if (GetSystemMetrics(SM_DBCSENABLED))
        *pdwOSInfo |= CIC_OSINFO_DBCS;
}

// Get an instance handle that is already loaded
EXTERN_C
HINSTANCE
cicGetSystemModuleHandle(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CicSystemModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return GetModuleHandle(ModPath.m_szPath);
}

// Load a system library
EXTERN_C
HINSTANCE
cicLoadSystemLibrary(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CicSystemModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return ::LoadLibrary(ModPath.m_szPath);
}

BOOL
CicSystemModulePath::Init(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    SIZE_T cchPath;
    if (bSysWinDir)
    {
        // Usually C:\Windows or C:\ReactOS
        cchPath = ::GetSystemWindowsDirectory(m_szPath, _countof(m_szPath));
    }
    else
    {
        // Usually C:\Windows\system32 or C:\ReactOS\system32
        cchPath = ::GetSystemDirectory(m_szPath, _countof(m_szPath));
    }

    m_szPath[_countof(m_szPath) - 1] = TEXT('\0'); // Avoid buffer overrun

    if ((cchPath == 0) || (cchPath > _countof(m_szPath) - 2))
        goto Failure;

    // Add backslash if necessary
    if ((cchPath > 0) && (m_szPath[cchPath - 1] != TEXT('\\')))
    {
        m_szPath[cchPath + 0] = TEXT('\\');
        m_szPath[cchPath + 1] = TEXT('\0');
    }

    // Append pszFileName
    if (FAILED(StringCchCat(m_szPath, _countof(m_szPath), pszFileName)))
        goto Failure;

    m_cchPath = _tcslen(m_szPath);
    return TRUE;

Failure:
    m_szPath[0] = UNICODE_NULL;
    m_cchPath = 0;
    return FALSE;
}

static FN_CoCreateInstance
_cicGetSetUserCoCreateInstance(FN_CoCreateInstance fnUserCoCreateInstance)
{
    static FN_CoCreateInstance s_fn = NULL;
    if (fnUserCoCreateInstance)
        s_fn = fnUserCoCreateInstance;
    return s_fn;
}

EXTERN_C
HRESULT
cicRealCoCreateInstance(
    _In_ REFCLSID rclsid,
    _In_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID iid,
    _Out_ LPVOID *ppv)
{
    static HINSTANCE s_hOle32 = NULL;
    static FN_CoCreateInstance s_fnCoCreateInstance = NULL;
    if (!s_fnCoCreateInstance)
    {
        if (!s_hOle32)
            s_hOle32 = cicLoadSystemLibrary(TEXT("ole32.dll"), FALSE);
        s_fnCoCreateInstance = (FN_CoCreateInstance)GetProcAddress(s_hOle32, "CoCreateInstance");
        if (!s_fnCoCreateInstance)
            return E_NOTIMPL;
    }

    return s_fnCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/**
 * @implemented
 */
HRESULT
cicCoCreateInstance(
    _In_ REFCLSID rclsid,
    _In_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID iid,
    _Out_ LPVOID *ppv)
{
    // NOTE: It looks like Cicero wants to hook CoCreateInstance
    FN_CoCreateInstance fnUserCoCreateInstance = _cicGetSetUserCoCreateInstance(NULL);
    if (fnUserCoCreateInstance)
        return fnUserCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);

    return cicRealCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/**
 * @implemented
 */
EXTERN_C
BOOL
TFInitLib(FN_CoCreateInstance fnCoCreateInstance)
{
    if (fnCoCreateInstance)
        _cicGetSetUserCoCreateInstance(fnCoCreateInstance);
    return TRUE;
}
