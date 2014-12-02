/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         User mode part of the TcpIp.sys test suite
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>

#include "tcpip.h"

static
void
LoadTcpIpTestDriver(void)
{
    /* Start the special-purpose driver */
    KmtLoadDriver(L"TcpIp", FALSE);
    KmtOpenDriver();
}

static
void
UnloadTcpIpTestDriver(void)
{
    /* Stop the driver. */
    KmtCloseDriver();
    KmtUnloadDriver();
}

START_TEST(TcpIpTdi)
{
    LoadTcpIpTestDriver();

    ok(KmtSendToDriver(IOCTL_TEST_TDI) == ERROR_SUCCESS, "\n");

    UnloadTcpIpTestDriver();
}
