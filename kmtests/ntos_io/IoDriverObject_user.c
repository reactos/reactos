/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object test user-mode part
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

START_TEST(IoDriverObject)
{
    KmtLoadDriver(L"IoDriverObject", FALSE);
    KmtOpenDriver();
    KmtCloseDriver();
    KmtUnloadDriver();
}
