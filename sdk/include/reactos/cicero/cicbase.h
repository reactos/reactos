/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero base
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

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

#ifdef __cplusplus
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
#endif // __cplusplus

// FIXME: Use msutb.dll and header
static inline void ClosePopupTipbar(void)
{
}

// FIXME: Use msutb.dll and header
static inline void GetPopupTipbar(HWND hwnd, BOOL fWinLogon)
{
}

/* The flags of GetOSInfo() */
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

// ntdll!NtQueryInformationProcess
typedef NTSTATUS (WINAPI *FN_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

// Is the current process on WoW64?
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
