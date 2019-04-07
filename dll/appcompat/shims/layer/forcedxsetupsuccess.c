/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ForceDxSetupSuccess shim
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include "ntndk.h"

typedef HMODULE(WINAPI* LOADLIBRARYAPROC)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYWPROC)(LPCWSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYEXAPROC)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI* LOADLIBRARYEXWPROC)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef FARPROC(WINAPI* GETPROCADDRESSPROC)(HMODULE hModule, LPCSTR lpProcName);
typedef BOOL   (WINAPI* FREELIBRARYPROC)(HINSTANCE hLibModule);


#define SHIM_NS         ForceDXSetupSuccess
#include <setup_shim.inl>


#define DSETUPERR_SUCCESS 0

INT_PTR WINAPI DirectXSetup(HWND hWnd, LPSTR lpszRootPath, DWORD dwFlags)
{
    SHIM_MSG("Returning DSETUPERR_SUCCESS\n");
    return DSETUPERR_SUCCESS;
}

INT_PTR WINAPI DirectXSetupA(HWND hWnd, LPSTR lpszRootPath, DWORD dwFlags)
{
    SHIM_MSG("Returning DSETUPERR_SUCCESS\n");
    return DSETUPERR_SUCCESS;
}

INT_PTR WINAPI DirectXSetupW(HWND hWnd, LPWSTR lpszRootPath, DWORD dwFlags)
{
    SHIM_MSG("Returning DSETUPERR_SUCCESS\n");
    return DSETUPERR_SUCCESS;
}

INT_PTR WINAPI DirectXSetupGetVersion(DWORD *lpdwVersion, DWORD *lpdwMinorVersion)
{
    if (lpdwVersion)
        *lpdwVersion = MAKELONG(7, 4);     // DirectX 7.0
    if (lpdwMinorVersion)
        *lpdwMinorVersion = MAKELONG(1792, 0);

    return TRUE;
}

static BOOL IsDxSetupW(PCUNICODE_STRING LibraryPath)
{
    static const UNICODE_STRING DxSetupDlls[] = {
        RTL_CONSTANT_STRING(L"dsetup.dll"),
        RTL_CONSTANT_STRING(L"dsetup32.dll"),
        RTL_CONSTANT_STRING(L"dsetup"),
        RTL_CONSTANT_STRING(L"dsetup32"),
    };
    static const UNICODE_STRING PathDividerFind = RTL_CONSTANT_STRING(L"\\/");
    UNICODE_STRING LibraryName;
    USHORT PathDivider;
    DWORD n;

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, LibraryPath, &PathDividerFind, &PathDivider)))
        PathDivider = 0;

    if (PathDivider)
        PathDivider += sizeof(WCHAR);

    LibraryName.Buffer = LibraryPath->Buffer + PathDivider / sizeof(WCHAR);
    LibraryName.Length = LibraryPath->Length - PathDivider;
    LibraryName.MaximumLength = LibraryPath->MaximumLength - PathDivider;

    for (n = 0; n < ARRAYSIZE(DxSetupDlls); ++n)
    {
        if (RtlEqualUnicodeString(&LibraryName, DxSetupDlls + n, TRUE))
        {
            SHIM_MSG("Found %wZ\n", DxSetupDlls + n);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL IsDxSetupA(PCANSI_STRING LibraryPath)
{
    BOOL bIsDxSetup;
    UNICODE_STRING LibraryPathW;

    if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&LibraryPathW, LibraryPath, TRUE)))
    {
        bIsDxSetup = IsDxSetupW(&LibraryPathW);
        RtlFreeUnicodeString(&LibraryPathW);
        return bIsDxSetup;
    }
    return FALSE;
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryA)(LPCSTR lpLibFileName)
{
    ANSI_STRING Library;

    RtlInitAnsiString(&Library, lpLibFileName);
    if (IsDxSetupA(&Library))
        return ShimLib_Instance();

    return CALL_SHIM(0, LOADLIBRARYAPROC)(lpLibFileName);
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryW)(LPCWSTR lpLibFileName)
{
    UNICODE_STRING Library;

    RtlInitUnicodeString(&Library, lpLibFileName);
    if (IsDxSetupW(&Library))
        return ShimLib_Instance();

    return CALL_SHIM(1, LOADLIBRARYWPROC)(lpLibFileName);
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryExA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    ANSI_STRING Library;

    RtlInitAnsiString(&Library, lpLibFileName);
    if (IsDxSetupA(&Library))
        return ShimLib_Instance();

    return CALL_SHIM(2, LOADLIBRARYEXAPROC)(lpLibFileName, hFile, dwFlags);
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    UNICODE_STRING Library;

    RtlInitUnicodeString(&Library, lpLibFileName);
    if (IsDxSetupW(&Library))
        return ShimLib_Instance();

    return CALL_SHIM(3, LOADLIBRARYEXWPROC)(lpLibFileName, hFile, dwFlags);
}

FARPROC WINAPI SHIM_OBJ_NAME(APIHook_GetProcAddress)(HMODULE hModule, LPCSTR lpProcName)
{
    static const STRING DxSetupFunctions[] = {
        RTL_CONSTANT_STRING("DirectXSetup"),
        RTL_CONSTANT_STRING("DirectXSetupA"),
        RTL_CONSTANT_STRING("DirectXSetupW"),
        RTL_CONSTANT_STRING("DirectXSetupGetVersion"),
    };
    static const FARPROC DxSetupRedirections[] = {
        DirectXSetup,
        DirectXSetupA,
        DirectXSetupW,
        DirectXSetupGetVersion,
    };
    DWORD n;

    if (hModule == ShimLib_Instance() && ((ULONG_PTR)lpProcName > MAXUSHORT))
    {
        STRING ProcName;
        RtlInitAnsiString(&ProcName, lpProcName);
        for (n = 0; n < ARRAYSIZE(DxSetupFunctions); ++n)
        {
            if (RtlEqualString(&ProcName, DxSetupFunctions + n, TRUE))
            {
                SHIM_MSG("Intercepted %Z\n", DxSetupFunctions + n);
                return DxSetupRedirections[n];
            }
        }
    }
    return CALL_SHIM(4, GETPROCADDRESSPROC)(hModule, lpProcName);
}

BOOL WINAPI SHIM_OBJ_NAME(APIHook_FreeLibrary)(HINSTANCE hLibModule)
{
    if (hLibModule == ShimLib_Instance())
    {
        SHIM_MSG("Intercepted\n");
        return TRUE;
    }

    return CALL_SHIM(5, FREELIBRARYPROC)(hLibModule);
}


#define SHIM_NUM_HOOKS  6
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "LoadLibraryA", SHIM_OBJ_NAME(APIHook_LoadLibraryA)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "LoadLibraryW", SHIM_OBJ_NAME(APIHook_LoadLibraryW)) \
    SHIM_HOOK(2, "KERNEL32.DLL", "LoadLibraryExA", SHIM_OBJ_NAME(APIHook_LoadLibraryExA)) \
    SHIM_HOOK(3, "KERNEL32.DLL", "LoadLibraryExW", SHIM_OBJ_NAME(APIHook_LoadLibraryExW)) \
    SHIM_HOOK(4, "KERNEL32.DLL", "GetProcAddress", SHIM_OBJ_NAME(APIHook_GetProcAddress)) \
    SHIM_HOOK(5, "KERNEL32.DLL", "FreeLibrary", SHIM_OBJ_NAME(APIHook_FreeLibrary))

#include <implement_shim.inl>
