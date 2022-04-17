#include "precomp.h"

#define IS_DRIVE_TYPE_VALID(type) ((type) != DRIVE_UNKNOWN && (type) != DRIVE_NO_ROOT_DIR)

START_TEST(GetDriveType)
{
    UINT Type, Type2, i;
    WCHAR Path[MAX_PATH];

    /* Note: Successful calls can set last error to at least ERROR_NOT_A_REPARSE_POINT, we don't test it here */
    SetLastError(0xdeadbeaf);

    Type = GetDriveTypeW(L"");
    ok(Type == DRIVE_NO_ROOT_DIR, "Expected DRIVE_NO_ROOT_DIR, got %u\n", Type);

    Type = GetDriveTypeW(L"\nC:\\");
    ok(Type == DRIVE_NO_ROOT_DIR, "Expected DRIVE_NO_ROOT_DIR, got %u\n", Type);

    Type = GetDriveTypeW(L"Z:\\");
    ok(Type == DRIVE_NO_ROOT_DIR, "Expected DRIVE_NO_ROOT_DIR, got %u\n", Type);

    ok(GetLastError() == 0xdeadbeaf, "Expected no errors, got %lu\n", GetLastError());

    /* Drive root is accepted without ending slash */
    Type = GetDriveTypeW(L"C:");
    ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);

    Type = GetDriveTypeW(L"C:\\");
    ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);

    Type = GetDriveTypeW(NULL);
    ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);

    i = GetCurrentDirectoryW(sizeof(Path)/sizeof(Path[0]), Path);
    if (i)
    {
        /* No trailing backslash returned unless we're at the drive root */
        if (Path[i - 1] != L'\\')
        {
            SetLastError(0xdeadbeaf);
            Type2 = GetDriveTypeW(Path);
            ok(Type2 == DRIVE_NO_ROOT_DIR, "Expected DRIVE_NO_ROOT_DIR, got %u\n", Type2);
            ok(GetLastError() == 0xdeadbeaf, "Expected no errors, got %lu\n", GetLastError());

            StringCchCopyW(Path + i, MAX_PATH - i, L"\\");
        }
        Type2 = GetDriveTypeW(Path);
        ok(Type == Type2, "Types are not equal: %u != %u\n", Type, Type2);
    }

    i = GetSystemDirectoryW(Path, sizeof(Path)/sizeof(Path[0]));
    if (i)
    {
        /* Note: there is no backslash at the end of Path */
        SetLastError(0xdeadbeaf);
        Type = GetDriveTypeW(Path);
        ok(Type == DRIVE_NO_ROOT_DIR, "Expected DRIVE_NO_ROOT_DIR, got %u\n", Type);
        ok(GetLastError() == 0xdeadbeaf, "Expected no errors, got %lu\n", GetLastError());

        StringCchCopyW(Path + i, MAX_PATH - i, L"\\");
        Type = GetDriveTypeW(Path);
        ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);

        StringCchCopyW(Path + i, MAX_PATH - i, L"/");
        Type = GetDriveTypeW(Path);
        ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);

        StringCchCopyW(Path + i, MAX_PATH - i, L"\\\\");
        Type = GetDriveTypeW(Path);
        ok(IS_DRIVE_TYPE_VALID(Type), "Expected valid drive type, got %u\n", Type);
    }
}
