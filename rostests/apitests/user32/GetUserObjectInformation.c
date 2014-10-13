/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for GetUserObjectInformation
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winuser.h>
#include <ndk/mmfuncs.h>
#include <ndk/pstypes.h>

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
PVOID
AllocateGuarded(
    SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

static
VOID
FreeGuarded(
    PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
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

static
void
TestGetUserObjectInfoW(void)
{
    USEROBJECTFLAGS UserObjectFlags;
    PWCHAR Buffer;
    ULONG BufferSize = 64 * sizeof(WCHAR);
    HDESK Desktop;
    BOOLEAN Check;

    Buffer = AllocateGuarded(BufferSize);

    TestUserObjectInfo(NULL,    5,         NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, (PVOID)1,         0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), FALSE, ERROR_INVALID_HANDLE,      0);

    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  (PVOID)1,         0,                       FALSE, ERROR_INVALID_HANDLE,      0);
    TestUserObjectInfo(NULL,    UOI_TYPE,  NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_TYPE,  (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(NULL,    UOI_TYPE,  Buffer,           BufferSize,              FALSE, ERROR_INVALID_HANDLE,      0);

    Desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!Desktop)
    {
        skip("Failed to get desktop handle\n");
        return;
    }

    TestUserObjectInfo(Desktop, 5,         NULL,             0,                       FALSE, ERROR_INVALID_PARAMETER,   0);
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), TRUE,  0xdeadbeef,                sizeof(USEROBJECTFLAGS));

    TestUserObjectInfo(Desktop, UOI_TYPE,  NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_TYPE,  (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof(L"Desktop") - 2,  FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    Check = CheckBuffer(Buffer, BufferSize, 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof(L"Desktop") - 1,  FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    Check = CheckBuffer(Buffer, BufferSize, 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof(L"Desktop"),      TRUE,  0xdeadbeef,                sizeof(L"Desktop"));
    ok(wcscmp(Buffer, L"Desktop") == 0, "Buffer '%ls'\n", Buffer);
    Check = CheckBuffer(Buffer + sizeof("Desktop"), BufferSize - sizeof(L"Desktop"), 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize,              TRUE,  0xdeadbeef,                sizeof(L"Desktop"));
    ok(wcscmp(Buffer, L"Desktop") == 0, "Buffer '%ls'\n", Buffer);
    Check = CheckBuffer(Buffer + sizeof("Desktop"), BufferSize - sizeof(L"Desktop"), 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");

    FreeGuarded(Buffer);

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

static
void
TestGetUserObjectInfoA(void)
{
    USEROBJECTFLAGS UserObjectFlags;
    PCHAR Buffer;
    ULONG BufferSize = 64;
    HDESK Desktop;
    BOOLEAN Check;

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

    Desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!Desktop)
    {
        skip("Failed to get desktop handle\n");
        return;
    }

    TestUserObjectInfo(Desktop, 5,         NULL,             0,                       FALSE, ERROR_INVALID_PARAMETER,   0);
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(USEROBJECTFLAGS));
    TestUserObjectInfo(Desktop, UOI_FLAGS, NULL,             1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, (PVOID)1,         1,                       FALSE, ERROR_NOACCESS,            NOTSET);
    TestUserObjectInfo(Desktop, UOI_FLAGS, &UserObjectFlags, sizeof(UserObjectFlags), TRUE,  0xdeadbeef,                sizeof(USEROBJECTFLAGS));

    TestUserObjectInfo(Desktop, UOI_TYPE,  NULL,             0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  (PVOID)1,         0,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  NULL,             1,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    TestUserObjectInfo(Desktop, UOI_TYPE,  (PVOID)1,         1,                       FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof("Desktop") - 2,   FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    Check = CheckBuffer(Buffer, BufferSize, 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof("Desktop") - 1,   FALSE, ERROR_INSUFFICIENT_BUFFER, sizeof(L"Desktop"));
    Check = CheckBuffer(Buffer, BufferSize, 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           sizeof("Desktop"),       TRUE,  0xdeadbeef,                sizeof("Desktop"));
    ok(strcmp(Buffer, "Desktop") == 0, "Buffer '%s'\n", Buffer);
    Check = CheckBuffer(Buffer + sizeof("Desktop"), BufferSize - sizeof("Desktop"), 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");
    RtlFillMemory(Buffer, BufferSize, 0x55);
    TestUserObjectInfo(Desktop, UOI_TYPE,  Buffer,           BufferSize,              TRUE,  0xdeadbeef,                sizeof("Desktop"));
    ok(strcmp(Buffer, "Desktop") == 0, "Buffer '%s'\n", Buffer);
    Check = CheckBuffer(Buffer + sizeof("Desktop"), BufferSize - sizeof("Desktop"), 0x55);
    ok(Check == TRUE, "CheckBuffer failed\n");

    FreeGuarded(Buffer);

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