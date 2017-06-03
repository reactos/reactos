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

START_TEST(MmMapLockedPagesSpecifyCache)
{
    QUERY_BUFFER QueryBuffer;
    READ_BUFFER ReadBuffer;
    DWORD Length;
    USHORT i;
    PULONG Buffer;
    USHORT BufferLength;

    KmtLoadDriver(L"MmMapLockedPagesSpecifyCache", FALSE);
    KmtOpenDriver();

    // Less than a page
    SET_BUFFER_LENGTH(BufferLength, 2048);
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = WRITE_PATTERN;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // 1 page
    SET_BUFFER_LENGTH(BufferLength, 4096);
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = WRITE_PATTERN;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // more than 1 page
    SET_BUFFER_LENGTH(BufferLength, 4096 + 2048);
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = WRITE_PATTERN;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // 2 pages
    SET_BUFFER_LENGTH(BufferLength, 2 * 4096);
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = WRITE_PATTERN;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    // more than 2 pages
    SET_BUFFER_LENGTH(BufferLength, 2 * 4096 + 2048);
    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = BufferLength;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, BufferLength);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = WRITE_PATTERN;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(ULONG); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    KmtCloseDriver();
    KmtUnloadDriver();
}
