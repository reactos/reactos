/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite power IRP management test user-mode part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "PoIrp.h"

START_TEST(PoIrp)
{
    KmtLoadDriver(L"PoIrp", TRUE);
    KmtOpenDriver();
    KmtSendToDriver(IOCTL_RUN_TEST);
    KmtCloseDriver();
    KmtUnloadDriver();
}
