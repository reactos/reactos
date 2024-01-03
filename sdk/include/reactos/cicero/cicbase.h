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

struct CicNoThrow { };
#define cicNoThrow CicNoThrow{}

inline void* operator new(size_t size, const CicNoThrow&) noexcept
{
    return cicMemAllocClear(size);
}
inline void* operator new[](size_t size, const CicNoThrow&) noexcept
{
    return cicMemAllocClear(size);
}
inline void operator delete(void* ptr) noexcept
{
    cicMemFree(ptr);
}
inline void operator delete[](void* ptr) noexcept
{
    cicMemFree(ptr);
}
inline void operator delete(void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}
inline void operator delete[](void* ptr, size_t size) noexcept
{
    cicMemFree(ptr);
}

typedef struct CIC_LIBTHREAD
{
    IUnknown *m_pUnknown1;
    ITfDisplayAttributeMgr *m_pDisplayAttrMgr;
} CIC_LIBTHREAD, *PCIC_LIBTHREAD;

/* The flags of cicGetOSInfo() */
#define CIC_OSINFO_NT     0x01
#define CIC_OSINFO_2KPLUS 0x02
#define CIC_OSINFO_95     0x04
#define CIC_OSINFO_98PLUS 0x08
#define CIC_OSINFO_CJK    0x10
#define CIC_OSINFO_IMM    0x20
#define CIC_OSINFO_DBCS   0x40
#define CIC_OSINFO_XPPLUS 0x80

static inline void
cicGetOSInfo(LPUINT puACP, LPDWORD pdwOSInfo)
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

struct CicSystemModulePath
{
    TCHAR m_szPath[MAX_PATH + 2];
    SIZE_T m_cchPath;

    CicSystemModulePath()
    {
        m_szPath[0] = UNICODE_NULL;
        m_cchPath = 0;
    }

    BOOL Init(_In_ LPCTSTR pszFileName, _In_ BOOL bSysWinDir);
};

// Get an instance handle that is already loaded
static inline HINSTANCE
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
static inline HINSTANCE
cicLoadSystemLibrary(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CicSystemModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return ::LoadLibrary(ModPath.m_szPath);
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
        HMODULE hNTDLL = cicGetSystemModuleHandle(TEXT("ntdll.dll"), FALSE);
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

// ole32!CoCreateInstance
typedef HRESULT (WINAPI *FN_CoCreateInstance)(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv);

static inline FN_CoCreateInstance
_cicGetSetUserCoCreateInstance(FN_CoCreateInstance fnUserCoCreateInstance)
{
    static FN_CoCreateInstance s_fn = NULL;
    if (fnUserCoCreateInstance)
        s_fn = fnUserCoCreateInstance;
    return s_fn;
}

static inline HRESULT
cicRealCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
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
static inline HRESULT
cicCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
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
static inline BOOL
TFInitLib(FN_CoCreateInstance fnCoCreateInstance = NULL)
{
    if (fnCoCreateInstance)
        _cicGetSetUserCoCreateInstance(fnCoCreateInstance);
    return TRUE;
}

/**
 * @unimplemented
 */
static inline VOID
TFUninitLib(VOID)
{
    //FIXME
}

/**
 * @implemented
 */
static inline VOID
TFUninitLib_Thread(PCIC_LIBTHREAD pLibThread)
{
    if (!pLibThread)
        return;

    if (pLibThread->m_pUnknown1)
    {
        pLibThread->m_pUnknown1->Release();
        pLibThread->m_pUnknown1 = NULL;
    }

    if (pLibThread->m_pDisplayAttrMgr)
    {
        pLibThread->m_pDisplayAttrMgr->Release();
        pLibThread->m_pDisplayAttrMgr = NULL;
    }
}
