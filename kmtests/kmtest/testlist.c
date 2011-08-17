/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite user-mode test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_IoDeviceObject;
KMT_TESTFUNC Test_RtlAvlTree;
KMT_TESTFUNC Test_RtlMemory;
KMT_TESTFUNC Test_RtlSplayTree;

/* tests with a leading '-' will not be listed */
const KMT_TEST TestList[] =
{
    { "Example",            Test_Example },
    { "IoDeviceObject",     Test_IoDeviceObject },
    { "RtlAvlTree",         Test_RtlAvlTree },
    { "RtlMemory",          Test_RtlMemory },
    { "RtlSplayTree",       Test_RtlSplayTree },
    { NULL,                 NULL },
};
