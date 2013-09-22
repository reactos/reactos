/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDosSearchPath_U
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/rtlfuncs.h>

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
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists", CustomPath);
    Success = CreateDirectoryW(FileName, NULL);
    ok(Success, "CreateDirectory failed, results might not be accurate\n");
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\ThisFileExists", CustomPath);
    Handle = CreateFileW(FileName, 0, 0, NULL, CREATE_NEW, 0, NULL);
    ok(Handle != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (Handle != INVALID_HANDLE_VALUE)
        CloseHandle(Handle);

    /* NULL parameters */
    StartSeh() RtlDosSearchPath_U(NULL, NULL, NULL, 0, NULL, NULL);             EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() Length = RtlDosSearchPath_U(NULL, L"", NULL, 0, NULL, NULL);     EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() Length = RtlDosSearchPath_U(NULL, L"", NULL, 0, Buffer, NULL);   EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() Length = RtlDosSearchPath_U(NULL, L"", NULL, 1, Buffer, NULL);   EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() Length = RtlDosSearchPath_U(NULL, L"", NULL, 2, Buffer, NULL);   EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() Length = RtlDosSearchPath_U(L"", NULL, NULL, 0, NULL, NULL);     EndSeh(STATUS_ACCESS_VIOLATION);

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

    /* Empty path string searches in current directory */
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists", CustomPath);
    Success = SetCurrentDirectoryW(FileName);
    ok(Success, "SetCurrentDirectory failed\n");
    PartName = InvalidPointer;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"ThisFileExists", NULL, sizeof(Buffer), Buffer, &PartName);
        ok(Length == wcslen(FileName) * sizeof(WCHAR) + sizeof(L"\\ThisFileExists") - sizeof(UNICODE_NULL), "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(PartName == &Buffer[wcslen(FileName) + 1], "PartName = %p\n", PartName);
    wcscat(FileName, L"\\ThisFileExists");
    Okay = CheckStringBuffer(Buffer, Length, sizeof(Buffer), FileName);
    ok(Okay, "CheckStringBuffer failed\n");

    /* Absolute path in FileName is also okay */
    PartName = InvalidPointer;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    StartSeh()
        Length = RtlDosSearchPath_U(L"", L"C:\\", NULL, sizeof(Buffer), Buffer, &PartName);
        ok(Length == sizeof(L"C:\\") - sizeof(UNICODE_NULL), "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(PartName == NULL, "PartName = %p\n", PartName);
    Okay = CheckStringBuffer(Buffer, Length, sizeof(Buffer), L"C:\\");
    ok(Okay, "CheckStringBuffer failed\n");

    /* Empty FileName also works */
    PartName = InvalidPointer;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    StartSeh()
        Length = RtlDosSearchPath_U(L"C:\\", L"", NULL, sizeof(Buffer), Buffer, &PartName);
        ok(Length == sizeof(L"C:\\") - sizeof(UNICODE_NULL), "Length %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(PartName == NULL, "PartName = %p\n", PartName);
    Okay = CheckStringBuffer(Buffer, Length, sizeof(Buffer), L"C:\\");
    ok(Okay, "CheckStringBuffer failed\n");

    /* Clean up test folder */
    SetCurrentDirectoryW(L"C:\\");
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists\\ThisFileExists", CustomPath);
    Success = DeleteFileW(FileName);
    ok(Success, "DeleteFile failed, test might leave stale file\n");
    swprintf(FileName, L"C:\\%ls\\ThisFolderExists", CustomPath);
    Success = RemoveDirectoryW(FileName);
    ok(Success, "RemoveDirectory failed %(lu), test might leave stale directory\n", GetLastError());
    swprintf(FileName, L"C:\\%ls", CustomPath);
    Success = RemoveDirectoryW(FileName);
    ok(Success, "RemoveDirectory failed (%lu), test might leave stale directory\n", GetLastError());
}
