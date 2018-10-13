/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosApplyFileIsolationRedirection_Ustr
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include "precomp.h"

#define ok_eq_hex(value, expected) ok((value) == (expected), #value " = 0x%lx, expected 0x%lx\n", value, expected)
#define ok_eq_pointer(value, expected) ok((value) == (expected), #value " = %p, expected %p\n", value, expected)

#define EXPECT_IN_SAME_DIR (WCHAR*)0xdead

UNICODE_STRING DotDll = RTL_CONSTANT_STRING(L".DLL");

struct test_data
{
    int testline;
    NTSTATUS ExpectedStatus;
    WCHAR* Param;
    WCHAR* ExpectedSubString;
};

struct test_data Tests[] =
{
    /* Not redirected file */
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"somefilesomefile", NULL},
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"ntdll.dll", NULL},
    /* Files redirected with sxs */
    {__LINE__, STATUS_SUCCESS, L"GDIPLUS", L"\\winsxs\\x86_microsoft.windows.gdiplus_6595b64144ccf1df_1."},
    {__LINE__, STATUS_SUCCESS, L"GDIPLUS.DLL", L"\\winsxs\\x86_microsoft.windows.gdiplus_6595b64144ccf1df_1."},
    {__LINE__, STATUS_SUCCESS, L"COMCTL32.DLL", L"\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82"},
    {__LINE__, STATUS_SUCCESS, L"comctl32.DLL", L"\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82"},
    {__LINE__, STATUS_SUCCESS, L"c:\\windows\\system32\\comctl32.DLL", L"\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82"},
    /* Files not redirected with sxs */
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"MSVCR90.DLL", NULL},
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"c:\\windows\\system32\\MSVCR90.DLL", NULL},
    /* Files defined in the manifest, one exists, one doesn't */
    {__LINE__, STATUS_SUCCESS, L"deptest.dll", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"adllfile.dll", EXPECT_IN_SAME_DIR},
    /* A file that exists in the same dir but isn't mentioned in the manifest */
    {__LINE__, STATUS_SUCCESS, L"fil1.txt", EXPECT_IN_SAME_DIR},
    /* This is a weird case; the source doesn't exist but does get redirected */
    {__LINE__, STATUS_SUCCESS, L"c:\\windows\\system32\\gdiplus.DLL", L"\\winsxs\\x86_microsoft.windows.gdiplus_6595b64144ccf1df_1."},
    /* But redirecting gdiplus from a different directory doesn't work */
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"c:\\GDIPLUS.DLL", NULL},
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"c:\\comctl32.DLL", NULL},
    /* Redirection based on .local */
    {__LINE__, STATUS_SUCCESS, L"test", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"test.dll", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"c:\\test.dll", EXPECT_IN_SAME_DIR},
    /* known dlls are also covered */
    {__LINE__, STATUS_SUCCESS, L"shell32", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"shell32.dll", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"c:\\shell32.dll", EXPECT_IN_SAME_DIR}
};

HANDLE _CreateActCtxFromFile(LPCWSTR FileName, int line)
{
    ACTCTXW ActCtx = {sizeof(ACTCTX)};
    HANDLE h;
    WCHAR buffer[MAX_PATH] , *separator;

    ok (GetModuleFileNameW(NULL, buffer, MAX_PATH), "GetModuleFileName failed\n");
    separator = wcsrchr(buffer, L'\\');
    if (separator)
        wcscpy(separator + 1, FileName);

    ActCtx.lpSource = buffer;

    SetLastError(0xdeaddead);
    h = CreateActCtxW(&ActCtx);
    ok_(__FILE__, line)(h != INVALID_HANDLE_VALUE, "CreateActCtx failed for %S\n", FileName);
    // In win10 last error is unchanged and in win2k3 it is ERROR_BAD_EXE_FORMAT
    ok_(__FILE__, line)(GetLastError() == ERROR_BAD_EXE_FORMAT || GetLastError() == 0xdeaddead, "Wrong last error %lu\n", GetLastError());

    return h;
}

