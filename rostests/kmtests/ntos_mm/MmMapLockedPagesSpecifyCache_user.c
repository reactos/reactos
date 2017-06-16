/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite MmMapLockedPagesSpecifyCache test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#include "MmMapLockedPagesSpecifyCache.h"

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
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");
    CHECK_ALLOC(QueryBuffer.Buffer, BufferLength);

    Length = 0;
    FILL_READ_BUFFER(QueryBuffer, ReadBuffer);
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    KmtCloseDriver();
    KmtUnloadDriver();
}
