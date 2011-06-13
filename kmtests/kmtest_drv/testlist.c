/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>

KMT_TESTFUNC Test_Example;

const KMT_TEST TestList[] =
{
    { "Example", Test_Example },
    { NULL, NULL }
};
