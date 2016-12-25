/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Read/Write operations test declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_IOREADFILE_H_
#define _KMTEST_IOREADFILE_H_

#define TEST_FILE_SIZE 17

#define KEY_SUCCEED                 0x00
#define KEY_SUCCESS_WAIT1           0x01

#define KEY_INFO_EXISTS             0x41

#define KEY_FAIL_MISALIGNED         0x81
#define KEY_FAIL_OVERFLOW           0x82
#define KEY_FAIL_PARTIAL            0x83
#define KEY_FAIL_BUSY               0x84
#define KEY_FAIL_VERIFY_REQUIRED    0x85

#define KEY_FAIL_UNSUCCESSFUL       0xc1
#define KEY_FAIL_NOT_IMPLEMENTED    0xc2
#define KEY_FAIL_ACCESS_VIOLATION   0xc3
#define KEY_FAIL_IN_PAGE_ERROR      0xc4
#define KEY_FAIL_EOF                0xc5
#define KEY_FAIL_ACCESS_DENIED      0xc6
#define KEY_FAIL_MISALIGNED_ERROR   0xc7
#define KEY_RESULT_MASK             0xff

#define KEY_NEXT(key) ( (key) == KEY_FAIL_MISALIGNED_ERROR ? 0xff :                 \
                        (key) == KEY_FAIL_VERIFY_REQUIRED ? KEY_FAIL_UNSUCCESSFUL : \
                        (key) == KEY_INFO_EXISTS ? KEY_FAIL_MISALIGNED :            \
                        (key) == KEY_SUCCESS_WAIT1 ? KEY_INFO_EXISTS :              \
                        (key) + 1 )
#define KEY_ERROR(key) (((key) & 0xc0) == 0xc0)
static
NTSTATUS
TestGetReturnStatus(
    _In_ ULONG LockKey)
{
    switch (LockKey & KEY_RESULT_MASK)
    {
        case KEY_SUCCEED:
            return STATUS_SUCCESS;
        case KEY_SUCCESS_WAIT1:
            return STATUS_WAIT_1;

        case KEY_INFO_EXISTS:
            return STATUS_OBJECT_NAME_EXISTS;

        case KEY_FAIL_MISALIGNED:
            return STATUS_DATATYPE_MISALIGNMENT;
        case KEY_FAIL_OVERFLOW:
            return STATUS_BUFFER_OVERFLOW;
        case KEY_FAIL_PARTIAL:
            return STATUS_PARTIAL_COPY;
        case KEY_FAIL_BUSY:
            return STATUS_DEVICE_BUSY;
        case KEY_FAIL_VERIFY_REQUIRED:
            return STATUS_VERIFY_REQUIRED;

        case KEY_FAIL_UNSUCCESSFUL:
            return STATUS_UNSUCCESSFUL;
        case KEY_FAIL_NOT_IMPLEMENTED:
            return STATUS_NOT_IMPLEMENTED;
        case KEY_FAIL_ACCESS_VIOLATION:
            return STATUS_ACCESS_VIOLATION;
        case KEY_FAIL_IN_PAGE_ERROR:
            return STATUS_IN_PAGE_ERROR;
        case KEY_FAIL_EOF:
            return STATUS_END_OF_FILE;
        case KEY_FAIL_ACCESS_DENIED:
            return STATUS_ACCESS_DENIED;
        case KEY_FAIL_MISALIGNED_ERROR:
            return STATUS_DATATYPE_MISALIGNMENT_ERROR;
        default:
            ok(0, "Key = %lx\n", LockKey);
            return STATUS_INVALID_PARAMETER;
    }
}

#define KEY_USE_FASTIO              0x100
#define KEY_RETURN_PENDING          0x200

#define KEY_DATA(c) (((c) & 0xff) << 24)
#define KEY_GET_DATA(key) ((key) >> 24)

#endif /* !defined _KMTEST_IOREADFILE_H_ */
