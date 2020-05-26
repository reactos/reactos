/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGetFullPathName_UstrEx
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/*
NTSTATUS
NTAPI
RtlGetFullPathName_UstrEx(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING StaticString,
    IN PUNICODE_STRING DynamicString,
    IN PUNICODE_STRING *StringUsed,
    IN PSIZE_T FilePartSize OPTIONAL,
    OUT PBOOLEAN NameInvalid,
    OUT RTL_PATH_TYPE* PathType,
    OUT PSIZE_T LengthNeeded OPTIONAL
);
*/

NTSTATUS
(NTAPI
*pRtlGetFullPathName_UstrEx)(
    IN PUNICODE_STRING FileName,
    IN PUNICODE_STRING StaticString,
    IN PUNICODE_STRING DynamicString,
    IN PUNICODE_STRING *StringUsed,
    IN PSIZE_T FilePartSize OPTIONAL,
    OUT PBOOLEAN NameInvalid,
    OUT RTL_PATH_TYPE* PathType,
    OUT PSIZE_T LengthNeeded OPTIONAL
);

#define ok_eq_ustr(str1, str2) do {                                                     \
        ok((str1)->Buffer        == (str2)->Buffer,        "Buffer modified\n");        \
        ok((str1)->Length        == (str2)->Length,        "Length modified\n");        \
        ok((str1)->MaximumLength == (str2)->MaximumLength, "MaximumLength modified\n"); \
    } while (0)

