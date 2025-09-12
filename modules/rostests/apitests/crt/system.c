/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for system()
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <apitest_guard.h>

START_TEST(system)
{
    int ret;
    CHAR szCmdExe[MAX_PATH];

    GetSystemDirectoryA(szCmdExe, _countof(szCmdExe));
    lstrcatA(szCmdExe, "\\cmd.exe");

    SetEnvironmentVariableA("COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = system(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = system(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = system(NULL);
    ok_int(errno, 0xDEADBEEF);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = system("echo This is a test");
    ok_int(errno, 0);
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = system("echo This is a test");
    ok_int(errno, 0);
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = system("echo This is a test");
    ok_int(errno, 0);
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", NULL);
    errno = 0xDEADBEEF;
    ret = system("InvalidCommandLine");
    ok_int(errno, 0);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    errno = 0xDEADBEEF;
    ret = system("InvalidCommandLine");
    ok_int(errno, 0);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", szCmdExe);
    errno = 0xDEADBEEF;
    ret = system("InvalidCommandLine");
    ok_int(errno, 0);
    ok_int(ret, 1);
}
