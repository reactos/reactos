/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero base
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

struct CicNoThrow { };
#define cicNoThrow CicNoThrow{}

void* operator new(size_t size, const CicNoThrow&) noexcept;
void* operator new[](size_t size, const CicNoThrow&) noexcept;
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;

/* The flags of cicGetOSInfo() */
#define CIC_OSINFO_NT     0x01
#define CIC_OSINFO_2KPLUS 0x02
#define CIC_OSINFO_95     0x04
#define CIC_OSINFO_98PLUS 0x08
#define CIC_OSINFO_CJK    0x10
#define CIC_OSINFO_IMM    0x20
#define CIC_OSINFO_DBCS   0x40
#define CIC_OSINFO_XPPLUS 0x80

EXTERN_C
void cicGetOSInfo(LPUINT puACP, LPDWORD pdwOSInfo);

#ifdef __cplusplus
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
#endif

// Get an instance handle that is already loaded
EXTERN_C
HINSTANCE
cicGetSystemModuleHandle(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir);

// Load a system library
EXTERN_C
HINSTANCE
cicLoadSystemLibrary(
    _In_ LPCTSTR pszFileName,
    _In_ BOOL bSysWinDir);

#ifdef __cplusplus
template <typename T_FN>
static inline BOOL
cicGetFN(HINSTANCE& hinstDLL, T_FN& fn, LPCTSTR pszDllName, LPCSTR pszFuncName)
{
    if (fn)
        return TRUE;
    if (!hinstDLL)
        hinstDLL = cicLoadSystemLibrary(pszDllName, FALSE);
    if (!hinstDLL)
        return FALSE;
    fn = reinterpret_cast<T_FN>(GetProcAddress(hinstDLL, pszFuncName));
    return !!fn;
}
#endif

/* Is the current process on WoW64? */
EXTERN_C
BOOL cicIsWow64(VOID);

EXTERN_C
HRESULT
cicRealCoCreateInstance(
    _In_ REFCLSID rclsid,
    _In_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID iid,
    _Out_ LPVOID *ppv);

EXTERN_C
HRESULT
cicCoCreateInstance(
    _In_ REFCLSID rclsid,
    _In_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID iid,
    _Out_ LPVOID *ppv);

// ole32!CoCreateInstance
typedef HRESULT (WINAPI *FN_CoCreateInstance)(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv);

EXTERN_C BOOL TFInitLib(FN_CoCreateInstance fnCoCreateInstance = NULL);
EXTERN_C VOID TFUninitLib(VOID);
