/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for Read/Write operations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "IoReadWrite.h"

static
VOID
TestRead(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Cached,
    _In_ BOOLEAN UseFastIo,
    _In_ BOOLEAN ReturnPending)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    HANDLE EventHandle;
    UCHAR Buffer[32];
    LARGE_INTEGER Offset;
    ULONG BaseKey, StatusKey, Key;
    DWORD WaitStatus;

    BaseKey = (UseFastIo ? KEY_USE_FASTIO : 0) |
              (ReturnPending ? KEY_RETURN_PENDING : 0);

    EventHandle = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(EventHandle != NULL, "CreateEvent failed with %lu\n", GetLastError());

    for (StatusKey = KEY_SUCCEED ; StatusKey != 0xff; StatusKey = KEY_NEXT(StatusKey))
    {
        //trace("\tSTATUS KEY: %lx\n", StatusKey);
        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x11);
        Offset.QuadPart = 0;
        Status = NtReadFile(FileHandle,
                            EventHandle,
                            NULL,
                            NULL,
                            &IoStatus,
                            NULL,
                            0,
                            &Offset,
                            &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        if (ReturnPending)
            ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
        else
            ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
        if (Cached && UseFastIo && !ReturnPending)
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, STATUS_BUFFER_OVERFLOW);
            ok_eq_ulongptr(IoStatus.Information, TEST_FILE_SIZE);
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }

        KmtStartSeh()
        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x22);
        Offset.QuadPart = 0;
        Status = NtReadFile(FileHandle,
                            EventHandle,
                            NULL,
                            NULL,
                            &IoStatus,
                            NULL,
                            sizeof(Buffer),
                            &Offset,
                            &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
        ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
        ok_eq_hex(IoStatus.Status, 0x55555555);
        ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        KmtEndSeh(STATUS_SUCCESS);

        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x33);
        Offset.QuadPart = 0;
        Status = NtReadFile(FileHandle,
                            EventHandle,
                            NULL,
                            NULL,
                            &IoStatus,
                            Buffer,
                            0,
                            &Offset,
                            &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        if (ReturnPending)
            ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
        else
            ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
        if (Cached && UseFastIo && !ReturnPending)
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, STATUS_BUFFER_OVERFLOW);
            ok_eq_ulongptr(IoStatus.Information, TEST_FILE_SIZE);
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }
        ok_eq_uint(Buffer[0], 0x55);

        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x44);
        Offset.QuadPart = 0;
        Status = NtReadFile(FileHandle,
                            EventHandle,
                            NULL,
                            NULL,
                            &IoStatus,
                            Buffer,
                            sizeof(Buffer),
                            &Offset,
                            &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_hex(Status, TestGetReturnStatus(StatusKey));
        if ((Cached && UseFastIo && !ReturnPending &&
             (StatusKey == KEY_SUCCEED || StatusKey == KEY_FAIL_OVERFLOW || StatusKey == KEY_FAIL_EOF)) ||
            !KEY_ERROR(StatusKey))
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, TestGetReturnStatus(StatusKey));
            ok_eq_ulongptr(IoStatus.Information, TEST_FILE_SIZE);
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }
        if ((StatusKey != KEY_FAIL_VERIFY_REQUIRED && !KEY_ERROR(StatusKey)) ||
            Cached)
        {
            ok_eq_uint(Buffer[0], 0x44);
            ok_eq_uint(Buffer[TEST_FILE_SIZE - 1], 0x44);
            ok_eq_uint(Buffer[TEST_FILE_SIZE], 0x55);
        }
        else
        {
            ok_eq_uint(Buffer[0], 0x55);
        }
    }
}

