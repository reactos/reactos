/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for FsRtlRemoveDotsFromPath
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define InitConstString(s, c)                    \
    wcscpy(s.Buffer, c);                         \
    s.Buffer[sizeof(c) / sizeof(WCHAR) - 1] = 0; \
    s.Length = sizeof(c) - sizeof(UNICODE_NULL)

NTSTATUS NTAPI FsRtlRemoveDotsFromPath(PWSTR OriginalString,
                                       USHORT PathLength, USHORT *NewLength);

static
NTSTATUS
(NTAPI *pFsRtlRemoveDotsFromPath)(PWSTR OriginalString,
                                  USHORT PathLength, USHORT *NewLength);

START_TEST(FsRtlRemoveDotsFromPath)
{
    WCHAR Buf[255];
    UNICODE_STRING TestString;
    NTSTATUS Status;

    TestString.Buffer = Buf;
    TestString.MaximumLength = sizeof(Buf);
    KmtGetSystemOrEmbeddedRoutineAddress(FsRtlRemoveDotsFromPath);
    ASSERT(pFsRtlRemoveDotsFromPath);

    InitConstString(TestString, L"\\..");
    Status = pFsRtlRemoveDotsFromPath(TestString.Buffer, TestString.Length, &TestString.Length);
    ok_eq_hex(Status, STATUS_IO_REPARSE_DATA_INVALID);

    InitConstString(TestString, L"..");
    Status = pFsRtlRemoveDotsFromPath(TestString.Buffer, TestString.Length, &TestString.Length);
    ok_eq_hex(Status, STATUS_IO_REPARSE_DATA_INVALID);

    InitConstString(TestString, L"..\\anyOtherContent");
    Status = pFsRtlRemoveDotsFromPath(TestString.Buffer, TestString.Length, &TestString.Length);
    ok_eq_hex(Status, STATUS_IO_REPARSE_DATA_INVALID);

    InitConstString(TestString, L"\\\\..");
    Status = pFsRtlRemoveDotsFromPath(TestString.Buffer, TestString.Length, &TestString.Length);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_wstr(TestString.Buffer, L"\\");

    InitConstString(TestString, L"\\dir1\\dir2\\..\\dir3\\.\\file.txt");
    Status = pFsRtlRemoveDotsFromPath(TestString.Buffer, TestString.Length, &TestString.Length);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_wstr(TestString.Buffer, L"\\dir1\\dir3\\file.txt");
}

