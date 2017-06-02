/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite MmMapLockedPagesSpecifyCache test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#include "MmMapLockedPagesSpecifyCache.h"

START_TEST(MmMapLockedPagesSpecifyCache)
{
    QUERY_BUFFER QueryBuffer;
    READ_BUFFER ReadBuffer;
    DWORD Length;
    USHORT i;
    PUSHORT Buffer;

    KmtLoadDriver(L"MmMapLockedPagesSpecifyCache", FALSE);
    KmtOpenDriver();

    QueryBuffer.Length = 0x100;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = FALSE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, 0x100);
    ok_eq_pointer(QueryBuffer.Buffer, NULL);

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    QueryBuffer.Length = 0x100;
    QueryBuffer.Buffer = NULL;
    QueryBuffer.Cached = TRUE;
    Length = sizeof(QUERY_BUFFER);
    ok(KmtSendBufferToDriver(IOCTL_QUERY_BUFFER, &QueryBuffer, sizeof(QUERY_BUFFER), &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(QueryBuffer.Length, 0x100);
    ok(QueryBuffer.Buffer != NULL, "Buffer is NULL\n");

    ReadBuffer.Buffer = QueryBuffer.Buffer;
    if (!skip(QueryBuffer.Buffer != NULL, "Buffer is NULL\n"))
    {
        ReadBuffer.Pattern = 0xA7;
        ReadBuffer.Length = QueryBuffer.Length;
        Buffer = QueryBuffer.Buffer;
        for (i = 0; i < ReadBuffer.Length / sizeof(USHORT); ++i)
        {
            Buffer[i] = ReadBuffer.Pattern;
        }
    }

    Length = 0;
    ok(KmtSendBufferToDriver(IOCTL_READ_BUFFER, &ReadBuffer, sizeof(READ_BUFFER), &Length) == ERROR_SUCCESS, "\n");

    KmtCloseDriver();
    KmtUnloadDriver();
}
