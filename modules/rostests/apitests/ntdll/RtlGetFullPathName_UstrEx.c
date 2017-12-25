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

#if defined(_UNITY_BUILD_ENABLED_) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wformat"
#pragma GCC diagnostic warning "-Wformat-extra-args"
#endif

static
BOOLEAN
CheckUnicodeStringBuffer(
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

#define RtlPathTypeNotSet 123

/* winetest_platform is "windows" for us, so broken() doesn't do what it should :( */
#undef broken
#define broken(x) 0

static
VOID
RtlGetFullPathName_UstrEx_RunTestCases(VOID)
{
    /* TODO: don't duplicate this in the other tests */
    /* TODO: Drive Relative tests don't work yet if the current drive isn't C: */
    struct
    {
        PCWSTR FileName;
        PREFIX_TYPE PrefixType;
        PCWSTR FullPathName;
        RTL_PATH_TYPE PathType;
        PREFIX_TYPE FilePartPrefixType;
        SIZE_T FilePartSize;
    } TestCases[] =
    {
        { L"C:",                 PrefixCurrentPath, L"", RtlPathTypeDriveRelative, PrefixCurrentPathWithoutLastPart },
        { L"C:\\",               PrefixNone, L"C:\\", RtlPathTypeDriveAbsolute },
        { L"C:\\test",           PrefixNone, L"C:\\test", RtlPathTypeDriveAbsolute, PrefixCurrentDrive },
        { L"C:\\test\\",         PrefixNone, L"C:\\test\\", RtlPathTypeDriveAbsolute },
        { L"C:/test/",           PrefixNone, L"C:\\test\\", RtlPathTypeDriveAbsolute },

        { L"C:\\\\test",         PrefixNone, L"C:\\test", RtlPathTypeDriveAbsolute, PrefixCurrentDrive },
        { L"test",               PrefixCurrentPath, L"\\test", RtlPathTypeRelative, PrefixCurrentPath, sizeof(WCHAR) },
        { L"\\test",             PrefixCurrentDrive, L"test", RtlPathTypeRooted, PrefixCurrentDrive },
        { L"/test",              PrefixCurrentDrive, L"test", RtlPathTypeRooted, PrefixCurrentDrive },
        { L".\\test",            PrefixCurrentPath, L"\\test", RtlPathTypeRelative, PrefixCurrentPath, sizeof(WCHAR) },

        { L"\\.",                PrefixCurrentDrive, L"", RtlPathTypeRooted },
        { L"\\.\\",              PrefixCurrentDrive, L"", RtlPathTypeRooted },
        { L"\\\\.",              PrefixNone, L"\\\\.\\", RtlPathTypeRootLocalDevice },
        { L"\\\\.\\",            PrefixNone, L"\\\\.\\", RtlPathTypeLocalDevice },
        { L"\\\\.\\Something\\", PrefixNone, L"\\\\.\\Something\\", RtlPathTypeLocalDevice },

        { L"\\??\\",             PrefixCurrentDrive, L"??\\", RtlPathTypeRooted },
        { L"\\??\\C:",           PrefixCurrentDrive, L"??\\C:", RtlPathTypeRooted, PrefixCurrentDrive, 3 * sizeof(WCHAR) },
        { L"\\??\\C:\\",         PrefixCurrentDrive, L"??\\C:\\", RtlPathTypeRooted },
        { L"\\??\\C:\\test",     PrefixCurrentDrive, L"??\\C:\\test", RtlPathTypeRooted, PrefixCurrentDrive, 6 * sizeof(WCHAR) },
        { L"\\??\\C:\\test\\",   PrefixCurrentDrive, L"??\\C:\\test\\", RtlPathTypeRooted },

        { L"\\\\??\\",           PrefixNone, L"\\\\??\\", RtlPathTypeUncAbsolute },
        { L"\\\\??\\C:",         PrefixNone, L"\\\\??\\C:", RtlPathTypeUncAbsolute },
        { L"\\\\??\\C:\\",       PrefixNone, L"\\\\??\\C:\\", RtlPathTypeUncAbsolute },
        { L"\\\\??\\C:\\test",   PrefixNone, L"\\\\??\\C:\\test", RtlPathTypeUncAbsolute, PrefixNone, sizeof(L"\\\\??\\C:\\") },
        { L"\\\\??\\C:\\test\\", PrefixNone, L"\\\\??\\C:\\test\\", RtlPathTypeUncAbsolute },
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
            ok(Status == STATUS_SUCCESS, "status = %lx\n", Status);
        EndSeh(STATUS_SUCCESS);
        ok_eq_ustr(&FileName, &TempString);
        ok(FullPathName.Buffer        == FullPathNameBuffer,         "Buffer modified\n");
        ok(FullPathName.MaximumLength == sizeof(FullPathNameBuffer), "MaximumLength modified\n");
        Okay = CheckUnicodeStringBuffer(&FullPathName, ExpectedPathName);
        ok(Okay, "Wrong path name '%wZ', expected '%S'\n", &FullPathName, ExpectedPathName);
        ok(StringUsed == &FullPathName, "StringUsed = %p, expected %p\n", StringUsed, &FullPathName);
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
                        ok(0, "GetCurrentDirectory returned %S\n", CurrentPath);
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
            "FilePartSize = %lu, expected %lu\n", (ULONG)FilePartSize, (ULONG)ExpectedFilePartSize);
        ok(NameInvalid == FALSE, "NameInvalid = %u\n", NameInvalid);
        ok(PathType == TestCases[i].PathType, "PathType = %d, expected %d\n", PathType, TestCases[i].PathType);
        ok(LengthNeeded == 0, "LengthNeeded = %lu\n", (ULONG)LengthNeeded);
    }
}

#if defined(_UNITY_BUILD_ENABLED_) && (defined(__GNUC__) || defined(__clang__))
#pragma GCC diagnostic pop
#endif

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
    ok(PathType == RtlPathTypeUnknown ||
       broken(PathType == RtlPathTypeNotSet) /* Win7 */, "PathType = %d\n", PathType);

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
    ok(PathType == RtlPathTypeUnknown ||
       broken(PathType == RtlPathTypeNotSet) /* Win7 */, "PathType = %d\n", PathType);
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
    ok(NameInvalid == FALSE ||
       broken(NameInvalid == (BOOLEAN)-1) /* Win7 */, "NameInvalid = %u\n", NameInvalid);
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
    RtlGetFullPathName_UstrEx_RunTestCases();
}
