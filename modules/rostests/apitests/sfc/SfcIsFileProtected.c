/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SfcIsFileProtected
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <apitest.h>
#include <strsafe.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>

BOOL (WINAPI *SfcIsFileProtected)(HANDLE  RpcHandle,LPCWSTR ProtFileName);
static DWORD g_WinVersion;

typedef struct _osrange
{
    DWORD Min;
    DWORD Max;
} osrange;

typedef struct _testdata
{
    PCWSTR Path;
    BOOL Expand;
    osrange Success;
} testdata;


#define _WIN32_WINNT_MINVER 0x0001

#define MAKERANGE(from, to) \
    { _WIN32_WINNT_ ## from, _WIN32_WINNT_ ## to }

#define PASS(from, to) \
    MAKERANGE(from, to)

static testdata tests[] =
{
    { L"%systemroot%\\system32\\kernel32.dll", TRUE,                PASS(MINVER, WIN10) },
    { L"%SYSTEMROOT%\\SYSTEM32\\KERNEL32.DLL", TRUE,                PASS(MINVER, WIN10) },
    /* Paths are normalized on newer windows */
    { L"%systemroot%//system32\\kernel32.dll", TRUE,                PASS(VISTA, WIN10) },
    { L"%systemroot%\\system32\\..\\system32\\kernel32.dll", TRUE,  PASS(VISTA, WIN10) },

    /* These are rejected as-is */
    { L"kernel32.dll", FALSE,                                       PASS(MINVER, MINVER) },
    { L"%systemroot%//system32\\kernel32.dll", FALSE,               PASS(MINVER, MINVER) },
    /* Environment strings are expanded on older windows */
    { L"%systemroot%\\system32\\kernel32.dll", FALSE,               PASS(MINVER, WS03) },
    { L"%SYSTEMROOT%\\SYSTEM32\\KERNEL32.DLL", FALSE,               PASS(MINVER, WS03) },

    /* Show some files under SFC protection */
    { L"%systemroot%\\system32\\user32.dll", TRUE,                  PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\shell32.dll", TRUE,                 PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\browseui.dll", TRUE,                PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\apphelp.dll", TRUE,                 PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\sfc.dll", TRUE,                     PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\sfc_os.dll", TRUE,                  PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\sdbinst.exe", TRUE,                 PASS(MINVER, WIN10) },
    { L"%systemroot%\\AppPatch\\sysmain.sdb", TRUE,                 PASS(MINVER, WIN10) },
    { L"%systemroot%\\fonts\\tahoma.ttf", TRUE,                     PASS(MINVER, WIN10) },
    { L"%systemroot%\\fonts\\tahomabd.ttf", TRUE,                   PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\ntoskrnl.exe", TRUE,                PASS(MINVER, WIN10) },
    { L"%systemroot%\\system32\\c_1252.nls", TRUE,                  PASS(MINVER, WIN10) },
    { L"%systemroot%\\NOTEPAD.EXE", TRUE,                           PASS(MINVER, WIN10) },
};


static void Test_ProtectedFiles()
{
    ULONG n;
    BOOL Protected;
    WCHAR Buffer[MAX_PATH * 3];

    for (n = 0; n < _countof(tests); ++n)
    {
        if (tests[n].Expand)
            ExpandEnvironmentStringsW(tests[n].Path, Buffer, _countof(Buffer));
        else
            StringCchCopyW(Buffer, _countof(Buffer), tests[n].Path);

        Protected = SfcIsFileProtected(NULL, Buffer);

        if (g_WinVersion >= tests[n].Success.Min && g_WinVersion <= tests[n].Success.Max)
        {
            ok(Protected, "[%lu,0x%04lx]: Failed: %S\n", n, g_WinVersion, Buffer);
        }
        else
        {
            ok(!Protected, "[%lu,0x%04lx]: Succeeded: %S\n", n, g_WinVersion, Buffer);
        }
    }
}


START_TEST(SfcIsFileProtected)
{
    RTL_OSVERSIONINFOW rtlinfo = { sizeof(rtlinfo) };
    HMODULE mod;

    mod = LoadLibraryA("sfc_os.dll");
    ok(mod != NULL, "Unable to load sfc_os: 0x%lu\n", GetLastError());
    if (!mod)
        return;

    SfcIsFileProtected = (void*)GetProcAddress(mod, "SfcIsFileProtected");
    ok(SfcIsFileProtected != NULL, "Unable to resolve SfcIsFileProtected: 0x%lu\n", GetLastError());
    if (!SfcIsFileProtected)
        return;

    RtlGetVersion(&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;

    Test_ProtectedFiles();
}
