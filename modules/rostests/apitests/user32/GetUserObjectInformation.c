/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for GetUserObjectInformation
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"
#include <apitest_guard.h>

#include <ndk/mmfuncs.h>
#include <ndk/pstypes.h>
#include <strsafe.h>

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

#define xok ok // Make the test succeed on Win2003
//#define xok(...) // This should make the test succeed on all Windows versions
#define NOTSET 1234

#define TestUserObjectInfo(Handle, Index, Buffer, Length, Ret, Error, LengthNeeded) do  \
    {                                                                                   \
        DWORD _LengthNeeded = NOTSET;                                                   \
        DECLSPEC_ALIGN(16) CHAR _LengthBuffer[2 * sizeof(DWORD)];                       \
        DWORD _Error;                                                                   \
        BOOL _Ret;                                                                      \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationW(Handle, Index, Buffer, Length, NULL);          \
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationW(Handle, Index, Buffer, Length, &_LengthNeeded);\
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
        xok(_LengthNeeded == (LengthNeeded), "LengthNeeded = %lu\n", _LengthNeeded);    \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        *(PDWORD)&_LengthBuffer[1] = NOTSET;                                            \
        _Ret = GetUserObjectInformationW(Handle, Index, Buffer, Length,                 \
                                         (PDWORD)&_LengthBuffer[1]);                    \
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
        _LengthNeeded = *(PDWORD)&_LengthBuffer[1];                                     \
        xok(_LengthNeeded == (LengthNeeded), "LengthNeeded = %lu\n", _LengthNeeded);    \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationW(Handle, Index, Buffer, Length, (PVOID)-4);     \
        _Error = GetLastError();                                                        \
        ok(_Ret == FALSE, "Ret = %d\n", _Ret);                                          \
        ok(_Error == ERROR_NOACCESS, "Error = %lu\n", _Error);                          \
    } while (0)

#define TestUserObjectInfoWithString(Handle, Index, Buffer, BufferSize, String) do                                                          \
    {                                                                                                                                       \
        BOOLEAN _Check;                                                                                                                     \
        SIZE_T SizeOfString = wcslen(String) * sizeof(WCHAR) + sizeof(UNICODE_NULL);                                                         \
        TestUserObjectInfo(Handle,  Index,     NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, SizeOfString);  \
        TestUserObjectInfo(Handle,  Index,     UlongToPtr(1),    0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, SizeOfString);  \
        TestUserObjectInfo(Handle,  Index,     NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);        \
        TestUserObjectInfo(Handle,  Index,     UlongToPtr(1),    1,                       FALSE, ERROR_NOACCESS,            NOTSET);        \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           SizeOfString - 2,        FALSE, ERROR_INSUFFICIENT_BUFFER, SizeOfString);  \
        _Check = CheckBuffer(Buffer, BufferSize, 0x55);                                                                                     \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           SizeOfString - 1,        FALSE, ERROR_INSUFFICIENT_BUFFER, SizeOfString);  \
        _Check = CheckBuffer(Buffer, BufferSize, 0x55);                                                                                     \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                            \
        Buffer[BufferSize / sizeof(WCHAR) - 1] = UNICODE_NULL;                                                                              \
        TestUserObjectInfo(Handle,  Index,     Buffer,           SizeOfString,            TRUE,  0xdeadbeef,                SizeOfString);  \
        ok(wcscmp(Buffer, String) == 0, "Buffer '%ls', expected '%ls'\n", Buffer, String);                                                  \
        _Check = CheckBuffer(Buffer + SizeOfString / sizeof(Buffer[0]), BufferSize - SizeOfString - sizeof(WCHAR), 0x55);                   \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                            \
        Buffer[BufferSize / sizeof(WCHAR) - 1] = UNICODE_NULL;                                                                              \
        TestUserObjectInfo(Handle,  Index,     Buffer,           BufferSize,              TRUE,  0xdeadbeef,                SizeOfString);  \
        ok(wcscmp(Buffer, String) == 0, "Buffer '%ls', expected '%ls'\n", Buffer, String);                                                  \
        _Check = CheckBuffer(Buffer + SizeOfString / sizeof(Buffer[0]), BufferSize - SizeOfString - sizeof(WCHAR), 0x55);                   \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                         \
    } while (0)

