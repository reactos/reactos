/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosApplyFileIsolationRedirection_Ustr
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

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
    /* This is a weird case; the source doesn't exist but does get redirected */
    {__LINE__, STATUS_SUCCESS, L"c:\\windows\\system32\\gdiplus.DLL", L"\\winsxs\\x86_microsoft.windows.gdiplus_6595b64144ccf1df_1."},
    /* But redirecting gdiplus from a different directory doesn't work */
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"c:\\GDIPLUS.DLL", NULL},
    {__LINE__, STATUS_SXS_KEY_NOT_FOUND, L"c:\\comctl32.DLL", NULL},
#if 0
    /* Redirection based on .local */
    {__LINE__, STATUS_SUCCESS, L"test", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"test.dll", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"c:\\test.dll", EXPECT_IN_SAME_DIR},
    /* known dlls are also covered */
    {__LINE__, STATUS_SUCCESS, L"shell32", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"shell32.dll", EXPECT_IN_SAME_DIR},
    {__LINE__, STATUS_SUCCESS, L"c:\\shell32.dll", EXPECT_IN_SAME_DIR}
#endif
};

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
        ok(Status == Tests[i].ExpectedStatus, "Status 0x%lx, expected 0x%lx\n", value, expected)
        ok_eq_hex(Status, Tests[i].ExpectedStatus);

        if (DynamicString.Buffer)
        {
            BOOL exists = RtlDoesFileExists_U(DynamicString.Buffer);
            ok(exists, "%d: Expected file %S to exist!\n", Tests[i].testline, DynamicString.Buffer);
        }

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

START_TEST(RtlDosApplyFileIsolationRedirection_Ustr)
{
#if 0
    WCHAR TestPath[MAX_PATH];
    WCHAR* separator;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL created;
    HANDLE file;

    /* Create .local files */
    GetModuleFileNameW(NULL, TestPath, MAX_PATH);

    wcscat(TestPath, L".local");
    file = CreateFileW(TestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING , NULL);
    CloseHandle(file);
    separator = wcsrchr(TestPath, L'\\');
    separator++;
    wcscpy(separator, L"test.dll");
    file = CreateFileW(TestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING , NULL);
    CloseHandle(file);
    wcscpy(separator, L"shell32.dll");
    file = CreateFileW(TestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING , NULL);
    CloseHandle(file);
    *separator = 0;    
#endif
    TestRedirection();
#if 0
    /*Cleanup*/
    wcscpy(separator, L"test.dll");
    DeleteFileW(TestPath);
    wcscpy(separator, L"shell32.dll");
    DeleteFileW(TestPath);
    GetModuleFileNameW(NULL, TestPath, MAX_PATH);
    wcscat(TestPath, L".local");
    DeleteFileW(TestPath);
#endif
}