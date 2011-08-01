/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite user-mode test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_RtlMemory;

/* tests with a leading '-' will not be listed */
const KMT_TEST TestList[] =
{
    { "Example",            Test_Example },
    { "RtlMemory",          Test_RtlMemory },
    { NULL,                 NULL },
};