static
void
TestGetUserObjectInfoW(void)
{
    USEROBJECTFLAGS UserObjectFlags;
    PWCHAR Buffer;
    ULONG BufferSize = 64 * sizeof(WCHAR);
    HDESK Desktop;
    HDESK Desktop2;
    HWINSTA WinSta;
    HANDLE Token;
    TOKEN_STATISTICS Statistics;
    WCHAR WinStaName[64];
    BOOL Success;
    ULONG Length;

    Buffer = AllocateGuarded(BufferSize);

    TestUserObjectInfo(NULL,    5,         NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, UlongToPtr(1),    0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, UlongToPtr(1),    1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), FALSE, ERROR_INVALID_HANDLE,      0);

    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  UlongToPtr(1),    0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_TYPE,  UlongToPtr(1),    1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_TYPE,  Buffer,           BufferSize,              FALSE, ERROR_INVALID_HANDLE,      0);

    TestUserObjectInfo(NULL,    UOI_NAME,  NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  UlongToPtr(1),    0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_NAME,  UlongToPtr(1),    1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_NAME,  Buffer,           BufferSize,              FALSE, ERROR_INVALID_HANDLE,      0);

    Desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!Desktop)
    {
        skip("Failed to get desktop handle\n");
        FreeGuarded(Buffer);
        return;
    }

    WinSta = GetProcessWindowStation();
    if (!WinSta)
    {
        skip("Failed to get winsta handle\n");
        FreeGuarded(Buffer);
        return;
    }

    TestUserObjectInfo(Desktop, 5,         NULL,             0,                       FALSE, ERROR_INVALID_PARAMETER,   0);
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, UlongToPtr(1),    0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, UlongToPtr(1),    1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), TRUE,  0xdeadbeef,                sizeof(USEROBJECTFLAGS));

    TestUserObjectInfoWithString(Desktop, UOI_TYPE, Buffer, BufferSize, L"Desktop");
    TestUserObjectInfoWithString(Desktop, UOI_NAME, Buffer, BufferSize, L"Default");

    TestUserObjectInfoWithString(WinSta, UOI_TYPE, Buffer, BufferSize, L"WindowStation");
    TestUserObjectInfoWithString(WinSta, UOI_NAME, Buffer, BufferSize, L"WinSta0");

    /* Autogenerated name will be Service-0x<luidhigh>-<luidlow>$ */
    StringCbCopyW(WinStaName, sizeof(WinStaName), L"<failed>");
    Success = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);
    ok(Success == TRUE, "OpenProcessToken failed with %lu\n", GetLastError());
    if (Success)
    {
        Success = GetTokenInformation(Token,
                                      TokenStatistics,
                                      &Statistics,
                                      sizeof(Statistics),
                                      &Length);
        ok(Success == TRUE, "GetTokenInformation failed with %lu\n", GetLastError());
        if (Success)
        {
            StringCbPrintfW(WinStaName,
                            sizeof(WinStaName),
                            L"Service-0x%lx-%lx$",
                            Statistics.AuthenticationId.HighPart,
                            Statistics.AuthenticationId.LowPart);
            trace("Expected autogenerated Winsta name: %ls\n", WinStaName);
        }
        CloseHandle(Token);
    }

    /* Create our own Winsta */
    WinSta = CreateWindowStationW(NULL, 0, WINSTA_READATTRIBUTES, NULL);
    ok(WinSta != NULL, "CreateWindowStationW failed with %lu\n", GetLastError());
    if (WinSta)
    {
        TestUserObjectInfoWithString(WinSta, UOI_TYPE, Buffer, BufferSize, L"WindowStation");
        TestUserObjectInfoWithString(WinSta, UOI_NAME, Buffer, BufferSize, WinStaName);
        CloseWindowStation(WinSta);
    }
    else
    {
        skip("Failed to create winsta\n");
    }

    WinSta = CreateWindowStationW(L"", 0, WINSTA_READATTRIBUTES, NULL);
    ok(WinSta != NULL, "CreateWindowStationW failed with %lu\n", GetLastError());
    if (WinSta)
    {
        TestUserObjectInfoWithString(WinSta, UOI_TYPE, Buffer, BufferSize, L"WindowStation");
        TestUserObjectInfoWithString(WinSta, UOI_NAME, Buffer, BufferSize, WinStaName);
        CloseWindowStation(WinSta);
    }
    else
    {
        skip("Failed to create winsta\n");
    }

    WinSta = CreateWindowStationW(L"GetUserObjectInformation_apitest_winsta", 0, WINSTA_READATTRIBUTES, NULL);
    ok(WinSta != NULL, "CreateWindowStationW failed with %lu\n", GetLastError());
    if (WinSta)
    {
        TestUserObjectInfoWithString(WinSta, UOI_TYPE, Buffer, BufferSize, L"WindowStation");
        TestUserObjectInfoWithString(WinSta, UOI_NAME, Buffer, BufferSize, L"GetUserObjectInformation_apitest_winsta");
        CloseWindowStation(WinSta);
    }
    else
    {
        skip("Failed to create winsta\n");
    }

    WinSta = CreateWindowStationW(L"1", 0, WINSTA_READATTRIBUTES, NULL);
    ok(WinSta != NULL, "CreateWindowStationW failed with %lu\n", GetLastError());
    if (WinSta)
    {
        TestUserObjectInfoWithString(WinSta, UOI_TYPE, Buffer, BufferSize, L"WindowStation");
        TestUserObjectInfoWithString(WinSta, UOI_NAME, Buffer, BufferSize, L"1");
        CloseWindowStation(WinSta);
    }
    else
    {
        skip("Failed to create winsta\n");
    }

    /* Create our own desktop */
    Desktop2 = CreateDesktopW(NULL, NULL, NULL, 0, DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS, NULL);
    ok(Desktop2 == NULL, "CreateDesktopW succeeded\n");
    if (Desktop2) CloseDesktop(Desktop2);

    Desktop2 = CreateDesktopW(L"", NULL, NULL, 0, DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS, NULL);
    ok(Desktop2 == NULL, "CreateDesktopW succeeded\n");
    if (Desktop2) CloseDesktop(Desktop2);

    Desktop2 = CreateDesktopW(L"2", NULL, NULL, 0, DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS, NULL);
    ok(Desktop2 != NULL, "CreateDesktopW failed with %lu\n", GetLastError());
    if (Desktop2)
    {
        TestUserObjectInfoWithString(Desktop2, UOI_TYPE, Buffer, BufferSize, L"Desktop");
        TestUserObjectInfoWithString(Desktop2, UOI_NAME, Buffer, BufferSize, L"2");
    }
    else
    {
        skip("Failed to create winsta\n");
    }

    CloseDesktop(Desktop2);

    FreeGuarded(Buffer);

    /* Make sure nothing behind the needed buffer is touched */
    BufferSize = sizeof(L"Desktop");
    Buffer = AllocateGuarded(BufferSize);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize,              TRUE,  0xdeadbeef,                sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize + 1,          FALSE, ERROR_NOACCESS,            NOTSET);
    FreeGuarded(Buffer);
}