void TestRedirection(void)
{
    WCHAR SystemDir[MAX_PATH];
    WCHAR TestPath[MAX_PATH];
    WCHAR ParameterBuffer[MAX_PATH];
    WCHAR* separator;
    NTSTATUS Status;
    int i;

    GetSystemDirectoryW(SystemDir, MAX_PATH);
    GetModuleFileNameW(NULL, TestPath, MAX_PATH);
    separator = wcsrchr(TestPath, L'\\');
    separator++;
    *separator = 0;

    for (i = 0; i < _countof(Tests); i ++)
    {
        UNICODE_STRING OriginalName, DynamicString;
        PUNICODE_STRING StringUsed = NULL;

        if (memcmp(Tests[i].Param, L"c:\\windows\\system32", 38) == 0)
        {
            wcscpy(ParameterBuffer, SystemDir);
            wcscat(ParameterBuffer, &Tests[i].Param[19]);
        }
        else
        {
            wcscpy(ParameterBuffer, Tests[i].Param);
        }

        RtlInitUnicodeString(&OriginalName, ParameterBuffer);
        RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);

        Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                          &OriginalName,
                                                          &DotDll,
                                                          NULL,
                                                          &DynamicString,
                                                          &StringUsed,
                                                          NULL,
                                                          NULL,
                                                          NULL);
        ok(Status == Tests[i].ExpectedStatus, "%d: Status 0x%lx, expected 0x%lx\n", Tests[i].testline, Status, Tests[i].ExpectedStatus);

        if(Tests[i].ExpectedSubString && DynamicString.Buffer == NULL)
        {
            ok(0, "%d: Expected a returned string\n", Tests[i].testline);
        }

        if (Tests[i].ExpectedSubString  && DynamicString.Buffer != NULL)
        {
            if (Tests[i].ExpectedSubString == EXPECT_IN_SAME_DIR)
            {
                ok(wcsstr(DynamicString.Buffer, TestPath) != 0, "%d: Expected string %S in %S\n", Tests[i].testline, TestPath, DynamicString.Buffer);
            }
            else
            {
                RtlDowncaseUnicodeString(&DynamicString, &DynamicString, FALSE);
                ok(wcsstr(DynamicString.Buffer, Tests[i].ExpectedSubString) != 0, "%d: Expected string %S in %S\n", Tests[i].testline, Tests[i].ExpectedSubString, DynamicString.Buffer);
            }
        }
    }
}

