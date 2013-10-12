/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite user-mode test list
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_FindFile;
KMT_TESTFUNC Test_IoDeviceObject;
KMT_TESTFUNC Test_RtlAvlTree;
KMT_TESTFUNC Test_RtlException;
KMT_TESTFUNC Test_RtlMemory;
KMT_TESTFUNC Test_RtlRegistry;
KMT_TESTFUNC Test_RtlSplayTree;
KMT_TESTFUNC Test_RtlUnicodeString;

/* tests with a leading '-' will not be listed */
const KMT_TEST TestList[] =
{
    { "Example",            Test_Example },
    { "FindFile",           Test_FindFile },
    { "IoDeviceObject",     Test_IoDeviceObject },
    { "RtlAvlTree",         Test_RtlAvlTree },
    { "RtlException",       Test_RtlException },
    { "RtlMemory",          Test_RtlMemory },
    { "RtlRegistry",        Test_RtlRegistry },
    { "RtlSplayTree",       Test_RtlSplayTree },
    { "RtlUnicodeString",   Test_RtlUnicodeString },
    { NULL,                 NULL },
};
