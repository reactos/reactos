/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero base
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifndef __cplusplus
    #error Cicero needs C++.
#endif

#include "cicpath.h"

static inline LPVOID cicMemAlloc(SIZE_T size)
{
    return LocalAlloc(0, size);
}

static inline LPVOID cicMemAllocClear(SIZE_T size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}

static inline LPVOID cicMemReAlloc(LPVOID ptr, SIZE_T newSize)
{
    if (!ptr)
        return LocalAlloc(LMEM_ZEROINIT, newSize);
    return LocalReAlloc(ptr, newSize, LMEM_ZEROINIT);
}

static inline void cicMemFree(LPVOID ptr)
{
    if (ptr)
        LocalFree(ptr);
}

inline void* __cdecl operator new(size_t size) noexcept
{
    return cicMemAllocClear(size);
}

inline void __cdecl operator delete(void* ptr) noexcept
{
    cicMemFree(ptr);
}

inline void __cdecl operator delete(void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}

// FIXME: Use msutb.dll and header
static inline void ClosePopupTipbar(void)
{
}

// FIXME: Use msutb.dll and header
static inline void GetPopupTipbar(HWND hwnd, BOOL fWinLogon)
{
}

/* The flags of cicGetOSInfo() */
#define OSINFO_NT    0x01
#define OSINFO_CJK   0x10
#define OSINFO_IMM   0x20
#define OSINFO_DBCS  0x40

static inline DWORD
cicGetOSInfo(VOID)
{
    DWORD dwOsInfo = 0;

    /* Check OS version info */
    OSVERSIONINFOW VerInfo = { sizeof(VerInfo) };
    GetVersionExW(&VerInfo);
    if (VerInfo.dwPlatformId == DLLVER_PLATFORM_NT)
        dwOsInfo |= OSINFO_NT;

    /* Check codepage */
    switch (GetACP())
    {
        case 932: /* Japanese (Japan) */
        case 936: /* Chinese (PRC, Singapore) */
        case 949: /* Korean (Korea) */
        case 950: /* Chinese (Taiwan, Hong Kong) */
            dwOsInfo |= OSINFO_CJK;
            break;
    }

    if (GetSystemMetrics(SM_IMMENABLED))
        dwOsInfo |= OSINFO_IMM;

    if (GetSystemMetrics(SM_DBCSENABLED))
        dwOsInfo |= OSINFO_DBCS;

    /* I'm not interested in other flags */

    return dwOsInfo;
}

struct CicSystemModulePath
{
    WCHAR m_szPath[MAX_PATH];
    SIZE_T m_cchPath;

    CicSystemModulePath()
    {
        m_szPath[0] = UNICODE_NULL;
        m_cchPath = 0;
    }

    BOOL Init(_In_ LPCWSTR pszFileName, _In_ BOOL bSysWinDir);
};

// Get an instance handle that is already loaded
static inline HINSTANCE
GetSystemModuleHandle(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CicSystemModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return GetModuleHandleW(ModPath.m_szPath);
}

// Load a system library
static inline HINSTANCE
LoadSystemLibrary(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CicSystemModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return ::LoadLibraryW(ModPath.m_szPath);
}

#include <ndk/pstypes.h> /* for PROCESSINFOCLASS */

/* ntdll!NtQueryInformationProcess */
typedef NTSTATUS (WINAPI *FN_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

/* Is the current process on WoW64? */
static inline BOOL cicIsWow64(VOID)
{
    static FN_NtQueryInformationProcess s_fnNtQueryInformationProcess = NULL;
    ULONG_PTR Value;
    NTSTATUS Status;

    if (!s_fnNtQueryInformationProcess)
    {
        HMODULE hNTDLL = GetSystemModuleHandle(L"ntdll.dll", FALSE);
        if (!hNTDLL)
            return FALSE;

        s_fnNtQueryInformationProcess =
            (FN_NtQueryInformationProcess)GetProcAddress(hNTDLL, "NtQueryInformationProcess");
        if (!s_fnNtQueryInformationProcess)
            return FALSE;
    }

    Value = 0;
    Status = s_fnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information,
                                           &Value, sizeof(Value), NULL);
    if (!NT_SUCCESS(Status))
        return FALSE;

    return !!Value;
}

inline BOOL
CicSystemModulePath::Init(
    _In_ LPCWSTR pszFileName,
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
        cchPath = ::GetSystemDirectoryW(m_szPath, _countof(m_szPath));
    }

    m_szPath[_countof(m_szPath) - 1] = UNICODE_NULL; // Avoid buffer overrun

    if ((cchPath == 0) || (cchPath > _countof(m_szPath) - 2))
        goto Failure;

    // Add backslash if necessary
    if ((cchPath > 0) && (m_szPath[cchPath - 1] != L'\\'))
    {
        m_szPath[cchPath + 0] = L'\\';
        m_szPath[cchPath + 1] = UNICODE_NULL;
    }

    // Append pszFileName
    if (FAILED(StringCchCatW(m_szPath, _countof(m_szPath), pszFileName)))
        goto Failure;

    m_cchPath = wcslen(m_szPath);
    return TRUE;

Failure:
    m_szPath[0] = UNICODE_NULL;
    m_cchPath = 0;
    return FALSE;
}