void TestBuffers()
{
    UNICODE_STRING Param, DynamicString, StaticString;
    PUNICODE_STRING StringUsed = NULL;
    NTSTATUS Status;
    WCHAR buffer[MAX_PATH];

    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_INVALID_PARAMETER, "0x%lx\n", Status);

    RtlInitEmptyUnicodeString(&Param, NULL, 0);
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SXS_KEY_NOT_FOUND, "0x%lx\n", Status);

    /* Tests for NULL termination of OriginalName */
    Param.MaximumLength = Param.Length = 12 * sizeof(WCHAR);
    Param.Buffer = L"comctl32.dllcrapcrap";
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");

    /* Tests for the Extension parameter */
    RtlInitUnicodeString(&Param, L"comctl32.dll");
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");

    RtlInitUnicodeString(&Param, L"comctl32");
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SXS_KEY_NOT_FOUND, "0x%lx\n", Status);

    RtlInitUnicodeString(&Param, L"comctl32");
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      &DotDll,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");

    /* Tests for the DynamicString parameter */
    RtlInitUnicodeString(&Param, L"comctl32.dll");
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      NULL,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");
    ok(DynamicString.Length >0 , "\n");
    ok(DynamicString.MaximumLength == DynamicString.Length + sizeof(WCHAR) , "\n");
    if (DynamicString.Buffer && DynamicString.Length)
        ok(wcslen(DynamicString.Buffer) * sizeof(WCHAR) == DynamicString.Length, "got %d and %d\n", wcslen(DynamicString.Buffer)  * sizeof(WCHAR) , DynamicString.Length);
    else
        ok(DynamicString.Buffer != NULL, "Expected non NULL buffer\n");
    ok(StringUsed == &DynamicString, "\n");

    /* Tests for the StaticString parameter */
    wcscpy(buffer, L"comctl32.dll");
    StaticString.Buffer = buffer;
    StaticString.Length = sizeof(L"comctl32.dll");
    StaticString.MaximumLength = sizeof(buffer);
    Param = StaticString;
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      &StaticString,
                                                      NULL,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SXS_KEY_NOT_FOUND, "0x%lx\n", Status);

    wcscpy(buffer, L"comctl32.dll");
    StaticString.Buffer = buffer;
    StaticString.Length = sizeof(L"comctl32.dll");
    StaticString.MaximumLength = sizeof(buffer);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &StaticString,
                                                      NULL,
                                                      &StaticString,
                                                      NULL,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SXS_KEY_NOT_FOUND, "0x%lx\n", Status);

    RtlInitUnicodeString(&Param, L"comctl32.dll");
    RtlInitEmptyUnicodeString(&StaticString, buffer, sizeof(buffer));
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      &StaticString,
                                                      NULL,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");
    ok(StaticString.Length >0 , "\n");
    ok(StaticString.MaximumLength == sizeof(buffer) , "\n");
    if (StaticString.Buffer && StaticString.Length)
        ok(wcslen(StaticString.Buffer) * sizeof(WCHAR) == StaticString.Length, "got %d and %d\n", wcslen(StaticString.Buffer)  * sizeof(WCHAR) , StaticString.Length);
    else
        ok(StaticString.Length != 0, "Expected non 0 lenght\n");
    ok(StringUsed == &StaticString, "\n");

    RtlInitEmptyUnicodeString(&StaticString, buffer, 5 * sizeof(WCHAR));
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      &StaticString,
                                                      NULL,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status == STATUS_BUFFER_TOO_SMALL, "0x%lx\n", Status);

    RtlInitUnicodeString(&Param, L"comctl32.dll");
    RtlInitEmptyUnicodeString(&StaticString, buffer, sizeof(buffer));
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      &StaticString,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");
    ok(StaticString.Length >0 , "\n");
    ok(StaticString.MaximumLength == sizeof(buffer) , "\n");
    if (StaticString.Buffer && StaticString.Length)
        ok(wcslen(StaticString.Buffer) * sizeof(WCHAR) == StaticString.Length, "got %d and %d\n", wcslen(StaticString.Buffer)  * sizeof(WCHAR) , StaticString.Length);
    else
        ok(StaticString.Length != 0, "Expected non 0 lenght\n");
    ok(DynamicString.Buffer == NULL, "\n");
    ok(DynamicString.Length == 0, "\n");
    ok(DynamicString.MaximumLength == 0, "\n");
    ok(StringUsed == &StaticString, "\n");

    /* Test a small buffer and a dynamic buffer */
    RtlInitUnicodeString(&Param, L"comctl32.dll");
    RtlInitEmptyUnicodeString(&StaticString, buffer, 5 * sizeof(WCHAR));
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    StaticString.Buffer[0] = 1;
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      &Param,
                                                      NULL,
                                                      &StaticString,
                                                      &DynamicString,
                                                      &StringUsed,
                                                      NULL,
                                                      NULL,
                                                      NULL);
    ok(Status ==STATUS_SUCCESS, "\n");
    ok(StaticString.Buffer == buffer, "\n");
    ok(StaticString.Length == 0 , "%d\n", StaticString.Length);
    ok(StaticString.Buffer[0] == 0, "\n");
    ok(StaticString.MaximumLength == 5 * sizeof(WCHAR) , "%d\n", StaticString.MaximumLength);
    ok(DynamicString.Length >0 , "\n");
    ok(DynamicString.MaximumLength == DynamicString.Length + sizeof(WCHAR) , "\n");
    if (DynamicString.Buffer && DynamicString.Length)
        ok(wcslen(DynamicString.Buffer) * sizeof(WCHAR) == DynamicString.Length, "got %d and %d\n", wcslen(DynamicString.Buffer)  * sizeof(WCHAR) , DynamicString.Length);
    else
        ok(DynamicString.Length != 0, "Expected non 0 lenght\n");

    ok(StringUsed == &DynamicString, "\n");
}

START_TEST(RtlDosApplyFileIsolationRedirection_Ustr)
{
    int argc;
    char **test_argv;
    argc = winetest_get_mainargs( &test_argv );
    if (argc >= 3)
    {
        HANDLE h = _CreateActCtxFromFile(L"ntdlltest.manifest", __LINE__);
        BOOL bactivated = FALSE;
        ULONG_PTR cookie;

        if (h != INVALID_HANDLE_VALUE)
            bactivated = ActivateActCtx(h, &cookie);

        TestRedirection();
        TestBuffers();

        if (bactivated)
            DeactivateActCtx(0, cookie);
    }
    else
    {
        WCHAR TestPath[MAX_PATH];
        WCHAR* separator;
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        BOOL created;

        GetModuleFileNameW(NULL, TestPath, MAX_PATH);
        separator = wcsrchr(TestPath, L'\\');
        separator++;
        wcscpy(separator, L"testdata\\ntdll_apitest.exe RtlDosApplyFileIsolationRedirection_Ustr DoTest");

        created = CreateProcessW(NULL, TestPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(created, "Expected CreateProcess to succeed\n");
        if (created)
        {
            winetest_wait_child_process(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}