#undef TestUserObjectInfo
#define TestUserObjectInfo(Handle, Index, Buffer, Length, Ret, Error, LengthNeeded) do  \
    {                                                                                   \
        DWORD _LengthNeeded = NOTSET;                                                   \
        DECLSPEC_ALIGN(16) CHAR _LengthBuffer[2 * sizeof(DWORD)];                       \
        DWORD _Error;                                                                   \
        BOOL _Ret;                                                                      \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationA(Handle, Index, Buffer, Length, NULL);          \
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationA(Handle, Index, Buffer, Length, &_LengthNeeded);\
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
        xok(_LengthNeeded == (LengthNeeded), "LengthNeeded = %lu\n", _LengthNeeded);    \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        *(PDWORD)&_LengthBuffer[1] = NOTSET;                                            \
        _Ret = GetUserObjectInformationA(Handle, Index, Buffer, Length,                 \
                                         (PDWORD)&_LengthBuffer[1]);                    \
        _Error = GetLastError();                                                        \
        ok(_Ret == (Ret), "Ret = %d\n", _Ret);                                          \
        xok(_Error == (Error), "Error = %lu\n", _Error);                                \
        _LengthNeeded = *(PDWORD)&_LengthBuffer[1];                                     \
        xok(_LengthNeeded == (LengthNeeded), "LengthNeeded = %lu\n", _LengthNeeded);    \
                                                                                        \
        SetLastError(0xdeadbeef);                                                       \
        _Ret = GetUserObjectInformationA(Handle, Index, Buffer, Length, (PVOID)-4);     \
        _Error = GetLastError();                                                        \
        ok(_Ret == FALSE, "Ret = %d\n", _Ret);                                          \
        ok(_Error == ERROR_NOACCESS, "Error = %lu\n", _Error);                          \
    } while (0)

