/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDoesFileExists_U*
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/rtlfuncs.h>

#define ok_bool_file(value, expected, file) do {                                \
        if (expected)                                                           \
            ok(value == TRUE, "File '%ls' should exist, but does not\n", file); \
        else                                                                    \
            ok(value == FALSE, "File '%ls' should not exist, but does\n", file);\
    } while (0)

#define ok_eq_ustr(str1, str2) do {                                                     \
        ok((str1)->Buffer        == (str2)->Buffer,        "Buffer modified\n");        \
        ok((str1)->Length        == (str2)->Length,        "Length modified\n");        \
        ok((str1)->MaximumLength == (str2)->MaximumLength, "MaximumLength modified\n"); \
    } while (0)

/*
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    IN PCWSTR FileName
);

BOOLEAN
NTAPI
RtlDoesFileExists_UEx(
    IN PCWSTR FileName,
    IN BOOLEAN SucceedIfBusy
);

BOOLEAN
NTAPI
RtlDoesFileExists_UStr(
    IN PUNICODE_STRING FileName
);

BOOLEAN
NTAPI
RtlDoesFileExists_UstrEx(
        IN PCUNICODE_STRING FileName,
        IN BOOLEAN SucceedIfBusy
);
*/

static
BOOLEAN
(NTAPI
*RtlDoesFileExists_UEx)(
    IN PCWSTR FileName,
    IN BOOLEAN SucceedIfBusy
)
//= (PVOID)0x7c8187d0 // 2003 sp1 x86
//= (PVOID)0x7769aeb2 // win7 sp1 wow64
;

static
BOOLEAN
(NTAPI
*RtlDoesFileExists_UStr)(
    IN PUNICODE_STRING FileName
)
//= (PVOID)0x7c8474e5 // 2003 sp1 x86
//= (PVOID)0x776ff304 // win7 sp1 wow64
;

static
BOOLEAN
(NTAPI
*RtlDoesFileExists_UstrEx)(
        IN PCUNICODE_STRING FileName,
        IN BOOLEAN SucceedIfBusy
)
//= (PVOID)0x7c830f89 // 2003 sp1 x86
//= (PVOID)0x7769addb // win7 sp1 wow64
;

