/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosSearchPath_U
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/*
ULONG
NTAPI
RtlDosSearchPath_U(
    IN PCWSTR Path,
    IN PCWSTR FileName,
    IN PCWSTR Extension,
    IN ULONG BufferSize,
    OUT PWSTR Buffer,
    OUT PWSTR *PartName
);
*/

#define PrintablePointer(p) ((p) == InvalidPointer ? NULL : (p))

static
BOOLEAN
CheckStringBuffer(
    PCWSTR Buffer,
    SIZE_T Length,
    SIZE_T MaximumLength,
    PCWSTR Expected)
{
    SIZE_T ExpectedLength = wcslen(Expected) * sizeof(WCHAR);
    SIZE_T EqualLength;
    BOOLEAN Result = TRUE;
    SIZE_T i;

    if (Length != ExpectedLength)
    {
        ok(0, "String length is %lu, expected %lu\n", (ULONG)Length, (ULONG)ExpectedLength);
        Result = FALSE;
    }

    EqualLength = RtlCompareMemory(Buffer, Expected, Length);
    if (EqualLength != Length)
    {
        ok(0, "String is '%S', expected '%S'\n", Buffer, Expected);
        Result = FALSE;
    }

    if (Buffer[Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        ok(0, "Not null terminated\n");
        Result = FALSE;
    }

    /* the function nulls the rest of the buffer! */
    for (i = Length + sizeof(UNICODE_NULL); i < MaximumLength; i++)
    {
        UCHAR Char = ((PUCHAR)Buffer)[i];
        if (Char != 0)
        {
            ok(0, "Found 0x%x at offset %lu, expected 0x%x\n", Char, (ULONG)i, 0);
            /* don't count this as a failure unless the string was actually wrong */
            //Result = FALSE;
            /* don't flood the log */
            break;
        }
    }

    return Result;
}

static
BOOLEAN
CheckBuffer(
    PVOID Buffer,
    SIZE_T Size,
    UCHAR Value)
{
    PUCHAR Array = Buffer;
    SIZE_T i;

    for (i = 0; i < Size; i++)
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    return TRUE;
}

static
VOID
RunTestCases(
    PCWSTR CustomPath)
{
    struct
    {
        PCWSTR SearchPath;
        PCWSTR FileName;
        PCWSTR Extension;
        PCWSTR ResultPath;
        PCWSTR ResultFileName;
    } Tests[] =
    {
        { L"",                 L"",                     NULL,    NULL,                           NULL },
        { L"C:\\%ls\\Folder1", L"File1",                NULL,    L"C:\\%ls\\Folder1\\",          L"File1" },
        /* No path: current directory */
        { L"",                 L"File1",                NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        /* Full path as FileName */
        { L"",                 L"C:\\",                 NULL,    L"C:\\",                        NULL },
        { L"",                 L"C:\\%ls\\Folder1",     NULL,    L"C:\\%ls\\",                   L"Folder1" },
        /* No FileName */
        { L"C:\\",             L"",                     NULL,    L"C:\\",                        NULL },
        { L"C:\\%ls\\Folder1", L"",                     NULL,    L"C:\\%ls\\Folder1\\",          NULL },
        /* Full path as FileName */
        { L"", L"C:\\%ls\\Folder1\\SomeProgram.exe",    NULL,    L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"", L"C:\\%ls\\Folder1\\SomeProgram.exe",    L".exe", L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"", L"C:\\%ls\\Folder1\\SomeProgram",        NULL,    NULL,                           NULL },
        // 10
        { L"", L"C:\\%ls\\Folder1\\SomeProgram",        L".exe", NULL,                           NULL },
        /* Both SearchPath and FileName */
        { L"C:\\%ls\\Folder1\\", L"SomeProgram.exe",    NULL,    L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"C:\\%ls\\Folder1\\", L"SomeProgram.exe",    L".exe", L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"C:\\%ls\\Folder1\\", L"SomeProgram",        NULL,    NULL,                           NULL },
        { L"C:\\%ls\\Folder1\\", L"SomeProgram",        L".exe", L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"C:\\%ls\\Folder1",   L"SomeProgram.exe",    NULL,    L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"C:\\%ls\\Folder1",   L"SomeProgram.exe",    L".exe", L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        { L"C:\\%ls\\Folder1",   L"SomeProgram",        NULL,    NULL,                           NULL },
        { L"C:\\%ls\\Folder1",   L"SomeProgram",        L".exe", L"C:\\%ls\\Folder1\\",          L"SomeProgram.exe" },
        /* Full path to file in SearchPath doesn't work */
        { L"C:\\%ls\\Folder1\\SomeProgram.exe", L"",    NULL,    NULL,                           NULL },
        // 20
        { L"C:\\%ls\\Folder1\\SomeProgram.exe", L"",    L".exe", NULL,                           NULL },
        { L"C:\\%ls\\Folder1\\SomeProgram",     L"",    NULL,    NULL,                           NULL },
        { L"C:\\%ls\\Folder1\\SomeProgram",     L"",    L".exe", NULL,                           NULL },
        /* */
        { L"C:\\%ls\\Folder1",          L"File1",       NULL,    L"C:\\%ls\\Folder1\\",          L"File1" },
        { L"C:\\%ls\\CurrentDirectory", L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L"C:\\%ls\\Folder1 ",         L"File1",       NULL,    NULL,                           NULL },
        { L"C:\\%ls\\CurrentDirectory ",L"File1",       NULL,    NULL,                           NULL },
        { L" C:\\%ls\\Folder1",         L"File1",       NULL,    NULL,                           NULL },
        { L" C:\\%ls\\CurrentDirectory",L"File1",       NULL,    NULL,                           NULL },
        { L" C:\\%ls\\Folder1 ",        L"File1",       NULL,    NULL,                           NULL },
        // 30
        { L" C:\\%ls\\CurrentDirectory ",L"File1",      NULL,    NULL,                           NULL },
        /* Multiple search paths */
        { L"C:\\%ls\\Folder1;C:\\%ls\\CurrentDirectory",
                                        L"File1",       NULL,    L"C:\\%ls\\Folder1\\",          L"File1" },
        { L"C:\\%ls\\CurrentDirectory;C:\\%ls\\Folder1",
                                        L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L"C:\\%ls\\CurrentDirectory ; C:\\%ls\\Folder1",
                                        L"File1",       NULL,    NULL,                           NULL },
        { L"C:\\%ls\\CurrentDirectory ;C:\\%ls\\Folder1",
                                        L"File1",       NULL,    L"C:\\%ls\\Folder1\\",          L"File1" },
        { L"C:\\%ls\\CurrentDirectory; C:\\%ls\\Folder1",
                                        L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L";C:\\%ls\\Folder1",         L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L";C:\\%ls\\Folder1;",        L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L";C:\\%ls\\Folder1;",        L"File1",       NULL,    L"C:\\%ls\\CurrentDirectory\\", L"File1" },
        { L"C:\\%ls\\Folder1",          L"OnlyInCurr",  NULL,    NULL,                           NULL },
        // 40
        { L"",                          L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"",                          L"OnlyInCurr ", NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"",                          L" OnlyInCurr", NULL,    NULL,                           NULL },
        { L" ",                         L"OnlyInCurr",  NULL,    NULL,                           NULL },
        { L";",                         L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"; ",                        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L" ;",                        L"OnlyInCurr",  NULL,    NULL,                           NULL },
        { L" ; ",                       L"OnlyInCurr",  NULL,    NULL,                           NULL },
        { L";C:\\%ls\\Folder1",         L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"C:\\%ls\\Folder1;",         L"OnlyInCurr",  NULL,    NULL,                           NULL },
        // 50
        { L"C:\\%ls\\Folder1;;",        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L";C:\\%ls\\Folder1;",        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"C:\\%ls\\Folder1;C:\\%ls\\Folder2",
                                        L"OnlyInCurr",  NULL,    NULL,                           NULL },
        { L";C:\\%ls\\Folder1;C:\\%ls\\Folder2",
                                        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"C:\\%ls\\Folder1;;C:\\%ls\\Folder2",
                                        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        { L"C:\\%ls\\Folder1;C:\\%ls\\Folder2;",
                                        L"OnlyInCurr",  NULL,    NULL,                           NULL },
        { L"C:\\%ls\\Folder1;C:\\%ls\\Folder2;;",
                                        L"OnlyInCurr",  NULL,    L"C:\\%ls\\CurrentDirectory\\", L"OnlyInCurr" },
        /* Spaces in FileName! */
        { L"", L"C:\\%ls\\Folder1\\SomeProgram With Spaces",
                                                        L".exe", NULL,                           NULL },
        { L"", L"C:\\%ls\\Folder1\\SomeProgram With Spaces.exe",
                                                        L".exe", NULL,                           NULL },
        { L"", L"C:\\%ls\\Folder1\\Program",            L".exe", NULL,                           NULL },
        // 60
        { L"", L"C:\\%ls\\Folder1\\Program.exe",        L".exe", L"C:\\%ls\\Folder1\\",          L"Program.exe" },
        { L"", L"C:\\%ls\\Folder1\\Program With",       L".exe", NULL,                           NULL },
        { L"", L"C:\\%ls\\Folder1\\Program With.exe",   L".exe", L"C:\\%ls\\Folder1\\",          L"Program With.exe" },
        { L"", L"C:\\%ls\\Folder1\\Program With Spaces",L".exe", NULL,                           NULL },
        { L"", L"C:\\%ls\\Folder1\\Program With Spaces.exe",
                                                        L".exe", L"C:\\%ls\\Folder1\\",          L"Program With Spaces.exe" },
        /* Same tests with path in SearchPath - now extensions are appended */
        { L"C:\\%ls\\Folder1", L"SomeProgram With Spaces",
                                                        L".exe", NULL,                           NULL },
        { L"C:\\%ls\\Folder1", L"SomeProgram With Spaces.exe",
                                                        L".exe", NULL,                           NULL },
        { L"C:\\%ls\\Folder1", L"Program",              L".exe", L"C:\\%ls\\Folder1\\",          L"Program.exe" },
        { L"C:\\%ls\\Folder1", L"Program.exe",          L".exe", L"C:\\%ls\\Folder1\\",          L"Program.exe" },
        { L"C:\\%ls\\Folder1", L"Program With",         L".exe", L"C:\\%ls\\Folder1\\",          L"Program With.exe" },
        // 70
        { L"C:\\%ls\\Folder1", L"Program With.exe",     L".exe", L"C:\\%ls\\Folder1\\",          L"Program With.exe" },
        { L"C:\\%ls\\Folder1", L"Program With Spaces",  L".exe", L"C:\\%ls\\Folder1\\",          L"Program With Spaces.exe" },
        { L"C:\\%ls\\Folder1", L"Program With Spaces.exe",
                                                        L".exe", L"C:\\%ls\\Folder1\\",          L"Program With Spaces.exe" },
    };

    ULONG i;
    ULONG Length;
    PWSTR PartName;
    WCHAR SearchPath[MAX_PATH];
    WCHAR FileName[MAX_PATH];
    WCHAR ResultPath[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    BOOLEAN Okay;

    for (i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
    {
        swprintf(SearchPath, Tests[i].SearchPath, CustomPath, CustomPath, CustomPath, CustomPath);
        swprintf(FileName, Tests[i].FileName, CustomPath, CustomPath, CustomPath, CustomPath);
        RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
        PartName = InvalidPointer;

        StartSeh()
            Length = RtlDosSearchPath_U(SearchPath,
                                        FileName,
                                        Tests[i].Extension,
                                        sizeof(Buffer),
                                        Buffer,
                                        &PartName);
        EndSeh(STATUS_SUCCESS);

        if (Tests[i].ResultPath)
        {
            swprintf(ResultPath, Tests[i].ResultPath, CustomPath, CustomPath, CustomPath, CustomPath);
            if (Tests[i].ResultFileName)
            {
                ok(PartName == &Buffer[wcslen(ResultPath)],
                   "PartName = %p (%ls), expected %p\n",
                   PartName, PrintablePointer(PartName), &Buffer[wcslen(ResultPath)]);
                wcscat(ResultPath, Tests[i].ResultFileName);
            }
            else
            {
                ok(PartName == NULL,
                   "PartName = %p (%ls), expected NULL\n",
                   PartName, PrintablePointer(PartName));
            }
            Okay = CheckStringBuffer(Buffer, Length, sizeof(Buffer), ResultPath);
            ok(Okay == TRUE, "CheckStringBuffer failed. Got '%ls', expected '%ls'\n", Buffer, ResultPath);
        }
        else
        {
            Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
            ok(Okay == TRUE, "CheckBuffer failed\n");
            ok(Length == 0, "Length = %lu\n", Length);
            ok(PartName == InvalidPointer,
               "PartName = %p (%ls), expected %p\n",
               PartName, PrintablePointer(PartName), InvalidPointer);
        }
    }
}

#define MAKE_DIRECTORY(path)                                                \
do {                                                                        \
    swprintf(FileName, path, CustomPath);                                   \
    Success = CreateDirectoryW(FileName, NULL);                             \
    ok(Success, "CreateDirectory failed, results might not be accurate\n"); \
} while (0)

#define MAKE_FILE(path)                                                     \
do {                                                                        \
    swprintf(FileName, path, CustomPath);                                   \
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);        \
    ok(Handle != INVALID_HANDLE_VALUE,                                      \
       "CreateFile failed, results might not be accurate\n");               \
    if (Handle != INVALID_HANDLE_VALUE) CloseHandle(Handle);                \
} while (0)

#define DELETE_DIRECTORY(path)                                              \
do {                                                                        \
    swprintf(FileName, path, CustomPath);                                   \
    Success = RemoveDirectoryW(FileName);                                   \
    ok(Success,                                                             \
       "RemoveDirectory failed (%lu), test might leave stale directory\n",  \
       GetLastError());                                                     \
} while (0)

#define DELETE_FILE(path)                                                   \
do {                                                                        \
    swprintf(FileName, path, CustomPath);                                   \
    Success = DeleteFileW(FileName);                                        \
    ok(Success,                                                             \
       "DeleteFile failed (%lu), test might leave stale file\n",            \
       GetLastError());                                                     \
} while (0)

START_TEST(RtlDosSearchPath_U)
{
    ULONG Length = 0;
    WCHAR Buffer[MAX_PATH];
    PWSTR PartName;
    BOOLEAN Okay;
    BOOL Success;
    WCHAR FileName[MAX_PATH];
    WCHAR CustomPath[MAX_PATH] = L"RtlDosSearchPath_U_TestPath";
    HANDLE Handle;

    swprintf(FileName, L"C:\\%ls", CustomPath);
    /* Make sure this directory doesn't exist */
    while (GetFileAttributesW(FileName) != INVALID_FILE_ATTRIBUTES)
    {
        wcscat(CustomPath, L"X");
        swprintf(FileName, L"C:\\%ls", CustomPath);
    }
    Success = CreateDirectoryW(FileName, NULL);
    ok(Success, "CreateDirectory failed, results might not be accurate\n");

    MAKE_DIRECTORY(L"C:\\%ls\\Folder1");
    MAKE_DIRECTORY(L"C:\\%ls\\Folder2");
    MAKE_DIRECTORY(L"C:\\%ls\\CurrentDirectory");
    Success = SetCurrentDirectoryW(FileName);
    ok(Success, "SetCurrentDirectory failed\n");
    MAKE_FILE(L"C:\\%ls\\Folder1\\File1");
    MAKE_FILE(L"C:\\%ls\\Folder1\\SomeProgram.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\SomeProgram2.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\SomeProgram2.exe.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\SomeProgram3.exe.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\Program.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\Program With.exe");
    MAKE_FILE(L"C:\\%ls\\Folder1\\Program With Spaces.exe");
    MAKE_FILE(L"C:\\%ls\\CurrentDirectory\\File1");
    MAKE_FILE(L"C:\\%ls\\CurrentDirectory\\OnlyInCurr");

    /* NULL parameters */
    StartSeh() RtlDosSearchPath_U(NULL, NULL, NULL, 0, NULL  , NULL); EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlDosSearchPath_U(NULL, L"" , NULL, 0, NULL  , NULL); EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlDosSearchPath_U(NULL, L"" , NULL, 0, Buffer, NULL); EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlDosSearchPath_U(NULL, L"" , NULL, 1, Buffer, NULL); EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlDosSearchPath_U(NULL, L"" , NULL, 2, Buffer, NULL); EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlDosSearchPath_U(L"" , NULL, NULL, 0, NULL  , NULL); EndSeh(STATUS_ACCESS_VIOLATION);

    /* Empty strings - first one that doesn't crash */
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"", NULL, 0, NULL, NULL);
        ok(Length == 0, "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);

    /* Check what's initialized */
    PartName = InvalidPointer;
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"", NULL, 0, NULL, &PartName);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(PartName == InvalidPointer, "PartName = %p\n", PartName);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"", NULL, sizeof(Buffer), Buffer, NULL);
        ok(Length == 0, "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    PartName = InvalidPointer;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"", NULL, sizeof(Buffer), Buffer, &PartName);
        ok(Length == 0, "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(PartName == InvalidPointer, "PartName = %p\n", PartName);
    Okay = CheckBuffer(Buffer, sizeof(Buffer), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    /* Now test the actual functionality */
    RunTestCases(CustomPath);

    /*
     * Clean up test folder - We can't delete it
     * if our current directory is inside.
     */
    SetCurrentDirectoryW(L"C:\\");
    DELETE_FILE(L"C:\\%ls\\CurrentDirectory\\OnlyInCurr");
    DELETE_FILE(L"C:\\%ls\\CurrentDirectory\\File1");
    DELETE_FILE(L"C:\\%ls\\Folder1\\Program With Spaces.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\Program With.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\Program.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\SomeProgram3.exe.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\SomeProgram2.exe.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\SomeProgram2.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\SomeProgram.exe");
    DELETE_FILE(L"C:\\%ls\\Folder1\\File1");
    DELETE_DIRECTORY(L"C:\\%ls\\CurrentDirectory");
    DELETE_DIRECTORY(L"C:\\%ls\\Folder2");
    DELETE_DIRECTORY(L"C:\\%ls\\Folder1");
    DELETE_DIRECTORY(L"C:\\%ls");
}
