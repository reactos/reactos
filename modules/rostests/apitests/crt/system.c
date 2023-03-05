/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for system()
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <apitest_guard.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

START_TEST(system)
{
    int ret;

    SetEnvironmentVariableA("COMSPEC", NULL);
    ret = system(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "cmd.exe");
    ret = system(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    ret = system(NULL);
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", NULL);
    ret = system("dir");
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", "cmd.exe");
    ret = system("dir");
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    ret = system("dir");
    ok_int(ret, 0);

    SetEnvironmentVariableA("COMSPEC", NULL);
    ret = system("InvalidCommandLine");
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "cmd.exe");
    ret = system("InvalidCommandLine");
    ok_int(ret, 1);

    SetEnvironmentVariableA("COMSPEC", "InvalidComSpec");
    ret = system("InvalidCommandLine");
    ok_int(ret, 1);
}