static
BOOLEAN
CheckStringBuffer(
    PCUNICODE_STRING String,
    PCWSTR Expected)
{
    SIZE_T ExpectedLength = wcslen(Expected) * sizeof(WCHAR);
    SIZE_T EqualLength;
    BOOLEAN Result = TRUE;
    SIZE_T i;

    if (String->Length != ExpectedLength)
    {
        ok(0, "String length is %u, expected %lu\n", String->Length, (ULONG)ExpectedLength);
        Result = FALSE;
    }

    EqualLength = RtlCompareMemory(String->Buffer, Expected, ExpectedLength);
    if (EqualLength != ExpectedLength)
    {
        ok(0, "String is '%wZ', expected '%S'\n", String, Expected);
        Result = FALSE;
    }

    if (String->Buffer[String->Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        ok(0, "Not null terminated\n");
        Result = FALSE;
    }

    /* the function nulls the rest of the buffer! */
    for (i = String->Length + sizeof(UNICODE_NULL); i < String->MaximumLength; i++)
    {
        UCHAR Char = ((PUCHAR)String->Buffer)[i];
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
    {
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    }
    return TRUE;
}

#define RtlPathTypeNotSet 123

/* winetest_platform is "windows" for us, so broken() doesn't do what it should :( */
#undef broken
#define broken(x) 0

typedef enum
{
    PrefixNone,
    PrefixCurrentDrive,
    PrefixCurrentPath,
    PrefixCurrentPathWithoutLastPart
} PREFIX_TYPE;

static
VOID
RunTestCases(VOID)
{
    /* TODO: don't duplicate this in the other tests */
    /* TODO: Drive Relative tests don't work yet if the current drive isn't C: */
    struct
    {
        ULONG Line;
        PCWSTR FileName;
        PREFIX_TYPE PrefixType;
        PCWSTR FullPathName;
        RTL_PATH_TYPE PathType;
        PREFIX_TYPE FilePartPrefixType;
        SIZE_T FilePartSize;
    } TestCases[] =
    {
//        { __LINE__, L"C:",                 PrefixCurrentPath, L"", RtlPathTypeDriveRelative, PrefixCurrentPathWithoutLastPart }, // This is broken
        { __LINE__, L"C:\\",               PrefixNone, L"C:\\", RtlPathTypeDriveAbsolute },
        { __LINE__, L"C:\\test",           PrefixNone, L"C:\\test", RtlPathTypeDriveAbsolute, PrefixCurrentDrive },
        { __LINE__, L"C:\\test\\",         PrefixNone, L"C:\\test\\", RtlPathTypeDriveAbsolute },
        { __LINE__, L"C:/test/",           PrefixNone, L"C:\\test\\", RtlPathTypeDriveAbsolute },

        { __LINE__, L"C:\\\\test",         PrefixNone, L"C:\\test", RtlPathTypeDriveAbsolute, PrefixCurrentDrive },
        { __LINE__, L"test",               PrefixCurrentPath, L"\\test", RtlPathTypeRelative, PrefixCurrentPath, sizeof(WCHAR) },
        { __LINE__, L"\\test",             PrefixCurrentDrive, L"test", RtlPathTypeRooted, PrefixCurrentDrive },
        { __LINE__, L"/test",              PrefixCurrentDrive, L"test", RtlPathTypeRooted, PrefixCurrentDrive },
        { __LINE__, L".\\test",            PrefixCurrentPath, L"\\test", RtlPathTypeRelative, PrefixCurrentPath, sizeof(WCHAR) },

        { __LINE__, L"\\.",                PrefixCurrentDrive, L"", RtlPathTypeRooted },
        { __LINE__, L"\\.\\",              PrefixCurrentDrive, L"", RtlPathTypeRooted },
        { __LINE__, L"\\\\.",              PrefixNone, L"\\\\.\\", RtlPathTypeRootLocalDevice },
        { __LINE__, L"\\\\.\\",            PrefixNone, L"\\\\.\\", RtlPathTypeLocalDevice },
        { __LINE__, L"\\\\.\\Something\\", PrefixNone, L"\\\\.\\Something\\", RtlPathTypeLocalDevice },

        { __LINE__, L"\\??\\",             PrefixCurrentDrive, L"??\\", RtlPathTypeRooted },
        { __LINE__, L"\\??\\C:",           PrefixCurrentDrive, L"??\\C:", RtlPathTypeRooted, PrefixCurrentDrive, 3 * sizeof(WCHAR) },
        { __LINE__, L"\\??\\C:\\",         PrefixCurrentDrive, L"??\\C:\\", RtlPathTypeRooted },
        { __LINE__, L"\\??\\C:\\test",     PrefixCurrentDrive, L"??\\C:\\test", RtlPathTypeRooted, PrefixCurrentDrive, 6 * sizeof(WCHAR) },
        { __LINE__, L"\\??\\C:\\test\\",   PrefixCurrentDrive, L"??\\C:\\test\\", RtlPathTypeRooted },

        { __LINE__, L"\\\\??\\",           PrefixNone, L"\\\\??\\", RtlPathTypeUncAbsolute },
        { __LINE__, L"\\\\??\\C:",         PrefixNone, L"\\\\??\\C:", RtlPathTypeUncAbsolute },
        { __LINE__, L"\\\\??\\C:\\",       PrefixNone, L"\\\\??\\C:\\", RtlPathTypeUncAbsolute },
        { __LINE__, L"\\\\??\\C:\\test",   PrefixNone, L"\\\\??\\C:\\test", RtlPathTypeUncAbsolute, PrefixNone, sizeof(L"\\\\??\\C:\\") },
        { __LINE__, L"\\\\??\\C:\\test\\", PrefixNone, L"\\\\??\\C:\\test\\", RtlPathTypeUncAbsolute },
    };
    NTSTATUS Status;
    UNICODE_STRING FileName;
    UNICODE_STRING FullPathName;
    WCHAR FullPathNameBuffer[MAX_PATH];
    UNICODE_STRING TempString;
    PUNICODE_STRING StringUsed;
    SIZE_T FilePartSize;
    BOOLEAN NameInvalid;
    RTL_PATH_TYPE PathType;
    SIZE_T LengthNeeded;
    WCHAR ExpectedPathName[MAX_PATH];
    SIZE_T ExpectedFilePartSize;
    const INT TestCount = sizeof(TestCases) / sizeof(TestCases[0]);
    INT i;
    BOOLEAN Okay;

    for (i = 0; i < TestCount; i++)
    {
        switch (TestCases[i].PrefixType)
        {
            case PrefixNone:
                ExpectedPathName[0] = UNICODE_NULL;
                break;
            case PrefixCurrentDrive:
                GetCurrentDirectoryW(sizeof(ExpectedPathName) / sizeof(WCHAR), ExpectedPathName);
                ExpectedPathName[3] = UNICODE_NULL;
                break;
            case PrefixCurrentPath:
            {
                ULONG Length;
                Length = GetCurrentDirectoryW(sizeof(ExpectedPathName) / sizeof(WCHAR), ExpectedPathName);
                if (Length == 3 && TestCases[i].FullPathName[0])
                    ExpectedPathName[2] = UNICODE_NULL;
                break;
            }
            default:
                skip("Invalid test!\n");
                continue;
        }
        wcscat(ExpectedPathName, TestCases[i].FullPathName);
        RtlInitUnicodeString(&FileName, TestCases[i].FileName);
        RtlInitEmptyUnicodeString(&FullPathName, FullPathNameBuffer, sizeof(FullPathNameBuffer));
        RtlFillMemory(FullPathName.Buffer, FullPathName.MaximumLength, 0xAA);
        TempString = FileName;
        PathType = RtlPathTypeNotSet;
        StringUsed = InvalidPointer;
        FilePartSize = 1234;
        NameInvalid = (BOOLEAN)-1;
        LengthNeeded = 1234;
        StartSeh()
            Status = pRtlGetFullPathName_UstrEx(&FileName,
                                               &FullPathName,
                                               NULL,
                                               &StringUsed,
                                               &FilePartSize,
                                               &NameInvalid,
                                               &PathType,
                                               &LengthNeeded);
            ok(Status == STATUS_SUCCESS, "Line %lu: status = %lx\n", TestCases[i].Line, Status);
        EndSeh(STATUS_SUCCESS);
        ok_eq_ustr(&FileName, &TempString);
        ok(FullPathName.Buffer        == FullPathNameBuffer,         "Line %lu: Buffer modified\n", TestCases[i].Line);
        ok(FullPathName.MaximumLength == sizeof(FullPathNameBuffer), "Line %lu: MaximumLength modified\n", TestCases[i].Line);
        Okay = CheckStringBuffer(&FullPathName, ExpectedPathName);
        ok(Okay, "Line %lu: Wrong path name '%wZ', expected '%S'\n", TestCases[i].Line, &FullPathName, ExpectedPathName);
        ok(StringUsed == &FullPathName, "Line %lu: StringUsed = %p, expected %p\n", TestCases[i].Line, StringUsed, &FullPathName);
        switch (TestCases[i].FilePartPrefixType)
        {
            case PrefixNone:
                ExpectedFilePartSize = 0;
                break;
            case PrefixCurrentDrive:
                ExpectedFilePartSize = sizeof(L"C:\\");
                break;
            case PrefixCurrentPath:
                ExpectedFilePartSize = GetCurrentDirectoryW(0, NULL) * sizeof(WCHAR);
                if (ExpectedFilePartSize == sizeof(L"C:\\"))
                    ExpectedFilePartSize -= sizeof(WCHAR);
                break;
            case PrefixCurrentPathWithoutLastPart:
            {
                WCHAR CurrentPath[MAX_PATH];
                PCWSTR BackSlash;
                ExpectedFilePartSize = GetCurrentDirectoryW(sizeof(CurrentPath) / sizeof(WCHAR), CurrentPath) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
                if (ExpectedFilePartSize == sizeof(L"C:\\"))
                    ExpectedFilePartSize = 0;
                else
                {
                    BackSlash = wcsrchr(CurrentPath, L'\\');
                    if (BackSlash)
                        ExpectedFilePartSize -= wcslen(BackSlash + 1) * sizeof(WCHAR);
                    else
                        ok(0, "Line %lu: GetCurrentDirectory returned %S\n", TestCases[i].Line, CurrentPath);
                }
                break;
            }
            default:
                skip("Invalid test!\n");
                continue;
        }
        ExpectedFilePartSize += TestCases[i].FilePartSize;
        if (ExpectedFilePartSize != 0)
            ExpectedFilePartSize = (ExpectedFilePartSize - sizeof(UNICODE_NULL)) / sizeof(WCHAR);
        ok(FilePartSize == ExpectedFilePartSize,
            "Line %lu: FilePartSize = %lu, expected %lu\n", TestCases[i].Line, (ULONG)FilePartSize, (ULONG)ExpectedFilePartSize);
        ok(NameInvalid == FALSE, "Line %lu: NameInvalid = %u\n", TestCases[i].Line, NameInvalid);
        ok(PathType == TestCases[i].PathType, "Line %lu: PathType = %d, expected %d\n", TestCases[i].Line, PathType, TestCases[i].PathType);
        ok(LengthNeeded == 0, "Line %lu: LengthNeeded = %lu\n", TestCases[i].Line, (ULONG)LengthNeeded);
    }
}

START_TEST(RtlGetFullPathName_UstrEx)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    UNICODE_STRING TempString;
    UNICODE_STRING StaticString;
    PUNICODE_STRING StringUsed;
    SIZE_T FilePartSize;
    BOOLEAN NameInvalid;
    BOOLEAN NameInvalidArray[sizeof(ULONGLONG)];
    RTL_PATH_TYPE PathType;
    SIZE_T LengthNeeded;
    BOOLEAN Okay;

    pRtlGetFullPathName_UstrEx = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlGetFullPathName_UstrEx");
    if (!pRtlGetFullPathName_UstrEx)
    {
        skip("RtlGetFullPathName_UstrEx unavailable\n");
        return;
    }

    /* NULL parameters */
    StartSeh()
        pRtlGetFullPathName_UstrEx(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    RtlInitUnicodeString(&FileName, NULL);
    TempString = FileName;
    StartSeh()
        pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_ustr(&FileName, &TempString);

    RtlInitUnicodeString(&FileName, L"");
    TempString = FileName;
    StartSeh()
        pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_ustr(&FileName, &TempString);

    PathType = RtlPathTypeNotSet;
    StartSeh()
        pRtlGetFullPathName_UstrEx(NULL, NULL, NULL, NULL, NULL, NULL, &PathType, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok(PathType == RtlPathTypeNotSet, "PathType = %d\n", PathType);

    /* Check what else is initialized before it crashes */
    PathType = RtlPathTypeNotSet;
    StringUsed = InvalidPointer;
    FilePartSize = 1234;
    NameInvalid = (BOOLEAN)-1;
    LengthNeeded = 1234;
    StartSeh()
        pRtlGetFullPathName_UstrEx(NULL, NULL, NULL, &StringUsed, &FilePartSize, &NameInvalid, &PathType, &LengthNeeded);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok(StringUsed == NULL, "StringUsed = %p\n", StringUsed);
    ok(FilePartSize == 0, "FilePartSize = %lu\n", (ULONG)FilePartSize);
    ok(NameInvalid == FALSE, "NameInvalid = %u\n", NameInvalid);
    ok(PathType == RtlPathTypeNotSet, "PathType = %d\n", PathType);
    ok(LengthNeeded == 0, "LengthNeeded = %lu\n", (ULONG)LengthNeeded);

    RtlInitUnicodeString(&FileName, L"");
    TempString = FileName;
    StringUsed = InvalidPointer;
    FilePartSize = 1234;
    NameInvalid = (BOOLEAN)-1;
    LengthNeeded = 1234;
    StartSeh()
        pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, &StringUsed, &FilePartSize, &NameInvalid, NULL, &LengthNeeded);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_ustr(&FileName, &TempString);
    ok(StringUsed == NULL, "StringUsed = %p\n", StringUsed);
    ok(FilePartSize == 0, "FilePartSize = %lu\n", (ULONG)FilePartSize);
    ok(NameInvalid == (BOOLEAN)-1, "NameInvalid = %u\n", NameInvalid);
    ok(LengthNeeded == 0, "LengthNeeded = %lu\n", (ULONG)LengthNeeded);

    /* This is the first one that doesn't crash. FileName and PathType cannot be NULL */
    RtlInitUnicodeString(&FileName, NULL);
    TempString = FileName;
    PathType = RtlPathTypeNotSet;
    StartSeh()
        Status = pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NULL, &PathType, NULL);
        ok(Status == STATUS_OBJECT_NAME_INVALID, "status = %lx\n", Status);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileName, &TempString);
    ok(PathType == RtlPathTypeUnknown, "PathType = %d\n", PathType);

    RtlInitUnicodeString(&FileName, L"");
    TempString = FileName;
    PathType = RtlPathTypeNotSet;
    StartSeh()
        Status = pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NULL, &PathType, NULL);
        ok(Status == STATUS_OBJECT_NAME_INVALID, "status = %lx\n", Status);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileName, &TempString);
    ok(PathType == RtlPathTypeUnknown, "PathType = %d\n", PathType);

    /* Show that NameInvalid is indeed BOOLEAN */
    RtlInitUnicodeString(&FileName, L"");
    TempString = FileName;
    PathType = RtlPathTypeNotSet;
    RtlFillMemory(NameInvalidArray, sizeof(NameInvalidArray), 0x55);
    StartSeh()
        Status = pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NameInvalidArray, &PathType, NULL);
        ok(Status == STATUS_OBJECT_NAME_INVALID, "status = %lx\n", Status);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileName, &TempString);
    ok(PathType == RtlPathTypeUnknown, "PathType = %d\n", PathType);
    ok(NameInvalidArray[0] == FALSE, "NameInvalid = %u\n", NameInvalidArray[0]);
    Okay = CheckBuffer(NameInvalidArray + 1, sizeof(NameInvalidArray) - sizeof(NameInvalidArray[0]), 0x55);
    ok(Okay, "CheckBuffer failed\n");

    /* Give it a valid path */
    RtlInitUnicodeString(&FileName, L"C:\\test");
    TempString = FileName;
    PathType = RtlPathTypeNotSet;
    StartSeh()
        Status = pRtlGetFullPathName_UstrEx(&FileName, NULL, NULL, NULL, NULL, NULL, &PathType, NULL);
        ok(Status == STATUS_BUFFER_TOO_SMALL, "status = %lx\n", Status);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileName, &TempString);
    ok(PathType == RtlPathTypeDriveAbsolute, "PathType = %d\n", PathType);

    /* Zero-length static string */
    RtlInitUnicodeString(&FileName, L"C:\\test");
    TempString = FileName;
    RtlInitUnicodeString(&StaticString, NULL);
    PathType = RtlPathTypeNotSet;
    StartSeh()
        Status = pRtlGetFullPathName_UstrEx(&FileName, &StaticString, NULL, NULL, NULL, NULL, &PathType, NULL);
        ok(Status == STATUS_BUFFER_TOO_SMALL, "status = %lx\n", Status);
    EndSeh(STATUS_SUCCESS);
    ok_eq_ustr(&FileName, &TempString);
    ok(PathType == RtlPathTypeDriveAbsolute, "PathType = %d\n", PathType);

    /* TODO: play around with StaticString and DynamicString */

    /* Check the actual functionality with different paths */
    RunTestCases();
}