static
VOID
TestWrite(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Cached,
    _In_ BOOLEAN UseFastIo,
    _In_ BOOLEAN ReturnPending)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    HANDLE EventHandle;
    UCHAR Buffer[32];
    LARGE_INTEGER Offset;
    ULONG BaseKey, StatusKey, Key;
    DWORD WaitStatus;

    BaseKey = (UseFastIo ? KEY_USE_FASTIO : 0) |
              (ReturnPending ? KEY_RETURN_PENDING : 0);

    EventHandle = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(EventHandle != NULL, "CreateEvent failed with %lu\n", GetLastError());

    for (StatusKey = KEY_SUCCEED ; StatusKey != 0xff; StatusKey = KEY_NEXT(StatusKey))
    {
        //trace("\tSTATUS KEY: %lx\n", StatusKey);
        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x11);
        Offset.QuadPart = 0;
        Status = NtWriteFile(FileHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             NULL,
                             0,
                             &Offset,
                             &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_hex(Status, TestGetReturnStatus(StatusKey));
        if (!KEY_ERROR(StatusKey))
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, TestGetReturnStatus(StatusKey));
            ok_eq_ulongptr(IoStatus.Information, 0);
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }

        KmtStartSeh()
        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x22);
        Offset.QuadPart = 0;
        Status = NtWriteFile(FileHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             NULL,
                             sizeof(Buffer),
                             &Offset,
                             &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
        ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
        ok_eq_hex(IoStatus.Status, 0x55555555);
        ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        KmtEndSeh(STATUS_SUCCESS);

        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
        Key = BaseKey | StatusKey | KEY_DATA(0x33);
        Offset.QuadPart = 0;
        Status = NtWriteFile(FileHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             Buffer,
                             0,
                             &Offset,
                             &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_hex(Status, TestGetReturnStatus(StatusKey));
        if (!KEY_ERROR(StatusKey))
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, TestGetReturnStatus(StatusKey));
            ok_eq_ulongptr(IoStatus.Information, 0);
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }

        ResetEvent(EventHandle);
        RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
        RtlFillMemory(Buffer, sizeof(Buffer), 0x44);
        Key = BaseKey | StatusKey | KEY_DATA(0x44);
        Offset.QuadPart = 0;
        Status = NtWriteFile(FileHandle,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatus,
                             Buffer,
                             sizeof(Buffer),
                             &Offset,
                             &Key);
        WaitStatus = WaitForSingleObject(EventHandle, 0);
        ok_eq_hex(Status, TestGetReturnStatus(StatusKey));
        if (!KEY_ERROR(StatusKey))
        {
            ok_eq_ulong(WaitStatus, WAIT_OBJECT_0);
            ok_eq_hex(IoStatus.Status, TestGetReturnStatus(StatusKey));
            ok_eq_ulongptr(IoStatus.Information, sizeof(Buffer));
        }
        else
        {
            ok_eq_ulong(WaitStatus, WAIT_TIMEOUT);
            ok_eq_hex(IoStatus.Status, 0x55555555);
            ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0x5555555555555555);
        }
    }
}

START_TEST(IoReadWrite)
{
    HANDLE FileHandle;
    UNICODE_STRING CachedFileName = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-IoReadWrite\\Cached");
    UNICODE_STRING NonCachedFileName = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-IoReadWrite\\NonCached");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    KmtLoadDriver(L"IoReadWrite", FALSE);
    KmtOpenDriver();

    RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
    InitializeObjectAttributes(&ObjectAttributes,
                               &NonCachedFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &IoStatus,
                        0,
                        FILE_NON_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "No file\n"))
    {
        ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
        ok_eq_ulongptr(IoStatus.Information, FILE_OPENED);
        trace("Non-Cached read, no FastIo, direct return\n");
        TestRead(FileHandle, FALSE, FALSE, FALSE);
        trace("Non-Cached read, allow FastIo, direct return\n");
        TestRead(FileHandle, FALSE, TRUE, FALSE);
        trace("Non-Cached read, no FastIo, pending return\n");
        TestRead(FileHandle, FALSE, FALSE, TRUE);
        trace("Non-Cached read, allow FastIo, pending return\n");
        TestRead(FileHandle, FALSE, TRUE, TRUE);

        trace("Non-Cached write, no FastIo, direct return\n");
        TestWrite(FileHandle, FALSE, FALSE, FALSE);
        trace("Non-Cached write, allow FastIo, direct return\n");
        TestWrite(FileHandle, FALSE, TRUE, FALSE);
        trace("Non-Cached write, no FastIo, pending return\n");
        TestWrite(FileHandle, FALSE, FALSE, TRUE);
        trace("Non-Cached write, allow FastIo, pending return\n");
        TestWrite(FileHandle, FALSE, TRUE, TRUE);
        NtClose(FileHandle);
    }

    RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
    InitializeObjectAttributes(&ObjectAttributes,
                               &CachedFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        FILE_ALL_ACCESS,
                        &ObjectAttributes,
                        &IoStatus,
                        0,
                        FILE_NON_DIRECTORY_FILE |
                        FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "No file\n"))
    {
        ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
        ok_eq_ulongptr(IoStatus.Information, FILE_OPENED);
        trace("Cached read, no FastIo, direct return\n");
        TestRead(FileHandle, TRUE, FALSE, FALSE);
        trace("Cached read, allow FastIo, direct return\n");
        TestRead(FileHandle, TRUE, TRUE, FALSE);
        trace("Cached read, no FastIo, pending return\n");
        TestRead(FileHandle, TRUE, FALSE, TRUE);
        trace("Cached read, allow FastIo, pending return\n");
        TestRead(FileHandle, TRUE, TRUE, TRUE);

        trace("Cached write, no FastIo, direct return\n");
        TestWrite(FileHandle, TRUE, FALSE, FALSE);
        trace("Cached write, allow FastIo, direct return\n");
        TestWrite(FileHandle, TRUE, TRUE, FALSE);
        trace("Cached write, no FastIo, pending return\n");
        TestWrite(FileHandle, TRUE, FALSE, TRUE);
        trace("Cached write, allow FastIo, pending return\n");
        TestWrite(FileHandle, TRUE, TRUE, TRUE);
        NtClose(FileHandle);
    }

    KmtCloseDriver();
    KmtUnloadDriver();
}