START_TEST(RtlDoesFileExists)
{
    BOOLEAN Ret;
    struct
    {
        PCWSTR FileName;
        BOOLEAN Exists;
    } Tests[] =
    {
        { L"",                                          FALSE },
        { L"?",                                         FALSE },
        { L"*",                                         FALSE },
        { L":",                                         FALSE },
        { L";",                                         FALSE },
        { L"\"",                                        FALSE },
        { L".",                                         TRUE },
        { L"..",                                        TRUE },
        { L"/",                                         TRUE },
        { L"//",                                        FALSE },
        { L"///",                                       FALSE },
        { L"\\/",                                       FALSE },
        { L"\\//",                                      FALSE },
        { L"\\\\/",                                     FALSE },
        { L"\\/\\",                                     FALSE },
        { L"\\/\\\\",                                   FALSE },
        { L"\\/\\/\\",                                  FALSE },
        { L"\\",                                        TRUE },
        { L"\\\\",                                      FALSE },
        { L"\\\\\\",                                    FALSE },
        { L"\\\\.",                                     FALSE },
        { L"\\\\.\\",                                   FALSE },
        { L"\\\\.\\GLOBAL??",                           FALSE },
        { L"\\\\.\\GLOBAL??\\",                         FALSE },
        { L"\\\\?",                                     FALSE },
        { L"\\\\??",                                    FALSE },
        { L"\\\\??\\",                                  FALSE },
        { L"\\\\??\\C:\\",                              FALSE },
        { L"\\\\.",                                     FALSE },
        { L"\\\\.\\",                                   FALSE },
        { L"C:",                                        TRUE },
        { L"C:/",                                       TRUE },
        { L"C:/\\",                                     TRUE },
        { L"C:\\/",                                     TRUE },
        { L"C:\\/\\",                                   TRUE },
        { L"C://",                                      TRUE },
        { L"C:\\",                                      TRUE },
        { L"C:\\\\",                                    TRUE },
        { L"C:\\%ls",                                   TRUE },
        { L"C:/%ls",                                    TRUE },
        { L"C://%ls",                                   TRUE },
        { L"C:\\/%ls",                                  TRUE },
        { L"C:/\\%ls",                                  TRUE },
        { L"C:\\/\\%ls",                                TRUE },
        { L"C:\\%ls\\",                                 TRUE },
        { L"C:\\%ls\\ThisFolderExists",                 TRUE },
        { L"C:\\%ls\\ThisFolderExists\\",               TRUE },
        { L"C:\\%ls\\ThisFolderExists ",                TRUE },
        { L"C:\\%ls\\ThisFolderExists  ",               TRUE },
        { L"C:\\%ls\\ThisFolderExists   ",              TRUE },
        { L"C:\\%ls\\ThisFolderExists:",                FALSE },
        { L"C:\\%ls\\ThisFolderExists\t",               FALSE },
        { L"C:\\%ls\\ThisFolderExists\n",               FALSE },
        { L"C:\\%ls\\ThisFolderExists\r",               FALSE },
        { L" C:\\%ls\\ThisFolderExists",                FALSE },
        { L"C:\\%ls\\ ThisFolderExists",                FALSE },
        { L"C:\\%ls \\ThisFolderExists",                FALSE },
        { L"C:\\%ls\\ThisDoesntExist",                  FALSE },
        { L"C:\\\\%ls\\\\ThisFolderExists",             TRUE },
        { L"C:\\%ls\\ThisFolderExists\\ThisFileExists", TRUE },
        { L"c:\\%ls\\thisfolderexists\\thisfileexists", TRUE },
        { L"C:\\%ls\\THISFOLDEREXISTS\\THISFILEEXISTS", TRUE },
        { L"C:\\%ls\\ThisFolderExists\\SomeProgram.exe",TRUE },
        { L"C:\\%ls\\ThisFolderExists\\SomeProgram",    FALSE },
        { L"C:\\%ls\\ThisFolderExists\\With",           FALSE },
        { L"C:\\%ls\\ThisFolderExists\\With Space",     TRUE },
        { L"C:\\%ls\\ThisFolderExists\\Without",        TRUE },
        { L"C:\\%ls\\ThisFolderExists\\Without Space",  FALSE },
        { L"C:\\%ls abc",                               FALSE },
        { L"\"C:\\%ls\" abc",                           FALSE },
        { L"\"C:\\\"",                                  FALSE },
        { L"C:\\%ls;C:\\",                              FALSE },
        { L"%%SystemRoot%%",                            FALSE },
        { L"%%SystemRoot%%\\",                          FALSE },
        { L"%%SystemRoot%%\\system32",                  FALSE },
        { L"NUL",                                       FALSE },
        { L"CON",                                       FALSE },
        { L"COM1",                                      FALSE },
        { L"\\?",                                       FALSE },
        { L"\\??",                                      FALSE },
        { L"\\??\\",                                    FALSE },
        { L"\\??\\C",                                   FALSE },
        { L"\\??\\C:",                                  FALSE },
        { L"\\??\\C:\\",                                FALSE }, // TRUE on Win7
        { L"\\??\\C:\\%ls\\ThisFolderExists",           FALSE }, // TRUE on Win7
    };
    ULONG i;
    WCHAR FileName[MAX_PATH];
    WCHAR CustomPath[MAX_PATH] = L"RtlDoesFileExists_U_TestPath";
    BOOL Success;
    HANDLE Handle;

    if (!RtlDoesFileExists_UEx)
    {
        RtlDoesFileExists_UEx = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlDoesFileExists_UEx");
        if (!RtlDoesFileExists_UEx)
            skip("RtlDoesFileExists_UEx unavailable\n");
    }

    if (!RtlDoesFileExists_UStr)
    {
        RtlDoesFileExists_UStr = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlDoesFileExists_UStr");
        if (!RtlDoesFileExists_UStr)
            skip("RtlDoesFileExists_UStr unavailable\n");
    }

    if (!RtlDoesFileExists_UstrEx)
    {
        RtlDoesFileExists_UstrEx = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlDoesFileExists_UstrEx");
        if (!RtlDoesFileExists_UstrEx)
            skip("RtlDoesFileExists_UstrEx unavailable\n");
    }

    StartSeh()
        Ret = RtlDoesFileExists_U(NULL);
        ok(Ret == FALSE, "NULL file exists?!\n");
    EndSeh(STATUS_SUCCESS);

    if (RtlDoesFileExists_UEx)
    {
        StartSeh()
            Ret = RtlDoesFileExists_UEx(NULL, TRUE);
            ok(Ret == FALSE, "NULL file exists?!\n");
            Ret = RtlDoesFileExists_UEx(NULL, FALSE);
            ok(Ret == FALSE, "NULL file exists?!\n");
        EndSeh(STATUS_SUCCESS);
    }

    if (RtlDoesFileExists_UStr)
    {
        StartSeh() Ret = RtlDoesFileExists_UStr(NULL);      EndSeh(STATUS_ACCESS_VIOLATION);
    }

    if (RtlDoesFileExists_UstrEx)
    {
        StartSeh() RtlDoesFileExists_UstrEx(NULL, FALSE);   EndSeh(STATUS_ACCESS_VIOLATION);
        StartSeh() RtlDoesFileExists_UstrEx(NULL, TRUE);    EndSeh(STATUS_ACCESS_VIOLATION);
    }

    swprintf(FileName, L"C:\\%ls", CustomPath);
    /* Make sure this directory doesn't exist */
    while (GetFileAttributesW(FileName) != INVALID_FILE_ATTRIBUTES)
    {
        wcscat(CustomPath, L"X");
        swprintf(FileName, L"C:\\%ls", CustomPath);
    }
    Success = CreateDirectoryW(FileName, NULL);
    ok(Success, "CreateDirectory failed, results might not be accurate\n");
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists", CustomPath);
    Success = CreateDirectoryW(FileName, NULL);
    ok(Success, "CreateDirectory failed, results might not be accurate\n");
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\ThisFileExists", CustomPath);
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);
    ok(Handle != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (Handle != INVALID_HANDLE_VALUE)
    {
        /* Check SucceedIfBusy behavior */
        if (RtlDoesFileExists_UEx)
        {
            Ret = RtlDoesFileExists_UEx(FileName, TRUE);
            ok_bool_file(Ret, TRUE, FileName);
            /* TODO: apparently we have to do something worse to make this fail */
            Ret = RtlDoesFileExists_UEx(FileName, FALSE);
            ok_bool_file(Ret, TRUE, FileName);
        }
        if (RtlDoesFileExists_UstrEx)
        {
            UNICODE_STRING FileNameString;
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&FileNameString, FileName);
            TempString = FileNameString;
            Ret = RtlDoesFileExists_UstrEx(&FileNameString, TRUE);
            ok_eq_ustr(&FileNameString, &TempString);
            ok_bool_file(Ret, TRUE, FileName);
            /* TODO: apparently we have to do something worse to make this fail */
            Ret = RtlDoesFileExists_UstrEx(&FileNameString, FALSE);
            ok_eq_ustr(&FileNameString, &TempString);
            ok_bool_file(Ret, TRUE, FileName);
        }
        CloseHandle(Handle);
    }

    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\SomeProgram.exe", CustomPath);
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);
    ok(Handle != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle);

    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\With Space", CustomPath);
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);
    ok(Handle != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle);

    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\Without", CustomPath);
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);
    ok(Handle != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle);

    for (i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
    {
        swprintf(FileName, Tests[i].FileName, CustomPath);
        StartSeh()
            Ret = RtlDoesFileExists_U(FileName);
            ok_bool_file(Ret, Tests[i].Exists, FileName);
        EndSeh(STATUS_SUCCESS);
        if (RtlDoesFileExists_UEx)
        {
            StartSeh()
                Ret = RtlDoesFileExists_UEx(FileName, TRUE);
                ok_bool_file(Ret, Tests[i].Exists, FileName);
            EndSeh(STATUS_SUCCESS);
            StartSeh()
                Ret = RtlDoesFileExists_UEx(FileName, FALSE);
                ok_bool_file(Ret, Tests[i].Exists, FileName);
            EndSeh(STATUS_SUCCESS);
        }
        /* TODO: use guarded memory to make sure these don't touch the null terminator */
        if (RtlDoesFileExists_UStr)
        {
            UNICODE_STRING FileNameString;
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&FileNameString, FileName);
            TempString = FileNameString;
            StartSeh()
                Ret = RtlDoesFileExists_UStr(&FileNameString);
                ok_bool_file(Ret, Tests[i].Exists, FileName);
            EndSeh(STATUS_SUCCESS);
            ok_eq_ustr(&FileNameString, &TempString);
        }
        if (RtlDoesFileExists_UstrEx)
        {
            UNICODE_STRING FileNameString;
            UNICODE_STRING TempString;
            RtlInitUnicodeString(&FileNameString, FileName);
            TempString = FileNameString;
            StartSeh()
                Ret = RtlDoesFileExists_UstrEx(&FileNameString, TRUE);
                ok_bool_file(Ret, Tests[i].Exists, FileName);
            EndSeh(STATUS_SUCCESS);
            ok_eq_ustr(&FileNameString, &TempString);
            StartSeh()
                Ret = RtlDoesFileExists_UstrEx(&FileNameString, FALSE);
                ok_bool_file(Ret, Tests[i].Exists, FileName);
            EndSeh(STATUS_SUCCESS);
            ok_eq_ustr(&FileNameString, &TempString);
        }
    }

    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\Without", CustomPath);
    Success = DeleteFileW(FileName);
    ok(Success, "DeleteFile failed (%lu), test might leave stale file\n", GetLastError());
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\With Space", CustomPath);
    Success = DeleteFileW(FileName);
    ok(Success, "DeleteFile failed (%lu), test might leave stale file\n", GetLastError());
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\SomeProgram.exe", CustomPath);
    Success = DeleteFileW(FileName);
    ok(Success, "DeleteFile failed (%lu), test might leave stale file\n", GetLastError());

    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\ThisFileExists", CustomPath);
    Success = DeleteFileW(FileName);
    ok(Success, "DeleteFile failed (%lu), test might leave stale file\n", GetLastError());
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists", CustomPath);
    Success = RemoveDirectoryW(FileName);
    ok(Success, "RemoveDirectory failed (%lu), test might leave stale directory\n", GetLastError());
    swprintf(FileName, L"C:\\%ls", CustomPath);
    Success = RemoveDirectoryW(FileName);
    ok(Success, "RemoveDirectory failed (%lu), test might leave stale directory\n", GetLastError());
}
