/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlDetermineDosPathNameType_U/RtlDetermineDosPathNameType_Ustr
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

/*
ULONG
NTAPI
RtlDetermineDosPathNameType_U(
    IN PCWSTR Path
);

ULONG
NTAPI
RtlDetermineDosPathNameType_Ustr(
    IN PCUNICODE_STRING Path
);
*/

static
ULONG
(NTAPI
*RtlDetermineDosPathNameType_Ustr)(
    IN PCUNICODE_STRING Path
)
//= (PVOID)0x7c830669;
;

START_TEST(RtlDetermineDosPathNameType)
{
    RTL_PATH_TYPE PathType;
    struct
    {
        PCWSTR FileName;
        RTL_PATH_TYPE PathType;
    } Tests[] =
    {
        { L"",                  RtlPathTypeRelative },
        { L"xyz",               RtlPathTypeRelative },
        { L"CON",               RtlPathTypeRelative },
        { L"NUL",               RtlPathTypeRelative },
        { L":",                 RtlPathTypeRelative },
        { L"::",                RtlPathTypeDriveRelative },
        { L":::",               RtlPathTypeDriveRelative },
        { L"::::",              RtlPathTypeDriveRelative },
        { L"::\\",              RtlPathTypeDriveAbsolute },
        { L"\\",                RtlPathTypeRooted },
        { L"\\:",               RtlPathTypeRooted },
        { L"\\C:",              RtlPathTypeRooted },
        { L"\\C:\\",            RtlPathTypeRooted },
        { L"/",                 RtlPathTypeRooted },
        { L"/:",                RtlPathTypeRooted },
        { L"/C:",               RtlPathTypeRooted },
        { L"/C:/",              RtlPathTypeRooted },
        { L"C",                 RtlPathTypeRelative },
        { L"C:",                RtlPathTypeDriveRelative },
        { L"C:a",               RtlPathTypeDriveRelative },
        { L"C:a\\",             RtlPathTypeDriveRelative },
        { L"C:\\",              RtlPathTypeDriveAbsolute },
        { L"C:/",               RtlPathTypeDriveAbsolute },
        { L"C:\\a",             RtlPathTypeDriveAbsolute },
        { L"C:/a",              RtlPathTypeDriveAbsolute },
        { L"C:\\\\",            RtlPathTypeDriveAbsolute },
        { L"\\\\",              RtlPathTypeUncAbsolute },
        { L"\\\\\\",            RtlPathTypeUncAbsolute },
        { L"\\\\;",             RtlPathTypeUncAbsolute },
        { L"\\\\f\\b\\",        RtlPathTypeUncAbsolute },
        { L"\\\\f\\b",          RtlPathTypeUncAbsolute },
        { L"\\\\f\\",           RtlPathTypeUncAbsolute },
        { L"\\\\f",             RtlPathTypeUncAbsolute },
        { L"\\??\\",            RtlPathTypeRooted },
        { L"\\??\\UNC",         RtlPathTypeRooted },
        { L"\\??\\UNC\\",       RtlPathTypeRooted },
        { L"\\?",               RtlPathTypeRooted },
        { L"\\?\\",             RtlPathTypeRooted },
        { L"\\?\\UNC",          RtlPathTypeRooted },
        { L"\\?\\UNC\\",        RtlPathTypeRooted },
        { L"\\\\?\\UNC\\",      RtlPathTypeLocalDevice },
        { L"\\\\?",             RtlPathTypeRootLocalDevice },
        { L"\\\\??",            RtlPathTypeUncAbsolute },
        { L"\\\\??\\",          RtlPathTypeUncAbsolute },
        { L"\\\\??\\C:\\",      RtlPathTypeUncAbsolute },
        { L"\\\\.",             RtlPathTypeRootLocalDevice },
        { L"\\\\.\\",           RtlPathTypeLocalDevice },
        { L"\\\\.\\C:\\",       RtlPathTypeLocalDevice },
        { L"\\/",               RtlPathTypeUncAbsolute },
        { L"/\\",               RtlPathTypeUncAbsolute },
        { L"//",                RtlPathTypeUncAbsolute },
        { L"///",               RtlPathTypeUncAbsolute },
        { L"//;",               RtlPathTypeUncAbsolute },
        { L"//?",               RtlPathTypeRootLocalDevice },
        { L"/\\?",              RtlPathTypeRootLocalDevice },
        { L"\\/?",              RtlPathTypeRootLocalDevice },
        { L"//??",              RtlPathTypeUncAbsolute },
        { L"//?" L"?/",         RtlPathTypeUncAbsolute },
        { L"//?" L"?/C:/",      RtlPathTypeUncAbsolute },
        { L"//.",               RtlPathTypeRootLocalDevice },
        { L"\\/.",              RtlPathTypeRootLocalDevice },
        { L"/\\.",              RtlPathTypeRootLocalDevice },
        { L"//./",              RtlPathTypeLocalDevice },
        { L"//./C:/",           RtlPathTypeLocalDevice },
        { L"%SystemRoot%",      RtlPathTypeRelative },
    };
    ULONG i;
    PWSTR FileName;
    USHORT Length;

    if (!RtlDetermineDosPathNameType_Ustr)
    {
        RtlDetermineDosPathNameType_Ustr = (PVOID)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlDetermineDosPathNameType_Ustr");
        if (!RtlDetermineDosPathNameType_Ustr)
            skip("RtlDetermineDosPathNameType_Ustr unavailable\n");
    }

    StartSeh() RtlDetermineDosPathNameType_U(NULL);     EndSeh(STATUS_ACCESS_VIOLATION);

    if (RtlDetermineDosPathNameType_Ustr)
    {
        UNICODE_STRING PathString;
        StartSeh() RtlDetermineDosPathNameType_Ustr(NULL);  EndSeh(STATUS_ACCESS_VIOLATION);

        RtlInitEmptyUnicodeString(&PathString, NULL, MAXUSHORT);
        PathType = RtlDetermineDosPathNameType_Ustr(&PathString);
        ok(PathType == RtlPathTypeRelative, "PathType = %d\n", PathType);
    }

    for (i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
    {
        Length = (USHORT)wcslen(Tests[i].FileName) * sizeof(WCHAR);
        FileName = AllocateGuarded(Length + sizeof(UNICODE_NULL));
        RtlCopyMemory(FileName, Tests[i].FileName, Length + sizeof(UNICODE_NULL));
        MakeReadOnly(FileName, Length + sizeof(UNICODE_NULL));
        StartSeh()
            PathType = RtlDetermineDosPathNameType_U(FileName);
            ok(PathType == Tests[i].PathType, "PathType is %d, expected %d for '%S'\n", PathType, Tests[i].PathType, Tests[i].FileName);
        EndSeh(STATUS_SUCCESS);
        FreeGuarded(FileName);

        if (RtlDetermineDosPathNameType_Ustr)
        {
            UNICODE_STRING PathString;

            FileName = AllocateGuarded(Length);
            RtlCopyMemory(FileName, Tests[i].FileName, Length);
            MakeReadOnly(FileName, Length);
            PathString.Buffer = FileName;
            PathString.Length = Length;
            PathString.MaximumLength = MAXUSHORT;
            StartSeh()
                PathType = RtlDetermineDosPathNameType_Ustr(&PathString);
                ok(PathType == Tests[i].PathType, "PathType is %d, expected %d for '%S'\n", PathType, Tests[i].PathType, Tests[i].FileName);
            EndSeh(STATUS_SUCCESS);
            FreeGuarded(FileName);
        }
    }
}
