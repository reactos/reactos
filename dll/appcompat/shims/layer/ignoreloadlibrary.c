/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IgnoreLoadLibrary shim
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include "ntndk.h"

typedef HMODULE(WINAPI* LOADLIBRARYAPROC)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYEXAPROC)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI* LOADLIBRARYWPROC)(LPCWSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYEXWPROC)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);


#define SHIM_NS         IgnoreLoadLibrary
#include <setup_shim.inl>


HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryA)(LPCSTR lpLibFileName)
{
    HMODULE Module;
    DWORD dwOldErrorMode;

    dwOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    Module = CALL_SHIM(0, LOADLIBRARYAPROC)(lpLibFileName);
    SetErrorMode(dwOldErrorMode);

    return Module;
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryExA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE Module;
    DWORD dwOldErrorMode;

    dwOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    Module = CALL_SHIM(1, LOADLIBRARYEXAPROC)(lpLibFileName, hFile, dwFlags);
    SetErrorMode(dwOldErrorMode);

    return Module;
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryW)(LPCWSTR lpLibFileName)
{
    HMODULE Module;
    DWORD dwOldErrorMode;

    dwOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    Module = CALL_SHIM(2, LOADLIBRARYWPROC)(lpLibFileName);
    SetErrorMode(dwOldErrorMode);

    return Module;
}

HMODULE WINAPI SHIM_OBJ_NAME(APIHook_LoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    HMODULE Module;
    DWORD dwOldErrorMode;

    dwOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    Module = CALL_SHIM(3, LOADLIBRARYEXWPROC)(lpLibFileName, hFile, dwFlags);
    SetErrorMode(dwOldErrorMode);

    return Module;
}


#define SHIM_NUM_HOOKS  4
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "LoadLibraryA", SHIM_OBJ_NAME(APIHook_LoadLibraryA)) \
    SHIM_HOOK(1, "KERNEL32.DLL", "LoadLibraryExA", SHIM_OBJ_NAME(APIHook_LoadLibraryExA)) \
    SHIM_HOOK(2, "KERNEL32.DLL", "LoadLibraryW", SHIM_OBJ_NAME(APIHook_LoadLibraryW)) \
    SHIM_HOOK(3, "KERNEL32.DLL", "LoadLibraryExW", SHIM_OBJ_NAME(APIHook_LoadLibraryExW))

#include <implement_shim.inl>