#undef TestUserObjectInfoWithString
#define TestUserObjectInfoWithString(Handle, Index, Buffer, BufferSize, String) do                                                                          \
    {                                                                                                                                                       \
        BOOLEAN _Check;                                                                                                                                     \
        TestUserObjectInfo(Handle,  Index,     NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        TestUserObjectInfo(Handle,  Index,     (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        TestUserObjectInfo(Handle,  Index,     NULL,             1,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        TestUserObjectInfo(Handle,  Index,     (PVOID)1,         1,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           sizeof(String) - 2,      FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        _Check = CheckBuffer(Buffer, BufferSize, 0x55);                                                                                                     \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           sizeof(String) - 1,      FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(String) * sizeof(WCHAR));\
        _Check = CheckBuffer(Buffer, BufferSize, 0x55);                                                                                                     \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           sizeof(String),          TRUE,  0xdeadbeef,                sizeof(String));                \
        ok(strcmp(Buffer, String) == 0, "Buffer '%s'\n", Buffer);                                                                                           \
        _Check = CheckBuffer(Buffer + sizeof(String), BufferSize - sizeof(String), 0x55);                                                                   \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                                         \
        RtlFillMemory(Buffer, BufferSize, 0x55);                                                                                                            \
        TestUserObjectInfo(Handle,  Index,     Buffer,           BufferSize,              TRUE,  0xdeadbeef,                sizeof(String));                \
        ok(strcmp(Buffer, String) == 0, "Buffer '%s'\n", Buffer);                                                                                           \
        _Check = CheckBuffer(Buffer + sizeof(String), BufferSize - sizeof(String), 0x55);                                                                   \
        ok(_Check == TRUE, "CheckBuffer failed\n");                                                                                                         \
    } while (0)

static
void
TestGetUserObjectInfoA(void)
{
    USEROBJECTFLAGS UserObjectFlags;
    PCHAR Buffer;
    ULONG BufferSize = 64;
    HDESK Desktop;
    HWINSTA WinSta;

    Buffer = AllocateGuarded(BufferSize);

    TestUserObjectInfo(NULL,    5,         NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, (PVOID)1,         0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), FALSE, ERROR_INVALID_HANDLE,      0);

    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  (PVOID)1,         0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             1,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  (PVOID)1,         1,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  Buffer,           BufferSize,              FALSE, ERROR_INVALID_HANDLE,      0);

    TestUserObjectInfo(NULL,    UOI_NAME,  NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  (PVOID)1,         0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  NULL,             1,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  (PVOID)1,         1,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_NAME,  Buffer,           BufferSize,              FALSE, ERROR_INVALID_HANDLE,      0);

    Desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!Desktop)
    {
        skip("Failed to get desktop handle\n");
        FreeGuarded(Buffer);
        return;
    }

    WinSta = GetProcessWindowStation();
    if (!WinSta)
    {
        skip("Failed to get winsta handle\n");
        FreeGuarded(Buffer);
        return;
    }

    TestUserObjectInfo(Desktop, 5,         NULL,             0,                       FALSE, ERROR_INVALID_PARAMETER,   0);
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), TRUE,  0xdeadbeef,                sizeof(USEROBJECTFLAGS));

    TestUserObjectInfoWithString(Desktop, UOI_TYPE, Buffer, BufferSize, "Desktop");
    TestUserObjectInfoWithString(Desktop, UOI_NAME, Buffer, BufferSize, "Default");

    TestUserObjectInfoWithString(WinSta,  UOI_TYPE, Buffer, BufferSize, "WindowStation");
    TestUserObjectInfoWithString(WinSta,  UOI_NAME, Buffer, BufferSize, "WinSta0");

    FreeGuarded(Buffer);

    /* Make sure nothing behind the needed buffer is touched */
    BufferSize = sizeof("Desktop");
    Buffer = AllocateGuarded(BufferSize);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize,              TRUE,  0xdeadbeef,                sizeof("Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize + 1,          TRUE,  0xdeadbeef,                sizeof("Desktop"));
    FreeGuarded(Buffer);
}

START_TEST(GetUserObjectInformation)
{
    TestGetUserObjectInfoW();
    TestGetUserObjectInfoA();
}
