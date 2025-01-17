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

static VOID CleanupWindowList(VOID)
{
    GetWindowListForClose(&s_List2);
    CloseNewWindows(&s_List1, &s_List2);
    FreeWindowList(&s_List1);
    FreeWindowList(&s_List2);
}

START_TEST(ShellExec_RunDLL)
{
    HWND hwndNotepad;

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLL();
    Sleep(1000);
    hwndNotepad = FindWindowW(L"Notepad", NULL);
    ok(hwndNotepad != NULL, "Notepad not found\n");
    CleanupWindowList();

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLLA();
    Sleep(1000);
    hwndNotepad = FindWindowW(L"Notepad", NULL);
    ok(hwndNotepad != NULL, "Notepad not found\n");
    CleanupWindowList();

    GetWindowList(&s_List1);
    TEST_ShellExec_RunDLLW();
    Sleep(1000);
    hwndNotepad = FindWindowW(L"Notepad", NULL);
    ok(hwndNotepad != NULL, "Notepad not found\n");
    CleanupWindowList();
}
