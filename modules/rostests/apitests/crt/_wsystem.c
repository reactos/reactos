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

    GetSystemDirectoryW(szCmdExe, _countof(szCmdExe));
    lstrcatW(szCmdExe, L"\\cmd.exe");

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    ret = _wsystem(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    ret = _wsystem(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    ret = _wsystem(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    ret = _wsystem(L"echo This is a test");
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    ret = _wsystem(L"echo This is a test");
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    ret = _wsystem(L"echo This is a test");
    ok_int(ret, 0);

    SetEnvironmentVariableW(L"COMSPEC", NULL);
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", L"InvalidComSpec");
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(ret, 1);

    SetEnvironmentVariableW(L"COMSPEC", szCmdExe);
    ret = _wsystem(L"InvalidCommandLine");
    ok_int(ret, 1);
}
