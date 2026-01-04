/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wsystem()
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <apitest_guard.h>

START_TEST(_wsystem)
{
    int ret;
    WCHAR szCmdExe[MAX_PATH];

    /* ReactOS behaves like Vista here */
    DWORD dwOsVer = is_reactos() ? _WIN32_WINNT_VISTA : GetNTVersion();

    GetSystemDirectoryW(szCmdExe, _countof(szCmdExe));
    lstrcatW(szCmdExe, L"\\cmd.exe");

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = _wsystem(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0 : 1);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = _wsystem(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0 : 1);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = _wsystem(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0 : 1);

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = _wsystem(L"echo This is a test");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = _wsystem(L"echo This is a test");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = _wsystem(L"echo This is a test");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(errno, (dwOsVer >= _WIN32_WINNT_VISTA) ? 0xdeadbeef : 0);
    ok_int(ret, 1);
}
