/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite MmMapLockedPagesSpecifyCache test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>
#include <ndk/exfuncs.h>

#include "MmMapLockedPagesSpecifyCache.h"

#define ALIGN_DOWN_BY(size, align) \
        ((ULONG_PTR)(size) & ~((ULONG_PTR)(align) - 1))

#define SET_BUFFER_LENGTH(Var, Length)         \
{                                              \
    C_ASSERT(((Length) % sizeof(ULONG)) == 0); \
    Var = (Length);                            \
}

#define FILL_QUERY_BUFFER(QueryBuffer, BufferLength, UseCache) \
{                                                              \
    QueryBuffer.Length = BufferLength;                         \
    QueryBuffer.Buffer = NULL;                                 \
    QueryBuffer.Cached = UseCache;                             \
    QueryBuffer.Status = STATUS_SUCCESS;                       \
}

#define FILL_READ_BUFFER(QueryBuffer, ReadBuffer)               \
{                                                               \
    PULONG Buffer;                                              \
    ReadBuffer.Buffer = QueryBuffer.Buffer;                     \
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))  \
    {                                                           \
        ReadBuffer.Pattern = WRITE_PATTERN;                     \
        ReadBuffer.Length = QueryBuffer.Length;                 \
        Buffer = QueryBuffer.Buffer;                            \
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i) \
        {                                                       \
            Buffer[i] = ReadBuffer.Pattern;                     \
        }                                                       \
    }                                                           \
}

#define CHECK_ALLOC(MappedBuffer, BufferLength)                 \
{                                                               \
    NTSTATUS Status;                                            \
    PVOID BaseAddress;                                          \
    SIZE_T Size;                                                \
    BaseAddress = MappedBuffer;                                 \
    Size = BufferLength;                                        \
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),        \
                                     &BaseAddress,              \
                                     0,                         \
                                     &Size,                     \
                                     MEM_RESERVE,               \
                                     PAGE_READWRITE);           \
    ok_eq_hex(Status, STATUS_CONFLICTING_ADDRESSES);            \
    BaseAddress = MappedBuffer;                                 \
    Size = 0;                                                   \
    Status = NtFreeVirtualMemory(NtCurrentProcess(),            \
                                 &BaseAddress,                  \
                                 &Size,                         \
                                 MEM_DECOMMIT);                 \
    ok_eq_hex(Status, STATUS_UNABLE_TO_DELETE_SECTION);         \
    BaseAddress = MappedBuffer;                                 \
    Size = 0;                                                   \
    Status = NtFreeVirtualMemory(NtCurrentProcess(),            \
                                 &BaseAddress,                  \
                                 &Size,                         \
                                 MEM_RELEASE);                  \
    ok_eq_hex(Status, STATUS_UNABLE_TO_DELETE_SECTION);         \
    Status = NtUnmapViewOfSection(NtCurrentProcess(),           \
                                  MappedBuffer);                \
    ok_eq_hex(Status, STATUS_NOT_MAPPED_VIEW);                  \
}

START_TEST(MmMapLockedPagesSpecifyCache)
{
    QUERY_BUFFER QueryBuffer;
    READ_BUFFER ReadBuffer;
    DWORD Length;
    USHORT i;
    USHORT BufferLength;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    ULONG_PTR HighestAddress;

    KmtLoadDriver(L"MmMapLockedPagesSpecifyCache", FALSE);
    KmtOpenDriver();

    // Less than a page
    SET_BUFFER_LENGTH(BufferLength, 2048);
    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, TRUE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // 1 page
    SET_BUFFER_LENGTH(BufferLength, 4096);
    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, TRUE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // more than 1 page
    SET_BUFFER_LENGTH(BufferLength, 4096 + 2048);
    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, TRUE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // 2 pages
    SET_BUFFER_LENGTH(BufferLength, 2 * 4096);
    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    Length = sizeof(QUERY_BUFFER);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, TRUE);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // more than 2 pages
    SET_BUFFER_LENGTH(BufferLength, 2 * 4096 + 2048);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, TRUE);
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // ask for a specific address (we know that ReadBuffer.Buffer is free)
    SET_BUFFER_LENGTH(BufferLength, 4096);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    QueryBuffer.Buffer = ReadBuffer.Buffer;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer == ReadBuffer.Buffer, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // ask for an unaligned address
    SET_BUFFER_LENGTH(BufferLength, 4096);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    QueryBuffer.Buffer = (PVOID)((ULONG_PTR)ReadBuffer.Buffer + 2048);
    QueryBuffer.Status = STATUS_INVALID_ADDRESS;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer == NULL, "Buffer is %p\n", QueryBuffer.Buffer);

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_CLEAN, NULL, 0, &Length) == ERROR_SUCCESS, "\n");

    // get system info for MmHighestUserAddress
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    trace("MaximumUserModeAddress: %lx\n", BasicInfo.MaximumUserModeAddress);
    HighestAddress = ALIGN_DOWN_BY(BasicInfo.MaximumUserModeAddress, PAGE_SIZE);

    // near MmHighestUserAddress
    SET_BUFFER_LENGTH(BufferLength, 4096);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    QueryBuffer.Buffer = (PVOID)(HighestAddress - 15 * PAGE_SIZE); // 7ffe0000
    QueryBuffer.Status = STATUS_INVALID_ADDRESS;
    trace("QueryBuffer.Buffer %p\n", QueryBuffer.Buffer);
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer == NULL, "Buffer is %p\n", QueryBuffer.Buffer);

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_CLEAN, NULL, 0, &Length) == ERROR_SUCCESS, "\n");

    // far enough away from MmHighestUserAddress
    SET_BUFFER_LENGTH(BufferLength, 4096);
    FILL_QUERY_BUFFER(QueryBuffer, BufferLength, FALSE);
    QueryBuffer.Buffer = (PVOID)(HighestAddress - 16 * PAGE_SIZE); // 7ffdf000
    QueryBuffer.Status = -1;
    trace("QueryBuffer.Buffer %p\n", QueryBuffer.Buffer);
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Status == STATUS_SUCCESS ||
       QueryBuffer.Status == STATUS_CONFLICTING_ADDRESSES, "Status = %lx\n", QueryBuffer.Status);

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_CLEAN, NULL, 0, &Length) == ERROR_SUCCESS, "\n");

    KmtCloseDriver();
    KmtUnloadDriver();
}
