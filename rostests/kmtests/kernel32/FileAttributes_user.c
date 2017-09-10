/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for GetFileAttributes/SetFileAttributes
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#include "kernel32_test.h"

START_TEST(FileAttributes)
{
    PCWSTR FileName = L"\\\\.\\Global\\GLOBALROOT\\Device\\Kmtest-kernel32\\Somefile";
    BOOL Ret;
    DWORD Attributes;

    KmtLoadDriver(L"kernel32", FALSE);
    KmtOpenDriver();

    /* Set read-only attribute */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL);
    Ret = SetFileAttributesW(FileName, FILE_ATTRIBUTE_READONLY);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Set normal attribute */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
    Ret = SetFileAttributesW(FileName, FILE_ATTRIBUTE_NORMAL);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Set 0 attribute (driver should receive normal) */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
    Ret = SetFileAttributesW(FileName, 0);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Query read-only attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_READONLY);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_READONLY);

    /* Query read-only + normal attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL);

    /* Query normal attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_NORMAL);

    /* Query 0 attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, 0);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, 0);

    KmtCloseDriver();
    KmtUnloadDriver();
}
