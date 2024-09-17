/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for reactos compatibility layer
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>
#include <windows.h>
#include <stdio.h>
#include <ndk/rtlfuncs.h>

DWORD g_WinVersion;

struct
{
    PSTR DllName;
    PSTR FunctionName;
    ULONG MinVersion;
    ULONG MaxVersion;
} g_TestCases[] =
{
    {"ntdll.dll", "NtCreateChannel", 0x0500, 0x0500},
    {"ntdll.dll", "LdrFindCreateProcessManifest", 0x0501, 0x0502},
    {"ntdll.dll", "NtCreateEnlistment", 0x0600, 0xFFFF},
};

static
void
InsideCompatMode(PSTR CompatMode)
{
    CHAR szEnvVar[20];
    HMODULE hModule;
    PVOID pAddress;

    GetEnvironmentVariableA("__COMPAT_LAYER", szEnvVar, _countof(szEnvVar));
    fprintf(stderr, "Running test as %s, __COMPAT_LAYER='%s', g_Winver=0x%lx\n",
            CompatMode,
            szEnvVar,
            g_WinVersion);

    for (ULONG i = 0; i < _countof(g_TestCases); i++)
    {
        fprintf(stderr, "i=%lu, 0x%lx [0x%lx .. 0x%lx]\n",
                i,
                g_WinVersion,
                g_TestCases[i].MinVersion,
                g_TestCases[i].MaxVersion);

        hModule = LoadLibraryA(g_TestCases[i].DllName);
        if (hModule == NULL)
        {
            printf("Failed to load %s\n", g_TestCases[i].DllName);
            continue;
        }

        pAddress = GetProcAddress(hModule, g_TestCases[i].FunctionName);
        if ((g_WinVersion >= g_TestCases[i].MinVersion) &&
            (g_WinVersion <= g_TestCases[i].MaxVersion))
        {
            fprintf(stderr, "1\n");
            ok(pAddress != NULL, "%s should be present\n", g_TestCases[i].FunctionName);
        }
        else
        {
            fprintf(stderr, "2\n");
            ok(pAddress == NULL, "%s should not be present\n", g_TestCases[i].FunctionName);
        }

        FreeLibrary(hModule);
    }

    // TODO: Load a DLL with 0 exports and check if it's loaded

}

static
int
RunTestInCompatMode(PSTR CompatMode)
{
    CHAR szCmdLine[MAX_PATH];
    DWORD ret;

    ret = GetModuleFileNameA(NULL, szCmdLine, _countof(szCmdLine));
    lstrcatA(szCmdLine, " roscompat ");
    lstrcatA(szCmdLine, CompatMode);

    // Set the compatibility mode
    SetEnvironmentVariableA("__COMPAT_LAYER", CompatMode);

    // Run the test
    ret = system(szCmdLine);

    // Clear the compatibility mode
    SetEnvironmentVariableA("__COMPAT_LAYER", NULL);

    return ret;
}

BOOL
IsThisReactOS()
{
    HKEY hKey;
    LONG lRes;

    lRes = RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ReactOS", &hKey);
    return (lRes == ERROR_SUCCESS);
}

START_TEST(roscompat)
{
#if 0
    if (!IsThisReactOS())
    {
        skip("This test is only for ReactOS\n")
        return;
    }
#endif
    g_WinVersion = NtCurrentPeb()->OSMajorVersion << 8 | NtCurrentPeb()->OSMinorVersion;

    if (__argc > 2)
    {
        InsideCompatMode(__argv[2]);
        return;
    }

    RunTestInCompatMode("Win2k");
    RunTestInCompatMode("WinXP");
    RunTestInCompatMode("Win2k3");
    RunTestInCompatMode("Vista");
    RunTestInCompatMode("Win7");
    RunTestInCompatMode("Win8");
}
