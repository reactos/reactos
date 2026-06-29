/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for ShellExec_RunDLL
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include "closewnd.h"
#include <undocshell.h>

static WINDOW_LIST s_List1, s_List2;

static VOID TEST_ShellExec_RunDLL(VOID)
{
    ShellExec_RunDLL(NULL, NULL, "?0?notepad.exe", SW_SHOWNORMAL);
}

static VOID TEST_ShellExec_RunDLLA(VOID)
{
    ShellExec_RunDLLA(NULL, NULL, "?0?notepad.exe", SW_SHOWNORMAL);
}

static VOID TEST_ShellExec_RunDLLW(VOID)
{
    ShellExec_RunDLLW(NULL, NULL, L"?0?notepad.exe", SW_SHOWNORMAL);
}

static BOOL CloseNotepad(VOID)
{
    HWND hwndNew;
    WCHAR szClass[64];

    // Execution can be asynchronous; you have to wait for it to finish.
    Sleep(1000);

    // Close newly-opened window(s)
    GetWindowList(&s_List2);
    hwndNew = FindNewWindow(&s_List1, &s_List2);
    if (!GetClassNameW(hwndNew, szClass, _countof(szClass)))
        szClass[0] = UNICODE_NULL;
    CloseNewWindows(&s_List1, &s_List2);
    FreeWindowList(&s_List1);
    FreeWindowList(&s_List2);
    return lstrcmpiW(szClass, L"Notepad") == 0;
}

START_TEST(ShellExec_RunDLL)
{
    BOOL ret;

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLL();
    ret = CloseNotepad();
    ok(ret, "Notepad not found\n");

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLLA();
    ret = CloseNotepad();
    ok(ret, "Notepad not found\n");

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLLW();
    ret = CloseNotepad();
    ok(ret, "Notepad not found\n");
}
