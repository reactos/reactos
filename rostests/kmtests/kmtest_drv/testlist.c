/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_ExCallback;
KMT_TESTFUNC Test_ExDoubleList;
KMT_TESTFUNC Test_ExFastMutex;
KMT_TESTFUNC Test_ExHardError;
KMT_TESTFUNC Test_ExHardErrorInteractive;
KMT_TESTFUNC Test_ExInterlocked;
KMT_TESTFUNC Test_ExPools;
KMT_TESTFUNC Test_ExResource;
KMT_TESTFUNC Test_ExSequencedList;
KMT_TESTFUNC Test_ExSingleList;
KMT_TESTFUNC Test_ExTimer;
KMT_TESTFUNC Test_FsRtlExpression;
KMT_TESTFUNC Test_FsRtlMcb;
KMT_TESTFUNC Test_FsRtlTunnel;
KMT_TESTFUNC Test_IoDeviceInterface;
KMT_TESTFUNC Test_IoEvent;
KMT_TESTFUNC Test_IoInterrupt;
KMT_TESTFUNC Test_IoIrp;
KMT_TESTFUNC Test_IoMdl;
KMT_TESTFUNC Test_KeApc;
KMT_TESTFUNC Test_KeDpc;
KMT_TESTFUNC Test_KeEvent;
KMT_TESTFUNC Test_KeGuardedMutex;
KMT_TESTFUNC Test_KeIrql;
KMT_TESTFUNC Test_KeMutex;
KMT_TESTFUNC Test_KeProcessor;
KMT_TESTFUNC Test_KeTimer;
KMT_TESTFUNC Test_KernelType;
KMT_TESTFUNC Test_MmSection;
KMT_TESTFUNC Test_ObReference;
KMT_TESTFUNC Test_ObType;
KMT_TESTFUNC Test_ObTypeClean;
KMT_TESTFUNC Test_ObTypeNoClean;
KMT_TESTFUNC Test_ObTypes;
KMT_TESTFUNC Test_PsNotify;
KMT_TESTFUNC Test_RtlAvlTree;
KMT_TESTFUNC Test_RtlException;
KMT_TESTFUNC Test_RtlMemory;
KMT_TESTFUNC Test_RtlSplayTree;
KMT_TESTFUNC Test_ZwAllocateVirtualMemory;
KMT_TESTFUNC Test_ZwCreateSection;
KMT_TESTFUNC Test_ZwMapViewOfSection;

const KMT_TEST TestList[] =
{
    { "ExCallback",                         Test_ExCallback },
    { "ExDoubleList",                       Test_ExDoubleList },
    { "ExFastMutex",                        Test_ExFastMutex },
    { "ExHardError",                        Test_ExHardError },
    { "-ExHardErrorInteractive",            Test_ExHardErrorInteractive },
    { "ExInterlocked",                      Test_ExInterlocked },
    { "ExPools",                            Test_ExPools },
    { "ExResource",                         Test_ExResource },
    { "ExSequencedList",                    Test_ExSequencedList },
    { "ExSingleList",                       Test_ExSingleList },
    { "-ExTimer",                           Test_ExTimer },
    { "Example",                            Test_Example },
    { "FsRtlExpression",                    Test_FsRtlExpression },
    /* Skipped on testman. See ROSTESTS-106. */
    { "-FsRtlMcb",                          Test_FsRtlMcb },
    { "-FsRtlTunnel",                       Test_FsRtlTunnel },
    { "IoDeviceInterface",                  Test_IoDeviceInterface },
    { "IoEvent",                            Test_IoEvent },
    { "IoInterrupt",                        Test_IoInterrupt },
    { "IoIrp",                              Test_IoIrp },
    { "IoMdl",                              Test_IoMdl },
    { "KeApc",                              Test_KeApc },
    { "KeDpc",                              Test_KeDpc },
    { "KeEvent",                            Test_KeEvent },
    { "KeGuardedMutex",                     Test_KeGuardedMutex },
    { "KeIrql",                             Test_KeIrql },
    { "KeMutex",                            Test_KeMutex },
    { "-KeProcessor",                       Test_KeProcessor },
    { "KeTimer",                            Test_KeTimer },
    { "-KernelType",                        Test_KernelType },
    { "MmSection",                          Test_MmSection },
    { "ObReference",                        Test_ObReference },
    { "ObType",                             Test_ObType },
    { "-ObTypeClean",                       Test_ObTypeClean },
    { "-ObTypeNoClean",                     Test_ObTypeNoClean },
    { "ObTypes",                            Test_ObTypes },
    { "PsNotify",                           Test_PsNotify },
    { "RtlAvlTreeKM",                       Test_RtlAvlTree },
    { "RtlExceptionKM",                     Test_RtlException },
    { "RtlMemoryKM",                        Test_RtlMemory },
    { "RtlSplayTreeKM",                     Test_RtlSplayTree },
    { "ZwAllocateVirtualMemory",            Test_ZwAllocateVirtualMemory },
    { "ZwCreateSection",                    Test_ZwCreateSection },
    { "ZwMapViewOfSection",                 Test_ZwMapViewOfSection },
    { NULL,                                 NULL }
};
