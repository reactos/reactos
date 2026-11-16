/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        base/applications/verreport/verreport.c
 * PURPOSE:     Print the runtime NT version and the unmanifested GetVersion lie
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#define NTOS_MODE_USER
#include <ndk/ketypes.h>
#include <reactos/runtimever.h>

typedef LONG (NTAPI *PFN_RTL_GET_VERSION)(OSVERSIONINFOEXW *);
typedef ULONG (__cdecl *PFN_DBG_PRINT)(PCSTR, ...);

static VOID
VerpPrint(PFN_DBG_PRINT DbgPrint, PCSTR Text)
{
    if (DbgPrint)
        DbgPrint("%s", Text);
}

static VOID
VerpQueryString(HKEY Key, PCWSTR Name, CHAR Buffer[], DWORD Cch)
{
    WCHAR Wide[64];
    DWORD Type = 0;
    DWORD Bytes = sizeof(Wide) - sizeof(WCHAR);
    LSTATUS Status;

    Buffer[0] = ANSI_NULL;
    Wide[0] = UNICODE_NULL;
    Status = RegQueryValueExW(Key, Name, NULL, &Type, (LPBYTE)Wide, &Bytes);
    if (Status != ERROR_SUCCESS || (Type != REG_SZ && Type != REG_EXPAND_SZ))
        return;

    Wide[ARRAYSIZE(Wide) - 1] = UNICODE_NULL;
    for (Bytes = 0; Wide[Bytes] != UNICODE_NULL && Bytes + 1 < Cch; Bytes++)
        Buffer[Bytes] = (CHAR)Wide[Bytes];
    Buffer[Bytes] = ANSI_NULL;
}

int
WINAPI
wWinMain(HINSTANCE Instance,
         HINSTANCE Previous,
         LPWSTR Command,
         int Show)
{
    PFN_RTL_GET_VERSION RtlGetVersion;
    PFN_DBG_PRINT DbgPrint;
    OSVERSIONINFOEXW Info;
    DWORD Version;
    HKEY Key;
    CHAR CurrentVersion[32];
    CHAR CurrentBuild[32];
    DWORD MajorNumber = 0;
    DWORD MajorBytes = sizeof(MajorNumber);
    DWORD MajorType = 0;
    CHAR Line[160];
    ULONG SharedBuild;
    HMODULE Ntdll;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Previous);
    UNREFERENCED_PARAMETER(Command);
    UNREFERENCED_PARAMETER(Show);

    Ntdll = GetModuleHandleW(L"ntdll.dll");
    RtlGetVersion = (PFN_RTL_GET_VERSION)GetProcAddress(Ntdll, "RtlGetVersion");
    DbgPrint = (PFN_DBG_PRINT)GetProcAddress(Ntdll, "DbgPrint");

    SharedBuild = *(PULONG)((PUCHAR)SharedUserData + RUNTIME_NT_SHARED_BUILD_OFFSET);
    _snprintf(Line, sizeof(Line), "VERPORT shared %lu.%lu.%lu\n",
              SharedUserData->NtMajorVersion,
              SharedUserData->NtMinorVersion,
              SharedBuild);
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);

    ZeroMemory(&Info, sizeof(Info));
    Info.dwOSVersionInfoSize = sizeof(Info);
    if (RtlGetVersion && RtlGetVersion(&Info) == 0)
    {
        _snprintf(Line, sizeof(Line), "VERPORT rtl %lu.%lu.%lu\n",
                  Info.dwMajorVersion, Info.dwMinorVersion, Info.dwBuildNumber);
    }
    else
    {
        _snprintf(Line, sizeof(Line), "VERPORT rtl failed\n");
    }
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);

    Version = GetVersion();
    _snprintf(Line, sizeof(Line), "VERPORT getversion %u.%u.%u\n",
              LOBYTE(LOWORD(Version)), HIBYTE(LOWORD(Version)), HIWORD(Version));
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);

    ZeroMemory(&Info, sizeof(Info));
    Info.dwOSVersionInfoSize = sizeof(Info);
    if (GetVersionExW((LPOSVERSIONINFOW)&Info))
    {
        _snprintf(Line, sizeof(Line), "VERPORT getversionex %lu.%lu.%lu\n",
                  Info.dwMajorVersion, Info.dwMinorVersion, Info.dwBuildNumber);
    }
    else
    {
        _snprintf(Line, sizeof(Line), "VERPORT getversionex failed\n");
    }
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);

    CurrentVersion[0] = UNICODE_NULL;
    CurrentBuild[0] = UNICODE_NULL;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0,
                      KEY_QUERY_VALUE,
                      &Key) == ERROR_SUCCESS)
    {
        VerpQueryString(Key, L"CurrentVersion", CurrentVersion, ARRAYSIZE(CurrentVersion));
        VerpQueryString(Key, L"CurrentBuildNumber", CurrentBuild, ARRAYSIZE(CurrentBuild));
        if (RegQueryValueExW(Key,
                             L"CurrentMajorVersionNumber",
                             NULL,
                             &MajorType,
                             (LPBYTE)&MajorNumber,
                             &MajorBytes) != ERROR_SUCCESS ||
            MajorType != REG_DWORD)
        {
            MajorNumber = 0;
        }
        RegCloseKey(Key);
    }

    _snprintf(Line, sizeof(Line), "VERPORT currentversion %s\n", CurrentVersion);
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);
    _snprintf(Line, sizeof(Line), "VERPORT major %lu\n", MajorNumber);
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);
    _snprintf(Line, sizeof(Line), "VERPORT build %s\n", CurrentBuild);
    Line[sizeof(Line) - 1] = ANSI_NULL;
    VerpPrint(DbgPrint, Line);
    VerpPrint(DbgPrint, "VERPORT END\n");
    return 0;
}
