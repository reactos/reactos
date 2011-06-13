/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example Test kernel-mode part
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>
#include <kmt_log.h>

VOID Test_Example(VOID)
{
    /* TODO: this should be trace(), as in winetests */
    LogPrint("Message from kernel\n");
}
