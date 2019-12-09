/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGetFullPathName_U
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/*
ULONG
NTAPI
RtlGetFullPathName_U(
    IN PCWSTR FileName,
    IN ULONG Size,
    IN PWSTR Buffer,
    OUT PWSTR *ShortName
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

    /* The function nulls the rest of the buffer! */
    for (i = Length + sizeof(UNICODE_NULL); i < MaximumLength; i++)
    {
        UCHAR Char = ((PUCHAR)Buffer)[i];
        if (Char != 0)
        {
            ok(0, "Found 0x%x at offset %lu, expected 0x%x\n", Char, (ULONG)i, 0);
            /* Don't count this as a failure unless the string was actually wrong */
            //Result = FALSE;
            /* Don't flood the log */
            break;
        }
    }

    return Result;
}

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
        PREFIX_TYPE FilePartPrefixType;
        SIZE_T FilePartSize;
    } TestCases[] =
    {
//        { __LINE__, L"C:",                 PrefixCurrentPath, L"", PrefixCurrentPathWithoutLastPart },
        { __LINE__, L"C:\\",               PrefixNone, L"C:\\" },
        { __LINE__, L"C:\\test",           PrefixNone, L"C:\\test", PrefixCurrentDrive },
        { __LINE__, L"C:\\test\\",         PrefixNone, L"C:\\test\\" },
        { __LINE__, L"C:/test/",           PrefixNone, L"C:\\test\\" },

        { __LINE__, L"C:\\\\test",         PrefixNone, L"C:\\test", PrefixCurrentDrive },
        { __LINE__, L"test",               PrefixCurrentPath, L"\\test", PrefixCurrentPath, sizeof(WCHAR) },
        { __LINE__, L"\\test",             PrefixCurrentDrive, L"test", PrefixCurrentDrive },
        { __LINE__, L"/test",              PrefixCurrentDrive, L"test", PrefixCurrentDrive },
        { __LINE__, L".\\test",            PrefixCurrentPath, L"\\test", PrefixCurrentPath, sizeof(WCHAR) },

        { __LINE__, L"\\.",                PrefixCurrentDrive, L"" },
        { __LINE__, L"\\.\\",              PrefixCurrentDrive, L"" },
        { __LINE__, L"\\\\.",              PrefixNone, L"\\\\.\\" },
        { __LINE__, L"\\\\.\\",            PrefixNone, L"\\\\.\\" },
        { __LINE__, L"\\\\.\\Something\\", PrefixNone, L"\\\\.\\Something\\" },

        { __LINE__, L"\\??\\",             PrefixCurrentDrive, L"??\\" },
        { __LINE__, L"\\??\\C:",           PrefixCurrentDrive, L"??\\C:", PrefixCurrentDrive, 3 * sizeof(WCHAR) },
        { __LINE__, L"\\??\\C:\\",         PrefixCurrentDrive, L"??\\C:\\" },
        { __LINE__, L"\\??\\C:\\test",     PrefixCurrentDrive, L"??\\C:\\test", PrefixCurrentDrive, 6 * sizeof(WCHAR) },
        { __LINE__, L"\\??\\C:\\test\\",   PrefixCurrentDrive, L"??\\C:\\test\\" },

        { __LINE__, L"\\\\??\\",           PrefixNone, L"\\\\??\\" },
        { __LINE__, L"\\\\??\\C:",         PrefixNone, L"\\\\??\\C:" },
        { __LINE__, L"\\\\??\\C:\\",       PrefixNone, L"\\\\??\\C:\\" },
        { __LINE__, L"\\\\??\\C:\\test",   PrefixNone, L"\\\\??\\C:\\test", PrefixNone, sizeof(L"\\\\??\\C:\\") },
        { __LINE__, L"\\\\??\\C:\\test\\", PrefixNone, L"\\\\??\\C:\\test\\" },
    };
    WCHAR FullPathNameBuffer[MAX_PATH];
    PWSTR ShortName;
    SIZE_T Length;
    WCHAR ExpectedPathName[MAX_PATH];
    SIZE_T FilePartSize;
    SIZE_T ExpectedFilePartSize;
    const INT TestCount = sizeof(TestCases) / sizeof(TestCases[0]);
    INT i;
    BOOLEAN Okay;

    for (i = 0; i < TestCount; i++)
    {
        trace("i = %d\n", i);
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
                Length = GetCurrentDirectoryW(sizeof(ExpectedPathName) / sizeof(WCHAR), ExpectedPathName);
                if (Length == 3 && TestCases[i].FullPathName[0])
                    ExpectedPathName[2] = UNICODE_NULL;
                break;
            default:
                skip("Invalid test!\n");
                continue;
        }
        wcscat(ExpectedPathName, TestCases[i].FullPathName);
        RtlFillMemory(FullPathNameBuffer, sizeof(FullPathNameBuffer), 0xAA);
        Length = 0;
        StartSeh()
            Length = RtlGetFullPathName_U(TestCases[i].FileName,
                                          sizeof(FullPathNameBuffer),
                                          FullPathNameBuffer,
                                          &ShortName);
        EndSeh(STATUS_SUCCESS);

        Okay = CheckStringBuffer(FullPathNameBuffer, Length, sizeof(FullPathNameBuffer), ExpectedPathName);
        ok(Okay, "Line %lu: Wrong path name '%S', expected '%S'\n", TestCases[i].Line, FullPathNameBuffer, ExpectedPathName);

        if (!ShortName)
            FilePartSize = 0;
        else
            FilePartSize = ShortName - FullPathNameBuffer;

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
    }
}

START_TEST(RtlGetFullPathName_U)
{
    PCWSTR FileName;
    PWSTR ShortName;
    ULONG Length;

    /* Parameter checks */
    StartSeh()
        Length = RtlGetFullPathName_U(NULL, 0, NULL, NULL);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = RtlGetFullPathName_U(L"", 0, NULL, NULL);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);

    ShortName = InvalidPointer;
    StartSeh()
        Length = RtlGetFullPathName_U(NULL, 0, NULL, &ShortName);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(ShortName == InvalidPointer ||
        broken(ShortName == NULL) /* Win7 */, "ShortName = %p\n", ShortName);

    StartSeh()
        Length = RtlGetFullPathName_U(L"", 0, NULL, NULL);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);

    ShortName = InvalidPointer;
    StartSeh()
        Length = RtlGetFullPathName_U(L"", 0, NULL, &ShortName);
        ok(Length == 0, "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(ShortName == InvalidPointer ||
        broken(ShortName == NULL) /* Win7 */, "ShortName = %p\n", ShortName);

    StartSeh()
        Length = RtlGetFullPathName_U(L"C:\\test", 0, NULL, NULL);
        ok(Length == sizeof(L"C:\\test"), "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);

    FileName = L"C:\\test";
    ShortName = InvalidPointer;
    StartSeh()
        Length = RtlGetFullPathName_U(FileName, 0, NULL, &ShortName);
        ok(Length == sizeof(L"C:\\test"), "Length = %lu\n", Length);
    EndSeh(STATUS_SUCCESS);
    ok(ShortName == InvalidPointer ||
        broken(ShortName == NULL) /* Win7 */, "ShortName = %p\n", ShortName);

    /* Check the actual functionality with different paths */
    RunTestCases();
}
