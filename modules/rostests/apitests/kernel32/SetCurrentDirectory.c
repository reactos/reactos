/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SetCurrentDirectory
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
VOID
SetUnrelatedDirectory(VOID)
{
    WCHAR Buffer[MAX_PATH];
    BOOL Ret;

    GetWindowsDirectoryW(Buffer, sizeof(Buffer) / sizeof(WCHAR));
    Ret = SetCurrentDirectoryW(Buffer);
    ok(Ret == TRUE, "SetCurrentDirectoryW failed\n");
}

static
VOID
TestSetCurrentDirectoryA(VOID)
{
    BOOL Ret;
    CHAR Buffer[MAX_PATH];
    DWORD Length;

    SetUnrelatedDirectory();

    Ret = SetCurrentDirectoryA("C:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryA failed\n");
    Length = GetCurrentDirectoryA(sizeof(Buffer), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!strcmp(Buffer, "C:\\"), "Current directory is %s\n", Buffer);

    /* Same directory - succeeds */
    Ret = SetCurrentDirectoryA("C:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryA failed\n");
    Length = GetCurrentDirectoryA(sizeof(Buffer), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!strcmp(Buffer, "C:\\"), "Current directory is %s\n", Buffer);

    /* Same directory, lowercase - succeeds and does not change case */
    Ret = SetCurrentDirectoryA("c:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryA failed\n");
    Length = GetCurrentDirectoryA(sizeof(Buffer), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!strcmp(Buffer, "C:\\"), "Current directory is %s\n", Buffer);

    SetUnrelatedDirectory();

    /* Now this one does change case */
    Ret = SetCurrentDirectoryA("c:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryA failed\n");
    Length = GetCurrentDirectoryA(sizeof(Buffer), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!strcmp(Buffer, "c:\\"), "Current directory is %s\n", Buffer);
}

static
VOID
TestSetCurrentDirectoryW(VOID)
{
    BOOL Ret;
    WCHAR Buffer[MAX_PATH];
    DWORD Length;

    SetUnrelatedDirectory();

    Ret = SetCurrentDirectoryW(L"C:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryW failed\n");
    Length = GetCurrentDirectoryW(sizeof(Buffer) / sizeof(WCHAR), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!wcscmp(Buffer, L"C:\\"), "Current directory is %ls\n", Buffer);

    /* Same directory - succeeds */
    Ret = SetCurrentDirectoryW(L"C:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryW failed\n");
    Length = GetCurrentDirectoryW(sizeof(Buffer) / sizeof(WCHAR), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!wcscmp(Buffer, L"C:\\"), "Current directory is %ls\n", Buffer);

    /* Same directory, lowercase - succeeds and does not change case */
    Ret = SetCurrentDirectoryW(L"c:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryW failed\n");
    Length = GetCurrentDirectoryW(sizeof(Buffer) / sizeof(WCHAR), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!wcscmp(Buffer, L"C:\\"), "Current directory is %ls\n", Buffer);

    SetUnrelatedDirectory();

    /* Now this one does change case */
    Ret = SetCurrentDirectoryW(L"c:\\");
    ok(Ret == TRUE, "SetCurrentDirectoryW failed\n");
    Length = GetCurrentDirectoryW(sizeof(Buffer) / sizeof(WCHAR), Buffer);
    ok(Length == 3, "Length = %lu\n", Length);
    ok(!wcscmp(Buffer, L"c:\\"), "Current directory is %ls\n", Buffer);
}

START_TEST(SetCurrentDirectory)
{
    TestSetCurrentDirectoryA();
    TestSetCurrentDirectoryW();
}
