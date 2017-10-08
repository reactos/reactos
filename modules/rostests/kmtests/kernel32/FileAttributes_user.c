/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for GetFileAttributes/SetFileAttributes
 * COPYRIGHT:   Copyright 2017 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2017 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

    /* Set invalid attributes */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_VALID_SET_FLAGS);
    Ret = SetFileAttributesW(FileName, INVALID_FILE_ATTRIBUTES);
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

    /* Query invalid attributes */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, INVALID_FILE_ATTRIBUTES);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, INVALID_FILE_ATTRIBUTES);

    /** Directory attribute **/
    /* Set read-only and directory attribute */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY);
    Ret = SetFileAttributesW(FileName, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Set normal and directory attribute */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
    Ret = SetFileAttributesW(FileName, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Set directory attribute */
    KmtSendUlongToDriver(IOCTL_EXPECT_SET_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL);
    Ret = SetFileAttributesW(FileName, FILE_ATTRIBUTE_DIRECTORY);
    ok(Ret == TRUE, "SetFileAttributesW returned %d, error %lu\n", Ret, GetLastError());

    /* Query read-only and directory attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY);

    /* Query read-only + normal + directory attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);

    /* Query normal and directory attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);

    /* Query directory attribute */
    KmtSendUlongToDriver(IOCTL_RETURN_QUERY_ATTRIBUTES, FILE_ATTRIBUTE_DIRECTORY);
    Attributes = GetFileAttributesW(FileName);
    ok_eq_hex(Attributes, FILE_ATTRIBUTE_DIRECTORY);

    KmtCloseDriver();
    KmtUnloadDriver();
}
